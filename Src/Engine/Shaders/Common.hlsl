#ifndef __COMMON__
#define __COMMON__

// Compiler attributes

#if DIRECTX || VULKAN

// Avoids flow control constructs.
#define UNROLL [unroll]

// Gives preference to flow control constructs.
#define LOOP [loop]

// Performs branching by using control flow instructions like jmp and label.
#define BRANCH [branch]

/// Performs branching by using the cnd instructions.
#define FLATTEN [flatten]

#endif

// Compiler attribute fallback

#ifndef UNROLL
#define UNROLL
#endif

#ifndef LOOP
#define LOOP
#endif

#ifndef BRANCH
#define BRANCH
#endif

#ifndef FLATTEN
#define FLATTEN
#endif


#define META_VS(isVisible, minFeatureLevel)
#define META_VS_IN_ELEMENT(type, index, format, slot, offset, slotClass, stepRate, isVisible)
#define META_HS(isVisible, minFeatureLevel)
#define META_HS_PATCH(inControlPoints)
#define META_DS(isVisible, minFeatureLevel)
#define META_GS(isVisible, minFeatureLevel)
#define META_PS(isVisible, minFeatureLevel)
#define META_CS(isVisible, minFeatureLevel)
#define META_FLAG(flag)
#define META_PERMUTATION_1(param0)
#define META_PERMUTATION_2(param0, param1)
#define META_PERMUTATION_3(param0, param1, param2)
#define META_PERMUTATION_4(param0, param1, param2, param3)
#define META_CB_BEGIN(index, name) cbuffer name : register(b##index) {
#define META_CB_END };

// Static samplers
SamplerState SamplerLinearClamp : register(s0);
SamplerState SamplerPointClamp : register(s1);
SamplerState SamplerLinearWrap : register(s2);
SamplerState SamplerPointWrap : register(s3);

SamplerComparisonState ShadowSampler : register(s4);
SamplerComparisonState ShadowSamplerPCF : register(s5);

#define SAMPLE_RT(rt, texCoord) rt.SampleLevel(SamplerPointClamp, texCoord, 0)
#define SAMPLE_RT_LINEAR(rt, texCoord) rt.SampleLevel(SamplerLinearClamp, texCoord, 0)


#define SHADING_MODEL_UNLIT 0
#define SHADING_MODEL_LIT 1
#define SHADING_MODEL_SUBSURFACE 2
#define SHADING_MODEL_FOLIAGE 3


// Samples the unwrapped 3D texture (eg. volume texture of size 16x16x16 would be unwrapped to 256x16)
float4 SampleUnwrappedTexture3D(Texture2D tex, SamplerState s, float3 uvw, float size)
{
    float intW = floor(uvw.z * size - 0.5);
    half fracW = uvw.z * size - 0.5 - intW;
    float u = (uvw.x + intW) / size;
    float v = uvw.y;
    float4 rg0 = tex.Sample(s, float2(u, v));
    float4 rg1 = tex.Sample(s, float2(u + 1.0f / size, v));
    return lerp(rg0, rg1, fracW);
}

#endif