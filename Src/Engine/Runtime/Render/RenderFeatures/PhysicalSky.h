#pragma once

#include <Runtime/Resource/AssetRef.h>
#include <Runtime/Resource/Assets/Materials/Shader.h>

namespace SE
{
    class GPUPipelineState;
    class GPUSampler;
    struct RenderContext;

    // Owned by the rendering pipeline and used only on its render thread.
    class PhysicalSky
    {
    public:
        ~PhysicalSky();

        // The caller binds the GBuffer MRTs and scene depth. Draws only far-depth pixels.
        // Skips drawing when this view has no submitted LUTs or required resources/bindings are not ready.
        void Render(RenderContext& renderContext);

    private:
        bool SetupResources();
        void OnShaderReloading(Asset* pAsset);

        AssetRef<Shader>        m_Shader;
        GPUPipelineState*      m_Pipeline = nullptr;
        ShaderProgramInstance* m_Program = nullptr;
        GPUSampler*            m_Sampler = nullptr;
    };
} // namespace SE
