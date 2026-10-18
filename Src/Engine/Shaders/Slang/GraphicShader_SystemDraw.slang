#include "Shaders/Slang/SolarShaderProgram.slang"

SHADER_PROGRAM(GBuffer,
    SHADER_VS(SystemDrawVS),
    SHADER_PS(SystemDrawPS),
    SHADER_MACRO(MODE_SYSTEM, MODE_UNKNOWN)
)

struct MaterialParameters { float4 Tint; };
struct ViewSharedParameters { float4 Scale; };
struct ViewParameters
{
    float4 Ambient;
    ConstantBuffer<ViewSharedParameters> Shared;
};
struct DrawParameters { float4 Offset; };
struct MaterialSharedParameters { float4 Weight; };
struct PerMaterialParameters { ConstantBuffer<MaterialSharedParameters> Shared; };
struct SceneTextureParameters
{
    Texture2D Image;
    SamplerState ImageSampler;
};

ParameterBlock<MaterialParameters> Material;
ParameterBlock<ViewParameters> PerView;
ParameterBlock<DrawParameters> PerDraw;
ParameterBlock<PerMaterialParameters> PerMaterial;
ParameterBlock<SceneTextureParameters> SceneTexture;
#if defined(MODE_UNKNOWN)
// Keep the scene block active to exercise physical descriptor sets beyond the graphics stage count.
struct UnknownParameters { float4 Value; };
ParameterBlock<UnknownParameters> Unknown;
#endif

struct VertexInput { float3 Position : POSITION; };
struct VertexOutput { float4 Position : SV_Position; };

VertexOutput SystemDrawVS(VertexInput input)
{
    VertexOutput output;
    output.Position = float4(input.Position, 1.0);
    return output;
}

float4 SystemDrawPS(VertexOutput input) : SV_Target
{
    float4 color = Material.Tint + PerView.Ambient + PerView.Shared.Scale +
        PerDraw.Offset + PerMaterial.Shared.Weight;
#if defined(MODE_UNKNOWN)
    color += Unknown.Value;
#endif
    color += SceneTexture.Image.SampleLevel(SceneTexture.ImageSampler, float2(0.5, 0.5), 0.0);
    return color;
}
