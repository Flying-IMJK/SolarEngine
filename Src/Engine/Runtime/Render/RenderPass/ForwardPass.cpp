#include "ForwardPass.h"

#include "Runtime/Core/Profiler/Profiler.h"

#include "Runtime/Graphics/GPUContext.h"
#include "Runtime/Graphics/GPUDevice.h"
#include "Runtime/Graphics/RenderTargetPool.h"
#include "Runtime/Graphics/Base/GPUPipelineState.h"
#include "Runtime/Graphics/Shaders/GPUShader.h"
#include "Runtime/Render/RenderBuffers.h"
#include "Runtime/Render/RenderContext.h"
#include "Runtime/Render/RenderEnum.h"
#include "Runtime/Render/RenderList.h"

namespace SE
{
    ForwardPass::ForwardPass()
        : _shader(nullptr)
        , _psApplyDistortion(nullptr)
    {
    }

    String ForwardPass::ToString() const
    {
        return SE_TEXT("ForwardPass");
    }

    bool ForwardPass::Init()
    {
        // Prepare resources
        _psApplyDistortion = GPUDevice::instance->CreatePipelineState();
        _shader = AssetContent::LoadAsyncInternal<Shader>(SE_TEXT("Shaders/Forward"));

        if (_shader == nullptr)
        {
            return false;
        }

        _shader->WaitForLoaded();

        // #if COMPILE_WITH_DEV_ENV
        //     _shader.Get()->OnReloading.Bind<ForwardPass, &ForwardPass::OnShaderReloading>(this);
        // #endif

        return true;
    }
    void ForwardPass::Dispose()
    {
        RendererPass<ForwardPass>::Dispose();
    }

    bool ForwardPass::SetupResources()
    {
        if (!_shader.HasReference())
        {
            return false;
        }

        // Check shader
        if (!_shader->IsLoaded())
        {
            return false;
        }
        const auto shader = _shader->GetShader();

        // Create pipeline stages
        GPUPipelineState::Description psDesc;
        if (!_psApplyDistortion->IsValid())
        {
            psDesc = GPUPipelineState::Description::DefaultFullscreenTriangle;
            psDesc.PS = shader->GetPS(SE_TEXT("PS_ApplyDistortion"));
            if (!_psApplyDistortion->Init(psDesc))
                return false;
        }

        return true;
    }

    void ForwardPass::Render(RenderContext &renderContext, GPUTexture *input, GPUTexture *output)
    {
        PROFILE_GPU_CPU("Forward");
        auto context = renderContext.gpuContext;;
        auto& view = renderContext.view;
        auto mainCache = renderContext.list;
    
        context->ResetRenderTarget();
        context->ResetSR();
    
        // Try to use read-only depth if supported
        GPUTexture* depthBuffer = renderContext.buffers->DepthBuffer;
        GPUTextureView* depthBufferHandle = depthBuffer->View();
        if (depthBuffer->Flags().IsFlag(GPUTextureFlags::ReadOnlyDepthView))
        {
            depthBufferHandle = depthBuffer->ViewReadOnlyDepth();
        }
    
        // Check if there is no objects to render or no resources ready
        auto& forwardList = mainCache->DrawCallsLists[(int32)DrawCallsListType::Forward];
        auto& distortionList = mainCache->DrawCallsLists[(int32)DrawCallsListType::Distortion];
        if (/*distortionList.IsEmpty() || */CheckIfSkipPass())
        {
            // Copy frame
            context->SetRenderTarget(output->View());
            context->Draw(input);
        }
        /*else
        {
            PROFILE_GPU_CPU("Distortion");
    
            // Peek temporary render target for the distortion pass
            // TODO: render distortion in half-res?
            const int32 width = renderContext.buffers->GetWidth();
            const int32 height = renderContext.buffers->GetHeight();
            const int32 distortionWidth = width;
            const int32 distortionHeight = height;
            const auto tempDesc = GPUTextureDescription::New2D(distortionWidth, distortionHeight, PixelFormat::R8G8B8A8_UNorm);
            auto distortionRT = RenderTargetPool::Get(tempDesc);
            RENDER_TARGET_POOL_SET_NAME(distortionRT, "Forward.Distortion");
    
            // Clear distortion vectors
            context->Clear(distortionRT->View(), Colors::Transparent);
            context->SetViewportAndScissors((float)distortionWidth, (float)distortionHeight);
            context->SetRenderTarget(depthBufferHandle, distortionRT->View());
    
            // Render distortion pass
            view.Pass = DrawPass::Distortion;
            mainCache->ExecuteDrawCalls(renderContext, distortionList);
    
            // Copy combined frame with distortion from transparent materials
            context->SetViewportAndScissors((float)width, (float)height);
            context->ResetRenderTarget();
            context->ResetSR();
            context->BindSR(0, input);
            context->BindSR(1, distortionRT);
            context->SetRenderTarget(output->View());
            context->SetState(_psApplyDistortion);
            context->DrawFullscreenTriangle();
    
            RenderTargetPool::Release(distortionRT);
        }*/
    
        if (!forwardList.IsEmpty())
        {
            // Run forward pass
            /*view.Pass = DrawPass::Forward;
            context->SetRenderTarget(depthBufferHandle, output->View());
            mainCache->ExecuteDrawCalls(renderContext, forwardList, input->View());*/
        }
    }
}
