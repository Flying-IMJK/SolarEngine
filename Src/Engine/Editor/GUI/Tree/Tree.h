#pragma once

#include "Runtime/UI/GUI/ContainerControl.h"

namespace SE::Editor
{
    class TreeNode;

    /// <summary>
    /// Delegate for selected tree nodes collection change.
    /// </summary>
    /// <param name="before">The before state.</param>
    /// <param name="after">The after state.</param>
    using SelectionChangedDelegate = Delegate<List<TreeNode*>&, List<TreeNode*>&>;

    /// <summary>
    /// Delegate for node click events.
    /// </summary>
    /// <param name="node">The node.</param>
    /// <param name="location">The location.</param>
    using NodeClickDelegate = Delegate<TreeNode*, Float2>;

    SE_CLASS(Reflect)
    class Tree : public ContainerControl
    {
        SE_DEFINE_CLASS(Tree, ContainerControl)
    public:
        /// <summary>
        /// The key updates timeout in seconds.
        /// </summary>
        static constexpr float KeyUpdateTimeout = 0.25f;

    private:
        float m_KeyUpdateTime;
        bool m_SupportMultiSelect;
        Margin m_Margin;
        bool m_AutoSize = true;

    public:
        /// <summary>
        /// The TreeNode that is being dragged over. This could have a value when not dragging.
        /// </summary>
        TreeNode* DraggedOverNode = nullptr;

        /// <summary>
        /// Action fired when tree nodes selection gets changed.
        /// </summary>
        SelectionChangedDelegate SelectedChanged;

        /// <summary>
        /// Action fired when mouse goes right click up on node.
        /// </summary>
        NodeClickDelegate RightClick;

        /// <summary>
        /// List with all selected nodes
        /// </summary>
        // [HideInEditor, NoSerialize]
        List<TreeNode*> Selection = List<TreeNode*>();

        /// <summary>
        /// Gets the first selected node or null.
        /// </summary>
        PRO_GET(SelectedNode, Tree, TreeNode*, __GetSelectedNode);

        /// <summary>
        /// Gets or sets the margin for the child tree nodes.
        /// </summary>
        // [EditorOrder(0), Tooltip("The margin applied to the child tree nodes.")]
        PRO_REF(Margin, Tree, Margin, __GetMargin, __SetMargin);

        /// <summary>
        /// Gets or sets the value indicating whenever the tree will auto-size to the tree nodes dimensions.
        /// </summary>
        // [EditorOrder(10), Tooltip("If checked, the tree will auto-size to the tree nodes dimensions.")]
        PRO(AutoSize, Tree, bool, __GetAutoSize, __SetAutoSize);

        /// <summary>
        /// Initializes a new instance of the <see cref="Tree"/> class.
        /// </summary>
        Tree();

        /// <summary>
        /// Initializes a new instance of the <see cref="Tree"/> class.
        /// </summary>
        /// <param name="supportMultiSelect">True if support multi selection for tree nodes, otherwise false.</param>
        Tree(bool supportMultiSelect);

        void OnRightClickInternal(TreeNode* node, Float2& location);

        /// <summary>
        /// Selects single tree node.
        /// </summary>
        /// <param name="node">Node to select.</param>
        void Select(TreeNode* node);

        /// <summary>
        /// Selects tree nodes.
        /// </summary>
        /// <param name="nodes">Nodes to select.</param>
        void Select(List<TreeNode*> &nodes);

        /// <summary>
        /// Clears the selection.
        /// </summary>
        void Deselect();

        /// <summary>
        /// Adds or removes node to/from the selection
        /// </summary>
        /// <param name="node">The node.</param>
        void AddOrRemoveSelection(TreeNode* node);

        /// <summary>
        /// Selects tree nodes range (used to select part of the tree using Shift+Mouse).
        /// </summary>
        /// <param name="endNode">End range node</param>
        void SelectRange(TreeNode* endNode);

        /// <summary>
        /// Select all expanded nodes
        /// </summary>
        void SelectAllExpanded();

        /// <summary>
        /// Deselect all nodes
        /// </summary>
        void DeselectAll();

        /// <inheritdoc />
        void Update(float deltaTime) override;

        /// <inheritdoc />
        bool OnKeyDown(KeyboardKeys key) override;

        /// <inheritdoc />
        void OnGetFocus() override;

        /// <inheritdoc />
        void OnParentResized() override;

    protected:
        /// <inheritdoc />
        void PerformLayoutBeforeChildren() override;

        /// <inheritdoc />
        void PerformLayoutAfterChildren() override;

    private:
        void WalkSelectExpandedTree(List<TreeNode*> selection, TreeNode* node);

        void BulkSelectUpdateExpanded(bool select = true);

        void WalkSelectRangeExpandedTree(List<TreeNode*> selection, TreeNode* node, Rectangle &range);

        Rectangle CalcNodeRangeRect(TreeNode* node);

        bool __GetAutoSize() { return m_AutoSize; }
        void __SetAutoSize(bool value)
        {
            m_AutoSize = value;
            PerformLayout();
        }

        ::SE::Margin& __GetMargin() { return m_Margin; }
        void __SetMargin(::SE::Margin & value)
        {
            m_Margin = value;
            PerformLayout();
        }

        TreeNode* __GetSelectedNode()
        {
            return Selection.Count() > 0 ? Selection[0] : nullptr;;
        }
    };
} // SE

