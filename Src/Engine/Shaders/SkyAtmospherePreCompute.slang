#include "Shaders/ProgramCommon.hlsl"
#include "Shaders/SkyAtmosphereCommon.hlsl"

SHADER_PROGRAM(Transmittance, SHADER_CS(CS_Transmittance))
SHADER_PROGRAM(MultiScattering, SHADER_CS(CS_MultiScattering))
SHADER_PROGRAM(SkyView, SHADER_CS(CS_SkyView))
SHADER_PROGRAM(AerialPerspective, SHADER_CS(CS_AerialPerspective))
SHADER_PROGRAM(DistantSkyLight, SHADER_CS(CS_DistantSkyLight))

RWTexture2D<float4> TransmittanceOutput;
RWTexture2D<float4> MultiScatteringOutput;
RWTexture2D<float4> SkyViewOutput;
RWTexture3D<float4> AerialPerspectiveOutput;
RWTexture2D<float4> DistantSkyLightOutput;

groupshared float3 DistantSkySamples[64];

// UE integrates 64 sphere directions at a fixed 6 km altitude using an isotropic phase.
[numthreads(8, 8, 1)]
void CS_DistantSkyLight(uint3 threadId : SV_GroupThreadID)
{
    uint index = threadId.y * 8 + threadId.x;
    float mu = 1.0 - 2.0 * (float(index) + 0.5) / 64.0;
    float longitude = float(index) * 2.399963229728653;
    float radius = sqrt(saturate(1.0 - mu * mu));
    float3 direction = float3(radius * cos(longitude), mu, radius * sin(longitude));
    float3 origin = float3(0.0, data.Atmosphere.RadiiDensity.x + 6.0, 0.0);
    float distance = 1.e9;
    float3 luminance = 0.0;
    // Thin atmospheres can put the fixed sample altitude above the top boundary.
    if (AtmosphereMoveToTop(origin, direction, distance))
    {
        AtmosphereIntegrationResult result = IntegrateAtmosphere(origin, direction, distance,
            data.View.SunDirection.xyz, data.View.SunIlluminance.rgb, 10.0, false, false, true, true);
        luminance = result.Luminance;
    }
    DistantSkySamples[index] = luminance;
    GroupMemoryBarrierWithGroupSync();

    for (uint stride = 32; stride > 0; stride >>= 1)
    {
        if (index < stride)
            DistantSkySamples[index] += DistantSkySamples[index + stride];
        GroupMemoryBarrierWithGroupSync();
    }
    if (index == 0)
    {
        // Sphere solid angle and uniform phase cancel, leaving the sample average.
        DistantSkyLightOutput[uint2(0, 0)] = float4(DistantSkySamples[0] / 64.0, 0.0);
        float3 ground = float3(0.0, data.Atmosphere.RadiiDensity.x + AtmosphereBoundaryOffset, 0.0);
        float3 sunlight = data.View.SunDirection.y > 0.0 ?
            data.View.SunIlluminance.rgb * AtmosphereTransmittance(ground, data.View.SunDirection.xyz) : 0.0;
        DistantSkyLightOutput[uint2(1, 0)] = float4(sunlight, 0.0);
    }
}

[numthreads(8, 8, 1)]
void CS_Transmittance(uint3 id : SV_DispatchThreadID)
{
    if (any(id.xy >= uint2(256, 64)))
    {
        return;
    }
    float radius, mu;
    AtmosphereTransmittanceParameters((float2(id.xy) + 0.5) / float2(256.0, 64.0), radius, mu);
    float3 origin = float3(0.0, 0.0, radius);
    float3 direction = float3(sqrt(saturate(1.0 - mu * mu)), 0.0, mu);
    float distance = AtmosphereNearestIntersection(origin, direction, data.Atmosphere.RadiiDensity.y);
    float groundDistance = AtmosphereNearestIntersection(origin, direction, data.Atmosphere.RadiiDensity.x);
    if (groundDistance >= 0.0)
    {
        distance = min(distance, groundDistance);
    }
    float3 opticalDepth = 0.0;
    for (uint i = 0; i < 10; i++)
    {
        float3 position = origin + direction * (distance * (float(i) + 0.3) / 10.0);
        opticalDepth += SampleAtmosphereMedium(position).Extinction * (distance / 10.0);
    }
    TransmittanceOutput[id.xy] = float4(exp(-opticalDepth), 1.0);
}

[numthreads(8, 8, 1)]
void CS_MultiScattering(uint3 id : SV_DispatchThreadID)
{
    if (any(id.xy >= uint2(32, 32)))
    {
        return;
    }
    float2 uv = (float2(id.xy) + 0.5) / 32.0;
    float muSun = uv.x * 2.0 - 1.0;
    float radius = lerp(data.Atmosphere.RadiiDensity.x, data.Atmosphere.RadiiDensity.y, uv.y);
    float3 origin = float3(0.0, 0.0, radius);
    float3 lightDirection = float3(0.0, sqrt(saturate(1.0 - muSun * muSun)), muSun);
    // UE's default low-cost branch samples the local up/down directions using isotropic phase.
    AtmosphereIntegrationResult up = IntegrateAtmosphere(origin, float3(0.0, 0.0, 1.0), 1.e9, lightDirection, 1.0, 15.0, false, true, false, true);
    AtmosphereIntegrationResult down = IntegrateAtmosphere(origin, float3(0.0, 0.0, -1.0), 1.e9, lightDirection, 1.0, 15.0, false, true, false, true);
    float3 base = 0.5 * (up.Luminance + down.Luminance);
    float3 ratio = 0.5 * (up.MultiScatAsOne + down.MultiScatAsOne);
    float3 ratio2 = ratio * ratio;
    float3 luminance = base * (1.0 + ratio + ratio2 + ratio2 * ratio + ratio2 * ratio2);
    MultiScatteringOutput[id.xy] = float4(luminance * data.Atmosphere.GroundAlbedo.w, 0.0);
}

[numthreads(8, 8, 1)]
void CS_SkyView(uint3 id : SV_DispatchThreadID)
{
    if (any(id.xy >= uint2(192, 104)))
    {
        return;
    }
    float3 origin = data.View.CameraPositionKm.xyz;
    float3 direction = AtmosphereLocalToWorld(AtmosphereSkyViewDirection((float2(id.xy) + 0.5) / float2(192.0, 104.0), length(origin)));
    float distance = 1.e9;
    if (!AtmosphereMoveToTop(origin, direction, distance))
    {
        SkyViewOutput[id.xy] = float4(0.0, 0.0, 0.0, 1.0);
        return;
    }
    AtmosphereIntegrationResult result = IntegrateAtmosphere(origin, direction, distance, data.View.SunDirection.xyz, data.View.SunIlluminance.rgb,
        0.0, true, false, true, false);
    SkyViewOutput[id.xy] = float4(result.Luminance, dot(result.Transmittance, float3(1.0 / 3.0)));
}

[numthreads(4, 4, 4)]
void CS_AerialPerspective(uint3 id : SV_DispatchThreadID)
{
    if (any(id >= uint3(32, 32, 16)))
    {
        return;
    }
    float2 uv = (float2(id.xy) + 0.5) / 32.0;
    // Match SolarEngine's row-vector matrix convention and top-left texture UVs.
    float4 viewPosition = mul(float4(uv * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.5, 1.0), data.View.InvProjection);
    if (abs(viewPosition.w) < 1.e-9)
    {
        AerialPerspectiveOutput[id] = float4(0.0, 0.0, 0.0, 1.0);
        return;
    }
    viewPosition.xyz /= viewPosition.w;
    float3 camera = data.View.CameraPositionKm.xyz;
    float3 direction;
    if (data.View.AerialPerspective.z > 0.0)
    {
        camera += mul(float4(viewPosition.xy, 0.0, 0.0), data.View.InvView).xyz * 0.001;
        direction = normalize(mul(float4(0.0, 0.0, 1.0, 0.0), data.View.InvView).xyz);
    }
    else
    {
        direction = normalize(mul(float4(viewPosition.xyz, 0.0), data.View.InvView).xyz);
    }
    if (length(camera) < data.Atmosphere.RadiiDensity.x)
    {
        // Orthographic pixel origins can lie below the planet even with a valid view origin.
        AerialPerspectiveOutput[id] = float4(0.0, 0.0, 0.0, 1.0);
        return;
    }
    float3 origin = camera + direction * data.View.AerialPerspective.x;
    float distance = AtmosphereAerialDistance(float(id.z));
    float3 voxel = origin + direction * distance;
    float bottom = data.Atmosphere.RadiiDensity.x;
    float planetDistance = AtmosphereNearestIntersection(camera, direction, bottom);
    bool belowHorizon = planetDistance > 0.0 && length(voxel - camera) > planetDistance;
    bool underGround = length(voxel) < bottom;
    if (belowHorizon || underGround)
    {
        // UE's ground/horizon correction avoids froxel discontinuities at the curved ground.
        camera += normalize(camera) * 0.02;
        float3 voxelNormal = voxel / max(length(voxel), 1.e-6);
        float3 cameraGround = normalize(camera) * bottom;
        float3 voxelGround = voxelNormal * bottom;
        float3 groundToCamera = camera - voxelGround;
        if (belowHorizon && dot(groundToCamera / max(length(groundToCamera), 1.e-6), voxelNormal) < 0.0001)
        {
            float3 middleOnGround = normalize(0.5 * (cameraGround + voxelGround)) * bottom;
            voxel = camera + 2.0 * (middleOnGround - camera);
        }
        else if (underGround)
        {
            voxel = voxelGround;
        }
        direction = normalize(voxel - camera);
        origin = camera + direction * data.View.AerialPerspective.x;
        distance = max(0.0, length(voxel - camera) - data.View.AerialPerspective.x);
    }
    if (distance <= 0.0 || !AtmosphereMoveToTop(origin, direction, distance))
    {
        AerialPerspectiveOutput[id] = float4(0.0, 0.0, 0.0, 1.0);
        return;
    }
    AtmosphereIntegrationResult result = IntegrateAtmosphere(origin, direction, distance, data.View.SunDirection.xyz, data.View.SunIlluminance.rgb,
        max(1.0, (float(id.z) + 1.0) * 2.0), false, false, true, false);
    AerialPerspectiveOutput[id] = float4(result.Luminance, dot(result.Transmittance, float3(1.0 / 3.0)));
}
