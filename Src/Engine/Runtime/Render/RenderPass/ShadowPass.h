#pragma once

#include <Runtime/Graphics/Base/GPUPipelineState.h>
#include <Runtime/Render/Assets/Geometry/Model.h>
#include <Runtime/Render/RenderList.h>
#include <Runtime/Render/RendererPass.h>
#include <Runtime/Resource/AssetRef.h>

namespace SE
{
    class GraphicShader;
    class GPUConstantBuffer;
    class GPUSampler;
    class ShaderProgramInstance;

    template<typename T>
    bool CanRenderShadow(const RenderView& view, const T& light)
    {
        bool result = false;
        switch ((ShadowsCastingMode)light.ShadowsMode)
        {
        case ShadowsCastingMode::StaticOnly:
            result = view.IsOfflinePass;
            break;
        case ShadowsCastingMode::DynamicOnly:
            result = !view.IsOfflinePass;
            break;
        case ShadowsCastingMode::All:
            result = true;
            break;
        default:
            break;
        }
        return result && light.ShadowsStrength > Math::ZeroTolerance;
    }

    class ShadowsPass : public RendererPass<ShadowsPass>
    {
    private:

        struct ShadowData
        {
            int32 ContextIndex;
            int32 ContextCount;
            bool BlendCSM;
            LightShadowData Constants;
        };

        // Shader stuff
        AssetRef<GraphicShader> m_Shader;

        SLC2GPUPipelineState*  m_SpoShadowDir;
        SLC2GPUPipelineState*  m_SpoShadowPoint;
        SLC2GPUPipelineState*  m_SpoShadowSpot;
        ShaderProgramInstance* m_SPShadowDir;
        ShaderProgramInstance* m_SPShadowPoint;
        ShaderProgramInstance* m_SPShadowSpot;
        GPUConstantBuffer*     m_GPUConstantBuffer;
        GPUSampler*            m_ShadowSamplerPCF;
        PixelFormat            m_ShadowMapFormat;

        // Shadow maps stuff
        int32 m_ShadowMapsSizeCSM;
        int32 m_ShadowMapsSizeCube;
        GPUTexture* m_ShadowMapCSM;
        GPUTexture* m_ShadowMapCube;
        Quality m_CurrentShadowMapsQuality;

        // Shadow map rendering stuff
        AssetRef<Model> m_SphereModel;
        List<ShadowData> m_ShadowData;

        // Cached state for the current frame rendering (setup via Prepare)
        int32 maxShadowsQuality;

    public:

        /// <summary>
        /// Init
        /// </summary>
        ShadowsPass();

    public:

        /// <summary>
        /// Gets current GPU memory usage by the shadow maps
        /// </summary>
        /// <returns>GPU memory used in bytes</returns>
        uint64 GetShadowMapsMemoryUsage() const;

    public:

        // TODO: use full scene shadow map atlas with dynamic slots allocation
        int32 LastDirLightIndex = -1;
        GPUTextureView* LastDirLightShadowMap = nullptr;
        LightShadowData LastDirLight;

    public:
        void Prepare();

        /// <summary>
        /// Setups the shadows rendering for batched scene drawing. Checks which lights will cast a shadow.
        /// </summary>
        void SetupShadows(RenderContext& renderContext, RenderContextBatch& renderContextBatch);

        /// <summary>
        /// Determines whether can render shadow for the specified light.
        /// </summary>
        /// <param name="renderContext">The rendering context.</param>
        /// <param name="light">The light.</param>
        /// <returns><c>true</c> if can render shadow for the specified light; otherwise, <c>false</c>.</returns>
        bool CanRenderShadow(const RenderContext& renderContext, const RendererPointLightData& light);

        /// <summary>
        /// Determines whether can render shadow for the specified light.
        /// </summary>
        /// <param name="renderContext">The rendering context.</param>
        /// <param name="light">The light.</param>
        /// <returns><c>true</c> if can render shadow for the specified light; otherwise, <c>false</c>.</returns>
        bool CanRenderShadow(const RenderContext& renderContext, const RendererSpotLightData& light);

        /// <summary>
        /// Determines whether can render shadow for the specified light.
        /// </summary>
        /// <param name="renderContext">The rendering context.</param>
        /// <param name="light">The light.</param>
        /// <returns><c>true</c> if can render shadow for the specified light; otherwise, <c>false</c>.</returns>
        bool CanRenderShadow(const RenderContext& renderContext, const RendererDirectionalLightData& light);

        /// <summary>
        /// Renders the shadow mask for the given light.
        /// </summary>
        /// <param name="renderContextBatch">The rendering context batch.</param>
        /// <param name="light">The light.</param>
        /// <param name="pShadowMask">The shadow mask (output).</param>
        void RenderShadow(RenderContextBatch&     renderContextBatch,
                          RendererPointLightData& light,
                          GPUTextureView*         pShadowMask);

        /// <summary>
        /// Renders the shadow mask for the given light.
        /// </summary>
        /// <param name="renderContextBatch">The rendering context batch.</param>
        /// <param name="light">The light.</param>
        /// <param name="pShadowMask">The shadow mask (output).</param>
        void
        RenderShadow(RenderContextBatch& renderContextBatch, RendererSpotLightData& light, GPUTextureView* pShadowMask);

        /// <summary>
        /// Renders the shadow mask for the given light.
        /// </summary>
        /// <param name="renderContextBatch">The rendering context batch.</param>
        /// <param name="light">The light.</param>
        /// <param name="index">The light index.</param>
        /// <param name="pShadowMask">The shadow mask (output).</param>
        void RenderShadow(RenderContextBatch&           renderContextBatch,
                          RendererDirectionalLightData& light,
                          int32                         index,
                          GPUTextureView*               pShadowMask);

    private:

        void UpdateShadowMapSize();
        void SetupRenderContext(RenderContext& renderContext, RenderContext& shadowContext);
        void SetupLight(RenderContext& renderContext, RenderContextBatch& renderContextBatch, RendererDirectionalLightData& light);
        void SetupLight(RenderContext& renderContext, RenderContextBatch& renderContextBatch, RendererPointLightData& light);
        void SetupLight(RenderContext& renderContext, RenderContextBatch& renderContextBatch, RendererSpotLightData& light);

#if COMPILE_WITH_DEV_ENV
        void OnShaderReloading(Asset* pObj);
#endif

    public:

        // [RendererPass]
        String ToString() const override;
        bool Init() override;
        void Dispose() override;

    protected:

        // [RendererPass]
        bool SetupResources() override;
    };
} // SE

