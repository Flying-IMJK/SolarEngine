
#include "NewItem.h"

#include "Editor/EditorIcons.h"

namespace SE::Editor
{

	NewItem::NewItem(String path, ContentOperate* proxy, void* arg)	: ContentItem(path)
	{
		Proxy = proxy;
		Argument = arg;
	}

	ContentItemType NewItem::__GetItemType()
	{
		return ContentItemType::Other;
	}
	ContentItemSearchFilter NewItem::__GetSearchFilter()
	{
		return ContentItemSearchFilter::Other;
	}

	SpriteHandle NewItem::__GetDefaultThumbnail()
	{
		return EditorIcons::Document128;
	}
} // SE