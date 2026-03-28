
#include "CubeTexture.h"
#include "Runtime/Resource/Factories/BinaryAssetFactory.h"

namespace SE
{
	BINARY_ASSET_FACTORY(CubeTexture, false);

	CubeTexture::CubeTexture(const AssetInfo* info) : TextureBase(info)
	{
	}

	uint32 CubeTexture::GetSerializedVersion() const
	{
		return ASSET_VERSION_CUBETEXTURE;
	}

} // SE