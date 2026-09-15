#include "Shaders/Slang/SolarShaderProgram.slang"

SHADER_PROGRAM(CompileOnlySparseVertexInput,
	SHADER_VS(SparsePositionVS),
	SHADER_PS(SparseMinimalPS)
)

struct SparseVertexInput
{
	float3 position : POSITION;
	float2 texcoord : TEXCOORD0;
};

struct SparseVertexOutput
{
	float4 position : SV_Position;
};

SparseVertexOutput SparsePositionVS(SparseVertexInput input)
{
	SparseVertexOutput output;
	output.position = float4(input.position + float3(input.texcoord, 0.0), 1.0);
	return output;
}

float4 SparseMinimalPS(SparseVertexOutput input) : SV_Target
{
	return float4(1.0, 1.0, 1.0, 1.0);
}
