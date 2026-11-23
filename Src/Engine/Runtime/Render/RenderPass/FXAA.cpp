
#include <Runtime/Render/RenderPass/FXAA.h>

#include <Runtime/Core/Profiler/Profiler.h>
#include <Runtime/Graphics/GPUContext.h>
#include <Runtime/Graphics/GPUDevice.h>
#include <Runtime/Graphics/Shaders/ShaderProgramInstance.h>
#include <Runtime/Graphics/Textures/GPUTexture.h>
#include <Runtime/Resource/Assets/Materials/Shader.h>

namespace SE
{
    String FXAA::ToString() const { return SE_TEXT("FXAA"); }

    bool FXAA::Init()
	{
        m_Shader = AssetContent::LoadAsyncInternal<Shader>(SE_TEXT("Shaders/PostProcess/FXAA"));
        if (m_Shader == nullptr)
        {
            return false;
        }
        m_Shader->OnReloadingEvent.Bind<FXAA, &FXAA::OnShaderReloading>(this);

        return true;
    }

    void FXAA::OnShaderReloading(Asset* pAsset)
    {
        DeleteObjectSafe(m_PipelineState);
        DeleteObjectSafe(m_ProgramInstance);
        InvalidateResources();
    }

    bool FXAA::SetupResources()
    {
        if (!m_Shader.HasReference() || !m_Shader->WaitForLoaded())
		{
			return false;
		}

		GPUDevice* device = GPUDevice::instance;

		m_PipelineState = device->CreatePipelineState();
        m_ProgramInstance       = New<ShaderProgramInstance>();

        return m_PipelineState && m_ProgramInstance;
    }

    void FXAA::Dispose()
	{
        if (m_Shader)
            m_Shader->OnReloadingEvent.Unbind<FXAA, &FXAA::OnShaderReloading>(this);

        // Base
        RendererPass::Dispose();

        Delete(m_ProgramInstance);
        Delete(m_PipelineState);
		m_Shader = nullptr;
	}

	void FXAA::Render(RenderContext& renderContext, GPUTexture* input, GPUTextureView* output)
	{
		auto context = renderContext.gpuContext;
        if (CheckIfSkipPass())
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

        GPUShaderProgram* program = nullptr;
        bool              validAA = m_Shader->SelectProgram(SE_TEXT("FXAA"), {macro}, program);
        if (validAA && (m_ProgramInstance->GetProgram() != program || !m_PipelineState->IsValid()))
        {
            m_PipelineState->ReleaseGPU();
            validAA = m_ProgramInstance->Initialize(program) && m_PipelineState->Init(GPUPipelineState::Description::DefaultFullscreenTriangle, program);
        }

        if (validAA)
        {
            Float4 screenSize(static_cast<float>(input->Width()), static_cast<float>(input->Height()), 1.0f / input->Width(), 1.0f / input->Height());
            validAA = m_ProgramInstance->SetFloat4(SE_TEXT("ScreenSize"), screenSize) && m_ProgramInstance->SetTexture(SE_TEXT("Source"), input->View());
        }

		context->SetRenderTarget(output);
        if (validAA)
        {
            context->SetState(m_PipelineState);
            context->DrawFullscreenTriangle(m_ProgramInstance);
        }
        else
        {
            LOG_ERROR("Render", "Cannot prepare or bind FXAA; copying encoded scene color.");
            context->Draw(input);
        }
    }
} // namespace SE
