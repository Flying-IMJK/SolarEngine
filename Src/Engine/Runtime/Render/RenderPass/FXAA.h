#pragma once

#include "Runtime/Graphics/Base/GPUPipelineStatePermutations.h"
#include "Runtime/Render/RenderContext.h"
#include "Runtime/Render/RenderEnum.h"
#include "Runtime/Render/RendererPass.h"
#include "Runtime/Resource/AssetRef.h"

namespace SE
{
    class GraphicShader;
    class GPUTextureView;

	/// <summary>
	/// Fast-Approximate Anti-Aliasing effect.
	/// </summary>
	class FXAA final : public RendererPass<FXAA>
	{
	private:
		AssetRef<GraphicShader> m_Shader;
        SLC2GPUPipelineState* m_PipelineState;
        ShaderProgramInstance*  m_ProgramInstance;

	public:
		/// <summary>
		/// Performs AA pass rendering for the input task.
		/// </summary>
		/// <param name="renderContext">The rendering context.</param>
		/// <param name="input">The source render target.</param>
		/// <param name="output">The result render target.</param>
		void Render(RenderContext& renderContext, GPUTexture* input, GPUTextureView* output);

	private:
#if COMPILE_WITH_DEV_ENV
		void OnShaderReloading(Asset* obj)
		{
			_psFXAA.Release();
			invalidateResources();
		}
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

