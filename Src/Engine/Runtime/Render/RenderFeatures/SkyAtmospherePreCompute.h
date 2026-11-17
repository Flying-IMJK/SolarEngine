#pragma once

#include <Runtime/Core/Math/Matrix.h>
#include <Runtime/Core/Math/Vector4.h>

#include <Runtime/Render/RenderDrawCall.h>
#include <Runtime/Resource/AssetRef.h>
#include <Runtime/Resource/Assets/Materials/Shader.h>

namespace SE
{
    class GPUTexture;
    class GPUSampler;
    struct RenderContext;

    // GPU layout: distances in km and scattering/absorption coefficients in km^-1.
    struct SkyAtmosphereParameters
    {
        Float4 RadiiDensity;
        Float4 RayleighScattering;
        Float4 MieScattering;
        Float4 MieAbsorption;
        Float4 AbsorptionExtinction;
        Float4 AbsorptionDensity;
        Float4 GroundAlbedo;
    };

    // Stored with each RenderBuffers instance; independent of LinkedCustomBuffers.
    struct SkyAtmosphereViewParameters
    {
        Matrix InvProjection;
        Matrix InvView;
        Float4 CameraPositionKm;
        // XYZ: direction toward the sun; W: disk angular radius in radians.
        Float4 SunDirection;
        // RGB: unattenuated engine illuminance; W: height-fog contribution scale.
        Float4 SunIlluminance;
        Float4 SkyRight;
        Float4 SkyForward;
        Float4 SkyUp;
        // Start distance (km), volume depth (km), orthographic flag, padding.
        Float4 AerialPerspective;
    };

    // Render-thread only. Owned by the rendering pipeline; LUTs and cache live in RenderBuffers.
    class SkyAtmospherePreCompute
    {
    public:
        ~SkyAtmospherePreCompute();
        // Requires a non-null parameter provider. Queues LUT writes without waiting for GPU completion.
        // Returns false when resources are not ready, the camera is invalid, or required bindings fail.
        bool Update(RenderContext& renderContext, ISkyAtmosphereRenderer* atmosphereRender);
        void Release();

    private:
        bool SetupResources();
        void OnShaderReloading(Asset* pAsset);

        AssetRef<Shader> m_Shader;
        ShaderProgramInstance* m_TransmittanceProgram = nullptr;
        ShaderProgramInstance* m_MultiScatteringProgram = nullptr;
        ShaderProgramInstance* m_SkyViewProgram = nullptr;
        ShaderProgramInstance* m_AerialPerspectiveProgram = nullptr;
        ShaderProgramInstance* m_DistantSkyLightProgram = nullptr;
        GPUSampler* m_Sampler = nullptr;
        uint64 m_ShaderVersion = 1;
    };
} // namespace SE
