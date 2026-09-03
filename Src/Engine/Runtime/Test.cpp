#include <Runtime/Core/Platform/File.h>
#include <Runtime/Core/Memory/Memory.h>
#include <Runtime/Core/Systems.h>
#include <Runtime/EngineContext.h>
#include <Runtime/Graphics/GPUContext.h>
#include <Runtime/Graphics/GPUDevice.h>
#include <Runtime/Graphics/Shaders/SLC2GPUShader.h>
#include <Runtime/Graphics/Shaders/ShaderProgramInstance.h>
#include <Runtime/ShaderCompilation/ShadersCompilation.h>

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


			_shader = GPUDevice::instance->CreateSLC2Shader(SE_TEXT("SLC2RuntimeSmokeTest"));
			if (_shader == nullptr)
			{
				LOG_ERROR("Test", "SLC2 runtime smoke test failed. Cannot create SLC2 GPU shader.");
				return;
			}

			String error;
            if (!_shader->Load(result.SLC2Data))
			{
				LOG_ERROR("Test", "SLC2 runtime smoke test failed. Cannot load SLC2 cache}");
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
