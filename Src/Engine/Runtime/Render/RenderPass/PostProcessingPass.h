#pragma once
#include <Runtime/Core/Math/Color.h>
#include <Runtime/Core/Math/Matrix.h>
#include <Runtime/Core/Math/Vector2.h>
#include <Runtime/Core/Math/Vector3.h>
#include <Runtime/Graphics/Base/GPUPipelineState.h>
#include <Runtime/Render/Assets/Texture/Texture.h>
#include <Runtime/Render/RenderContext.h>
#include <Runtime/Render/RendererPass.h>
#include <Runtime/Resource/AssetRef.h>
#include <Runtime/Resource/Assets/Materials/Shader.h>

namespace SE
{
    class Shader;

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

        float OutputDithering;
        float Padding;
        float VignetteIntensity;
        float LensDirtIntensity;

        Matrix LensFlareStarMat;
        });

    GES_PACK_STRUCT(struct GaussianBlurData {
        Float2 Size;
        float Dummy3;
        float Dummy4;
        Float4 GaussianBlurCache[GB_KERNEL_SIZE]; // x-weight, y-offset
        });

    // Post Processing
    AssetRef<Shader>  m_Shader = nullptr;
    GPUPipelineState* m_PSOThreshold = nullptr;
    GPUPipelineState* m_PSOScale     = nullptr;
    GPUPipelineState* m_PSOBlurH     = nullptr;
    GPUPipelineState* m_PSOBlurV     = nullptr;
    GPUPipelineState* m_PSOGenGhosts = nullptr;
    GPUPipelineState* m_PSOComposite = nullptr;

    ShaderProgramInstance* m_SPThreshold = nullptr;
    ShaderProgramInstance* m_SPScale = nullptr;
    ShaderProgramInstance* m_SPBlurH = nullptr;
    ShaderProgramInstance* m_SPBlurV = nullptr;
    ShaderProgramInstance* m_SPGenGhosts = nullptr;
    ShaderProgramInstance* m_SPComposite = nullptr;

    GPUConstantBuffer* m_DataBuffer         = nullptr;
    GPUConstantBuffer* m_GaussianBlurBuffer = nullptr;

    GaussianBlurData _gbData;
    Float4 GaussianBlurCacheH[GB_KERNEL_SIZE];
    Float4 GaussianBlurCacheV[GB_KERNEL_SIZE];

    AssetRef<Texture> _defaultLensColor = nullptr;
    AssetRef<Texture> _defaultLensStar = nullptr;
    AssetRef<Texture> _defaultLensDirt = nullptr;

public:

    /// <summary>
    /// Init
    /// </summary>
    PostProcessingPass() = default;

    // [RendererPass]
    String ToString() const override;
    bool Init() override;
    void Dispose() override;

public:
    /// <summary>
    /// Queue the view's HDR post processing and SDR output transform on the render thread.
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    /// <param name="pInput">Borrowed scene-linear HDR input, kept alive through submission by the caller.</param>
    /// <param name="pOutput">Borrowed target receiving the processed frame. Copies pInput unchanged if post processing is unavailable.</param>
    void Render(RenderContext& renderContext, GPUTexture* pInput, GPUTexture* pOutput);

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

    void OnShaderReloading(Asset* pAsset);

protected:

    // [RendererPass]
    bool SetupResources() override;
};

} // SE
