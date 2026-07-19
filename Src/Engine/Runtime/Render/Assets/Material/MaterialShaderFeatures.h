#pragma once
#include "MaterialShader.h"
#include "Runtime/Core/Math/Rectangle.h"
#include "Runtime/Render/RenderList.h"

namespace SE
{
    // Material shader features are plugin-based functionalities that are reusable between different material domains.
    struct MaterialShaderFeature
    {
#if SE_EDITOR
        struct GeneratorData
        {
            const Char* Template;
        };
#endif
    };

    // Material shader feature that add support for Forward shading inside the material shader.
    struct ForwardShadingFeature : MaterialShaderFeature
    {
        enum { MaxLocalLights = 4 };

        enum { SRVs = 3 };

        GES_PACK_STRUCT(struct Data
            {
            LightData DirectionalLight;
            // LightShadowData DirectionalLightShadow;
            LightData SkyLight;
            /*ProbeData EnvironmentProbe;
            ExponentialHeightFogData ExponentialHeightFog;*/
            Float3 Dummy2;
            uint32 LocalLightsCount;
            LightData LocalLights[MaxLocalLights];
            });

        static void Bind(MaterialShader::BindParameters& params, Span<byte>& cb, int32& srv);
#if SE_EDITOR
        static void Generate(GeneratorData& data);
#endif
    };

    // Material shader feature that add support for Deferred shading inside the material shader.
    struct DeferredShadingFeature : MaterialShaderFeature
    {
#if SE_EDITOR
        static void Generate(GeneratorData& data);
#endif
    };

    // Material shader feature that adds geometry hardware tessellation (using Hull and Domain shaders).
    struct TessellationFeature : MaterialShaderFeature
    {
#if SE_EDITOR
        static void Generate(GeneratorData& data);
#endif
    };

    // Material shader feature that adds lightmap sampling feature.
    struct LightmapFeature : MaterialShaderFeature
    {
        enum { SRVs = 3 };

        GES_PACK_STRUCT(struct Data
            {
            Rectangle LightmapArea;
            });

        static bool Bind(MaterialShader::BindParameters& params, Span<byte>& cb, int32& srv);
#if SE_EDITOR
        static void Generate(GeneratorData& data);
#endif
    };

    // Material shader feature that adds Global Illumination sampling feature (light probes).
    /*struct GlobalIlluminationFeature : MaterialShaderFeature
    {
        enum { SRVs = 3 };

        GES_PACK_STRUCT(struct Data
            {
            DynamicDiffuseGlobalIlluminationPass::ConstantsData DDGI;
            });

        static bool Bind(MaterialShader::BindParameters& params, Span<byte>& cb, int32& srv);
#if SE_EDITOR
        static void Generate(GeneratorData& data);
#endif
    };*/

    // Material shader feature that adds distortion vectors rendering pass.
    struct DistortionFeature : MaterialShaderFeature
    {
#if SE_EDITOR
        static void Generate(GeneratorData& data);
#endif
    };

    // Material shader feature that adds motion vectors rendering pass.
    struct MotionVectorsFeature : MaterialShaderFeature
    {
#if SE_EDITOR
        static void Generate(GeneratorData& data);
#endif
    };
} // SE


