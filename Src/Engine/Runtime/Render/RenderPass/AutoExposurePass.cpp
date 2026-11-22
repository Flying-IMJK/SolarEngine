#include <Runtime/Render/RenderPass/AutoExposurePass.h>

#include <Runtime/Core/Profiler/Profiler.h>
#include <Runtime/Graphics/Base/GPULimits.h>
#include <Runtime/Graphics/GPUContext.h>
#include <Runtime/Graphics/GPUDevice.h>
#include <Runtime/Graphics/Textures/GPUTexture.h>
#include <Runtime/Render/RenderBuffers.h>
#include <Runtime/Render/RenderContext.h>
#include <Runtime/Render/RenderList.h>
#include <Runtime/Render/RenderTask.h>
#include <Runtime/Resource/AssetContent.h>
#include <Runtime/Utilities/Time.h>

namespace SE
{
    String AutoExposurePass::ToString() const { return SE_TEXT("AutoExposure"); }

    bool AutoExposurePass::Init() 
    { 
        m_Shader = AssetContent::LoadAsyncInternal<Shader>(SE_TEXT("Shaders/PostProcess/AutoExposure"));

        if (!m_Shader)
        {
            return false;
        }

        if (!m_Shader->WaitForLoaded())
        {
            return false;
        }

        m_Shader->OnReloadingEvent.Bind<AutoExposurePass, &AutoExposurePass::OnShaderReloading>(this);

        return true; 
    }

    void AutoExposurePass::OnShaderReloading(Asset* pAsset)
    {
        DeleteObjectSafe(m_ResetProgram);
        DeleteObjectSafe(m_HistogramProgram);
        DeleteObjectSafe(m_AdaptProgram);
        InvalidateResources();
    }

    void AutoExposurePass::Dispose()
    {
        if (m_Shader)
        {
            m_Shader->OnReloadingEvent.Unbind<AutoExposurePass, &AutoExposurePass::OnShaderReloading>(this);
        }
        OnShaderReloading(nullptr);
        m_Shader = nullptr;
        RendererPass<AutoExposurePass>::Dispose();
    }

    bool AutoExposurePass::SetupResources()
    {
        auto                device   = GPUDevice::instance;
        const FormatSupport required = FormatSupport::Texture2D | FormatSupport::ShaderLoad | FormatSupport::TypedUnorderedAccessView;
        if (!device->GetGPULimits().HasCompute || (device->GetPixelFormatFeatures(PixelFormat::R32_UInt).Support & required) != required || (device->GetPixelFormatFeatures(PixelFormat::R32G32B32A32_Float).Support & required) != required)
        {
            LOG_ERROR("Render", "AutoExposure requires compute and loadable/storage R32UI and RGBA32F textures.");
            return false;
        }

        ShaderProgramInstance** instances[] = {&m_ResetProgram, &m_HistogramProgram, &m_AdaptProgram};
        const Char*             names[]     = {SE_TEXT("ResetHistogram"), SE_TEXT("BuildHistogram"), SE_TEXT("AdaptExposure")};
        for (int32 i = 0; i < 3; i++)
        {
            if (*instances[i])
            {
                continue;
            }
            GPUShaderProgram* program = nullptr;
            if (!m_Shader->SelectProgram(names[i], {}, program))
            {
                return false;
            }
            *instances[i] = New<ShaderProgramInstance>();
            if (!(*instances[i])->Initialize(program))
            {
                DeleteObjectSafe(*instances[i]);
                return false;
            }
        }
        return true;
    }

    GPUTexture* AutoExposurePass::Render(RenderContext& renderContext, GPUTexture* pSceneColor)
    {
        PROFILE_GPU_CPU("AutoExposure");
        if (CheckIfSkipPass())
        {
            return nullptr;
        }

        auto         buffers    = renderContext.buffers;
        auto         context    = renderContext.gpuContext;
        GPUTexture** textures[] = {&buffers->AutoExposureHistogram, &buffers->AutoExposureHistory[0], &buffers->AutoExposureHistory[1]};
        const Char*  names[]    = {SE_TEXT("AutoExposure.Histogram"), SE_TEXT("AutoExposure.History0"), SE_TEXT("AutoExposure.History1")};
        for (int32 i = 0; i < 3; i++)
        {
            if (*textures[i] && (*textures[i])->IsAllocated())
            {
                continue;
            }
            if (!*textures[i])
            {
                *textures[i] = GPUDevice::instance->CreateTexture(names[i]);
            }

            GPUTextureDescription description;
            if (i == 0)
            {
                description = GPUTextureDescription::New2D(256, 1, PixelFormat::R32_UInt, GPUTextureFlags::ShaderResource | GPUTextureFlags::UnorderedAccess);    
            }
            else
            {
                description = GPUTextureDescription::New2D(1, 1, PixelFormat::R32G32B32A32_Float, GPUTextureFlags::ShaderResource | GPUTextureFlags::UnorderedAccess);    
            }

            buffers->AutoExposureHistoryValid = false;
            if (!*textures[i] || !(*textures[i])->Init(description))
            {
                return nullptr;
            }
        }

        auto settings = renderContext.list->PostSettings.AutoExposure;
        const double now   = Time::Render.UnscaledTime.GetTotalSeconds();
        const bool   reset = !buffers->AutoExposureHistoryValid || renderContext.view.IsSingleFrame || (renderContext.task && renderContext.task->IsCameraCut);
        struct Data
        {
            Float2 InputSize;
            Float2 EVLimits;
            Float4 Metering;
            Float4 Adaptation;
        } data;
        data.InputSize  = Float2(static_cast<float>(pSceneColor->Width()), static_cast<float>(pSceneColor->Height()));
        data.EVLimits   = Float2(settings.MinEV100, settings.MaxEV100);
        data.Metering   = Float4(settings.LowPercentile, settings.HighPercentile, settings.ExposureCompensation, 0.0f);
        data.Adaptation = Float4(reset ? 0.0f : static_cast<float>(Math::Max(now - buffers->LastAutoExposureTime, 0.0)), settings.SpeedUp, settings.SpeedDown, reset ? 1.0f : 0.0f);

        auto        previous  = buffers->AutoExposureHistory[buffers->AutoExposureHistoryIndex];
        const int32 nextIndex = 1 - buffers->AutoExposureHistoryIndex;
        auto        output    = buffers->AutoExposureHistory[nextIndex];

        if (!m_ResetProgram->SetTexture(SE_TEXT("HistogramOutput"), buffers->AutoExposureHistogram->View()) || 
            !m_HistogramProgram->SetConstantBuffer(SE_TEXT("data"), &data, sizeof(data)) ||
            !m_HistogramProgram->SetTexture(SE_TEXT("SceneColor"), pSceneColor->View()) || 
            !m_HistogramProgram->SetTexture(SE_TEXT("HistogramOutput"), buffers->AutoExposureHistogram->View()) ||
            !m_AdaptProgram->SetConstantBuffer(SE_TEXT("data"), &data, sizeof(data)) || 
            !m_AdaptProgram->SetTexture(SE_TEXT("HistogramInput"), buffers->AutoExposureHistogram->View()) ||
            !m_AdaptProgram->SetTexture(SE_TEXT("ExposureHistory"), previous->View()) || 
            !m_AdaptProgram->SetTexture(SE_TEXT("ExposureOutput"), output->View()))
        {
            buffers->AutoExposureHistoryValid = false;
            return nullptr;
        }
        context->ResetRenderTarget();
        context->Dispatch(*m_ResetProgram, 1, 1, 1);
        context->Dispatch(*m_HistogramProgram, (pSceneColor->Width() + 15) / 16, (pSceneColor->Height() + 15) / 16, 1);
        context->Dispatch(*m_AdaptProgram, 1, 1, 1);
        buffers->AutoExposureHistoryIndex = nextIndex;
        buffers->AutoExposureHistoryValid = true;
        buffers->LastAutoExposureTime     = now;
        return output;
    }
} // namespace SE
