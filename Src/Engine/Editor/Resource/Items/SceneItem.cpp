
#include "SceneItem.h"

#include "Editor/EditorIcons.h"
#include "../../../Runtime/Level/Scene/Scene.h"

namespace SE::Editor
{
	SceneItem::SceneItem(String path, UID id) : JsonAssetItem(path, id, Typeof<Scene>())
	{
	}

	ContentItemType SceneItem::__GetItemType()
	{
		return ContentItemType::Scene;
	}

	ContentItemSearchFilter SceneItem::__GetSearchFilter()
	{
		return ContentItemSearchFilter::Scene;
	}

	SpriteHandle SceneItem::__GetDefaultThumbnail()
	{
		return EditorIcons::Scene128;
	}
} // SE