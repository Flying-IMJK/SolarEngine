
#include "FileItem.h"

#include "Editor/EditorIcons.h"

namespace SE::Editor
{
	FileItem::FileItem(StringView path)	: ContentItem(path)
	{
	}

	SpriteHandle FileItem::__GetDefaultThumbnail()
	{
		return EditorIcons::Document128;
	}
} // SE