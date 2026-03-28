
#include "DeferredMaterialShader.h"

#include "MaterialParams.h"
#include "Core/Math/Matrix.h"
#include "Runtime/Graphics/GPUContext.h"
#include "Runtime/Graphics/GPUDevice.h"
#include "Runtime/Graphics/Shaders/GPUShader.h"
#include "Runtime/Render/RenderContext.h"
#include "Runtime/Render/RenderDrawCall.h"
#include "Runtime/Render/RenderList.h"

namespace SE
{
    GES_PACK_STRUCT(struct DeferredMaterialShaderData {
    Matrix WorldMatrix;
    Matrix PrevWorldMatrix;
    Float2 Dummy0;
    float LODDitherFactor;
    float PerInstanceRandom;
    Float3 GeometrySize;
    float WorldDeterminantSign;
    });

    EnumFlags<DrawPass> DeferredMaterialShader::GetDrawModes() const
    {
        return {DrawPass::Depth, DrawPass::GBuffer, DrawPass::GlobalSurfaceAtlas, DrawPass::MotionVectors, DrawPass::QuadOverdraw};
    }

    bool DeferredMaterialShader::CanUseLightmap() const
    {
        return true;
    }

    bool DeferredMaterialShader::CanUseInstancing(InstancingHandler& handler) const
    {
        handler = { SurfaceDrawCallHandler::GetHash, SurfaceDrawCallHandler::CanBatch, SurfaceDrawCallHandler::WriteDrawCall, };
        return true;
    }

    void DeferredMaterialShader::Bind(BindParameters& params)
    {
        //PROFILE_CPU();
        // Prepare
        auto context = params.gpuContext;
        auto& view = params.renderContext.view;
        auto& drawCall = *params.firstDrawCall;
        Span<byte> cb(m_CBData.Get(), m_CBData.Count());
        ASSERT_LOW_LAYER(cb.Length() >= sizeof(DeferredMaterialShaderData));
        auto materialData = reinterpret_cast<DeferredMaterialShaderData*>(cb.Get());
        cb = Span<byte>(cb.Get() + sizeof(DeferredMaterialShaderData), cb.Length() - sizeof(DeferredMaterialShaderData));
        int32 srv = 2;

        // Setup features
        const bool useLightmap = false; //m_Info.BlendMode == MaterialBlendMode::Opaque && LightmapFeature::Bind(params, cb, srv);

        // Setup parameters
        MaterialParameter::BindMeta bindMeta;
        bindMeta.Context = context;
        bindMeta.Constants = cb;
        bindMeta.Input = nullptr;
        bindMeta.Buffers = params.renderContext.buffers;
        bindMeta.CanSampleDepth = false;
        bindMeta.CanSampleGBuffer = false;
        MaterialParams::Bind(params.paramsLink, bindMeta);

        // Setup material constants
        {
            Matrix::Transpose(drawCall.World, materialData->WorldMatrix);
            Matrix::Transpose(drawCall.Surface.PrevWorld, materialData->PrevWorldMatrix);
            materialData->WorldDeterminantSign = drawCall.WorldDeterminantSign;
            materialData->LODDitherFactor = drawCall.Surface.LODDitherFactor;
            materialData->PerInstanceRandom = drawCall.PerInstanceRandom;
            materialData->GeometrySize = drawCall.Surface.GeometrySize;
        }

        // Check if is using mesh skinning
        const bool useSkinning = false;//drawCall.Surface.Skinning != nullptr;
        bool perBoneMotionBlur = false;
        /*if (useSkinning)
        {
            // Bind skinning buffer
            ASSERT(drawCall.Surface.Skinning->IsReady());
            context->BindSR(0, drawCall.Surface.Skinning->BoneMatrices->View());
            if (drawCall.Surface.Skinning->PrevBoneMatrices && drawCall.Surface.Skinning->PrevBoneMatrices->IsAllocated())
            {
                context->BindSR(1, drawCall.Surface.Skinning->PrevBoneMatrices->View());
                perBoneMotionBlur = true;
            }
        }*/

        // Bind constants
        if (m_CB)
        {
            context->UpdateCB(m_CB, m_CBData.Get());
            context->BindCB(0, m_CB);
        }

        // Select pipeline state based on current pass and render mode
        const bool wireframe = m_Info.FeaturesFlags.IsFlag(MaterialFeatures::Wireframe) || view.Mode == ViewMode::Wireframe;
        CullMode cullMode = view.Pass.IsFlag(DrawPass::Depth) ? CullMode::TwoSided : m_Info.CullMode;
#if SE_EDITOR
        /*if (IsRunningRadiancePass)
            cullMode = CullMode::TwoSided;*/
#endif
        if (cullMode != CullMode::TwoSided && drawCall.WorldDeterminantSign < 0)
        {
            // Invert culling when scale is negative
            if (cullMode == CullMode::Normal)
                cullMode = CullMode::Inverted;
            else
                cullMode = CullMode::Normal;
        }
        // No support for instancing skinned meshes
        if (useSkinning && params.drawCallsCount > 1)
        {
            return;
        }
        const auto cache = params.drawCallsCount == 1 ? &_cache : &_cacheInstanced;
        PipelineStateCache* psCache = cache->GetPS(view.Pass, useLightmap, useSkinning, perBoneMotionBlur);
        ENGINE_ASSERT(psCache);
        GPUPipelineState* state = psCache->GetPS(cullMode, wireframe);

        // Bind pipeline
        context->SetState(state);
    }

    void DeferredMaterialShader::Unload()
    {
        // Base
        MaterialShader::Unload();

        _cache.Release();
        _cacheInstanced.Release();
    }

    bool DeferredMaterialShader::OnLoad()
    {
        bool failed = false;
        auto psDesc = GPUPipelineState::Description::Default;
        psDesc.DepthWriteEnable = m_Info.FeaturesFlags.IsNotFlag(MaterialFeatures::DisableDepthWrite);
        if (m_Info.FeaturesFlags.IsFlag(MaterialFeatures::DisableDepthTest))
        {
            psDesc.DepthFunc = ComparisonFunc::Always;
            if (!psDesc.DepthWriteEnable)
                psDesc.DepthEnable = false;
        }


        // Check if use tessellation (both material and runtime supports it)
        const bool useTess = m_Info.TessellationMode != TessellationMethod::None && GPUDevice::instance->GetGPULimits().HasTessellation;
        if (useTess)
        {
            psDesc.HS = m_Shader->GetHS(SE_TEXT("HS"));
            psDesc.DS = m_Shader->GetDS(SE_TEXT("DS"));
        }

        // GBuffer Pass
        psDesc.VS = m_Shader->GetVS(SE_TEXT("VS"));
        failed |= psDesc.VS == nullptr;
        psDesc.PS = m_Shader->GetPS(SE_TEXT("PS_GBuffer"));
        _cache.Default.Init(psDesc);
        psDesc.VS = m_Shader->GetVS(SE_TEXT("VS"), 1);
        failed |= psDesc.VS == nullptr;
        _cacheInstanced.Default.Init(psDesc);

        // GBuffer Pass with lightmap (pixel shader permutation for USE_LIGHTMAP=1)
        psDesc.VS = m_Shader->GetVS(SE_TEXT("VS"));
        failed |= psDesc.VS == nullptr;
        psDesc.PS = m_Shader->GetPS(SE_TEXT("PS_GBuffer"), 1);
        _cache.DefaultLightmap.Init(psDesc);
        psDesc.VS = m_Shader->GetVS(SE_TEXT("VS"), 1);
        failed |= psDesc.VS == nullptr;
        _cacheInstanced.DefaultLightmap.Init(psDesc);

        // GBuffer Pass with skinning
        psDesc.VS = m_Shader->GetVS(SE_TEXT("VS_Skinned"));
        psDesc.PS = m_Shader->GetPS(SE_TEXT("PS_GBuffer"));
        _cache.DefaultSkinned.Init(psDesc);

#if SE_EDITOR
        if (m_Shader->HasShader(SE_TEXT("PS_QuadOverdraw")))
        {
            // Quad Overdraw
            psDesc.VS = m_Shader->GetVS(SE_TEXT("VS"));
            psDesc.PS = m_Shader->GetPS(SE_TEXT("PS_QuadOverdraw"));
            _cache.QuadOverdraw.Init(psDesc);
            psDesc.VS = m_Shader->GetVS(SE_TEXT("VS"), 1);
            _cacheInstanced.Depth.Init(psDesc);
            psDesc.VS = m_Shader->GetVS(SE_TEXT("VS_Skinned"));
            _cache.QuadOverdrawSkinned.Init(psDesc);
        }
#endif

        // Motion Vectors pass
        psDesc.DepthWriteEnable = false;
        psDesc.DepthEnable = true;
        psDesc.DepthFunc = ComparisonFunc::LessEqual;
        psDesc.VS = m_Shader->GetVS(SE_TEXT("VS"));
        psDesc.PS = m_Shader->GetPS(SE_TEXT("PS_MotionVectors"));
        _cache.MotionVectors.Init(psDesc);

        // Motion Vectors pass with skinning
        psDesc.VS = m_Shader->GetVS(SE_TEXT("VS_Skinned"));
        _cache.MotionVectorsSkinned.Init(psDesc);

        // Motion Vectors pass with skinning (with per-bone motion blur)
        psDesc.VS = m_Shader->GetVS(SE_TEXT("VS_Skinned"), 1);
        _cache.MotionVectorsSkinnedPerBone.Init(psDesc);

        // Depth Pass
        psDesc.CullMode = CullMode::TwoSided;
        psDesc.DepthClipEnable = false;
        psDesc.DepthWriteEnable = true;
        psDesc.DepthEnable = true;
        psDesc.DepthFunc = ComparisonFunc::Less;
        psDesc.HS = nullptr;
        psDesc.DS = nullptr;
        GPUShaderProgramVS* instancedDepthPassVS;
        if (m_Info.UsageFlags.AllFlagsSet(MaterialUsage::UseMask, MaterialUsage::UsePositionOffset))
        {
            // Materials with masking need full vertex buffer to get texcoord used to sample textures for per pixel masking.
            // Materials with world pos offset need full VB to apply offset using texcoord etc.
            psDesc.VS = m_Shader->GetVS(SE_TEXT("VS"));
            instancedDepthPassVS = m_Shader->GetVS(SE_TEXT("VS"), 1);
            psDesc.PS = m_Shader->GetPS(SE_TEXT("PS_Depth"));
        }
        else
        {
            psDesc.VS = m_Shader->GetVS(SE_TEXT("VS_Depth"));
            instancedDepthPassVS = m_Shader->GetVS(SE_TEXT("VS_Depth"), 1);
            psDesc.PS = nullptr;
        }
        _cache.Depth.Init(psDesc);
        psDesc.VS = instancedDepthPassVS;
        _cacheInstanced.Depth.Init(psDesc);

        // Depth Pass with skinning
        psDesc.VS = m_Shader->GetVS(SE_TEXT("VS_Skinned"));
        _cache.DepthSkinned.Init(psDesc);

        return !failed;
    }
} // SE