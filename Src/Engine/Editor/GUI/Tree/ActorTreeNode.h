#pragma once
#include "TreeNode.h"
#include "Editor/GUI/Drag/DragHandlers.h"
#include "Editor/GUI/Drag/DragActors.hpp"
#include "Editor/GUI/Drag/DragAssets.hpp"

namespace SE
{
    class Panel;
    class Actor;
}

namespace SE::Editor
{
    class AssetItem;
    class RenamePopup;
    class EditorWindow;
    class DragHandlers;
    class ActorGraphNode;
    class DragActors;
    
    class ActorTreeNode : public TreeNode
    {
        SE_CLASS(ActorTreeNode, TreeNode)
    private:
        int _orderInParent;
        DragActors* _dragActors;
        DragAssets* _dragAssets;
        /*DragActorType _dragActorType;
        DragControlType _dragControlType;*/
        DragHandlers* _dragHandlers;
        List<Rectangle> _highlights;
        bool _hasSearchFilter;

        /// <summary>
        /// The actor node that owns this node.
        /// </summary>
    protected:
        ActorGraphNode* _actorNode;
        
    public:
        /// <summary>
        /// Gets the actor.
        /// </summary>
        Actor* GetActor();

        /// <summary>
        /// Gets the actor node.
        /// </summary>
        ActorGraphNode* GetActorGraphNode();

        /// <summary>
        /// Initializes a new instance of the <see cref="ActorTreeNode"/> class.
        /// </summary>
        ActorTreeNode();

        virtual void LinkNode(ActorGraphNode* node);

        void OnParentChanged(Actor* actor, ActorGraphNode* parentNode);

        void OnOrderInParentChanged();

        /// <summary>
        /// Updates the tree node text.
        /// </summary>
        virtual void UpdateText();

        /// <summary>
        /// Updates the query search filter.
        /// </summary>
        /// <param name="filterText">The filter text.</param>
        void UpdateFilter(String filterText);

        /// <inheritdoc />
        void Update(float deltaTime) override;


        /// <inheritdoc />
        bool OnShowTooltip(String text, Float2& location, Rectangle& area) override;

        /// <inheritdoc />
        void OnDestroy() override
        {
            Delete(_dragActors);
            Delete(_dragAssets);
            if (_dragHandlers != nullptr)
            {
                _dragHandlers->Clear();
            }
            Delete(_dragHandlers);
            _highlights.ClearDelete();

            _dragHandlers = nullptr;
            _dragActors = nullptr;
            _dragAssets = nullptr;

            TreeNode::OnDestroy();
        }


        int Compare(const Control* other) const override;

        /// <summary>
        /// Starts the actor renaming action.
        /// </summary>
        void StartRenaming(EditorWindow* window, Panel* treePanel = nullptr);

        /// <inheritdoc />
        void Draw() override;

        void DoDragDrop(DragData* data) override;

    protected:

        bool ShowTooltip() override;

        /// <inheritdoc />
        Color CacheTextColor() override;

        /// <inheritdoc />
        void OnExpandedChanged() override;

        /// <inheritdoc />
        DragDropEffect OnDragEnterHeader(DragData* data) override;

        /// <inheritdoc />
        DragDropEffect OnDragMoveHeader(DragData* data) override;

        /// <inheritdoc />
        void OnDragLeaveHeader() override
        {
            _dragHandlers->OnDragLeave();
        }

        /// <inheritdoc />
        DragDropEffect OnDragDropHeader(DragData* data) override;

    private:
        void OnRenamed(RenamePopup* renamePopup);

        bool ValidateDragActor(ActorGraphNode* actorNode);

        bool ValidateDragAsset(AssetItem* assetItem);
    };

} // SE

