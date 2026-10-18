#include "Shaders/Slang/SolarShaderProgram.slang"

SHADER_PROGRAM(GBuffer,
    SHADER_VS(MaterialDrawVS),
    SHADER_PS(MaterialDrawPS),
    SHADER_MACRO(MODE_BASE, MODE_DETAIL)
)

struct LocalParameters
{
    float4 Offset;
};

struct MaterialParameters
{
    // Each variant declares only the fields it can bind and consume.
#if defined(MODE_DETAIL)
    float4 Detail;
#else
    float4 Tint;
#endif
    ConstantBuffer<LocalParameters> Local;
#if defined(MODE_DETAIL)
    Texture2D Image;
    SamplerState ImageSampler;
    StructuredBuffer<float4> Lookup;
#endif
};

ParameterBlock<MaterialParameters> Material;

struct VertexInput
{
    float3 Position : POSITION;
};

struct VertexOutput
{
    float4 Position : SV_Position;
};

VertexOutput MaterialDrawVS(VertexInput input)
{
    VertexOutput output;
    output.Position = float4(input.Position, 1.0);
    return output;
}

float4 MaterialDrawPS(VertexOutput input) : SV_Target
{
#if defined(MODE_DETAIL)
    return Material.Detail + Material.Local.Offset + Material.Lookup[0] +
        Material.Image.SampleLevel(Material.ImageSampler, float2(0.5, 0.5), 0.0);
#else
    return Material.Tint + Material.Local.Offset;
#endif
}
