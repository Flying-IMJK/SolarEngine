#ifndef __COMMON__
#define __COMMON__

#include "Shaders/ProgramCommon.hlsl"

// Avoids flow control constructs.
#define UNROLL [unroll]

// Gives preference to flow control constructs.
#define LOOP [loop]

// Performs branching by using control flow instructions like jmp and label.
#define BRANCH [branch]

/// Performs branching by using the cnd instructions.
#define FLATTEN [flatten]

// Static samplers
SamplerState SamplerLinearClamp;
SamplerState SamplerPointClamp;
SamplerState SamplerLinearWrap;
SamplerState SamplerPointWrap;

SamplerComparisonState ShadowSampler;
SamplerComparisonState ShadowSamplerPCF;

#define SAMPLE_RT(rt, texCoord) rt.SampleLevel(SamplerPointClamp, texCoord, 0)
#define SAMPLE_RT_LINEAR(rt, texCoord) rt.SampleLevel(SamplerLinearClamp, texCoord, 0)


// Samples the unwrapped 3D texture (eg. volume texture of size 16x16x16 would be unwrapped to 256x16)
float4 SampleUnwrappedTexture3D(Texture2D tex, SamplerState s, float3 uvw, float size)
{
    float intW = floor(uvw.z * size - 0.5);
    float fracW = uvw.z * size - 0.5 - intW;
    float u = (uvw.x + intW) / size;
    float v = uvw.y;
    float4 rg0 = tex.Sample(s, float2(u, v));
    float4 rg1 = tex.Sample(s, float2(u + 1.0f / size, v));
    return lerp(rg0, rg1, fracW);
}

#endif
