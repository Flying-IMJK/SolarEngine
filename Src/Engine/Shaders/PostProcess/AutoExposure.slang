#include "Shaders/ProgramCommon.hlsl"

SHADER_PROGRAM(ResetHistogram, SHADER_CS(CS_ResetHistogram))
SHADER_PROGRAM(BuildHistogram, SHADER_CS(CS_BuildHistogram))
SHADER_PROGRAM(AdaptExposure, SHADER_CS(CS_AdaptExposure))

struct AutoExposureData
{
    float2 InputSize;
    float2 EVLimits;
    float4 Metering;   // low/high pixel fractions, compensation in stops, padding
    float4 Adaptation; // elapsed seconds, brighter/darker environment rates, reset
};
ConstantBuffer<AutoExposureData> data;
Texture2D<float4> SceneColor;
RWTexture2D<uint> HistogramOutput;
Texture2D<uint> HistogramInput;
Texture2D<float4> ExposureHistory;
RWTexture2D<float4> ExposureOutput;

groupshared uint HistogramBins[256];
groupshared float WeightedLogLuminance[256];
groupshared float SampleWeights[256];

// Bin zero excludes black/non-finite samples; the other bins span 64 luminance stops.
static const float MinLogLuminance = -24.0;
static const float LogLuminanceRange = 64.0;

[numthreads(256, 1, 1)]
void CS_ResetHistogram(uint index : SV_GroupIndex)
{
    HistogramOutput[uint2(index, 0)] = 0;
}

[numthreads(16, 16, 1)]
void CS_BuildHistogram(uint3 id : SV_DispatchThreadID, uint index : SV_GroupIndex)
{
    HistogramBins[index] = 0;
    GroupMemoryBarrierWithGroupSync();
    if (all(id.xy < uint2(data.InputSize)))
    {
        float3 color = SceneColor.Load(int3(id.xy, 0)).rgb;
        float luminance = dot(max(color, 0.0), float3(0.2126, 0.7152, 0.0722));
        uint bin = 0;
        if (!any(isnan(color)) && !any(isinf(color)) && luminance > exp2(MinLogLuminance))
        {
            float normalized = saturate((log2(luminance) - MinLogLuminance) / LogLuminanceRange);
            bin = 1 + min(uint(normalized * 255.0), 254u);
        }
        InterlockedAdd(HistogramBins[bin], 1);
    }
    GroupMemoryBarrierWithGroupSync();
    if (HistogramBins[index] > 0)
        InterlockedAdd(HistogramOutput[uint2(index, 0)], HistogramBins[index]);
}

[numthreads(256, 1, 1)]
void CS_AdaptExposure(uint index : SV_GroupIndex)
{
    uint count = index == 0 ? 0 : HistogramInput.Load(int3(index, 0, 0));
    HistogramBins[index] = count;
    GroupMemoryBarrierWithGroupSync();

    // Inclusive prefix counts allow partially retained percentile boundary bins.
    for (uint offset = 1; offset < 256; offset <<= 1)
    {
        uint addend = HistogramBins[index >= offset ? index - offset : 0] * (index >= offset ? 1u : 0u);
        GroupMemoryBarrierWithGroupSync();
        HistogramBins[index] += addend;
        GroupMemoryBarrierWithGroupSync();
    }
    float total = float(HistogramBins[255]);
    float low = total * data.Metering.x;
    float high = total * data.Metering.y;
    float end = float(HistogramBins[index]);
    float begin = end - float(count);
    float weight = max(min(end, high) - max(begin, low), 0.0);
    float logLuminance = MinLogLuminance + (float(index) - 0.5) * (LogLuminanceRange / 255.0);
    WeightedLogLuminance[index] = weight * logLuminance;
    SampleWeights[index] = weight;
    GroupMemoryBarrierWithGroupSync();
    for (uint stride = 128; stride > 0; stride >>= 1)
    {
        if (index < stride)
        {
            WeightedLogLuminance[index] += WeightedLogLuminance[index + stride];
            SampleWeights[index] += SampleWeights[index + stride];
        }
        GroupMemoryBarrierWithGroupSync();
    }
    if (index == 0)
    {
        // Black scenes use the brightest allowed exposure, without log2(0).
        float targetEV = data.EVLimits.x;
        if (SampleWeights[0] > 0.0)
            targetEV = clamp(WeightedLogLuminance[0] / SampleWeights[0] + 3.0 - data.Metering.z,
                             data.EVLimits.x, data.EVLimits.y);
        float adaptedEV = targetEV;
        if (data.Adaptation.w == 0.0)
        {
            float previousEV = clamp(ExposureHistory.Load(int3(0, 0, 0)).y, data.EVLimits.x, data.EVLimits.y);
            float speed = targetEV > previousEV ? data.Adaptation.y : data.Adaptation.z;
            adaptedEV = lerp(previousEV, targetEV, 1.0 - exp(-data.Adaptation.x * speed));
        }
        ExposureOutput[uint2(0, 0)] = float4(exp2(-adaptedEV) / 1.2, adaptedEV, 0.0, 0.0);
    }
}
