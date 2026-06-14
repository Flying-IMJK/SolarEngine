#pragma once
#include "ContentItem.h"

namespace SE::Editor
{
	/// <summary>
	/// Content item for the auxiliary files.
	/// </summary>
	class FileItem : public ContentItem
	{
		SE_DEFINE_CLASS_DEFAULT(FileItem, ContentItem)
	public:
		/// <summary>
		/// Initializes a new instance of the <see cref="FileItem"/> class.
		/// </summary>
		/// <param name="path">The path to the file.</param>
		FileItem(StringView path);

	protected:
		ContentItemType __GetItemType() override { return ContentItemType::Other; }
		ContentItemSearchFilter __GetSearchFilter() override { return ContentItemSearchFilter::Other; }
		SpriteHandle __GetDefaultThumbnail() override;
	};

} // SE

