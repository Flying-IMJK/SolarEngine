
#include "FXAA.h"

#include "Runtime/Core/Profiler/Profiler.h"
#include "Runtime/Graphics/GPUContext.h"
#include "Runtime/Graphics/Textures/GPUTexture.h"
#include "Runtime/Resource/Assets/SLC2Materials/GraphicShader.h"

namespace SE
{
	String FXAA::ToString() const
	{
		return SE_TEXT("FXAA");
	}

	bool FXAA::Init()
	{
		m_Shader = AssetContent::LoadAsyncInternal<GraphicShader>(SE_TEXT("Shaders/PostProcess/FXAA"));
		if (m_Shader == nullptr)
		{
			return false;
		}
#if COMPILE_WITH_DEV_ENV
		m_Shader.Get()->OnReloading.Bind<FXAA, &FXAA::OnShaderReloading>(this);
#endif

		return true;
	}

	bool FXAA::SetupResources()
	{
		if (!m_Shader.HasReference() || !m_Shader->WaitForLoaded())
		{
			return false;
		}

		GPUDevice* device = GPUDevice::instance;

		m_PipelineState = device->CreateSLC2PipelineState();
        m_ProgramInstance       = New<ShaderProgramInstance>();


		return true;
	}

	void FXAA::Dispose()
	{
		// Base
		RendererPass::Dispose();

		Delete(m_ProgramInstance);
        Delete(m_PipelineState);
		m_Shader = nullptr;
	}

	void FXAA::Render(RenderContext& renderContext, GPUTexture* input, GPUTextureView* output)
	{
		auto context = renderContext.gpuContext;
		if (!CheckIfSkipPass())
		{
			// Resources are missing. Do not perform rendering, just copy input frame.
			context->SetRenderTarget(output);
			context->Draw(input);
			return;
		}
		PROFILE_GPU_CPU("Fast Approximate Antialiasing");

		const auto qualityLevel = Math::Clamp(static_cast<int32>(1), 0, static_cast<int32>(Quality::MAX) - 1);

		String macro;
        switch (qualityLevel)
        {
            case 0:
                macro = SE_TEXT("QUALITY0");
                break;
            case 1:
                macro = SE_TEXT("QUALITY1");
                break;
            case 2:
                macro = SE_TEXT("QUALITY2");
                break;
            case 3:
                macro = SE_TEXT("QUALITY3");
                break;
        }

		SLC2GPUShaderProgram* program;
        if (!m_Shader->SelectProgram(SE_TEXT("FXAA"), {macro}, program))
        {
            return;
        }

        if (m_ProgramInstance->GetProgram() == nullptr || m_ProgramInstance->GetProgram() != program)
        {
            m_ProgramInstance->Initialize(program);

			m_PipelineState->Init(SLC2GPUPipelineState::Description::DefaultFullscreenTriangle, program);
        }

		// Bind input
        m_ProgramInstance->SetFloat4(SE_TEXT("ScreenSize"), renderContext.view.ScreenSize);
        m_ProgramInstance->SetTexture(SE_TEXT("Source"), input->View());

		// Render
		context->SetRenderTarget(output);
        context->SetSLC2State(m_PipelineState);
        context->DrawFullscreenTriangle(m_ProgramInstance);
	}
} // SE
