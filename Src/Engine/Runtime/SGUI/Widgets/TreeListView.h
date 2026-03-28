#pragma once

#include "Runtime/API.h"
#include "Runtime/SGUI/GUILayout.h"
#include "Core/Types/Delegate.h"
#include "Core/Types/Strings/StringID.h"
#include "Core/Types/BitFlags.h"

//-------------------------------------------------------------------------

namespace SE::GUI
{
    //-------------------------------------------------------------------------
    // Tree List Item
    //-------------------------------------------------------------------------

    class SE_API_RUNTIME TreeListViewItem
    {
        friend class TreeListView;

    public:
        enum ItemState
        {
            None,
            Selected,
            Active
        };

    public:
        TreeListViewItem(TreeListViewItem *pParent) : m_pParent(pParent) {}
        virtual ~TreeListViewItem() = default;

        //-------------------------------------------------------------------------

        // The unique ID is need to be able to ID, record and restore tree state
        virtual uint64 GetUniqueID() const = 0;

        // The name ID is the name of the item relative to its parent. This is not guaranteed to be unique per item
        // Used by default for sorting
        virtual StringView GetName() const = 0;

        // Does this item have a context menu?
        virtual bool HasContextMenu() const { return false; }

        // Can this item be set as the active item (note: this is different from the selected item)
        virtual bool IsActivatable() const { return false; }

        // Sort comparator function
        virtual bool Compare(TreeListViewItem const *pItem) const;

        // Signals
        //-------------------------------------------------------------------------

        // Called whenever this item is activated/deactivated
        virtual void OnActivationStateChanged() {}

        // Called whenever this item is selected/unselected
        virtual void OnSelectionStateChanged() {}

        // Called whenever we double click an item - return true if you require a visual tree rebuild
        virtual bool OnDoubleClick() { return false; }

        // Called whenever we hit enter on a selected item - return true if you require a visual tree rebuild
        virtual bool OnEnterPressed() { return false; }

        // Visuals
        //-------------------------------------------------------------------------

        // The friendly display name printed in the UI (generally the same as the nameID)
        // This is separate from the name since we might want to add an icon or other decoration to the display name without changing the name
        // The return by value is intentional and makes a bunch of operations easier/safer but should be changed if it becomes a perf issue
        virtual String GetDisplayName() const
        {
            String const nameID = GetName();
            return !nameID.IsEmpty() ? nameID : String("!!! Invalid Name !!!");
        }

        virtual String GetDisplayIcon() const
        {
            return SE_TEXT("");
        }

        // The color that the display name should be printed in
        virtual Color GetDisplayColor(ItemState state) const;

        // Is this a header item (i.e. should be framed)
        virtual bool IsHeader() const { return false; }

        // Get the tooltip for this item if it has one
        virtual Char const *GetTooltipText() const { return nullptr; }

        // Expansion
        //-------------------------------------------------------------------------

        // Is this item a leaf node (i.e., should we draw the expansion arrow)
        virtual bool IsLeaf() const { return m_children.IsEmpty(); }

        inline void SetExpanded(bool isExpanded, bool applyToChildren = false)
        {
            m_isExpanded = isExpanded;

            if (applyToChildren)
            {
                for (auto pChild : m_children)
                {
                    pChild->SetExpanded(isExpanded, applyToChildren);
                }
            }
        }

        inline bool IsExpanded() const { return m_isExpanded; }

        // Visibility
        //-------------------------------------------------------------------------

        inline bool IsVisible() const { return m_isVisible; }

        inline bool HasVisibleChildren() const { return !m_children.IsEmpty(); }

        // Update visibility for this branch based on a user-supplied delegate
        void UpdateVisibility(Function<bool(TreeListViewItem * const&)> const &isVisibleFunction, bool showParentItemsWithNoVisibleChildren = false);

        // Drag and drop
        //-------------------------------------------------------------------------

        // Can we be dragged somewhere?
        virtual bool IsDragAndDropSource() const { return false; }

        // Are we a valid target for drag and drop operations?
        virtual bool IsDragAndDropTarget() const { return false; }

        // Set the ImGui payload data for the drag and drop operation
        virtual void SetDragAndDropPayloadData() const {}

        // Hierarchy
        //-------------------------------------------------------------------------

        bool HasChildren() const { return !m_children.IsEmpty(); }
        List<TreeListViewItem *>  &GetChildren() { return m_children; }

        // Destroy specific child - be careful when calling this, you need to make sure the visual tree is kept in sync
        void DestroyChild(uint64 uniqueItemID);

        // Destroys all child for this branch - be careful when calling this, you need to make sure the visual tree is kept in sync
        void DestroyChildren();

        // Find a specific child
        TreeListViewItem const *FindChild(uint64 uniqueID) const;

        // Find a specific child
        TreeListViewItem *FindChild(uint64 uniqueID) { return const_cast<TreeListViewItem *>(const_cast<TreeListViewItem const *>(this)->FindChild(uniqueID)); }

        // Find a specific child
        TreeListViewItem *FindChild(Function<bool(TreeListViewItem * const&)> const &searchPredicate);

        // Recursively search all children
        TreeListViewItem *SearchChildren(Function<bool(TreeListViewItem * const&)> const &searchPredicate);

        // Apply some operation for all elements in this branch
        void ForEachChild(Function<void(TreeListViewItem *)> const &function)
        {
            for (auto &pChild : m_children)
            {
                function(pChild);
                pChild->ForEachChild(function);
            }
        }

        // Apply some operation for all elements in this branch
        void ForEachChildConst(Function<void(TreeListViewItem const *)> const &function) const
        {
            for (auto &pChild : m_children)
            {
                function(pChild);
                pChild->ForEachChildConst(function);
            }
        }

        // Sorts the children of this item, Note: this will not be reflected into the tree view unless you rebuild the visual tree
        void SortChildren();

    private:
        // Disable copies/moves
        TreeListViewItem &operator=(TreeListViewItem const &) = delete;
        TreeListViewItem &operator=(TreeListViewItem &&) = delete;

    protected:
        TreeListViewItem *m_pParent = nullptr;
        List<TreeListViewItem *> m_children;
        bool m_isVisible = true;
        bool m_isExpanded = false;
        bool m_isActivated = false;
        bool m_isSelected = false;
    };

    //-------------------------------------------------------------------------
    // Tree List View Context
    //-------------------------------------------------------------------------

    struct TreeListViewContext
    {
        // This function is called to rebuild the tree, you are expected to fill the root item
		Function<void(TreeListViewItem * /*rootItem*/)> rebuildTree;

        // 当对目标项进行拖放操作时调用
		Function<void(TreeListViewItem * /*dropTarget*/)> onDragAndDrop;

        // 为列设置额外的列头
		Function<void()> m_setupExtraColumnHeaders;

        // 绘制给定项可能需要的任何自定义项控件
        Function<void(TreeListViewItem * /*pBaseItem*/, int32 /*extraColumnIdx*/)> drawItemExtraColumns;

        // 绘制可能需要的任何自定义项上下文菜单
		Function<void(List<TreeListViewItem *> const & /*items*/)> drawItemContextMenu;

        // 要绘制的额外列数
        int32 numExtraColumns = 0;
    };

    //-------------------------------------------------------------------------
    // Tree List View
    //-------------------------------------------------------------------------

    class SE_API_RUNTIME TreeListView
    {
    protected:
        class TreeRootItem final : public TreeListViewItem
        {
        public:
            TreeRootItem() : TreeListViewItem(nullptr)
            {
                m_isExpanded = true;
            }

			StringView GetName() const override
			{
				return m_ID.ToString();
			}

			uint64 GetUniqueID() const override
			{
				return 0;
			}

        private:
            StringID m_ID = StringID(SE_TEXT("Root"));
        };

        enum class VisualTreeState
        {
            UpToDate,
            NeedsRebuild,
            NeedsRebuildAndFocusSelection
        };

        struct VisualTreeItem
        {
            VisualTreeItem() = default;

            VisualTreeItem(TreeListViewItem *pItem, int32 hierarchyLevel)
                : pItem(pItem), hierarchyLevel(hierarchyLevel)
            {
                ENGINE_ASSERT(pItem != nullptr && hierarchyLevel >= 0);

                pItem->ForEachChild(Function<void(TreeListViewItem *)>([this](TreeListViewItem * pChildItem){
                    if (!pChildItem->IsLeaf())
                    {
                        childHasBranch = true;
                    }
                }));
            }

        public:
            TreeListViewItem *pItem = nullptr;
            int32 hierarchyLevel = -1;
            bool childHasBranch = false;
        };

    public:
        enum Flags
        {
            ShowBranchesFirst = 1 << 0,
            MultiSelectionAllowed = 1 << 2,
            DrawRowBackground = 1 << 3,  // 绘制奇偶行不同颜色
            DrawBorders = 1 << 4,        // 绘制边框
            OnlyShowBranch = 1 << 5,     // 只绘制分支
            UseSmallFont = 1 << 6,
            ShowBulletsOnLeaves = 1 << 7,
            SortTree = 1 << 8,       // Sort all items in the tree
            TrackSelection = 1 << 9, // Update the viewed items to match the current selection
        };

    public:
        TreeListView(EnumFlags<Flags> flags = EnumFlags<Flags>(ShowBranchesFirst, DrawRowBackground, DrawBorders, UseSmallFont)) : m_flags(flags)
        {
        }

        template <typename... Args, class Enable = std::enable_if_t<(... && std::is_convertible_v<Args, Flags>)>>
        TreeListView(Args &&...args) : m_flags(args...)
        {
        }

        ~TreeListView();

        // Rebuilds the tree's internal data
        void RebuildTree(TreeListViewContext& context, bool maintainSelection = false);

        // Returns true if the selection has changed
        bool UpdateAndDraw(TreeListViewContext& context, float listHeight = 0.0f);

        // Visual
        //-------------------------------------------------------------------------

        // Refreshes the visual tree and reflects any changes to expansion, visibility or sorting
        inline void RefreshVisualState() { m_visualTreeState = VisualTreeState::NeedsRebuild; }

        // Check if we are currently drawing the tree - useful to assert about when certain external operations are performed
        inline bool IsCurrentlyDrawingTree() const { return m_isDrawingTree; }

        // Set option flag
        void SetFlag(Flags flag, bool value = true) { value ? m_flags.SetFlag(flag) : m_flags.RemoveFlag(flag); }

        // Set all option flags
        void SetFlags(EnumFlags<Flags> flags) { m_flags = flags; }

        // Activation
        //-------------------------------------------------------------------------

        // Do we have anything selected?
        inline bool HasActiveItem() const { return m_pActiveItem != nullptr; }

        // Clear active item
        inline void ClearActiveItem() { m_pActiveItem = nullptr; }

        // Get the currently active item
        inline TreeListViewItem *GetActiveItem() const { return m_pActiveItem; }

        // Selection
        //-------------------------------------------------------------------------

        // Do we have anything selected?
        inline bool HasSelection() const { return !m_selection.IsEmpty(); }

        // Clear the current selection
        void ClearSelection();

        // Get the current selection
        inline List<TreeListViewItem *> const &GetSelection() const { return m_selection; }

        // Set the view to the selected item
        void SetViewToSelection();

        // Find an item based on its unique ID
        TreeListViewItem *FindItem(uint64 uniqueID);

        // Find an item based on its unique ID
        inline TreeListViewItem const *FindItem(uint64 uniqueID) const { return const_cast<TreeListView *>(this)->FindItem(uniqueID); }

        // Is this item selected?
        inline bool IsItemSelected(uint64 uniqueID) const
        {
            auto pItem = FindItem(uniqueID);
            return pItem == nullptr ? false : pItem->m_isSelected;
        }

        // Set the selection to a single item - Notification will only be fired if the selection actually changes
        inline void SetSelection(TreeListViewItem *pItem) { SetSelectionInternal(pItem, m_flags.IsFlag(TrackSelection)); }

        // Add to the current selection - Notification will only be fired if the selection actually changes
        inline void AddToSelection(TreeListViewItem *pItem) { AddToSelectionInternal(pItem, m_flags.IsFlag(TrackSelection)); }

        // Add an item range to the selection - Notification will only be fired if the selection actually changes
        inline void AddToSelection(List<TreeListViewItem *> const &itemRange) { AddToSelectionInternal(itemRange, m_flags.IsFlag(TrackSelection)); }

        // Add an item range to the selection - Notification will only be fired if the selection actually changes
        inline void SetSelection(List<TreeListViewItem *> const &itemRange) { SetSelectionInternal(itemRange, m_flags.IsFlag(TrackSelection)); }

        // Remove an item from the current selection - Notification will only be fired if the selection actually changes
        inline void RemoveFromSelection(TreeListViewItem *pItem) { RemoveFromSelectionInternal(pItem, m_flags.IsFlag(TrackSelection)); }

        // Set the selection to a single item - Notification will only be fired if the selection actually changes
        inline void SetSelection(uint64 itemID) { SetSelectionInternal(FindItem(itemID), m_flags.IsFlag(TrackSelection)); }

        // Add to the current selection - Notification will only be fired if the selection actually changes
        inline void AddToSelection(uint64 itemID) { AddToSelectionInternal(FindItem(itemID), m_flags.IsFlag(TrackSelection)); }

        // Remove an item from the current selection - Notification will only be fired if the selection actually changes
        inline void RemoveFromSelection(uint64 itemID) { RemoveFromSelectionInternal(FindItem(itemID), m_flags.IsFlag(TrackSelection)); }

        // Bulk Item Operations
        //-------------------------------------------------------------------------

        void ForEachItem(Function<void(TreeListViewItem *)> const &function, bool refreshVisualState = true)
        {
            m_rootItem.ForEachChild(function);

            if (refreshVisualState)
            {
                RefreshVisualState();
            }
        }

        void ForEachItemConst(Function<void(TreeListViewItem const *)> const &function) const
        {
            m_rootItem.ForEachChildConst(function);
        }

        void UpdateItemVisibility(Function<bool(TreeListViewItem * const&)> const &isVisibleFunction, bool showParentItemsWithNoVisibleChildren = false)
        {
            m_rootItem.UpdateVisibility(isVisibleFunction, showParentItemsWithNoVisibleChildren);
            RefreshVisualState();
        }

    protected:
        inline int32 GetNumItems() const { return (int32_t)m_visualTree.Count(); }

        void DestroyItem(uint64 uniqueID);

        //-------------------------------------------------------------------------

        // Tears down the entire tree
        void DestroyTree();

        // Caches the current selection and expansion state
        void CacheSelectionAndExpansionState();

        // Tries to restore the current selection and expansion state
        void RestoreCachedSelectionAndExpansionState();

        void HandleItemSelection(const Float2& rect_min, const Float2& rect_max, TreeListViewItem *pItem);
        void SetSelectionInternal(TreeListViewItem *pItem, bool updateView);
        void AddToSelectionInternal(TreeListViewItem *pItem, bool updateView);
        void AddToSelectionInternal(List<TreeListViewItem *> const &itemRange, bool updateView);
        void SetSelectionInternal(List<TreeListViewItem *> const &itemRange, bool updateView);
        void RemoveFromSelectionInternal(TreeListViewItem *pItem, bool updateView);

        void DrawVisualItem(TreeListViewContext context, VisualTreeItem &visualTreeItem);
        void TryAddItemToVisualTree(TreeListViewItem *pItem, int32_t hierarchyLevel);

        void RebuildVisualTree();

        int32 GetVisualTreeItemIndex(TreeListViewItem const *pBaseItem) const;
        int32 GetVisualTreeItemIndex(uint64 uniqueID) const;

    protected:
        // The root of the tree - fill this with your items
        TreeRootItem m_rootItem;

        // The active item is an item that is activated (and deactivated) via a double click
        TreeListViewItem *m_pActiveItem = nullptr;

        // Control tree view behavior
        EnumFlags<Flags> m_flags;

        List<VisualTreeItem> m_visualTree;
        VisualTreeState m_visualTreeState = VisualTreeState::NeedsRebuild;
        float m_estimatedRowHeight = -1.0f;
        float m_estimatedTreeHeight = -1.0f;
        int32 m_firstVisibleRowItemIdx = 0;
        float m_itemControlColumnWidth = 0;
        bool m_maintainVisibleRowIdx = false;
        bool m_isDrawingTree = false;

        // The currently selected item (changes frequently due to clicks/focus/etc...) - In order of selection time, first is oldest
        List<TreeListViewItem *> m_selection;
        uint64 m_cachedActiveItemID = 0;
        List<uint64> m_selectedItemIDs;
        List<uint64> m_originalExpandedItems;
    };
}