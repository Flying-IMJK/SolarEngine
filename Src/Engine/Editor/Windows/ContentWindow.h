#pragma once

#include "EditorWindow.h"
#include "Editor/GUI/ComboBox.h"

namespace SE
{
	class SplitPanel;
	class TextBox;
}

namespace SE::Editor
{
	class ContentOperate;
	class AssetItem;
	class ContentFolder;
	enum class ContentViewType;
	class TreeNode;
	class ContentTreeNode;
	enum class SortType;
	class Tree;
	class ToolStripButton;
	class ToolStrip;
	class ContentView;
	class RootContentTreeNode;
	class NavigationBar;
	class NewItem;
	class RenamePopup;

	/// <summary>
	/// One of the main editor windows used to present workspace content and user scripts.
	/// Provides various functionalities for asset operations.
	/// </summary>
	class ContentWindow : public EditorWindow
	{
	public:
		class ViewDropdown;

	private:
		const String ProjectDataLastViewedFolder = SE_TEXT("LastViewedFolder");
		bool _isWorkspaceDirty;
		String _workspaceRebuildLocation;
		SplitPanel* m_Split;
		Panel* m_ContentViewPanel;
		Panel* m_ContentTreePanel;
		ContentView* m_View;

		ToolStrip* m_ToolStrip;
		ToolStripButton* m_ImportButton;
		ToolStripButton* m_NavigateBackwardButton;
		ToolStripButton* m_NavigateForwardButton;
		ToolStripButton* m_NavigateUpButton;

		NavigationBar* m_NavigationBar;
		Tree* m_Tree;
		TextBox* m_FoldersSearchBox;
		TextBox* m_ItemsSearchBox;
		ViewDropdown* m_ViewDropdown;
		SortType _sortType;
		bool _showEngineFiles = true, _showPluginsFiles = true, _showAllFiles = true, _showGeneratedFiles = false;

		RootContentTreeNode* _root;

		bool m_NavigationUnlocked;
		List<ContentTreeNode*> m_NavigationUndo = List<ContentTreeNode*>(32);
		List<ContentTreeNode*> m_NavigationRedo = List<ContentTreeNode*>(32);

		NewItem* m_NewElement;

		static List<ContentTreeNode*> NavUpdateCache;

	public:

		ContentWindow();

		/// <summary>
		/// Refreshes the current view items collection.
		/// </summary>
		void RefreshView();

		/// <summary>
		/// Refreshes the view.
		/// </summary>
		/// <param name="target">The target location.</param>
		void RefreshView(ContentTreeNode* target);

        /// <summary>
        ///  Enables or disables vertical and horizontal scrolling on the content tree panel
        /// </summary>
        /// <param name="enabled">The state to set scrolling to</param>
		void ScrollingOnTreeView(bool enabled);

        /// <summary>
        ///  Enables or disables vertical and horizontal scrolling on the content view panel
        /// </summary>
        /// <param name="enabled">The state to set scrolling to</param>
		void ScrollingOnContentView(bool enabled);

        /// <summary>
        /// Shows popup dialog with UI to rename content item.
        /// </summary>
        /// <param name="item">The item to rename.</param>
        /// <returns>The created renaming popup.</returns>
		void Rename(ContentItem* item);

        /// <summary>
        /// Renames the specified item.
        /// </summary>
        /// <param name="item">The item.</param>
        /// <param name="newShortName">New name (without extension, just the filename).</param>
		void Rename(ContentItem* item, String& newShortName);

        /// <summary>
        /// Deletes the specified item. Asks user first and uses some GUI.
        /// </summary>
        /// <param name="item">The item to delete.</param>
		void Delete(ContentItem* item);

        /// <summary>
        /// Deletes the specified items. Asks user first and uses some GUI.
        /// </summary>
        /// <param name="items">The items to delete.</param>
		void Delete(List<ContentItem*>& items);

		/// <summary>
		/// Navigates backward.
		/// </summary>
		void NavigateBackward();

		/// <summary>
		/// Navigates forward.
		/// </summary>
		void NavigateForward();

		/// <summary>
		/// Navigates directory up.
		/// </summary>
		void NavigateUp();

		/// <summary>
		/// Clears the navigation history.
		/// </summary>
		void NavigationClearHistory();

		/// <summary>
		/// Gets the selected tree node.
		/// </summary>
		PRO_GET(SelectedNode, ContentWindow, ContentTreeNode*, __GetSelectedNode);

		/// <summary>
		/// Gets the current view folder.
		/// </summary>
		PRO_GET(CurrentViewFolder, ContentWindow, ContentFolder*, __GetCurrentViewFolder);

		/// <summary>
		/// Shows the root folder.
		/// </summary>
		void ShowRoot();

		/// <summary>
		/// Navigates to the specified target content location.
		/// </summary>
		/// <param name="target">The target.</param>
		void Navigate(ContentTreeNode* target);

		/// <summary>
		/// Selects the specified asset in the content view.
		/// </summary>
		/// <param name="asset">The asset to select.</param>
		void Select(Asset* asset);

		/// <summary>
		/// Selects the specified item in the content view.
		/// </summary>
		/// <param name="item">The item to select.</param>
		/// <param name="fastScroll">True of scroll to the item quickly without smoothing.</param>
		void Select(ContentItem* item, bool fastScroll = false);

		/// <summary>
		/// Opens the specified content item.
		/// </summary>
		/// <param name="item">The content item.</param>
		void Open(ContentItem* item);

        /// <summary>
        /// Clones the specified item.
        /// </summary>
        /// <param name="item">The item.</param>
		void Duplicate(ContentItem* item);

        /// <summary>
        /// Duplicates the specified items.
        /// </summary>
        /// <param name="items">The items.</param>
		void Duplicate(List<ContentItem*>& items);

		/// <summary>
		/// Pastes the specified files.
		/// </summary>
		/// <param name="files">The files paths to import.</param>
		void Paste(List<String>& files);

        /// <summary>
        /// Starts creating new item.
        /// </summary>
        /// <param name="proxy">The new item proxy.</param>
        /// <param name="argument">The argument passed to the proxy for the item creation. In most cases it is null.</param>
        /// <param name="created">The event called when the item is crated by the user. The argument is the new item.</param>
        /// <param name="initialName">The initial item name.</param>
        /// <param name="withRenaming">True if start initial item renaming by user, or tru to skip it.</param>
		void CreateItem(ContentOperate* proxy, Function<void(ContentItem*)> created = Function<void(ContentItem*)>(), String initialName = String::Empty, bool withRenaming = false, void* argument = nullptr);

		/// <summary>
		/// Clears the items searching query text and filters.
		/// </summary>
		void ClearItemsSearch();

		/// <inheritdoc />
        void OnInit() override;

        /// <inheritdoc />
		void Update(float deltaTime) override;

        /// <inheritdoc />
		void OnExit() override;

		bool OnMouseDown(Float2 location, MouseButton button) override;

		/// <inheritdoc />
		bool OnMouseUp(Float2 location, MouseButton button) override;

	public:
		/// <summary>
		/// Opens the specified item in dedicated editor window.
		/// </summary>
		/// <param name="item">The content item.</param>
		/// <param name="disableAutoShow">True if disable automatic window showing. Used during workspace layout loading to deserialize it faster.</param>
		/// <returns>Opened window or null if cannot open item.</returns>
		EditorWindow* ContentOpen(ContentItem* item, bool disableAutoShow = false);


	protected:
		void PerformLayoutBeforeChildren() override;

	private:

		void UpdateUI();
		
		ContextMenu* OnViewDropdownPopupCreate(ComboBox* comboBox);

		/*void OnOptionsChanged(EditorOptions options)
        {
            _split.Orientation = options.Interface.ContentWindowOrientation;

            RefreshView();
        }*/

		void UpdateItemsSearchFilter(ContentFolder* folder, List<ContentItem*> items, bool filters[], bool showAllFiles);

		void UpdateItemsSearchFilter(ContentFolder* folder, List<ContentItem*> items, bool filters[], bool showAllFiles, String& filterText);

		bool TryParseAssetId(String &text, AssetItem*& item);

		void OnFoldersSearchBoxTextChanged();

		void UpdateItemsSearch();

		void OnViewTypeButtonClicked(ContextMenuButton* button);

		void OnFilterClicked(ContextMenuButton* filterButton);

		void OnSortByButtonClicked(ContextMenuButton* button);

		void OnContentDatabaseItemRemoved(ContentItem* contentItem);

		void OnTreeSelectionChanged(List<TreeNode*> from, List<TreeNode*> to);

		bool OnRenameValidate(RenamePopup* popup, String& value);

		void OnRenameClosed(RenamePopup* popup);

		void OnTreeSelectionChanged(List<TreeNode*>& from, List<TreeNode*>& to);
		
		void Navigate(ContentTreeNode* source, ContentTreeNode* target);

		void UpdateNavigationBar();

		void RefreshViewItemsThumbnails();

		String GetClonedAssetPath(ContentItem* item);

		ContentFolder* __GetCurrentViewFolder();

		ContentTreeNode* __GetSelectedNode();

#pragma region ContextMenu
		void ShowContextMenuForItem(ContentItem* item, Float2& location, bool isTreeNode);

		void OnExpandAllClicked(ContextMenuButton* button);

		void OnCollapseAllClicked(ContextMenuButton* button);

#pragma endregion

	public:
		class ViewDropdown : public ComboBox
        {
			SE_DEFINE_CLASS_DEFAULT(ViewDropdown, ComboBox)
		public:
			void OnClicked(int index);

            /// <inheritdoc />
			void Draw() override;

            /// <inheritdoc />
			bool OnMouseUp(Float2 location, MouseButton button) override;
        };
	};
} // SE
