#pragma once
#include "Runtime/Graphics/Base/GPUPipelineStatePermutations.h"
#include "Runtime/Graphics/Base/PixelFormat.h"
#include "Runtime/Render/RenderContext.h"
#include "Runtime/Render/RendererPass.h"
#include "Runtime/Resource/AssetRef.h"
#include "Runtime/Resource/Assets/Materials/Shader.h"

namespace SE
{
	/// <summary>
	/// Color Grading and Tone Mapping rendering service. Generates HDR LUT for PostFx pass.
	/// </summary>
	class ColorGradingPass : public RendererPass<ColorGradingPass>
	{
	private:
		bool m_UseVolumeTexture;
		PixelFormat m_LutFormat;
		AssetRef<Shader> m_Shader;
		GPUPipelineStatePermutationsPs<3> m_PsLut;

	public:

		/// <summary>
		/// Init
		/// </summary>
		ColorGradingPass();

	public:

		/// <summary>
		/// Performs Look Up Table rendering for the input task.
		/// </summary>
		/// <param name="renderContext">The rendering context.</param>
		/// <returns>Allocated temp render target with a rendered LUT. Can be 2d or 3d based on current graphics hardware caps. Release after usage.</returns>
		GPUTexture* RenderLUT(RenderContext& renderContext);

	private:

#if COMPILE_WITH_DEV_ENV
		void OnShaderReloading(Asset* obj)
		{
			_psLut.Release();
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
}


