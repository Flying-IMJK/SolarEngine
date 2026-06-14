#pragma once

#include "TextureBase.h"

namespace SE
{
	#define ASSET_VERSION_TEXTURE 4

	SE_CLASS(Reflect, API, NoSpawn)
	class SE_API_RUNTIME Texture : public TextureBase
	{
		SE_DEFINE_CLASS_DEFAULT(Texture, TextureBase);
		ASSET_HEADER(Texture);

	public:

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