#include "Shaders/Common.hlsl"
#include "Shaders/MaterialCommon.hlsl"
#include "Shaders/GBuffer.hlsl"

struct SkyData
{
    float4x4 WVP;
    float3 ViewOffset;
    float Padding;
    GBufferData GBuffer;
};

ConstantBuffer<SkyData> data;
TextureCube SkyBoxTex;

struct SkyInput
{
    float4 Position : SV_Position;
    float4 ScreenPos : TEXCOORD0;
};

SHADER_PROGRAM(Sky, SHADER_VS(VS), SHADER_PS(PS_Sky))
SHADER_PROGRAM(SkyBox, SHADER_VS(VS), SHADER_PS(PS_SkyBox))

SkyInput VS(ModelInput_PosOnly input)
{
    SkyInput output;
    output.Position = mul(float4(input.Position, 1), data.WVP);
    output.ScreenPos = output.Position;
    output.Position = output.Position.xyzz;
    return output;
}

GBufferOutput PackSky(float4 color)
{
    GBufferOutput output = (GBufferOutput)0;
    output.Light = color;
    output.RT1 = float4(1, 0, 0, SHADING_MODEL_UNLIT);
    return output;
}

GBufferOutput PS_Sky(SkyInput input)
{
    return PackSky(1.0);
}

GBufferOutput PS_SkyBox(SkyInput input)
{
    float2 uv = (input.ScreenPos.xy / input.ScreenPos.w) * float2(0.5, -0.5) + 0.5;
    float3 position = GetViewPos(data.GBuffer, uv, LinearZ2DeviceDepth(data.GBuffer, 1));
    float3 world = mul(float4(position, 1), data.GBuffer.InvViewMatrix).xyz;
    return PackSky(SkyBoxTex.SampleLevel(SamplerLinearClamp, world - data.GBuffer.ViewPos, 0));
}
