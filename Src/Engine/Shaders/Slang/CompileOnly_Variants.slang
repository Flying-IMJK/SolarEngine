
#include "Shaders/Slang/SolarShaderProgram.slang"

SHADER_PROGRAM(CompileOnlyVariants,
	SHADER_VS(MainVS),
	SHADER_PS(MainPS),
	SHADER_MACRO(MATERIAL_LIT, MATERIAL_UNLIT),
	SHADER_MACRO(_, FOG_ON)
)

uniform float4 textVale;

Texture2D tex;
Texture2D tex1;
SamplerState texSampler;

struct VertexInput
{
	float3 position : POSITION;
};

struct VertexOutput
{
	float4 position : SV_Position;
};

VertexOutput MainVS(VertexInput input)
{
	VertexOutput output;
	output.position = float4(input.position, 1.0);
	return output;
}

float4 MainPS(VertexOutput input) : SV_Target
{
    float4 color = tex.Sample(texSampler, float2(0.5, 0.5));
#if defined(MATERIAL_UNLIT)
    return float4(color.x, 0.0, 1.0, 1.0);
#elif defined(FOG_ON)
    return float4(0.5, textVale.y, 0.5, 1.0);
#else
    return textVale;
#endif
}
