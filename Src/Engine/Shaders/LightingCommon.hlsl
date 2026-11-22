
#ifndef __LIGHTING_COMMON__
#define __LIGHTING_COMMON__

#include "Shaders/BRDF.hlsl"
#include "Shaders/GBufferCommon.hlsl"

// Disables directional lighting (no shadowing with dot(N, L), eg. for smoke particles)
#ifndef LIGHTING_NO_DIRECTIONAL
#define LIGHTING_NO_DIRECTIONAL 0
#endif

// Disables specular lighting (diffuse-only)
#ifndef LIGHTING_NO_SPECULAR
#define LIGHTING_NO_SPECULAR 0
#endif

// Structure that contains information about light
struct LightData
{
    float2 SpotAngles;
    float SourceRadius;
    float SourceLength;

    float3 Color;
    float MinRoughness;

    float3 Position;
    float CastShadows;

    float3 Direction;
    float Radius;

    float RadiusInv;
    float Padding0;
    float Padding1;
    float Padding2;
};

// Structure that contains information about shadow
struct ShadowData
{
    float SurfaceShadow;
    float TransmissionShadow;
};

// Structure that contains information about direct lighting calculations result
struct LightingData
{
    float3 Diffuse;
    float3 Specular;
    float3 Transmission;
};

// Calculates radial light (point or spot) attenuation factors (distance, spot and radius mask)
void GetRadialLightAttenuation(
    LightData lightData,
    bool isSpotLight,
    float3 N,
    float distanceSqr,
    float3 toLight,
    float3 L,
    inout float NoL,
    inout float attenuation)
{
    // Distances are meters. A one-centimeter lower bound avoids the point singularity.
    float factor = distanceSqr * Square(lightData.RadiusInv);
    float window = saturate(1.0 - factor * factor);
    attenuation = window * window / max(distanceSqr, 0.0001);
    NoL = saturate(dot(N, L));

    // Spot mask attenuation
    if (isSpotLight)
    {
        float cosOuterCone = lightData.SpotAngles.x;
        float invCosConeDifference = lightData.SpotAngles.y;
        attenuation *= Square(saturate((dot(normalize(-L), lightData.Direction) - cosOuterCone) * invCosConeDifference));
    }
}

// Find representative incoming light direction and energy modification
float AreaLightSpecular(LightData lightData, float roughness, inout float3 toLight, inout float3 L, float3 V, half3 N)
{
    float energy = 1;
    float m = roughness * roughness;
    float3 r = reflect(-V, N);
    float invDistToLight = rsqrt(max(dot(toLight, toLight), 0.0001));

    BRANCH
    if (lightData.SourceLength > 0)
    {
        float lineAngle = saturate(lightData.SourceLength * invDistToLight);
        energy *= m / saturate(m + 0.5 * lineAngle);
        float3 l01 = lightData.Direction * lightData.SourceLength;
        float3 l0 = toLight - 0.5 * l01;
        float a = Square(lightData.SourceLength);
        float b = dot(r, l01);
        float t = saturate(dot(l0, b * r - l01) / max(a - b * b, 1e-6));
        toLight = l0 + t * l01;
    }

    BRANCH
    if (lightData.SourceRadius > 0)
    {
        float sphereAngle = saturate(lightData.SourceRadius * invDistToLight);
        energy *= Square(m / saturate(m + 0.5 * sphereAngle));
        float3 closestPointOnRay = dot(toLight, r) * r;
        float3 centerToRay = closestPointOnRay - toLight;
        float3 closestPointOnSphere = toLight + centerToRay * saturate(lightData.SourceRadius * rsqrt(max(dot(centerToRay, centerToRay), 1e-8)));
        toLight = closestPointOnSphere;
    }

    if (dot(toLight, toLight) > 1e-8)
        L = normalize(toLight);
    return energy;
}

#endif
