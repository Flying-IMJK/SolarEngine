
#include "JsonAssetItem.h"

#include "Editor/EditorIcons.h"

namespace SE::Editor
{
	JsonAssetItem::JsonAssetItem()
	{
		_thumbnail = EditorIcons::Json128;
	}

	JsonAssetItem::JsonAssetItem(String path, UID id, TypeID typeId) : AssetItem(path, typeId, id)
	{
		_thumbnail = EditorIcons::Json128;
	}

	JsonAssetItem::JsonAssetItem(String path, UID id, TypeID typeId, SpriteHandle thumbnail) : AssetItem(path, typeId, id)
	{
		_thumbnail = thumbnail;
	}

	SpriteHandle JsonAssetItem::__GetDefaultThumbnail()
	{
		return _thumbnail;
	}

	bool JsonAssetItem::__GetHasDefaultThumbnail()
	{
		return true;
	}

	ContentItemSearchFilter JsonAssetItem::__GetSearchFilter()
	{
		return ContentItemSearchFilter::Json;
	}
} // SE