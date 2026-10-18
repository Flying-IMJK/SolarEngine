#include "Shaders/Slang/SolarShaderProgram.slang"

SHADER_PROGRAM(CompileOnlyCounterAppend,
	SHADER_CS(AppendCS)
)

SHADER_PROGRAM(CompileOnlyCounterConsume,
	SHADER_CS(ConsumeCS)
)

AppendStructuredBuffer<float4> AppendValues;
ConsumeStructuredBuffer<float4> ConsumeValues;
RWStructuredBuffer<uint> ConsumeResult;

[numthreads(1, 1, 1)]
void AppendCS(uint3 dispatchThreadID : SV_DispatchThreadID)
{
	float value = float(dispatchThreadID.x + 1);
	AppendValues.Append(float4(value, value, value, value));
}

[numthreads(1, 1, 1)]
void ConsumeCS(uint3 dispatchThreadID : SV_DispatchThreadID)
{
	ConsumeResult[dispatchThreadID.x] = uint(ConsumeValues.Consume().x);
}
