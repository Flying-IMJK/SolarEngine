

#include "AssetItem.h"

namespace SE::Editor
{
	AssetItem::TooltipDoubleClickHook::TooltipDoubleClickHook()
	{
		AnchorPreset = AnchorPresets::StretchAll;
		Offsets = Margin::Zero;
	}

	bool AssetItem::TooltipDoubleClickHook::OnMouseDoubleClick(Float2 location, MouseButton button)
	{
		return Item->OnMouseDoubleClick(Item->PointFromScreen(PointToScreen(location)), button);
	}

	bool AssetItem::GetIsLoaded()
	{
		Asset* asset = AssetContent::GetAsset(id);
		return asset != nullptr && asset->IsLoaded();
	}

	AssetItem::AssetItem(StringView path, TypeID typeID, UID id): ContentItem(path)
	{
		m_TypeID = typeID;
		m_ID = id;
	}

	Asset* AssetItem::LoadAsync()
	{
		return AssetContent::LoadAsync<Asset>(id);
	}

	void AssetItem::Reload()
	{
		Asset* asset = AssetContent::GetAsset(id);
		if (asset != nullptr && asset->IsLoaded())
		{
			asset->Reload();
		}
	}

	ContentItem* AssetItem::Find(UID id)
	{
		return id == id ? this : nullptr;
	}

	ContentItemType AssetItem::__GetItemType()
	{
		return ContentItemType::Asset;
	}
} // SE