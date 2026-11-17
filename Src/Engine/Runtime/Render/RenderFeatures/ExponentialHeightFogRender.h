#pragma once

#include <Runtime/Resource/AssetRef.h>
#include <Runtime/Resource/Assets/Materials/Shader.h>

namespace SE
{
    class GPUPipelineState;
    class GPUTextureView;
    struct RenderContext;

    /// Pipeline-owned, render-thread-only height fog. Programs are released before shader reload.
    class ExponentialHeightFogRender
    {
    public:
        ~ExponentialHeightFogRender();

        /// Blends into an existing HDR target without reading its color or writing scene depth.
        /// Skips absent fog and unavailable draw resources; atmosphere data must belong to this view.
        void Render(RenderContext& renderContext, GPUTextureView* pOutput);

    private:
        bool SetupResources(bool useAtmosphere);
        void OnShaderReloading(Asset* pAsset);

        AssetRef<Shader>       m_Shader;
        GPUPipelineState*      m_Pipelines[2] = {};
        ShaderProgramInstance* m_Programs[2]  = {};
    };
} // namespace SE
