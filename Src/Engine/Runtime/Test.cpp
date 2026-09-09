#include <Runtime/Core/Platform/File.h>
#include <Runtime/Core/Memory/Memory.h>
#include <Runtime/Core/Systems.h>
#include <Runtime/EngineContext.h>
#include <Runtime/Graphics/GPUContext.h>
#include <Runtime/Graphics/GPUDevice.h>
#include <Runtime/Graphics/Shaders/SLC2GPUShader.h>
#include <Runtime/Graphics/Shaders/ShaderProgramInstance.h>
#include <Runtime/ShaderCompilation/ShadersCompilation.h>
#include <Runtime/ShaderCompilation/Slang/SLC2/SLC2Reader.h>

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
				return;
			}

			_hasRun = true;
			RunSLC2VertexInputRoundTripTest();
			RunSlangVariantCacheGenerationTest();
			RunSLC2ComputeDispatchSmokeTest();
		}

		void OnDispose() override
		{
			if (_shader != nullptr)
			{
				Delete(_shader);
				_shader = nullptr;
			}
		}

	private:
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
			if (program.VertexBufferLayout.Bindings.Count() != expectedCount ||
				program.VertexBufferLayout.Elements.Count() != expectedCount)
			{
				return false;
			}
			if (expectInput)
			{
				const SLC2VertexBufferBinding& binding = program.VertexBufferLayout.Bindings[0];
				const SLC2VertexInputElement& element = program.VertexBufferLayout.Elements[0];
				if (binding.Slot != 0 || binding.Stride != 12 ||
					binding.InputRate != SLC2VertexInputRate::PerVertex || binding.InstanceStepRate != 0 ||
					element.Semantic != SE_TEXT("POSITION") || element.SemanticIndex != 0 ||
					element.Format != PixelFormat::R32G32B32_Float || element.Slot != 0 || element.Offset != 0)
				{
					return false;
				}
			}

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
			if (!SLC2Reader::Read(result.SLC2Data, artifact, error) || artifact.Version != 3)
			{
				LOG_ERROR("Test", "SLC2 v3 vertex input round-trip failed: {0}", error);
				return;
			}

			const SLC2ProgramRecord* positionProgram =
				FindProgram(artifact, SE_TEXT("CompileOnlyVertexInputMinimal"));
			const SLC2ProgramRecord* systemOnlyProgram = FindProgram(artifact, SE_TEXT("CompileOnlyVertexSystemOnly"));
			if (positionProgram == nullptr || systemOnlyProgram == nullptr ||
				!ValidateVertexProgram(*positionProgram, true) || !ValidateVertexProgram(*systemOnlyProgram, false))
			{
				LOG_ERROR("Test", "SLC2 v3 vertex input content does not match the minimal contract.");
				return;
			}

			// 空数组是合法值，但字段本身属于 v3 必需结构；通过修改真实产物字段名覆盖两条缺失路径。
			if (!ReaderRejectsRenamedField(result.SLC2Data, "vertexBufferLayout", "missingVertexBufferLayout") ||
				!ReaderRejectsRenamedField(result.SLC2Data, "vertexInputSignature", "missingVertexInputSignature"))
			{
				LOG_ERROR("Test", "SLC2 reader unexpectedly accepted a cache with missing vertex input fields.");
				return;
			}

			const char legacyHeader[] =
				"{\"format\":\"SLC2\",\"version\":2,\"compilerBuildTag\":\"test\",\"programs\":[]}";
			SLC2Artifact legacyArtifact;
			String legacyError;
			if (!SLC2Reader::Read(reinterpret_cast<const byte*>(legacyHeader),
								 static_cast<int32>(sizeof(legacyHeader) - 1),
								 legacyArtifact,
								 legacyError))
			{
				LOG_ERROR("Test", "SLC2 reader unexpectedly accepted a v2 cache.");
			}
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

			File::WriteAllBytes(EngineContext::StartupFolder + SE_TEXT("/Shaders/Slang/CompileOnly_Variants.slang.slangcache"), result.SLC2Data.Get(), result.SLC2Data.Count());
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
			if (computeProgram == nullptr || computeProgram->VertexBufferLayout.Bindings.HasItems() ||
				computeProgram->VertexBufferLayout.Elements.HasItems())
			{
				LOG_ERROR("Test", "SLC2 compute program must have an empty vertex buffer layout.");
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
		SLC2GPUShader* _shader = nullptr;
		ShaderProgramInstance _instance;
	};

	ENGINE_SYSTEM_REGISTER(TestSystem)
}
