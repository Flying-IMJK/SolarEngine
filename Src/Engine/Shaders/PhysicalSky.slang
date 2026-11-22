#include "Shaders/Common.hlsl"
#include "Shaders/GBufferCommon.hlsl"
#include "Shaders/MaterialCommon.hlsl"
#include "Shaders/QuadCommon.hlsl"
#include "Shaders/SkyAtmosphereCommon.hlsl"

SHADER_PROGRAM(PhysicalSky, SHADER_VS(VS_PhysicalSky), SHADER_PS(PS_PhysicalSky))

Texture2D<float4> SkyViewLut;

Quad_VS2PS VS_PhysicalSky(float2 position : POSITION0, float2 uv : TEXCOORD0)
{
    Quad_VS2PS output;
    // The scene clears depth to 1. Equal depth testing keeps all opaque pixels intact.
    output.Position = float4(position, 1.0, 1.0);
    output.TexCoord = uv;
    return output;
}

float3 PhysicalSkySunDisk(float3 origin, float3 direction)
{
    float angularRadius = data.View.SunDirection.w;
    if (angularRadius <= 0.0 || !any(data.View.SunIlluminance.rgb > 0.0))
        return 0.0;

    // Chord distance is stable around the sun center and avoids acos derivatives there.
    float radius = 2.0 * sin(angularRadius * 0.5);
    float separation = length(direction - data.View.SunDirection.xyz);
    float edgeWidth = max(fwidth(separation), 1.e-6);
    float coverage = 1.0 - smoothstep(radius - 0.5 * edgeWidth, radius + 0.5 * edgeWidth, separation);
    if (coverage <= 0.0 || AtmosphereNearestIntersection(origin, direction, data.Atmosphere.RadiiDensity.x) >= 0.0)
        return 0.0;

    float3 transmittance = 1.0;
    float distance = 1.e9;
    if (AtmosphereMoveToTop(origin, direction, distance))
        transmittance = AtmosphereTransmittance(origin, direction);
    float sineRadius = sin(angularRadius);
    float3 luminance = data.View.SunIlluminance.rgb / max(AtmospherePi * sineRadius * sineRadius, 1.e-12);
    // The scene color is full precision until exposure is applied in post processing.
    return luminance * transmittance * coverage;
}

GBufferOutput PS_PhysicalSky(Quad_VS2PS input)
{
    GBufferOutput output = (GBufferOutput)0;
    output.Light = float4(0.0, 0.0, 0.0, 1.0);
    output.RT1 = float4(1.0, 0.0, 0.0, SHADING_MODEL_UNLIT);

    float4 viewPosition = mul(float4(input.TexCoord * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.5, 1.0), data.View.InvProjection);
    if (abs(viewPosition.w) < 1.e-9)
    {
        return output;
    }
    viewPosition.xyz /= viewPosition.w;
    float3 origin = data.View.CameraPositionKm.xyz;
    float3 direction;
    bool orthographic = data.View.AerialPerspective.z > 0.0;
    if (orthographic)
    {
        origin += mul(float4(viewPosition.xy, 0.0, 0.0), data.View.InvView).xyz * 0.001;
        direction = normalize(mul(float4(0.0, 0.0, 1.0, 0.0), data.View.InvView).xyz);
    }
    else
    {
        direction = normalize(mul(float4(viewPosition.xyz, 0.0), data.View.InvView).xyz);
    }
    float height = length(origin);
    if (height < data.Atmosphere.RadiiDensity.x)
        return output;

    float3 luminance;
    if (!orthographic && height < data.Atmosphere.RadiiDensity.y)
    {
        float2 uv = AtmosphereSkyViewUv(AtmosphereWorldToLocal(direction), height);
        luminance = SkyViewLut.SampleLevel(AtmosphereSampler, uv, 0).rgb;
    }
    else
    {
        float3 rayStart = origin;
        float distance = 1.e9;
        luminance = 0.0;
        if (AtmosphereMoveToTop(rayStart, direction, distance))
        {
            AtmosphereIntegrationResult result = IntegrateAtmosphere(rayStart, direction, distance,
                data.View.SunDirection.xyz, data.View.SunIlluminance.rgb, 0.0, true, false, true, false);
            luminance = result.Luminance;
        }
    }
    output.Light.rgb = luminance + PhysicalSkySunDisk(origin, direction);
    return output;
}
