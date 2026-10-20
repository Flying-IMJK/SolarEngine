#include <Runtime/Core/Platform/File.h>
#include <Runtime/Core/Platform/FileSystem.h>
#include <Runtime/Core/Platform/Compiler.h>
#include <Runtime/Core/Memory/Memory.h>
#include <Runtime/Core/Math/Matrix.h>
#include <Runtime/Core/Systems.h>
#include <Runtime/Core/Logging/LoggingSystem.h>
#include <Runtime/Core/Serialization/MemoryReadStream.h>
#include <Runtime/Core/Serialization/MemoryWriteStream.h>
#include <Runtime/Utilities/Variant.h>
#include <Runtime/EngineContext.h>
#include <Runtime/Graphics/GPUContext.h>
#include <Runtime/Graphics/GPUDevice.h>
#include <Runtime/Graphics/Base/GPUBuffer.h>
#include <Runtime/Graphics/Base/RenderGeometry.h>
#include <Runtime/Graphics/Base/VertexFactoryLayout.h>
#include <Runtime/Graphics/GlobalSettings_GPU.h>
#include <Runtime/Graphics/Shaders/Config.h>
#include <Runtime/Graphics/Shaders/GPUConstantBuffer.h>
#include <Runtime/Graphics/Shaders/SLC2GPUShader.h>
#include <Runtime/Graphics/Shaders/SLC2GPUShaderProgram.h>
#include <Runtime/Graphics/Shaders/SLC2VertexInputLayout.h>
#include <Runtime/Graphics/Shaders/ShaderProgramInstance.h>
#include <Runtime/Graphics/Shaders/Reflection/ShaderNameResolver.h>
#include <Runtime/Graphics/Shaders/Reflection/ShaderProgramReflection.h>
#include <Runtime/Graphics/Textures/GPUTexture.h>
#include <Runtime/Graphics/Textures/GPUSampler.h>
#include <Runtime/Resource/Factories/BinaryAssetFactory.h>
#include <Runtime/Resource/AssetContent.h>
#include <Runtime/Resource/AssetRef.h>
#include <Runtime/Resource/Assets/SLC2Materials/GraphicShader.h>
#include <Runtime/Resource/Assets/SLC2Materials/SLC2Material.h>
#include <Runtime/ShaderCompilation/ShadersCompilation.h>
#include <Runtime/ShaderCompilation/Slang/SLC2/SLC2Reader.h>
#include <Runtime/Render/Assets/Geometry/Model.h>
#include <Runtime/Render/Assets/Geometry/MeshDataLayout.h>
#include <Runtime/Render/Assets/Geometry/StaticMeshVertexFactory.h>
#include <Runtime/Render/Assets/Material/MaterialRuntime.h>

namespace SE
{
	class TestGPUConstantBuffer final : public GPUConstantBuffer
	{
	public:
		explicit TestGPUConstantBuffer(const uint32 size)
		{
			m_Size = size;
		}
	};

	class MaterialRuntimeTestProvider final : public IMaterialRuntimeBindingProvider
	{
	public:
		void SetResources(GPUTextureView* texture, GPUSampler* sampler)
		{
			m_Texture = texture;
			m_Sampler = sampler;
		}

		bool SupportsBinding(const MaterialRuntimeBindingSource source, const MaterialParameterType parameterType, const ShaderProgramBinding& binding) const override
		{
			switch (source)
			{
			case MaterialRuntimeBindingSource::PerView:
			case MaterialRuntimeBindingSource::PerDraw:
				return parameterType == MaterialParameterType::Invalid && binding.Kind == ShaderProgramBindingKind::Uniform;
			case MaterialRuntimeBindingSource::DefaultResource:
				return binding.Kind == ShaderProgramBindingKind::Texture &&
					(parameterType == MaterialParameterType::Texture || parameterType == MaterialParameterType::CubeTexture ||
					 parameterType == MaterialParameterType::NormalMap || parameterType == MaterialParameterType::GPUTexture ||
					 parameterType == MaterialParameterType::GPUTextureArray || parameterType == MaterialParameterType::GPUTextureVolume ||
					 parameterType == MaterialParameterType::GPUTextureCube);
			case MaterialRuntimeBindingSource::SceneTexture:
				return parameterType == MaterialParameterType::SceneTexture && binding.Kind == ShaderProgramBindingKind::Texture;
			case MaterialRuntimeBindingSource::TextureGroupSampler:
				return parameterType == MaterialParameterType::TextureGroupSampler && binding.Kind == ShaderProgramBindingKind::Sampler;
			case MaterialRuntimeBindingSource::GameplayGlobal:
				return parameterType == MaterialParameterType::GameplayGlobal && binding.Kind == ShaderProgramBindingKind::Uniform;
			case MaterialRuntimeBindingSource::GlobalSDF:
				return parameterType == MaterialParameterType::GlobalSDF &&
					(binding.Kind == ShaderProgramBindingKind::Texture || binding.Kind == ShaderProgramBindingKind::Uniform);
			default:
				return false;
			}
		}

		bool WriteBinding(const MaterialRuntimeBindingSource source, const MaterialParameter* parameter, const ShaderProgramBinding& binding, const MaterialDrawContext& context, ShaderProgramInstance& instance) const override
		{
			(void)context;
			const MaterialParameterType parameterType = parameter != nullptr ? parameter->GetParameterType() : MaterialParameterType::Invalid;
			if (!SupportsBinding(source, parameterType, binding))
			{
				return false;
			}

			// Provider 使用真实资源，确保强类型 setter 与生命周期契约也在 Material 入口生效。
			switch (binding.Kind)
			{
			case ShaderProgramBindingKind::Uniform:
			{
				const Float4 value = source == MaterialRuntimeBindingSource::PerView ? Float4::One : Float4::Zero;
				return instance.SetUniform(binding.Path, &value, sizeof(value));
			}
			case ShaderProgramBindingKind::Texture:
				return instance.SetTexture(binding.Path, m_Texture);
			case ShaderProgramBindingKind::Sampler:
				return instance.SetSampler(binding.Path, m_Sampler);
			default:
				return false;
			}
		}

	private:
		GPUTextureView* m_Texture = nullptr;
		GPUSampler*     m_Sampler = nullptr;
	};

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
			InitializeSLC2BindingTestResources();
			RunVertexFactoryContractTest();
			RunStaticMeshVertexFactoryTest();
			RunSLC2VertexInputLayoutMatchingTest();
			RunSLC2VertexInputRoundTripTest();
			RunSLC2ParameterBlockReflectionTest();
			RunSLC2AppendConsumeCounterTest();
			RunSlangVariantCacheGenerationTest();
			RunSLC2IndexedRenderGeometryDrawTest();
			RunGraphicShaderAssetDrawTest();
			RunSLC2MaterialDrawTest();
			RunSLC2SystemMaterialDrawTest();
			RunSLC2AlternateLayoutPipelineTest();
			RunSLC2NonIndexedSparseSlotDrawTest();
			RunSLC2InstancedRenderGeometryDrawTest();
			RunMaterialSLC2RuntimeTest();
			RunSLC2ComputeDispatchSmokeTest();
			WriteTestLog();
		}

		void OnDispose() override
		{
			// Program handles must release their old/new generations before the asset and test geometry.
			if (GPUDevice::instance != nullptr && GPUDevice::instance->GetMainContext() != nullptr)
				GPUDevice::instance->GetMainContext()->SetSLC2State(nullptr);
			_graphicShaderOld.reset();
			_graphicShaderNew.reset();
			_slc2MaterialBaseDraw.reset();
			_slc2MaterialDetailDraw.reset();
			_slc2MaterialNewDraw.reset();
			_slc2SystemFirstDraw.reset();
			_slc2SystemSecondDraw.reset();
			if (_slc2MaterialChild)
			{
				SLC2MaterialInstance* asset = _slc2MaterialChild.Get();
				_slc2MaterialChild = nullptr;
				AssetContent::UnloadAsset(asset);
			}
			if (_slc2MaterialParent)
			{
				SLC2MaterialInstance* asset = _slc2MaterialParent.Get();
				_slc2MaterialParent = nullptr;
				AssetContent::UnloadAsset(asset);
			}
			if (_slc2Material)
			{
				SLC2Material* asset = _slc2Material.Get();
				_slc2Material = nullptr;
				AssetContent::UnloadAsset(asset);
			}
			if (_slc2MaterialShader)
			{
				GraphicShader* asset = _slc2MaterialShader.Get();
				_slc2MaterialShader = nullptr;
				AssetContent::UnloadAsset(asset);
			}
			if (_graphicShaderAsset)
			{
				GraphicShader* asset = _graphicShaderAsset.Get();
				_graphicShaderAsset = nullptr;
				AssetContent::UnloadAsset(asset);
			}
			if (_graphicShaderAssetPath.HasChars())
				FileSystem::DeleteFile(_graphicShaderAssetPath);
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
			Delete(_externalGraphicsConstantBuffer);
			_externalGraphicsConstantBuffer = nullptr;
			Delete(_graphicsVertexBuffer);
			_graphicsVertexBuffer = nullptr;
			Delete(_graphicsIndexBuffer);
			_graphicsIndexBuffer = nullptr;
			Delete(_alternateVertexBuffer);
			_alternateVertexBuffer = nullptr;
			Delete(_graphicsTarget);
			_graphicsTarget = nullptr;
			Delete(_bindingSampler);
			_bindingSampler = nullptr;
			Delete(_bindingTypedBuffer);
			_bindingTypedBuffer = nullptr;
			Delete(_bindingRWTypedBuffer);
			_bindingRWTypedBuffer = nullptr;
			Delete(_bindingStructuredBuffer);
			_bindingStructuredBuffer = nullptr;
			Delete(_bindingRWStructuredBuffer);
			_bindingRWStructuredBuffer = nullptr;
			Delete(_bindingAppendBuffer);
			_bindingAppendBuffer = nullptr;
			Delete(_bindingRawBuffer);
			_bindingRawBuffer = nullptr;
			Delete(_bindingRWRawBuffer);
			_bindingRWRawBuffer = nullptr;
			Delete(_bindingTexture);
			_bindingTexture = nullptr;
			Delete(_systemSceneTexture);
			_systemSceneTexture = nullptr;
			Delete(_counterReadbackBuffer);
			_counterReadbackBuffer = nullptr;
			Delete(_counterResultBuffer);
			_counterResultBuffer = nullptr;
			Delete(_counterShader);
			_counterShader = nullptr;
			Delete(_sparseUVBuffer);
			_sparseUVBuffer = nullptr;
			Delete(_sparsePipeline);
			_sparsePipeline = nullptr;
			Delete(_sparseShader);
			_sparseShader = nullptr;
			Delete(_instancedPipeline);
			_instancedPipeline = nullptr;
			Delete(_instanceVertexBuffer);
			_instanceVertexBuffer = nullptr;
			Delete(_graphicsShader);
			_graphicsShader = nullptr;
			if (_shader != nullptr)
			{
				Delete(_shader);
				_shader = nullptr;
			}
		}

	private:
		void InitializeSLC2BindingTestResources()
		{
			if (GPUDevice::instance == nullptr)
			{
				LOG_ERROR("Test", "SLC2 typed binding tests require a GPU device.");
				return;
			}

			_bindingTexture = GPUDevice::instance->CreateTexture(SE_TEXT("SLC2BindingTest.Texture"));
			_bindingSampler = GPUDevice::instance->CreateSampler();
			_bindingTypedBuffer = GPUDevice::instance->CreateBuffer(SE_TEXT("SLC2BindingTest.Typed"));
			_bindingRWTypedBuffer = GPUDevice::instance->CreateBuffer(SE_TEXT("SLC2BindingTest.RWTyped"));
			_bindingStructuredBuffer = GPUDevice::instance->CreateBuffer(SE_TEXT("SLC2BindingTest.Structured"));
			_bindingRWStructuredBuffer = GPUDevice::instance->CreateBuffer(SE_TEXT("SLC2BindingTest.RWStructured"));
			_bindingAppendBuffer = GPUDevice::instance->CreateBuffer(SE_TEXT("SLC2BindingTest.Append"));
			_bindingRawBuffer = GPUDevice::instance->CreateBuffer(SE_TEXT("SLC2BindingTest.Raw"));
			_bindingRWRawBuffer = GPUDevice::instance->CreateBuffer(SE_TEXT("SLC2BindingTest.RWRaw"));

			const GPUBufferDescription rawRead = GPUBufferDescription::Buffer(
				64, GPUBufferFlags::RawBuffer | GPUBufferFlags::ShaderResource, PixelFormat::Undefined, nullptr, 4);
			const GPUBufferDescription rawWrite = GPUBufferDescription::Buffer(
				64, GPUBufferFlags::RawBuffer | GPUBufferFlags::UnorderedAccess, PixelFormat::Undefined, nullptr, 4);
			if (_bindingTexture == nullptr ||
				!_bindingTexture->Init(GPUTextureDescription::New2D(4, 4, PixelFormat::R8G8B8A8_UNorm)) ||
				_bindingSampler == nullptr ||
				!_bindingSampler->Init(GPUSamplerDescription::New(GPUSamplerFilter::Bilinear)) ||
				_bindingTypedBuffer == nullptr ||
				!_bindingTypedBuffer->Init(GPUBufferDescription::Typed(4, PixelFormat::R32G32B32A32_Float)) ||
				_bindingRWTypedBuffer == nullptr ||
				!_bindingRWTypedBuffer->Init(GPUBufferDescription::Typed(4, PixelFormat::R32G32B32A32_Float, true)) ||
				_bindingStructuredBuffer == nullptr ||
				!_bindingStructuredBuffer->Init(GPUBufferDescription::Structured(4, sizeof(Float4))) ||
				_bindingRWStructuredBuffer == nullptr ||
				!_bindingRWStructuredBuffer->Init(GPUBufferDescription::Structured(4, sizeof(Float4), true)) ||
				_bindingAppendBuffer == nullptr ||
				!_bindingAppendBuffer->Init(GPUBufferDescription::StructuredAppend(4, sizeof(Float4))) ||
				_bindingRawBuffer == nullptr || !_bindingRawBuffer->Init(rawRead) ||
				_bindingRWRawBuffer == nullptr || !_bindingRWRawBuffer->Init(rawWrite))
			{
				LOG_ERROR("Test", "Failed to initialize SLC2 typed binding test resources.");
				return;
			}
			_materialRuntimeProvider.SetResources(_bindingTexture->View(), _bindingSampler);
		}

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

		bool MatchesStaticMeshInstanceBinding(const VertexFactoryLayout& layout) const
		{
			const VertexFactoryBufferBinding* binding = layout.FindBinding(StaticMeshVertexFactory::InstanceSlot);
			return binding != nullptr &&
				binding->Stride == sizeof(StaticMeshInstanceData) &&
				binding->InputRate == VertexFactoryInputRate::PerInstance &&
				binding->InstanceStepRate == 1;
		}

		void RunStaticMeshVertexFactoryTest()
		{
			const VertexFactoryLayout& layout = StaticMeshVertexFactory::GetLayout();
			if (!layout.IsValid() || layout.GetBindings().Count() != 4 || layout.GetElements().Count() != 11 ||
				!MatchesStaticMeshBinding(layout, StaticMeshVertexFactory::PositionSlot, sizeof(VB0ElementType)) ||
				!MatchesStaticMeshBinding(layout, StaticMeshVertexFactory::AttributesSlot, sizeof(VB1ElementType)) ||
				!MatchesStaticMeshBinding(layout, StaticMeshVertexFactory::ColorSlot, sizeof(VB2ElementType)) ||
				!MatchesStaticMeshInstanceBinding(layout) ||
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
					StaticMeshVertexFactory::ColorSlot, OFFSET_OF(VB2ElementType, Color)) ||
				!MatchesStaticMeshElement(layout, SE_TEXT("ATTRIBUTE"), 0, PixelFormat::R32G32B32A32_Float,
					StaticMeshVertexFactory::InstanceSlot, OFFSET_OF(StaticMeshInstanceData, InstanceOrigin)) ||
				!MatchesStaticMeshElement(layout, SE_TEXT("ATTRIBUTE"), 1, PixelFormat::R32G32B32A32_Float,
					StaticMeshVertexFactory::InstanceSlot, OFFSET_OF(StaticMeshInstanceData, InstanceTransform1)) ||
				!MatchesStaticMeshElement(layout, SE_TEXT("ATTRIBUTE"), 2, PixelFormat::R32G32B32_Float,
					StaticMeshVertexFactory::InstanceSlot, OFFSET_OF(StaticMeshInstanceData, InstanceTransform2)) ||
				!MatchesStaticMeshElement(layout, SE_TEXT("ATTRIBUTE"), 3, PixelFormat::R32G32B32_Float,
					StaticMeshVertexFactory::InstanceSlot, OFFSET_OF(StaticMeshInstanceData, InstanceTransform3)) ||
				!MatchesStaticMeshElement(layout, SE_TEXT("ATTRIBUTE"), 4, PixelFormat::R16G16B16A16_Float,
					StaticMeshVertexFactory::InstanceSlot, OFFSET_OF(StaticMeshInstanceData, InstanceLightmapArea)))
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
				!EnumHasAllFlags(positionDescription.Flags, GPUBufferFlags::VertexBuffer))
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

		const ShaderIRParameterBlock* FindParameterBlock(const ShaderReflectionIR& layout, const String& name) const
		{
			for (int32 blockIndex = 0; blockIndex < layout.ParameterBlocks.Count(); blockIndex++)
			{
				if (layout.ParameterBlocks[blockIndex].Name == name)
				{
					return &layout.ParameterBlocks[blockIndex];
				}
			}
			return nullptr;
		}

		const ShaderIRDescriptorRange* FindDescriptor(
			const ShaderIRParameterBlock& block,
			const String& descriptorType) const
		{
			for (int32 bindingIndex = 0; bindingIndex < block.RangeBindings.Count(); bindingIndex++)
			{
				const ShaderIRRangeBinding& binding = block.RangeBindings[bindingIndex];
				for (int32 descriptorIndex = 0; descriptorIndex < binding.DescriptorRanges.Count(); descriptorIndex++)
				{
					const ShaderIRDescriptorRange& descriptor = binding.DescriptorRanges[descriptorIndex];
					if (descriptor.DescriptorType == descriptorType)
					{
						return &descriptor;
					}
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

		bool ValidateInstancedVertexProgram(const SLC2ProgramRecord& program) const
		{
			for (int32 targetIndex = 0; targetIndex < program.Targets.Count(); targetIndex++)
			{
				const SLC2TargetRecord& target = program.Targets[targetIndex];
				for (int32 variantIndex = 0; variantIndex < target.Variants.Count(); variantIndex++)
				{
					const SLC2StageRecord* vertexStage = FindStage(target.Variants[variantIndex], ShaderStage::Vertex);
					if (vertexStage == nullptr || vertexStage->VertexInputSignature.Count() != 6)
					{
						return false;
					}

					const VertexFactoryLayout& layout = StaticMeshVertexFactory::GetLayout();
					for (int32 inputIndex = 0; inputIndex < vertexStage->VertexInputSignature.Count(); inputIndex++)
					{
						const SLC2VertexInputSignatureElement& input = vertexStage->VertexInputSignature[inputIndex];
						const VertexFactoryInputElement* element = layout.FindElement(input.Semantic, input.SemanticIndex);
						SLC2VertexInputType expectedType = SLC2VertexInputType::Float;
						bool recognizedInput = false;
						if (input.Semantic == SE_TEXT("POSITION") && input.SemanticIndex == 0)
						{
							expectedType = SLC2VertexInputType::Float3;
							recognizedInput = true;
						}
						else if (input.Semantic == SE_TEXT("ATTRIBUTE") &&
							(input.SemanticIndex == 0 || input.SemanticIndex == 1 || input.SemanticIndex == 4))
						{
							expectedType = SLC2VertexInputType::Float4;
							recognizedInput = true;
						}
						else if (input.Semantic == SE_TEXT("ATTRIBUTE") &&
							(input.SemanticIndex == 2 || input.SemanticIndex == 3))
						{
							expectedType = SLC2VertexInputType::Float3;
							recognizedInput = true;
						}
						if (!recognizedInput || element == nullptr ||
							input.ShaderType != expectedType ||
							(input.Semantic == SE_TEXT("POSITION") && element->Slot != StaticMeshVertexFactory::PositionSlot) ||
							(input.Semantic == SE_TEXT("ATTRIBUTE") && element->Slot != StaticMeshVertexFactory::InstanceSlot))
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
			const SLC2ProgramRecord* instancedProgram = FindProgram(artifact, SE_TEXT("CompileOnlyVertexInputInstanced"));
			if (positionProgram == nullptr || systemOnlyProgram == nullptr || instancedProgram == nullptr ||
				!ValidateVertexProgram(*positionProgram, true) || !ValidateVertexProgram(*systemOnlyProgram, false) ||
				!ValidateInstancedVertexProgram(*instancedProgram))
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

		void RunSLC2ParameterBlockReflectionTest()
		{
			ShaderCompileRequest request;
			request.ShaderName = SE_TEXT("ParameterBlockReflection");
			request.SourcePath = SE_TEXT("Shaders/Slang/CompileOnly_ParameterBlocks.slang");
			request.Targets = {
				{ShaderTargetPlatform::Windows, ShaderProfile::Vulkan_SM5, FeatureLevel::SM5},
			};

			if (!File::ReadAllText(EngineContext::StartupFolder + SE_TEXT("/Shaders/Slang/CompileOnly_ParameterBlocks.slang"), request.SourceCode))
			{
				LOG_ERROR("Test", "Failed to read SLC2 ParameterBlock reflection shader source.");
				return;
			}

			ShaderCompileResult result = ShadersCompilation::CompileSlang(request);
			if (result.Status == ShaderCompileStatus::Failed)
			{
				LOG_ERROR("Test", "{0}", result.CompileMessage.Text);
				return;
			}

			// 必须通过共享 Reader 验证序列化后的布局，避免只检查 Builder 的临时 IR。
			SLC2Artifact artifact;
			String error;
			if (!SLC2Reader::Read(result.SLC2Data, artifact, error))
			{
				LOG_ERROR("Test", "SLC2 ParameterBlock reflection round-trip failed: {0}", error);
				return;
			}

			const SLC2ProgramRecord* program = FindProgram(artifact, SE_TEXT("ParameterBlockReflection"));
			if (program == nullptr || program->Targets.Count() != 1 || program->Targets[0].Variants.Count() != 1)
			{
				LOG_ERROR("Test", "SLC2 ParameterBlock reflection program topology is invalid.");
				return;
			}

			const ShaderReflectionIR& layout = program->Targets[0].Variants[0].Layout;
			const ShaderIRParameterBlock* outer = FindParameterBlock(layout, SE_TEXT("Outer"));
			const ShaderIRParameterBlock* nested = FindParameterBlock(layout, SE_TEXT("Nested"));
			const ShaderIRParameterBlock* constants = FindParameterBlock(layout, SE_TEXT("Constants"));
			const ShaderIRParameterBlock* root = layout.RootBlockId < static_cast<uint32>(layout.ParameterBlocks.Count()) ? &layout.ParameterBlocks[layout.RootBlockId] : nullptr;
			if (layout.ParameterBlocks.Count() != 4 || FindParameterBlock(layout, SE_TEXT("Unused")) != nullptr ||
				root == nullptr || outer == nullptr || nested == nullptr || constants == nullptr ||
				!root->HasDefaultUniformBuffer ||
				!outer->HasDefaultUniformBuffer || !nested->HasDefaultUniformBuffer || !constants->HasDefaultUniformBuffer)
			{
				LOG_ERROR("Test", "SLC2 ParameterBlock pruning or implicit uniform buffer reflection is invalid.");
				return;
			}

			const ShaderIRDescriptorRange* nestedTexture = FindDescriptor(*nested, SE_TEXT("Texture"));
			const ShaderIRDescriptorRange* constantsTexture = FindDescriptor(*constants, SE_TEXT("Texture"));
			if (outer->DefaultUniformBuffer.StageMask != SE_TEXT("vs") ||
				constants->DefaultUniformBuffer.StageMask != SE_TEXT("vs") ||
				nested->DefaultUniformBuffer.StageMask != SE_TEXT("ps") ||
				nestedTexture == nullptr || nestedTexture->StageMask != SE_TEXT("ps") ||
				constantsTexture == nullptr || constantsTexture->StageMask != SE_TEXT("vs"))
			{
				LOG_ERROR("Test", "SLC2 ParameterBlock descriptor Stage masks are not precise.");
				return;
			}

			// PB 的隐式 UBO 与成员资源位于同一 set；CB 继续使用父 PB 的 set。
			// 同一块内两者必须占用不同 binding，否则 container/element 偏移发生了丢失或重复累计。
			if (outer->DefaultUniformBuffer.Set == nested->DefaultUniformBuffer.Set ||
				outer->DefaultUniformBuffer.Set != constants->DefaultUniformBuffer.Set ||
				nested->DefaultUniformBuffer.Set != nestedTexture->Set ||
				nested->DefaultUniformBuffer.Binding == nestedTexture->Binding ||
				constants->DefaultUniformBuffer.Set != constantsTexture->Set ||
				constants->DefaultUniformBuffer.Binding == constantsTexture->Binding ||
				BuildPipelineLayoutFingerprint(layout) != layout.PipelineLayoutFingerprint)
			{
				LOG_ERROR("Test", "SLC2 ParameterBlock absolute descriptor locations or fingerprint are invalid.");
				return;
			}

			// 运行时 Reflection 在 Program 初始化时构建稳定的 Uniform Block 与用户资源槽，
			// NamePath 每次解析到这些索引，不缓存跨调用 Handle。
			ShaderProgramReflection runtimeReflection;
			ShaderVarLocation location;
			ShaderVarLocation rootBlockLocation;
			ShaderVarLocation outerBlockLocation;
			ShaderVarLocation nestedBlockLocation;
			ShaderVarLocation constantsBlockLocation;
			if (!runtimeReflection.Initialize(layout) ||
				!ShaderNameResolver::ResolveUniformBlock(runtimeReflection, String::Empty, rootBlockLocation) ||
				!ShaderNameResolver::ResolveUniformBlock(runtimeReflection, SE_TEXT("Outer"), outerBlockLocation) ||
				!ShaderNameResolver::ResolveUniformBlock(runtimeReflection, SE_TEXT("Outer.Nested"), nestedBlockLocation) ||
				!ShaderNameResolver::ResolveUniformBlock(runtimeReflection, SE_TEXT("Outer.Constants"), constantsBlockLocation) ||
				rootBlockLocation.BlockId != layout.RootBlockId ||
				outerBlockLocation.BlockId != outer->Id || nestedBlockLocation.BlockId != nested->Id ||
				constantsBlockLocation.BlockId != constants->Id ||
				rootBlockLocation.UniformBlockIndex != 0 || outerBlockLocation.UniformBlockIndex != 1 ||
				nestedBlockLocation.UniformBlockIndex != 2 || constantsBlockLocation.UniformBlockIndex != 3 ||
				rootBlockLocation.UniformByteSize != root->UniformByteSize ||
				outerBlockLocation.UniformByteSize != outer->UniformByteSize ||
				nestedBlockLocation.UniformByteSize != nested->UniformByteSize ||
				constantsBlockLocation.UniformByteSize != constants->UniformByteSize)
			{
				LOG_ERROR("Test", "SLC2 NamePath uniform block precomputation is invalid.");
				return;
			}
			ShaderVarLocation nestedPixelLocation;
			ShaderVarLocation nestedTintLocation;
			if (!ShaderNameResolver::Resolve(runtimeReflection, SE_TEXT("RootScale"), location) ||
				location.Kind != ShaderParameterAddressKind::UniformValue ||
				location.UniformBlockIndex != rootBlockLocation.UniformBlockIndex || location.UniformByteSize != sizeof(float) ||
				!ShaderNameResolver::Resolve(runtimeReflection, SE_TEXT("Outer.VertexScale"), location) ||
				location.Kind != ShaderParameterAddressKind::UniformValue ||
				location.UniformBlockIndex != outerBlockLocation.UniformBlockIndex || location.UniformByteOffset != 0 ||
				!ShaderNameResolver::Resolve(runtimeReflection, SE_TEXT("Outer.Nested.PixelValue"), nestedPixelLocation) ||
				!ShaderNameResolver::Resolve(runtimeReflection, SE_TEXT("Outer.Nested.Deep.Tint"), nestedTintLocation) ||
				nestedPixelLocation.Kind != ShaderParameterAddressKind::UniformValue ||
				nestedTintLocation.Kind != ShaderParameterAddressKind::UniformValue ||
				nestedPixelLocation.UniformBlockIndex != nestedBlockLocation.UniformBlockIndex ||
				nestedTintLocation.UniformBlockIndex != nestedBlockLocation.UniformBlockIndex ||
				nestedPixelLocation.UniformByteOffset != 0 || nestedTintLocation.UniformByteOffset != 16)
			{
				LOG_ERROR("Test", "SLC2 NamePath ordinary-data address resolution is invalid.");
				return;
			}
			ShaderVarLocation firstTextureLocation;
			ShaderVarLocation secondTextureLocation;
			ShaderVarLocation constantsTextureLocation;
			ShaderVarLocation samplerLocation;
            if (!ShaderNameResolver::Resolve(
                    runtimeReflection, SE_TEXT("Outer.Nested.PixelTextures[0]"), firstTextureLocation) ||
                !ShaderNameResolver::Resolve(
                    runtimeReflection, SE_TEXT("Outer.Nested.PixelTextures[1]"), secondTextureLocation) ||
                !ShaderNameResolver::Resolve(
                    runtimeReflection, SE_TEXT("Outer.Nested.LinearSampler"), samplerLocation) ||
                !ShaderNameResolver::Resolve(
                    runtimeReflection, SE_TEXT("Outer.Constants.VertexTexture"), constantsTextureLocation) ||
                firstTextureLocation.Kind != ShaderParameterAddressKind::Resource ||
                secondTextureLocation.Kind != ShaderParameterAddressKind::Resource ||
                constantsTextureLocation.Kind != ShaderParameterAddressKind::Resource ||
                firstTextureLocation.LogicalResourceSlotIndex != 0 ||
                secondTextureLocation.LogicalResourceSlotIndex != 1 ||
                samplerLocation.LogicalResourceSlotIndex != 2 ||
				constantsTextureLocation.LogicalResourceSlotIndex != 3)
            {
                LOG_ERROR("Test", "SLC2 NamePath logical resource slot resolution is invalid.");
                return;
            }
            if (ShaderNameResolver::Resolve(runtimeReflection, String::Empty, location) ||
                ShaderNameResolver::Resolve(runtimeReflection, SE_TEXT("Outer.Nested.PixelTextures[2]"), location) ||
                ShaderNameResolver::Resolve(runtimeReflection, SE_TEXT("Outer.Missing"), location) ||
                ShaderNameResolver::Resolve(runtimeReflection, SE_TEXT("Outer.VertexScale.Value"), location) ||
                ShaderNameResolver::ResolveUniformBlock(runtimeReflection, SE_TEXT("Outer.VertexScale"), location))
            {
                LOG_ERROR("Test", "SLC2 NamePath invalid path rejection is incomplete.");
                return;
            }

            // Instance 是 ordinary data 的唯一 owner；字段写入允许任意未越界字节数，整块写入要求精确大小。
            const SLC2TargetRecord*  target  = &program->Targets[0];
            const SLC2VariantRecord* variant = &target->Variants[0];
            SLC2GPUShaderProgram     runtimeProgram;
            ShaderProgramInstance    instance;
            if (!runtimeProgram.Initialize(program, target, variant) || !instance.Initialize(&runtimeProgram) ||
                !instance.SetTexture(SE_TEXT("Outer.Nested.PixelTextures[0]"),
                                     _bindingTexture != nullptr ? _bindingTexture->View() : nullptr) ||
                !instance.SetTexture(SE_TEXT("Outer.Nested.PixelTextures[1]"),
                                     _bindingTexture != nullptr ? _bindingTexture->View() : nullptr) ||
                !instance.SetSampler(SE_TEXT("Outer.Nested.LinearSampler"), _bindingSampler) ||
                !instance.SetTexture(SE_TEXT("Outer.Constants.VertexTexture"),
                                     _bindingTexture != nullptr ? _bindingTexture->View() : nullptr) ||
                !instance.SetBool(SE_TEXT("RootScale"), true) ||
                !instance.SetInt(SE_TEXT("RootScale"), 0x11223344) ||
                !instance.SetUInt(SE_TEXT("Outer.VertexScale"), 0x55667788u) ||
                !instance.SetFloat(SE_TEXT("Outer.VertexScale"), 3.5f))
            {
                LOG_ERROR("Test", "SLC2 ProgramInstance uniform state initialization failed.");
                return;
            }

            // Setter 只验证逻辑类别、现有 shape 与物理读写能力；失败必须保留此前的正确绑定。
            if (instance.SetSampler(SE_TEXT("Outer.Nested.PixelTextures[0]"), _bindingSampler) ||
                instance.SetTexture(SE_TEXT("Outer.Nested.LinearSampler"),
                                    _bindingTexture != nullptr ? _bindingTexture->View() : nullptr))
            {
                LOG_ERROR("Test", "SLC2 typed resource setters accepted a category, shape, or access mismatch.");
                return;
            }

            const byte            partialPixelValue[] = {0x5a, 0xa5};
            ShaderBindingSnapshot frozenSnapshot;
            if (!instance.SetUniform(
                    SE_TEXT("Outer.Nested.PixelValue"), partialPixelValue, sizeof(partialPixelValue)) ||
                !instance.BuildSnapshot(frozenSnapshot) || !frozenSnapshot.IsFrozen() ||
                frozenSnapshot.GetProgram() != &runtimeProgram || frozenSnapshot.GetResourceCount() != 4 ||
                frozenSnapshot.GetUniformData().Count() != 4)
            {
                LOG_ERROR("Test", "SLC2 ProgramInstance failed to build a complete frozen snapshot.");
                return;
            }
            if (frozenSnapshot.GetResource(0) == nullptr ||
				frozenSnapshot.GetResource(0)->GetTexture() != _bindingTexture->View())
            {
                LOG_ERROR("Test", "Failed resource replacement did not preserve the previous binding.");
                return;
            }

            ShaderProgramInstance incompleteInstance;
            if (!incompleteInstance.Initialize(&runtimeProgram) || incompleteInstance.BuildSnapshot(frozenSnapshot) ||
                !frozenSnapshot.IsFrozen() || frozenSnapshot.GetProgram() != &runtimeProgram ||
                frozenSnapshot.GetResourceCount() != 4)
            {
                LOG_ERROR("Test", "Failed BuildSnapshot did not preserve the caller's previous frozen snapshot.");
                return;
            }

            const ShaderBindingUniformData* frozenRoot       = frozenSnapshot.GetUniformData(0);
            const ShaderBindingUniformData* frozenOuter      = frozenSnapshot.GetUniformData(1);
            const ShaderBindingUniformData* frozenNested     = frozenSnapshot.GetUniformData(2);
            const ShaderBindingUniformData* frozenConstants  = frozenSnapshot.GetUniformData(3);
            int32                           frozenRootValue  = 0;
            float                           frozenOuterValue = 0.0f;
            if (frozenRoot == nullptr || frozenOuter == nullptr || frozenNested == nullptr ||
                frozenConstants == nullptr || frozenRoot->Data.Count() != static_cast<int32>(root->UniformByteSize) ||
                frozenOuter->Data.Count() != static_cast<int32>(outer->UniformByteSize) ||
                frozenNested->Data.Count() != static_cast<int32>(nested->UniformByteSize) ||
                frozenConstants->Data.Count() != static_cast<int32>(constants->UniformByteSize))
            {
                LOG_ERROR("Test", "SLC2 snapshot uniform blocks are not in reflected index order.");
                return;
            }
            Platform::MemoryCopy(&frozenRootValue, frozenRoot->Data.Get(), sizeof(frozenRootValue));
            Platform::MemoryCopy(&frozenOuterValue, frozenOuter->Data.Get(), sizeof(frozenOuterValue));
            if (frozenRootValue != 0x11223344 || frozenOuterValue != 3.5f ||
                Platform::MemoryCompare(frozenNested->Data.Get(), partialPixelValue, sizeof(partialPixelValue)) != 0)
            {
                LOG_ERROR("Test", "SLC2 typed or raw ordinary-data field writes produced unexpected bytes.");
                return;
            }

            List<byte> nestedBytes;
            nestedBytes.Resize(static_cast<int32>(nested->UniformByteSize));
            for (int32 index = 0; index < nestedBytes.Count(); index++)
            {
                nestedBytes[index] = 0x2a;
            }
            if (!instance.SetConstantBuffer(SE_TEXT("Outer.Nested"), nestedBytes.Get(), nested->UniformByteSize) ||
                !instance.SetFloat(SE_TEXT("Outer.Nested.PixelValue"), 7.0f) ||
                instance.SetConstantBuffer(SE_TEXT("Outer.Nested"), nestedBytes.Get(), nested->UniformByteSize - 1))
            {
                LOG_ERROR("Test", "SLC2 exact-size ConstantBuffer update contract failed.");
                return;
            }

            ShaderBindingSnapshot mixedSnapshot;
            if (!instance.BuildSnapshot(mixedSnapshot))
            {
                LOG_ERROR("Test", "SLC2 mixed field and ConstantBuffer writes did not build a snapshot.");
                return;
            }
            const ShaderBindingUniformData* mixedNested = mixedSnapshot.GetUniformData(2);
            if (mixedNested == nullptr)
            {
                LOG_ERROR("Test", "SLC2 mixed snapshot is missing the nested uniform block.");
                return;
            }
            float mixedPixelValue = 0.0f;
            Platform::MemoryCopy(&mixedPixelValue, mixedNested->Data.Get(), sizeof(mixedPixelValue));
            if (mixedPixelValue != 7.0f || mixedNested->Data[sizeof(float)] != 0x2a ||
                Platform::MemoryCompare(frozenNested->Data.Get(), partialPixelValue, sizeof(partialPixelValue)) != 0)
            {
                LOG_ERROR("Test", "SLC2 write ordering or snapshot immutability is invalid.");
                return;
            }

			// External 模式只冻结 GPUConstantBuffer 对象身份；字段写入被拒绝，完整字节写入切回 Internal。
			TestGPUConstantBuffer externalNested(nested->UniformByteSize);
			TestGPUConstantBuffer undersizedNested(nested->UniformByteSize - 1);
			if (instance.SetConstantBuffer(SE_TEXT("Outer.Nested.PixelValue"), &externalNested) ||
				instance.SetConstantBuffer(SE_TEXT("Outer.Nested"), &undersizedNested))
			{
				LOG_ERROR("Test", "SLC2 external ConstantBuffer accepted an invalid target or size.");
				return;
			}
			ShaderBindingSnapshot failedBindingSnapshot;
			if (!instance.BuildSnapshot(failedBindingSnapshot) ||
				failedBindingSnapshot.GetUniformData(2) == nullptr ||
				failedBindingSnapshot.GetUniformData(2)->Source != ShaderUniformBlockSource::InternalData ||
				!instance.SetConstantBuffer(SE_TEXT("Outer.Nested"), &externalNested) ||
				instance.SetFloat(SE_TEXT("Outer.Nested.PixelValue"), 11.0f))
			{
				LOG_ERROR("Test", "SLC2 failed external binding changed state or field-write rejection failed.");
				return;
			}

			ShaderBindingSnapshot externalSnapshot;
			if (!instance.BuildSnapshot(externalSnapshot))
			{
				LOG_ERROR("Test", "SLC2 external ConstantBuffer snapshot failed.");
				return;
			}
			const ShaderBindingUniformData* externalNestedBlock = externalSnapshot.GetUniformData(2);
			if (externalNestedBlock == nullptr ||
				externalNestedBlock->Source != ShaderUniformBlockSource::ExternalConstantBuffer ||
				externalNestedBlock->ExternalBuffer != &externalNested || externalNestedBlock->Data.HasItems())
			{
				LOG_ERROR("Test", "SLC2 snapshot did not preserve the external ConstantBuffer tagged source.");
				return;
			}

			ShaderProgramInstance sharedExternalInstance;
			ShaderBindingSnapshot sharedExternalSnapshot;
			if (!sharedExternalInstance.Initialize(&runtimeProgram) ||
				!sharedExternalInstance.SetTexture(SE_TEXT("Outer.Nested.PixelTextures[0]"),
											 _bindingTexture != nullptr ? _bindingTexture->View() : nullptr) ||
				!sharedExternalInstance.SetTexture(SE_TEXT("Outer.Nested.PixelTextures[1]"),
											 _bindingTexture != nullptr ? _bindingTexture->View() : nullptr) ||
				!sharedExternalInstance.SetSampler(SE_TEXT("Outer.Nested.LinearSampler"), _bindingSampler) ||
				!sharedExternalInstance.SetTexture(SE_TEXT("Outer.Constants.VertexTexture"),
											 _bindingTexture != nullptr ? _bindingTexture->View() : nullptr) ||
				!sharedExternalInstance.SetConstantBuffer(SE_TEXT("Outer.Nested"), &externalNested) ||
				!sharedExternalInstance.BuildSnapshot(sharedExternalSnapshot) ||
				sharedExternalSnapshot.GetUniformData(2) == nullptr ||
				sharedExternalSnapshot.GetUniformData(2)->ExternalBuffer != &externalNested)
			{
				LOG_ERROR("Test", "SLC2 ProgramInstances failed to share one external ConstantBuffer.");
				return;
			}

			if (!instance.SetConstantBuffer(SE_TEXT("Outer.Nested"), nestedBytes.Get(), nested->UniformByteSize) ||
				!instance.SetFloat(SE_TEXT("Outer.Nested.PixelValue"), 7.0f))
			{
				LOG_ERROR("Test", "SLC2 complete bytes failed to switch an external ConstantBuffer back to Internal Data.");
				return;
			}
			ShaderBindingSnapshot switchedInternalSnapshot;
			if (!instance.BuildSnapshot(switchedInternalSnapshot) ||
				switchedInternalSnapshot.GetUniformData(2) == nullptr ||
				switchedInternalSnapshot.GetUniformData(2)->Source != ShaderUniformBlockSource::InternalData ||
				switchedInternalSnapshot.GetUniformData(2)->ExternalBuffer != nullptr)
			{
				LOG_ERROR("Test", "SLC2 external-to-internal ConstantBuffer mode switch was not reflected in Snapshot.");
				return;
			}

            List<byte> constantsBytes;
            constantsBytes.Resize(static_cast<int32>(constants->UniformByteSize));
            for (int32 index = 0; index < constantsBytes.Count(); index++)
            {
                constantsBytes[index] = 0x3c;
            }
            List<byte> rootBytes;
            rootBytes.Resize(static_cast<int32>(root->UniformByteSize));
            for (int32 index = 0; index < rootBytes.Count(); index++)
            {
                rootBytes[index] = 0x4d;
            }
            if (!instance.SetFloat(SE_TEXT("Outer.Constants.VertexOffset"), 9.0f) ||
                !instance.SetConstantBuffer(
                    SE_TEXT("Outer.Constants"), constantsBytes.Get(), constants->UniformByteSize) ||
                !instance.SetConstantBuffer(String::Empty, rootBytes.Get(), root->UniformByteSize) ||
                instance.SetConstantBuffer(SE_TEXT("Outer.VertexScale"), rootBytes.Get(), root->UniformByteSize))
            {
                LOG_ERROR("Test", "SLC2 root or ParameterBlock ConstantBuffer targeting failed.");
                return;
            }

            List<byte> overflowBytes;
            overflowBytes.Resize(
                static_cast<int32>(nested->UniformByteSize - nestedTintLocation.UniformByteOffset + 1));
            if (instance.SetUniform(
                    SE_TEXT("Outer.Nested.Deep.Tint"), overflowBytes.Get(), static_cast<uint32>(overflowBytes.Count())))
            {
                LOG_ERROR("Test", "SLC2 ordinary-data field write crossed its uniform block boundary.");
                return;
            }

            ShaderBindingSnapshot wholeBlockSnapshot;
            SLC2GPUShaderProgram  otherRuntimeProgram;
            const bool wholeBlockBuilt = instance.BuildSnapshot(wholeBlockSnapshot);
            const ShaderBindingUniformData* wholeNested = wholeBlockSnapshot.GetUniformData(2);
            float wholeNestedPixelValue = 0.0f;
            if (wholeNested != nullptr)
            {
                Platform::MemoryCopy(&wholeNestedPixelValue, wholeNested->Data.Get(), sizeof(wholeNestedPixelValue));
            }
            if (!wholeBlockBuilt || wholeBlockSnapshot.GetResourceCount() != 4 ||
                wholeBlockSnapshot.GetUniformData(0) == nullptr || wholeBlockSnapshot.GetUniformData(3) == nullptr ||
                wholeNested == nullptr || wholeBlockSnapshot.GetUniformData(0)->Data[0] != 0x4d ||
                wholeBlockSnapshot.GetUniformData(3)->Data[0] != 0x3c || wholeNestedPixelValue != 7.0f ||
                wholeNested->Data[sizeof(float)] != 0x2a ||
                !otherRuntimeProgram.Initialize(program, target, variant) ||
                wholeBlockSnapshot.GetProgram() == &otherRuntimeProgram)
            {
                LOG_ERROR("Test", "SLC2 whole-block update, resource preservation, or Program identity failed.");
                return;
            }

            GPUSampler* releasedSampler = GPUDevice::instance->CreateSampler();
            ShaderBindingSnapshot releasedSnapshot;
			if (releasedSampler == nullptr ||
				!releasedSampler->Init(GPUSamplerDescription::New(GPUSamplerFilter::Point)) ||
				!instance.SetSampler(SE_TEXT("Outer.Nested.LinearSampler"), releasedSampler) ||
                !instance.BuildSnapshot(releasedSnapshot))
            {
				Delete(releasedSampler);
                LOG_ERROR("Test", "SLC2 released-resource test setup failed.");
                return;
            }
			releasedSampler->ReleaseGPU();
            const bool instanceRejectedReleasedResource = !instance.BuildSnapshot(releasedSnapshot);
			const bool snapshotInvalidatedReleasedResource = releasedSnapshot.GetResource(2) == nullptr ||
				!releasedSnapshot.GetResource(2)->IsValid();
			Delete(releasedSampler);
            if (!instanceRejectedReleasedResource || !snapshotInvalidatedReleasedResource ||
				!instance.SetSampler(SE_TEXT("Outer.Nested.LinearSampler"), _bindingSampler))
            {
                LOG_ERROR("Test", "SLC2 tracked resource release did not invalidate Instance and Snapshot state.");
                return;
            }

            LOG_INFO("Test", "SLC2 ParameterBlock reflection test passed.");
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
			const ShaderUniformBlockLayout* rootUniformBlock =
				_graphicsInstance.GetProgram()->GetReflection().GetUniformBlockLayout(0);
			if (rootUniformBlock == nullptr)
			{
				LOG_ERROR("Test", "SLC2 external ConstantBuffer draw test has no reflected root uniform block.");
				return;
			}
			_externalGraphicsConstantBuffer = GPUDevice::instance->CreateConstantBuffer(
				rootUniformBlock->ByteSize, SE_TEXT("SLC2ExternalConstantBufferTest"));
			const Float4 externalScale = Float4::One;
			if (_externalGraphicsConstantBuffer == nullptr ||
				rootUniformBlock->ByteSize != sizeof(externalScale))
			{
				LOG_ERROR("Test", "Failed to create the SLC2 external ConstantBuffer draw resource.");
				return;
			}
			context->UpdateCB(_externalGraphicsConstantBuffer, &externalScale);
			if (!_graphicsInstance.SetConstantBuffer(String::Empty, _externalGraphicsConstantBuffer))
			{
				LOG_ERROR("Test", "Failed to bind the SLC2 external ConstantBuffer to the root uniform block.");
				return;
			}
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

		void RunGraphicShaderAssetDrawTest()
		{
			if (_graphicsVertexBuffer == nullptr || _graphicsIndexBuffer == nullptr || _graphicsTarget == nullptr)
			{
				LOG_ERROR("Test", "GraphicShader asset draw needs the SLC2 indexed geometry resources.");
				return;
			}

			ShaderCompileRequest request;
			request.ShaderName = SE_TEXT("GraphicShaderAssetDraw");
			request.SourcePath = SE_TEXT("Shaders/Slang/GraphicShader_Draw.slang");
			request.Targets = {{ShaderTargetPlatform::Windows, ShaderProfile::Vulkan_SM5, FeatureLevel::SM5}};
			if (!File::ReadAllText(EngineContext::StartupFolder / request.SourcePath, request.SourceCode))
			{
				LOG_ERROR("Test", "Cannot read GraphicShader asset draw source.");
				return;
			}
			const ShaderCompileResult compiled = ShadersCompilation::CompileSlang(request);
			if (compiled.Status == ShaderCompileStatus::Failed)
			{
				LOG_ERROR("Test", "GraphicShader compilation failed: {0}", compiled.CompileMessage.Text);
				return;
			}

			List<GraphicShaderPipeline> descriptions;
			GraphicShaderPipeline gbuffer;
			gbuffer.ProgramId = SE_TEXT("GBuffer");
			gbuffer.State = SLC2GPUPipelineState::Description::DefaultNoDepth;
			descriptions.Add(gbuffer);
			GraphicShaderPipeline forward = gbuffer;
			forward.ProgramId = SE_TEXT("Forward");
			descriptions.Add(forward);

#if SE_EDITOR
			// Round-trip through the public persisted asset entry, not a test-only GPUShader constructor.
			_graphicShaderAssetPath = AssetContent::CreateTemporaryAssetPath();
			if (!GraphicShader::SaveCompiledAsset(_graphicShaderAssetPath, compiled.SLC2Data, descriptions))
			{
				LOG_ERROR("Test", "GraphicShader asset could not be saved.");
				return;
			}
			_graphicShaderAsset = AssetContent::LoadAsync<GraphicShader>(_graphicShaderAssetPath);
#else
			_graphicShaderAsset = AssetContent::CreateVirtualAsset<GraphicShader>();
			if (_graphicShaderAsset && !_graphicShaderAsset->InstallCompiled(compiled.SLC2Data, descriptions))
			{
				LOG_ERROR("Test", "GraphicShader virtual asset could not be installed.");
				return;
			}
#endif
			// WaitForLoaded returns true when the asset has loaded successfully.
			if (!_graphicShaderAsset || !_graphicShaderAsset->WaitForLoaded())
			{
				LOG_ERROR("Test", "GraphicShader asset could not be loaded.");
				return;
			}

			const ShaderCompileTarget target =
				{ShaderTargetPlatform::Windows, ShaderProfile::Vulkan_SM5, FeatureLevel::SM5};
			std::unique_ptr<GraphicShaderDrawProgram> missing;
			if (_graphicShaderAsset->CreateDrawProgram(SE_TEXT("Shadow"), GraphicShaderGeometryMode::Static,
					target, {}, missing) || missing ||
				_graphicShaderAsset->CreateDrawProgram(SE_TEXT("gbuffer"), GraphicShaderGeometryMode::Static,
					target, {}, missing))
			{
				LOG_ERROR("Test", "GraphicShader accepted a missing or case-mismatched ProgramId.");
				return;
			}
			if (!_graphicShaderAsset->CreateDrawProgram(SE_TEXT("GBuffer"), GraphicShaderGeometryMode::Static,
					target, {}, _graphicShaderOld))
			{
				LOG_ERROR("Test", "GraphicShader did not select the declared GBuffer Program.");
				return;
			}

			List<byte> invalidArtifact = compiled.SLC2Data;
			invalidArtifact[0] = 0;
			SLC2GPUShaderProgram* oldProgram = _graphicShaderOld->GetInstance().GetProgram();
			if (_graphicShaderAsset->InstallCompiled(invalidArtifact, descriptions) ||
				_graphicShaderOld->GetInstance().GetProgram() != oldProgram)
			{
				LOG_ERROR("Test", "GraphicShader did not preserve the previous generation on failed replacement.");
				return;
			}
			descriptions[0].State.CullMode = CullMode::TwoSided;
#if SE_EDITOR
			const bool replaced = GraphicShader::SaveCompiledAsset(
				_graphicShaderAssetPath, compiled.SLC2Data, descriptions);
			if (replaced)
			{
				_graphicShaderAsset->Reload();
			}
			// Reload is complete only when the replacement asset loads successfully.
			const bool reloaded = replaced && _graphicShaderAsset->WaitForLoaded();
#else
			const bool reloaded = _graphicShaderAsset->InstallCompiled(compiled.SLC2Data, descriptions);
#endif
			if (!reloaded ||
				!_graphicShaderAsset->CreateDrawProgram(SE_TEXT("GBuffer"), GraphicShaderGeometryMode::Static,
					target, {}, _graphicShaderNew) ||
				_graphicShaderNew->GetInstance().GetProgram() == oldProgram)
			{
				LOG_ERROR("Test", "GraphicShader failed to publish an independent new generation.");
				return;
			}

			GPUContext* context = GPUDevice::instance->GetMainContext();
			RenderGeometry geometry = StaticMeshVertexFactory::CreateRenderGeometry(
				_graphicsVertexBuffer, nullptr, nullptr);
			geometry.IndexBuffer = _graphicsIndexBuffer;
			geometry.IndexFormat = PixelFormat::R16_UInt;
			context->SetRenderTarget(_graphicsTarget->View());
			context->SetViewportAndScissors(4.0f, 4.0f);
			if (!_graphicShaderOld->Prepare(*context, geometry))
			{
				LOG_ERROR("Test", "GraphicShader old generation could not prepare geometry.");
				return;
			}
			context->DrawIndexedInstanced(_graphicShaderOld->GetInstance(), 3, 1, 0, 0, 0);
			if (!_graphicShaderNew->Prepare(*context, geometry))
			{
				LOG_ERROR("Test", "GraphicShader new generation could not prepare geometry.");
				return;
			}
			context->DrawIndexedInstanced(_graphicShaderNew->GetInstance(), 3, 1, 0, 0, 0);
			LOG_INFO("Test", "GraphicShader asset selection, failed replacement and old/new indexed draws recorded.");
		}

		void RunSLC2MaterialDrawTest()
		{
			if (_graphicsVertexBuffer == nullptr || _graphicsIndexBuffer == nullptr ||
				_graphicsTarget == nullptr || _bindingTexture == nullptr || _bindingSampler == nullptr ||
				_bindingStructuredBuffer == nullptr || _externalGraphicsConstantBuffer == nullptr)
			{
				LOG_ERROR("Test", "SLC2 Material direct draw needs initialized GPU fixtures.");
				return;
			}

			ShaderCompileRequest request;
			request.ShaderName = SE_TEXT("SLC2MaterialDraw");
			request.SourcePath = SE_TEXT("Shaders/Slang/GraphicShader_MaterialDraw.slang");
			request.Targets = {{ShaderTargetPlatform::Windows, ShaderProfile::Vulkan_SM5, FeatureLevel::SM5}};
			if (!File::ReadAllText(EngineContext::StartupFolder / request.SourcePath, request.SourceCode))
			{
				LOG_ERROR("Test", "SLC2 Material draw shader source is missing.");
				return;
			}
			const ShaderCompileResult compiled = ShadersCompilation::CompileSlang(request);
			if (compiled.Status == ShaderCompileStatus::Failed)
			{
				LOG_ERROR("Test", "SLC2 Material shader compilation failed: {0}", compiled.CompileMessage.Text);
				return;
			}

			GraphicShaderPipeline pipeline;
			pipeline.ProgramId = SE_TEXT("GBuffer");
			pipeline.State = SLC2GPUPipelineState::Description::DefaultNoDepth;
			List<GraphicShaderPipeline> descriptions;
			descriptions.Add(pipeline);
			_slc2MaterialShader = AssetContent::CreateVirtualAsset<GraphicShader>();
			_slc2Material = AssetContent::CreateVirtualAsset<SLC2Material>();
			_slc2MaterialParent = AssetContent::CreateVirtualAsset<SLC2MaterialInstance>();
			_slc2MaterialChild = AssetContent::CreateVirtualAsset<SLC2MaterialInstance>();
			if (!_slc2MaterialShader || !_slc2Material || !_slc2MaterialParent || !_slc2MaterialChild ||
				!_slc2MaterialShader->InstallCompiled(compiled.SLC2Data, descriptions))
			{
				LOG_ERROR("Test", "SLC2 Material virtual assets could not be initialized.");
				return;
			}

			_slc2Material->SetGraphicShader(_slc2MaterialShader.Get());
			const Float4 tint(0.1f, 0.2f, 0.3f, 1.0f);
			const Float4 parentTint(0.7f, 0.2f, 0.1f, 1.0f);
			const Float4 detail(0.2f, 0.3f, 0.4f, 1.0f);
			const Float4 childDetail(0.4f, 0.5f, 0.6f, 1.0f);
			const Float4 localOffset(0.1f, 0.1f, 0.1f, 0.0f);
			const Float4 inactive(1.0f, 0.0f, 1.0f, 1.0f);
			auto& defaults = _slc2Material->GetParameters();
			if (!defaults.SetUniform(SE_TEXT("Material.Tint"), &tint, sizeof(tint)) ||
				!defaults.SetUniform(SE_TEXT("Material.Detail"), &detail, sizeof(detail)) ||
				!defaults.SetConstantBuffer(SE_TEXT("Material.Local"), &localOffset, sizeof(localOffset)) ||
				!defaults.SetUniform(SE_TEXT("Material.NeverUsed"), &inactive, sizeof(inactive)) ||
				!defaults.SetTextureReference(SE_TEXT("Material.NeverUsedTexture"), UID::New()) ||
				!defaults.SetTextureReference(SE_TEXT("Material.Image"), UID::New()) ||
				!_slc2MaterialParent->GetOverrides().SetUniform(
					SE_TEXT("Material.Tint"), &parentTint, sizeof(parentTint)) ||
				!_slc2MaterialChild->GetOverrides().SetUniform(
					SE_TEXT("Material.Detail"), &childDetail, sizeof(childDetail)))
			{
				LOG_ERROR("Test", "SLC2 Material NamePath authoring failed.");
				return;
			}
			_slc2MaterialParent->SetBaseMaterial(_slc2Material.Get());
			if (!_slc2MaterialChild->SetParentInstance(_slc2MaterialParent.Get()) ||
				_slc2MaterialParent->SetParentInstance(_slc2MaterialChild.Get()))
			{
				LOG_ERROR("Test", "SLC2 MaterialInstance ancestry rejected a valid chain or accepted a cycle.");
				return;
			}

			const ShaderCompileTarget target =
				{ShaderTargetPlatform::Windows, ShaderProfile::Vulkan_SM5, FeatureLevel::SM5};
			const List<String> baseDefines = {SE_TEXT("MODE_BASE")};
			const List<String> detailDefines = {SE_TEXT("MODE_DETAIL")};
			SLC2MaterialRuntimeValues baseResources;
			SLC2MaterialRuntimeValues detailResources;
			SLC2MaterialDrawContext baseContext;
			SLC2MaterialDrawContext detailContext;
			baseContext.Material = &baseResources;
			detailContext.Material = &detailResources;
			detailResources.SetTexture(SE_TEXT("Material.Image"), _bindingTexture->View());
			detailResources.SetSampler(SE_TEXT("Material.ImageSampler"), _bindingSampler);
			detailResources.SetBuffer(SE_TEXT("Material.Lookup"), _bindingStructuredBuffer);
			if (!_slc2MaterialChild->PrepareDraw(SE_TEXT("GBuffer"), GraphicShaderGeometryMode::Static,
					target, baseDefines, baseContext, _slc2MaterialBaseDraw) ||
				!_slc2MaterialChild->PrepareDraw(SE_TEXT("GBuffer"), GraphicShaderGeometryMode::Static,
					target, detailDefines, detailContext, _slc2MaterialDetailDraw))
			{
				LOG_ERROR("Test", "SLC2 Material failed to prepare both active variants.");
				return;
			}

			// Check selected values through the public snapshot, not a private material cache.
			const auto hasUniform = [](GraphicShaderDrawProgram& draw, const String& path, const Float4& expected)
			{
				const auto& reflection = draw.GetInstance().GetProgram()->GetReflection();
				ShaderVarLocation location;
				ShaderBindingSnapshot snapshot;
				if (!ShaderNameResolver::Resolve(reflection, path, location) ||
					!draw.GetInstance().BuildSnapshot(snapshot))
					return false;
				const auto* block = snapshot.GetUniformData(static_cast<uint32>(location.UniformBlockIndex));
				return block != nullptr && block->Source == ShaderUniformBlockSource::InternalData &&
					location.UniformByteOffset >= 0 &&
					block->Data.Count() >= location.UniformByteOffset + static_cast<int32>(sizeof(expected)) &&
					Platform::MemoryCompare(block->Data.Get() + location.UniformByteOffset,
						&expected, sizeof(expected)) == 0;
			};
			if (!hasUniform(*_slc2MaterialBaseDraw, SE_TEXT("Material.Tint"), parentTint) ||
				!hasUniform(*_slc2MaterialDetailDraw, SE_TEXT("Material.Detail"), childDetail) ||
				defaults.Find(SE_TEXT("Material.NeverUsed")) == nullptr ||
				defaults.Find(SE_TEXT("Material.NeverUsedTexture")) == nullptr)
			{
				LOG_ERROR("Test", "SLC2 Material variant selection or multi-level NamePath override failed.");
				return;
			}

			// A required value fails only its active variant; restoring it preserves other authored paths.
			defaults.Remove(SE_TEXT("Material.Tint"));
			std::unique_ptr<GraphicShaderDrawProgram> missing;
			if (_slc2Material->PrepareDraw(SE_TEXT("GBuffer"), GraphicShaderGeometryMode::Static,
					target, baseDefines, baseContext, missing) || missing ||
				!_slc2Material->PrepareDraw(SE_TEXT("GBuffer"), GraphicShaderGeometryMode::Static,
					target, detailDefines, detailContext, missing))
			{
				LOG_ERROR("Test", "SLC2 Material rejected an inactive value or accepted a missing active value.");
				return;
			}
			missing.reset();
			defaults.SetUniform(SE_TEXT("Material.Tint"), &tint, sizeof(tint));
			SLC2MaterialRuntimeValues wrongResources = detailResources;
			wrongResources.SetSampler(SE_TEXT("Material.Image"), _bindingSampler);
			SLC2MaterialDrawContext wrongContext = detailContext;
			wrongContext.Material = &wrongResources;
			if (_slc2MaterialChild->PrepareDraw(SE_TEXT("GBuffer"), GraphicShaderGeometryMode::Static,
					target, detailDefines, wrongContext, missing) || missing)
			{
				LOG_ERROR("Test", "SLC2 Material accepted a wrong-kind active resource.");
				return;
			}

			// External ConstantBuffers and all GPU resources are transient Draw inputs, never asset records.
			GPUContext* context = GPUDevice::instance->GetMainContext();
			context->UpdateCB(_externalGraphicsConstantBuffer, &localOffset);
			SLC2MaterialRuntimeValues externalResources = detailResources;
			externalResources.SetConstantBuffer(SE_TEXT("Material.Local"), _externalGraphicsConstantBuffer);
			SLC2MaterialDrawContext externalContext = detailContext;
			externalContext.Material = &externalResources;
			const auto* authoredImage = defaults.Find(SE_TEXT("Material.Image"));
			if (!_slc2MaterialChild->PrepareDraw(SE_TEXT("GBuffer"), GraphicShaderGeometryMode::Static,
					target, detailDefines, externalContext, _slc2MaterialDetailDraw) ||
				authoredImage == nullptr || authoredImage->Texture ||
				defaults.Find(SE_TEXT("Material.Lookup")) != nullptr ||
				defaults.Find(SE_TEXT("Material.ImageSampler")) != nullptr)
			{
				LOG_ERROR("Test", "SLC2 Material transient resource or external CB preparation failed.");
				return;
			}

			SLC2GPUShaderProgram* oldProgram = _slc2MaterialDetailDraw->GetInstance().GetProgram();
			descriptions[0].State.CullMode = CullMode::TwoSided;
			if (!_slc2MaterialShader->InstallCompiled(compiled.SLC2Data, descriptions) ||
				!_slc2MaterialChild->PrepareDraw(SE_TEXT("GBuffer"), GraphicShaderGeometryMode::Static,
					target, detailDefines, detailContext, _slc2MaterialNewDraw) ||
				_slc2MaterialNewDraw->GetInstance().GetProgram() == oldProgram)
			{
				LOG_ERROR("Test", "SLC2 Material failed to select the new GraphicShader generation.");
				return;
			}

			RenderGeometry geometry = StaticMeshVertexFactory::CreateRenderGeometry(
				_graphicsVertexBuffer, nullptr, nullptr);
			geometry.IndexBuffer = _graphicsIndexBuffer;
			geometry.IndexFormat = PixelFormat::R16_UInt;
			context->SetRenderTarget(_graphicsTarget->View());
			context->SetViewportAndScissors(4.0f, 4.0f);
			if (!_slc2MaterialBaseDraw->Prepare(*context, geometry))
			{
				LOG_ERROR("Test", "SLC2 Material base draw could not prepare geometry.");
				return;
			}
			context->DrawIndexedInstanced(_slc2MaterialBaseDraw->GetInstance(), 3, 1, 0, 0, 0);
			if (!_slc2MaterialDetailDraw->Prepare(*context, geometry))
			{
				LOG_ERROR("Test", "SLC2 Material old detail draw could not prepare geometry.");
				return;
			}
			context->DrawIndexedInstanced(_slc2MaterialDetailDraw->GetInstance(), 3, 1, 0, 0, 0);
			if (!_slc2MaterialNewDraw->Prepare(*context, geometry))
			{
				LOG_ERROR("Test", "SLC2 Material new detail draw could not prepare geometry.");
				return;
			}
			context->DrawIndexedInstanced(_slc2MaterialNewDraw->GetInstance(), 3, 1, 0, 0, 0);
			LOG_INFO("Test", "SLC2 Material NamePath variants, inheritance, transient resources and old/new draws recorded.");
		}

		void RunSLC2SystemMaterialDrawTest()
		{
			if (!_slc2Material || !_slc2MaterialShader || _bindingTexture == nullptr ||
				_bindingSampler == nullptr || _externalGraphicsConstantBuffer == nullptr ||
				_graphicsTarget == nullptr || _graphicsVertexBuffer == nullptr || _graphicsIndexBuffer == nullptr)
			{
				LOG_ERROR("Test", "SLC2 system material draw needs the Material and indexed GPU fixtures.");
				return;
			}

			ShaderCompileRequest request;
			request.ShaderName = SE_TEXT("SLC2SystemMaterialDraw");
			request.SourcePath = SE_TEXT("Shaders/Slang/GraphicShader_SystemDraw.slang");
			request.Targets = {{ShaderTargetPlatform::Windows, ShaderProfile::Vulkan_SM5, FeatureLevel::SM5}};
			if (!File::ReadAllText(EngineContext::StartupFolder / request.SourcePath, request.SourceCode))
			{
				LOG_ERROR("Test", "SLC2 system material shader source is missing.");
				return;
			}
			const ShaderCompileResult compiled = ShadersCompilation::CompileSlang(request);
			if (compiled.Status == ShaderCompileStatus::Failed)
			{
				LOG_ERROR("Test", "SLC2 system material shader compilation failed: {0}", compiled.CompileMessage.Text);
				return;
			}
			GraphicShaderPipeline pipeline;
			pipeline.ProgramId = SE_TEXT("GBuffer");
			pipeline.State = SLC2GPUPipelineState::Description::DefaultNoDepth;
			List<GraphicShaderPipeline> pipelines;
			pipelines.Add(pipeline);
			if (!_slc2MaterialShader->InstallCompiled(compiled.SLC2Data, pipelines))
			{
				LOG_ERROR("Test", "SLC2 system material Program could not be installed.");
				return;
			}

			_systemSceneTexture = GPUDevice::instance->CreateTexture(SE_TEXT("SLC2SystemMaterialDraw.SceneB"));
			if (_systemSceneTexture == nullptr ||
				!_systemSceneTexture->Init(GPUTextureDescription::New2D(4, 4, PixelFormat::R8G8B8A8_UNorm)))
			{
				LOG_ERROR("Test", "SLC2 system material second scene texture could not be created.");
				return;
			}

			// Two draws share one uploaded CB while view, draw and scene-texture inputs change.
			GPUContext* gpuContext = GPUDevice::instance->GetMainContext();
			const Float4 shared(0.1f, 0.2f, 0.3f, 0.4f);
			const Float4 viewA(0.2f, 0.0f, 0.0f, 0.0f);
			const Float4 viewB(0.0f, 0.3f, 0.0f, 0.0f);
			const Float4 offsetA(0.1f, 0.0f, 0.0f, 0.0f);
			const Float4 offsetB(0.0f, 0.4f, 0.0f, 0.0f);
			gpuContext->UpdateCB(_externalGraphicsConstantBuffer, &shared);
			SLC2MaterialRuntimeValues perMaterial, perViewA, perViewB, perDrawA, perDrawB, sceneA, sceneB;
			perMaterial.SetConstantBuffer(SE_TEXT("PerMaterial.Shared"), _externalGraphicsConstantBuffer);
			perViewA.SetConstantBuffer(SE_TEXT("PerView.Shared"), _externalGraphicsConstantBuffer);
			perViewA.SetUniform(SE_TEXT("PerView.Ambient"), &viewA, sizeof(viewA));
			perViewB.SetConstantBuffer(SE_TEXT("PerView.Shared"), _externalGraphicsConstantBuffer);
			perViewB.SetUniform(SE_TEXT("PerView.Ambient"), &viewB, sizeof(viewB));
			perDrawA.SetUniform(SE_TEXT("PerDraw.Offset"), &offsetA, sizeof(offsetA));
			perDrawB.SetConstantBuffer(SE_TEXT("PerDraw"), &offsetB, sizeof(offsetB));
			sceneA.SetTexture(SE_TEXT("SceneTexture.Image"), _bindingTexture->View());
			sceneA.SetSampler(SE_TEXT("SceneTexture.ImageSampler"), _bindingSampler);
			sceneB.SetTexture(SE_TEXT("SceneTexture.Image"), _systemSceneTexture->View());
			sceneB.SetSampler(SE_TEXT("SceneTexture.ImageSampler"), _bindingSampler);
			SLC2MaterialDrawContext first, second;
			first.PerMaterial = second.PerMaterial = &perMaterial;
			first.PerView = &perViewA;
			second.PerView = &perViewB;
			first.PerDraw = &perDrawA;
			second.PerDraw = &perDrawB;
			first.SceneTexture = &sceneA;
			second.SceneTexture = &sceneB;
			const ShaderCompileTarget target =
				{ShaderTargetPlatform::Windows, ShaderProfile::Vulkan_SM5, FeatureLevel::SM5};
			const List<String> systemVariant = {SE_TEXT("MODE_SYSTEM")};
			if (!_slc2Material->PrepareDraw(SE_TEXT("GBuffer"), GraphicShaderGeometryMode::Static,
					target, systemVariant, first, _slc2SystemFirstDraw) ||
				!_slc2Material->PrepareDraw(SE_TEXT("GBuffer"), GraphicShaderGeometryMode::Static,
					target, systemVariant, second, _slc2SystemSecondDraw))
			{
				LOG_ERROR("Test", "SLC2 system material could not prepare adjacent draws.");
				return;
			}

			const auto verify = [&](GraphicShaderDrawProgram& draw, const Float4& ambient,
					const Float4& offset, GPUTextureView* image)
			{
				ShaderBindingSnapshot snapshot;
				if (!draw.GetInstance().BuildSnapshot(snapshot)) return false;
				const auto& reflection = draw.GetInstance().GetProgram()->GetReflection();
				ShaderVarLocation viewLocation, drawLocation, imageLocation, viewBlock, materialBlock;
				if (!ShaderNameResolver::Resolve(reflection, SE_TEXT("PerView.Ambient"), viewLocation) ||
					!ShaderNameResolver::Resolve(reflection, SE_TEXT("PerDraw.Offset"), drawLocation) ||
					!ShaderNameResolver::Resolve(reflection, SE_TEXT("SceneTexture.Image"), imageLocation) ||
					!ShaderNameResolver::ResolveUniformBlock(reflection, SE_TEXT("PerView.Shared"), viewBlock) ||
					!ShaderNameResolver::ResolveUniformBlock(reflection, SE_TEXT("PerMaterial.Shared"), materialBlock))
					return false;
				const auto* viewData = snapshot.GetUniformData(static_cast<uint32>(viewLocation.UniformBlockIndex));
				const auto* drawData = snapshot.GetUniformData(static_cast<uint32>(drawLocation.UniformBlockIndex));
				const auto* viewShared = snapshot.GetUniformData(static_cast<uint32>(viewBlock.UniformBlockIndex));
				const auto* materialShared = snapshot.GetUniformData(static_cast<uint32>(materialBlock.UniformBlockIndex));
				const auto* sceneResource = snapshot.GetResource(static_cast<uint32>(imageLocation.LogicalResourceSlotIndex));
				return viewData != nullptr && drawData != nullptr && sceneResource != nullptr &&
					viewLocation.UniformByteOffset >= 0 && drawLocation.UniformByteOffset >= 0 &&
					viewData->Data.Count() >= viewLocation.UniformByteOffset + static_cast<int32>(sizeof(ambient)) &&
					drawData->Data.Count() >= drawLocation.UniformByteOffset + static_cast<int32>(sizeof(offset)) &&
					Platform::MemoryCompare(viewData->Data.Get() + viewLocation.UniformByteOffset, &ambient, sizeof(ambient)) == 0 &&
					Platform::MemoryCompare(drawData->Data.Get() + drawLocation.UniformByteOffset, &offset, sizeof(offset)) == 0 &&
					viewShared != nullptr && viewShared->Source == ShaderUniformBlockSource::ExternalConstantBuffer &&
					viewShared->ExternalBuffer == _externalGraphicsConstantBuffer &&
					materialShared != nullptr && materialShared->Source == ShaderUniformBlockSource::ExternalConstantBuffer &&
					materialShared->ExternalBuffer == _externalGraphicsConstantBuffer && sceneResource->GetTexture() == image;
			};
			if (!verify(*_slc2SystemFirstDraw, viewA, offsetA, _bindingTexture->View()) ||
				!verify(*_slc2SystemSecondDraw, viewB, offsetB, _systemSceneTexture->View()))
			{
				LOG_ERROR("Test", "SLC2 system material mixed values between adjacent draws.");
				return;
			}

			// Failed preparation must not return a previously usable draw or descriptor state.
			std::unique_ptr<GraphicShaderDrawProgram> rejected;
			SLC2MaterialDrawContext missing = first;
			missing.PerDraw = nullptr;
			SLC2MaterialRuntimeValues wrongScene = sceneA;
			wrongScene.SetSampler(SE_TEXT("SceneTexture.Image"), _bindingSampler);
			SLC2MaterialDrawContext wrong = first;
			wrong.SceneTexture = &wrongScene;
			const List<String> unknownVariant = {SE_TEXT("MODE_UNKNOWN")};
			std::unique_ptr<GraphicShaderDrawProgram> unknownSelected;
			if (!_slc2MaterialShader->CreateDrawProgram(SE_TEXT("GBuffer"),
					GraphicShaderGeometryMode::Static, target, unknownVariant, unknownSelected))
			{
				LOG_ERROR("Test", "SLC2 unknown-namespace variant failed before NamePath routing.");
				return;
			}
			ShaderVarLocation unknownLocation, unknownSceneLocation;
			const auto& unknownReflection = unknownSelected->GetInstance().GetProgram()->GetReflection();
			if (!ShaderNameResolver::Resolve(unknownReflection, SE_TEXT("Unknown.Value"), unknownLocation) ||
				!ShaderNameResolver::Resolve(unknownReflection, SE_TEXT("SceneTexture.Image"), unknownSceneLocation))
			{
				LOG_ERROR("Test", "SLC2 unknown-namespace variant lost Unknown.Value or SceneTexture.Image.");
				return;
			}
			// The negative variant must keep a physical set beyond the five legacy stage-indexed slots.
			bool hasExtendedSet = false;
			for (const ShaderIRParameterBlock& block : unknownReflection.GetLayout().ParameterBlocks)
			{
				hasExtendedSet |= block.HasDefaultUniformBuffer && block.DefaultUniformBuffer.Set >= 5;
				for (const ShaderIRRangeBinding& binding : block.RangeBindings)
				{
					for (const ShaderIRDescriptorRange& descriptor : binding.DescriptorRanges)
					{
						hasExtendedSet |= descriptor.Set >= 5;
					}
				}
			}
			if (!hasExtendedSet)
			{
				LOG_ERROR("Test", "SLC2 unknown-namespace variant did not exercise descriptor set index 5 or greater.");
				return;
			}
			unknownSelected.reset();
			if (_slc2Material->PrepareDraw(SE_TEXT("GBuffer"), GraphicShaderGeometryMode::Static,
					target, systemVariant, missing, rejected) || rejected ||
				_slc2Material->PrepareDraw(SE_TEXT("GBuffer"), GraphicShaderGeometryMode::Static,
					target, systemVariant, wrong, rejected) || rejected ||
				_slc2Material->PrepareDraw(SE_TEXT("GBuffer"), GraphicShaderGeometryMode::Static,
					target, unknownVariant, first, rejected) || rejected)
			{
				LOG_ERROR("Test", "SLC2 system material accepted missing, wrong-kind or unknown-namespace values.");
				return;
			}

			RenderGeometry geometry = StaticMeshVertexFactory::CreateRenderGeometry(
				_graphicsVertexBuffer, nullptr, nullptr);
			geometry.IndexBuffer = _graphicsIndexBuffer;
			geometry.IndexFormat = PixelFormat::R16_UInt;
			gpuContext->SetRenderTarget(_graphicsTarget->View());
			gpuContext->SetViewportAndScissors(4.0f, 4.0f);
			if (!_slc2SystemFirstDraw->Prepare(*gpuContext, geometry))
			{
				LOG_ERROR("Test", "SLC2 first system material draw could not prepare geometry.");
				return;
			}
			gpuContext->DrawIndexedInstanced(_slc2SystemFirstDraw->GetInstance(), 3, 1, 0, 0, 0);
			if (!_slc2SystemSecondDraw->Prepare(*gpuContext, geometry))
			{
				LOG_ERROR("Test", "SLC2 second system material draw could not prepare geometry.");
				return;
			}
			gpuContext->DrawIndexedInstanced(_slc2SystemSecondDraw->GetInstance(), 3, 1, 0, 0, 0);
			LOG_INFO("Test", "SLC2 system NamePath sources, shared constant buffers and adjacent draws recorded.");
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

		void RunSLC2InstancedRenderGeometryDrawTest()
		{
			if (GPUDevice::instance == nullptr || GPUDevice::instance->GetMainContext() == nullptr ||
				_graphicsShader == nullptr || _graphicsVertexBuffer == nullptr ||
				_graphicsIndexBuffer == nullptr || _graphicsTarget == nullptr)
			{
				LOG_ERROR("Test", "SLC2 instanced RenderGeometry test requires the indexed draw resources.");
				return;
			}

			ShaderProgramSelection selection;
			selection.ProgramId = SE_TEXT("CompileOnlyVertexInputInstanced");
			selection.Target = {ShaderTargetPlatform::Windows, ShaderProfile::Vulkan_SM5, FeatureLevel::SM5};
			if (!_graphicsShader->CreateProgramInstance(selection, _instancedGraphicsInstance))
			{
				LOG_ERROR("Test", "Failed to create the SLC2 instanced shader program instance.");
				return;
			}
			if (_externalGraphicsConstantBuffer == nullptr ||
				!_instancedGraphicsInstance.SetConstantBuffer(String::Empty, _externalGraphicsConstantBuffer))
			{
				LOG_ERROR("Test", "Instanced ProgramInstance failed to share the external ConstantBuffer.");
				return;
			}

			_instancedPipeline = SLC2GPUPipelineState::New();
			if (_instancedPipeline == nullptr ||
				!_instancedPipeline->Init(SLC2GPUPipelineState::Description::DefaultNoDepth, _instancedGraphicsInstance.GetProgram()))
			{
				LOG_ERROR("Test", "Failed to initialize the SLC2 instanced pipeline state.");
				return;
			}

			StaticMeshInstanceData instances[4] = {};
			for (int32 index = 0; index < ARRAY_SIZE(instances); index++)
			{
				instances[index].InstanceOrigin = Float3(static_cast<float>(index), 0.0f, 0.0f);
				instances[index].InstanceTransform1 = Float3(1.0f, 0.0f, 0.0f);
				instances[index].InstanceTransform2 = Float3(0.0f, 1.0f, 0.0f);
				instances[index].InstanceTransform3 = Float3(0.0f, 0.0f, 1.0f);
				instances[index].InstanceLightmapArea = Half4(0.0f, 0.0f, 1.0f, 1.0f);
			}

			_instanceVertexBuffer = GPUDevice::instance->CreateBuffer(SE_TEXT("SLC2InstancedRenderGeometryTest.InstanceVB"));
			if (_instanceVertexBuffer == nullptr ||
				!_instanceVertexBuffer->Init(StaticMeshVertexFactory::CreateVertexBufferDescription(
					StaticMeshVertexStream::Instance, ARRAY_SIZE(instances), instances)))
			{
				LOG_ERROR("Test", "Failed to create the StaticMesh instance vertex buffer.");
				return;
			}

			RenderGeometry baseGeometry = StaticMeshVertexFactory::CreateRenderGeometry(
				_graphicsVertexBuffer, nullptr, nullptr);
			baseGeometry.IndexBuffer = _graphicsIndexBuffer;
			baseGeometry.IndexFormat = PixelFormat::R16_UInt;
			const uint64 baseLayoutHash = baseGeometry.Layout->GetHash();

			RenderGeometry firstBatchGeometry;
			if (!StaticMeshVertexFactory::CreateRenderGeometryWithInstanceBuffer(
					baseGeometry, _instanceVertexBuffer, 0, firstBatchGeometry) ||
				baseGeometry.FindVertexBuffer(StaticMeshVertexFactory::InstanceSlot) != nullptr ||
				firstBatchGeometry.Layout != baseGeometry.Layout ||
				firstBatchGeometry.Layout->GetHash() != baseLayoutHash)
			{
				LOG_ERROR("Test", "StaticMesh instance geometry changed the base Geometry or LayoutHash.");
				return;
			}

			GPUContext* context = GPUDevice::instance->GetMainContext();
			context->SetRenderTarget(_graphicsTarget->View());
			context->SetViewportAndScissors(4.0f, 4.0f);
			context->SetSLC2State(_instancedPipeline);

			// 实例 Program 必须拒绝缺少 Slot 3 的基础 Geometry，并清除失败状态。
			if (context->BindRenderGeometry(baseGeometry))
			{
				LOG_ERROR("Test", "Instanced SLC2 program accepted Geometry without the instance stream.");
				return;
			}
			context->DrawIndexedInstanced(_instancedGraphicsInstance, 3, 2, 0, 0, 0);

			if (!context->BindRenderGeometry(firstBatchGeometry))
			{
				LOG_ERROR("Test", "Instanced SLC2 program rejected the first instance batch Geometry.");
				return;
			}
			context->DrawIndexedInstanced(_instancedGraphicsInstance, 3, 1, 0, 0, 0);
			context->DrawIndexedInstanced(_instancedGraphicsInstance, 3, 2, 0, 0, 0);

			const uint32 secondBatchOffset = sizeof(StaticMeshInstanceData) * 2;
			RenderGeometry secondBatchGeometry;
			if (!StaticMeshVertexFactory::CreateRenderGeometryWithInstanceBuffer(
					baseGeometry, _instanceVertexBuffer, secondBatchOffset, secondBatchGeometry))
			{
				LOG_ERROR("Test", "Failed to create the second instance batch Geometry.");
				return;
			}
			if (!context->BindRenderGeometry(secondBatchGeometry))
			{
				LOG_ERROR("Test", "Instanced SLC2 program rejected a valid instance Buffer offset.");
				return;
			}
			context->DrawIndexedInstanced(_instancedGraphicsInstance, 3, 2, 0, 0, 0);
			// Offset 后只剩两条记录，超出的实例数量必须在提交 Vulkan Draw 前被拒绝。
			context->DrawIndexedInstanced(_instancedGraphicsInstance, 3, 3, 0, 0, 0);

			// 恢复非实例 Program 和基础 Geometry，确认上一批次的 Slot 3 不成为隐式依赖。
			context->SetSLC2State(_graphicsPipeline);
			if (!context->BindRenderGeometry(baseGeometry))
			{
				LOG_ERROR("Test", "Failed to restore the base Geometry after an instanced draw.");
				return;
			}
			context->DrawIndexedInstanced(_graphicsInstance, 3, 1, 0, 0, 0);
			LOG_INFO("Test", "SLC2 optional instance stream and batched RenderGeometry draw test passed.");
		}

		MaterialSLC2RuntimeData BuildMaterialRuntimeData(const List<byte>& artifact, const MaterialParams& parameters)
		{
			MaterialSLC2RuntimeData data;
			data.SLC2Data = artifact;
			data.BindingProvider = &_materialRuntimeProvider;
			auto addBinding = [&data, &parameters](const StringView& path, const StringView& parameterName, const ShaderProgramBindingKind kind)
			{
				const MaterialParameter* parameter = parameters.Get(parameterName);
				ENGINE_ASSERT(parameter != nullptr);
				MaterialParameterBinding binding;
				binding.ShaderPath = path;
				binding.ParameterID = parameter->GetParameterID();
				binding.BindingKind = kind;
				data.Bindings.Add(MoveTemp(binding));
			};

			addBinding(SE_TEXT("Material.Enabled"), SE_TEXT("Enabled"), ShaderProgramBindingKind::Uniform);
			addBinding(SE_TEXT("Material.LayerIndex"), SE_TEXT("LayerIndex"), ShaderProgramBindingKind::Uniform);
			addBinding(SE_TEXT("Material.UVScale"), SE_TEXT("UVScale"), ShaderProgramBindingKind::Uniform);
			addBinding(SE_TEXT("Material.Emissive"), SE_TEXT("Emissive"), ShaderProgramBindingKind::Uniform);
			addBinding(SE_TEXT("Material.Tint"), SE_TEXT("Tint"), ShaderProgramBindingKind::Uniform);
			addBinding(SE_TEXT("Material.BaseColor"), SE_TEXT("BaseColor"), ShaderProgramBindingKind::Uniform);
			addBinding(SE_TEXT("Material.Roughness"), SE_TEXT("Roughness"), ShaderProgramBindingKind::Uniform);
			addBinding(SE_TEXT("Material.Transform"), SE_TEXT("Transform"), ShaderProgramBindingKind::Uniform);
			addBinding(SE_TEXT("Material.ChannelMask"), SE_TEXT("ChannelMask"), ShaderProgramBindingKind::Uniform);
			addBinding(SE_TEXT("Material.AlbedoTexture"), SE_TEXT("AlbedoTexture"), ShaderProgramBindingKind::Texture);
			addBinding(SE_TEXT("Material.EnvironmentTexture"), SE_TEXT("EnvironmentTexture"), ShaderProgramBindingKind::Texture);
			addBinding(SE_TEXT("Material.NormalTexture"), SE_TEXT("NormalTexture"), ShaderProgramBindingKind::Texture);
			addBinding(SE_TEXT("Material.RuntimeTexture"), SE_TEXT("RuntimeTexture"), ShaderProgramBindingKind::Texture);
			addBinding(SE_TEXT("Material.RuntimeTextureArray"), SE_TEXT("RuntimeTextureArray"), ShaderProgramBindingKind::Texture);
			addBinding(SE_TEXT("Material.RuntimeTextureVolume"), SE_TEXT("RuntimeTextureVolume"), ShaderProgramBindingKind::Texture);
			addBinding(SE_TEXT("Material.RuntimeTextureCube"), SE_TEXT("RuntimeTextureCube"), ShaderProgramBindingKind::Texture);
			addBinding(SE_TEXT("Material.SceneTexture"), SE_TEXT("SceneTexture"), ShaderProgramBindingKind::Texture);
			addBinding(SE_TEXT("Material.TextureGroupSampler"), SE_TEXT("TextureGroupSampler"), ShaderProgramBindingKind::Sampler);
			addBinding(SE_TEXT("Material.GameplayValue"), SE_TEXT("GameplayValue"), ShaderProgramBindingKind::Uniform);
			// GlobalSDF 用同一个 ParameterID 驱动多个不同种类的反射路径。
			addBinding(SE_TEXT("Material.GlobalSDFTexture"), SE_TEXT("GlobalSDF"), ShaderProgramBindingKind::Texture);
			addBinding(SE_TEXT("Material.GlobalSDFConstants"), SE_TEXT("GlobalSDF"), ShaderProgramBindingKind::Uniform);
			addBinding(SE_TEXT("Material.OptionalTexture"), SE_TEXT("OptionalTexture"), ShaderProgramBindingKind::Texture);

			const ShaderCompileTarget target = {ShaderTargetPlatform::Windows, ShaderProfile::Vulkan_SM5, FeatureLevel::SM5};
			auto addProgram = [&data, &target](const DrawPass pass, const MaterialGeometryMode mode, const StringView& programID)
			{
				MaterialProgramContract contract;
				contract.Pass = pass;
				contract.GeometryMode = mode;
				contract.Selection.ProgramId = programID;
				contract.Selection.Target = target;
				contract.PipelineDescription = SLC2GPUPipelineState::Description::DefaultNoDepth;
				data.Programs.Add(MoveTemp(contract));
			};

			addProgram(DrawPass::GBuffer, MaterialGeometryMode::Static, SE_TEXT("MaterialDeferredStatic"));
			addProgram(DrawPass::GBuffer, MaterialGeometryMode::Instanced, SE_TEXT("MaterialDeferredInstanced"));
			addProgram(DrawPass::Forward, MaterialGeometryMode::Static, SE_TEXT("MaterialForwardStatic"));
			addProgram(DrawPass::Forward, MaterialGeometryMode::Instanced, SE_TEXT("MaterialForwardInstanced"));
			return data;
		}

		SerializedMaterialParam& AddMaterialRuntimeParameter(List<SerializedMaterialParam>& parameters, const MaterialParameterType type, const StringView& name)
		{
			SerializedMaterialParam& parameter = parameters.AddOne();
			parameter.Type = type;
			parameter.ID = UID::New();
			parameter.IsPublic = true;
			parameter.Override = true;
			parameter.Name = name;
			return parameter;
		}

		void RunMaterialSLC2RuntimeTest()
		{
			if (GPUDevice::instance == nullptr)
			{
				LOG_ERROR("Test", "Material SLC2 runtime test requires a GPU device.");
				return;
			}

			ShaderCompileRequest request;
			request.ShaderName = SE_TEXT("MaterialRuntimeTest");
			request.SourcePath = SE_TEXT("Shaders/Slang/CompileOnly_MaterialRuntime.slang");
			request.Targets = {
				{ShaderTargetPlatform::Windows, ShaderProfile::Vulkan_SM5, FeatureLevel::SM5},
			};
			if (!File::ReadAllText(EngineContext::StartupFolder / request.SourcePath, request.SourceCode))
			{
				LOG_ERROR("Test", "Failed to read the Material SLC2 runtime shader source.");
				return;
			}
			ShaderCompileResult result = ShadersCompilation::CompileSlang(request);
			if (result.Status == ShaderCompileStatus::Failed)
			{
				LOG_ERROR("Test", "{0}", result.CompileMessage.Text);
				return;
			}

			List<SerializedMaterialParam> serializedParameters;
			AddMaterialRuntimeParameter(serializedParameters, MaterialParameterType::Bool, SE_TEXT("Enabled")).AsBool = true;
			AddMaterialRuntimeParameter(serializedParameters, MaterialParameterType::Integer, SE_TEXT("LayerIndex")).AsInteger = 2;
			AddMaterialRuntimeParameter(serializedParameters, MaterialParameterType::Vector2, SE_TEXT("UVScale")).AsFloat2 = Float2::One;
			AddMaterialRuntimeParameter(serializedParameters, MaterialParameterType::Vector3, SE_TEXT("Emissive")).AsFloat3 = Float3::One;
			SerializedMaterialParam& tint = AddMaterialRuntimeParameter(serializedParameters, MaterialParameterType::Vector4, SE_TEXT("Tint"));
			*(Float4*)&tint.AsData = Float4::One;
			AddMaterialRuntimeParameter(serializedParameters, MaterialParameterType::Color, SE_TEXT("BaseColor")).AsColor = Colors::Red;
			AddMaterialRuntimeParameter(serializedParameters, MaterialParameterType::Float, SE_TEXT("Roughness")).AsFloat = 0.5f;
			SerializedMaterialParam& transform = AddMaterialRuntimeParameter(serializedParameters, MaterialParameterType::Matrix, SE_TEXT("Transform"));
			*(Matrix*)&transform.AsData = Matrix::Identity;
			AddMaterialRuntimeParameter(serializedParameters, MaterialParameterType::ChannelMask, SE_TEXT("ChannelMask")).AsInteger = 1;
			AddMaterialRuntimeParameter(serializedParameters, MaterialParameterType::Texture, SE_TEXT("AlbedoTexture"));
			AddMaterialRuntimeParameter(serializedParameters, MaterialParameterType::CubeTexture, SE_TEXT("EnvironmentTexture"));
			AddMaterialRuntimeParameter(serializedParameters, MaterialParameterType::NormalMap, SE_TEXT("NormalTexture"));
			AddMaterialRuntimeParameter(serializedParameters, MaterialParameterType::GPUTexture, SE_TEXT("RuntimeTexture"));
			AddMaterialRuntimeParameter(serializedParameters, MaterialParameterType::GPUTextureArray, SE_TEXT("RuntimeTextureArray"));
			AddMaterialRuntimeParameter(serializedParameters, MaterialParameterType::GPUTextureVolume, SE_TEXT("RuntimeTextureVolume"));
			AddMaterialRuntimeParameter(serializedParameters, MaterialParameterType::GPUTextureCube, SE_TEXT("RuntimeTextureCube"));
			AddMaterialRuntimeParameter(serializedParameters, MaterialParameterType::SceneTexture, SE_TEXT("SceneTexture")).AsInteger = 0;
			AddMaterialRuntimeParameter(serializedParameters, MaterialParameterType::TextureGroupSampler, SE_TEXT("TextureGroupSampler")).AsInteger = 0;
			AddMaterialRuntimeParameter(serializedParameters, MaterialParameterType::GameplayGlobal, SE_TEXT("GameplayValue"));
			AddMaterialRuntimeParameter(serializedParameters, MaterialParameterType::GlobalSDF, SE_TEXT("GlobalSDF"));
			AddMaterialRuntimeParameter(serializedParameters, MaterialParameterType::Texture, SE_TEXT("OptionalTexture"));

			BytesContainer serializedData;
			MaterialParams::Save(serializedData, &serializedParameters);
			MemoryReadStream parameterStream(serializedData.Get(), serializedData.Length());
			MaterialParams parametersA;
			if (parametersA.Load(&parameterStream))
			{
				LOG_ERROR("Test", "Failed to load Material SLC2 runtime test parameters.");
				return;
			}
			MaterialParams parametersB;
			parametersA.Clone(parametersB);
			const MaterialParameter* baseColor = parametersA.Get(SE_TEXT("BaseColor"));
			if (baseColor == nullptr || parametersB.Get(baseColor->GetParameterID()) == nullptr ||
				parametersB.Get(baseColor->GetParameterID())->TrySetValue(Colors::Blue) != MaterialParameterSetResult::Success)
			{
				LOG_ERROR("Test", "Material SLC2 runtime test could not create an isolated parameter override.");
				return;
			}
			// A non-overriding leaf value must fall through to the base parameter set.
			parametersB.Get(baseColor->GetParameterID())->SetIsOverride(false);
			List<const MaterialParams*> parameterChainA;
			parameterChainA.Add(&parametersA);
			List<const MaterialParams*> parameterChainB;
			parameterChainB.Add(&parametersB);
			parameterChainB.Add(&parametersA);

			MaterialInfo info;
			String error;
            SharedPtr<MaterialRuntimeGeneration> generation1 = MaterialRuntimeGeneration::Create(
				BuildMaterialRuntimeData(result.SLC2Data, parametersA), info, parametersA, error);
			if (generation1 == nullptr)
			{
				LOG_ERROR("Test", "Failed to create Material Runtime Generation: {0}", error);
				return;
			}

			MaterialDrawContext context;
			context.Pass = DrawPass::GBuffer;
			context.GeometryMode = MaterialGeometryMode::Static;
			MaterialProgramInstanceCache cacheA;
			MaterialProgramInstanceCache cacheB;
			MaterialSLC2Binding bindingA;
			MaterialSLC2Binding bindingB;
			if (!generation1->Prepare(context, parameterChainA, cacheA, bindingA) ||
				!generation1->Prepare(context, parameterChainB, cacheB, bindingB) ||
				bindingA.ProgramInstance == nullptr || bindingB.ProgramInstance == nullptr ||
				bindingA.ProgramInstance == bindingB.ProgramInstance || bindingA.Pipeline != bindingB.Pipeline)
			{
				LOG_ERROR("Test", "Material instances did not isolate ProgramInstance state while sharing the Pipeline.");
				return;
			}

			ShaderVar baseColorVar;
			ShaderBindingSnapshot inheritedSnapshot;
			if (!bindingB.ProgramInstance->FindVar(SE_TEXT("Material.BaseColor"), baseColorVar) ||
				!bindingB.ProgramInstance->BuildSnapshot(inheritedSnapshot))
			{
				LOG_ERROR("Test", "Material SLC2 runtime could not inspect the resolved instance override.");
				return;
			}
			const ShaderVarLocation& baseColorLocation = baseColorVar.GetLocation();
			const ShaderBindingUniformData* inheritedData = inheritedSnapshot.GetUniformData(baseColorLocation.UniformBlockIndex);
			const Color expectedInheritedColor = Colors::Red;
			if (inheritedData == nullptr ||
				baseColorLocation.UniformByteOffset < 0 ||
				static_cast<uint32>(baseColorLocation.UniformByteOffset) + sizeof(expectedInheritedColor) > static_cast<uint32>(inheritedData->Data.Count()) ||
				Platform::MemoryCompare(inheritedData->Data.Get() + baseColorLocation.UniformByteOffset, &expectedInheritedColor, sizeof(expectedInheritedColor)) != 0)
			{
				LOG_ERROR("Test", "Material SLC2 runtime did not inherit the base parameter value.");
				return;
			}

			parametersB.Get(baseColor->GetParameterID())->SetIsOverride(true);
			if (!generation1->Prepare(context, parameterChainB, cacheB, bindingB))
			{
				LOG_ERROR("Test", "Material SLC2 runtime rejected a valid instance override.");
				return;
			}
			ShaderBindingSnapshot overrideSnapshot;
			if (!bindingB.ProgramInstance->BuildSnapshot(overrideSnapshot))
			{
				LOG_ERROR("Test", "Material SLC2 runtime could not freeze the instance override.");
				return;
			}
			const ShaderBindingUniformData* overrideData = overrideSnapshot.GetUniformData(baseColorLocation.UniformBlockIndex);
			const Color expectedOverrideColor = Colors::Blue;
			if (overrideData == nullptr ||
				static_cast<uint32>(baseColorLocation.UniformByteOffset) + sizeof(expectedOverrideColor) > static_cast<uint32>(overrideData->Data.Count()) ||
				Platform::MemoryCompare(overrideData->Data.Get() + baseColorLocation.UniformByteOffset, &expectedOverrideColor, sizeof(expectedOverrideColor)) != 0)
			{
				LOG_ERROR("Test", "Material SLC2 runtime did not apply the instance override value.");
				return;
			}

			context.Pass = DrawPass::Forward;
			context.GeometryMode = MaterialGeometryMode::Instanced;
			if (!generation1->Prepare(context, parameterChainA, cacheA, bindingA) || bindingA.GenerationID != generation1->GetID())
			{
				LOG_ERROR("Test", "Material SLC2 runtime failed to select the Forward/Instanced Program contract.");
				return;
			}

			SharedPtr<MaterialRuntimeGeneration> generation2 = MaterialRuntimeGeneration::Create(
				BuildMaterialRuntimeData(result.SLC2Data, parametersA), info, parametersA, error);
			if (generation2 == nullptr || generation2->GetID() == generation1->GetID() ||
				!generation2->Prepare(context, parameterChainA, cacheA, bindingA) || bindingA.GenerationID != generation2->GetID())
			{
				LOG_ERROR("Test", "Material Runtime Generation replacement did not invalidate the instance cache: {0}", error);
				return;
			}

			MaterialSLC2RuntimeData invalidData = BuildMaterialRuntimeData(result.SLC2Data, parametersA);
			invalidData.Bindings.Add(invalidData.Bindings[0]);
			if (MaterialRuntimeGeneration::Create(MoveTemp(invalidData), info, parametersA, error) != nullptr)
			{
				LOG_ERROR("Test", "Material Runtime Generation accepted a duplicate global binding path.");
				return;
			}

			context.Pass = DrawPass::Depth;
			bindingA.ProgramInstance = bindingB.ProgramInstance;
			bindingA.Pipeline = bindingB.Pipeline;
			if (generation2->Prepare(context, parameterChainA, cacheA, bindingA) || bindingA.ProgramInstance != nullptr || bindingA.Pipeline != nullptr)
			{
				LOG_ERROR("Test", "Failed Material SLC2 preparation did not clear its output atomically.");
				return;
			}

			LOG_INFO("Test", "Material SLC2 RuntimeGeneration, global binding and instance isolation test passed.");
		}

		void RunSLC2AppendConsumeCounterTest()
		{
			if (GPUDevice::instance == nullptr || _bindingAppendBuffer == nullptr || !_bindingAppendBuffer->IsAllocated())
			{
				LOG_ERROR("Test", "SLC2 Append/Consume counter test requires an initialized GPU buffer.");
				return;
			}

			GPUContext* context = GPUDevice::instance->GetMainContext();
			if (context == nullptr)
			{
				LOG_ERROR("Test", "SLC2 Append/Consume counter test requires a GPU main context.");
				return;
			}

			ShaderCompileRequest request;
			request.ShaderName = SE_TEXT("AppendConsumeCounter");
			request.SourcePath = SE_TEXT("Shaders/Slang/CompileOnly_AppendConsume.slang");
			request.Targets = {
				{ShaderTargetPlatform::Windows, ShaderProfile::Vulkan_SM5, FeatureLevel::SM5},
			};
			if (!File::ReadAllText(
				EngineContext::StartupFolder + SE_TEXT("/Shaders/Slang/CompileOnly_AppendConsume.slang"),
				request.SourceCode))
			{
				LOG_ERROR("Test", "Failed to read SLC2 Append/Consume counter shader source.");
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
				LOG_ERROR("Test", "SLC2 Append/Consume counter artifact is invalid: {0}", error);
				return;
			}

			const SLC2ProgramRecord* appendProgram = FindProgram(artifact, SE_TEXT("CompileOnlyCounterAppend"));
			const SLC2ProgramRecord* consumeProgram = FindProgram(artifact, SE_TEXT("CompileOnlyCounterConsume"));
			if (appendProgram == nullptr || consumeProgram == nullptr || appendProgram->Targets.Count() != 1 ||
				consumeProgram->Targets.Count() != 1 || appendProgram->Targets[0].Variants.Count() != 1 ||
				consumeProgram->Targets[0].Variants.Count() != 1)
			{
				LOG_ERROR("Test", "SLC2 Append/Consume counter program topology is invalid.");
				return;
			}

			// 每个 Program 只检查当前 Variant 的 active 资源。Append/Consume 需要附属 counter，
			// 普通 RWStructuredBuffer 仍只对应一个 primary descriptor。
			const ShaderReflectionIR& appendLayout = appendProgram->Targets[0].Variants[0].Layout;
			const ShaderReflectionIR& consumeLayout = consumeProgram->Targets[0].Variants[0].Layout;
			const ShaderIRParameterBlock* appendRoot = appendLayout.RootBlockId < static_cast<uint32>(appendLayout.ParameterBlocks.Count())
				? &appendLayout.ParameterBlocks[appendLayout.RootBlockId]
				: nullptr;
			const ShaderIRParameterBlock* consumeRoot = consumeLayout.RootBlockId < static_cast<uint32>(consumeLayout.ParameterBlocks.Count())
				? &consumeLayout.ParameterBlocks[consumeLayout.RootBlockId]
				: nullptr;
			ShaderProgramReflection appendReflection;
			ShaderProgramReflection consumeReflection;
			ShaderVarLocation appendLocation;
			ShaderVarLocation consumeLocation;
			ShaderVarLocation resultLocation;
			if (appendRoot == nullptr || consumeRoot == nullptr || !appendReflection.Initialize(appendLayout) ||
				!consumeReflection.Initialize(consumeLayout) ||
				!ShaderNameResolver::Resolve(appendReflection, SE_TEXT("AppendValues"), appendLocation) ||
				!ShaderNameResolver::Resolve(consumeReflection, SE_TEXT("ConsumeValues"), consumeLocation) ||
				!ShaderNameResolver::Resolve(consumeReflection, SE_TEXT("ConsumeResult"), resultLocation) ||
				appendLocation.ResourceRangeIndex < 0 ||
				appendLocation.ResourceRangeIndex >= appendRoot->RangeBindings.Count() ||
				consumeLocation.ResourceRangeIndex < 0 ||
				consumeLocation.ResourceRangeIndex >= consumeRoot->RangeBindings.Count() ||
				resultLocation.ResourceRangeIndex < 0 ||
				resultLocation.ResourceRangeIndex >= consumeRoot->RangeBindings.Count())
			{
				LOG_ERROR("Test", "SLC2 Append/Consume active resource reflection is invalid.");
				return;
			}

			const ShaderIRRangeBinding& appendBinding = appendRoot->RangeBindings[appendLocation.ResourceRangeIndex];
			const ShaderIRRangeBinding& consumeBinding = consumeRoot->RangeBindings[consumeLocation.ResourceRangeIndex];
			const ShaderIRRangeBinding& resultBinding = consumeRoot->RangeBindings[resultLocation.ResourceRangeIndex];
			if (appendBinding.DescriptorRanges.Count() != 2 ||
				appendBinding.DescriptorRanges[0].Role != SE_TEXT("primary") ||
				appendBinding.DescriptorRanges[1].Role != SE_TEXT("counter") ||
				consumeBinding.DescriptorRanges.Count() != 2 ||
				consumeBinding.DescriptorRanges[0].Role != SE_TEXT("primary") ||
				consumeBinding.DescriptorRanges[1].Role != SE_TEXT("counter") ||
				resultBinding.DescriptorRanges.Count() != 1 ||
				resultBinding.DescriptorRanges[0].Role != SE_TEXT("primary"))
			{
				LOG_ERROR("Test", "SLC2 counter and ordinary structured-buffer descriptor roles are invalid.");
				return;
			}

			_counterShader = GPUDevice::instance->CreateSLC2Shader(SE_TEXT("SLC2AppendConsumeCounterTest"));
			if (_counterShader == nullptr || !_counterShader->Load(result.SLC2Data))
			{
				LOG_ERROR("Test", "SLC2 Append/Consume counter shader could not be loaded.");
				return;
			}

			ShaderProgramSelection selection;
			selection.Target = {ShaderTargetPlatform::Windows, ShaderProfile::Vulkan_SM5, FeatureLevel::SM5};
			selection.ProgramId = SE_TEXT("CompileOnlyCounterAppend");
			if (!_counterShader->CreateProgramInstance(selection, _counterAppendInstance))
			{
				LOG_ERROR("Test", "SLC2 Append counter program instance could not be created.");
				return;
			}
			selection.ProgramId = SE_TEXT("CompileOnlyCounterConsume");
			if (!_counterShader->CreateProgramInstance(selection, _counterConsumeInstance))
			{
				LOG_ERROR("Test", "SLC2 Consume counter program instance could not be created.");
				return;
			}
			// Append/Consume requires the parent buffer to own a counter; an ordinary writable
			// structured buffer must be rejected before it can replace a valid logical slot.
			if (_counterAppendInstance.SetBuffer(SE_TEXT("AppendValues"), _bindingRWStructuredBuffer) ||
				_counterConsumeInstance.SetBuffer(SE_TEXT("ConsumeValues"), _bindingRWStructuredBuffer))
			{
				LOG_ERROR("Test", "SLC2 Append/Consume accepted a structured buffer without counter capability.");
				return;
			}

			_counterResultBuffer = GPUDevice::instance->CreateBuffer(SE_TEXT("SLC2CounterTest.Result"));
			_counterReadbackBuffer = GPUDevice::instance->CreateBuffer(SE_TEXT("SLC2CounterTest.Readback"));
			const GPUBufferDescription readbackDescription = GPUBufferDescription::Buffer(
				sizeof(uint32), GPUBufferFlags::None, PixelFormat::Undefined, nullptr, sizeof(uint32),
				GPUResourceUsage::StagingReadback);
			if (_counterResultBuffer == nullptr ||
				!_counterResultBuffer->Init(GPUBufferDescription::Structured(1, sizeof(uint32), true)) ||
				_counterReadbackBuffer == nullptr || !_counterReadbackBuffer->Init(readbackDescription) ||
				!_counterAppendInstance.SetBuffer(SE_TEXT("AppendValues"), _bindingAppendBuffer) ||
				!_counterConsumeInstance.SetBuffer(SE_TEXT("ConsumeValues"), _bindingAppendBuffer) ||
				!_counterConsumeInstance.SetBuffer(SE_TEXT("ConsumeResult"), _counterResultBuffer))
			{
				LOG_ERROR("Test", "SLC2 Append/Consume counter resources could not be initialized or bound.");
				return;
			}

			// Both programs bind the same logical parent buffer. The backend resolves its hidden
			// counter descriptor, so three appends followed by one consume must leave count two.
			context->ResetCounter(_bindingAppendBuffer);
			context->Dispatch(_counterAppendInstance, 3, 1, 1);
			context->Dispatch(_counterConsumeInstance, 1, 1, 1);
			context->CopyCounter(_counterReadbackBuffer, 0, _bindingAppendBuffer);
			context->Flush();
			GPUDevice::instance->WaitForGPU();

			BytesContainer counterData;
			if (_counterReadbackBuffer->GetData(counterData) || counterData.Length() != sizeof(uint32) ||
				*reinterpret_cast<const uint32*>(counterData.Get()) != 2)
			{
				LOG_ERROR("Test", "SLC2 Append/Consume counter readback did not equal 2.");
				return;
			}

			LOG_INFO("Test", "SLC2 Append/Consume parent-buffer counter binding test passed.");
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

			context->Dispatch(_instance, 1, 1, 1);
		}

	private:
		bool _hasRun = false;
		uint32 _testLogFrame = 0;
		SLC2GPUShader* _shader = nullptr;
		ShaderProgramInstance _instance;
		SLC2GPUShader* _graphicsShader = nullptr;
		AssetRef<GraphicShader> _graphicShaderAsset;
		String _graphicShaderAssetPath;
		std::unique_ptr<GraphicShaderDrawProgram> _graphicShaderOld;
		std::unique_ptr<GraphicShaderDrawProgram> _graphicShaderNew;
		AssetRef<GraphicShader> _slc2MaterialShader;
		AssetRef<SLC2Material> _slc2Material;
		AssetRef<SLC2MaterialInstance> _slc2MaterialParent;
		AssetRef<SLC2MaterialInstance> _slc2MaterialChild;
		std::unique_ptr<GraphicShaderDrawProgram> _slc2MaterialBaseDraw;
		std::unique_ptr<GraphicShaderDrawProgram> _slc2MaterialDetailDraw;
		std::unique_ptr<GraphicShaderDrawProgram> _slc2MaterialNewDraw;
		std::unique_ptr<GraphicShaderDrawProgram> _slc2SystemFirstDraw;
		std::unique_ptr<GraphicShaderDrawProgram> _slc2SystemSecondDraw;
		ShaderProgramInstance _graphicsInstance;
		SLC2GPUPipelineState* _graphicsPipeline = nullptr;
		GPUBuffer* _graphicsVertexBuffer = nullptr;
		GPUBuffer* _graphicsIndexBuffer = nullptr;
		GPUConstantBuffer* _externalGraphicsConstantBuffer = nullptr;
		GPUBuffer* _alternateVertexBuffer = nullptr;
		GPUTexture* _graphicsTarget = nullptr;
		GPUTexture* _bindingTexture = nullptr;
		GPUTexture* _systemSceneTexture = nullptr;
		GPUSampler* _bindingSampler = nullptr;
		GPUBuffer* _bindingTypedBuffer = nullptr;
		GPUBuffer* _bindingRWTypedBuffer = nullptr;
		GPUBuffer* _bindingStructuredBuffer = nullptr;
		GPUBuffer* _bindingRWStructuredBuffer = nullptr;
		GPUBuffer* _bindingAppendBuffer = nullptr;
		GPUBuffer* _bindingRawBuffer = nullptr;
		GPUBuffer* _bindingRWRawBuffer = nullptr;
		SLC2GPUShader* _counterShader = nullptr;
		ShaderProgramInstance _counterAppendInstance;
		ShaderProgramInstance _counterConsumeInstance;
		GPUBuffer* _counterResultBuffer = nullptr;
		GPUBuffer* _counterReadbackBuffer = nullptr;
		Model* _meshTestModel = nullptr;
		BytesContainer _attributesData;
		Threading::Task* _attributesDownloadTask = nullptr;
		Threading::Task* _finishedAttributesDownloadTask = nullptr;
		SLC2GPUPipelineState* _systemOnlyPipeline = nullptr;
		SLC2GPUShader* _sparseShader = nullptr;
		ShaderProgramInstance _sparseInstance;
		SLC2GPUPipelineState* _sparsePipeline = nullptr;
		GPUBuffer* _sparseUVBuffer = nullptr;
		ShaderProgramInstance _instancedGraphicsInstance;
		SLC2GPUPipelineState* _instancedPipeline = nullptr;
		GPUBuffer* _instanceVertexBuffer = nullptr;
		MaterialRuntimeTestProvider _materialRuntimeProvider;
	};

	ENGINE_SYSTEM_REGISTER(TestSystem)
}
