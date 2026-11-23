
#ifndef __GAMMA_CORRECTION_COMMON__
#define __GAMMA_CORRECTION_COMMON__

#include "Shaders/Math.hlsl"

// Fast reversible tonemapper
// http://gpuopen.com/optimized-reversible-tonemapper-for-resolve/
float3 FastTonemap(float3 c)
{
    return c * rcp(max(max(c.r, c.g), c.b) + 1.0);
}

float4 FastTonemap(float4 c)
{
    return float4(FastTonemap(c.rgb), c.a);
}

float3 FastTonemap(float3 c, float w)
{
    return c * (w * rcp(max(max(c.r, c.g), c.b) + 1.0));
}

float4 FastTonemap(float4 c, float w)
{
    return float4(FastTonemap(c.rgb, w), c.a);
}

float3 FastTonemapInvert(float3 c)
{
    return c * rcp(1.0 - max(max(c.r, c.g), c.b));
}

float4 FastTonemapInvert(float4 c)
{
    return float4(FastTonemapInvert(c.rgb), c.a);
}

float LinearToSrgbChannel(float linearColor)
{
    if (linearColor <= 0.0031308)
        return linearColor * 12.92;
    return pow(linearColor, (1.0 / 2.4)) * 1.055 - 0.055;
}

float3 LinearToSrgb(float3 linearColor)
{
    return float3(
        LinearToSrgbChannel(linearColor.r),
        LinearToSrgbChannel(linearColor.g),
        LinearToSrgbChannel(linearColor.b));
}

float SrgbToLinearChannel(float color)
{
    if (color <= 0.04045)
        return color / 12.92;
    return pow((color + 0.055) / 1.055, 2.4);
}

float3 sRGBToLinear(float3 color)
{
    return float3(
        SrgbToLinearChannel(color.r),
        SrgbToLinearChannel(color.g),
        SrgbToLinearChannel(color.b));
}

float3 LogToLinear(float3 logColor)
{
    const float linearRange = 14.0f;
    const float linearGrey = 0.18f;
    const float exposureGrey = 444.0f;
    return exp2((logColor - exposureGrey / 1023.0) * linearRange) * linearGrey;
}

float3 LinearToLog(float3 linearColor)
{
    const float linearRange = 14.0f;
    const float linearGrey = 0.18f;
    const float exposureGrey = 444.0f;
    return saturate(log2(linearColor) / linearRange - log2(linearGrey) / linearRange + exposureGrey / 1023.0f);
}

#endif
