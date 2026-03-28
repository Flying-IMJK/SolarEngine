
#include "./Shaders/GUICommon.hlsl"
#include "./Shaders/QuadCommon.hlsl"

struct VertexInput
{
    float2 pos		: POSITION;
    float2 uv		: TEXCOORD0;
    float4 col		: COLOR0;
};

struct VertexOutput
{
    float4 pos : SV_POSITION;
    float2 uv  : TEXCOORD0;
    float4 col : TEXCOORD1;
};


META_CB_BEGIN(0, Data)
float2 scale;
float2 offset;
META_CB_END

META_VS(true, FEATURE_LEVEL_ES2)
META_VS_IN_ELEMENT(POSITION,  0, R32G32_FLOAT,   0, ALIGN, PER_VERTEX, 0, true)
META_VS_IN_ELEMENT(TEXCOORD,  0, R32G32_FLOAT,   0, ALIGN, PER_VERTEX, 0, true)
META_VS_IN_ELEMENT(COLOR,     0, R8G8B8A8_UNorm, 0, ALIGN, PER_VERTEX, 0, true)
VertexOutput VS(VertexInput input)
{
    VertexOutput output;
    output.pos = float4(input.pos.xy * scale + offset, 0.5f, 1.f);
    output.uv = input.uv;
    output.col = input.col;
    return output;
}

Texture2D texture0 : register(t0);
SamplerState sampler0 : register(s0);

META_PS(true, FEATURE_LEVEL_ES2)
float4 PS(VertexOutput input) : SV_Target0
{
    return input.col * texture0.Sample(sampler0, input.uv);
}