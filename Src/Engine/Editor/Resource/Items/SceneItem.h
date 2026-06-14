#pragma once
#include "JsonAssetItem.h"

namespace SE::Editor
{
	/// <summary>
	/// Content item that contains data.
	/// </summary>
	class SceneItem : public JsonAssetItem
	{
		SE_DEFINE_CLASS(SceneItem, JsonAssetItem)
	public:
		/// <summary>
		/// Initializes a new instance of the <see cref="SceneItem"/> class.
		/// </summary>
		/// <param name="path">The asset path.</param>
		/// <param name="id">The asset identifier.</param>
		SceneItem(String path, UID id);

	protected:
		ContentItemType __GetItemType() override;

		ContentItemSearchFilter __GetSearchFilter() override;

		SpriteHandle __GetDefaultThumbnail() override;

		/*/// <inheritdoc />
		public override ContentItemType ItemType => ContentItemType.Scene;

		/// <inheritdoc />
		public override ContentItemSearchFilter SearchFilter => ContentItemSearchFilter.Scene;

		/// <inheritdoc />
		public override string TypeDescription => "Scene";

		/// <inheritdoc />
		public override SpriteHandle DefaultThumbnail => Editor.Instance.Icons.Scene128;

		/// <inheritdoc />
		public override bool IsOfType(Type type)
		{
			return type.IsAssignableFrom(typeof(SceneAsset));
		}*/
	};

} // SE

