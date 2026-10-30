
#include "Shaders/ProgramCommon.hlsl"
#include "Shaders/GUICommon.hlsl"
#include "Shaders/QuadCommon.hlsl"


#ifndef BLUR_V
#define BLUR_V 0
#endif

#define MAX_SAMPLES 64

struct Data
{
	float4x4 ViewProjection;
}


struct BlurData
{
    float2 InvBufferSize;
    uint SampleCount;
    float Dummy0;
    float4 Bounds;
    float4 WeightAndOffsets[MAX_SAMPLES / 2];
};

ParameterBlock<Data> data;
ParameterBlock<BlurData> blurData;
Texture2D ImageTex;
SamplerState SamplerLinearClamp;
SamplerState SamplerPointClamp;

SHADER_PROGRAM(Image,
	SHADER_VS(VS),
	SHADER_PS(PS_Image)
)

SHADER_PROGRAM(ImagePoint,
	SHADER_VS(VS),
	SHADER_PS(PS_ImagePoint)
)

SHADER_PROGRAM(Color, 
	SHADER_VS(VS),
	SHADER_PS(PS_Color)
)

SHADER_PROGRAM(LineAA, 
	SHADER_VS(VS),
	SHADER_PS(PS_LineAA)
)

SHADER_PROGRAM(Font, 
	SHADER_VS(VS),
	SHADER_PS(PS_Font)
)

SHADER_PROGRAM(Downscale, 
	SHADER_VS(VS),
	SHADER_PS(PS_Downscale)
)

SHADER_PROGRAM(Blur, 
	SHADER_VS(VS),
	SHADER_PS(PS_Blur),
	SHADER_MACRO(_, BLUR_V)
)



VS2PS VS(Render2DVertex input)
{
	VS2PS output;
	
	// Render2D::RenderingFeatures::VertexSnapping
	if ((int)input.CustomDataAndClipOrigin.y & 1)
		input.Position = (float2)(int2)input.Position;

    output.Position = mul(float4(input.Position, 0, 1), data.ViewProjection);
	output.Color = input.Color;
	output.TexCoord = input.TexCoord;
	output.ClipOriginAndPos = float4(input.CustomDataAndClipOrigin.zw, input.Position);
	output.ClipExtents = input.ClipExtents;
	output.CustomData = input.CustomDataAndClipOrigin.xy;

	return output;
}

float4 PS_Image(VS2PS input) : SV_Target0
{
	PerformClipping(input);

	return ImageTex.Sample(SamplerLinearClamp, input.TexCoord) * input.Color;
}


float4 PS_ImagePoint(VS2PS input) : SV_Target0
{
	PerformClipping(input);

    return ImageTex.Sample(SamplerPointClamp, input.TexCoord) * input.Color;
}


float4 PS_Color(VS2PS input) : SV_Target0
{
	PerformClipping(input);

	return input.Color;
}

float4 PS_LineAA(VS2PS input) : SV_Target0
{
	PerformClipping(input);

	float lineWidth = input.CustomData.x; // Set per-vertex
	float filterWidthScale = 1; // Must match C++ code
	float gradient = input.TexCoord.x;
	float2 gradientDerivative = float2(abs(ddx(gradient)), abs(ddy(gradient)));
	float pixelSizeInUV = sqrt(dot(gradientDerivative, gradientDerivative));
	float halfLineWidthUV = 0.5f * pixelSizeInUV * lineWidth;
	float halfFilterWidthUV = filterWidthScale * pixelSizeInUV;
	float distanceToLineCenter = abs(0.5f - gradient);
	float lineCoverage = smoothstep(halfLineWidthUV + halfFilterWidthUV, halfLineWidthUV - halfFilterWidthUV, distanceToLineCenter);
	if (lineCoverage <= 0.0f)
		discard;
	input.Color.a *= lineCoverage;

	return input.Color;
}

float4 PS_Font(VS2PS input) : SV_Target0
{
	PerformClipping(input);

	float4 color = input.Color;
	color.a *= ImageTex.Sample(SamplerLinearClamp, input.TexCoord).r;
	return color;
}

float4 GetSample(float weight, float offset, float2 uv)
{
#if BLUR_V
    const float2 uvOffset = float2(offset * blurData.InvBufferSize.x, 0);
#else
    const float2 uvOffset = float2(0, offset * blurData.InvBufferSize.y);
#endif

	return ImageTex.Sample(SamplerLinearClamp, uv + uvOffset) * weight
	     + ImageTex.Sample(SamplerLinearClamp, uv - uvOffset) * weight;
}

// 1:-1 to 0:1
float2 ClipToUv(float2 clipPos)
{
	return clipPos * float2(0.5, -0.5) + float2(0.5, 0.5);
}

float4 PS_Downscale(Quad_VS2PS input) : SV_Target0
{
    float2 boundsPos = input.TexCoord * blurData.Bounds.zw + blurData.Bounds.xy;

	float4 clipPos = mul(float4(boundsPos, 0, 1), data.ViewProjection);
	clipPos.xy /= clipPos.w;

	float2 uvPos = ClipToUv(clipPos.xy);

	// TODO: use 4-tap box blur to reduce aliasing when downscaling

	return ImageTex.Sample(SamplerLinearClamp, uvPos);
}

float4 PS_Blur(Quad_VS2PS input) : SV_Target0
{
    float2 uv = input.TexCoord;
    float4 result = ImageTex.Sample(SamplerLinearClamp, uv) * blurData.WeightAndOffsets[0].x;

    {
        float weight = blurData.WeightAndOffsets[0].z;
        float offset = blurData.WeightAndOffsets[0].w;
		result += GetSample(weight, offset, uv);
	}

    for (uint i = 2; i < blurData.SampleCount; i += 2)
	{
		uint index = i / 2;
        {
            float weight = blurData.WeightAndOffsets[index].x;
            float offset = blurData.WeightAndOffsets[index].y;
			result += GetSample(weight, offset, uv);
		}

        {
            float weight = blurData.WeightAndOffsets[index].z;
            float offset = blurData.WeightAndOffsets[index].w;
			result += GetSample(weight, offset, uv);
		}
	}

	return float4(result.rgb, 1);
}
