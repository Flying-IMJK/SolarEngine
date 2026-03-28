#pragma once

#include "MaterialShader.h"

namespace SE
{

	/// <summary>
	/// Represents material that can be used to render GUI.
	/// </summary>
	class GUIMaterialShader : public MaterialShader
	{
	private:
		struct Cache
		{
			GPUPipelineState* Depth = nullptr;
			GPUPipelineState* NoDepth = nullptr;

			void Release();
		};

	private:
		Cache m_Cache;

	public:
		/// <summary>
		/// Init
		/// </summary>
		/// <param name="name">Material resource name</param>
		GUIMaterialShader(const StringView& name)
			: MaterialShader(name)
		{
		}

	public:
		// [MaterialShader]
		void Bind(BindParameters& params) override;
		void Unload() override;

	protected:
		// [MaterialShader]
		bool OnLoad() override;
	};

} // SE
