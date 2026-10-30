
#define USE_GBUFFER_CUSTOM_DATA

#include "Shaders/Common.hlsl"
#include "Shaders/QuadCommon.hlsl"
#include "Shaders/GBuffer.hlsl"
#include "Shaders/MaterialCommon.hlsl"
#include "Shaders/ShadowsSampling.hlsl"

struct PerLight
{
	GBufferData GBuffer;
	LightData Light;
	LightShadowData LightShadow;
	float4x4 WVP;
	float4x4 ViewProjectionMatrix;
	float2 Dummy0;
	float ContactShadowsDistance;
	float ContactShadowsLength;
}

ParameterBlock<PerLight> preLight;

DECLARE_GBUFFERDATA_ACCESS(GBuffer)
DECLARE_LIGHTSHADOWDATA_ACCESS(LightShadow);

#if CONTACT_SHADOWS

float RayCastScreenSpaceShadow(GBufferData gBufferData, GBufferSample gBuffer, float3 rayStartWS, float3 rayDirWS, float rayLength)
{
#if SHADOWS_QUALITY == 3
	const uint maxSteps = 16;
#elif SHADOWS_QUALITY == 3
	const uint maxSteps = 12;
#else
	const uint maxSteps = 8;
#endif
	float distanceFade = 1 - saturate(pow(length(gBuffer.WorldPos - gBufferData.ViewPos) / ContactShadowsDistance, 2));
	float maxShadowLength = gBufferData.InvProjectionMatrix[1][1] * gBuffer.ViewPos.z * rayLength * distanceFade;
	float4 rayStartCS = mul(float4(rayStartWS, 1), ViewProjectionMatrix);
	float4 rayEndCS = mul(float4(rayStartWS + rayDirWS * maxShadowLength, 1), ViewProjectionMatrix);
	float4 rayStepCS = (rayEndCS - rayStartCS) / maxSteps;
	float4 rayCS = rayStartCS + rayStepCS;
	float lightAmountMax = 0;
	for (uint step = 0; step < maxSteps; step++)
	{
		float3 rayUV = rayCS.xyz / rayCS.w;
		rayUV.xy = rayUV.xy * float2(0.5, -0.5) + float2(0.5, 0.5);
		float sceneDepth = SampleDepth(gBufferData, rayUV.xy) * gBufferData.ViewFar;
		float rayDepth = (gBufferData.ViewInfo.w / (rayUV.z - gBufferData.ViewInfo.z)) * gBufferData.ViewFar * 0.998;
		float surfaceThickness = 0.035f + rayDepth * rayLength;
		float depthTestHardness = 0.005f;
		float lightAmount = saturate((rayDepth - sceneDepth) / depthTestHardness) * saturate((sceneDepth + surfaceThickness - rayDepth) / depthTestHardness);
		lightAmountMax = max(lightAmountMax, lightAmount);
		rayCS += rayStepCS;
	}
	return 1 - lightAmountMax;
}

#endif

// Vertex Shader for shadow volume model rendering
Model_VS2PS VS_Model(ModelInput_PosOnly input)
{
	Model_VS2PS output;
	output.Position = mul(float4(input.Position.xyz, 1), preLight.WVP);
	output.ScreenPos = output.Position;
	return output;
}



TextureCube<float> ShadowMapPoint;

// Pixel shader for point light shadow rendering
SHADER_PROGRAM(
	PointLight,
	SHADER_VS(VS_Model),
	SHADER_PS(PS_PointLight),
	SHADER_MACRO(SHADOWS_QUALITY0, SHADOWS_QUALITY1, SHADOWS_QUALITY2, SHADOWS_QUALITY3),
	SHADER_MACRO(_, CONTACT_SHADOWS)
)

float4 PS_PointLight(Model_VS2PS input) : SV_Target0
{
	float shadow = 1;
	float subsurfaceShadow = 1;

	// Obtain texture coordinates corresponding to the current pixel
	float2 uv = (input.ScreenPos.xy / input.ScreenPos.w) * float2(0.5, -0.5) + float2(0.5, 0.5);

	// Sample GBuffer
	GBufferData gBufferData = GetGBufferData();
	GBufferSample gBuffer = SampleGBuffer(gBufferData, uv);

	// Sample shadow
	LightShadowData lightShadowData = GetLightShadowData();
	shadow = SampleShadow(preLight.Light, lightShadowData, ShadowMapPoint, gBuffer, subsurfaceShadow);

#if CONTACT_SHADOWS
	// Calculate screen-space contact shadow
	shadow *= RayCastScreenSpaceShadow(gBufferData, gBuffer, gBuffer.WorldPos, normalize(Light.Position - gBuffer.WorldPos), ContactShadowsLength);
#endif

	return float4(shadow, subsurfaceShadow, 1, 1);
}



Texture2DArray ShadowMapDir

// shader for directional light shadow rendering
SHADER_PROGRAM(
	DirLight,
	SHADER_VS(VS_Quad),
	SHADER_PS(PS_DirLight),
	SHADER_MACRO(SHADOWS_QUALITY0, SHADOWS_QUALITY1, SHADOWS_QUALITY2, SHADOWS_QUALITY3),
	SHADER_MACRO(_, CSM_BLENDING),
	SHADER_MACRO(_, CONTACT_SHADOWS)
)


float4 PS_DirLight(Quad_VS2PS input) : SV_Target0
{
	float shadow = 1;
	float subsurfaceShadow = 1;

	// Sample GBuffer
	GBufferData gBufferData = GetGBufferData();
	GBufferSample gBuffer = SampleGBuffer(gBufferData, input.TexCoord);

	// Sample shadow
	LightShadowData lightShadowData = GetLightShadowData();
	shadow = SampleShadow(preLight.Light, lightShadowData, ShadowMapDir, gBuffer, subsurfaceShadow);

#if CONTACT_SHADOWS
	// Calculate screen-space contact shadow
	shadow *= RayCastScreenSpaceShadow(gBufferData, gBuffer, gBuffer.WorldPos, Light.Direction, preLight.ContactShadowsLength);
#endif

	return float4(shadow, subsurfaceShadow, 1, 1);
}



Texture2D ShadowMapSpot;

// shader for spot light shadow rendering
SHADER_PROGRAM(
	SpotLight,
	SHADER_VS(VS_Model),
	SHADER_PS(PS_DirLight),
	SHADER_MACRO(SHADOWS_QUALITY0, SHADOWS_QUALITY1, SHADOWS_QUALITY2, SHADOWS_QUALITY3),
	SHADER_MACRO(_, CSM_BLENDING),
	SHADER_MACRO(_, CONTACT_SHADOWS)
)


float4 PS_SpotLight(Model_VS2PS input) : SV_Target0
{
	float shadow = 1;
	float subsurfaceShadow = 1;

	// Obtain texture coordinates corresponding to the current pixel
	float2 uv = (input.ScreenPos.xy / input.ScreenPos.w) * float2(0.5, -0.5) + float2(0.5, 0.5);

	// Sample GBuffer
	GBufferData gBufferData = GetGBufferData();
	GBufferSample gBuffer = SampleGBuffer(gBufferData, uv);

	// Sample shadow
	LightShadowData lightShadowData = GetLightShadowData();
	shadow = SampleShadow(preLight.Light, lightShadowData, ShadowMapSpot, gBuffer, subsurfaceShadow);

#if CONTACT_SHADOWS
	// Calculate screen-space contact shadow
	shadow *= RayCastScreenSpaceShadow(gBufferData, gBuffer, gBuffer.WorldPos, normalize(preLight.Light.Position - gBuffer.WorldPos), preLight.ContactShadowsLength);
#endif

	return float4(shadow, preLight.subsurfaceShadow, 1, 1);
}
