#include <Runtime/Core/Platform/File.h>
#include <Runtime/Core/Platform/Compiler.h>
#include <Runtime/Core/Memory/Memory.h>
#include <Runtime/Core/Systems.h>
#include <Runtime/Core/Logging/LoggingSystem.h>
#include <Runtime/EngineContext.h>
#include <Runtime/Graphics/GPUContext.h>
#include <Runtime/Graphics/GPUDevice.h>
#include <Runtime/Graphics/Base/GPUBuffer.h>
#include <Runtime/Graphics/Base/RenderGeometry.h>
#include <Runtime/Graphics/Base/VertexFactoryLayout.h>
#include <Runtime/Graphics/GlobalSettings_GPU.h>
#include <Runtime/Graphics/Shaders/Config.h>
#include <Runtime/Graphics/Shaders/SLC2GPUShader.h>
#include <Runtime/Graphics/Shaders/SLC2VertexInputLayout.h>
#include <Runtime/Graphics/Shaders/ShaderProgramInstance.h>
#include <Runtime/Graphics/Textures/GPUTexture.h>
#include <Runtime/Resource/Factories/BinaryAssetFactory.h>
#include <Runtime/ShaderCompilation/ShadersCompilation.h>
#include <Runtime/ShaderCompilation/Slang/SLC2/SLC2Reader.h>
#include <Runtime/Render/Assets/Geometry/Model.h>
#include <Runtime/Render/Assets/Geometry/MeshDataLayout.h>
#include <Runtime/Render/Assets/Geometry/StaticMeshVertexFactory.h>

namespace SE
{
	class TestSystem : public ISystem
	{
		ENGINE_SYSTEM(TestSystem)

	public:
		TestSystem()
			: ISystem(SE_TEXT("TestSystem"), 1000)
		{
		}

		void OnUpdate() override
		{
			if (_hasRun)
			{
				if (_attributesDownloadTask != nullptr && _attributesDownloadTask->IsEnded() &&
					(_attributesDownloadTask->GetContinueWithTask()->IsEnded()))
				{
					ValidateStaticMeshPackedData();
					_finishedAttributesDownloadTask = _attributesDownloadTask;
					_attributesDownloadTask = nullptr;
					WriteTestLog();
				}
				if (++_testLogFrame == 120)
				{
					WriteTestLog();
				}
				return;
			}

			_hasRun = true;
			RunVertexFactoryContractTest();
			RunStaticMeshVertexFactoryTest();
			RunSLC2VertexInputLayoutMatchingTest();
			RunSLC2VertexInputRoundTripTest();
			RunSlangVariantCacheGenerationTest();
			RunSLC2IndexedRenderGeometryDrawTest();
			RunSLC2AlternateLayoutPipelineTest();
			RunSLC2NonIndexedSparseSlotDrawTest();
			RunSLC2ComputeDispatchSmokeTest();
			WriteTestLog();
		}

		void OnDispose() override
		{
			if (_attributesDownloadTask != nullptr)
			{
				_attributesDownloadTask->Cancel();
			}
			if (_finishedAttributesDownloadTask != nullptr)
			{
				Delete(_finishedAttributesDownloadTask->GetContinueWithTask());
				Delete(_finishedAttributesDownloadTask);
				_finishedAttributesDownloadTask = nullptr;
			}
			if (_meshTestModel != nullptr)
			{
				Delete(_meshTestModel);
				_meshTestModel = nullptr;
			}
			Delete(_systemOnlyPipeline);
			_systemOnlyPipeline = nullptr;
			Delete(_graphicsPipeline);
			_graphicsPipeline = nullptr;
			Delete(_graphicsVertexBuffer);
			_graphicsVertexBuffer = nullptr;
			Delete(_graphicsIndexBuffer);
			_graphicsIndexBuffer = nullptr;
			Delete(_alternateVertexBuffer);
			_alternateVertexBuffer = nullptr;
			Delete(_graphicsTarget);
			_graphicsTarget = nullptr;
			Delete(_sparseUVBuffer);
			_sparseUVBuffer = nullptr;
			Delete(_sparsePipeline);
			_sparsePipeline = nullptr;
			Delete(_sparseShader);
			_sparseShader = nullptr;
			Delete(_graphicsShader);
			_graphicsShader = nullptr;
			if (_shader != nullptr)
			{
				Delete(_shader);
				_shader = nullptr;
			}
		}

	private:
		bool RejectsVertexFactoryLayout(
			const List<VertexFactoryBufferBinding>& bindings,
			const List<VertexFactoryInputElement>& elements) const
		{
			VertexFactoryLayout layout;
			return !VertexFactoryLayout::Create(bindings, elements, layout);
		}

		void RunVertexFactoryContractTest()
		{
			const List<VertexFactoryBufferBinding> bindings = {
				{1, 16, VertexFactoryInputRate::PerVertex, 0},
				{0, 12, VertexFactoryInputRate::PerVertex, 0},
			};
			const List<VertexFactoryInputElement> elements = {
				{SE_TEXT("normal"), 0, PixelFormat::R8G8B8A8_SNorm, 1, 0},
				{SE_TEXT("position"), 0, PixelFormat::R32G32B32_Float, 0, 0},
				{SE_TEXT("texcoord"), 0, PixelFormat::R16G16_Float, 1, 4},
			};

			VertexFactoryLayout layout;
			if (!VertexFactoryLayout::Create(bindings, elements, layout) ||
				!layout.IsValid() || layout.GetBindings().Count() != 2 || layout.GetElements().Count() != 3 ||
				layout.GetBindings()[0].Slot != 0 || layout.GetElements()[0].Semantic != SE_TEXT("POSITION") ||
				layout.FindElement(SE_TEXT("PoSiTiOn"), 0) == nullptr)
			{
				LOG_ERROR("Test", "VertexFactory layout normalization failed");
				return;
			}

			// 输入顺序和 Semantic 大小写不属于物理布局身份。
			const List<VertexFactoryBufferBinding> reorderedBindings = {
				{0, 12, VertexFactoryInputRate::PerVertex, 0},
				{1, 16, VertexFactoryInputRate::PerVertex, 0},
			};
			const List<VertexFactoryInputElement> reorderedElements = {
				{SE_TEXT("TEXCOORD"), 0, PixelFormat::R16G16_Float, 1, 4},
				{SE_TEXT("POSITION"), 0, PixelFormat::R32G32B32_Float, 0, 0},
				{SE_TEXT("NORMAL"), 0, PixelFormat::R8G8B8A8_SNorm, 1, 0},
			};
			VertexFactoryLayout reorderedLayout;
			if (!VertexFactoryLayout::Create(reorderedBindings, reorderedElements, reorderedLayout) ||
				reorderedLayout.GetHash() != layout.GetHash())
			{
				LOG_ERROR("Test", "VertexFactory layout hash is not stable across construction order.");
				return;
			}

			List<VertexFactoryBufferBinding> changedBindings = reorderedBindings;
			changedBindings[1].Stride = 20;
			VertexFactoryLayout changedLayout;
			if (!VertexFactoryLayout::Create(changedBindings, reorderedElements, changedLayout) ||
				changedLayout.GetHash() == layout.GetHash())
			{
				LOG_ERROR("Test", "VertexFactory layout hash ignored a physical binding change.");
				return;
			}
			VertexFactoryLayout instanceLayout;
			if (!VertexFactoryLayout::Create(
					{{3, 16, VertexFactoryInputRate::PerInstance, 1}},
					{{SE_TEXT("INSTANCE_DATA"), 0, PixelFormat::R32G32B32A32_Float, 3, 0}},
					instanceLayout))
			{
				LOG_ERROR("Test", "Valid per-instance VertexFactory layout was rejected.");
				return;
			}

			List<VertexFactoryInputElement> tooManyElements;
			for (uint32 elementIndex = 0; elementIndex <= VERTEX_SHADER_MAX_INPUT_ELEMENTS; elementIndex++)
			{
				tooManyElements.Add({SE_TEXT("ATTRIBUTE"), elementIndex, PixelFormat::R32_Float, 0, 0});
			}

			// 通过公共 Create 入口覆盖布局的主要安全边界和诊断输出
            LOG_INFO("Test", "报错正常 ////////////////////////////////////////////////////////.");

            bool rejects1 = !RejectsVertexFactoryLayout(
                {{0, 12, VertexFactoryInputRate::PerVertex, 0}, {0, 16, VertexFactoryInputRate::PerVertex, 0}}, {});

            bool rejects2 = !RejectsVertexFactoryLayout(bindings,
                                                        {{SE_TEXT("POSITION"), 0, PixelFormat::R32G32B32_Float, 0, 0},
                                                         {SE_TEXT("position"), 0, PixelFormat::R16G16_Float, 1, 0}});
            bool rejects3 = !RejectsVertexFactoryLayout({{0, 12, VertexFactoryInputRate::PerVertex, 0}},
                                                        {{SE_TEXT("POSITION"), 0, PixelFormat::R32G32B32_Float, 1, 0}});

            bool rejects4 = !RejectsVertexFactoryLayout({{0, 12, VertexFactoryInputRate::PerVertex, 0}},
                                                        {{SE_TEXT("POSITION"), 0, PixelFormat::R32G32B32_Float, 0, 4}});

			bool rejects5 = !RejectsVertexFactoryLayout({{0, 16, VertexFactoryInputRate::PerVertex, 0}},
                                                        {{SE_TEXT("POSITION"), 0, PixelFormat::R32G32B32_Float, 0, 0},
                                                         {SE_TEXT("TEXCOORD"), 0, PixelFormat::R16G16_Float, 0, 8}});

            bool rejects6 = !RejectsVertexFactoryLayout({{0, 12, VertexFactoryInputRate::PerVertex, 1}}, {});

			bool rejects7 = !RejectsVertexFactoryLayout({{0, 12, VertexFactoryInputRate::PerInstance, 0}}, {});

			bool rejects8 = !RejectsVertexFactoryLayout({{0, 4, VertexFactoryInputRate::PerVertex, 0}},
                                                        {{SE_TEXT("POSITION"), 0, PixelFormat::D32_Float, 0, 0}});

			bool rejects9 = !RejectsVertexFactoryLayout({{GPU_MAX_VB_BINDED, 12, VertexFactoryInputRate::PerVertex, 0}}, {});

			bool rejects10 = !RejectsVertexFactoryLayout({{0, 4, VertexFactoryInputRate::PerVertex, 0},
                                                          {1, 4, VertexFactoryInputRate::PerVertex, 0},
                                                          {2, 4, VertexFactoryInputRate::PerVertex, 0},
                                                          {3, 4, VertexFactoryInputRate::PerVertex, 0},
                                                          {4, 4, VertexFactoryInputRate::PerVertex, 0}},
                                                         {});
            bool rejects11 = !RejectsVertexFactoryLayout({{0, 4, VertexFactoryInputRate::PerVertex, 0}}, tooManyElements);

			LOG_INFO("Test", "////////////////////////////////////////////////////////.");

			if (rejects1 || rejects2 || rejects3 || rejects4 ||
				rejects5 || rejects6 || rejects7 || rejects8 ||
				rejects9 || rejects10 || rejects11 ||
				VertexFactoryLayout::IsVertexFormatSupported(PixelFormat::R8G8B8A8_UNorm_SRGB) ||
				VertexFactoryLayout::IsVertexFormatSupported(PixelFormat::BC1_UNorm) ||
				VertexFactoryLayout::IsVertexFormatSupported(PixelFormat::NV12) ||
				VertexFactoryLayout::IsVertexFormatSupported(PixelFormat::R9G9B9E5_SHAREDEXP))
			{
				LOG_ERROR("Test", "VertexFactory layout accepted an invalid public description.");
				return;
			}

			VertexFactoryLayout emptyLayout;
			if (!VertexFactoryLayout::Create({}, {}, emptyLayout))
			{
				LOG_ERROR("Test", "Empty VertexFactory layout creation failed.");
				return;
			}
			RenderGeometry geometry;
			geometry.Layout = &emptyLayout;
			if (!geometry.Validate(false) || geometry.Validate(true))
			{
				LOG_ERROR("Test", "RenderGeometry indexed/non-indexed validation is inconsistent.");
				return;
			}

			geometry.IndexOffset = 2;
			geometry.IndexFormat = PixelFormat::R16_UInt;
			if (geometry.Validate(false))
			{
				LOG_ERROR("Test", "RenderGeometry accepted index metadata without an index buffer.");
			}
		}

		bool MatchesStaticMeshBinding(
			const VertexFactoryLayout& layout,
			const uint32 slot,
			const uint32 stride) const
		{
			const VertexFactoryBufferBinding* binding = layout.FindBinding(slot);
			return binding != nullptr &&
				binding->Stride == stride &&
				binding->InputRate == VertexFactoryInputRate::PerVertex &&
				binding->InstanceStepRate == 0;
		}

		bool MatchesStaticMeshElement(
			const VertexFactoryLayout& layout,
			const StringView& semantic,
			const uint32 semanticIndex,
			const PixelFormat format,
			const uint32 slot,
			const uint32 offset) const
		{
			const VertexFactoryInputElement* element = layout.FindElement(semantic, semanticIndex);
			return element != nullptr &&
				element->Format == format &&
				element->Slot == slot &&
				element->Offset == offset;
		}

		void RunStaticMeshVertexFactoryTest()
		{
			const VertexFactoryLayout& layout = StaticMeshVertexFactory::GetLayout();
			if (!layout.IsValid() || layout.GetBindings().Count() != 3 || layout.GetElements().Count() != 6 ||
				!MatchesStaticMeshBinding(layout, StaticMeshVertexFactory::PositionSlot, sizeof(VB0ElementType)) ||
				!MatchesStaticMeshBinding(layout, StaticMeshVertexFactory::AttributesSlot, sizeof(VB1ElementType)) ||
				!MatchesStaticMeshBinding(layout, StaticMeshVertexFactory::ColorSlot, sizeof(VB2ElementType)) ||
				!MatchesStaticMeshElement(layout, SE_TEXT("POSITION"), 0, PixelFormat::R32G32B32_Float,
					StaticMeshVertexFactory::PositionSlot, OFFSET_OF(VB0ElementType, Position)) ||
				!MatchesStaticMeshElement(layout, SE_TEXT("TEXCOORD"), 0, PixelFormat::R16G16_Float,
					StaticMeshVertexFactory::AttributesSlot, OFFSET_OF(VB1ElementType, TexCoord)) ||
				!MatchesStaticMeshElement(layout, SE_TEXT("NORMAL"), 0, PixelFormat::R10G10B10A2_UNorm,
					StaticMeshVertexFactory::AttributesSlot, OFFSET_OF(VB1ElementType, Normal)) ||
				!MatchesStaticMeshElement(layout, SE_TEXT("TANGENT"), 0, PixelFormat::R10G10B10A2_UNorm,
					StaticMeshVertexFactory::AttributesSlot, OFFSET_OF(VB1ElementType, Tangent)) ||
				!MatchesStaticMeshElement(layout, SE_TEXT("TEXCOORD"), 1, PixelFormat::R16G16_Float,
					StaticMeshVertexFactory::AttributesSlot, OFFSET_OF(VB1ElementType, LightmapUVs)) ||
				!MatchesStaticMeshElement(layout, SE_TEXT("COLOR"), 0, PixelFormat::R8G8B8A8_UNorm,
					StaticMeshVertexFactory::ColorSlot, OFFSET_OF(VB2ElementType, Color)))
			{
				LOG_ERROR("Test", "StaticMesh VertexFactory layout does not match MeshDataLayout.");
				return;
			}

			VB0ElementType positionData[3] = {};
			const GPUBufferDescription positionDescription = StaticMeshVertexFactory::CreateVertexBufferDescription(
				StaticMeshVertexStream::Position,
				3,
				positionData);
			if (positionDescription.Stride != sizeof(VB0ElementType) ||
				positionDescription.Size != sizeof(positionData) ||
				positionDescription.InitData != positionData ||
				!positionDescription.Flags.IsFlag(GPUBufferFlags::VertexBuffer))
			{
				LOG_ERROR("Test", "StaticMesh VertexFactory produced an invalid vertex buffer description.");
				return;
			}

			// 通过公开 Model/Mesh 接口验证 VertexFactory 布局确实被用于 RenderGeometry。
			AssetInfo info;
			info.id = UID::New();
			info.typeID = Typeof<Model>();
			BinaryAssetFactory<Model, true> modelFactory;
			_meshTestModel = static_cast<Model*>(modelFactory.NewVirtual(&info));
			if (_meshTestModel == nullptr)
			{
				LOG_ERROR("Test", "Failed to create a virtual Model for StaticMesh RenderGeometry validation.");
				return;
			}
			_meshTestModel->InitAsVirtual();
			int32 meshCount = 1;
			if (_meshTestModel->SetupLODs(ToSpan(&meshCount, 1)))
			{
				LOG_ERROR("Test", "Failed to setup the virtual Model LOD for StaticMesh RenderGeometry validation.");
				return;
			}

			Mesh& mesh = _meshTestModel->LODs[0].Meshes[0];
			const Float3 meshVertices[3] = {
				Float3(-0.5f, -0.5f, 0.0f),
				Float3(0.0f, 0.5f, 0.0f),
				Float3(0.5f, -0.5f, 0.0f),
			};
			const uint16 meshIndices[3] = {0, 1, 2};
			if (mesh.UpdateMesh(ARRAY_SIZE(meshVertices), 1, meshVertices, meshIndices, nullptr, nullptr, nullptr, nullptr))
			{
				LOG_ERROR("Test", "Virtual Mesh packing failed for a StaticMesh RenderGeometry test.");
				return;
			}

			const RenderGeometry geometry = mesh.GetRenderGeometry();
			const RenderGeometryVertexBuffer* positionBuffer = geometry.FindVertexBuffer(StaticMeshVertexFactory::PositionSlot);
			const RenderGeometryVertexBuffer* attributesBuffer = geometry.FindVertexBuffer(StaticMeshVertexFactory::AttributesSlot);
			if (geometry.Layout != &layout || positionBuffer == nullptr || attributesBuffer == nullptr ||
				geometry.FindVertexBuffer(StaticMeshVertexFactory::ColorSlot) != nullptr ||
				positionBuffer->Offset != 0 || attributesBuffer->Offset != 0 ||
				positionBuffer->Buffer->GetStride() != sizeof(VB0ElementType) ||
				attributesBuffer->Buffer->GetStride() != sizeof(VB1ElementType) ||
				positionBuffer->Buffer->GetSize() != sizeof(meshVertices) ||
				attributesBuffer->Buffer->GetSize() != sizeof(VB1ElementType) * ARRAY_SIZE(meshVertices) ||
				geometry.IndexBuffer == nullptr || geometry.IndexFormat != PixelFormat::R16_UInt || geometry.IndexOffset != 0)
			{
				LOG_ERROR("Test", "Mesh RenderGeometry does not match the StaticMesh VertexFactory contract.");
				return;
			}

			// 缺失的法线、切线和 UV 在资产打包阶段生成，读回属性流验证默认值没有被 Draw 阶段替代。
			_attributesDownloadTask = mesh.DownloadDataGPUAsync(MeshBufferType::Vertex1, _attributesData);
			if (_attributesDownloadTask == nullptr)
			{
				LOG_ERROR("Test", "Failed to create GPU readback task for packed StaticMesh attributes.");
				return;
			}
			_attributesDownloadTask->Start();
			LOG_INFO("Test", "StaticMesh RenderGeometry test passed; packed attribute readback is pending.");
		}

		void ValidateStaticMeshPackedData()
		{
			if (_attributesDownloadTask->GetState() != Threading::TaskState::Finished ||
				_attributesDownloadTask->GetContinueWithTask()->GetState() != Threading::TaskState::Finished ||
				_attributesData.Length() != sizeof(VB1ElementType) * 3)
			{
				LOG_ERROR("Test", "Packed StaticMesh attribute data could not be read back.");
				return;
			}
			const VB1ElementType* packedAttributes = reinterpret_cast<const VB1ElementType*>(_attributesData.Get());
			for (uint32 index = 0; index < 3; index++)
			{
				const VB1ElementType& attribute = packedAttributes[index];
				if (!Float3::NearEqual(attribute.Normal.ToFloat3(), Float3::UnitZ, 0.01f) ||
					!Float3::NearEqual(attribute.Tangent.ToFloat3(), Float3::UnitX, 0.01f) ||
					!Float2::NearEqual(attribute.TexCoord.ToFloat2(), Float2::Zero, 0.01f) ||
					!Float2::NearEqual(attribute.LightmapUVs.ToFloat2(), Float2::Zero, 0.01f))
				{
					LOG_ERROR("Test", "StaticMesh missing-attribute defaults were not packed as expected at vertex {0}.", index);
					return;
				}
			}

			LOG_INFO("Test", "StaticMesh VertexFactory layout, RenderGeometry and packed default data test passed.");
		}

		void WriteTestLog() const
		{
			// 单独保存测试运行的日志快照，不改变应用的全局日志路径。
			String output;
            List<Log::LogEntry> const &entries = Log::System::GetLogEntries();
			for (const Log::LogEntry& entry : entries)
			{
				output += String::Format(SE_TEXT("[{0}] [Severity={1}] [{2}] {3}\r\n"),
					entry.timestamp, static_cast<int32>(entry.severity), entry.category, entry.message);
			}
			File::WriteAllText(EngineContext::StartupFolder + SE_TEXT("/VertexFactoryTest.log"), output, Encoding::EncodingType::Unicode);
		}

		void RunSLC2AlternateLayoutPipelineTest()
		{
			if (GPUDevice::instance == nullptr || GPUDevice::instance->GetMainContext() == nullptr ||
				_graphicsShader == nullptr || _graphicsPipeline == nullptr || _graphicsTarget == nullptr)
			{
				LOG_ERROR("Test", "SLC2 alternate-layout pipeline test requires the indexed test resources.");
				return;
			}

			// 同一逻辑 Signature 使用不同的物理 Slot、Stride 和 Offset，必须生成独立 Vertex Input State。
			VertexFactoryLayout alternateLayout;
			if (!VertexFactoryLayout::Create(
					{{1, 24, VertexFactoryInputRate::PerVertex, 0}},
					{{SE_TEXT("POSITION"), 0, PixelFormat::R32G32B32_Float, 1, 8}},
					alternateLayout) || alternateLayout.GetHash() == StaticMeshVertexFactory::GetLayout().GetHash())
			{
				LOG_ERROR("Test", "Failed to create a distinct alternate VertexFactory layout.");
				return;
			}

			const Float3 positions[3] = {
				Float3(-0.5f, -0.5f, 0.0f),
				Float3(0.0f, 0.5f, 0.0f),
				Float3(0.5f, -0.5f, 0.0f),
			};
			List<byte> alternateData;
			alternateData.Resize(24 * ARRAY_SIZE(positions));
			Platform::MemoryClear(alternateData.Get(), alternateData.Count());
			for (uint32 index = 0; index < ARRAY_SIZE(positions); index++)
			{
				Platform::MemoryCopy(alternateData.Get() + index * 24 + 8, &positions[index], sizeof(Float3));
			}
			_alternateVertexBuffer = GPUDevice::instance->CreateBuffer(SE_TEXT("SLC2AlternateLayoutPipelineTest.VB"));
			if (_alternateVertexBuffer == nullptr ||
				!_alternateVertexBuffer->Init(GPUBufferDescription::Vertex(24, ARRAY_SIZE(positions), alternateData.Get())))
			{
				LOG_ERROR("Test", "Failed to create the alternate-layout vertex buffer.");
				return;
			}

			RenderGeometry alternateGeometry;
			alternateGeometry.Layout = &alternateLayout;
			alternateGeometry.VertexBuffers.Add({1, _alternateVertexBuffer, 0});
			GPUContext* context = GPUDevice::instance->GetMainContext();
			context->SetSLC2State(_graphicsPipeline);
			if (!context->BindRenderGeometry(alternateGeometry))
			{
				LOG_ERROR("Test", "SLC2 pipeline rejected the alternate physical VertexFactory layout.");
				return;
			}
			context->SetRenderTarget(_graphicsTarget->View());
			context->SetViewportAndScissors(4.0f, 4.0f);
			context->DrawInstanced(_graphicsInstance, ARRAY_SIZE(positions), 1, 0, 0);

			RenderGeometry staticGeometry = StaticMeshVertexFactory::CreateRenderGeometry(
				_graphicsVertexBuffer, nullptr, nullptr);
			if (!context->BindRenderGeometry(staticGeometry))
			{
				LOG_ERROR("Test", "SLC2 pipeline failed to restore the StaticMesh physical layout.");
				return;
			}
			context->DrawInstanced(_graphicsInstance, ARRAY_SIZE(positions), 1, 0, 0);
			LOG_INFO("Test", "SLC2 alternate VertexFactory layout created and used an independent pipeline state.");
		}

		void RunSLC2VertexInputLayoutMatchingTest()
		{
			const List<VertexFactoryBufferBinding> bindings = {
				{0, 12, VertexFactoryInputRate::PerVertex, 0},
				{1, 16, VertexFactoryInputRate::PerVertex, 0},
				{3, 8, VertexFactoryInputRate::PerVertex, 0},
			};
			const List<VertexFactoryInputElement> elements = {
				{SE_TEXT("POSITION"), 0, PixelFormat::R32G32B32_Float, 0, 0},
				{SE_TEXT("NORMAL"), 0, PixelFormat::R10G10B10A2_UNorm, 1, 0},
				{SE_TEXT("TEXCOORD"), 0, PixelFormat::R16G16_Float, 3, 0},
			};
			VertexFactoryLayout layout;
			if (!VertexFactoryLayout::Create(bindings, elements, layout))
			{
				LOG_ERROR("Test", "Failed to create SLC2 vertex input matching layout.");
				return;
			}

			const List<SLC2VertexInputSignatureElement> signature = {
				{SE_TEXT("POSITION"), 0, 0, SLC2VertexInputType::Float3},
				{SE_TEXT("TEXCOORD"), 0, 4, SLC2VertexInputType::Float2},
			};
			SLC2VertexInputMatchResult match;
			String error;
			if (!SLC2VertexInputLayoutMatcher::Match(signature, layout, match, error) ||
				match.Elements.Count() != 2 || match.RequiredVertexBufferSlotsMask != ((1u << 0) | (1u << 3)) ||
				!SLC2VertexInputLayoutMatcher::IsCompatible(SLC2VertexInputType::Float3, PixelFormat::R10G10B10A2_UNorm) ||
				SLC2VertexInputLayoutMatcher::IsCompatible(SLC2VertexInputType::UInt, PixelFormat::R32_Float))
			{
				LOG_ERROR("Test", "SLC2 vertex input matching failed: {0}", error);
				return;
			}

			// 相同逻辑签名可以匹配不同的物理 Slot、Stride 和 Offset，证明 Shader 不依赖具体 VertexFactory。
			VertexFactoryLayout alternateLayout;
			if (!VertexFactoryLayout::Create(
					{{1, 24, VertexFactoryInputRate::PerVertex, 0},
					 {2, 16, VertexFactoryInputRate::PerVertex, 0}},
					{{SE_TEXT("POSITION"), 0, PixelFormat::R32G32B32_Float, 1, 8},
					 {SE_TEXT("TEXCOORD"), 0, PixelFormat::R16G16_Float, 2, 0}},
					alternateLayout) ||
				!SLC2VertexInputLayoutMatcher::Match(signature, alternateLayout, match, error) ||
				match.RequiredVertexBufferSlotsMask != ((1u << 1) | (1u << 2)) ||
				match.Elements.Count() != signature.Count() ||
				match.Elements[0]->Slot != 1 || match.Elements[0]->Offset != 8 ||
				match.Elements[1]->Slot != 2 || match.Elements[1]->Offset != 0)
			{
				LOG_ERROR("Test", "SLC2 vertex input matcher rejected an alternate physical layout: {0}", error);
				return;
			}

			SLC2VertexInputSignatureElement missing = {SE_TEXT("TANGENT"), 0, 5, SLC2VertexInputType::Float3};
			List<SLC2VertexInputSignatureElement> missingSignature = signature;
			missingSignature.Add(missing);
			if (SLC2VertexInputLayoutMatcher::Match(missingSignature, layout, match, error) || error.IsEmpty())
			{
				LOG_ERROR("Test", "SLC2 vertex input matcher accepted a missing semantic.");
				return;
			}

			VertexFactoryLayout incompatibleLayout;
			if (!VertexFactoryLayout::Create(
					{{0, 4, VertexFactoryInputRate::PerVertex, 0}},
					{{SE_TEXT("POSITION"), 0, PixelFormat::R8G8_UNorm, 0, 0}},
					incompatibleLayout) ||
				SLC2VertexInputLayoutMatcher::Match(
					{signature[0]}, incompatibleLayout, match, error))
			{
				LOG_ERROR("Test", "SLC2 vertex input matcher accepted an incompatible format.");
				return;
			}

			LOG_INFO("Test", "SLC2 VertexInputSignature and VertexFactoryLayout matching test passed.");
		}

		bool ReaderRejectsRenamedField(const List<byte>& data, const char* field, const char* replacement) const
		{
			StringAnsi mutated(reinterpret_cast<const char*>(data.Get()), data.Count());
			if (mutated.Replace(field, replacement) <= 0)
			{
				return false;
			}

			SLC2Artifact artifact;
			String error;
			return !SLC2Reader::Read(reinterpret_cast<const byte*>(mutated.Get()), mutated.Length(), artifact, error);
		}

		const SLC2ProgramRecord* FindProgram(const SLC2Artifact& artifact, const String& programId) const
		{
			for (int32 programIndex = 0; programIndex < artifact.Programs.Count(); programIndex++)
			{
				if (artifact.Programs[programIndex].ProgramId == programId)
				{
					return &artifact.Programs[programIndex];
				}
			}
			return nullptr;
		}

		const SLC2StageRecord* FindStage(const SLC2VariantRecord& variant, const ShaderStage stage) const
		{
			for (int32 stageIndex = 0; stageIndex < variant.Stages.Count(); stageIndex++)
			{
				if (variant.Stages[stageIndex].Stage == stage)
				{
					return &variant.Stages[stageIndex];
				}
			}
			return nullptr;
		}

		bool ValidateVertexProgram(const SLC2ProgramRecord& program, const bool expectInput) const
		{
			const int32 expectedCount = expectInput ? 1 : 0;
			for (int32 targetIndex = 0; targetIndex < program.Targets.Count(); targetIndex++)
			{
				const SLC2TargetRecord& target = program.Targets[targetIndex];
				for (int32 variantIndex = 0; variantIndex < target.Variants.Count(); variantIndex++)
				{
					const SLC2StageRecord* vertexStage = FindStage(target.Variants[variantIndex], ShaderStage::Vertex);
					if (vertexStage == nullptr || vertexStage->VertexInputSignature.Count() != expectedCount)
					{
						return false;
					}
					if (expectInput)
					{
						const SLC2VertexInputSignatureElement& input = vertexStage->VertexInputSignature[0];
						if (input.Semantic != SE_TEXT("POSITION") || input.SemanticIndex != 0 ||
							input.Location != 0 || input.ShaderType != SLC2VertexInputType::Float3)
						{
							return false;
						}
					}
				}
			}
			return true;
		}

		void RunSLC2VertexInputRoundTripTest()
		{
			ShaderCompileRequest request;
			request.ShaderName = SE_TEXT("VertexInputMinimal");
			request.SourcePath = SE_TEXT("Shaders/Slang/CompileOnly_VertexInputMinimal.slang");
			request.Targets = {
				{ShaderTargetPlatform::Windows, ShaderProfile::Vulkan_SM5, FeatureLevel::SM5},
			};

			if (!File::ReadAllText(
					EngineContext::StartupFolder + SE_TEXT("/Shaders/Slang/CompileOnly_VertexInputMinimal.slang"),
					request.SourceCode))
			{
				LOG_ERROR("Test", "Failed to read minimal SLC2 vertex input shader source.");
				return;
			}

			ShaderCompileResult result = ShadersCompilation::CompileSlang(request);
			if (result.Status == ShaderCompileStatus::Failed)
			{
				LOG_ERROR("Test", "{0}", result.CompileMessage.Text);
				return;
			}

			// 通过共享 Reader 重新读取编译产物，验证运行时实际消费的数据，而不是编译器临时结构。
			SLC2Artifact artifact;
			String error;
			if (!SLC2Reader::Read(result.SLC2Data, artifact, error) || artifact.Version != 4)
			{
				LOG_ERROR("Test", "SLC2 v4 vertex input round-trip failed: {0}", error);
				return;
			}

			const SLC2ProgramRecord* positionProgram =
				FindProgram(artifact, SE_TEXT("CompileOnlyVertexInputMinimal"));
			const SLC2ProgramRecord* systemOnlyProgram = FindProgram(artifact, SE_TEXT("CompileOnlyVertexSystemOnly"));
			if (positionProgram == nullptr || systemOnlyProgram == nullptr ||
				!ValidateVertexProgram(*positionProgram, true) || !ValidateVertexProgram(*systemOnlyProgram, false))
			{
				LOG_ERROR("Test", "SLC2 v4 vertex input content does not match the logical signature contract.");
				return;
			}

			// 新缓存不再包含 Program 级物理布局；Vertex Stage 签名字段仍必须存在。
			const StringAnsi serialized(reinterpret_cast<const char*>(result.SLC2Data.Get()), result.SLC2Data.Count());
			if (serialized.Contains("vertexBufferLayout") ||
				!ReaderRejectsRenamedField(result.SLC2Data, "vertexInputSignature", "missingVertexInputSignature"))
			{
				LOG_ERROR("Test", "SLC2 v4 vertex input fields are invalid.");
				return;
			}
			LOG_INFO("Test", "SLC2 v4 logical vertex signature round-trip test passed.");
		}

		void RunSlangVariantCacheGenerationTest()
		{
			ShaderCompileRequest request;
			request.ShaderName = SE_TEXT("TestShader");
			request.SourcePath = SE_TEXT("Shaders/Slang/CompileOnly_Variants.slang");
			request.Targets = {
				{ShaderTargetPlatform::Windows, ShaderProfile::Vulkan_SM5, FeatureLevel::SM5},
			};

			if (!File::ReadAllText(EngineContext::StartupFolder + SE_TEXT("/Shaders/Slang/CompileOnly_Variants.slang"), request.SourceCode))
			{
				LOG_ERROR("Test", "Failed to read shader source code.");
				return;
			}

			ShaderCompileResult result = ShadersCompilation::CompileSlang(request);
			if (result.Status == ShaderCompileStatus::Failed)
			{
				LOG_ERROR("Test", "{0}", result.CompileMessage.Text);
				return;
			}

			SLC2Artifact artifact;
			String error;
			const StringAnsi serialized(reinterpret_cast<const char*>(result.SLC2Data.Get()), result.SLC2Data.Count());
			if (!SLC2Reader::Read(result.SLC2Data, artifact, error) ||
				serialized.Contains("vertexBufferLayout"))
			{
				LOG_ERROR("Test", "SLC2 variant cache does not satisfy the v4 logical signature contract: {0}", error);
				return;
			}
			const SLC2ProgramRecord* program = FindProgram(artifact, SE_TEXT("CompileOnlyVariants"));
			if (program == nullptr || program->Targets.Count() != 1 || program->Targets[0].Variants.Count() != 4 ||
				!ValidateVertexProgram(*program, true))
			{
				LOG_ERROR("Test", "SLC2 variant vertex signatures are missing or unstable.");
				return;
			}

			File::WriteAllBytes(EngineContext::StartupFolder + SE_TEXT("/Shaders/Slang/CompileOnly_Variants.slang.slangcache"), result.SLC2Data.Get(), result.SLC2Data.Count());
			LOG_INFO("Test", "SLC2 v4 multi-variant logical vertex signature test passed.");
		}

		void RunSLC2IndexedRenderGeometryDrawTest()
		{
			if (GPUDevice::instance == nullptr || GPUDevice::instance->GetMainContext() == nullptr)
			{
				LOG_ERROR("Test", "SLC2 indexed RenderGeometry test requires a GPU device and main context.");
				return;
			}

			ShaderCompileRequest request;
			request.ShaderName = SE_TEXT("IndexedRenderGeometryShader");
			request.SourcePath = SE_TEXT("Shaders/Slang/CompileOnly_VertexInputMinimal.slang");
			request.Targets = {
				{ShaderTargetPlatform::Windows, ShaderProfile::Vulkan_SM5, FeatureLevel::SM5},
			};
			if (!File::ReadAllText(
					EngineContext::StartupFolder + SE_TEXT("/Shaders/Slang/CompileOnly_VertexInputMinimal.slang"),
					request.SourceCode))
			{
				LOG_ERROR("Test", "Failed to read SLC2 indexed RenderGeometry shader source.");
				return;
			}

			ShaderCompileResult result = ShadersCompilation::CompileSlang(request);
			if (result.Status == ShaderCompileStatus::Failed)
			{
				LOG_ERROR("Test", "{0}", result.CompileMessage.Text);
				return;
			}

			_graphicsShader = GPUDevice::instance->CreateSLC2Shader(SE_TEXT("SLC2IndexedRenderGeometryTest"));
			if (_graphicsShader == nullptr || !_graphicsShader->Load(result.SLC2Data))
			{
				LOG_ERROR("Test", "Failed to load the SLC2 indexed RenderGeometry shader.");
				return;
			}

			ShaderProgramSelection selection;
			selection.ProgramId = SE_TEXT("CompileOnlyVertexInputMinimal");
			selection.Target = {ShaderTargetPlatform::Windows, ShaderProfile::Vulkan_SM5, FeatureLevel::SM5};
			if (!_graphicsShader->CreateProgramInstance(selection, _graphicsInstance))
			{
				LOG_ERROR("Test", "Failed to create the SLC2 indexed RenderGeometry program instance.");
				return;
			}

			_graphicsPipeline = SLC2GPUPipelineState::New();
			if (_graphicsPipeline == nullptr ||
				!_graphicsPipeline->Init(SLC2GPUPipelineState::Description::DefaultNoDepth, _graphicsInstance.GetProgram()))
			{
				LOG_ERROR("Test", "Failed to initialize the SLC2 indexed RenderGeometry pipeline state.");
				return;
			}

			const VB0ElementType vertices[3] = {
				{Float3(-0.5f, -0.5f, 0.0f)},
				{Float3(0.0f, 0.5f, 0.0f)},
				{Float3(0.5f, -0.5f, 0.0f)},
			};
			const uint16 indices[3] = {0, 1, 2};
			_graphicsVertexBuffer = GPUDevice::instance->CreateBuffer(SE_TEXT("SLC2IndexedRenderGeometryTest.VB"));
			_graphicsIndexBuffer = GPUDevice::instance->CreateBuffer(SE_TEXT("SLC2IndexedRenderGeometryTest.IB"));
			if (_graphicsVertexBuffer == nullptr || _graphicsIndexBuffer == nullptr ||
				!_graphicsVertexBuffer->Init(StaticMeshVertexFactory::CreateVertexBufferDescription(
					StaticMeshVertexStream::Position, ARRAY_SIZE(vertices), vertices)) ||
				!_graphicsIndexBuffer->Init(GPUBufferDescription::Index(sizeof(uint16), ARRAY_SIZE(indices), indices)))
			{
				LOG_ERROR("Test", "Failed to create SLC2 indexed RenderGeometry buffers.");
				return;
			}

			RenderGeometry geometry = StaticMeshVertexFactory::CreateRenderGeometry(
				_graphicsVertexBuffer, nullptr, nullptr);
			geometry.IndexBuffer = _graphicsIndexBuffer;
			geometry.IndexFormat = PixelFormat::R16_UInt;

			GPUContext* context = GPUDevice::instance->GetMainContext();
			context->SetSLC2State(_graphicsPipeline);
			RenderGeometry missingPosition = StaticMeshVertexFactory::CreateRenderGeometry(nullptr, nullptr, nullptr);
			missingPosition.IndexBuffer = _graphicsIndexBuffer;
			missingPosition.IndexFormat = PixelFormat::R16_UInt;
			if (context->BindRenderGeometry(missingPosition))
			{
				LOG_ERROR("Test", "BindRenderGeometry accepted a missing required POSITION buffer.");
				return;
			}
			// 绑定失败必须清除之前的 Geometry 状态，失败后立即 Draw 不应提交 Vulkan 命令。
			context->DrawIndexedInstanced(_graphicsInstance, ARRAY_SIZE(indices), 1, 0, 0, 0);
			if (!context->BindRenderGeometry(geometry))
			{
				LOG_ERROR("Test", "BindRenderGeometry rejected a valid StaticMesh geometry.");
				return;
			}

			// Geometry 接口允许暂时不带 IB，以便复用给非索引 Draw；索引 Draw 必须在提交前拒绝该状态。
			RenderGeometry missingIndex = geometry;
			missingIndex.IndexBuffer = nullptr;
			missingIndex.IndexFormat = PixelFormat::Undefined;
			if (!context->BindRenderGeometry(missingIndex))
			{
				LOG_ERROR("Test", "BindRenderGeometry rejected a valid non-indexed geometry representation.");
				return;
			}
			context->DrawIndexedInstanced(_graphicsInstance, ARRAY_SIZE(indices), 1, 0, 0, 0);

			RenderGeometry invalidFormat = geometry;
			invalidFormat.IndexFormat = PixelFormat::R8_UInt;
			if (context->BindRenderGeometry(invalidFormat))
			{
				LOG_ERROR("Test", "BindRenderGeometry accepted an invalid index format.");
				return;
			}

			RenderGeometry invalidOffset = geometry;
			invalidOffset.IndexOffset = 1;
			if (context->BindRenderGeometry(invalidOffset))
			{
				LOG_ERROR("Test", "BindRenderGeometry accepted an unaligned index offset.");
				return;
			}

			if (!context->BindRenderGeometry(geometry))
			{
				LOG_ERROR("Test", "BindRenderGeometry failed while restoring a valid geometry.");
				return;
			}
			// 索引数量超过 Buffer 可访问范围时，Draw 必须在 vkCmdDrawIndexed 前返回。
			context->DrawIndexedInstanced(_graphicsInstance, ARRAY_SIZE(indices) + 1, 1, 0, 0, 0);

			_graphicsTarget = GPUDevice::instance->CreateTexture(SE_TEXT("SLC2IndexedRenderGeometryTest.RT"));
			if (_graphicsTarget == nullptr ||
				!_graphicsTarget->Init(GPUTextureDescription::New2D(4, 4, PixelFormat::R8G8B8A8_UNorm)))
			{
				LOG_ERROR("Test", "Failed to create SLC2 indexed RenderGeometry render target.");
				return;
			}

			context->SetRenderTarget(_graphicsTarget->View());
			context->SetViewportAndScissors(4.0f, 4.0f);
			// 重新绑定合法 Geometry 后才允许提交一次真实的索引 Draw。
			const RenderGeometry meshGeometry = _meshTestModel != nullptr && _meshTestModel->LODs[0].Meshes[0].IsInitialized()
				? _meshTestModel->LODs[0].Meshes[0].GetRenderGeometry() : geometry;
			if (!context->BindRenderGeometry(meshGeometry))
			{
				LOG_ERROR("Test", "BindRenderGeometry failed before the valid indexed draw.");
				return;
			}
			context->DrawIndexedInstanced(_graphicsInstance, ARRAY_SIZE(indices), 1, 0, 0, 0);
			LOG_INFO("Test", "SLC2 indexed RenderGeometry draw command was recorded.");
		}

		void RunSLC2NonIndexedSparseSlotDrawTest()
		{
			if (GPUDevice::instance == nullptr || GPUDevice::instance->GetMainContext() == nullptr ||
				_graphicsShader == nullptr || _graphicsVertexBuffer == nullptr || _graphicsTarget == nullptr)
			{
				LOG_ERROR("Test", "SLC2 non-indexed RenderGeometry test requires the indexed test resources.");
				return;
			}

			GPUContext* context = GPUDevice::instance->GetMainContext();

			// system-value-only 顶点着色器允许空签名和空 Vertex Buffer 集合。
			ShaderProgramSelection systemOnlySelection;
			systemOnlySelection.ProgramId = SE_TEXT("CompileOnlyVertexSystemOnly");
			systemOnlySelection.Target = {ShaderTargetPlatform::Windows, ShaderProfile::Vulkan_SM5, FeatureLevel::SM5};
			ShaderProgramInstance systemOnlyInstance;
			if (!_graphicsShader->CreateProgramInstance(systemOnlySelection, systemOnlyInstance))
			{
				LOG_ERROR("Test", "Failed to create the system-only non-indexed shader program instance.");
				return;
			}
			_systemOnlyPipeline = SLC2GPUPipelineState::New();
			if (_systemOnlyPipeline == nullptr ||
				!_systemOnlyPipeline->Init(SLC2GPUPipelineState::Description::DefaultNoDepth, systemOnlyInstance.GetProgram()))
			{
				LOG_ERROR("Test", "Failed to initialize the system-only non-indexed pipeline state.");
				return;
			}
			VertexFactoryLayout emptyLayout;
			if (!VertexFactoryLayout::Create({}, {}, emptyLayout))
			{
				LOG_ERROR("Test", "Failed to create an empty VertexFactory layout for system-only Draw.");
				return;
			}
			RenderGeometry emptyGeometry;
			emptyGeometry.Layout = &emptyLayout;
			context->SetSLC2State(_systemOnlyPipeline);
			if (!context->BindRenderGeometry(emptyGeometry))
			{
				LOG_ERROR("Test", "BindRenderGeometry rejected empty system-only geometry.");
				return;
			}
			context->SetRenderTarget(_graphicsTarget->View());
			context->SetViewportAndScissors(4.0f, 4.0f);
			context->DrawInstanced(systemOnlyInstance, 3, 1, 0, 0);

			// 普通非索引 Draw 允许没有 Index Buffer，并按当前 Signature 只需要 POSITION Slot。
			RenderGeometry positionOnly = StaticMeshVertexFactory::CreateRenderGeometry(
				_graphicsVertexBuffer, nullptr, nullptr);
			context->SetSLC2State(_graphicsPipeline);
			if (!context->BindRenderGeometry(positionOnly))
			{
				LOG_ERROR("Test", "BindRenderGeometry rejected a valid non-indexed single-slot geometry.");
				return;
			}
			context->DrawInstanced(_graphicsInstance, 3, 1, 0, 0);

			// Required PerVertex Slot 没有完整元素时仍必须校验，不能因元素数为零跳过。
			RenderGeometry exhaustedPosition = positionOnly;
			exhaustedPosition.VertexBuffers[0].Offset = _graphicsVertexBuffer->GetSize();
			if (!context->BindRenderGeometry(exhaustedPosition))
			{
				LOG_ERROR("Test", "BindRenderGeometry rejected a valid end-offset representation.");
				return;
			}
			context->DrawInstanced(_graphicsInstance, 1, 1, 0, 0);
			if (!context->BindRenderGeometry(positionOnly))
			{
				LOG_ERROR("Test", "Failed to restore non-indexed geometry after the zero-element range test.");
				return;
			}

			RenderGeometry missingPosition = positionOnly;
			missingPosition.VertexBuffers.Clear();
			if (context->BindRenderGeometry(missingPosition))
			{
				LOG_ERROR("Test", "BindRenderGeometry accepted a non-indexed geometry missing POSITION.");
				return;
			}
			context->DrawInstanced(_graphicsInstance, 3, 1, 0, 0);

			ShaderCompileRequest request;
			request.ShaderName = SE_TEXT("SparseVertexInputShader");
			request.SourcePath = SE_TEXT("Shaders/Slang/CompileOnly_SparseVertexInput.slang");
			request.Targets = {
				{ShaderTargetPlatform::Windows, ShaderProfile::Vulkan_SM5, FeatureLevel::SM5},
			};
			if (!File::ReadAllText(
					EngineContext::StartupFolder + SE_TEXT("/Shaders/Slang/CompileOnly_SparseVertexInput.slang"),
					request.SourceCode))
			{
				LOG_ERROR("Test", "Failed to read the sparse-slot shader source.");
				return;
			}
			ShaderCompileResult result = ShadersCompilation::CompileSlang(request);
			if (result.Status == ShaderCompileStatus::Failed)
			{
				LOG_ERROR("Test", "{0}", result.CompileMessage.Text);
				return;
			}
			_sparseShader = GPUDevice::instance->CreateSLC2Shader(SE_TEXT("SLC2SparseSlotRenderGeometryTest"));
			if (_sparseShader == nullptr || !_sparseShader->Load(result.SLC2Data))
			{
				LOG_ERROR("Test", "Failed to load the sparse-slot SLC2 shader.");
				return;
			}
			ShaderProgramSelection sparseSelection;
			sparseSelection.ProgramId = SE_TEXT("CompileOnlySparseVertexInput");
			sparseSelection.Target = {ShaderTargetPlatform::Windows, ShaderProfile::Vulkan_SM5, FeatureLevel::SM5};
			if (!_sparseShader->CreateProgramInstance(sparseSelection, _sparseInstance))
			{
				LOG_ERROR("Test", "Failed to create the sparse-slot shader program instance.");
				return;
			}
			_sparsePipeline = SLC2GPUPipelineState::New();
			if (_sparsePipeline == nullptr ||
				!_sparsePipeline->Init(SLC2GPUPipelineState::Description::DefaultNoDepth, _sparseInstance.GetProgram()))
			{
				LOG_ERROR("Test", "Failed to initialize the sparse-slot pipeline state.");
				return;
			}

			const VB1ElementType texcoords[3] = {};
			_sparseUVBuffer = GPUDevice::instance->CreateBuffer(SE_TEXT("SLC2SparseSlotRenderGeometryTest.UV"));
			if (_sparseUVBuffer == nullptr ||
				!_sparseUVBuffer->Init(GPUBufferDescription::Vertex(sizeof(VB1ElementType), ARRAY_SIZE(texcoords), texcoords)))
			{
				LOG_ERROR("Test", "Failed to create the sparse-slot vertex buffer.");
				return;
			}

			VertexFactoryLayout sparseLayout;
			if (!VertexFactoryLayout::Create(
					{{0, sizeof(VB0ElementType), VertexFactoryInputRate::PerVertex, 0},
					 {3, sizeof(VB1ElementType), VertexFactoryInputRate::PerVertex, 0}},
					{{SE_TEXT("POSITION"), 0, PixelFormat::R32G32B32_Float, 0, 0},
					 {SE_TEXT("TEXCOORD"), 0, PixelFormat::R16G16_Float, 3, 0}},
					sparseLayout))
			{
				LOG_ERROR("Test", "Failed to create the sparse Slot 0/3 VertexFactory layout.");
				return;
			}
			RenderGeometry sparseGeometry;
			sparseGeometry.Layout = &sparseLayout;
			sparseGeometry.VertexBuffers.Add({0, _graphicsVertexBuffer, 0});
			sparseGeometry.VertexBuffers.Add({3, _sparseUVBuffer, 0});
			context->SetSLC2State(_sparsePipeline);
			if (!context->BindRenderGeometry(sparseGeometry))
			{
				LOG_ERROR("Test", "BindRenderGeometry rejected valid sparse Slot 0/3 geometry.");
				return;
			}
			context->DrawInstanced(_sparseInstance, 3, 1, 0, 0);

			RenderGeometry missingSparseSlot = sparseGeometry;
			missingSparseSlot.VertexBuffers.RemoveAt(1);
			if (context->BindRenderGeometry(missingSparseSlot))
			{
				LOG_ERROR("Test", "BindRenderGeometry accepted sparse geometry missing required Slot 3.");
				return;
			}
			context->DrawInstanced(_sparseInstance, 3, 1, 0, 0);

			VertexFactoryLayout extraLayout;
			if (!VertexFactoryLayout::Create(
					{{0, sizeof(VB0ElementType), VertexFactoryInputRate::PerVertex, 0},
					 {3, sizeof(VB1ElementType), VertexFactoryInputRate::PerVertex, 0},
					 {2, sizeof(VB1ElementType), VertexFactoryInputRate::PerVertex, 0}},
					{{SE_TEXT("POSITION"), 0, PixelFormat::R32G32B32_Float, 0, 0},
					 {SE_TEXT("TEXCOORD"), 0, PixelFormat::R16G16_Float, 3, 0},
					 {SE_TEXT("COLOR"), 0, PixelFormat::R8G8B8A8_UNorm, 2, 0}},
					extraLayout))
			{
				LOG_ERROR("Test", "Failed to create the extra-slot VertexFactory layout.");
				return;
			}
			RenderGeometry extraSlotGeometry = sparseGeometry;
			extraSlotGeometry.Layout = &extraLayout;
			extraSlotGeometry.VertexBuffers.Add({2, _sparseUVBuffer, 0});
			if (!context->BindRenderGeometry(extraSlotGeometry))
			{
				LOG_ERROR("Test", "BindRenderGeometry rejected geometry with an unused extra Slot 2.");
				return;
			}
			context->DrawInstanced(_sparseInstance, 3, 1, 0, 0);

			// 顶点范围检查在提交 vkCmdDraw 前拒绝越界参数。
			if (!context->BindRenderGeometry(sparseGeometry))
			{
				LOG_ERROR("Test", "BindRenderGeometry failed while restoring sparse geometry.");
				return;
			}
			context->DrawInstanced(_sparseInstance, ARRAY_SIZE(texcoords) + 1, 1, 0, 0);
			LOG_INFO("Test", "SLC2 non-indexed and sparse Slot RenderGeometry test passed.");
			Delete(_systemOnlyPipeline);
			_systemOnlyPipeline = nullptr;
		}

		void RunSLC2ComputeDispatchSmokeTest()
		{
			if (GPUDevice::instance == nullptr)
			{
				LOG_ERROR("Test", "SLC2 runtime smoke test failed. GPU device is null.");
				return;
			}

			GPUContext* context = GPUDevice::instance->GetMainContext();
			if (context == nullptr)
			{
				LOG_ERROR("Test", "SLC2 runtime smoke test failed. GPU main context is null.");
				return;
			}

			ShaderCompileRequest request;
            request.ShaderName = SE_TEXT("TestCSShader");
            request.SourcePath = SE_TEXT("Shaders/Slang/CompileOnly_MinimalCS.slang");
            request.Targets    = {
                {ShaderTargetPlatform::Windows, ShaderProfile::Vulkan_SM5, FeatureLevel::SM5},
            };

            if (!File::ReadAllText(EngineContext::StartupFolder + SE_TEXT("/Shaders/Slang/CompileOnly_MinimalCS.slang"),
                                   request.SourceCode))
            {
                LOG_ERROR("Test", "Failed to read shader source code.");
                return;
            }

            ShaderCompileResult result = ShadersCompilation::CompileSlang(request);
            if (result.Status == ShaderCompileStatus::Failed)
            {
                LOG_ERROR("Test", "{0}", result.CompileMessage.Text);
				return;
			}

			SLC2Artifact artifact;
			String error;
			if (!SLC2Reader::Read(result.SLC2Data, artifact, error))
			{
				LOG_ERROR("Test", "SLC2 compute cache read failed: {0}", error);
				return;
			}
			const SLC2ProgramRecord* computeProgram = FindProgram(artifact, SE_TEXT("CompileOnlyMinimalCS"));
			if (computeProgram == nullptr)
			{
				LOG_ERROR("Test", "SLC2 compute program is missing.");
				return;
			}
			_shader = GPUDevice::instance->CreateSLC2Shader(SE_TEXT("SLC2RuntimeSmokeTest"));
			if (_shader == nullptr)
			{
				LOG_ERROR("Test", "SLC2 runtime smoke test failed. Cannot create SLC2 GPU shader.");
				return;
			}

			if (!_shader->Load(result.SLC2Data))
			{
				LOG_ERROR("Test", "SLC2 runtime smoke test failed. Cannot load SLC2 cache.");
				return;
			}

			ShaderProgramSelection selection;
			selection.ProgramId = SE_TEXT("CompileOnlyMinimalCS");
			selection.Target = { ShaderTargetPlatform::Windows, ShaderProfile::Vulkan_SM5, FeatureLevel::SM5 };

			if (!_shader->CreateProgramInstance(selection, _instance))
			{
				LOG_ERROR("Test", "SLC2 runtime smoke test failed. Cannot create shader program instance");
				return;
			}

			_instance.GetRootVar().SetUniform(SE_TEXT("g_TestValue"), float(42.0f));

			context->Dispatch(_instance, 1, 1, 1);
		}

	private:
		bool _hasRun = false;
		uint32 _testLogFrame = 0;
		SLC2GPUShader* _shader = nullptr;
		ShaderProgramInstance _instance;
		SLC2GPUShader* _graphicsShader = nullptr;
		ShaderProgramInstance _graphicsInstance;
		SLC2GPUPipelineState* _graphicsPipeline = nullptr;
		GPUBuffer* _graphicsVertexBuffer = nullptr;
		GPUBuffer* _graphicsIndexBuffer = nullptr;
		GPUBuffer* _alternateVertexBuffer = nullptr;
		GPUTexture* _graphicsTarget = nullptr;
		Model* _meshTestModel = nullptr;
		BytesContainer _attributesData;
		Threading::Task* _attributesDownloadTask = nullptr;
		Threading::Task* _finishedAttributesDownloadTask = nullptr;
		SLC2GPUPipelineState* _systemOnlyPipeline = nullptr;
		SLC2GPUShader* _sparseShader = nullptr;
		ShaderProgramInstance _sparseInstance;
		SLC2GPUPipelineState* _sparsePipeline = nullptr;
		GPUBuffer* _sparseUVBuffer = nullptr;
	};

	ENGINE_SYSTEM_REGISTER(TestSystem)
}
