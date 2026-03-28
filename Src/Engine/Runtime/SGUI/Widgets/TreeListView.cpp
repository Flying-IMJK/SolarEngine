#include "TreeListView.h"
#include "Core/Math/NumericRange.h"
#include "Core/Types/Collections/ListExtensions.h"
#include "Core/Types/Collections/Sorting.h"
#include "Core/Types/Strings/StringConverter.h"

//-------------------------------------------------------------------------

namespace SE::GUI
{
    bool TreeListViewItem::Compare(TreeListViewItem const* pItem ) const
    {
        StringView pNameA = GetName().ToString();
		StringView pNameB = pItem->GetName().ToString();

        int32 const result = StringUtils::Compare(pNameA.Get(), pNameA.Length(), pNameB.Get(), pNameB.Length());
        return result < 0;
    }

    Color TreeListViewItem::GetDisplayColor( ItemState state ) const
    {
        Color color = Style::s_colorText;

        switch ( state )
        {
            case ItemState::Selected:
            {
                color = Style::s_colorAccent1;
            }
            break;

            case ItemState::Active:
            {
                color = Style::s_colorAccent0;
            }
            break;

            default:
            break;
        }

        return color;
    }

    void TreeListViewItem::DestroyChild( uint64 uniqueItemID )
    {
        for ( int32 i = 0; i < (int32) m_children.Count(); i++ )
        {
            if ( m_children[i]->GetUniqueID() == uniqueItemID )
            {
                TreeListViewItem * item = m_children[i];
                item->DestroyChildren();
                Delete(item);
                m_children.RemoveAt(  i );
                break;
            }
        }
    }

    void TreeListViewItem::DestroyChildren()
    {
        for ( auto& pChild : m_children )
        {
            pChild->DestroyChildren();
            Delete( pChild );
        }

        m_children.Clear();
    }

    TreeListViewItem const* TreeListViewItem::FindChild( uint64 uniqueID ) const
    {
        for ( auto pChild : m_children )
        {
            if ( pChild->GetUniqueID() == uniqueID )
            {
                return pChild;
            }
        }

        return nullptr;
    }

    TreeListViewItem* TreeListViewItem::FindChild(Function<bool(TreeListViewItem * const&)> const& searchPredicate )
    {
        TreeListViewItem* pFoundItem = nullptr;
        auto itemIndex = ListExtensions::IndexOf(m_children, searchPredicate);
        if (itemIndex != INVALID_INDEX)
        {
            pFoundItem = m_children[itemIndex];
        }
        return pFoundItem;
    }

    TreeListViewItem* TreeListViewItem::SearchChildren(Function<bool(TreeListViewItem * const&)> const& searchPredicate)
    {
        TreeListViewItem* pFoundItem = nullptr;

        // Search immediate children
        //-------------------------------------------------------------------------

		auto itemIndex = ListExtensions::IndexOf(m_children, searchPredicate);
        if ( itemIndex != INVALID_INDEX )
        {
            pFoundItem = m_children[itemIndex];
            return pFoundItem;
        }

        // Recursively search each child
        //-------------------------------------------------------------------------

        for ( auto pChild : m_children )
        {
            pFoundItem = pChild->SearchChildren( searchPredicate );
            if ( pFoundItem != nullptr )
            {
                return pFoundItem;
            }
        }

        return pFoundItem;
    }

    void TreeListViewItem::UpdateVisibility(Function<bool(TreeListViewItem * const&)> const& isVisibleFunction, bool showParentItemsWithNoVisibleChildren )
    {
        if ( HasChildren() )
        {
            // Always update child visibility before our own - this allows clients to affect our visibility based on our children's visibility
            bool hasVisibleChild = false;
            for ( auto& pChild : m_children )
            {
                pChild->UpdateVisibility(isVisibleFunction, showParentItemsWithNoVisibleChildren);
                if ( pChild->IsVisible() )
                {
                    hasVisibleChild = true;
                }
            }

            // If we have a visible child, we are always visible
            if ( hasVisibleChild )
            {
                m_isVisible = true;
            }
            else // Optional: run visibility check if requested
            {
                if (showParentItemsWithNoVisibleChildren)
                {
                    m_isVisible = isVisibleFunction(this);
                }
                else
                {
                    m_isVisible = false;
                }
            }
        }
        else // Leaf nodes always run visibility check
        {
            m_isVisible = isVisibleFunction( this );
        }
    }

    void TreeListViewItem::SortChildren()
    {
        (m_children, [&] (TreeListViewItem const* pItemA, TreeListViewItem const* pItemB )
                   {
                       if (pItemA->IsLeaf() && !pItemB->IsLeaf())
                       {
                          return true;
                       }
                       else if (!pItemA->IsLeaf() && pItemB->IsLeaf())
                       {
                           return false;
                       }

                       return pItemA->Compare(pItemB);
                   });

        //-------------------------------------------------------------------------

        for ( auto pChild : m_children )
        {
            pChild->SortChildren();
        }
    }

    //-------------------------------------------------------------------------

    TreeListView::~TreeListView()
    {
        m_rootItem.DestroyChildren();
    }

    void TreeListView::SetViewToSelection()
    {
        // Expand all parents
        for ( auto pSelectedItem : m_selection )
        {
            auto pParentItem = pSelectedItem->m_pParent;
            while (pParentItem != nullptr)
            {
                pParentItem->SetExpanded( true );
                pParentItem = pParentItem->m_pParent;
            }
        }

        m_visualTreeState = VisualTreeState::NeedsRebuildAndFocusSelection;
    }

    TreeListViewItem* TreeListView::FindItem( uint64 uniqueID )
    {
        if ( m_rootItem.GetUniqueID() == uniqueID )
        {
            return &m_rootItem;
        }

		Function<bool(TreeListViewItem * const&)> SearchPredicate = [uniqueID] (TreeListViewItem const* pItem)
        {
            return pItem->GetUniqueID() == uniqueID;
        };

        auto pFoundItem = m_rootItem.SearchChildren(SearchPredicate);
        return pFoundItem;
    }

    void TreeListView::DestroyItem( uint64 uniqueID )
    {
        ENGINE_ASSERT( m_rootItem.GetUniqueID() != uniqueID );

        // Remove from visual tree
        //-------------------------------------------------------------------------

        for ( auto i = 0; i < m_visualTree.Count(); i++ )
        {
            if (m_visualTree[i].pItem->GetUniqueID() == uniqueID )
            {
                m_visualTree.RemoveAt(  i );
                break;
            }
        }

        // Remove for regular tree
        //-------------------------------------------------------------------------

		Function<bool(TreeListViewItem * const&)> SearchPredicate = [uniqueID] ( TreeListViewItem const* pItem )
        {
            return pItem->FindChild( uniqueID ) != nullptr;
        };

        auto pFoundParentItem = m_rootItem.SearchChildren(SearchPredicate);
        ENGINE_ASSERT( pFoundParentItem != nullptr );
        pFoundParentItem->DestroyChild( uniqueID );
    }

    void TreeListView::RebuildTree( TreeListViewContext& context, bool maintainSelection )
    {
        ENGINE_ASSERT( !IsCurrentlyDrawingTree() );

        // Record current state
        //-------------------------------------------------------------------------

        if ( maintainSelection )
        {
            CacheSelectionAndExpansionState();
        }

        // Reset tree state
        //-------------------------------------------------------------------------

        DestroyTree();

        // Rebuild Tree
        //-------------------------------------------------------------------------

        ENGINE_ASSERT(context.rebuildTree.IsBinded());
        context.rebuildTree( &m_rootItem );

        // Sort Tree
        //-------------------------------------------------------------------------

        if ( m_flags.IsFlag( Flags::SortTree ) )
        {
            m_rootItem.SortChildren();
        }

        // Restore original state
        //-------------------------------------------------------------------------

        if ( maintainSelection )
        {
            RestoreCachedSelectionAndExpansionState();
        }

        m_visualTreeState = VisualTreeState::NeedsRebuild;
        RebuildVisualTree();

        if ( m_flags.IsFlag( TrackSelection ) )
        {
            SetViewToSelection();
        }
    }

    void TreeListView::DestroyTree()
    {
        m_selection.Clear();
        m_pActiveItem = nullptr;
        m_rootItem.DestroyChildren();
        m_rootItem.SetExpanded( true );
        m_visualTree.Clear();
    }

    //-------------------------------------------------------------------------

    void TreeListView::CacheSelectionAndExpansionState()
    {
        m_cachedActiveItemID = 0;
        if ( m_pActiveItem != nullptr )
        {
            m_cachedActiveItemID = m_pActiveItem->GetUniqueID();
        }

        //-------------------------------------------------------------------------

        m_selectedItemIDs.Clear();
        for ( auto pSelectedItem : m_selection )
        {
            m_selectedItemIDs.Add( pSelectedItem->GetUniqueID() );
        }

        //-------------------------------------------------------------------------

        m_originalExpandedItems.Clear();
        m_originalExpandedItems.EnsureCapacity( GetNumItems() );
		
        ForEachItemConst([this] ( TreeListViewItem const* pItem )
		{
		  if ( pItem->IsExpanded() )
		  {
			  m_originalExpandedItems.Add( pItem->GetUniqueID() );
		  }
		});
    }

    void TreeListView::RestoreCachedSelectionAndExpansionState()
    {
        // Restore Item State
        //-------------------------------------------------------------------------
        ForEachItem([this] ( TreeListViewItem* const&pItem )
		{
		  if ( m_originalExpandedItems.Contains( pItem->GetUniqueID() ) )
		  {
			  pItem->SetExpanded( true );
		  }

		  pItem->m_isSelected = false;
		});

        // Restore active item
        //-------------------------------------------------------------------------

		Function<bool(TreeListViewItem * const&)> findActiveItem = [this] ( TreeListViewItem * const&pItem ) {
			return pItem->GetUniqueID() == m_cachedActiveItemID;
		};
        m_pActiveItem = m_rootItem.SearchChildren(findActiveItem);

        // Restore selection
        //-------------------------------------------------------------------------

        m_selection.Clear();
        for ( auto selectedItemID : m_selectedItemIDs )
        {
            auto pPreviouslySelectedItem = m_rootItem.SearchChildren([selectedItemID] ( TreeListViewItem const* pItem )
			{
			  return pItem->GetUniqueID() == selectedItemID;
			});
			
            if ( pPreviouslySelectedItem != nullptr )
            {
                m_selection.Add( pPreviouslySelectedItem );
                pPreviouslySelectedItem->m_isSelected = true;
            }
        }
    }

    //-------------------------------------------------------------------------

    void TreeListView::TryAddItemToVisualTree( TreeListViewItem* pItem, int32 hierarchyLevel )
    {
        ENGINE_ASSERT( pItem != nullptr );
        ENGINE_ASSERT( hierarchyLevel >= 0 );

        //-------------------------------------------------------------------------

        if ( !pItem->IsVisible() )
        {
            return;
        }

        m_visualTree.Add(VisualTreeItem(pItem, hierarchyLevel));

        //-------------------------------------------------------------------------

        if ( pItem->IsExpanded() )
        {
            // If we want to sort branches vs leaves, we need to run a two pass update
            if ( m_flags.IsFlag( ShowBranchesFirst ) )
            {
                // Always add branch items first
                for ( auto& pChildItem : pItem->m_children )
                {
                    if ( !pChildItem->IsLeaf() )
                    {
                        TryAddItemToVisualTree( pChildItem, hierarchyLevel + 1 );
                    }
                }

                if (!m_flags.IsFlag(OnlyShowBranch))
                {
                    // Add leaf items last
                    for (auto &pChildItem: pItem->m_children)
                    {
                        if (pChildItem->IsLeaf())
                        {
                            TryAddItemToVisualTree(pChildItem, hierarchyLevel + 1);
                        }
                    }
                }
            }
            else // Maintain sorted order irrespective of branch/leaf status
            {
                for ( auto& pChildItem : pItem->m_children )
                {
                    if (!m_flags.IsFlag(OnlyShowBranch) || !pChildItem->IsLeaf())
                    {
                        TryAddItemToVisualTree( pChildItem, hierarchyLevel + 1 );
                    }
                }
            }
        }
    }

    void TreeListView::RebuildVisualTree()
    {
        ENGINE_ASSERT( m_visualTreeState != VisualTreeState::UpToDate );

        //-------------------------------------------------------------------------

        m_visualTree.Clear();

        // If we want to sort branches vs leaves, we need to run a two pass update
        if (m_flags.IsFlag( ShowBranchesFirst ))
        {
            // Always add branch items first
            for (auto& pChildItem : m_rootItem.m_children)
            {
                if (!pChildItem->IsLeaf())
                {
                    TryAddItemToVisualTree(pChildItem, 0);
                }
            }

            if (!m_flags.IsFlag(OnlyShowBranch))
            {
                // Add leaf items last
                for ( auto& pChildItem : m_rootItem.m_children)
                {
                    if (pChildItem->IsLeaf())
                    {
                        TryAddItemToVisualTree(pChildItem, 0);
                    }
                }
            }
        }
        else // Maintain sorted order irrespective of branch/leaf status
        {
            for ( auto& pChildItem : m_rootItem.m_children )
            {
                if (!m_flags.IsFlag(OnlyShowBranch) || !pChildItem->IsLeaf())
                {
                    TryAddItemToVisualTree( pChildItem, 0 );
                }
            }
        }

        m_estimatedRowHeight = -1.0f;
        m_estimatedTreeHeight = -1.0f;

        //-------------------------------------------------------------------------

        // Set view to selection
        if ( m_visualTreeState == VisualTreeState::NeedsRebuildAndFocusSelection )
        {
            for ( auto i = 0; i < m_visualTree.Count(); i++ )
            {
                if ( m_visualTree[i].pItem->m_isSelected )
                {
                    m_firstVisibleRowItemIdx = i;
                    m_maintainVisibleRowIdx = true;
                    break;
                }
            }
        }

        m_visualTreeState = VisualTreeState::UpToDate;
    }

    int32 TreeListView::GetVisualTreeItemIndex( TreeListViewItem const* pBaseItem ) const
    {
        int32 const numItems = (int32) m_visualTree.Count();
        for ( auto i = 0; i < numItems; i++ )
        {
            if (m_visualTree[i].pItem == pBaseItem )
            {
                return i;
            }
        }

        return -1;
    }

    int32 TreeListView::GetVisualTreeItemIndex( uint64 uniqueID ) const
    {
        int32 const numItems = (int32) m_visualTree.Count();
        for ( auto i = 0; i < numItems; i++ )
        {
            if (m_visualTree[i].pItem->GetUniqueID() == uniqueID )
            {
                return i;
            }
        }

        return -1;
    }

    //-------------------------------------------------------------------------
    
    void TreeListView::ClearSelection()
    {
        if ( !m_selection.IsEmpty() )
        {
            for ( auto pItem : m_selection )
            {
                pItem->m_isSelected = false;
                pItem->OnSelectionStateChanged();
            }

            m_selection.Clear();
        }
    }

    void TreeListView::AddToSelectionInternal( TreeListViewItem* pItem, bool updateView )
    {
        ENGINE_ASSERT( pItem != nullptr );

        if ( !m_selection.Contains(pItem))
        {
            if ( !m_flags.IsFlag( MultiSelectionAllowed ) )
            {
                ClearSelection();
            }

            pItem->m_isSelected = true;
            pItem->OnSelectionStateChanged();
            m_selection.Add( pItem );

            if ( updateView )
            {
                SetViewToSelection();
            }
        }
    }

    void TreeListView::SetSelectionInternal( TreeListViewItem* pItem, bool updateView )
    {
        ENGINE_ASSERT( pItem != nullptr );

        // Check if we need to actually modify the selection
        if ( m_selection.Count() == 1 && m_selection[0] == pItem )
        {
            return;
        }

        //-------------------------------------------------------------------------

        ClearSelection();
        AddToSelectionInternal( pItem, updateView );
    }

    void TreeListView::AddToSelectionInternal( List<TreeListViewItem*> const& itemRange, bool updateView )
    {
        if ( itemRange.IsEmpty() )
        {
            return;
        }

        //-------------------------------------------------------------------------

        for ( auto pItem : itemRange )
        {
            ENGINE_ASSERT( pItem != nullptr );
            AddToSelectionInternal( pItem, false );
        }

        if ( updateView )
        {
            SetViewToSelection();
        }
    }

    void TreeListView::RemoveFromSelectionInternal( TreeListViewItem* pItem, bool updateView )
    {
        ENGINE_ASSERT( m_flags.IsFlag( MultiSelectionAllowed ) );
        ENGINE_ASSERT( pItem != nullptr );

        if ( m_selection.Contains( pItem ) )
        {
			m_selection.Remove(pItem);
            pItem->m_isSelected = false;
            pItem->OnSelectionStateChanged();
        }

        if ( updateView )
        {
            SetViewToSelection();
        }
    }

    void TreeListView::SetSelectionInternal( List<TreeListViewItem*> const& itemRange, bool updateView )
    {
        bool shouldModifySelection = true;

        if ( itemRange.IsEmpty() )
        {
            ClearSelection();
            return;
        }

        //-------------------------------------------------------------------------

        // Deselect items not in the specified range
        for( int32 i = (int32) m_selection.Count() - 1; i >=0; i-- )
        {
            if ( itemRange.Contains( m_selection[i] ) )
            {
                RemoveFromSelectionInternal( m_selection[i], false );
            }
        }

        // Select new items
        for ( auto pItem : itemRange )
        {
            ENGINE_ASSERT( pItem != nullptr );

            if ( !m_selection.Contains( pItem ) )
            {
                AddToSelectionInternal( pItem, false );
            }
        }

        // Update final view
        if ( updateView )
        {
            SetViewToSelection();
        }
    }

    void TreeListView::HandleItemSelection(const Float2& rect_min, const Float2& rect_max, TreeListViewItem* pItem )
    {
        ENGINE_ASSERT( pItem != nullptr );
        bool const isSelected = pItem->m_isSelected;

        // Left click follows regular selection logic
        if (ImGui::IsWindowFocused() && ImGui::IsMouseHoveringRect(rect_min, rect_max) && ImGui::IsMouseDown(ImGuiMouseButton_Left))
        {
            if ( m_flags.IsFlag( MultiSelectionAllowed ) && ImGui::GetIO().KeyShift )
            {
                if ( m_selection.IsEmpty() )
                {
                    AddToSelectionInternal( pItem, false );
                }
                else
                {
                    // Get the index of the clicked item
                    int32 const clickedItemIdx = GetVisualTreeItemIndex( pItem );

                    // Get the index of the selection range start item
                    TreeListViewItem const* const pLastSelectedItem = m_selection.Last();
                    int32 const lastSelectedItemIdx = GetVisualTreeItemIndex( pLastSelectedItem );

                    // Get the items we want to add to the selection
                    List<TreeListViewItem*> itemsToSelect;
                    itemsToSelect.Resize( 20 );

                    // Ensure that we always keep the last selected item as last item in the range (i.e. the last selected)
                    if ( lastSelectedItemIdx <= clickedItemIdx )
                    {
                        for ( auto i = clickedItemIdx; i >= lastSelectedItemIdx; i-- )
                        {
                            itemsToSelect.Add( m_visualTree[i].pItem );
                        }
                    }
                    else // Selection is upwards in tree
                    {
                        for ( auto i = clickedItemIdx; i <= lastSelectedItemIdx; i++ )
                        {
                            itemsToSelect.Add( m_visualTree[i].pItem );
                        }
                    }

                    // Modify the selection
                    SetSelectionInternal( itemsToSelect, false );
                }
            }
            else  if ( m_flags.IsFlag( MultiSelectionAllowed ) && ::ImGui::GetIO().KeyCtrl )
            {
                if ( isSelected )
                {
                    RemoveFromSelectionInternal( pItem, false );
                }
                else
                {
                    AddToSelectionInternal( pItem, false );
                }
            }
            else
            {
                SetSelectionInternal( pItem, false );
            }
        }
        // Right click never deselects! Nor does it support multi-selection
        else if (ImGui::IsWindowFocused() && ImGui::IsMouseHoveringRect(rect_min, rect_max) && ImGui::IsItemClicked( ImGuiMouseButton_Right ))
        {
            if (!isSelected)
            {
                SetSelectionInternal( pItem, false );
            }
        }
    }

    //-------------------------------------------------------------------------

    void TreeListView::DrawVisualItem( TreeListViewContext context, VisualTreeItem& visualTreeItem )
    {
        ENGINE_ASSERT(visualTreeItem.pItem != nullptr && visualTreeItem.hierarchyLevel >= 0 );

        TreeListViewItem* const pItem = visualTreeItem.pItem;
        bool const isSelectedItem = m_selection.Contains( pItem );
        ENGINE_ASSERT( isSelectedItem == pItem->m_isSelected );
        bool const isActiveItem = ( pItem == m_pActiveItem );

        ImGui::PushID( pItem );
        ImGui::TableNextRow();

        // Draw label column
        //-------------------------------------------------------------------------

        ImGui::TableSetColumnIndex( 0 );

        // Set tree indent level
        float const indentLevel = visualTreeItem.hierarchyLevel * 8;
        bool const requiresIndent = indentLevel > 0;
        if ( requiresIndent )
        {
            ImGui::Indent( indentLevel );
        }

        // Set node flags
        uint32 treeNodeflags = ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_OpenOnArrow;

        if (pItem->IsHeader() )
        {
            treeNodeflags |= (ImGuiTreeNodeFlags_Framed);
        }

        if (isSelectedItem)
        {
            treeNodeflags |= ImGuiTreeNodeFlags_Selected;
        }

        // Leaf nodes
        if (pItem->IsLeaf())
        {
            treeNodeflags |= ImGuiTreeNodeFlags_Leaf;

            if (m_flags.IsFlag( ShowBulletsOnLeaves))
            {
                treeNodeflags |= ImGuiTreeNodeFlags_Bullet;
            }
        }
        else // Branch nodes
        {
            if (m_flags.IsFlag(OnlyShowBranch) && !visualTreeItem.childHasBranch)
            {
                treeNodeflags |= ImGuiTreeNodeFlags_Leaf;
            }

            ImGui::SetNextItemOpen( pItem->IsExpanded() );
        }

        bool newExpansionState = false;
        // Draw label
        String displayLabel = String::Format(SE_TEXT("{0} {1}"), pItem->GetDisplayIcon(), pItem->GetDisplayName());

        TreeListViewItem::ItemState const state = isActiveItem ? TreeListViewItem::Active :
                isSelectedItem ? TreeListViewItem::Selected : TreeListViewItem::None;

        Float2 rectMin, rectMax;
        {
            Font const activeItemFont = m_flags.IsFlag( UseSmallFont ) ? Font::SmallBold : Font::MediumBold;
            Font const inactiveItemFont = m_flags.IsFlag( UseSmallFont ) ? Font::Small : Font::Medium;

            ScopedFont font( isActiveItem ? activeItemFont : inactiveItemFont, pItem->GetDisplayColor( state ) );

            newExpansionState = ImGui::TreeNode(pItem->GetUniqueID(), String::Empty, treeNodeflags);

            Float2 size = ImGui::GetItemRectSize();
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() - size.y - 2);
            if (!pItem->IsLeaf())
            {
                ImGui::SetCursorPosX(20);
                size.x -= 20;
            }

            ImGui::Label(displayLabel);
            rectMin = ImGui::GetItemRectMin();
            rectMax = rectMin + size;
        }


        // Tooltip
        auto const pTooltipText = pItem->GetTooltipText();
        if ( pTooltipText != nullptr )
        {
            ImGui::ItemTooltip( pTooltipText );
        }

        // Drag and Drop
        if ( pItem->IsDragAndDropSource() )
        {
            if ( ImGui::BeginDragDropSource( ImGuiDragDropFlags_None ) )
            {
                pItem->SetDragAndDropPayloadData();
                ImGui::Label(displayLabel);
                ImGui::EndDragDropSource();
            }
        }

        if ( pItem->IsDragAndDropTarget() )
        {
            if ( ImGui::BeginDragDropTarget() )
            {
                if (context.onDragAndDrop.IsBinded())
                {
                    context.onDragAndDrop( pItem );
                }

                ImGui::EndDragDropTarget();
            }
        }

        // Handle expansion
        if ( newExpansionState )
        {
            ImGui::TreePop();
        }

        if (!pItem->IsLeaf() && pItem->IsExpanded() != newExpansionState )
        {
            pItem->SetExpanded( newExpansionState);
            m_visualTreeState = VisualTreeState::NeedsRebuild;
        }

        // Handle selection
        HandleItemSelection(rectMin, rectMax, pItem );

        // Context Menu
        if ( pItem->HasContextMenu() )
        {
            if ( ImGui::BeginPopupContextItem( "ItemContextMenu" ) )
            {
                if ( !isSelectedItem )
                {
                    if ( m_flags.IsFlag( MultiSelectionAllowed ) && ImGui::GetIO().KeyShift || ImGui::GetIO().KeyCtrl )
                    {
                        AddToSelectionInternal( pItem, false );
                    }
                    else // Switch selection to this item
                    {
                        SetSelectionInternal( pItem, false );
                    }
                }

                if (context.drawItemContextMenu.IsBinded())
                {
                    List<TreeListViewItem*> selectedItemsWithContextMenus;
                    for ( auto pSelectedItem : m_selection )
                    {
                        if ( pSelectedItem->HasContextMenu() )
                        {
                            selectedItemsWithContextMenus.Add( pSelectedItem );
                        }
                    }

                    context.drawItemContextMenu( selectedItemsWithContextMenus );
                    ImGui::EndPopup();
                }
            }
        }

        // Handle double click
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked( 0 ))
        {
            if ( pItem->OnDoubleClick() )
            {
                m_visualTreeState = VisualTreeState::NeedsRebuild;
            }

            // Activation
            //-------------------------------------------------------------------------

            if ( pItem->IsActivatable() )
            {
                if ( m_pActiveItem == pItem )
                {
                    m_pActiveItem = nullptr;
                    m_pActiveItem->m_isActivated = false;
                }
                else
                {
                    m_pActiveItem = pItem;
                    m_pActiveItem->m_isActivated = true;
                }

                pItem->OnActivationStateChanged();
            }
        }

        // Pop indent level if set
        if ( requiresIndent )
        {
            ImGui::Unindent( indentLevel );
        }

        // Draw extra columns
        //-------------------------------------------------------------------------

        for ( int32 i = 0; i < context.numExtraColumns; i++ )
        {
            ENGINE_ASSERT(context.drawItemExtraColumns.IsBinded());
            ImGui::TableSetColumnIndex( i + 1 );
            context.drawItemExtraColumns( pItem, i );
        }

        //-------------------------------------------------------------------------

        ImGui::PopID();
    }

    bool TreeListView::UpdateAndDraw( TreeListViewContext& context, float listHeight )
    {
        if ( m_visualTreeState != VisualTreeState::UpToDate )
        {
            RebuildVisualTree();
        }

        if ( m_visualTree.IsEmpty() )
        {
            return false;
        }

        //-------------------------------------------------------------------------

        List<TreeListViewItem*> previousSelection = m_selection;

        //-------------------------------------------------------------------------

        ImGuiTableFlags tableFlags = ImGuiTableFlags_Resizable | ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_ScrollY;

        if ( m_flags.IsFlag( DrawRowBackground ) )
        {
            tableFlags |= ImGuiTableFlags_RowBg;
        }

        if ( m_flags.IsFlag( DrawBorders ) )
        {
            tableFlags |= ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_NoBordersInBody | ImGuiTableFlags_BordersV;
        }

        //-------------------------------------------------------------------------

        ImGui::PushID( this );
        ImGui::PushStyleVar( ImGuiStyleVar_CellPadding, ImVec2( 2, 2 ) );
        if ( ImGui::BeginTable( "TreeViewTable", context.numExtraColumns + 1, tableFlags, {ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x / 2, -1 } ) )
        {
            ImGui::TableSetupColumn( "Label", ImGuiTableColumnFlags_WidthStretch );
            if (context.m_setupExtraColumnHeaders.IsBinded())
            {
                context.m_setupExtraColumnHeaders();
            }

            m_isDrawingTree = true;

            ImGuiListClipper clipper;
            clipper.Begin( m_visualTree.Count() );

            // If we want to maintain the current visible set of data, update the scroll bar position to keep the same first visible item
            if ( m_maintainVisibleRowIdx && m_estimatedRowHeight > 0 )
            {
                ImGui::SetScrollY( m_firstVisibleRowItemIdx * m_estimatedRowHeight );
                m_maintainVisibleRowIdx = false;
            }

            // Draw clipped list
            while ( clipper.Step() )
            {
                for ( int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i )
                {
                    float const cursorPosY = ImGui::GetCursorPosY();
                    DrawVisualItem( context, m_visualTree[i] );
                    m_estimatedRowHeight = ImGui::GetCursorPosY() - cursorPosY;
                }
            }
            m_isDrawingTree = false;

            ImGui::EndTable();
        }
        ImGui::PopStyleVar();
        ImGui::PopID();

        // Handle input
        //-------------------------------------------------------------------------

        if ( ImGui::IsWindowFocused( ImGuiFocusedFlags_ChildWindows/* | ImGuiFocusedFlags_DockHierarchy*/ ) )
        {
            bool rebuildVisualTree = false;
            if ( ImGui::IsKeyReleased( ImGuiKey_Enter ) )
            {
                for ( auto pItem : m_selection )
                {
                    rebuildVisualTree |= pItem->OnEnterPressed();
                }
            }

            if ( rebuildVisualTree )
            {
                m_visualTreeState = VisualTreeState::NeedsRebuild;
            }
        }

        // Check selection state
        //-------------------------------------------------------------------------

        if ( previousSelection.Count() != m_selection.Count() )
        {
            return true;
        }

        for ( int32 i = 0; i < (int32) m_selection.Count(); i++ )
        {
            if ( previousSelection[i] != m_selection[i] )
            {
                return true;
            }
        }

        return false;
    }
}