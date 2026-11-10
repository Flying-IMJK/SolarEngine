#include <Runtime/Render/Assets/Material/MaterialShaderFeatures.h>

namespace SE
{
#if SE_EDITOR
    void ForwardShadingFeature::Generate(GeneratorData& data)
    {
        data.Template = SE_TEXT("Features/ForwardShading.hlsl");
    }

    void DeferredShadingFeature::Generate(GeneratorData& data)
    {
        data.Template = SE_TEXT("Features/DeferredShading.hlsl");
    }

    void TessellationFeature::Generate(GeneratorData& data)
    {
        data.Template = SE_TEXT("Features/Tessellation.hlsl");
    }

    void LightmapFeature::Generate(GeneratorData& data)
    {
        data.Template = SE_TEXT("Features/Lightmap.hlsl");
    }

    void DistortionFeature::Generate(GeneratorData& data) { data.Template = SE_TEXT("Features/Distortion.hlsl"); }

    void MotionVectorsFeature::Generate(GeneratorData& data)
    {
        data.Template = SE_TEXT("Features/MotionVectors.hlsl");
    }

#endif
} // namespace SE
