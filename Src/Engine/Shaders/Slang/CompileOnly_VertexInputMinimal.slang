#include "Shaders/Slang/SolarShaderProgram.slang"

SHADER_PROGRAM(CompileOnlyVertexInputMinimal,
	SHADER_VS(PositionVS),
	SHADER_PS(MinimalPS)
)

SHADER_PROGRAM(CompileOnlyVertexSystemOnly,
	SHADER_VS(SystemOnlyVS),
	SHADER_PS(MinimalPS)
)

SHADER_PROGRAM(CompileOnlyVertexInputInstanced,
	SHADER_VS(InstancedPositionVS),
	SHADER_PS(MinimalPS)
)

uniform float4 ExternalScale;

struct VertexInput
{
	float3 position : POSITION;
};

struct VertexOutput
{
	float4 position : SV_Position;
};

struct InstancedVertexInput
{
	float3 position : POSITION;
	float4 instanceOrigin : ATTRIBUTE0;
	float4 instanceTransform1 : ATTRIBUTE1;
	float3 instanceTransform2 : ATTRIBUTE2;
	float3 instanceTransform3 : ATTRIBUTE3;
	float4 instanceLightmapArea : ATTRIBUTE4;
};

VertexOutput PositionVS(VertexInput input)
{
	VertexOutput output;
	output.position = float4(input.position * ExternalScale.xyz, 1.0);
	return output;
}

VertexOutput SystemOnlyVS(uint vertexId : SV_VertexID)
{
	VertexOutput output;
	const float x = vertexId == 0 ? 0.0 : 1.0;
	output.position = float4(x * ExternalScale.x, 0.0, 0.0, 1.0);
	return output;
}

VertexOutput InstancedPositionVS(InstancedVertexInput input)
{
	VertexOutput output;
	// 使用全部实例输入，保证反射签名完整覆盖 StaticMesh 的实例流契约。
	float3 position = input.position + input.instanceOrigin.xyz;
	position += input.instanceTransform1.xyz * 0.000001;
	position += input.instanceTransform2 * 0.000001;
	position += input.instanceTransform3 * 0.000001;
	position.xy += input.instanceLightmapArea.xy * 0.000001;
	output.position = float4(position * ExternalScale.xyz, 1.0);
	return output;
}

float4 MinimalPS(VertexOutput input) : SV_Target
{
	return float4(1.0, 1.0, 1.0, 1.0);
}
