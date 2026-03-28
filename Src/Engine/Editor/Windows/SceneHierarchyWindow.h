#pragma once
#include "EditorWindow.h"

namespace SE
{
    class TextBox;
}

namespace SE::Editor
{
    class ContextMenuButton;
    class DragAssets;
    class DragHandlers;
    class AssetItem;
    class TreeNode;
    class Tree;

    class SceneHierarchyWindow : public EditorWindow
    {
    private:
        TextBox* m_SearchBox;
        Tree* m_Tree;
        Panel* m_SceneTreePanel;
        bool _isUpdatingSelection;
        bool _isMouseDown;

        DragHandlers* _dragHandlers;
        DragAssets* _dragAssets;
        /*
        DragActorType* _dragActorType;
        DragControlType* _dragControlType;
        DragScriptItems* _dragScriptItems;
        */

        List<Pair<String, TypeID>> m_ActorCreateMenus;

    public:
        /// <summary>
        /// Scene tree panel.
        /// </summary>
        PRO_GET(SceneTreePanel, SceneHierarchyWindow, Panel*, __GetSceneTreePanel);

        /// <summary>
        /// Initializes a new instance of the <see cref="SceneTreeWindow"/> class.
        /// </summary>
        SceneHierarchyWindow();

        /// <summary>
        /// Enables or disables vertical and horizontal scrolling on the scene tree panel.
        /// </summary>
        /// <param name="enabled">The state to set scrolling to</param>
        void ScrollingOnSceneTreeView(bool enabled);
        
    public:
        /// <summary>
        /// Focuses search box.
        /// </summary>
        void Search();

        void ShowContextMenu(Control* parent, Float2 location);
        
        /// <inheritdoc />
        void OnInit() override;

        /// <inheritdoc />
        void Draw() override;

        /// <inheritdoc />
        bool OnMouseDown(Float2 location, MouseButton buttons) override;

        /// <inheritdoc />
        bool OnMouseUp(Float2 location, MouseButton buttons) override;

        /// <inheritdoc />
        void OnLostFocus() override;

        /// <inheritdoc />
        DragDropEffect OnDragEnter(const Float2& location, DragData* data) override;

        /// <inheritdoc />
        DragDropEffect OnDragMove(const Float2& location, DragData* data) override;

        /// <inheritdoc />
        void OnDragLeave() override;

        /// <inheritdoc />
        DragDropEffect OnDragDrop(const Float2& location, DragData* data) override;

        /// <inheritdoc />
        void OnDestroy() override;

    private:

        /// <summary>
        /// Scrolls to the selected node in the scene tree.
        /// </summary>
        void ScrollToSelectedNode();

        void OnSearchBoxTextChanged();

        void Rename();

        void Spawn(TypeID type);

        void Tree_OnSelectedChanged(List<TreeNode*>& before, List<TreeNode*>& after);

        void OnTreeRightClick(TreeNode* node, Float2 location);

        void OnSelectionChanged();

        bool ValidateDragAsset(AssetItem* assetItem);

        ContextMenu* CreateContextMenu();

        void OnExpandAllClicked(ContextMenuButton* button);

        void OnCollapseAllClicked(ContextMenuButton* button);

        /*static bool ValidateDragActorType(ScriptType actorType)
        {
            return Editor.Instance.CodeEditing.Actors.Get().Contains(actorType);
        }

        static bool ValidateDragControlType(ScriptType controlType)
        {
            return Editor.Instance.CodeEditing.Controls.Get().Contains(controlType);
        }

        static bool ValidateDragScriptItem(ScriptItem script)
        {
            return Editor.Instance.CodeEditing.Actors.Get(script) != ScriptType.Null;
        }*/

        Panel* __GetSceneTreePanel() { return m_SceneTreePanel; }
    };
} // SE

