#pragma once

#include <Runtime/Render/RendererPass.h>
#include <Runtime/Resource/AssetRef.h>
#include <Runtime/Resource/Assets/Materials/Shader.h>

namespace SE
{
    struct RenderContext;
    class GPUTexture;

    /// GPU metering and adaptation. Exposure history belongs to each RenderBuffers.
    class AutoExposurePass : public RendererPass<AutoExposurePass>
    {
    public:
        String ToString() const override;
        bool   Init() override;
        void   Dispose() override;

        /// Queues metering and returns this frame's exposure texture, or null if not ready.
        /// Called on the render thread; no CPU readback or wait for GPU completion.
        GPUTexture* Render(RenderContext& renderContext, GPUTexture* pSceneColor);

    protected:
        bool SetupResources() override;

    private:
        void OnShaderReloading(Asset* pAsset);

        AssetRef<Shader>       m_Shader;
        ShaderProgramInstance* m_ResetProgram     = nullptr;
        ShaderProgramInstance* m_HistogramProgram = nullptr;
        ShaderProgramInstance* m_AdaptProgram     = nullptr;
    };
} // namespace SE
