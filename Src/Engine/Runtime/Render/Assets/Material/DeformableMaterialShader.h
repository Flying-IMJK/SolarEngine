#pragma once

#include "MaterialShader.h"

namespace SE
{
	/// <summary>
	/// Represents material that can be used to render objects that can be deformed.
	/// </summary>
	class DeformableMaterialShader : public MaterialShader
	{
	private:
		struct Cache
		{
			PipelineStateCache Default;
			PipelineStateCache Depth;
#if SE_EDITOR
			PipelineStateCache QuadOverdraw;
#endif

			FORCE_INLINE PipelineStateCache* GetPS(const DrawPass pass)
			{
				switch (pass)
				{
				case DrawPass::Depth:
					return &Depth;
				case DrawPass::GBuffer:
				case DrawPass::GBuffer | DrawPass::GlobalSurfaceAtlas:
				case DrawPass::GlobalSurfaceAtlas:
				case DrawPass::Forward:
					return &Default;
/*				case DrawPass::Distortion:
					return &Distortion;*/
	#if USE_EDITOR
				case DrawPass::QuadOverdraw:
					return &QuadOverdraw;
	#endif
				default:
					return nullptr;
				}
			}

			FORCE_INLINE void Release()
			{
				Default.Release();
				Depth.Release();
#if SE_EDITOR
				QuadOverdraw.Release();
#endif
			}
		};

	private:
		Cache m_Cache;
        DrawPass m_DrawModes = DrawPass::None;

	public:
		DeformableMaterialShader(const StringView& name)
			: MaterialShader(name)
		{
		}

	public:
		// [MaterialShader]
		DrawPass GetDrawModes() const override;
		void Bind(BindParameters& params) override;
		void Unload() override;

	protected:
		// [MaterialShader]
		bool OnLoad() override;
	};
}
