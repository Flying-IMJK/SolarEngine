

#include "ForwardMaterialShader.h"

#include "MaterialParams.h"
#include "Runtime/Graphics/GPUContext.h"
#include "Runtime/Graphics/GPUDevice.h"
#include "Runtime/Graphics/Shaders/GPUShader.h"
#include "Runtime/Render/RenderContext.h"
#include "Runtime/Render/RenderList.h"

namespace SE
{
	GES_PACK_STRUCT(struct ForwardMaterialShaderData {
    Matrix WorldMatrix;
    Matrix PrevWorldMatrix;
    Float2 Dummy0;
    float LODDitherFactor;
    float PerInstanceRandom;
    Float3 GeometrySize;
    float WorldDeterminantSign;
    });

EnumFlags<DrawPass> ForwardMaterialShader::GetDrawModes() const
{
    return _drawModes;
}

bool ForwardMaterialShader::CanUseInstancing(InstancingHandler& handler) const
{
    handler = { SurfaceDrawCallHandler::GetHash, SurfaceDrawCallHandler::CanBatch, SurfaceDrawCallHandler::WriteDrawCall, };
    return true;
}

void ForwardMaterialShader::Bind(BindParameters& params)
{
    // Prepare
    auto context = params.gpuContext;
    auto& view = params.renderContext.view;
    auto& drawCall = *params.firstDrawCall;
    Span<byte> cb(m_CBData.Get(), m_CBData.Count());
    ASSERT_LOW_LAYER(cb.Length() >= sizeof(ForwardMaterialShaderData));
    auto materialData = reinterpret_cast<ForwardMaterialShaderData*>(cb.Get());
    cb = Span<byte>(cb.Get() + sizeof(ForwardMaterialShaderData), cb.Length() - sizeof(ForwardMaterialShaderData));
    int32 srv = 2;

    // Setup features
    /*if (m_Info.FeaturesFlags.IsFlagSet(MaterialFeatures::GlobalIllumination))
    {
        GlobalIlluminationFeature::Bind(params, cb, srv);
    }*/
    // ForwardShadingFeature::Bind(params, cb, srv);

    // Setup parameters
    MaterialParameter::BindMeta bindMeta;
    bindMeta.Context = context;
    bindMeta.Constants = cb;
    bindMeta.Input = params.Input;
    bindMeta.Buffers = params.renderContext.buffers;
    bindMeta.CanSampleDepth = GPUDevice::instance->GetGPULimits().HasReadOnlyDepth;
    bindMeta.CanSampleGBuffer = true;
    MaterialParams::Bind(params.paramsLink, bindMeta);

    // Check if is using mesh skinning
    const bool useSkinning = false;// drawCall.Surface.Skinning != nullptr;
    /*if (useSkinning)
    {
        // Bind skinning buffer
        ENGINE_ASSERT(drawCall.Surface.Skinning->IsReady());
        context->BindSR(0, drawCall.Surface.Skinning->BoneMatrices->View());
    }*/

    // Setup material constants
    {
        Matrix::Transpose(drawCall.World, materialData->WorldMatrix);
        Matrix::Transpose(drawCall.Surface.PrevWorld, materialData->PrevWorldMatrix);
        materialData->WorldDeterminantSign = drawCall.WorldDeterminantSign;
        materialData->LODDitherFactor = drawCall.Surface.LODDitherFactor;
        materialData->PerInstanceRandom = drawCall.PerInstanceRandom;
        materialData->GeometrySize = drawCall.Surface.GeometrySize;
    }

    // Bind constants
    if (m_CB)
    {
        context->UpdateCB(m_CB, m_CBData.Get());
        context->BindCB(0, m_CB);
    }

    // Select pipeline state based on current pass and render mode
    const bool wireframe = m_Info.FeaturesFlags.IsFlag(MaterialFeatures::Wireframe) || view.Mode == ViewMode::Wireframe;
    CullMode cullMode = view.Pass.Is(DrawPass::Depth) ? CullMode::TwoSided : m_Info.CullMode;
/*#if SE_EDITOR
    if (IsRunningRadiancePass)
        cullMode = CullMode::TwoSided;
#endif*/
    if (cullMode != CullMode::TwoSided && drawCall.WorldDeterminantSign < 0)
    {
        // Invert culling when scale is negative
        if (cullMode == CullMode::Normal)
            cullMode = CullMode::Inverted;
        else
            cullMode = CullMode::Normal;
    }
    // ASSERT_LOW_LAYER(!(useSkinning && params.DrawCallsCount > 1)); // No support for instancing skinned meshes
    const auto cacheObj = params.drawCallsCount == 1 ? &_cache : &_cacheInstanced;
    PipelineStateCache* psCache = cacheObj->GetPS(view.Pass, useSkinning);
    ENGINE_ASSERT(psCache);
    GPUPipelineState* state = psCache->GetPS(cullMode, wireframe);

    // Bind pipeline
    context->SetState(state);
}

void ForwardMaterialShader::Unload()
{
    // Base
    MaterialShader::Unload();

    _cache.Release();
    _cacheInstanced.Release();
}

bool ForwardMaterialShader::OnLoad()
{
    _drawModes = {DrawPass::Depth, DrawPass::Forward, DrawPass::QuadOverdraw};

    auto psDesc = GPUPipelineState::Description::Default;
    psDesc.DepthEnable = m_Info.FeaturesFlags.IsNotFlag(MaterialFeatures::DisableDepthTest);
    psDesc.DepthWriteEnable = m_Info.FeaturesFlags.IsNotFlag(MaterialFeatures::DisableDepthWrite);


    // Check if use tessellation (both material and runtime supports it)
    const bool useTess = m_Info.TessellationMode != TessellationMethod::None && GPUDevice::instance->GetGPULimits().HasTessellation;
    if (useTess)
    {
        psDesc.HS = m_Shader->GetHS(SE_TEXT("HS"));
        psDesc.DS = m_Shader->GetDS(SE_TEXT("DS"));
    }

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

    // Check if use transparent distortion pass
    if (m_Shader->HasShader(SE_TEXT("PS_Distortion")))
    {
        _drawModes.SetFlag(DrawPass::Distortion);

        // Accumulate Distortion Pass
        psDesc.VS = m_Shader->GetVS(SE_TEXT("VS"));
        psDesc.PS = m_Shader->GetPS(SE_TEXT("PS_Distortion"));
        psDesc.BlendMode = BlendingMode::Add;
        psDesc.DepthWriteEnable = false;
        _cache.Distortion.Init(psDesc);
        //psDesc.VS = m_Shader->GetVS(SE_TEXT("VS"), 1);
        //_cacheInstanced.Distortion.Init(psDesc);
        psDesc.VS = m_Shader->GetVS(SE_TEXT("VS_Skinned"));
        _cache.DistortionSkinned.Init(psDesc);
    }

    // Forward Pass
    psDesc.VS = m_Shader->GetVS(SE_TEXT("VS"));
    if (psDesc.VS == nullptr)
        return true;
    psDesc.PS = m_Shader->GetPS(SE_TEXT("PS_Forward"));
    psDesc.DepthWriteEnable = false;
    psDesc.BlendMode = BlendingMode::AlphaBlend;
    switch (m_Info.BlendMode)
    {
    case MaterialBlendMode::Transparent:
        psDesc.BlendMode = BlendingMode::AlphaBlend;
        break;
    case MaterialBlendMode::Additive:
        psDesc.BlendMode = BlendingMode::Additive;
        break;
    case MaterialBlendMode::Multiply:
        psDesc.BlendMode = BlendingMode::Multiply;
        break;
    }
    _cache.Default.Init(psDesc);
    //psDesc.VS = m_Shader->GetVS(SE_TEXT("VS"), 1);
    //_cacheInstanced.Default.Init(psDesc);
    psDesc.VS = m_Shader->GetVS(SE_TEXT("VS_Skinned"));
    _cache.DefaultSkinned.Init(psDesc);

    // Depth Pass
    psDesc = GPUPipelineState::Description::Default;
    psDesc.CullMode = CullMode::TwoSided;
    psDesc.DepthClipEnable = false;
    psDesc.DepthWriteEnable = true;
    psDesc.DepthEnable = true;
    psDesc.DepthFunc = ComparisonFunc::Less;
    psDesc.HS = nullptr;
    psDesc.DS = nullptr;
    psDesc.VS = m_Shader->GetVS(SE_TEXT("VS"));
    psDesc.PS = m_Shader->GetPS(SE_TEXT("PS_Depth"));
    _cache.Depth.Init(psDesc);
    psDesc.VS = m_Shader->GetVS(SE_TEXT("VS"), 1);
    _cacheInstanced.Depth.Init(psDesc);
    psDesc.VS = m_Shader->GetVS(SE_TEXT("VS_Skinned"));
    _cache.DepthSkinned.Init(psDesc);

    return false;
}


	
} // SE