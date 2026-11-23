#pragma once
#include <Runtime/Core/Math/Color.h>
#include <Runtime/Graphics/Base/GPUPipelineState.h>
#include <Runtime/Graphics/Base/PixelFormat.h>
#include <Runtime/Render/RenderContext.h>
#include <Runtime/Render/RendererPass.h>
#include <Runtime/Resource/AssetRef.h>
#include <Runtime/Resource/Assets/Materials/Shader.h>

namespace SE
{
    enum class ToneMappingMode;
    class Shader;

    /// <summary>
    /// Bakes scene-linear color grading, tone mapping and SDR output encoding into a combined LUT.
    /// </summary>
    class ColorGradingPass : public RendererPass<ColorGradingPass>
	{
	private:
		bool m_UseVolumeTexture;
		PixelFormat m_LutFormat;
        AssetRef<Shader>       m_Shader;
        GPUPipelineState*  m_PipelineState   = nullptr;
        ShaderProgramInstance* m_ProgramInstance = nullptr;
        GPUConstantBuffer*     m_GPUConstantBuffer = nullptr;

    public:

		/// <summary>
		/// Init
		/// </summary>
		ColorGradingPass();

        /// <summary>
        /// Performs Look Up Table rendering for the input task.
        /// </summary>
        /// <param name="renderContext">The rendering context.</param>
        /// <param name="screenFadeColor">Linear overlay RGB and blending weight. Transparent disables the overlay.</param>
        /// <returns>Temporary LUT with a submitted draw, or null on failure. Called on the render thread;
        /// the caller returns it to the pool after its final use is submitted. This does not wait for the GPU.</returns>
        GPUTexture* RenderLUT(RenderContext& renderContext, const Color& screenFadeColor);

    private:
        void OnShaderReloading(Asset* pAsset);

        bool UpdateProgram(ToneMappingMode mappingMode, bool useLdrLut);

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


