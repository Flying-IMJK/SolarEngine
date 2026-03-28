
#include "GUIMaterialShader.h"

#include "MaterialParams.h"
#include "Core/Math/Matrix.h"
#include "Runtime/Graphics/GPUContext.h"
#include "Runtime/Graphics/GPUDevice.h"
#include "Runtime/Graphics/Shaders/GPUShader.h"

namespace SE
{
    GES_PACK_STRUCT(struct GUIMaterialShaderData {
        Matrix ViewProjectionMatrix;
        Matrix WorldMatrix;
        Matrix ViewMatrix;
        Float3 ViewPos;
        float ViewFar;
        Float3 ViewDir;
        float TimeParam;
        Float4 ViewInfo;
        Float4 ScreenSize;
        Float4 ViewSize;
    });

    void GUIMaterialShader::Cache::Release()
    {
        DeleteObjectSafe(Depth);
        DeleteObjectSafe(NoDepth);
    }

    void GUIMaterialShader::Bind(BindParameters& params)
    {
        // Prepare
        /*GPUContext* context = params.gpuContext;
        Span cb(m_CBData.Get(), m_CBData.Count());
        ASSERT_LOW_LAYER(cb.Length() >= sizeof(GUIMaterialShaderData));
        auto materialData = reinterpret_cast<GUIMaterialShaderData*>(cb.Get());
        cb = Span<byte>(cb.Get() + sizeof(GUIMaterialShaderData), cb.Length() - sizeof(GUIMaterialShaderData));
        int32 srv = 0;
        const auto ps = context->IsDepthBufferBinded() ? m_Cache.Depth : m_Cache.NoDepth;
         auto customData = (Render2D::CustomData*)params.customData;

        // Setup parameters
        MaterialParameter::BindMeta bindMeta;
        bindMeta.Context = context;
        bindMeta.Constants = cb;
        bindMeta.Input = nullptr;
        bindMeta.Buffers = nullptr;
        bindMeta.CanSampleDepth = false;
        bindMeta.CanSampleGBuffer = false;
        MaterialParams::Bind(params.paramsLink, bindMeta);

        // Setup material constants
        {
            Matrix::Transpose(customData->ViewProjection, materialData->ViewProjectionMatrix);
            Matrix::Transpose(Matrix::Identity, materialData->WorldMatrix);
            Matrix::Transpose(Matrix::Identity, materialData->ViewMatrix);
            materialData->ViewPos = Float3::Zero;
            materialData->ViewFar = 0.0f;
            materialData->ViewDir = Float3::Forward;
            materialData->TimeParam = params.timeParam;
            materialData->ViewInfo = Float4::Zero;
            auto& viewport = Render2D::GetViewport();
            materialData->ScreenSize = Float4(viewport.Width, viewport.Height, 1.0f / viewport.Width, 1.0f / viewport.Height);
            materialData->ViewSize = Float4(customData->ViewSize.X, customData->ViewSize.Y, 1.0f / customData->ViewSize.X, 1.0f / customData->ViewSize.Y);
        }

        // Bind constants
        if (m_CB)
        {
            context->UpdateCB(m_CB, m_CBData.Get());
            context->BindCB(0, m_CB);
        }

        // Bind pipeline
        context->SetState(ps);*/
    }

    void GUIMaterialShader::Unload()
    {
        // Base
        MaterialShader::Unload();

        m_Cache.Release();
    }

    bool GUIMaterialShader::OnLoad()
    {
        GPUPipelineState::Description psDesc0 = GPUPipelineState::Description::DefaultFullscreenTriangle;
        psDesc0.Wireframe = m_Info.FeaturesFlags.IsFlag(MaterialFeatures::Wireframe);
        psDesc0.VS = m_Shader->GetVS(SE_TEXT("VS_GUI"));
        psDesc0.PS = m_Shader->GetPS(SE_TEXT("PS_GUI"));
        psDesc0.BlendMode = BlendingMode::AlphaBlend;

        psDesc0.DepthEnable = psDesc0.DepthWriteEnable = true;
        m_Cache.Depth = GPUDevice::instance->CreatePipelineState();
        m_Cache.NoDepth = GPUDevice::instance->CreatePipelineState();

        bool failed = m_Cache.Depth->Init(psDesc0);

        psDesc0.DepthEnable = psDesc0.DepthWriteEnable = false;
        failed |= m_Cache.NoDepth->Init(psDesc0);

        if (failed)
        {
            LOG_WARNING("Render", "Failed to create GUI material pipeline state.");
            return true;
        }

        return false;
    }
} // SE