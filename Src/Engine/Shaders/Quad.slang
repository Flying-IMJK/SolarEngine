
#include "Shaders/Common.hlsl"
#include "Shaders/QuadCommon.hlsl"

// Interpolants passed from the vertex shader to the PostFx material pixel shader
struct MaterialVertexOutput
{
	float4 Position      : SV_Position;
	float3 WorldPosition : TEXCOORD0;
	float2 TexCoord      : TEXCOORD1;
};

uniform float4 Color;


// Vertex Shader function for postFx materials rendering
MaterialVertexOutput VS_PostFx(float2 Position : POSITION0, float2 TexCoord : TEXCOORD0)
{
	MaterialVertexOutput output;

	output.Position = float4(Position, 0, 1);
	output.WorldPosition = output.Position.xyz;
	output.TexCoord = TexCoord;

	return output;
}

//#ifdef _PS_CopyLinear

Texture2D Source;

SHADER_PROGRAM(
	CopyLinear,
	SHADER_VS(VS_Quad),
	SHADER_PS(PS_CopyLinear)
)

SHADER_PROGRAM(
	Clear,
	SHADER_VS(VS_Quad),
	SHADER_PS(PS_Clear)
)

// Pixel Shader for screen space quad rendering for image copy (linear sampling)
float4 PS_CopyLinear(Quad_VS2PS input) : SV_Target
{
	return Source.SampleLevel(SamplerLinearClamp, input.TexCoord, 0);
}

//#endif

//#ifdef _PS_Clear

// Pixel Shader for clearing a render target with a solid color
float4 PS_Clear(Quad_VS2PS input) : SV_Target
{
	return Color;
}

//#endif
