#pragma once
#include "Runtime/Core/Types/Property.h"
#include "Runtime/Render/2D/SpriteAtlas.h"
#include "Runtime/UI/GUI/Control.h"

namespace SE::Editor
{
	class ContextMenu;
	class ContentItem;
	class ContentFolder;


	enum class ContentItemType
	{
		/// <summary>
		/// The binary or text asset.
		/// </summary>
		Asset,

		/// <summary>
		/// The directory.
		/// </summary>
		Folder,

		/// <summary>
		/// The script file.
		/// </summary>
		Script,

		/// <summary>
		/// The scene file.
		/// </summary>
		Scene,

		/// <summary>
		/// The other type.
		/// </summary>
		Other,
	};

	/// <summary>
	/// Content item filter types used for searching.
	/// </summary>
	SE_ENUM(Reflect)
	enum class ContentItemSearchFilter
	{
		/// <summary>
		/// The model.
		/// </summary>
		Model,

		/// <summary>
		/// The skinned model.
		/// </summary>
		SkinnedModel,

		/// <summary>
		/// The material.
		/// </summary>
		Material,

		/// <summary>
		/// The texture.
		/// </summary>
		Texture,

		/// <summary>
		/// The scene.
		/// </summary>
		Scene,

		/// <summary>
		/// The prefab.
		/// </summary>
		Prefab,

		/// <summary>
		/// The script.
		/// </summary>
		Script,

		/// <summary>
		/// The audio.
		/// </summary>
		Audio,

		/// <summary>
		/// The animation.
		/// </summary>
		Animation,

		/// <summary>
		/// The json.
		/// </summary>
		Json,

		/// <summary>
		/// The particles.
		/// </summary>
		Particles,

		/// <summary>
		/// The shader source files.
		/// </summary>
		Shader,

		/// <summary>
		/// The other.
		/// </summary>
		Other,
	};

	class IContentItemOwner
	{
	public:
		/// <summary>
		/// Called when referenced item gets deleted (asset unloaded, file deleted, etc.).
		/// Item should not be used after that.
		/// </summary>
		/// <param name="item">The item.</param>
		virtual void OnItemDeleted(ContentItem* item) = 0;

		/// <summary>
		/// Called when referenced item gets renamed (filename change, path change, etc.)
		/// </summary>
		/// <param name="item">The item.</param>
		virtual void OnItemRenamed(ContentItem* item) = 0;

		/// <summary>
		/// Called when item gets reimported or reloaded.
		/// </summary>
		/// <param name="item">The item.</param>
		virtual void OnItemReimported(ContentItem* item) = 0;

		/// <summary>
		/// Called when referenced item gets disposed (editor closing, database internal changes, etc.).
		/// Item should not be used after that.
		/// </summary>
		/// <param name="item">The item.</param>
		virtual void OnItemDispose(ContentItem* item) = 0;
	};

	SE_CLASS(Reflect)
	class ContentItem : public Control
	{
		SE_DEFINE_CLASS_DEFAULT(ContentItem, Control)
	public:
	    /// <summary>
	    /// The default margin size.
	    /// </summary>
	    static constexpr int DefaultMarginSize = 4;

	    /// <summary>
	    /// The default text height.
	    /// </summary>
	    static constexpr int DefaultTextHeight = 24;

	    /// <summary>
	    /// The default thumbnail size.
	    /// </summary>
	    static constexpr int DefaultThumbnailSize = 64;//PreviewsCache.AssetIconSize;

	    /// <summary>
	    /// The default width.
	    /// </summary>
	    static constexpr int DefaultWidth = (DefaultThumbnailSize + 2 * DefaultMarginSize);

	    /// <summary>
	    /// The default height.
	    /// </summary>
	    static constexpr int DefaultHeight = (DefaultThumbnailSize + 2 * DefaultMarginSize + DefaultTextHeight);

	private:
	    ContentFolder* _parentFolder;

	    bool _isMouseDown;
	    Float2 _mouseDownStartPos;
	    List<IContentItemOwner*> _references = List<IContentItemOwner*>(4);

	    SpriteHandle _shadowIcon;

	public:
	    /// <summary>
	    /// Gets the type of the item.
	    /// </summary>
	    PRO_GET(ItemType, ContentItem, ContentItemType, __GetItemType);

	    /// <summary>
	    /// Gets the type of the item searching filter to use.
	    /// </summary>
		PRO_GET(SearchFilter, ContentItem, ContentItemSearchFilter, __GetSearchFilter);

	    /// <summary>
	    /// Gets a value indicating whether this instance is asset.
	    /// </summary>
		PRO_GET(IsAsset, ContentItem, bool, __GetIsAsset);

	    /// <summary>
	    /// Gets a value indicating whether this instance is folder.
	    /// </summary>
		PRO_GET(IsFolder, ContentItem, bool, __GetIsFolder);

	    /// <summary>
	    /// Gets a value indicating whether this instance can have children.
	    /// </summary>
		PRO_GET(CanHaveChildren, ContentItem, bool, __GetCanHaveChildren);

	    /// <summary>
	    /// Determines whether this item can be renamed.
	    /// </summary>
		PRO_GET(CanRename, ContentItem, bool, __GetCanRename);

	    /// <summary>
	    /// Gets a value indicating whether this item can be dragged and dropped.
	    /// </summary>
		PRO_GET(CanDrag, ContentItem, bool, __GetCanDrag);

	    /// <summary>
	    /// Gets a value indicating whether this <see cref="ContentItem"/> exists on drive.
	    /// </summary>
		PRO_GET(Exists, ContentItem, bool, __GetExists);

	    /// <summary>
	    /// Gets the parent folder.
	    /// </summary>
		PRO(ParentFolder, ContentItem, ContentFolder*, __GetParentFolder, __SetParentFolder);

	    /// <summary>
	    /// Gets the path to the item.
	    /// </summary>
		String Path;

	    /// <summary>
	    /// Gets the item file name (filename with extension).
	    /// </summary>
	    String FileName;

	    /// <summary>
	    /// Gets the item short name (filename without extension).
	    /// </summary>
	    String ShortName;

	    /// <summary>
	    /// Gets the asset name relative to the project root folder (without asset file extension)
	    /// </summary>
		String NamePath;// => FlaxEditor.Utilities.Utils.GetAssetNamePath(Path);

	    /// <summary>
	    /// Gets the content item type description (for UI).
	    /// </summary>
		// abstract string TypeDescription { get; }

	    /// <summary>
	    /// Gets the default name of the content item thumbnail. Returns null if not used.
	    /// </summary>
		PRO_GET(DefaultThumbnail, ContentItem, SpriteHandle, __GetDefaultThumbnail);

	    /// <summary>
	    /// Gets a value indicating whether this item has default thumbnail.
	    /// </summary>
		PRO_GET(HasDefaultThumbnail, ContentItem, bool, __GetHasDefaultThumbnail);

	    /// <summary>
	    /// Gets or sets the item thumbnail. Warning, thumbnail may not be available if item has no references (<see cref="ReferencesCount"/>).
	    /// </summary>
		SpriteHandle Thumbnail;

	    /// <summary>
	    /// True if force show file extension.
	    /// </summary>
		bool ShowFileExtension;

	    /// <summary>
	    /// Updates the item path. Use with caution or even don't use it. It's dangerous.
	    /// </summary>
	    /// <param name="value">The new path.</param>
		virtual void UpdatePath(String value);

	    /// <summary>
	    /// Refreshes the item thumbnail.
	    /// </summary>
		virtual void RefreshThumbnail();

	    /// <summary>
	    /// Updates the tooltip text text.
	    /// </summary>
		virtual void UpdateTooltipText();

	    /// <summary>
	    /// Tries to find the item at the specified path.
	    /// </summary>
	    /// <param name="path">The path.</param>
	    /// <returns>Found item or null if missing.</returns>
		virtual ContentItem* Find(String path);

	    /// <summary>
	    /// Tries to find a specified item in the assets tree.
	    /// </summary>
	    /// <param name="item">The item.</param>
	    /// <returns>True if has been found, otherwise false.</returns>
	    virtual bool Find(ContentItem* item);

	    /// <summary>
	    /// Tries to find the item with the specified id.
	    /// </summary>
	    /// <param name="id">The id.</param>
	    /// <returns>Found item or null if missing.</returns>
		virtual ContentItem* Find(UID id);

	    /// <summary>
	    /// Tries to find script with the given name.
	    /// </summary>
	    /// <param name="scriptName">Name of the script.</param>
	    /// <returns>Found script or null if missing.</returns>
	    /*public virtual ScriptItem FindScriptWitScriptName(string scriptName)
	    {
	        return null;
	    }*/

	    /// <summary>
	    /// Gets a value indicating whether draw item shadow.
	    /// </summary>
	    // protected virtual bool DrawShadow => false;

	    /// <summary>
	    /// Gets the local space rectangle for element name text area.
	    /// </summary>
	    PRO_GET(TextRectangle, ContentItem, Rectangle, __GetTextRectangle);

	    /// <summary>
	    /// Draws the item thumbnail.
	    /// </summary>
	    /// <param name="rectangle">The thumbnail rectangle.</param>
		void DrawThumbnail(Rectangle rectangle);

	    /// <summary>
	    /// Draws the item thumbnail.
	    /// </summary>
	    /// <param name="rectangle">The thumbnail rectangle.</param>
	    /// /// <param name="shadow">Whether or not to draw the shadow. Overrides DrawShadow.</param>
	    void DrawThumbnail(Rectangle rectangle, bool shadow);

	    /// <summary>
	    /// Gets the amount of references to that item.
	    /// </summary>
	    PRO_GET(ReferencesCount, ContentItem, int, __GetReferencesCount);

	    /// <summary>
	    /// Adds the reference to the item.
	    /// </summary>
	    /// <param name="obj">The object.</param>
		void AddReference(IContentItemOwner* obj);

	    /// <summary>
	    /// Removes the reference from the item.
	    /// </summary>
	    /// <param name="obj">The object.</param>
	    void RemoveReference(IContentItemOwner* obj);

	    /// <summary>
	    /// Called when context menu is being prepared to show. Can be used to add custom options.
	    /// </summary>
	    /// <param name="menu">The menu.</param>
		virtual void OnContextMenu(ContextMenu* menu)
	    {
	    }

	    /// <summary>
	    /// Called when item gets renamed or location gets changed (path modification).
	    /// </summary>
		virtual void OnPathChanged()
	    {
	    }

	    /// <summary>
	    /// Called when content item gets removed (by the user or externally).
	    /// </summary>
		virtual void OnDelete();


	    /// <inheritdoc />
		/*bool OnShowTooltip(String& text, Float2& location, Rectangle& area) override
	    {
	        UpdateTooltipText();
	        var result = Control::OnShowTooltip(out text, out _, out area);
	        location = Size * new Float2(0.9f, 0.5f);
	        return result;
	    }*/

	    /// <inheritdoc />
		void NavigationFocus() override;

	    /// <inheritdoc />
		void Draw() override;

	    /// <inheritdoc />
	    bool OnMouseDown(Float2 location, MouseButton button) override;

	    /// <inheritdoc />
		bool OnMouseUp(Float2 location, MouseButton button) override;

	    /// <inheritdoc />
	    bool OnMouseDoubleClick(Float2 location, MouseButton button) override;

	    /// <inheritdoc />
		void OnMouseMove(Float2 location) override;

	    /// <inheritdoc />
		void OnMouseLeave() override;

	    /// <inheritdoc />
		void OnSubmit() override;

	    /// <inheritdoc />
		int Compare(const Control* other) const override;

	    /// <inheritdoc />
		void OnDestroy() override;

	protected:
		/// <summary>
		/// Initializes a new instance of the <see cref="ContentItem"/> class.
		/// </summary>
		/// <param name="path">The path to the item.</param>
		ContentItem(StringView path);


		/// <summary>
		/// Called when item parent folder gets changed.
		/// </summary>
		virtual void OnParentFolderChanged()
		{
		}

		/// <summary>
		/// Requests the thumbnail.
		/// </summary>
		void RequestThumbnail();

		/// <summary>
		/// Releases the thumbnail.
		/// </summary>
		void ReleaseThumbnail();

		/// <summary>
		/// Called when item gets reimported or reloaded.
		/// </summary>
		virtual void OnReimport();

		/// <summary>
		/// Does the drag and drop operation with this asset.
		/// </summary>
		virtual void DoDrag();

		/// <inheritdoc />
		bool ShowTooltip() override { return true; }

		/// <summary>
		/// Called when building tooltip text.
		/// </summary>
		/// <param name="sb">The output string builder.</param>
		virtual void OnBuildTooltipText(StringBuilder sb);

		virtual ContentItemType __GetItemType() = 0;
		virtual bool __GetCanRename();
		virtual bool __GetCanDrag();
		virtual bool __GetExists();
		virtual ContentItemSearchFilter __GetSearchFilter() = 0;
		virtual SpriteHandle __GetDefaultThumbnail();
		virtual bool __GetHasDefaultThumbnail();
	private:

		bool __GetIsAsset()
		{
			return ItemType == ContentItemType::Asset;
		}

		bool __GetIsFolder()
		{
			return ItemType == ContentItemType::Folder;
		}

		bool __GetCanHaveChildren()
		{
			return ItemType == ContentItemType::Folder;
		}

		int __GetReferencesCount(){ return _references.Count(); }

		Rectangle __GetTextRectangle();

		ContentFolder* __GetParentFolder();

		void __SetParentFolder(ContentFolder* value);
	};

} // SE

