#include <Runtime/Render/RenderFeatures/SkyAtmospherePreCompute.h>

#include <Runtime/Core/Platform/Platform.h>
#include <Runtime/Graphics/Base/GPULimits.h>
#include <Runtime/Graphics/GPUContext.h>
#include <Runtime/Graphics/GPUDevice.h>
#include <Runtime/Graphics/Textures/GPUSampler.h>
#include <Runtime/Graphics/Textures/GPUTexture.h>
#include <Runtime/Render/RenderBuffers.h>
#include <Runtime/Render/RenderContext.h>
#include <Runtime/Render/RenderDrawCall.h>
#include <Runtime/Render/RenderList.h>
#include <Runtime/Resource/AssetContent.h>

namespace SE
{
    SkyAtmospherePreCompute::~SkyAtmospherePreCompute()
    {
        Release();
    }

    void SkyAtmospherePreCompute::OnShaderReloading(Asset* pAsset)
    {
        // The asset destroys its immutable programs after this event.
        DeleteObjectSafe(m_TransmittanceProgram);
        DeleteObjectSafe(m_MultiScatteringProgram);
        DeleteObjectSafe(m_SkyViewProgram);
        DeleteObjectSafe(m_AerialPerspectiveProgram);
        DeleteObjectSafe(m_DistantSkyLightProgram);
        // Each render task invalidates its base LUTs when it next sees this version.
        ++m_ShaderVersion;
    }

    void SkyAtmospherePreCompute::Release()
    {
        if (m_Shader)
        {
            m_Shader->OnReloadingEvent.Unbind<SkyAtmospherePreCompute, &SkyAtmospherePreCompute::OnShaderReloading>(this);
        }
        OnShaderReloading(nullptr);
        m_Shader = nullptr;
        DeleteObjectSafe(m_Sampler);
    }

    bool SkyAtmospherePreCompute::SetupResources()
    {
        if (!m_Sampler)
        {
            m_Sampler = GPUDevice::instance->CreateSampler();
            auto description = GPUSamplerDescription::New(GPUSamplerFilter::Bilinear, GPUSamplerAddressMode::Clamp);
            if (!m_Sampler || !m_Sampler->Init(description))
            {
                DeleteObjectSafe(m_Sampler);
                return false;
            }
        }
        if (!m_Shader)
        {
            m_Shader = AssetContent::LoadAsyncInternal<Shader>(SE_TEXT("Shaders/SkyAtmospherePreCompute"));
            if (!m_Shader)
            {
                LOG_ERROR("Render", "Cannot load atmosphere precompute shader.");
                return false;
            }
            m_Shader->OnReloadingEvent.Bind<SkyAtmospherePreCompute, &SkyAtmospherePreCompute::OnShaderReloading>(this);
        }
        if (!m_Shader->IsLoaded())
        {
            return false;
        }

        ShaderProgramInstance** instances[] = {&m_TransmittanceProgram, &m_MultiScatteringProgram, &m_SkyViewProgram, &m_AerialPerspectiveProgram, &m_DistantSkyLightProgram};
        const Char* names[] = {SE_TEXT("Transmittance"), SE_TEXT("MultiScattering"), SE_TEXT("SkyView"), SE_TEXT("AerialPerspective"), SE_TEXT("DistantSkyLight")};
        for (int32 i = 0; i < 5; i++)
        {
            if (*instances[i])
            {
                continue;
            }
            GPUShaderProgram* program = nullptr;
            if (!m_Shader->SelectProgram(names[i], {}, program))
            {
                return false;
            }
            *instances[i] = New<ShaderProgramInstance>();
            if (!(*instances[i])->Initialize(program))
            {
                DeleteObjectSafe(*instances[i]);
                return false;
            }
        }

        auto  device   = GPUDevice::instance;
        const FormatSupport support  = device->GetPixelFormatFeatures(PixelFormat::R16G16B16A16_Float).Support;
        const FormatSupport required = FormatSupport::Texture2D | FormatSupport::Texture3D | FormatSupport::ShaderSample | FormatSupport::TypedUnorderedAccessView;
        if (!device->GetGPULimits().HasCompute)
        {
            LOG_ERROR("Render", "Atmosphere precompute requires compute.");
            return false;
        }

        if ((support & required) != required)
        {
            LOG_ERROR("Render", "Atmosphere precompute requires sampleable/storage RGBA16F 2D and 3D textures.");
            return false;
        }

        const FormatSupport hdrSupport = device->GetPixelFormatFeatures(PixelFormat::R32G32B32A32_Float).Support;
        if ((hdrSupport & required) != required)
        {
            LOG_ERROR("Render", "Atmosphere luminance requires sampleable/storage RGBA32F 2D and 3D textures.");
            return false;
        }
        return true;
    }

    bool SkyAtmospherePreCompute::Update(RenderContext& renderContext, ISkyAtmosphereRenderer* atmosphereRender)
    {
        auto buffers = renderContext.buffers;
        auto context = renderContext.gpuContext;
        buffers->SkyAtmosphereLutsSubmitted = false;
        buffers->SkyAtmosphereHeightFogSubmitted = false;
        if (!SetupResources())
        {
            return false;
        }

        SkyAtmosphereData atmosphereData;
        atmosphereRender->ApplySkyAtmosphere(atmosphereData);

        SkyAtmosphereParameters atmosphere = {};
        atmosphere.RadiiDensity            = Float4(atmosphereData.GroundRadius,
                                                atmosphereData.GroundRadius + atmosphereData.AtmosphereHeight,
                                                atmosphereData.RayleighScaleHeight,
                                                atmosphereData.MieScaleHeight);
        atmosphere.RayleighScattering      = Float4(atmosphereData.RayleighScattering.ToFloat3() * atmosphereData.RayleighScatteringScale, 0.0f);
        atmosphere.MieScattering           = Float4(atmosphereData.MieScattering.ToFloat3() * atmosphereData.MieScatteringScale, atmosphereData.MieAnisotropy);
        atmosphere.MieAbsorption           = Float4(atmosphereData.MieAbsorption.ToFloat3() * atmosphereData.MieAbsorptionScale, 0.0f);
        atmosphere.AbsorptionExtinction    = Float4(atmosphereData.AbsorptionExtinction.ToFloat3() * atmosphereData.AbsorptionExtinctionScale, 0.0f);
        atmosphere.AbsorptionDensity       = Float4(atmosphereData.AbsorptionTipAltitude, atmosphereData.AbsorptionWidth, 0.0f, 0.0f);
        atmosphere.GroundAlbedo            = Float4(atmosphereData.GroundAlbedo.ToFloat3(), atmosphereData.MultiScatteringFactor);


        bool changed = !buffers->SkyAtmosphereBaseLutsSubmitted ||
                       buffers->SkyAtmosphereShaderVersion != m_ShaderVersion ||
                       Platform::MemoryCompare(&atmosphere, &buffers->SkyAtmosphereCachedParameters, sizeof(atmosphere)) != 0;

        GPUTexture** textures[] = {&buffers->SkyAtmosphereTransmittanceLut, &buffers->SkyAtmosphereMultiScatteringLut, &buffers->SkyAtmosphereSkyViewLut, &buffers->SkyAtmosphereAerialPerspectiveLut, &buffers->SkyAtmosphereDistantSkyLightLut};
        const int32 widths[] = {256, 32, 192, 32, 2};
        const int32 heights[] = {64, 32, 104, 32, 1};
        const Char* textureNames[] = {SE_TEXT("Atmosphere.Transmittance"), SE_TEXT("Atmosphere.MultiScattering"), SE_TEXT("Atmosphere.SkyView"), SE_TEXT("Atmosphere.AerialPerspective"), SE_TEXT("Atmosphere.DistantSkyLight")};
        for (int32 i = 0; i < 5; i++)
        {
            if (*textures[i] && (*textures[i])->IsAllocated())
            {
                continue;
            }
            if (!*textures[i])
            {
                *textures[i] = renderContext.gpuContext->GetDevice()->CreateTexture(textureNames[i]);
            }
            if (!*textures[i])
            {
                return false;
            }

            const GPUTextureFlags flags = GPUTextureFlags::ShaderResource | GPUTextureFlags::UnorderedAccess;

            GPUTextureDescription description;
            if (i == 3)
            {
                description = GPUTextureDescription::New3D(32, 32, 16, PixelFormat::R32G32B32A32_Float, flags);
            }
            else
            {
                // Transmittance and unit-illuminance multiple scattering remain half precision.
                const auto format = i >= 2 ? PixelFormat::R32G32B32A32_Float : PixelFormat::R16G16B16A16_Float;
                description = GPUTextureDescription::New2D(widths[i], heights[i], format, flags, 1);
            }

            if (!(*textures[i])->Init(description))
            {
                return false;
            }
            if (i < 2)
            {
                changed = true;
                buffers->SkyAtmosphereBaseLutsSubmitted = false;
            }
        }

        const auto& view = renderContext.view;

        struct PreComputeData
        {
            SkyAtmosphereParameters Atmosphere;
            SkyAtmosphereViewParameters View;
        } data = {};
        data.Atmosphere = atmosphere;
        auto& viewData = data.View;

        // Subtract world positions before adding the planet radius; scene units are meters.
        Float3 cameraPosition = view.Origin + view.Position - atmosphereData.GroundPosition;
        cameraPosition.X *= 0.001f;
        cameraPosition.Y *= 0.001f;
        cameraPosition.Y += atmosphereData.GroundRadius;
        cameraPosition.Z *= 0.001f;

        const float height = cameraPosition.Length();
        if (cameraPosition.IsNaN() || cameraPosition.IsInfinity() || !std::isfinite(height) || height < atmosphereData.GroundRadius)
        {
            LOG_ERROR("Render", "Atmosphere camera must be finite and on or above the planet surface.");
            return false;
        }

        // A one-meter radial offset prevents the surface origin from intersecting itself.
        cameraPosition *= Math::Max(height, atmosphereData.GroundRadius + 0.001f) / height;
        const Float3 up = Float3::Normalize(cameraPosition);
        const Float3 reference = std::abs(up.Z) < 0.99f ? Float3::Forward : Float3::Right;
        const Float3 right = Float3::Normalize(Float3::Cross(up, reference));
        const Float3 forward = Float3::Cross(right, up);
        viewData.InvProjection = view.IP;
        viewData.InvView = view.IV;
        viewData.CameraPositionKm = Float4(cameraPosition, 0.0f);
        viewData.SkyRight = Float4(right, 0.0f);
        viewData.SkyForward = Float4(forward, 0.0f);
        viewData.SkyUp = Float4(up, 0.0f);
        Float3 direction = Float3::Up;
        Float3 illuminance = Float3::Zero;
        if (renderContext.list->DirectionalLights.HasItems())
        {
            const auto& light = renderContext.list->DirectionalLights.First();
            direction = -light.Direction;
            illuminance = light.Illuminance;
        }
        viewData.SunDirection = Float4(Float3::Normalize(direction), 0.5f * 0.545f * Math::DegreesToRadians * atmosphereData.SunDiscScale);
        viewData.SunIlluminance = Float4(illuminance, Math::Max(atmosphereData.HeightFogContribution, 0.0f));
        viewData.AerialPerspective = Float4(atmosphereData.AerialPerspectiveStartDepth, atmosphereData.AerialPerspectiveDepth, view.IsOrthographicProjection() ? 1.0f : 0.0f, 0.0f);

        // Bind only resources consumed by each entry point. Missing required bindings abort dispatch.
        if (changed)
        {
            if (!m_TransmittanceProgram->SetConstantBuffer(SE_TEXT("data"), &data, sizeof(data)) ||
                !m_TransmittanceProgram->SetTexture(SE_TEXT("TransmittanceOutput"), buffers->SkyAtmosphereTransmittanceLut->View()))
            {
                return false;
            }
            context->Dispatch(*m_TransmittanceProgram, 32, 8, 1);
            if (!m_MultiScatteringProgram->SetConstantBuffer(SE_TEXT("data"), &data, sizeof(data)) ||
                !m_MultiScatteringProgram->SetTexture(SE_TEXT("TransmittanceLut"), buffers->SkyAtmosphereTransmittanceLut->View()) ||
                !m_MultiScatteringProgram->SetSampler(SE_TEXT("AtmosphereSampler"), m_Sampler) ||
                !m_MultiScatteringProgram->SetTexture(SE_TEXT("MultiScatteringOutput"), buffers->SkyAtmosphereMultiScatteringLut->View()))
            {
                buffers->SkyAtmosphereBaseLutsSubmitted = false;
                return false;
            }
            context->Dispatch(*m_MultiScatteringProgram, 4, 4, 1);
            buffers->SkyAtmosphereCachedParameters = atmosphere;
            buffers->SkyAtmosphereShaderVersion = m_ShaderVersion;
            buffers->SkyAtmosphereBaseLutsSubmitted = true;
        }

        // Fog lighting has its own submission flag so a failed sky-view update cannot reuse old lighting.
        buffers->SkyAtmosphereView = viewData;
        if (!m_DistantSkyLightProgram->SetConstantBuffer(SE_TEXT("data"), &data, sizeof(data)) ||
            !m_DistantSkyLightProgram->SetTexture(SE_TEXT("TransmittanceLut"), buffers->SkyAtmosphereTransmittanceLut->View()) ||
            !m_DistantSkyLightProgram->SetTexture(SE_TEXT("MultiScatteringLut"), buffers->SkyAtmosphereMultiScatteringLut->View()) ||
            !m_DistantSkyLightProgram->SetSampler(SE_TEXT("AtmosphereSampler"), m_Sampler) ||
            !m_DistantSkyLightProgram->SetTexture(SE_TEXT("DistantSkyLightOutput"), buffers->SkyAtmosphereDistantSkyLightLut->View()))
        {
            return false;
        }
        context->Dispatch(*m_DistantSkyLightProgram, 1, 1, 1);
        buffers->SkyAtmosphereHeightFogSubmitted = true;

        ShaderProgramInstance* viewPrograms[] = {m_SkyViewProgram, m_AerialPerspectiveProgram};
        for (auto program : viewPrograms)
        {
            if (!program->SetConstantBuffer(SE_TEXT("data"), &data, sizeof(data)) ||
                !program->SetTexture(SE_TEXT("TransmittanceLut"), buffers->SkyAtmosphereTransmittanceLut->View()) ||
                !program->SetSampler(SE_TEXT("AtmosphereSampler"), m_Sampler) ||
                !program->SetTexture(SE_TEXT("MultiScatteringLut"), buffers->SkyAtmosphereMultiScatteringLut->View()))
            {
                return false;
            }
        }
        if (!m_SkyViewProgram->SetTexture(SE_TEXT("SkyViewOutput"), buffers->SkyAtmosphereSkyViewLut->View()) ||
            !m_AerialPerspectiveProgram->SetTexture(SE_TEXT("AerialPerspectiveOutput"), buffers->SkyAtmosphereAerialPerspectiveLut->ViewVolume()))
        {
            return false;
        }
        context->Dispatch(*m_SkyViewProgram, 24, 13, 1);
        context->Dispatch(*m_AerialPerspectiveProgram, 8, 8, 4);
        buffers->SkyAtmosphereView = viewData;
        buffers->SkyAtmosphereLutsSubmitted = true;
        return true;
    }
} // namespace SE
