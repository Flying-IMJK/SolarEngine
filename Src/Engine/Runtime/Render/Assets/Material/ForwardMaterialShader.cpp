#include <Runtime/Render/Assets/Material/ForwardMaterialShader.h>

#include <Runtime/Render/RenderContext.h>
#include <Runtime/Render/RenderList.h>

namespace SE
{
    DrawPass ForwardMaterialShader::GetDrawModes() const
    {
        return HasProgram(SE_TEXT("Distortion")) ? DrawPass::Depth | DrawPass::Forward | DrawPass::Distortion :
                                                   DrawPass::Depth | DrawPass::Forward;
    }

    bool ForwardMaterialShader::CanUseInstancing(InstancingHandler& handler) const
    {
        handler = {
            SurfaceDrawCallHandler::GetHash, SurfaceDrawCallHandler::CanBatch, SurfaceDrawCallHandler::WriteDrawCall};
        return true;
    }

    void ForwardMaterialShader::Bind(BindParameters& params)
    {
        m_SelectedProgram  = nullptr;
        auto   description = GetSurfacePipelineDescription(params);
        String program;
        switch (params.renderContext.view.Pass)
        {
            case DrawPass::Depth:
                program = SE_TEXT("Depth");
                break;
            case DrawPass::Distortion:
                program                      = SE_TEXT("Distortion");
                description.DepthWriteEnable = false;
                description.BlendMode        = BlendingMode::Add;
                break;
            case DrawPass::Forward:
                program                      = SE_TEXT("Forward");
                description.DepthWriteEnable = false;
                switch (m_Info.BlendMode)
                {
                    case MaterialBlendMode::Additive:
                        description.BlendMode = BlendingMode::Additive;
                        break;
                    case MaterialBlendMode::Multiply:
                        description.BlendMode = BlendingMode::Multiply;
                        break;
                    default:
                        description.BlendMode = BlendingMode::AlphaBlend;
                        break;
                }
                break;
            default:
                return;
        }
        BindProgram(params, program, description);
    }
} // namespace SE
