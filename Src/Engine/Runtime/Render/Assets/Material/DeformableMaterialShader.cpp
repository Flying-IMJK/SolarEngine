
#include "DeformableMaterialShader.h"

#include "MaterialParams.h"
#include "Core/Math/Matrix.h"
#include "Runtime/Graphics/GPUContext.h"
#include "Runtime/Graphics/GPUDevice.h"
#include "Runtime/Graphics/Base/GPUBuffer.h"
#include "Runtime/Graphics/Shaders/GPUShader.h"
#include "Runtime/Render/RenderContext.h"
#include "Runtime/Render/RenderDrawCall.h"

namespace SE
{
GES_PACK_STRUCT(struct DeformableMaterialShaderData {
    Matrix WorldMatrix;
    Matrix LocalMatrix;
    Float3 Dummy0;
    float WorldDeterminantSign;
    float MeshMinZ;
    float Segment;
    float ChunksPerSegment;
    float PerInstanceRandom;
    Float3 GeometrySize;
    float MeshMaxZ;
    });

EnumFlags<DrawPass> DeformableMaterialShader::GetDrawModes() const
{
    return m_DrawModes;
}

void DeformableMaterialShader::Bind(BindParameters& params)
{
    // Prepare
    auto context = params.gpuContext;
    auto& view = params.renderContext.view;
    auto& drawCall = *params.firstDrawCall;
    Span<byte> cb(m_CBData.Get(), m_CBData.Count());
    ASSERT_LOW_LAYER(cb.Length() >= sizeof(DeformableMaterialShaderData));
    auto materialData = reinterpret_cast<DeformableMaterialShaderData*>(cb.Get());
    cb = Span<byte>(cb.Get() + sizeof(DeformableMaterialShaderData), cb.Length() - sizeof(DeformableMaterialShaderData));
    int32 srv = 1;

    // Setup features
    /*if (m_Info.BlendMode != MaterialBlendMode::Opaque)
        ForwardShadingFeature::Bind(params, cb, srv);*/

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
        Matrix::Transpose(drawCall.Deformable.LocalMatrix, materialData->LocalMatrix);
        materialData->WorldDeterminantSign = drawCall.WorldDeterminantSign;
        materialData->Segment = drawCall.Deformable.Segment;
        materialData->ChunksPerSegment = drawCall.Deformable.ChunksPerSegment;
        materialData->MeshMinZ = drawCall.Deformable.MeshMinZ;
        materialData->MeshMaxZ = drawCall.Deformable.MeshMaxZ;
        materialData->PerInstanceRandom = drawCall.PerInstanceRandom;
        materialData->GeometrySize = drawCall.Deformable.GeometrySize;
    }

    // Bind spline deformation buffer
    context->BindSR(0, drawCall.Deformable.SplineDeformation->View());

    // Bind constants
    if (m_CB)
    {
        context->UpdateCB(m_CB, m_CBData.Get());
        context->BindCB(0, m_CB);
    }

    // Select pipeline state based on current pass and render mode
    const bool wireframe = m_Info.FeaturesFlags.IsFlag(MaterialFeatures::Wireframe) || view.Mode == ViewMode::Wireframe;
    CullMode cullMode = view.Pass.Is(DrawPass::Depth) ? CullMode::TwoSided : m_Info.CullMode;
    if (cullMode != CullMode::TwoSided && drawCall.WorldDeterminantSign < 0)
    {
        // Invert culling when scale is negative
        if (cullMode == CullMode::Normal)
            cullMode = CullMode::Inverted;
        else
            cullMode = CullMode::Normal;
    }
    PipelineStateCache* psCache = m_Cache.GetPS(view.Pass);
    ENGINE_ASSERT(psCache);
    GPUPipelineState* state = psCache->GetPS(cullMode, wireframe);

    // Bind pipeline
    context->SetState(state);
}

void DeformableMaterialShader::Unload()
{
    // Base
    MaterialShader::Unload();

    m_Cache.Release();
}

bool DeformableMaterialShader::OnLoad()
{
    m_DrawModes = {DrawPass::Depth, DrawPass::QuadOverdraw};
    auto psDesc = GPUPipelineState::Description::Default;
    psDesc.DepthEnable = !m_Info.FeaturesFlags.IsFlag(MaterialFeatures::DisableDepthTest);
    psDesc.DepthWriteEnable = !m_Info.FeaturesFlags.IsFlag(MaterialFeatures::DisableDepthWrite);

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
        psDesc.VS = m_Shader->GetVS(SE_TEXT("VS_SplineModel"));
        psDesc.PS = m_Shader->GetPS(SE_TEXT("PS_QuadOverdraw"));
        m_Cache.QuadOverdraw.Init(psDesc);
    }
#endif

    if (m_Info.BlendMode == MaterialBlendMode::Opaque)
    {
        m_DrawModes.SetFlags(DrawPass::GBuffer, DrawPass::GlobalSurfaceAtlas);

        // GBuffer Pass
        psDesc.VS = m_Shader->GetVS(SE_TEXT("VS_SplineModel"));
        psDesc.PS = m_Shader->GetPS(SE_TEXT("PS_GBuffer"));
        m_Cache.Default.Init(psDesc);
    }
    else
    {
        m_DrawModes.SetFlag(DrawPass::Forward);

        // Forward Pass
        psDesc.VS = m_Shader->GetVS(SE_TEXT("VS_SplineModel"));
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
        m_Cache.Default.Init(psDesc);
    }

    // Depth Pass
    psDesc.CullMode = CullMode::TwoSided;
    psDesc.DepthClipEnable = false;
    psDesc.DepthWriteEnable = true;
    psDesc.DepthEnable = true;
    psDesc.DepthFunc = ComparisonFunc::Less;
    psDesc.HS = nullptr;
    psDesc.DS = nullptr;
    m_Cache.Depth.Init(psDesc);

    return true;
}

	
} // SE