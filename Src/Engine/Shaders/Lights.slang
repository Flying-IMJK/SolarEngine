
#define USE_GBUFFER_CUSTOM_DATA

#include "Shaders/Common.hlsl"
#include "Shaders/MaterialCommon.hlsl"
#include "Shaders/LightingCommon.hlsl"
//#include "./Shaders/IESProfile.hlsl"
#include "Shaders/GBuffer.hlsl"
#include "Shaders/Lighting.hlsl"
#include "Shaders/QuadCommon.hlsl"

// Per light data
struct PerLight
{
	LightData Light;
	float4x4 WVP;
};

// Per frame data
struct PerFrame
{
	GBufferData GBuffer;
};

ParameterBlock<PerLight> perLight;
ParameterBlock<PerFrame> perFrame;


// Rendered shadow
#if USE_SHADOW_MASK
Texture2D Shadow;
#endif
// Texture2D IESTexture : register(t6);
// TextureCube CubeImage : register(t7);

// Vertex Shader for models rendering
Model_VS2PS VS_Model(ModelInput_PosOnly input)
{
	Model_VS2PS output;
	output.Position = mul(float4(input.Position.xyz, 1), perLight.WVP);
	output.ScreenPos = output.Position;
	return output;
}

// Pixel shader for directional light rendering
SHADER_PROGRAM(
	DirectionalLight,
	SHADER_VS(VS_Quad),
	SHADER_PS(PS_Directional),
	SHADER_MACRO(_, LIGHTING_NO_SPECULAR),
	SHADER_MACRO(_, USE_SHADOW_MASK)
)

void PS_Directional(Quad_VS2PS input, out float4 output : SV_Target0)
{
	output = 0;

	// Sample GBuffer
	GBufferData gBufferData = perFrame.GBuffer;
	GBufferSample gBuffer = SampleGBuffer(gBufferData, input.TexCoord);

	// Check if cannot shadow pixel
//	BRANCH
//	if (gBuffer.ShadingModel == SHADING_MODEL_UNLIT)
//	{
//		discard;
//		return;
//	}

	// Sample shadow mask
	float4 shadowMask = 1;
#if USE_SHADOW_MASK
	BRANCH
	if (perLight.Light.CastShadows > 0)
	{
		shadowMask = SAMPLE_RT(Shadow, input.TexCoord);
	}

#endif

	// Calculate lighting
	output = GetLighting(gBufferData.ViewPos, perLight.Light, gBuffer, shadowMask, false, false);
}

// Pixel shader for point light rendering

SHADER_PROGRAM(
	PointLight,
	SHADER_VS(VS_Model),
	SHADER_PS(PS_Point),
	SHADER_MACRO(_, LIGHTING_NO_SPECULAR),
	SHADER_MACRO(_, USE_SHADOW_MASK)
)

void PS_Point(Model_VS2PS input, out float4 output : SV_Target0)
{
	output = 0;

	// Obtain UVs corresponding to the current pixel
	float2 uv = (input.ScreenPos.xy / input.ScreenPos.w) * float2(0.5, -0.5) + float2(0.5, 0.5);

	// Sample GBuffer
	GBufferData gBufferData = perFrame.GBuffer;
	GBufferSample gBuffer = SampleGBuffer(gBufferData, uv);

	// Check if cannot shadow pixel
  /*BRANCH
	if (gBuffer.ShadingModel == SHADING_MODEL_UNLIT)
	{
		discard;
		return;
	} */

	// Sample shadow mask
	float4 shadowMask = 1;
#if USE_SHADOW_MASK
	BRANCH
	if (perLight.Light.CastShadows > 0)
	{
		shadowMask = SAMPLE_RT(Shadow, uv);
	}

#endif

	// Calculate lighting
	output = GetLighting(gBufferData.ViewPos, perLight.Light, gBuffer, shadowMask, true, false);

	// Apply IES texture
/* 	#if USE_IES_PROFILE
		output *= ComputeLightProfileMultiplier(IESTexture, gBuffer.WorldPos, Light.Position, -Light.Direction);
	#endif */
}

// Pixel shader for spot light rendering
/* META_PS(true, FEATURE_LEVEL_ES2)
META_PERMUTATION_2(LIGHTING_NO_SPECULAR=0, USE_IES_PROFILE=0)
META_PERMUTATION_2(LIGHTING_NO_SPECULAR=1, USE_IES_PROFILE=0)
META_PERMUTATION_2(LIGHTING_NO_SPECULAR=0, USE_IES_PROFILE=1)
META_PERMUTATION_2(LIGHTING_NO_SPECULAR=1, USE_IES_PROFILE=1)
void PS_Spot(Model_VS2PS input, out float4 output : SV_Target0)
{
	output = 0;

	// Obtain UVs corresponding to the current pixel
	float2 uv = (input.ScreenPos.xy / input.ScreenPos.w) * float2(0.5, -0.5) + float2(0.5, 0.5);

	// Sample GBuffer
	GBufferData gBufferData = GetGBufferData();
	GBufferSample gBuffer = SampleGBuffer(gBufferData, uv);

	// Check if cannot shadow pixel
	BRANCH
	if (gBuffer.ShadingModel == SHADING_MODEL_UNLIT)
	{
		discard;
		return;
	}

	// Sample shadow mask
	float4 shadowMask = 1;
	BRANCH
	if (Light.CastShadows > 0)
	{
		shadowMask = SAMPLE_RT(Shadow, uv);
	}

	// Calculate lighting
	output = GetLighting(gBufferData.ViewPos, Light, gBuffer, shadowMask, true, true);

	// Apply IES texture
	#if USE_IES_PROFILE
		output *= ComputeLightProfileMultiplier(IESTexture, gBuffer.WorldPos, Light.Position, -Light.Direction);
	#endif
} */

// Pixel shader for sky light rendering
/* META_PS(true, FEATURE_LEVEL_ES2)
float4 PS_Sky(Model_VS2PS input) : SV_Target0
{
	float4 output = 0;

	// Obtain UVs corresponding to the current pixel
	float2 uv = (input.ScreenPos.xy / input.ScreenPos.w) * float2(0.5, -0.5) + float2(0.5, 0.5);

	// Sample GBuffer
	GBufferData gBufferData = GetGBufferData();
	GBufferSample gBuffer = SampleGBuffer(gBufferData, uv);

	// Check if can light pixel
	if (gBuffer.ShadingModel != SHADING_MODEL_UNLIT)
	{
		output = GetSkyLightLighting(Light, gBuffer, CubeImage);
	}

	return output;
} */
