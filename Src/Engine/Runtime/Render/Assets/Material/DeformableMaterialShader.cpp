#include <Runtime/Render/Assets/Material/DeformableMaterialShader.h>

#include <Runtime/Render/RenderContext.h>

namespace SE
{
    DrawPass DeformableMaterialShader::GetDrawModes() const
    {
        return DrawPass::Depth | (m_Info.BlendMode == MaterialBlendMode::Opaque ? DrawPass::GBuffer : DrawPass::Forward);
    }

    void DeformableMaterialShader::Bind(BindParameters& params)
    {
        params.Program = nullptr;
        if (params.UseInstancing)
        {
            return;
        }

        String program;
        switch (params.renderContext.view.Pass)
        {
            case DrawPass::Depth:
                program = SE_TEXT("Depth");
                break;
            case DrawPass::GBuffer:
                program = SE_TEXT("GBuffer");
                break;
            case DrawPass::Forward:
                program = SE_TEXT("Forward");
                break;
            default:
                return;
        }
        auto description = GetSurfacePipelineDescription(params);

        if (params.renderContext.view.Pass == DrawPass::Forward)
        {
            description.DepthWriteEnable = false;
            description.BlendMode        = m_Info.BlendMode == MaterialBlendMode::Additive ? BlendingMode::Additive :
                                           m_Info.BlendMode == MaterialBlendMode::Multiply ? BlendingMode::Multiply :
                                                                                             BlendingMode::AlphaBlend;
        }

        BindProgram(params, program, description);
    }
} // namespace SE
