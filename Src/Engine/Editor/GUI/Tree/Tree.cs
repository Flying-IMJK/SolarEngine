#nullable disable annotations

using System;
using System.Collections.Generic;
using SE.GUI;

namespace SE.Editor.GUI
{
    /// <summary>
    /// Delegate for selected tree nodes collection change.
    /// </summary>
    /// <param name="before">The before state.</param>
    /// <param name="after">The after state.</param>
    public delegate void SelectionChangedDelegate(List<TreeNode> before, List<TreeNode> after);

    /// <summary>
    /// Delegate for node click events.
    /// </summary>
    /// <param name="node">The node.</param>
    /// <param name="location">The location.</param>
    public delegate void NodeClickDelegate(TreeNode node, Float2 location);

    public class Tree : ContainerControl
    {
        /// <summary>
        /// The key updates timeout in seconds.
        /// </summary>
        public const float KeyUpdateTimeout = 0.25f;

        private float m_KeyUpdateTime;
        private bool m_SupportMultiSelect;
        private Margin m_Margin;
        private bool m_AutoSize = true;

        /// <summary>
        /// The TreeNode that is being dragged over. This could have a value when not dragging.
        /// </summary>
        public TreeNode DraggedOverNode = null;

        /// <summary>
        /// Action fired when tree nodes selection gets changed.
        /// </summary>
        public SelectionChangedDelegate SelectedChanged;

        /// <summary>
        /// Action fired when mouse goes right click up on node.
        /// </summary>
        public NodeClickDelegate RightClick;

        /// <summary>
        /// List with all selected nodes
        /// </summary>
        // [HideInEditor, NoSerialize]
        public List<TreeNode> Selection = new List<TreeNode>();

        /// <summary>
        /// Gets the first selected node or null.
        /// </summary>
        public TreeNode SelectedNode => Selection.Count > 0 ? Selection[0] : null;

        /// <summary>
        /// Gets or sets the margin for the child tree nodes.
        /// </summary>
        // [EditorOrder(0), Tooltip("The margin applied to the child tree nodes.")]
        public Margin Margin
        {
            get
            {
                return m_Margin;
            }
            set
            {
                m_Margin = value;
                PerformLayout();
            }
        }

        /// <summary>
        /// Gets or sets the value indicating whenever the tree will auto-size to the tree nodes dimensions.
        /// </summary>
        // [EditorOrder(10), Tooltip("If checked, the tree will auto-size to the tree nodes dimensions.")]
        public bool AutoSize
        {
            get
            {
                return m_AutoSize;
            }
            set
            {
                m_AutoSize = value;
                PerformLayout();
            }
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Tree"/> class.
        /// </summary>
        public Tree() : this(false)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Tree"/> class.
        /// </summary>
        /// <param name="supportMultiSelect">True if support multi selection for tree nodes, otherwise false.</param>
        public Tree(bool supportMultiSelect) : base(0, 0, 100, 100)
        {
            IsScrollable = true;
            AutoFocus = false;

            m_SupportMultiSelect = supportMultiSelect;
            m_KeyUpdateTime = KeyUpdateTimeout;
            m_Margin = new Margin(0, 0, 0, 0);
        }

        public void OnRightClickInternal(TreeNode node, ref Float2 location)
        {
            if (RightClick != null)
            {
                RightClick(node, location);
            }
        }

        /// <summary>
        /// Selects single tree node.
        /// </summary>
        /// <param name="node">Node to select.</param>
        public void Select(TreeNode node)
        {
            SE.Debug.Assert(node != null, "Invalid tree node.");

            // Check if won't change
            if (Selection.Count == 1 && SelectedNode == node)
            {
                return;
            }

            // Cache previous state
            List<TreeNode> prev = new List<TreeNode>(Selection);

            // Update selection
            Selection.Clear();
            Selection.Add(node);

            // Ensure that node can be visible (all it's parents are expanded)
            node.ExpandAllParents();

            node.Focus();

            // Fire event
            if (SelectedChanged != null)
            {
                SelectedChanged(prev, Selection);
            }
        }

        /// <summary>
        /// Selects tree nodes.
        /// </summary>
        /// <param name="nodes">Nodes to select.</param>
        public void Select(List<TreeNode> nodes)
        {
            // Check if won't change
            bool unchanged = Selection.Count == nodes.Count;
            if (unchanged)
            {
                for (int i = 0; i < Selection.Count; i++)
                {
                    if ((object)Selection[i] != (object)nodes[i])
                    {
                        unchanged = false;
                        break;
                    }
                }
            }
            if (unchanged)
            {
                return;
            }

            // Cache previous state
            List<TreeNode> prev = new List<TreeNode>(Selection);

            // Update selection
            Selection.Clear();
            if (m_SupportMultiSelect)
            {
                Selection.AddRange(nodes);
            }
            else if (nodes.Count > 0)
            {
                Selection.Add(nodes[0]);
            }

            // Ensure that every selected node can be visible (all it's parents are expanded)
            // TODO: maybe use faster tree walk or faster algorythm?
            for (int i = 0; i < Selection.Count; i++)
            {
                Selection[i].ExpandAllParents();
            }

            // Fire event
            if (SelectedChanged != null)
            {
                SelectedChanged(prev, Selection);
            }
        }

        /// <summary>
        /// Clears the selection.
        /// </summary>
        public void Deselect()
        {
            // Check if won't change
            if (Selection.Count == 0)
            {
                return;
            }

            // Cache previous state
            List<TreeNode> prev = new List<TreeNode>(Selection);

            // Update selection
            Selection.Clear();

            // Fire event
            if (SelectedChanged != null)
            {
                SelectedChanged(prev, Selection);
            }
        }

        /// <summary>
        /// Adds or removes node to/from the selection
        /// </summary>
        /// <param name="node">The node.</param>
        public void AddOrRemoveSelection(TreeNode node)
        {
            // Cache previous state
            List<TreeNode> prev = new List<TreeNode>(Selection);

            // Check if is selected
            int index = -1;
            for (int i = 0; i < Selection.Count; i++)
            {
                if ((object)Selection[i] == (object)node)
                {
                    index = i;
                    break;
                }
            }
            if (index != -1)
            {
                // Remove
                Selection.RemoveAt(index);
            }
            else
            {
                if (!m_SupportMultiSelect)
                {
                    Selection.Clear();
                }

                // Add
                Selection.Add(node);
            }

            // Fire event
            if (SelectedChanged != null)
            {
                SelectedChanged(prev, Selection);
            }
        }

        /// <summary>
        /// Selects tree nodes range (used to select part of the tree using Shift+Mouse).
        /// </summary>
        /// <param name="endNode">End range node</param>
        public void SelectRange(TreeNode endNode)
        {
            if (m_SupportMultiSelect && Selection.Count > 0)
            {
                // Cache previous state
                List<TreeNode> prev = new List<TreeNode>(Selection);

                // Update selection
                Rectangle selectionRect = CalcNodeRangeRect(Selection[0]);
                for (int i = 1; i < Selection.Count; i++)
                {
                    selectionRect = Rectangle.Union(selectionRect, CalcNodeRangeRect(Selection[i]));
                }
                Rectangle endNodeRect = CalcNodeRangeRect(endNode);
                if (endNodeRect.Top - Mathf.Epsilon <= selectionRect.Top)
                {
                    float diff = selectionRect.Top - endNodeRect.Top;
                    selectionRect.Location.Y -= diff;
                    selectionRect.Size.Y += diff;
                }
                else if (endNodeRect.Bottom + Mathf.Epsilon >= selectionRect.Bottom)
                {
                    float diff = endNodeRect.Bottom - selectionRect.Bottom;
                    selectionRect.Size.Y += diff;
                }
                Selection.Clear();
                WalkSelectRangeExpandedTree(Selection, m_Children[0] as TreeNode, ref selectionRect);

                // Check if changed
                bool changed = Selection.Count != prev.Count;
                if (!changed)
                {
                    for (int i = 0; i < Selection.Count; i++)
                    {
                        if ((object)Selection[i] != (object)prev[i])
                        {
                            changed = true;
                            break;
                        }
                    }
                }
                if (changed)
                {
                    // Fire event
                    if (SelectedChanged != null)
                    {
                        SelectedChanged(prev, Selection);
                    }
                }
            }
            else
            {
                Select(endNode);
            }
        }

        /// <summary>
        /// Select all expanded nodes
        /// </summary>
        public void SelectAllExpanded()
        {
            BulkSelectUpdateExpanded(true);
        }

        /// <summary>
        /// Deselect all nodes
        /// </summary>
        public void DeselectAll()
        {
            BulkSelectUpdateExpanded(false);
        }

        /// <inheritdoc />
        public override void Update(float deltaTime)
        {
            TreeNode node = SelectedNode;

            // Check if has focus and if any node is focused and it isn't a root
            if (ContainsFocus && node != null && node.AutoFocus)
            {
                RootControl window = Root;
                if (window.GetKeyDown(KeyboardKeys.ArrowUp) || window.GetKeyDown(KeyboardKeys.ArrowDown))
                {
                    m_KeyUpdateTime = KeyUpdateTimeout;
                }

                WindowRootControl windowRoot = window as WindowRootControl;
                if (m_KeyUpdateTime >= KeyUpdateTimeout && windowRoot != null && windowRoot.Window.IsFocused)
                {
                    bool keyUpArrow = window.GetKey(KeyboardKeys.ArrowUp);
                    bool keyDownArrow = window.GetKey(KeyboardKeys.ArrowDown);

                    // Check if arrow flags are different
                    if (keyDownArrow != keyUpArrow)
                    {
                        ContainerControl nodeParent = node.Parent;
                        TreeNode parentNode = nodeParent as TreeNode;
                        int myIndex = nodeParent.GetChildIndex(node);
                        SE.Debug.Assert(myIndex != -1, "Selected node is missing from its parent.");

                        // Up
                        TreeNode toSelect = null;
                        if (keyUpArrow)
                        {
                            if (myIndex == 0)
                            {
                                // Select parent
                                toSelect = parentNode;
                            }
                            else
                            {
                                // Select previous parent child
                                toSelect = nodeParent.GetChild(myIndex - 1) as TreeNode;

                                // Select last child if is valid and expanded and has any children
                                if (toSelect != null && toSelect.IsExpanded && toSelect.HasAnyVisibleChild)
                                {
                                    toSelect = toSelect.GetChild(toSelect.ChildrenCount - 1) as TreeNode;
                                }
                            }
                        }
                        // Down
                        else
                        {
                            if (node.IsExpanded && node.HasAnyVisibleChild)
                            {
                                // Select the first child
                                toSelect = node.GetChild(0) as TreeNode;
                            }
                            else if (myIndex == nodeParent.ChildrenCount - 1)
                            {
                                // Select next node after parent
                                while (parentNode != null && toSelect == null)
                                {
                                    int parentIndex = parentNode.IndexInParent;
                                    if (parentIndex != -1 && parentIndex < parentNode.Parent.ChildrenCount - 1)
                                    {
                                        toSelect = parentNode.Parent.GetChild(parentIndex + 1) as TreeNode;
                                    }
                                    parentNode = parentNode.Parent as TreeNode;
                                }
                            }
                            else
                            {
                                // Select next parent child
                                toSelect = nodeParent.GetChild(myIndex + 1) as TreeNode;
                            }
                        }
                        if (toSelect != null && toSelect.AutoFocus)
                        {
                            // Select
                            Select(toSelect);
                            toSelect.Focus();
                        }

                        // Reset time
                        m_KeyUpdateTime = 0.0f;
                    }
                }
                else
                {
                    // Update time
                    m_KeyUpdateTime += deltaTime;
                }

                if (window.GetKeyDown(KeyboardKeys.ArrowRight))
                {
                    if (node.IsExpanded)
                    {
                        // Select first child if has
                        if (node.HasAnyVisibleChild)
                        {
                            Select(node.GetChild(0) as TreeNode);
                            node.Focus();
                        }
                    }
                    else
                    {
                        // Expand selected node
                        node.Expand();
                    }
                }
                else if (window.GetKeyDown(KeyboardKeys.ArrowLeft))
                {
                    if (node.IsCollapsed)
                    {
                        // Select parent if has and is not a root
                        TreeNode nodeParentNode = node.Parent as TreeNode;
                        if (node.HasParent && nodeParentNode != null && nodeParentNode.AutoFocus)
                        {
                            Select(nodeParentNode);
                            nodeParentNode.Focus();
                        }
                    }
                    else
                    {
                        // Collapse selected node
                        node.Collapse();
                    }
                }
            }

            base.Update(deltaTime);
        }

        /// <inheritdoc />
        public override bool OnKeyDown(KeyboardKeys key)
        {
            // Check if can use multi selection
            if (m_SupportMultiSelect)
            {
                /*InputOptions options = Editor.Instance.Options.Options.Input;

                // Select all expanded nodes
                if (options.SelectAll.Process(this))
                {
                    SelectAllExpanded();
                    return true;
                }
                else if (options.DeselectAll.Process(this))
                {
                    DeselectAll();
                    return true;
                }*/
            }

            return base.OnKeyDown(key);
        }

        /// <inheritdoc />
        public override void OnGetFocus()
        {
            // Reset timer
            m_KeyUpdateTime = 0;

            base.OnGetFocus();
        }

        /// <inheritdoc />
        public override void OnParentResized()
        {
            PerformLayout();

            base.OnParentResized();
        }

        /// <inheritdoc />
        protected override void PerformLayoutBeforeChildren()
        {
            if (m_AutoSize)
            {
                // Use max of parent clint area width and root node width
                ContainerControl parent = Parent;
                float tempWidth = parent != null ? Mathf.Max(parent.GetClientArea().Width, 0.0f) : 0.0f;
                for (int i = 0; i < m_Children.Count; i++)
                {
                    TreeNode node = m_Children[i] as TreeNode;
                    if (node != null && node.Visible)
                    {
                        tempWidth = Mathf.Max(node.MinimumWidth, tempWidth);
                    }
                }
                for (int i = 0; i < m_Children.Count; i++)
                {
                    TreeNode node = m_Children[i] as TreeNode;
                    if (node != null && node.Visible)
                    {
                        node.Width = tempWidth;
                    }
                }
                float marginWidth = m_Margin.Width;
                Width = tempWidth + marginWidth;
            }

            base.PerformLayoutBeforeChildren();
        }

        /// <inheritdoc />
        protected override void PerformLayoutAfterChildren()
        {
            base.PerformLayoutAfterChildren();

            // Arrange children
            float y = m_Margin.Top;
            for (int i = 0; i < m_Children.Count; i++)
            {
                TreeNode node = m_Children[i] as TreeNode;
                if (node != null && node.Visible)
                {
                    node.Location = new Float2(m_Margin.Left, y);
                    y += node.Height + TreeNode.DefaultNodeOffsetY;
                }
            }

            if (m_AutoSize)
            {
                // Update height based on the nodes
                float bottom = 0.0f;
                for (int i = 0; i < m_Children.Count; i++)
                {
                    TreeNode node = m_Children[i] as TreeNode;
                    if (node != null && node.Visible)
                    {
                        bottom = Mathf.Max(bottom, node.Bottom);
                    }
                }
                float marginBottom = m_Margin.Bottom;
                Height = bottom + marginBottom;
            }
        }

        private void WalkSelectExpandedTree(List<TreeNode> selection, TreeNode node)
        {
            for (int i = 0; i < node.ChildrenCount; i++)
            {
                TreeNode child = node.GetChild(i) as TreeNode;
                if (child != null)
                {
                    selection.Add(child);
                    if (child.IsExpanded)
                    {
                        WalkSelectExpandedTree(selection, child);
                    }
                }
            }
        }

        private void BulkSelectUpdateExpanded(bool select = true)
        {
            if (m_SupportMultiSelect)
            {
                // Cache previous state
                List<TreeNode> prev = new List<TreeNode>(Selection);

                // Update selection
                Selection.Clear();
                if (select)
                {
                    WalkSelectExpandedTree(Selection, m_Children[0] as TreeNode);
                }

                // Check if changed
                bool changed = Selection.Count != prev.Count;
                if (!changed)
                {
                    for (int i = 0; i < Selection.Count; i++)
                    {
                        if ((object)Selection[i] != (object)prev[i])
                        {
                            changed = true;
                            break;
                        }
                    }
                }
                if (changed)
                {
                    // Fire event
                    if (SelectedChanged != null)
                    {
                        SelectedChanged(prev, Selection);
                    }
                }
            }
        }

        private void WalkSelectRangeExpandedTree(List<TreeNode> selection, TreeNode node, ref Rectangle range)
        {
            for (int i = 0; i < node.ChildrenCount; i++)
            {
                TreeNode child = node.GetChild(i) as TreeNode;
                if (child != null && child.Visible)
                {
                    Float2 pos = child.PointToParent(this, Float2.One);
                    if (range.Contains(pos))
                    {
                        selection.Add(child);
                    }

                    Rectangle nodeArea = new Rectangle(pos, child.Size);
                    if (child.IsExpanded && range.Intersects(nodeArea))
                    {
                        WalkSelectRangeExpandedTree(selection, child, ref range);
                    }
                }
            }
        }

        private Rectangle CalcNodeRangeRect(TreeNode node)
        {
            Float2 pos = node.PointToParent(this, Float2.One);
            return new Rectangle(pos, new Float2(10000, 4));
        }
    }
}
