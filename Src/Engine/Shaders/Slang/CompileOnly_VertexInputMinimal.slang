#include "Shaders/Slang/SolarShaderProgram.slang"

SHADER_PROGRAM(CompileOnlyVertexInputMinimal,
	SHADER_VS(PositionVS),
	SHADER_PS(MinimalPS)
)

SHADER_PROGRAM(CompileOnlyVertexSystemOnly,
	SHADER_VS(SystemOnlyVS),
	SHADER_PS(MinimalPS)
)

struct VertexInput
{
	float3 position : POSITION;
};

struct VertexOutput
{
	float4 position : SV_Position;
};

VertexOutput PositionVS(VertexInput input)
{
	VertexOutput output;
	output.position = float4(input.position, 1.0);
	return output;
}

VertexOutput SystemOnlyVS(uint vertexId : SV_VertexID)
{
	VertexOutput output;
	const float x = vertexId == 0 ? 0.0 : 1.0;
	output.position = float4(x, 0.0, 0.0, 1.0);
	return output;
}

float4 MinimalPS(VertexOutput input) : SV_Target
{
	return float4(1.0, 1.0, 1.0, 1.0);
}
