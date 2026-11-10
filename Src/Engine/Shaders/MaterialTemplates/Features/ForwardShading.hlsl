// Forward material lighting uses the same LightData and BRDF as the scene lighting passes.
@0// Defines
#define MAX_LOCAL_LIGHTS 4
@1// Includes
#include "Shaders/Lighting.hlsl"
@2// Constants
LightData DirectionalLight;
uint LocalLightsCount;
LightData LocalLights[MAX_LOCAL_LIGHTS];
@3// Resources
@4// Utilities
@5// Shaders
void PS_Forward(in PixelInput input, out float4 output : SV_Target0)
{
#if USE_DITHERED_LOD_TRANSITION
    ClipLODTransition(input);
#endif
    MaterialInput materialInput = GetMaterialInput(input);
    Material material = GetMaterialPS(materialInput);
#if MATERIAL_MASKED
    clip(material.Mask - MATERIAL_MASK_THRESHOLD);
#endif
    output = float4(material.Emissive, material.Opacity);
#if MATERIAL_SHADING_MODEL != SHADING_MODEL_UNLIT
    GBufferSample gBuffer = (GBufferSample)0;
    gBuffer.Normal = material.WorldNormal;
    gBuffer.Roughness = material.Roughness;
    gBuffer.Metalness = material.Metalness;
    gBuffer.Color = material.Color;
    gBuffer.Specular = material.Specular;
    gBuffer.AO = material.AO;
    gBuffer.ViewPos = mul(float4(materialInput.WorldPosition, 1), ViewMatrix).xyz;
    gBuffer.WorldPos = materialInput.WorldPosition;
    gBuffer.ShadingModel = MATERIAL_SHADING_MODEL;
    gBuffer.CustomData = float4(material.SubsurfaceColor, material.Opacity);
    float4 light = GetLighting(ViewPos, DirectionalLight, gBuffer, 1.0, false, false);
    LOOP
    for (uint i = 0; i < LocalLightsCount; ++i)
        light += GetLighting(ViewPos, LocalLights[i], gBuffer, 1.0, true, LocalLights[i].SpotAngles.x > -2.0);
    output.rgb += light.rgb * material.AO;
#endif
#if MATERIAL_BLEND == MATERIAL_BLEND_ADDITIVE
    output.rgb *= material.Opacity;
#elif MATERIAL_BLEND == MATERIAL_BLEND_MULTIPLY
    output.rgb = lerp(1.0, output.rgb, material.Opacity);
#endif
}
