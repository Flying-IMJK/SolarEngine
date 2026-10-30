#pragma once

#include <Runtime/Graphics/Base/GPUPipelineState.h>
#include <Runtime/Render/Assets/Geometry/Model.h>
#include <Runtime/Render/RenderContext.h>
#include <Runtime/Render/RendererPass.h>
#include <Runtime/Resource/AssetRef.h>

namespace SE
{
    class GPUTextureView;
    class GraphicShader;
    class GPUConstantBuffer;
    class ShaderProgramInstance;

    class LightPass final : public RendererPass<LightPass>
    {
    private:
        AssetRef<GraphicShader> m_Shader;
        SLC2GPUPipelineState*   m_PsLightDir           = nullptr;
        SLC2GPUPipelineState*   m_PsLightPointNormal   = nullptr;
        SLC2GPUPipelineState*   m_PsLightPointInverted = nullptr;
        ShaderProgramInstance*  m_ProgramLightDir      = nullptr;
        ShaderProgramInstance*  m_ProgramLightPoint    = nullptr;
        GPUConstantBuffer*      m_PerLightBuffer       = nullptr;
        GPUConstantBuffer*      m_PerFrameBuffer       = nullptr;
        PixelFormat             m_ShadowMaskFormat;
        AssetRef<Model>         m_SphereModel;

    public:
        // Called on the render thread; GPU submissions consume the pass-owned constant buffers.
        void RenderLight(RenderContextBatch& renderContextBatch, GPUTextureView* pLightBuffer);

        String ToString() const override;
        bool   Init() override;
        void   Dispose() override;

    protected:
        bool SetupResources() override;

    private:
#if COMPILE_WITH_DEV_ENV
        void OnShaderReloading(Asset* pObj);
#endif
    };

} // namespace SE
