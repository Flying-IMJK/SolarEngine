#pragma once

#include "TextureBase.h"

namespace SE
{
	#define ASSET_VERSION_CUBETEXTURE 4

	/// <summary>
	/// Cube texture asset contains 6 images that is usually stored on a GPU as a cube map (one slice per each axis direction).
	/// </summary>
	class SE_API_RUNTIME CubeTexture : public TextureBase
	{
		SE_CLASS_DEFAULT(CubeTexture, TextureBase)

	public:
		CubeTexture(const AssetInfo* info);

		uint32 GetSerializedVersion() const override;
	};

} // SE

