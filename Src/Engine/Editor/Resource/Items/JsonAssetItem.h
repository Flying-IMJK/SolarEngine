#pragma once
#include "AssetItem.h"

namespace SE::Editor
{
	/// <summary>
	/// Asset item stored in a Json format file.
	/// </summary>
	class JsonAssetItem : public AssetItem
	{
		SE_DEFINE_CLASS(JsonAssetItem, AssetItem)

	protected:
		/// <summary>
		/// Asset icon.
		/// </summary>
		SpriteHandle _thumbnail;

	public:
		JsonAssetItem();

		/// <summary>
		/// Initializes a new instance of the <see cref="JsonAssetItem"/> class.
		/// </summary>
		/// <param name="path">The path.</param>
		/// <param name="id">The identifier.</param>
		/// <param name="typeId">Name of the resource type.</param>
		JsonAssetItem(String path, UID id, TypeID typeId);

		/// <summary>
		/// Initializes a new instance of the <see cref="JsonAssetItem"/> class.
		/// </summary>
		/// <param name="path">The path.</param>
		/// <param name="id">The identifier.</param>
		/// <param name="typeId">Name of the resource type.</param>
		/// <param name="thumbnail">Asset icon.</param>
		JsonAssetItem(String path, UID id, TypeID typeId, SpriteHandle thumbnail);

	protected:
		SpriteHandle __GetDefaultThumbnail() override;
		bool __GetHasDefaultThumbnail() override;

		ContentItemSearchFilter __GetSearchFilter() override;
	};

} // SE

