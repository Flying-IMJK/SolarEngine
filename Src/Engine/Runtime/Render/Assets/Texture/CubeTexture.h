#pragma once

#include "TextureBase.h"

namespace SE
{
	#define ASSET_VERSION_CUBETEXTURE 4

	/// <summary>
	/// Cube texture asset contains 6 images that is usually stored on a GPU as a cube map (one slice per each axis direction).
	/// </summary>
	SE_CLASS(Reflect, API, NoSpawn)
	class SE_API_RUNTIME CubeTexture : public TextureBase
	{
		SE_DEFINE_CLASS(CubeTexture, TextureBase)
		ASSET_HEADER(CubeTexture);
	public:
		uint32 GetSerializedVersion() const override;
	};

} // SE

