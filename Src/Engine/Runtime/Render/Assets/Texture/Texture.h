#pragma once

#include "TextureBase.h"

namespace SE
{
	#define ASSET_VERSION_TEXTURE 4

	class SE_API_RUNTIME Texture : public TextureBase
	{
		SE_CLASS_DEFAULT(Texture, TextureBase);

	public:
		Texture(const AssetInfo* info);

		/// <summary>
		/// Gets the texture format type.
		/// </summary>
		TextureFormatType GetFormatType() const;

		/// <summary>
		/// Returns true if texture is a normal map.
		/// </summary>
		bool IsNormalMap() const;

		uint32 GetSerializedVersion() const override;

	};

} // SE