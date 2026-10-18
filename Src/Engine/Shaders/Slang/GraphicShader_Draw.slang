#include "Shaders/Slang/SolarShaderProgram.slang"

SHADER_PROGRAM(GBuffer,
    SHADER_VS(GraphicShaderVS),
    SHADER_PS(GraphicShaderPS)
)

SHADER_PROGRAM(Forward,
    SHADER_VS(GraphicShaderVS),
    SHADER_PS(GraphicShaderPS)
)

struct GraphicShaderInput
{
    float3 position : POSITION;
};

struct GraphicShaderOutput
{
    float4 position : SV_Position;
};

GraphicShaderOutput GraphicShaderVS(GraphicShaderInput input)
{
    GraphicShaderOutput output;
    output.position = float4(input.position, 1.0);
    return output;
}

float4 GraphicShaderPS(GraphicShaderOutput input) : SV_Target
{
    return float4(0.0, 1.0, 0.0, 1.0);
}
