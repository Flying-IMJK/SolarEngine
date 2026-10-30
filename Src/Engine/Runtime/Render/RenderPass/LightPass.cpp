#include <Runtime/Render/RenderPass/LightPass.h>

#include <Runtime/Core/Profiler/Profiler.h>
#include <Runtime/Graphics/GPUContext.h>
#include <Runtime/Graphics/GPUDevice.h>
#include <Runtime/Graphics/RenderTargetPool.h>
#include <Runtime/Graphics/Shaders/GPUConstantBuffer.h>
#include <Runtime/Graphics/Textures/GPUTexture.h>
#include <Runtime/Graphics/Textures/GPUTextureDescription.h>
#include <Runtime/Render/RenderBuffers.h>
#include <Runtime/Render/RenderList.h>
#include <Runtime/Render/RenderPass/GBufferPass.h>
#include <Runtime/Render/RenderPass/ShadowPass.h>
#include <Runtime/Resource/Assets/SLC2Materials/GraphicShader.h>

namespace SE
{
    GES_PACK_STRUCT(struct PerLight {
        LightData Light;
        Matrix    WVP;
    });

    GES_PACK_STRUCT(struct PerFrame { GBufferData GBuffer; });

    bool LightPass::Init()
    {
        m_Shader      = AssetContent::LoadAsyncInternal<GraphicShader>(SE_TEXT("Shaders/Lights"));
        m_SphereModel = AssetContent::LoadAsyncInternal<Model>(SE_TEXT("Assets/Models/Sphere"));
        if (m_Shader == nullptr || m_SphereModel == nullptr)
        {
            return false;
        }

#if COMPILE_WITH_DEV_ENV
        m_Shader->OnReloadingEvent.Bind<LightPass, &LightPass::OnShaderReloading>(this);
#endif

        auto format = PixelFormat::R8G8_UNorm;
        if (!EnumHasAllFlags(GPUDevice::instance->GetPixelFormatFeatures(format).Support,
                             FormatSupport::RenderTarget | FormatSupport::ShaderSample | FormatSupport::Texture2D))
        {
            format = PixelFormat::B8G8R8A8_UNorm;
        }
        m_ShadowMaskFormat = format;

        return true;
    }

    void LightPass::Dispose()
    {
        RendererPass<LightPass>::Dispose();

#if COMPILE_WITH_DEV_ENV
        if (m_Shader.HasReference())
        {
            m_Shader->OnReloadingEvent.Unbind<LightPass, &LightPass::OnShaderReloading>(this);
        }
#endif

        Delete(m_PsLightDir);
        Delete(m_PsLightPointNormal);
        Delete(m_PsLightPointInverted);
        Delete(m_ProgramLightDir);
        Delete(m_ProgramLightPoint);
        Delete(m_PerLightBuffer);
        Delete(m_PerFrameBuffer);
        m_PsLightDir           = nullptr;
        m_PsLightPointNormal   = nullptr;
        m_PsLightPointInverted = nullptr;
        m_ProgramLightDir      = nullptr;
        m_ProgramLightPoint    = nullptr;
        m_PerLightBuffer       = nullptr;
        m_PerFrameBuffer       = nullptr;
        m_Shader               = nullptr;
        m_SphereModel          = nullptr;
    }

    bool LightPass::SetupResources()
    {
        if (!m_Shader.HasReference() || !m_SphereModel.HasReference() || !m_Shader->WaitForLoaded() ||
            !m_SphereModel->WaitForLoaded() || !m_SphereModel->CanBeRendered())
        {
            return false;
        }

        GPUDevice* device = GPUDevice::instance;
        if (m_PsLightDir == nullptr)
        {
            m_PsLightDir = device->CreateSLC2PipelineState();
        }
        if (m_PsLightPointNormal == nullptr)
        {
            m_PsLightPointNormal = device->CreateSLC2PipelineState();
        }
        if (m_PsLightPointInverted == nullptr)
        {
            m_PsLightPointInverted = device->CreateSLC2PipelineState();
        }
        if (m_ProgramLightDir == nullptr)
        {
            m_ProgramLightDir = New<ShaderProgramInstance>();
        }
        if (m_ProgramLightPoint == nullptr)
        {
            m_ProgramLightPoint = New<ShaderProgramInstance>();
        }
        if (m_PerLightBuffer == nullptr)
        {
            m_PerLightBuffer = device->CreateConstantBuffer(sizeof(PerLight), SE_TEXT("LightPass.PerLight"));
        }
        if (m_PerFrameBuffer == nullptr)
        {
            m_PerFrameBuffer = device->CreateConstantBuffer(sizeof(PerFrame), SE_TEXT("LightPass.PerFrame"));
        }

        return m_PsLightDir != nullptr && m_PsLightPointNormal != nullptr && m_PsLightPointInverted != nullptr &&
               m_ProgramLightDir != nullptr && m_ProgramLightPoint != nullptr && m_PerLightBuffer != nullptr &&
               m_PerFrameBuffer != nullptr;
    }

#if COMPILE_WITH_DEV_ENV
    void LightPass::OnShaderReloading(Asset* pObj)
    {
        // Asset reload runs on the main thread before the old GPU programs are released.
        Delete(m_PsLightDir);
        Delete(m_PsLightPointNormal);
        Delete(m_PsLightPointInverted);
        Delete(m_ProgramLightDir);
        Delete(m_ProgramLightPoint);
        m_PsLightDir           = nullptr;
        m_PsLightPointNormal   = nullptr;
        m_PsLightPointInverted = nullptr;
        m_ProgramLightDir      = nullptr;
        m_ProgramLightPoint    = nullptr;
        InvalidateResources();
    }
#endif

    void LightPass::RenderLight(RenderContextBatch& renderContextBatch, GPUTextureView* pLightBuffer)
    {
        if (CheckIfSkipPass())
        {
            return;
        }

        auto&       renderContext   = renderContextBatch.Contexts[0];
        GPUContext* context         = renderContext.gpuContext;
        auto&       view            = renderContext.view;
        auto        mainCache       = renderContext.list;
        const bool  useShadows      = ShadowsPass::Instance().IsReady();
        const bool  disableSpecular = false; // (view.flags & ViewFlags::SpecularLight) == ViewFlags::None;

        PerLight        perLight = {};
        PerFrame        perFrame;
        GPUTexture*     depthBuffer         = renderContext.buffers->DepthBuffer;
        const bool      depthBufferReadOnly = EnumHasAnyFlags(depthBuffer->Flags(), GPUTextureFlags::ReadOnlyDepthView);
        GPUTextureView* depthBufferRTV      = depthBufferReadOnly ? depthBuffer->ViewReadOnlyDepth() : nullptr;
        GPUTextureView* depthBufferSRV = depthBufferReadOnly ? depthBuffer->ViewReadOnlyDepth() : depthBuffer->View();

        GBufferPass::SetInputs(view, perFrame.GBuffer);
        context->UpdateCB(m_PerFrameBuffer, &perFrame);

        const float sphereModelScale = 3.0f;
        GPUTexture* shadowMask       = nullptr;

        // Each light refreshes its own bindings after ShadowPass changes the GPU context.
        for (int32 lightIndex = 0; lightIndex < mainCache->PointLights.Count(); lightIndex++)
        {
            PROFILE_GPU_CPU_NAMED("Point Light");

            auto&        light         = mainCache->PointLights[lightIndex];
            const float  lightRadius   = light.Radius;
            const Float3 lightPosition = light.Position;
            const bool   renderShadow  = useShadows && light.ShadowDataIndex != -1;
            const float  distance     = Float3::Distance(view.Position, lightPosition) - lightRadius * sphereModelScale;
            const bool   isViewInside = distance < 0;

            Matrix world, wvp, matrix;
            Matrix::Scaling(lightRadius * sphereModelScale, wvp);
            Matrix::Translation(lightPosition, matrix);
            Matrix::Multiply(wvp, matrix, world);
            Matrix::Multiply(world, view.ViewProjection(), wvp);

            List<String> macros;
            if (disableSpecular)
            {
                macros.Add(SE_TEXT("LIGHTING_NO_SPECULAR"));
            }
            if (renderShadow)
            {
                macros.Add(SE_TEXT("USE_SHADOW_MASK"));
            }
            SLC2GPUShaderProgram* program = nullptr;
            if (!m_Shader->SelectProgram(SE_TEXT("PointLight"), macros, program))
            {
                continue;
            }
            if (m_ProgramLightPoint->GetProgram() != program || !m_PsLightPointNormal->IsValid() ||
                !m_PsLightPointInverted->IsValid())
            {
                // Release cached pipelines before switching to another Program/Variant.
                m_PsLightPointNormal->ReleaseGPU();
                m_PsLightPointInverted->ReleaseGPU();
                if (!m_ProgramLightPoint->Initialize(program))
                {
                    continue;
                }

                auto desc = SLC2GPUPipelineState::Description::DefaultNoDepth;
                desc.BlendMode                       = BlendingMode::Add;
                desc.BlendMode.RenderTargetWriteMask = BlendingMode::ColorWrite::RGB;
                desc.CullMode                        = CullMode::Inverted;
                if (!m_PsLightPointInverted->Init(desc, program))
                {
                    continue;
                }
                desc.CullMode    = CullMode::Normal;
                desc.DepthEnable = true;
                if (!m_PsLightPointNormal->Init(desc, program))
                {
                    continue;
                }
            }

            if (renderShadow)
            {
                if (shadowMask == nullptr)
                {
                    auto desc = GPUTextureDescription::New2D(
                        renderContext.buffers->GetWidth(), renderContext.buffers->GetHeight(), m_ShadowMaskFormat);
                    shadowMask = RenderTargetPool::Get(desc);
                    RENDER_TARGET_POOL_SET_NAME(shadowMask, "ShadowMask");
                }
                ShadowsPass::Instance().RenderShadow(renderContextBatch, light, shadowMask->View());
            }

            light.SetupLightData(&perLight.Light, renderShadow);
            Matrix::Transpose(wvp, perLight.WVP);
            context->UpdateCB(m_PerLightBuffer, &perLight);

            if (!m_ProgramLightPoint->SetConstantBuffer(SE_TEXT("perLight"), m_PerLightBuffer) ||
                !m_ProgramLightPoint->SetConstantBuffer(SE_TEXT("perFrame"), m_PerFrameBuffer) ||
                !m_ProgramLightPoint->SetTexture(SE_TEXT("GBuffer0"), renderContext.buffers->GBuffer0->View()) ||
                !m_ProgramLightPoint->SetTexture(SE_TEXT("GBuffer1"), renderContext.buffers->GBuffer1->View()) ||
                !m_ProgramLightPoint->SetTexture(SE_TEXT("GBuffer2"), renderContext.buffers->GBuffer2->View()) ||
                !m_ProgramLightPoint->SetTexture(SE_TEXT("GBuffer3"), renderContext.buffers->GBuffer3->View()) ||
                !m_ProgramLightPoint->SetTexture(SE_TEXT("Depth"), depthBufferSRV))
            {
                continue;
            }

            if (renderShadow && !m_ProgramLightPoint->SetTexture(SE_TEXT("Shadow"), shadowMask->View()))
            {
                continue;
            }

            context->SetRenderTarget(pLightBuffer, depthBufferRTV);
            context->SetSLC2State(isViewInside ? m_PsLightPointInverted : m_PsLightPointNormal);
            for (const auto& mesh : m_SphereModel->LODs[0].Meshes)
            {
                if (!mesh.IsInitialized())
                {
                    continue;
                }
                if (context->BindRenderGeometry(mesh.GetRenderGeometry()))
                {
                    context->DrawIndexed(m_ProgramLightPoint, mesh.GetTriangleCount() * 3);
                }
            }
        }

        for (int32 lightIndex = 0; lightIndex < mainCache->DirectionalLights.Count(); lightIndex++)
        {
            PROFILE_GPU_CPU_NAMED("Directional Light");

            auto&        light        = mainCache->DirectionalLights[lightIndex];
            const bool   renderShadow = useShadows && light.ShadowDataIndex != -1;
            List<String> macros;
            if (disableSpecular)
            {
                macros.Add(SE_TEXT("LIGHTING_NO_SPECULAR"));
            }
            if (renderShadow)
            {
                macros.Add(SE_TEXT("USE_SHADOW_MASK"));
            }
            SLC2GPUShaderProgram* program = nullptr;
            if (!m_Shader->SelectProgram(SE_TEXT("DirectionalLight"), macros, program))
            {
                continue;
            }
            if (m_ProgramLightDir->GetProgram() != program || !m_PsLightDir->IsValid())
            {
                m_PsLightDir->ReleaseGPU();
                auto desc                            = SLC2GPUPipelineState::Description::DefaultFullscreenTriangle;
                desc.BlendMode                       = BlendingMode::Add;
                desc.BlendMode.RenderTargetWriteMask = BlendingMode::ColorWrite::RGB;
                if (!m_ProgramLightDir->Initialize(program) || !m_PsLightDir->Init(desc, program))
                {
                    continue;
                }
            }

            if (renderShadow)
            {
                if (shadowMask == nullptr)
                {
                    auto desc = GPUTextureDescription::New2D(
                        renderContext.buffers->GetWidth(), renderContext.buffers->GetHeight(), m_ShadowMaskFormat);
                    shadowMask = RenderTargetPool::Get(desc);
                    RENDER_TARGET_POOL_SET_NAME(shadowMask, "ShadowMask");
                }
                ShadowsPass::Instance().RenderShadow(renderContextBatch, light, lightIndex, shadowMask->View());
            }

            light.SetupLightData(&perLight.Light, renderShadow);
            context->UpdateCB(m_PerLightBuffer, &perLight);
            if (!m_ProgramLightDir->SetConstantBuffer(SE_TEXT("perLight"), m_PerLightBuffer) ||
                !m_ProgramLightDir->SetConstantBuffer(SE_TEXT("perFrame"), m_PerFrameBuffer) ||
                !m_ProgramLightDir->SetTexture(SE_TEXT("GBuffer0"), renderContext.buffers->GBuffer0->View()) ||
                !m_ProgramLightDir->SetTexture(SE_TEXT("GBuffer1"), renderContext.buffers->GBuffer1->View()) ||
                !m_ProgramLightDir->SetTexture(SE_TEXT("GBuffer2"), renderContext.buffers->GBuffer2->View()) ||
                !m_ProgramLightDir->SetTexture(SE_TEXT("GBuffer3"), renderContext.buffers->GBuffer3->View()) ||
                !m_ProgramLightDir->SetTexture(SE_TEXT("Depth"), depthBufferSRV))
            {
                continue;
            }
            if (renderShadow && !m_ProgramLightDir->SetTexture(SE_TEXT("Shadow"), shadowMask->View()))
            {
                continue;
            }

            context->SetRenderTarget(pLightBuffer, depthBufferRTV);
            context->SetSLC2State(m_PsLightDir);
            context->DrawFullscreenTriangle(m_ProgramLightDir);
        }

        // Detach the output before returning the temporary mask to the pool.
        context->ResetRenderTarget();
        RenderTargetPool::Release(shadowMask);
        context->ResetSR();
        context->ResetCB();
    }

    String LightPass::ToString() const { return SE_TEXT("LightPass"); }
} // namespace SE
