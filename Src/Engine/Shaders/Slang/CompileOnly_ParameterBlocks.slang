#include "Shaders/Slang/SolarShaderProgram.slang"

SHADER_PROGRAM(ParameterBlockReflection,
	SHADER_VS(ParameterBlockVS),
	SHADER_PS(ParameterBlockPS)
)

struct DeepParameters
{
	float4 Tint;
};

struct NestedParameters
{
	float4 PixelValue;
	DeepParameters Deep;
	Texture2D PixelTextures[2];
	SamplerState LinearSampler;
	Buffer<float4> TypedValues;
	RWBuffer<float4> RWTypedValues;
	StructuredBuffer<float4> StructuredValues;
	RWStructuredBuffer<float4> RWStructuredValues;
	AppendStructuredBuffer<float4> AppendValues;
	ByteAddressBuffer RawValues;
	RWByteAddressBuffer RWRawValues;
};

struct ConstantParameters
{
	float4 VertexOffset;
	Texture2D VertexTexture;
};

struct OuterParameters
{
	float4 VertexScale;
	ParameterBlock<NestedParameters> Nested;
	ConstantBuffer<ConstantParameters> Constants;
};

struct UnusedParameters
{
	float4 Value;
	Texture2D Texture;
};

ParameterBlock<OuterParameters> Outer;
ParameterBlock<UnusedParameters> Unused;
uniform float RootScale;

struct VertexInput
{
	float3 Position : POSITION;
};

struct VertexOutput
{
	float4 Position : SV_Position;
};

VertexOutput ParameterBlockVS(VertexInput input)
{
	VertexOutput output;
	float3 textureOffset = Outer.Constants.VertexTexture.Load(int3(0, 0, 0)).xyz;
	output.Position = float4(input.Position * Outer.VertexScale.xyz * RootScale + Outer.Constants.VertexOffset.xyz + textureOffset, 1.0);
	return output;
}

float4 ParameterBlockPS(VertexOutput input) : SV_Target
{
	Outer.Nested.AppendValues.Append(Outer.Nested.PixelValue);
	return (Outer.Nested.PixelValue + Outer.Nested.Deep.Tint) *
		Outer.Nested.PixelTextures[1].Sample(Outer.Nested.LinearSampler, float2(0.0, 0.0));
}
