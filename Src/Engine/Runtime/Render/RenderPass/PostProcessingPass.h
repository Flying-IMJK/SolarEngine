#pragma once
#include "Runtime/Core/Math/Color.h"
#include "Runtime/Core/Math/Matrix.h"
#include "Runtime/Core/Math/Vector2.h"
#include "Runtime/Core/Math/Vector3.h"
#include "Runtime/Graphics/Base/GPUPipelineStatePermutations.h"
#include "Runtime/Render/RenderContext.h"
#include "Runtime/Render/RendererPass.h"
#include "Runtime/Render/Assets/Texture/Texture.h"
#include "Runtime/Resource/AssetRef.h"
#include "Runtime/Resource/Assets/Materials/Shader.h"

namespace SE
{
    class GPUPipelineState;

#define GB_RADIUS 6
#define GB_KERNEL_SIZE (GB_RADIUS * 2 + 1)

/// <summary>
/// Post processing rendering service
/// </summary>
class PostProcessingPass : public RendererPass<PostProcessingPass>
{
private:

    GES_PACK_STRUCT(struct Data {
        float BloomLimit;
        float BloomThreshold;
        float BloomMagnitude;
        float BloomBlurSigma;

        Float3 VignetteColor;
        float VignetteShapeFactor;

        Float2 InputSize;
        float InputAspect;
        float GrainAmount;

        float GrainTime;
        float GrainParticleSize;
        int32 Ghosts;
        float HaloWidth;

        float HaloIntensity;
        float Distortion;
        float GhostDispersal;
        float LensFlareIntensity;

        Float2 LensInputDistortion;
        float LensScale;
        float LensBias;

        Float2 InvInputSize;
        float ChromaticDistortion;
        float Time;

        float Dummy1;
        float PostExposure;
        float VignetteIntensity;
        float LensDirtIntensity;

        Color ScreenFadeColor;

        Matrix LensFlareStarMat;
        });

    GES_PACK_STRUCT(struct GaussianBlurData {
        Float2 Size;
        float Dummy3;
        float Dummy4;
        Float4 GaussianBlurCache[GB_KERNEL_SIZE]; // x-weight, y-offset
        });

    // Post Processing
    AssetRef<Shader> _shader;
    GPUPipelineState* _psThreshold;
    GPUPipelineState* _psScale;
    GPUPipelineState* _psBlurH;
    GPUPipelineState* _psBlurV;
    GPUPipelineState* _psGenGhosts;
    GPUPipelineStatePermutationsPs<3> _psComposite;

    GaussianBlurData _gbData;
    Float4 GaussianBlurCacheH[GB_KERNEL_SIZE];
    Float4 GaussianBlurCacheV[GB_KERNEL_SIZE];

    AssetRef<Texture> _defaultLensColor;
    AssetRef<Texture> _defaultLensStar;
    AssetRef<Texture> _defaultLensDirt;

public:

    /// <summary>
    /// Init
    /// </summary>
    PostProcessingPass();

    // [RendererPass]
    String ToString() const override;
    bool Init() override;
    void Dispose() override;

public:

    /// <summary>
    /// Perform postFx rendering for the input task
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    /// <param name="input">Target with rendered HDR frame to post process</param>
    /// <param name="output">Output frame</param>
    /// <param name="colorGradingLUT">The prebaked LUT for color grading and tonemapping.</param>
    void Render(RenderContext& renderContext, GPUTexture* input, GPUTexture* output, GPUTexture* colorGradingLUT);

private:

    GPUTexture* GetCustomOrDefault(Texture* customTexture, AssetRef<Texture>& defaultTexture, const Char* defaultName);

    /// <summary>
    /// Calculates the Gaussian blur filter kernel. This implementation is
    /// ported from the original Java code appearing in chapter 16 of
    /// "Filthy Rich Clients: Developing Animated and Graphical Effects for Desktop Java".
    /// </summary>
    /// <param name="sigma">Gaussian Blur sigma parameter</param>
    /// <param name="width">Texture to blur width in pixels</param>
    /// <param name="height">Texture to blur height in pixels</param>
    void GB_ComputeKernel(float sigma, float width, float height);

#if COMPILE_WITH_DEV_ENV
    void OnShaderReloading(Asset* obj)
    {
        _psThreshold->ReleaseGPU();
        _psScale->ReleaseGPU();
        _psBlurH->ReleaseGPU();
        _psBlurV->ReleaseGPU();
        _psGenGhosts->ReleaseGPU();
        _psComposite.Release();
        invalidateResources();
    }
#endif

protected:

    // [RendererPass]
    bool SetupResources() override;
};

} // SE
