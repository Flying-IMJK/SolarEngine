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

			FORCE_INLINE PipelineStateCache* GetPS(const EnumFlags<DrawPass> pass)
			{
				if (pass.Is(DrawPass::Depth))
				{
					return &Depth;
				}

				if (pass.Is(DrawPass::GBuffer) ||
					pass == EnumFlags<DrawPass>{DrawPass::GBuffer, DrawPass::GlobalSurfaceAtlas} ||
					pass.Is(DrawPass::GlobalSurfaceAtlas) ||
					pass.Is(DrawPass::Forward))
				{
					return &Default;
				}

#if SE_EDITOR
				if (pass.Is(DrawPass::QuadOverdraw))
				{
					return &QuadOverdraw;
				}
#endif

				return nullptr;
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
		EnumFlags<DrawPass> m_DrawModes = DrawPass::None;

	public:
		DeformableMaterialShader(const StringView& name)
			: MaterialShader(name)
		{
		}

	public:
		// [MaterialShader]
		EnumFlags<DrawPass> GetDrawModes() const override;
		void Bind(BindParameters& params) override;
		void Unload() override;

	protected:
		// [MaterialShader]
		bool OnLoad() override;
	};
}