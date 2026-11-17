#include <Runtime/Render/RenderFeatures/ExponentialHeightFogRender.h>

#include <Runtime/Core/Profiler/Profiler.h>
#include <Runtime/Graphics/Base/GPUPipelineState.h>
#include <Runtime/Graphics/GPUContext.h>
#include <Runtime/Graphics/GPUDevice.h>
#include <Runtime/Graphics/Textures/GPUTexture.h>
#include <Runtime/Render/RenderBuffers.h>
#include <Runtime/Render/RenderContext.h>
#include <Runtime/Render/RenderList.h>
#include <Runtime/Resource/AssetContent.h>

namespace SE
{
    ExponentialHeightFogRender::~ExponentialHeightFogRender()
    {
        if (m_Shader)
        {
            m_Shader->OnReloadingEvent.Unbind<ExponentialHeightFogRender, &ExponentialHeightFogRender::OnShaderReloading>(this);
        }
        OnShaderReloading(nullptr);
    }

    void ExponentialHeightFogRender::OnShaderReloading(Asset* pAsset)
    {
        for (int32 i = 0; i < 2; i++)
        {
            DeleteObjectSafe(m_Pipelines[i]);
            DeleteObjectSafe(m_Programs[i]);
        }
    }

    bool ExponentialHeightFogRender::SetupResources(bool useAtmosphere)
    {
        auto device = GPUDevice::instance;
        if (!m_Shader)
        {
            m_Shader = AssetContent::LoadAsyncInternal<Shader>(SE_TEXT("Shaders/ExponentialHeightFog"));
            if (!m_Shader)
            {
                LOG_ERROR("Render", "Cannot load exponential height fog shader.");
                return false;
            }
            m_Shader->OnReloadingEvent.Bind<ExponentialHeightFogRender, &ExponentialHeightFogRender::OnShaderReloading>(this);
        }
        if (!m_Shader->IsLoaded())
        {
            return false;
        }

        const int32 index = useAtmosphere ? 1 : 0;
        if (m_Pipelines[index] && m_Programs[index])
        {
            return true;
        }
        GPUShaderProgram* program = nullptr;
        if (!m_Shader->SelectProgram(useAtmosphere ? SE_TEXT("HeightFogAtmosphere") : SE_TEXT("HeightFog"), {}, program))
        {
            return false;
        }

        auto description                            = GPUPipelineState::Description::DefaultFullscreenTriangle;
        description.BlendMode                       = BlendingMode::Add;
        description.BlendMode.DestBlend             = BlendingMode::Blend::SrcAlpha;
        description.BlendMode.RenderTargetWriteMask = BlendingMode::ColorWrite::RGB;
        m_Pipelines[index]                          = device->CreatePipelineState();
        if (!m_Pipelines[index] || !m_Pipelines[index]->Init(description, program))
        {
            DeleteObjectSafe(m_Pipelines[index]);
            return false;
        }
        m_Programs[index] = New<ShaderProgramInstance>();
        if (!m_Programs[index]->Initialize(program))
        {
            DeleteObjectSafe(m_Programs[index]);
            DeleteObjectSafe(m_Pipelines[index]);
            return false;
        }
        return true;
    }

    void ExponentialHeightFogRender::Render(RenderContext& renderContext, GPUTextureView* pOutput)
    {
        auto list = renderContext.list;
        if (!list->AtmosphericFog)
        {
            return;
        }
        ExponentialHeightFogData fog;
        list->AtmosphericFog->ApplyAtmosphericFog(fog);
        if ((fog.FogDensity <= 0.0f && fog.SecondFogDensity <= 0.0f) || fog.FogMaxOpacity <= 0.0f)
        {
            return;
        }

        auto       buffers       = renderContext.buffers;
        const bool useAtmosphere = list->SkyAtmosphere && buffers->SkyAtmosphereHeightFogSubmitted && buffers->SkyAtmosphereDistantSkyLightLut && buffers->SkyAtmosphereDistantSkyLightLut->IsAllocated();
        if (!SetupResources(useAtmosphere))
        {
            return;
        }

        PROFILE_GPU_CPU("ExponentialHeightFog");
        const auto& view = renderContext.view;
        struct FogData
        {
            Matrix InvProjection;
            Matrix InvView;
            Float4 CameraPosition;
            Float4 FogLayer0;
            Float4 FogLayer1;
            Float4 FogColorOpacity;
            Float4 Distances;
            Float4 LightDirection;
            Float4 DirectionalColor;
            Float4 AtmosphereAmbientScale;
        } data              = {};
        data.InvProjection  = view.IP;
        data.InvView        = view.IV;
        data.CameraPosition = Float4(view.Position, (view.Origin.Y - fog.FogHeight) + view.Position.Y);
        // UE stores artist densities per 1000 cm; SolarEngine ray lengths are meters.
        data.FogLayer0                = Float4(Math::Clamp(fog.FogDensity, 0.0f, 10.0f) * 0.1f, Math::Clamp(fog.FogHeightFalloff, 0.0f, 2.0f) * 0.1f, 0.0f, 0.0f);
        data.FogLayer1                = Float4(Math::Clamp(fog.SecondFogDensity, 0.0f, 10.0f) * 0.1f, Math::Clamp(fog.SecondFogHeightFalloff, 0.0f, 2.0f) * 0.1f, fog.SecondFogHeightOffset, 0.0f);
        data.FogColorOpacity          = Float4(fog.FogInscatteringColor.ToFloat3(), Math::Clamp(fog.FogMaxOpacity, 0.0f, 1.0f));
        data.Distances                = Float4(Math::Max(fog.StartDistance, 0.0f), Math::Max(fog.FogCutoffDistance, 0.0f), Math::Max(fog.DirectionalInscatteringStartDistance, 0.0f), view.IsOrthographicProjection() ? 1.0f : 0.0f);
        Float3 lightDirection         = Float3::Up;
        Float3 lightIlluminance       = Float3::Zero;
        float  atmosphereContribution = 0.0f;
        if (list->SkyAtmosphere)
        {
            if (useAtmosphere || buffers->SkyAtmosphereLutsSubmitted)
            {
                lightDirection         = Float3(buffers->SkyAtmosphereView.SunDirection);
                lightIlluminance       = Float3(buffers->SkyAtmosphereView.SunIlluminance);
                atmosphereContribution = buffers->SkyAtmosphereView.SunIlluminance.W;
            }
            else
            {
                // Only the base directional color needs this provider when no LUT was submitted.
                SkyAtmosphereData atmosphere;
                list->SkyAtmosphere->ApplySkyAtmosphere(atmosphere);
                lightDirection   = atmosphere.SunDirection;
                lightIlluminance = atmosphere.SunIlluminance;
            }
        }
        else if (list->DirectionalLights.HasItems())
        {
            lightDirection   = -list->DirectionalLights.First().Direction;
            lightIlluminance = list->DirectionalLights.First().Color;
        }
        const float lightLuminance  = Math::Max(Float3::Dot(lightIlluminance, Float3(0.2126f, 0.7152f, 0.0722f)), 0.0f);
        data.LightDirection         = Float4(Float3::Normalize(lightDirection), Math::Clamp(fog.DirectionalInscatteringExponent, 0.000001f, 1000.0f));
        data.DirectionalColor       = Float4(fog.DirectionalInscatteringColor.ToFloat3() * lightLuminance, atmosphereContribution);
        data.AtmosphereAmbientScale = Float4(fog.SkyAtmosphereAmbientContributionColorScale.ToFloat3(), 0.0f);

        const int32 index   = useAtmosphere ? 1 : 0;
        auto        program = m_Programs[index];
        if (!program->SetConstantBuffer(SE_TEXT("fogData"), &data, sizeof(data)) || !program->SetTexture(SE_TEXT("SceneDepth"), buffers->DepthBuffer->View()))
        {
            return;
        }
        if (useAtmosphere && !program->SetTexture(SE_TEXT("AtmosphereFogLighting"), buffers->SkyAtmosphereDistantSkyLightLut->View()))
        {
            return;
        }
        auto context = renderContext.gpuContext;
        context->ResetRenderTarget();
        context->SetRenderTarget(pOutput);
        context->SetState(m_Pipelines[index]);
        context->DrawFullscreenTriangle(program);
    }
} // namespace SE
