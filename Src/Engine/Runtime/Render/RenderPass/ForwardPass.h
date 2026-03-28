#pragma once

#include "Runtime/Render/RendererPass.h"
#include "Runtime/Resource/AssetRef.h"
#include "Runtime/Resource/Assets/Materials/Shader.h"

namespace SE
{
    struct RenderContext;
    class GPUPipelineState;
    class GPUTexture;

    class ForwardPass : public RendererPass<ForwardPass>
    {
    private:

        AssetRef<Shader> _shader;
        GPUPipelineState* _psApplyDistortion;

    public:
        /// <summary>
        /// Initializes a new instance of the <see cref="ForwardPass"/> class.
        /// </summary>
        ForwardPass();
    
        /// <summary>
        /// Performs forward pass rendering for the input task. Renders transparent objects.
        /// </summary>
        /// <param name="renderContext">The rendering context.</param>
        /// <param name="input">Target with renderer frame ready for further processing.</param>
        /// <param name="output">The output frame.</param>
        void Render(RenderContext& renderContext, GPUTexture* input, GPUTexture* output);

        bool Init() override;

        void Dispose() override;
        String ToString() const override;

    protected:
         
        bool SetupResources() override;
    };
}