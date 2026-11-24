
#include <Runtime/Render/RenderPass/PostProcessingPass.h>

#include <Runtime/Core/Profiler/Profiler.h>
#include <Runtime/Graphics/GPUContext.h>
#include <Runtime/Graphics/GPUDevice.h>
#include <Runtime/Graphics/RenderTargetPool.h>
#include <Runtime/Graphics/Shaders/GPUConstantBuffer.h>
#include <Runtime/Graphics/Shaders/ShaderProgramInstance.h>
#include <Runtime/Graphics/Textures/GPUTexture.h>
#include <Runtime/Render/PostProcessSettings.h>
#include <Runtime/Render/RenderList.h>
#include <Runtime/Render/RenderPass/AutoExposurePass.h>
#include <Runtime/Render/RenderPass/ColorGradingPass.h>
#include <Runtime/Resource/Assets/Materials/Shader.h>
#include <Runtime/Utilities/Time.h>

namespace SE
{
    String PostProcessingPass::ToString() const { return SE_TEXT("PostProcessingPass"); }

    bool PostProcessingPass::Init()
    {
        // Load shader
        m_Shader = AssetContent::LoadAsyncInternal<Shader>(SE_TEXT("Shaders/PostProcess/PostProcessing"));
        if (m_Shader == nullptr)
        {
            return false;
        }
        m_Shader->OnReloadingEvent.Bind<PostProcessingPass, &PostProcessingPass::OnShaderReloading>(this);

        GPUDevice* device = GPUDevice::instance;

        // Create pipeline states
        m_PSOThreshold = device->CreatePipelineState();
        m_PSOScale     = device->CreatePipelineState();
        m_PSOBlurH     = device->CreatePipelineState();
        m_PSOBlurV     = device->CreatePipelineState();
        m_PSOGenGhosts = device->CreatePipelineState();
        m_PSOComposite = device->CreatePipelineState();

        m_SPThreshold = New<ShaderProgramInstance>();
        m_SPScale     = New<ShaderProgramInstance>();
        m_SPBlurH     = New<ShaderProgramInstance>();
        m_SPBlurV     = New<ShaderProgramInstance>();
        m_SPGenGhosts = New<ShaderProgramInstance>();
        m_SPComposite = New<ShaderProgramInstance>();

        m_DataBuffer         = device->CreateConstantBuffer(sizeof(Data));
        m_GaussianBlurBuffer = device->CreateConstantBuffer(sizeof(GaussianBlurData));

        return true;
    }

    void PostProcessingPass::OnShaderReloading(Asset* pAsset)
    {
        DeleteObjectSafe(m_PSOThreshold);
        DeleteObjectSafe(m_SPThreshold);
        m_PSOThreshold = GPUDevice::instance->CreatePipelineState();
        m_SPThreshold  = New<ShaderProgramInstance>();
        DeleteObjectSafe(m_PSOScale);
        DeleteObjectSafe(m_SPScale);
        m_PSOScale = GPUDevice::instance->CreatePipelineState();
        m_SPScale  = New<ShaderProgramInstance>();
        DeleteObjectSafe(m_PSOBlurH);
        DeleteObjectSafe(m_SPBlurH);
        m_PSOBlurH = GPUDevice::instance->CreatePipelineState();
        m_SPBlurH  = New<ShaderProgramInstance>();
        DeleteObjectSafe(m_PSOBlurV);
        DeleteObjectSafe(m_SPBlurV);
        m_PSOBlurV = GPUDevice::instance->CreatePipelineState();
        m_SPBlurV  = New<ShaderProgramInstance>();
        DeleteObjectSafe(m_PSOGenGhosts);
        DeleteObjectSafe(m_SPGenGhosts);
        m_PSOGenGhosts = GPUDevice::instance->CreatePipelineState();
        m_SPGenGhosts  = New<ShaderProgramInstance>();
        DeleteObjectSafe(m_PSOComposite);
        DeleteObjectSafe(m_SPComposite);
        m_PSOComposite = GPUDevice::instance->CreatePipelineState();
        m_SPComposite  = New<ShaderProgramInstance>();
        InvalidateResources();
    }

    bool PostProcessingPass::SetupResources()
    {
        // Optional Bloom and lens programs must not gate the required display transform.
        return m_Shader && m_Shader->WaitForLoaded() && m_PSOComposite && m_SPComposite && m_DataBuffer;
    }

    GPUTexture* PostProcessingPass::GetCustomOrDefault(Texture* customTexture, AssetRef<Texture>& defaultTexture, const Char* defaultName)
    {
        // Check if use custom texture
        if (customTexture)
        {
            return customTexture->GetTexture();
        }

        // Check if need to load default texture
        if (defaultTexture == nullptr)
        {
            // Load default
            defaultTexture = AssetContent::LoadAsyncInternal<Texture>(defaultName);
        }

        // Use default texture or nothing
        return defaultTexture ? defaultTexture->GetTexture() : nullptr;
    }

    void PostProcessingPass::GB_ComputeKernel(float sigma, float width, float height)
    {
        float total          = 0.0f;
        float twoSigmaSquare = 2.0f * sigma * sigma;
        float sigmaRoot      = Math::Sqrt(twoSigmaSquare * Math::PI);
        float xOffset        = 1.0f / width;
        float yOffset        = 1.0f / height;

        // Calculate weights and offsets
        for (int32 i = -GB_RADIUS; i <= GB_RADIUS; i++)
        {
            // Calculate pixel distance and index
            const float distance = static_cast<float>(i * i);
            const int32 index    = i + GB_RADIUS;

            // Calculate pixel weight
            const float weight = Math::Exp(-distance / twoSigmaSquare) / sigmaRoot;

            // Calculate total weights sum
            total += weight;

            GaussianBlurCacheH[index] = Float4(weight, i * xOffset, 0, 0);
            GaussianBlurCacheV[index] = Float4(weight, i * yOffset, 0, 0);
        }

        // Normalize weights
        for (int32 i = 0; i < GB_KERNEL_SIZE; i++)
        {
            GaussianBlurCacheH[i].X /= total;
            GaussianBlurCacheV[i].X /= total;
        }

        // Assign size
        _gbData.Size = Float2(width, height);
    }

    void PostProcessingPass::Dispose()
    {
        if (m_Shader)
        {
            m_Shader->OnReloadingEvent.Unbind<PostProcessingPass, &PostProcessingPass::OnShaderReloading>(this);
        }

        // Base
        RendererPass::Dispose();

        // Cleanup
        DeleteObjectSafe(m_PSOThreshold);
        DeleteObjectSafe(m_PSOScale);
        DeleteObjectSafe(m_PSOBlurH);
        DeleteObjectSafe(m_PSOBlurV);
        DeleteObjectSafe(m_PSOGenGhosts);
        DeleteObjectSafe(m_PSOComposite);

        DeleteObjectSafe(m_SPThreshold);
        DeleteObjectSafe(m_SPScale);
        DeleteObjectSafe(m_SPBlurH);
        DeleteObjectSafe(m_SPBlurV);
        DeleteObjectSafe(m_SPGenGhosts);
        DeleteObjectSafe(m_SPComposite);
        DeleteObjectSafe(m_DataBuffer);
        DeleteObjectSafe(m_GaussianBlurBuffer);
        m_Shader           = nullptr;
        _defaultLensColor = nullptr;
        _defaultLensDirt  = nullptr;
        _defaultLensStar  = nullptr;
    }

    void PostProcessingPass::Render(RenderContext& renderContext, GPUTexture* pInput, GPUTexture* pOutput)
    {
        PROFILE_GPU_CPU("Post Processing");
        auto context = renderContext.gpuContext;
        if (CheckIfSkipPass())
        {
            context->SetViewportAndScissors(static_cast<float>(pOutput->Width()), static_cast<float>(pOutput->Height()));
            context->Draw(pOutput, pInput);
            return;
        }

        context->ResetRenderTarget();

        // HDR stage order follows the desktop UE pipeline. These stages are not implemented yet:
        // Before-Translucency materials -> DOF / separate translucency -> Before-Tonemapping materials
        // -> TAA / TSR -> motion blur / post-motion-blur translucency. All exchange linear HDR colors.
        // Separate translucency can be composed in DOF or TSR, depending on the future rendering path.

        Data        data         = {};
        const float time         = Time::Render.UnscaledTime.GetTotalSeconds();
        data.Time                = Math::Fractional(time);
        data.InputSize           = Float2(static_cast<float>(pInput->Width()), static_cast<float>(pInput->Height()));
        data.InvInputSize        = Float2(1.0f / data.InputSize.X, 1.0f / data.InputSize.Y);
        data.InputAspect         = data.InputSize.X / data.InputSize.Y;
        data.VignetteShapeFactor = 1.0f;
        data.GrainParticleSize   = 1.0f;
        data.GrainTime           = time;
        data.LensFlareStarMat    = Matrix::Identity;
        // CameraArtifacts settings are not connected. Keep effects neutral, but pass cinematic
        // overlays to the combined LUT in linear space, after legacy LDR LUT grading.
        const Color screenFadeColor = Colors::Transparent;

        // Meter the scene before Bloom, grading, overlays and output encoding. Exposure history
        // remains view-owned in RenderBuffers; the GPU result is borrowed through composite submission.
        auto exposure = AutoExposurePass::Instance().Render(renderContext, pInput);
        if (!exposure)
        {
            context->SetViewportAndScissors(static_cast<float>(pOutput->Width()), static_cast<float>(pOutput->Height()));
            context->Draw(pOutput, pInput);
            return;
        }

        // Local-exposure preparation belongs here. Bloom / Lens Flare preparation follows metering
        // and produces linear HDR textures for the Tonemapper. They are not enabled by this change.
        // Small-target restrictions belong to those optional stages, not to the required composite.
        auto colorGradingLUT = ColorGradingPass::Instance().RenderLUT(renderContext, screenFadeColor);
        if (!colorGradingLUT)
        {
            context->SetViewportAndScissors(static_cast<float>(pOutput->Width()), static_cast<float>(pOutput->Height()));
            context->Draw(pOutput, pInput);
            return;
        }

        List<String>    macros;
        GPUTextureView* lutView = colorGradingLUT->View();
        if (colorGradingLUT->IsVolume())
        {
            macros.Add(SE_TEXT("USE_3D_LUT"));
            lutView = colorGradingLUT->ViewVolume();
        }
        GPUShaderProgram* program        = nullptr;
        bool              validComposite = m_Shader->SelectProgram(SE_TEXT("Composite"), macros, program);
        if (validComposite && (m_SPComposite->GetProgram() != program || !m_PSOComposite->IsValid()))
        {
            m_PSOComposite->ReleaseGPU();
            validComposite = m_SPComposite->Initialize(program) && m_PSOComposite->Init(GPUPipelineState::Description::DefaultFullscreenTriangle, program);
        }
        if (validComposite)
        {
            context->UpdateCB(m_DataBuffer, &data);
            validComposite = m_SPComposite->SetConstantBuffer(SE_TEXT("data"), m_DataBuffer) && m_SPComposite->SetTexture(SE_TEXT("Input0"), pInput->View()) && m_SPComposite->SetTexture(SE_TEXT("AutoExposure"), exposure->View()) &&
                             m_SPComposite->SetTexture(SE_TEXT("ColorGradingLUT"), lutView);
        }
        if (validComposite)
        {
            context->SetViewportAndScissors(static_cast<float>(pOutput->Width()), static_cast<float>(pOutput->Height()));
            context->SetRenderTarget(pOutput->View());
            context->SetState(m_PSOComposite);
            context->DrawFullscreenTriangle(m_SPComposite);
        }
        else
        {
            context->SetViewportAndScissors(static_cast<float>(pOutput->Width()), static_cast<float>(pOutput->Height()));
            context->Draw(pOutput, pInput);
        }

        // Return pooled resources only after their final draw has been submitted. Do not wait on the CPU.
        context->ResetRenderTarget();
        context->FlushState();
        RenderTargetPool::Release(colorGradingLUT);
    }

} // namespace SE
