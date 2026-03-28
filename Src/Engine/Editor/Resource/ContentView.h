#pragma once
#include "Core/Logging/Exceptions/ArgumentNullException.h"
#include "Items/ContentItem.h"
#include "Runtime/UI/GUI/ContainerControl.h"

namespace SE::Editor
{
	enum class ContentViewType
	{
		/// <summary>
		/// The uniform tiles.
		/// </summary>
		Tiles,

		/// <summary>
		/// The vertical list.
		/// </summary>
		List,
	};

	/// <summary>
	/// The method sort for items.
	/// </summary>
	enum class SortType
	{
		/// <summary>
		/// The classic alphabetic sort method (A-Z).
		/// </summary>
		AlphabeticOrder,

		/// <summary>
		/// The reverse alphabetic sort method (Z-A).
		/// </summary>
		AlphabeticReverse
	};

	class ContentView : public ContainerControl, IContentItemOwner
	{
		SE_CLASS(ContentView, ContainerControl)
	private:
	    List<ContentItem*> _items = List<ContentItem*>(256);
	    List<ContentItem*> _selection = List<ContentItem*>();

	    float _viewScale = 1.0f;
	    ContentViewType _viewType = ContentViewType::Tiles;
	    bool _isRubberBandSpanning;
	    Float2 _mousePressLocation;
	    Rectangle _rubberBandRectangle;

	    bool _validDragOver;
	    // DragActors* _dragActors;

	public:
	    /// <summary>
	    /// Called when user wants to open the item.
	    /// </summary>
		Delegate<ContentItem*> OnOpen;

	    /// <summary>
	    /// Called when user wants to rename the item.
	    /// </summary>
		Delegate<ContentItem*> OnRename;

	    /// <summary>
	    /// Called when user wants to delete the item.
	    /// </summary>
		Delegate<List<ContentItem*>&> OnDelete;

	    /// <summary>
	    /// Called when user wants to paste the files/folders.
	    /// </summary>
		Delegate<List<String>&> OnPaste;

	    /// <summary>
	    /// Called when user wants to duplicate the item(s).
	    /// </summary>
		Delegate<List<ContentItem*>&> OnDuplicate;

	    /// <summary>
	    /// Called when user wants to navigate backward.
	    /// </summary>
		Action OnNavigateBack;

	    /// <summary>
	    /// Occurs when view scale gets changed.
	    /// </summary>
		Action ViewScaleChanged;

	    /// <summary>
	    /// Occurs when view type gets changed.
	    /// </summary>
		Action ViewTypeChanged;

	    /// <summary>
	    /// Gets the items.
	    /// </summary>
	    List<ContentItem*>& GetItems()
	    {
		    return _items;
	    }

	    /// <summary>
	    /// Gets the items count.
	    /// </summary>
		PRO_GET(ItemsCount, ContentView, int, __GetItemsCount);

	    /// <summary>
	    /// Gets the selected items.
	    /// </summary>
	    List<ContentItem*>& GetSelection()
	    {
		    return _selection;
	    };

	    /// <summary>
	    /// Gets the selected count.
	    /// </summary>
		PRO_GET(SelectedCount, ContentView, int, __GetSelectedCount);

	    /// <summary>
	    /// Gets a value indicating whether any item is selected.
	    /// </summary>
	    PRO_GET(HasSelection, ContentView, bool, __GetHasSelection);

	    /// <summary>
	    /// Gets or sets the view scale.
	    /// </summary>
		PRO(ViewScale, ContentView, float, __GetViewScale, __SetViewScale);

	    /// <summary>
	    /// Gets or sets the type of the view.
	    /// </summary>
	    PRO(ViewType, ContentView, ContentViewType, __GetViewType, __SetViewType);

	    /// <summary>
	    /// Flag is used to indicate if user is searching for items. Used to show a proper message to the user.
	    /// </summary>
	    bool IsSearching;

	    /// <summary>
	    /// Flag used to indicate whenever show full file names including extensions.
	    /// </summary>
	    bool ShowFileExtensions;

	    /// <summary>
	    /// The input actions collection to processed during user input.
	    /// </summary>
		// InputActionsContainer InputActions;

	    /// <summary>
	    /// Initializes a new instance of the <see cref="ContentView"/> class.
	    /// </summary>
		ContentView();

	    /// <summary>
	    /// Clears the items in the view.
	    /// </summary>
	    void ClearItems();

	    /// <summary>
	    /// Shows the items collection in the view.
	    /// </summary>
	    /// <param name="items">The items to show.</param>
	    /// <param name="sortType">The sort method for items.</param>
	    /// <param name="additive">If set to <c>true</c> items will be added to the current list. Otherwise items list will be cleared before.</param>
	    /// <param name="keepSelection">If set to <c>true</c> selected items list will be preserved. Otherwise selection will be cleared before.</param>
		void ShowItems(List<ContentItem*>& items, SortType sortType, bool additive = false, bool keepSelection = false);

	    /// <summary>
	    /// Determines whether the specified item is selected.
	    /// </summary>
	    /// <param name="item">The item.</param>
	    /// <returns><c>true</c> if the specified item is selected; otherwise, <c>false</c>.</returns>
		bool IsSelected(ContentItem* item);

	    /// <summary>
	    /// Clears the selected items collection.
	    /// </summary>
		void ClearSelection();

	    /// <summary>
	    /// Selects the specified items.
	    /// </summary>
	    /// <param name="items">The items.</param>
	    /// <param name="additive">If set to <c>true</c> items will be added to the current selection. Otherwise selection will be cleared before.</param>
		void Select(List<ContentItem*> &items, bool additive = false);

	    /// <summary>
	    /// Selects the specified item.
	    /// </summary>
	    /// <param name="item">The item.</param>
	    /// <param name="additive">If set to <c>true</c> item will be added to the current selection. Otherwise selection will be cleared before.</param>
		void Select(ContentItem* item, bool additive = false);

	    /// <summary>
	    /// Selects all the items.
	    /// </summary>
		void SelectAll();

	    /// <summary>
	    /// Deselects all the items.
	    /// </summary>
		void DeselectAll();

	    /// <summary>
	    /// Deselects the specified item.
	    /// </summary>
	    /// <param name="item">The item.</param>
		void Deselect(ContentItem* item);

	    /// <summary>
	    /// Duplicates the selected items.
	    /// </summary>
		void Duplicate();

	    /// <summary>
	    /// Copies the selected items (to the system clipboard).
	    /// </summary>
		void Copy();

	    /// <summary>
	    /// Returns true if user can paste data to the view (copied any files before).
	    /// </summary>
	    /// <returns>True if can paste files.</returns>
		bool CanPaste();

	    /// <summary>
	    /// Pastes the copied items (from the system clipboard).
	    /// </summary>
		void Paste();

	    /// <summary>
	    /// Gives focus and selects the first item in the view.
	    /// </summary>
		void SelectFirstItem();

	    /// <summary>
	    /// Refreshes thumbnails of all items in the <see cref="ContentView"/>.
	    /// </summary>
		void RefreshThumbnails();

	    /// <summary>
	    /// Called when user clicks on an item.
	    /// </summary>
	    /// <param name="item">The item.</param>
		void OnItemClick(ContentItem* item);

	    /// <summary>
	    /// Called when user wants to open item.
	    /// </summary>
	    /// <param name="item">The item.</param>
		void OnItemDoubleClick(ContentItem* item);

	    /// <inheritdoc />
	    void OnItemDeleted(ContentItem* item) override;

	    /// <inheritdoc />
	    void OnItemRenamed(ContentItem* item) override;

	    /// <inheritdoc />
	    void OnItemReimported(ContentItem* item) override;

	    /// <inheritdoc />
	    void OnItemDispose(ContentItem* item) override;

	    /// <inheritdoc />
	    void Draw() override;

	    /// <inheritdoc />
	    bool OnMouseDown(Float2 location, MouseButton button) override;

	    /// <inheritdoc />
	    void OnMouseMove(Float2 location) override;

	    /// <inheritdoc />
		bool OnMouseUp(Float2 location, MouseButton button) override;

	    /// <inheritdoc />
		bool OnMouseWheel(Float2 location, float delta) override;

	    /// <inheritdoc />
	    bool OnKeyDown(KeyboardKeys key) override;

	    /// <inheritdoc />
		bool OnCharInput(Char c) override;

	    /// <inheritdoc />
		DragDropEffect OnDragEnter(const Float2& location, DragData* data) override;

	    /// <inheritdoc />
		DragDropEffect OnDragMove(const Float2& location, DragData* data) override;

	    DragDropEffect OnDragDrop(const Float2& location, DragData* data) override;

	    /// <inheritdoc />
		void OnDragLeave() override;

		/// <inheritdoc />
		void OnDestroy() override;

	    /// <inheritdoc />
	protected:
		void PerformLayoutBeforeChildren() override;

	private:
		void BulkSelectUpdate(bool select = true);

		/*bool ValidateDragActors(ActorNode actor)
		{
			return actor.CanCreatePrefab && Editor.Instance.Windows.ContentWin.CurrentViewFolder.CanHaveAssets;
		}

		void ImportActors(DragActors actors, ContentFolder* location)
		{
			foreach (var actorNode in actors.Objects)
			{
				var actor = actorNode.Actor;
				if (actors.Objects.Contains(actorNode.ParentNode as ActorNode))
					continue;

				Editor.Instance.Prefabs.CreatePrefab(actor, false);
			}
		}*/

		ContentViewType __GetViewType() { return _viewType; }
		void __SetViewType(ContentViewType value);

		float __GetViewScale() { return _viewScale; }
		void __SetViewScale(float value);
		bool __GetHasSelection() { return _selection.Count() > 0; }
		int __GetSelectedCount() { return _selection.Count(); }
		int __GetItemsCount() { return _items.Count(); }
	};

} // SE

