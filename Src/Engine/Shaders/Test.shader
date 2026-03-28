

#define META_POSITION POSITION
#define META_COLOR(id) COLOR##id
#define META_NORMAL NORMAL
#define META_TEXCOORD(id) TEXCOORD##id
#define META_SV_POSITION(id) SV_POSITION

// Meta macros used by shaders parser

#define META_PASS_VS
#define META_PASS_PS

#define META_PASS(state, name)
#define META_PASS_BEGIN(name)
#define META_PASS_End()


#define META_CB_BEGIN(name) cbuffer name {
#define META_CB_END };


META_PASS_BEGIN(Test)
META_PASS(META_PASS_VS, VS1)
META_PASS(META_PASS_PS, PS2)
META_PASS_End()


struct VertexInput
{
   float2 pos		: POSITION;
   float2 uv       : TEXCOORD0;
   float4 col		: COLOR0;
};


struct VertexOutput
{
   float4 pos : SV_POSITION;
   float2 uv : TEXCOORD0;
   float4 col : COLOR0;
};

META_CB_BEGIN(Data)
float4x4 ProjectionMatrix;
META_CB_END

struct TestData
{
   float4 test1;
   float4 test2;
};

ParameterBlock<TestData> testData;

float4 test3;

[Shader("vertex")]
VertexOutput VS1(VertexInput input)
{
   VertexOutput output;
   output.pos = float4(input.pos.xy, 0.f, 1.f);
   output.col = input.col;
   output.uv = input.uv;
   
   return output;
}

[Shader("vertex")]
VertexOutput VS2(VertexInput input)
{
   VertexOutput output;
   output.pos = float4(input.pos.xy, 0.f, 1.f);
   output.col = input.col;
   output.uv = input.uv;
   return output;
}

Texture2D texture0 : register(t0);
SamplerState sampler0 : register(s0);


[Shader("fragment")]
float4 PS1(VertexOutput input) : SV_Target0
{
   return texture0.Sample(sampler0, input.uv);
}

[Shader("fragment")]
float4 PS2(VertexOutput input) : SV_Target0
{
   return texture0.Sample(sampler0, input.uv);
}