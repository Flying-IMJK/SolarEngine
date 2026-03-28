
#include "MaterialShaderFeatures.h"

#include "Core/Math/CollisionsHelper.h"
#include "Runtime/Graphics/GPUContext.h"
#include "Runtime/Render/RenderContext.h"

namespace SE
{
    void ForwardShadingFeature::Bind(MaterialShader::BindParameters& params, Span<byte>& cb, int32& srv)
    {
        auto cache = params.renderContext.list;
        auto& view = params.renderContext.view;
        auto& drawCall = *params.firstDrawCall;
        auto& data = *(Data*)cb.Get();
        ASSERT_LOW_LAYER(cb.Length() >= sizeof(Data));
        const int32 envProbeShaderRegisterIndex = srv + 0;
        const int32 skyLightShaderRegisterIndex = srv + 1;
        const int32 dirLightShaderRegisterIndex = srv + 2;
        const bool canUseShadow = !view.Pass.Is(DrawPass::Depth);

        // Set fog input
        /*if (cache->Fog)
        {
            cache->Fog->GetExponentialHeightFogData(view, data.ExponentialHeightFog);
        }
        else
        {
            data.ExponentialHeightFog.FogMinOpacity = 1.0f;
            data.ExponentialHeightFog.ApplyDirectionalInscattering = 0.0f;
        }*/

        // Set directional light input
        if (cache->DirectionalLights.HasItems())
        {
            const auto& dirLight = cache->DirectionalLights.First();
            const auto shadowPass = false;// ShadowsPass::Instance();
            const bool useShadow = false;// shadowPass->LastDirLightIndex == 0 && canUseShadow;
            if (useShadow)
            {
                /*data.DirectionalLightShadow = shadowPass->LastDirLight;
                params.gpuContext->BindSR(dirLightShaderRegisterIndex, shadowPass->LastDirLightShadowMap);*/
            }
            else
            {
                params.gpuContext->UnBindSR(dirLightShaderRegisterIndex);
            }
            dirLight.SetupLightData(&data.DirectionalLight, useShadow);
        }
        else
        {
            data.DirectionalLight.Color = Float3::Zero;
            data.DirectionalLight.CastShadows = 0.0f;
            params.gpuContext->UnBindSR(dirLightShaderRegisterIndex);
        }

        // Set sky light
        if (cache->SkyLights.HasItems())
        {
            /*auto& skyLight = cache->SkyLights.First();
            skyLight.SetupLightData(&data.SkyLight, false);
            const auto texture = skyLight.Image ? skyLight.Image->GetTexture() : nullptr;
            params.gpuContext->BindSR(skyLightShaderRegisterIndex, GET_TEXTURE_VIEW_SAFE(texture));*/
        }
        else
        {
            Platform::MemoryClear(&data.SkyLight, sizeof(data.SkyLight));
            params.gpuContext->UnBindSR(skyLightShaderRegisterIndex);
        }

        // Set reflection probe data
        /*EnvironmentProbe* probe = nullptr;
        // TODO: optimize env probe searching for a transparent material - use spatial cache for renderer to find it
        const BoundingSphere objectBoundsWorld(drawCall.ObjectPosition + view.Origin, drawCall.ObjectRadius);
        for (int32 i = 0; i < cache->EnvironmentProbes.Count(); i++)
        {
            const auto p = cache->EnvironmentProbes[i];
            if (CollisionsHelper::SphereIntersectsSphere(objectBoundsWorld, p->GetSphere()))
            {
                probe = p;
                break;
            }
        }
        if (probe && probe->GetProbe())
        {
            probe->SetupProbeData(params.renderContext, &data.EnvironmentProbe);
            params.gpuContext->BindSR(envProbeShaderRegisterIndex, probe->GetProbe());
        }
        else
        {
            data.EnvironmentProbe.Data1 = Float4::Zero;
            params.gpuContext->UnBindSR(envProbeShaderRegisterIndex);
        }*/

        // Set local lights
        data.LocalLightsCount = 0;
        const BoundingSphere objectBounds(drawCall.ObjectPosition, drawCall.ObjectRadius);
        // TODO: optimize lights searching for a transparent material - use spatial cache for renderer to find it
        for (int32 i = 0; i < cache->PointLights.Count() && data.LocalLightsCount < MaxLocalLights; i++)
        {
            const auto& light = cache->PointLights[i];
            if (CollisionsHelper::SphereIntersectsSphere(objectBounds, BoundingSphere(light.Position, light.Radius)))
            {
                light.SetupLightData(&data.LocalLights[data.LocalLightsCount], false);
                data.LocalLightsCount++;
            }
        }
        for (int32 i = 0; i < cache->SpotLights.Count() && data.LocalLightsCount < MaxLocalLights; i++)
        {
            const auto& light = cache->SpotLights[i];
            if (CollisionsHelper::SphereIntersectsSphere(objectBounds, BoundingSphere(light.Position, light.Radius)))
            {
                light.SetupLightData(&data.LocalLights[data.LocalLightsCount], false);
                data.LocalLightsCount++;
            }
        }

        cb = Span<byte>(cb.Get() + sizeof(Data), cb.Length() - sizeof(Data));
        srv += SRVs;
    }

    bool LightmapFeature::Bind(MaterialShader::BindParameters& params, Span<byte>& cb, int32& srv)
    {
        auto& drawCall = *params.firstDrawCall;
        ASSERT_LOW_LAYER(cb.Length() >= sizeof(Data));

        const bool useLightmap = false;// EnumHasAnyFlags(params.renderContext.view.Flags, ViewFlags::GI)
    /*#if SE_EDITOR
                && EnableLightmapsUsage
    #endif
                && drawCall.Surface.Lightmap != nullptr;*/
        if (useLightmap)
        {
            // Bind lightmap textures
            /*GPUTexture *lightmap0, *lightmap1, *lightmap2;
            drawCall.Features.Lightmap->GetTextures(&lightmap0, &lightmap1, &lightmap2);
            params.gpuContext->BindSR(srv + 0, lightmap0);
            params.gpuContext->BindSR(srv + 1, lightmap1);
            params.gpuContext->BindSR(srv + 2, lightmap2);*/

            // Set lightmap data
            auto& data = *(Data*)cb.Get();
            data.LightmapArea = drawCall.Features.LightmapUVsArea;
        }

        cb = Span<byte>(cb.Get() + sizeof(Data), cb.Length() - sizeof(Data));
        srv += SRVs;
        return useLightmap;
    }

    /*
    bool GlobalIlluminationFeature::Bind(MaterialShader::BindParameters& params, Span<byte>& cb, int32& srv)
    {
        auto& data = *(Data*)cb.Get();
        ASSERT_LOW_LAYER(cb.Length() >= sizeof(Data));
    
        bool useGI = false;
        if (EnumHasAnyFlags(params.RenderContext.View.Flags, ViewFlags::GI))
        {
            switch (params.RenderContext.List->Settings.GlobalIllumination.Mode)
            {
            case GlobalIlluminationMode::DDGI:
            {
                DynamicDiffuseGlobalIlluminationPass::BindingData bindingDataDDGI;
                if (!DynamicDiffuseGlobalIlluminationPass::Instance()->Get(params.RenderContext.Buffers, bindingDataDDGI))
                {
                    useGI = true;
    
                    // Bind DDGI data
                    data.DDGI = bindingDataDDGI.Constants;
                    params.GPUContext->BindSR(srv + 0, bindingDataDDGI.ProbesData);
                    params.GPUContext->BindSR(srv + 1, bindingDataDDGI.ProbesDistance);
                    params.GPUContext->BindSR(srv + 2, bindingDataDDGI.ProbesIrradiance);
                }
                break;
            }
            }
        }
        if (!useGI)
        {
            // Unbind SRVs to prevent issues
            data.DDGI.CascadesCount = 0;
            data.DDGI.FallbackIrradiance = Float3::Zero;
            params.gpuContext->UnBindSR(srv + 0);
            params.gpuContext->UnBindSR(srv + 1);
            params.gpuContext->UnBindSR(srv + 2);
        }
    
        cb = Span<byte>(cb.Get() + sizeof(Data), cb.Length() - sizeof(Data));
        srv += SRVs;
        return useGI;
    }
    */

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

    /*void GlobalIlluminationFeature::Generate(GeneratorData& data)
    {
        data.Template = SE_TEXT("Features/GlobalIllumination.hlsl");
    }*/

    void DistortionFeature::Generate(GeneratorData& data)
    {
        data.Template = SE_TEXT("Features/Distortion.hlsl");
    }

    void MotionVectorsFeature::Generate(GeneratorData& data)
    {
        data.Template = SE_TEXT("Features/MotionVectors.hlsl");
    }
#endif
    
} // SE