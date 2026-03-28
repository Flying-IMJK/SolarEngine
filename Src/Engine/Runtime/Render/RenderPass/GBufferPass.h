#pragma once

#include "Runtime/Render/RenderContext.h"
#include "Runtime/Render/RendererPass.h"
#include "Runtime/Render/Assets/Geometry/Model.h"
#include "Runtime/Resource/AssetRef.h"

namespace SE
{
	class GPUTexture;

	/// <summary>
	/// Structure that contains information about GBuffer for shaders.
	/// </summary>
	struct GBufferData
	{
		Float4 ViewInfo;
		Float4 ScreenSize;
		Float3 ViewPos;
		float ViewFar;
		Matrix InvViewMatrix;
		Matrix InvProjectionMatrix;
	};

	class GBufferPass final : public RendererPass<GBufferPass>
	{
	public:

		/// <summary>
		/// Set GBuffer inputs structure for given render task
		/// </summary>
		/// <param name="view">The rendering view.</param>
		/// <param name="gBuffer">GBuffer input to setup</param>
		static void SetInputs(const RenderView& view, GBufferData& gBuffer);

		void Execute(RenderContext& renderContext, GPUTexture* lightBuffer);

	private:
		void DrawSky(RenderContext& renderContext, GPUContext* context);

		AssetRef<Model> m_SkyModel;

	public:
		String ToString() const override;
		bool Init() override;
		void Dispose() override;

	protected:
		bool SetupResources() override;
	};

} // SE
