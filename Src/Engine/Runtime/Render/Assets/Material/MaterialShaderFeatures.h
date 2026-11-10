#pragma once

#include <Runtime/Core/Types/Strings/String.h>
#include <Runtime/API.h>

namespace SE
{
    struct MaterialShaderFeature
    {
#if SE_EDITOR
        struct GeneratorData
        {
            const Char* Template;
        };
#endif
    };

    struct ForwardShadingFeature : MaterialShaderFeature
    {
#if SE_EDITOR
        static void Generate(GeneratorData& data);
#endif
    };

    struct DeferredShadingFeature : MaterialShaderFeature
    {
#if SE_EDITOR
        static void Generate(GeneratorData& data);
#endif
    };

    struct TessellationFeature : MaterialShaderFeature
    {
#if SE_EDITOR
        static void Generate(GeneratorData& data);
#endif
    };

    struct LightmapFeature : MaterialShaderFeature
    {
#if SE_EDITOR
        static void Generate(GeneratorData& data);
#endif
    };

    struct DistortionFeature : MaterialShaderFeature
    {
#if SE_EDITOR
        static void Generate(GeneratorData& data);
#endif
    };

    struct MotionVectorsFeature : MaterialShaderFeature
    {
#if SE_EDITOR
        static void Generate(GeneratorData& data);
#endif
    };

} // namespace SE
