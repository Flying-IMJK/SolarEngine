
// Film Grain post-process shader v1.1
// Martins Upitis (martinsh) devlog-martinsh.blogspot.com
// 2013
//
// --------------------------
// This work is licensed under a Creative Commons Attribution 3.0 Unported License.
// So you are free to share, modify and adapt it for your needs, and even use it for commercial use.
// I would also love to hear about a project you are using it.
//
// Have fun,
// Martins
// --------------------------
//
// Perlin noise shader by toneburst:
// http://machinesdontcare.wordpress.com/2009/06/25/3d-perlin-noise-sphere-vertex-shader-sourcecode/
//
// Lens flares by John Chapman:
//https://john-chapman.github.io/2017/11/05/pseudo-lens-flare.html
//

#include "Shaders/Common.hlsl"
#include "Shaders/Random.hlsl"
#include "Shaders/PostProcess/GammaCorrectionCommon.hlsl"
#include "Shaders/QuadCommon.hlsl"

#define GB_RADIUS 6
#define GB_KERNEL_SIZE (GB_RADIUS * 2 + 1)

#ifndef USE_GRADING_LUT
#define USE_GRADING_LUT 0
#endif
#ifndef USE_3D_LUT
#define USE_3D_LUT 0
#endif


struct Data
{
	float BloomLimit;
	float BloomThreshold;
	float BloomMagnitude;
	float BloomBlurSigma;

	float3 VignetteColor;
	float VignetteShapeFactor;

	float2 InputSize;
	float InputAspect;
	float GrainAmount;

	float GrainTime;
	float GrainParticleSize;
	int Ghosts;
	float HaloWidth;

	float HaloIntensity;
	float Distortion;
	float GhostDispersal;
	float LensFlareIntensity;

	float2 LensInputDistortion;
	float LensScale;
	float LensBias;

	float2 InvInputSize;
	float ChromaticDistortion;
	float Time;

	float Dummy1;
	float PostExposure;
	float VignetteIntensity;
	float LensDirtIntensity;

	float4 ScreenFadeColor;

	float4x4 LensFlareStarMat;
}

struct GaussianBlurData
{
	float2 Size;
	float Dummy3;
	float Dummy4;
	float4 GaussianBlurCache[GB_KERNEL_SIZE]; // x-weight, y-offset
}

ParameterBlock<Data> data;
ParameterBlock<GaussianBlurData> gaussianBlurData;


// Film Grain
static const float permTexUnit = 1.0 / 256.0;      // Perm texture texel-size
static const float permTexUnitHalf = 0.5 / 256.0;  // Half perm texture texel-size

// Input textures
Texture2D Input0;
Texture2D Input1;
Texture2D Input2;
Texture2D Input3;
Texture2D LensDirt;
Texture2D LensStar;
Texture2D LensColor;
#if USE_3D_LUT
Texture3D ColorGradingLUT;
#else
Texture2D ColorGradingLUT;
#endif
static const float LUTSize = 32;

half3 ColorLookupTable(half3 linearColor)
{
	// Move from linear color to encoded LUT color space
	//float3 encodedColor = linearColor; // Default
	float3 encodedColor = LinearToLog(linearColor + LogToLinear(0)); // Log

	float3 uvw = encodedColor * ((LUTSize - 1) / LUTSize) + (0.5f / LUTSize);

#if USE_3D_LUT
	half3 color = ColorGradingLUT.Sample(SamplerLinearClamp, uvw).rgb;
#else
	half3 color = SampleUnwrappedTexture3D(ColorGradingLUT, SamplerLinearClamp, uvw, LUTSize).rgb;
#endif

	return color;
}

// A random texture generator
float4 rnmRGBA(in float2 tc, in float time)
{
    float noise =  sin(dot(tc + float2(time, time), float2(12.9898, 78.233))) * 43758.5453;
	float noiseR =  frac(noise) * 2.0 - 1.0;
	float noiseG =  frac(noise * 1.2154) * 2.0 - 1.0;
	float noiseB =  frac(noise * 1.3453) * 2.0 - 1.0;
	float noiseA =  frac(noise * 1.3647) * 2.0 - 1.0;
	return float4(noiseR, noiseG, noiseB, noiseA);
}

float3 rnmRGB(in float2 tc, in float time)
{
    float noise =  sin(dot(tc + float2(time, time), float2(12.9898, 78.233))) * 43758.5453;
	float noiseR =  frac(noise) * 2.0 - 1.0;
	float noiseG =  frac(noise * 1.2154) * 2.0 - 1.0;
	float noiseB =  frac(noise * 1.3453) * 2.0 - 1.0;
	return float3(noiseR, noiseG, noiseB);
}

float2 rnmRG(in float2 tc, in float time)
{
    float noise =  sin(dot(tc + float2(time, time), float2(12.9898, 78.233))) * 43758.5453;
	float noiseR =  frac(noise) * 2.0 - 1.0;
	float noiseG =  frac(noise * 1.2154) * 2.0 - 1.0;
	return float2(noiseR, noiseG);
}

float rnmA(in float2 tc, in float time)
{
    float noise =  sin(dot(tc + float2(time, time), float2(12.9898, 78.233))) * 43758.5453;
	float noiseA =  frac(noise * 1.3647) * 2.0 - 1.0;
	return noiseA;
}

float pnoise3D(in float3 p, in float time)
{
	// Integer part, scaled so +1 moves permTexUnit texel
	float3 pi = permTexUnit * floor(p) + permTexUnitHalf;
	// and offset 1/2 texel to sample texel centers. Fractional part for interpolation
	float3 pf = frac(p);

	// Noise contributions from (x=0, y=0), z=0 and z=1
	float perm00 = rnmA(pi.xy, time);
	float3  grad000 = rnmRGB(float2(perm00, pi.z), time) * 4.0 - 1.0;
	float n000 = dot(grad000, pf);
	float3  grad001 = rnmRGB(float2(perm00, pi.z + permTexUnit), time) * 4.0 - 1.0;
	float n001 = dot(grad001, pf - float3(0.0, 0.0, 1.0));

	// Noise contributions from (x=0, y=1), z=0 and z=1
	float perm01 = rnmA(pi.xy + float2(0.0, permTexUnit), time);
	float3  grad010 = rnmRGB(float2(perm01, pi.z), time) * 4.0 - 1.0;
	float n010 = dot(grad010, pf - float3(0.0, 1.0, 0.0));
	float3  grad011 = rnmRGB(float2(perm01, pi.z + permTexUnit), time) * 4.0 - 1.0;
	float n011 = dot(grad011, pf - float3(0.0, 1.0, 1.0));

	// Noise contributions from (x=1, y=0), z=0 and z=1
	float perm10 = rnmA(pi.xy + float2(permTexUnit, 0.0), time);
	float3  grad100 = rnmRGB(float2(perm10, pi.z), time) * 4.0 - 1.0;
	float n100 = dot(grad100, pf - float3(1.0, 0.0, 0.0));
	float3  grad101 = rnmRGB(float2(perm10, pi.z + permTexUnit), time) * 4.0 - 1.0;
	float n101 = dot(grad101, pf - float3(1.0, 0.0, 1.0));

	// Noise contributions from (x=1, y=1), z=0 and z=1
	float perm11 = rnmA(pi.xy + float2(permTexUnit, permTexUnit), time);
	float3  grad110 = rnmRGB(float2(perm11, pi.z), time) * 4.0 - 1.0;
	float n110 = dot(grad110, pf - float3(1.0, 1.0, 0.0));
	float3  grad111 = rnmRGB(float2(perm11, pi.z + permTexUnit), time) * 4.0 - 1.0;
	float n111 = dot(grad111, pf - float3(1.0, 1.0, 1.0));

	// Blend contributions along x
	float4 n_x = lerp(float4(n000, n001, n010, n011), float4(n100, n101, n110, n111), PerlinRamp(pf.x));

	// Blend contributions along y
	float2 n_xy = lerp(n_x.xy, n_x.zw, PerlinRamp(pf.y));

	// Blend contributions along z
	float n_xyz = lerp(n_xy.x, n_xy.y, PerlinRamp(pf.z));

	// We're done, return the final noise value
	return n_xyz;
}

float pnoise2D(in float2 p, in float time)
{
	// Integer part, scaled so +1 moves permTexUnit texel
	float2 pi = permTexUnit * floor(p) + permTexUnitHalf;
	// and offset 1/2 texel to sample texel centers. Fractional part for interpolation
	float2 pf = frac(p);

	// Noise contributions from (x=0, y=0)
	float perm00 = rnmA(pi.xy, time);
	float2 grad000 = rnmRG(float2(perm00, 0), time) * 4.0 - 1.0;
	float n000 = dot(grad000, pf);

	// Noise contributions from (x=0, y=1)
	float perm01 = rnmA(pi.xy + float2(0.0, permTexUnit), time);
	float2 grad010 = rnmRG(float2(perm01, 0), time) * 4.0 - 1.0;
	float n010 = dot(grad010, pf - float2(0.0, 1.0));

	// Noise contributions from (x=1, y=0)
	float perm10 = rnmA(pi.xy + float2(permTexUnit, 0.0), time);
	float2 grad100 = rnmRG(float2(perm10, 0), time) * 4.0 - 1.0;
	float n100 = dot(grad100, pf - float2(1.0, 0.0));

	// Noise contributions from (x=1, y=1)
	float perm11 = rnmA(pi.xy + float2(permTexUnit, permTexUnit), time);
	float2 grad110 = rnmRG(float2(perm11, 0), time) * 4.0 - 1.0;
	float n110 = dot(grad110, pf - float2(1.0, 1.0));

	// Blend contributions along x
	float2 n_x = lerp(float2(n000, n010), float2(n100, n110), PerlinRamp(pf.x));

	// Blend contributions along y
	float n_xy = lerp(n_x.x, n_x.y, PerlinRamp(pf.y));

	// We're done, return the final noise value
	return n_xy;
}

// 2d coordinate orientation thing
float2 coordRot(in float2 tc, in float angle)
{
	float rotX = ((tc.x * 2.0 - 1.0) * data.InputAspect * cos(angle)) - ((tc.y * 2.0 - 1.0) * sin(angle));
	float rotY = ((tc.y * 2.0 - 1.0) * cos(angle)) + ((tc.x * 2.0 - 1.0) * data.InputAspect * sin(angle));
	rotX = ((rotX / data.InputAspect) * 0.5 + 0.5);
	rotY = rotY * 0.5 + 0.5;
	return float2(rotX, rotY);
}


SHADER_PROGRAM(
	Threshold,
	SHADER_VS(VS_Quad),
	SHADER_PS(PS_Threshold)
)

SHADER_PROGRAM(
	Scale,
	SHADER_VS(VS_Quad),
	SHADER_PS(PS_Scale)
)

SHADER_PROGRAM(
	GaussainBlurH,
	SHADER_VS(VS_Quad),
	SHADER_PS(PS_GaussainBlurH)
)

SHADER_PROGRAM(
	GaussainBlurV,
	SHADER_VS(VS_Quad),
	SHADER_PS(PS_GaussainBlurV)
)

SHADER_PROGRAM(
	Ghosts,
	SHADER_VS(VS_Quad),
	SHADER_PS(PS_Ghosts)
)

// Uses a lower exposure to produce a value suitable for a bloom pass
float4 PS_Threshold(Quad_VS2PS input) : SV_Target
{
	float4 color = Input0.SampleLevel(SamplerLinearClamp, input.TexCoord, 0);
    return clamp(color - data.BloomThreshold, 0, data.BloomLimit);
}

// Uses hw bilinear filtering for upscaling or downscaling
float4 PS_Scale(Quad_VS2PS input) : SV_Target
{
	// TODO: we could use quality switch for bloom effect

	return Input0.SampleLevel(SamplerLinearClamp, input.TexCoord, 0);
	/*
	float3 color;
	// TODO: use gather for dx11 and dx12??
	color  = Input0.SampleLevel(SamplerLinearClamp, input.TexCoord, 0, int2( 0, 0)).rgb;
	color += Input0.SampleLevel(SamplerLinearClamp, input.TexCoord, 0, int2( 0, 1)).rgb;
	color += Input0.SampleLevel(SamplerLinearClamp, input.TexCoord, 0, int2( 0,-1)).rgb;
	color += Input0.SampleLevel(SamplerLinearClamp, input.TexCoord, 0, int2(-1, 0)).rgb;
	color += Input0.SampleLevel(SamplerLinearClamp, input.TexCoord, 0, int2( 1, 0)).rgb;
	color *= (1.0f / 5.0f);

	return float4(color, 1);
	*/
}

// Horizontal gaussian blur
float4 PS_GaussainBlurH(Quad_VS2PS input) : SV_Target
{
	float4 color = 0;

	UNROLL
	for (int i = 0; i < GB_KERNEL_SIZE; i++)
	{
		color += Input0.Sample(SamplerLinearClamp, input.TexCoord + float2(gaussianBlurData.GaussianBlurCache[i].y, 0.0)) * gaussianBlurData.GaussianBlurCache[i].x;
	}

	return color;
}

// Vertical gaussian blur
float4 PS_GaussainBlurV(Quad_VS2PS input) : SV_Target
{
	float4 color = 0;

	UNROLL
	for (int i = 0; i < GB_KERNEL_SIZE; i++)
	{
		color += Input0.Sample(SamplerLinearClamp, input.TexCoord + float2(0.0, gaussianBlurData.GaussianBlurCache[i].y)) * gaussianBlurData.GaussianBlurCache[i].x;
	}

	return color;
}

// Generate 'ghosts' for lens flare
float4 PS_Ghosts(Quad_VS2PS input) : SV_Target
{
	// Temporary data
	int i = 0;
	float weight;
	float2 offset;
	float2 haloFrac;
	float3 color;
	float3 result = 0;

	// Flip texcoordoords
	float2 texcoord = input.TexCoord * -1 + float2(1.0, 1.0);

	// Ahost vector to image centre
	float2 ghostVec = (float2(0.5, 0.5) - texcoord) * data.GhostDispersal;// TODO: optimize to MAD instruction
	float2 ghostVecnNorm = normalize(ghostVec);
	float2 haloVec = ghostVecnNorm * data.HaloWidth;

	// Calculate distortion vector
	float3 distortion = float3(data.LensInputDistortion.x, 0.0, data.LensInputDistortion.y);

	// Sample 'ghosts'
	// TODO: use uniform amount of ghosts and unroll loop
	LOOP
	for(; i < data.Ghosts; i++)
	{
		// Calculate ghost offset
		offset = frac(texcoord + ghostVec * (float)i);

		// Calculate ghost weight
		weight = pow(1.0 - length(float2(0.5, 0.5) - offset) / length(float2(0.71, 0.6)), 10.0);

		// Sample distored lens downsampled/threshold texture
		color = float3(
			Input3.Sample(SamplerLinearClamp, offset + ghostVecnNorm * distortion.r).r,
			Input3.Sample(SamplerLinearClamp, offset + ghostVecnNorm * distortion.g).g,
			Input3.Sample(SamplerLinearClamp, offset + ghostVecnNorm * distortion.b).b);
		color = clamp(color + data.LensBias, 0, 10) * (data.LensScale * weight);

		// Accumulate color
		result += color;
	}

	// Apply lens color
	result *= LensColor.Sample(SamplerLinearWrap, float2(length(float2(0.5, 0.5) - texcoord) / length(float2(0.5, 0.5)), 0)).rgb;

	// Add halo
	haloFrac = frac(texcoord + haloVec);
	weight = length(float2(0.5, 0.5) - haloFrac) / length(float2(0.5, 0.5));
	weight = pow(1.0 - weight, 5.0) * data.HaloIntensity;
	color = float3(
			Input3.Sample(SamplerLinearClamp, haloFrac + ghostVecnNorm * distortion.r).r,
			Input3.Sample(SamplerLinearClamp, haloFrac + ghostVecnNorm * distortion.g).g,
			Input3.Sample(SamplerLinearClamp, haloFrac + ghostVecnNorm * distortion.b).b);
	result += clamp((color + data.LensBias) * (data.LensScale * weight), 0, 8);

	return float4(result, 1);
}

float remap(float t, float a, float b)
{
	return clamp((t - a) / (b - a), 0.0, 1.0);
}

float2 remap(float2 t, float2 a, float2 b)
{
	return clamp((t - a) / (b - a), 0.0, 1.0);
}

float2 radialdistort(float2 coord, float2 amt)
{
	float2 cc = coord - 0.5;
	return coord + 2.0 * cc * amt;
}

float2 distort(float2 uv, float t, float2 min_distort, float2 max_distort)
{
    float2 dist = lerp(min_distort, max_distort, t);
    float2 cc = uv - 0.5;
	return uv + 4.0 * cc * dist;
}

float3 spectrum_offset(float t)
{
    float t0 = 3.0 * t - 1.5;
	return clamp(float3( -t0, 1.0 - abs(t0), t0), 0.0, 1.0);
}

float nrand(float2 n)
{
	return frac(sin(dot(n.xy, float2(12.9898, 78.233)))* 43758.5453);
}


SHADER_PROGRAM(
	Composite,
	SHADER_VS(VS_Quad),
	SHADER_PS(PS_Composite),
	SHADER_MACRO(_, USE_GRADING_LUT),
	SHADER_MACRO(_, USE_3D_LUT)
)

// Applies exposure, color grading and tone mapping to the input.
// Combines it with the results of the bloom pass and other postFx.
float4 PS_Composite(Quad_VS2PS input) : SV_Target
{
	float2 uv = input.TexCoord;
	float3 lensLight = 0;
	float4 color;

	// Chromatic Abberation
	if (data.ChromaticDistortion > 0)
	{
		const float MAX_DIST_PX = 24.0;
		float max_distort_px = MAX_DIST_PX * data.ChromaticDistortion;
		float2 max_distort = data.InvInputSize * max_distort_px;
		float2 min_distort = 0.5 * max_distort;

		float2 oversiz = distort(float2(1.0, 1.0), 1.0, min_distort, max_distort);
		uv = remap(uv, 1.0 - oversiz, oversiz);

		int iterations = (int)lerp(3, 10, data.ChromaticDistortion);
		float stepsiz = 1.0 / (float(iterations) - 1.0);
		float rnd = nrand(uv + data.Time);
		float t = rnd * stepsiz;

		float4 sumcol = 0;
		float4 sumw = 0;
		for (int i = 0; i < iterations; i++)
		{
			float4 w = float4(spectrum_offset(t), 1);
			sumw += w;
			float2 uvd = distort(uv, t, min_distort, max_distort);
			sumcol += Input0.Sample(SamplerLinearClamp, uvd) * w;
			t += stepsiz;
		}
		sumcol /= sumw;
		color = sumcol + (rnd / 255.0);
	}
	else
	{
		color = Input0.Sample(SamplerLinearClamp, uv);
	}

	// Lens Flares
	BRANCH
	if (data.LensFlareIntensity > 0)
	{
		// Get lens flare color
		float3 lensFlares = Input3.Sample(SamplerLinearClamp, uv).rgb * data.LensFlareIntensity;

		// Get lens star color and mix it with lens flares
		float2 lensStarTexcoord = uv - 0.5;
		lensStarTexcoord = mul(lensStarTexcoord, (float2x2)data.LensFlareStarMat).xy;
		lensStarTexcoord += 0.5;
		float3 lensStar = LensStar.Sample(SamplerLinearClamp, lensStarTexcoord).rgb;
		lensFlares *= lensStar * 2 + 0.5;

		// Accumulate final lens flares lght
		lensLight += lensFlares * 1.5f;
		color.rgb += lensFlares;
	}

	// Bloom
	BRANCH
	if (data.BloomMagnitude > 0)
	{
		// Sample the bloom
		float3 bloom = Input2.SampleLevel(SamplerLinearClamp, uv, 0).rgb;
		bloom = bloom * data.BloomMagnitude;

		// Accumulate final bloom lght
		lensLight += max(0, bloom * 3.0f + (- 1.0f * 3.0f));
		color.rgb += bloom;
	}

	// Lens Dirt
	float3 lensDirt = LensDirt.SampleLevel(SamplerLinearClamp, uv, 0).rgb;
	color.rgb += lensDirt * (lensLight * data.LensDirtIntensity);

	// Eye Adaptation post exposure
//	color.rgb *= PostExposure;

	// Color Grading and Tone Mapping
#if USE_GRADING_LUT
	color.rgb = ColorLookupTable(color.rgb);
#endif

	// Film Grain
	BRANCH
	if (data.GrainAmount > 0)
	{
		// Calculate noise
		float2 rotCoordsR = coordRot(uv, data.GrainTime);
		float noise = pnoise2D(rotCoordsR * (data.InputSize / data.GrainParticleSize), data.GrainTime);

		// Noisiness response curve based on scene luminance
		float luminance = Luminance(saturate(color.rgb));
		luminance += smoothstep(0.2, 0.0, luminance);

		// Add noise to the final color
		noise = lerp(noise, 0, min(pow(luminance, 4.0), 100));
		color.rgb += noise * data.GrainAmount;
	}

	// Vignette
	BRANCH
	if (data.VignetteIntensity > 0)
	{
		float2 uvCircle = uv * (1 - uv);
		float uvCircleScale = uvCircle.x * uvCircle.y * 16.0f;
		float mask = lerp(1, pow(uvCircleScale, data.VignetteShapeFactor), data.VignetteIntensity);
		color.rgb = lerp(data.VignetteColor, color.rgb, mask);
	}

	// Screen fade
	color.rgb = lerp(color.rgb, data.ScreenFadeColor.rgb, data.ScreenFadeColor.a);

	// Saturate color since it will be rendered to the screen
	color.rgb = saturate(color.rgb);

	// Return final pixel color (preserve input alpha)
	return color;
}
