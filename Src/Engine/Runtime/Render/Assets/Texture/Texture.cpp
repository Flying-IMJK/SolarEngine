
#include "Texture.h"
#include "Runtime/Resource/Factories/BinaryAssetFactory.h"

namespace SE
{
	BINARY_ASSET_FACTORY(Texture, false);

	Texture::Texture(const SpawnParams& params, const AssetInfo* info): TextureBase(params, info)
	{

	};

	TextureFormatType Texture::GetFormatType() const
	{

		return _texture.GetFormatType();
	}

	bool Texture::IsNormalMap() const
	{
		return _texture.GetFormatType() == TextureFormatType::NormalMap;
	}

	uint32 Texture::GetSerializedVersion() const
	{
		return 4;
	}

} // SE