#pragma once

#include "Runtime/Render/RenderContext.h"
#include "Runtime/Render/RendererPass.h"
#include "Runtime/Render/Assets/Geometry/Model.h"
#include "Runtime/Resource/AssetRef.h"
#include "Runtime/Resource/Assets/Materials/Shader.h"
#include "Runtime/Graphics/Base/GPUPipelineStatePermutations.h"

namespace SE
{
	class GPUTextureView;

	class LightPass final : public RendererPass<LightPass>
	{
	private:
		AssetRef<Shader> m_Shader;
		GPUPipelineStatePermutationsPs<2> m_PsLightDir;
		GPUPipelineStatePermutationsPs<4> m_PsLightPointNormal;
		GPUPipelineStatePermutationsPs<4> m_PsLightPointInverted;
		PixelFormat m_ShadowMaskFormat;
		AssetRef<Model> m_SphereModel;

	public:
		void RenderLight(RenderContextBatch& renderContextBatch, GPUTextureView* lightBuffer);

		String ToString() const override;
		bool Init() override;
		void Dispose() override;

	protected:
		bool SetupResources() override;
	};

} // SE
