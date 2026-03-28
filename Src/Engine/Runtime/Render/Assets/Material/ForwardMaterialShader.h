#pragma once
#include "MaterialShader.h"
#include "Runtime/Render/RenderEnum.h"

namespace SE
{
    /// <summary>
    /// Represents material that can be used to render objects with Forward Rendering.
    /// </summary>
    class ForwardMaterialShader : public MaterialShader
    {
    private:
        struct Cache
        {
            PipelineStateCache Default;
            PipelineStateCache DefaultSkinned;
            PipelineStateCache Depth;
            PipelineStateCache DepthSkinned;
            PipelineStateCache Distortion;
            PipelineStateCache DistortionSkinned;
#if SE_EDITOR
            PipelineStateCache QuadOverdraw;
            PipelineStateCache QuadOverdrawSkinned;
#endif

            FORCE_INLINE PipelineStateCache* GetPS(const EnumFlags<DrawPass> pass, const bool useSkinning)
            {
                if (pass.Is(DrawPass::Depth))
                {
                    return useSkinning ? &DepthSkinned : &Depth;
                }
                else if (pass.Is(DrawPass::Depth))
                {
                    return useSkinning ? &DistortionSkinned : &Distortion;
                }
                else if (pass.Is(DrawPass::Forward))
                {
                    return useSkinning ? &DefaultSkinned : &Default;
                }
#if SE_EDITOR
                else if (pass.Is(DrawPass::QuadOverdraw))
                {
                    return useSkinning ? &QuadOverdrawSkinned : &QuadOverdraw;
                }
#endif

                return nullptr;
            }

            FORCE_INLINE void Release()
            {
                Default.Release();
                DefaultSkinned.Release();
                Depth.Release();
                DepthSkinned.Release();
                Distortion.Release();
                DistortionSkinned.Release();
            }
        };

    private:
        Cache _cache;
        Cache _cacheInstanced;
        EnumFlags<DrawPass> _drawModes = DrawPass::None;

    public:
        /// <summary>
        /// Init
        /// </summary>
        /// <param name="name">Material resource name</param>
        ForwardMaterialShader(const StringView& name)
            : MaterialShader(name)
        {
        }

    public:
        // [MaterialShader]
        EnumFlags<DrawPass> GetDrawModes() const override;
        bool CanUseInstancing(InstancingHandler& handler) const override;
        void Bind(BindParameters& params) override;
        void Unload() override;

    protected:
        // [MaterialShader]
        bool OnLoad() override;
    };
} // SE
