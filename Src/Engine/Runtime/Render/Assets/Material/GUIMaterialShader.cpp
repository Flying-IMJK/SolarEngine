#include <Runtime/Render/Assets/Material/GUIMaterialShader.h>

#include <Runtime/Graphics/GPUContext.h>

namespace SE
{
    DrawPass GUIMaterialShader::GetDrawModes() const { return DrawPass::None; }

    void GUIMaterialShader::Bind(BindParameters& params)
    {
        params.Program               = nullptr;
        auto description             = SLC2GPUPipelineState::Description::DefaultFullscreenTriangle;
        description.Wireframe        = EnumHasAnyFlags(m_Info.FeaturesFlags, MaterialFeatures::Wireframe);
        description.BlendMode        = BlendingMode::AlphaBlend;
        description.DepthEnable      = params.gpuContext->IsDepthBufferBinded();
        description.DepthWriteEnable = description.DepthEnable;
        BindProgram(params, SE_TEXT("GUI"), description);
    }
} // namespace SE
