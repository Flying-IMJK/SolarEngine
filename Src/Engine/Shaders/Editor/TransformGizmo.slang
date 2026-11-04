#include "Shaders/Common.hlsl"

struct PerGizmo
{
    float4x4 WVP;
    float4 Color;
};

ParameterBlock<PerGizmo> perGizmo;

struct GizmoInput
{
    float3 Position : POSITION;
};

struct GizmoVertex
{
    float4 Position : SV_Position;
};

GizmoVertex VS_Gizmo(GizmoInput input)
{
    GizmoVertex output;
    output.Position = mul(float4(input.Position, 1.0), perGizmo.WVP);
    return output;
}

float4 PS_Gizmo(GizmoVertex input) : SV_Target0
{
    return perGizmo.Color;
}

SHADER_PROGRAM(TransformGizmo, SHADER_VS(VS_Gizmo), SHADER_PS(PS_Gizmo))
