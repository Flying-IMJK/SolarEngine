#include "Shaders/Slang/SolarShaderProgram.slang"

SHADER_PROGRAM(CompileOnlyMinimalCS,
	SHADER_CS(MainCS)
)

[numthreads(1, 1, 1)]
void MainCS(uint3 dispatchThreadID : SV_DispatchThreadID)
{
}
