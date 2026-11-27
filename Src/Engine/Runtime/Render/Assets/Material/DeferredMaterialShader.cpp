#include <Runtime/Render/Assets/Material/DeferredMaterialShader.h>

#include <Runtime/Render/RenderContext.h>
#include <Runtime/Render/RenderList.h>

namespace SE
{
    DrawPass DeferredMaterialShader::GetDrawModes() const
    {
        DrawPass modes = DrawPass::Depth | DrawPass::GBuffer | DrawPass::GlobalSurfaceAtlas;
        if (HasProgram(SE_TEXT("MotionVectors")))
            modes |= DrawPass::MotionVectors;
        return modes;
    }

    bool DeferredMaterialShader::CanUseInstancing(InstancingHandler& handler) const
    {
        handler = {SurfaceDrawCallHandler::GetHash, SurfaceDrawCallHandler::CanBatch, SurfaceDrawCallHandler::WriteDrawCall};
        return true;
    }

    void DeferredMaterialShader::Bind(BindParameters& params)
    {
        m_SelectedProgram = nullptr;
        String         program;
        const DrawPass pass = params.renderContext.view.Pass;
        if (pass == DrawPass::Depth)
        {
            program = SE_TEXT("Depth");
        }
        else if (pass == DrawPass::MotionVectors && HasProgram(SE_TEXT("MotionVectors")))
        {
            program = SE_TEXT("MotionVectors");
        }
        else if (EnumHasAnyFlags(pass, DrawPass::GBuffer | DrawPass::GlobalSurfaceAtlas))
        {
            program = SE_TEXT("GBuffer");
        }
        else
        {
            return;
        }

        BindProgram(params, program, GetSurfacePipelineDescription(params));
    }
} // namespace SE
