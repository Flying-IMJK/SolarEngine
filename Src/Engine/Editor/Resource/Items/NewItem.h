#pragma once
#include "ContentItem.h"

namespace SE::Editor
{
	class ContentOperate;

	/// <summary>
	/// Helper content item used to mock UI during creating new assets by <see cref="FlaxEditor.Windows.ContentWindow"/>.
	/// </summary>
	class NewItem : public ContentItem
	{
		SE_DEFINE_CLASS_DEFAULT(NewItem, ContentItem)
	public:
		/// <summary>
		/// Gets the proxy object related to the created asset.
		/// </summary>
		ContentOperate* Proxy;

		/// <summary>
		/// Gets the argument passed to the proxy for the item creation. In most cases it is null.
		/// </summary>
		void* Argument;

		/// <summary>
		/// Initializes a new instance of the <see cref="NewItem"/> class.
		/// </summary>
		/// <param name="path">The path for the new item.</param>
		/// <param name="proxy">The content proxy object.</param>
		/// <param name="arg">The argument passed to the proxy for the item creation. In most cases it is null.</param>
		NewItem(String path, ContentOperate* proxy, void* arg);

		/// <inheritdoc />
		// public override String TypeDescription => "New";

		/// <inheritdoc />
		// protected override bool DrawShadow => true;

		/// <inheritdoc />
		void UpdateTooltipText() override
		{
			// TooltipText = null;
		}

	protected:
		ContentItemType __GetItemType() override;
		ContentItemSearchFilter __GetSearchFilter() override;
		SpriteHandle __GetDefaultThumbnail() override;

	};

} // SE

