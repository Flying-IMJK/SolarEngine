#include "Shaders/ProgramCommon.hlsl"
#include "Shaders/QuadCommon.hlsl"

SHADER_PROGRAM(HeightFog, SHADER_VS(VS_Quad), SHADER_PS(PS_HeightFog))
SHADER_PROGRAM(HeightFogAtmosphere, SHADER_VS(VS_Quad), SHADER_PS(PS_HeightFogAtmosphere))

struct HeightFogParameters
{
    float4x4 InvProjection;
    float4x4 InvView;
    // XYZ: camera relative to the rendering origin; W: height relative to the fog actor.
    float4 CameraPosition;
    // Density and falloff in meters, then layer height relative to the actor.
    float4 FogLayer0;
    float4 FogLayer1;
    float4 FogColorOpacity;
    // Start distance, cutoff distance, directional start distance, orthographic flag.
    float4 Distances;
    float4 LightDirection;
    // Artist directional color weighted by light luminance; W: atmosphere contribution.
    float4 DirectionalColor;
    float4 AtmosphereAmbientScale;
};

ConstantBuffer<HeightFogParameters> fogData;
Texture2D<float> SceneDepth;
// Two texels: uniform-phase distant sky luminance, then sunlight after ground transmittance.
Texture2D<float4> AtmosphereFogLighting;

// UE's analytic integral, expressed in log space to avoid 0 * infinity on long downward rays.
// Returns log2 of the integral per meter; zero-density layers contribute no optical depth.
float HeightFogLayerLogIntegral(float3 layer, float originHeight, float heightDelta)
{
    if (layer.x <= 0.0)
        return -3.402823e38;

    float falloff = layer.y * heightDelta;
    float exponent = -layer.y * (originHeight - layer.z);
    float logIntegral;
    if (abs(falloff) < 0.01)
    {
        const float ln2 = 0.69314718056;
        logIntegral = exponent + log2(ln2 - 0.5 * ln2 * ln2 * falloff);
    }
    else
    {
        float magnitude = abs(falloff);
        logIntegral = max(exponent, exponent - falloff) + log2((1.0 - exp2(-magnitude)) / magnitude);
    }
    return log2(layer.x) + logIntegral;
}

float4 EvaluateHeightFog(Quad_VS2PS input, float3 ambientColor, float3 directionalColor)
{
    float depth = SceneDepth.Load(int3(int2(input.Position.xy), 0));
    float4 viewPosition = mul(float4(input.TexCoord * float2(2.0, -2.0) + float2(-1.0, 1.0), depth, 1.0), fogData.InvProjection);
    if (abs(viewPosition.w) < 1.e-9)
        return float4(0.0, 0.0, 0.0, 1.0);
    viewPosition.xyz /= viewPosition.w;

    float3 receiver = mul(float4(viewPosition.xyz, 1.0), fogData.InvView).xyz;
    float3 cameraToReceiver = receiver - fogData.CameraPosition.xyz;
    float rayLength = length(cameraToReceiver);
    float originHeight = fogData.CameraPosition.w;
    float3 direction;
    if (fogData.Distances.w > 0.0)
    {
        float3 offset = mul(float4(viewPosition.xy, 0.0, 0.0), fogData.InvView).xyz;
        originHeight += offset.y;
        direction = normalize(mul(float4(0.0, 0.0, 1.0, 0.0), fogData.InvView).xyz);
        rayLength = max(viewPosition.z, 0.0);
    }
    else
    {
        direction = cameraToReceiver / max(rayLength, 1.e-8);
    }
    if (rayLength <= fogData.Distances.x || (fogData.Distances.y > 0.0 && rayLength > fogData.Distances.y))
        return float4(0.0, 0.0, 0.0, 1.0);

    originHeight += direction.y * fogData.Distances.x;
    rayLength -= fogData.Distances.x;
    float heightDelta = direction.y * rayLength;
    float2 logIntegral = float2(HeightFogLayerLogIntegral(fogData.FogLayer0.xyz, originHeight, heightDelta),
                               HeightFogLayerLogIntegral(fogData.FogLayer1.xyz, originHeight, heightDelta));
    // Optical depths beyond 2^32 are already opaque; clamp before exponentiation to keep finite values.
    float2 opticalDepth = exp2(min(logIntegral + log2(rayLength), 32.0));
    float transmittance = max(exp2(-opticalDepth.x - opticalDepth.y), 1.0 - fogData.FogColorOpacity.w);
    float3 scattering = ambientColor * (1.0 - transmittance);

    float directionalLength = max(rayLength - fogData.Distances.z, 0.0);
    if (directionalLength > 0.0)
    {
        float2 directionalDepth = exp2(min(logIntegral + log2(directionalLength), 32.0));
        float phase = pow(saturate(dot(direction, fogData.LightDirection.xyz)), fogData.LightDirection.w) / (4.0 * 3.14159265358979323846);
        scattering += directionalColor * phase * (1.0 - exp2(-directionalDepth.x - directionalDepth.y));
    }
    return float4(scattering, transmittance);
}

float4 PS_HeightFog(Quad_VS2PS input) : SV_Target0
{
    return EvaluateHeightFog(input, fogData.FogColorOpacity.rgb, fogData.DirectionalColor.rgb);
}

float4 PS_HeightFogAtmosphere(Quad_VS2PS input) : SV_Target0
{
    float3 sky = AtmosphereFogLighting.Load(int3(0, 0, 0)).rgb;
    float3 sun = AtmosphereFogLighting.Load(int3(1, 0, 0)).rgb;
    return EvaluateHeightFog(input,
        fogData.FogColorOpacity.rgb + sky * fogData.DirectionalColor.w * fogData.AtmosphereAmbientScale.rgb,
        fogData.DirectionalColor.rgb + sun * fogData.DirectionalColor.w);
}
