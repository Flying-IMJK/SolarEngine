using System;
using System.Collections.Generic;
using SE.GUI;
namespace SE.Editor.GUI
{
    public sealed class Tree : ContainerControl
    {
        public const float KeyUpdateTimeout = 0.25f;

        private readonly List<TreeNode> m_Nodes = new List<TreeNode>();
        private readonly List<TreeNode> m_Selection = new List<TreeNode>();
        private readonly bool m_SupportMultiSelect;
        private float m_KeyUpdateTime = KeyUpdateTimeout;
        private Margin m_Margin;

        public Tree(bool supportMultiSelect = false)
            : base(new Rectangle(0, 0, 240, 320))
        {
            m_SupportMultiSelect = supportMultiSelect;
            AutoSize = true;
            SetBounds(0, 0, 240, 320);
        }

        public event Action<IReadOnlyList<TreeNode>, IReadOnlyList<TreeNode>>? SelectedChanged;
        public event Action<TreeNode, Float2>? RightClick;

        public IReadOnlyList<TreeNode> Selection => m_Selection;
        public TreeNode? SelectedNode => m_Selection.Count > 0 ? m_Selection[0] : null;
        public TreeNode? DraggedOverNode { get; set; }

        public Margin Margin
        {
            get => m_Margin;
            set
            {
                m_Margin = value;
                PerformLayout();
            }
        }

        public bool AutoSize { get; set; }

        public void OnRightClickInternal(TreeNode node, Float2 location)
        {
            RightClick?.Invoke(node, location);
        }

        public void Select(TreeNode node)
        {
            ArgumentNullException.ThrowIfNull(node);

            Select(new[] { node });
            node.Focus();
        }

        public void Select(IEnumerable<TreeNode> nodes)
        {
            ArgumentNullException.ThrowIfNull(nodes);

            List<TreeNode> before = new List<TreeNode>(m_Selection);
            m_Selection.Clear();
            foreach (TreeNode node in nodes)
            {
                if (!m_Selection.Contains(node))
                {
                    m_Selection.Add(node);
                    node.ExpandAllParents();
                }
                if (!m_SupportMultiSelect)
                    break;
            }

            RaiseSelectionChanged(before);
        }

        public void Deselect()
        {
            DeselectAll();
        }

        public void AddOrRemoveSelection(TreeNode node)
        {
            if (!m_SupportMultiSelect)
            {
                Select(node);
                return;
            }

            List<TreeNode> before = new List<TreeNode>(m_Selection);
            if (!m_Selection.Remove(node))
                m_Selection.Add(node);
            RaiseSelectionChanged(before);
        }

        public void SelectRange(TreeNode endNode)
        {
            if (!m_SupportMultiSelect || SelectedNode == null)
            {
                Select(endNode);
                return;
            }

            List<TreeNode> expanded = GetExpandedNodes();
            int start = expanded.IndexOf(SelectedNode);
            int end = expanded.IndexOf(endNode);
            if (start < 0 || end < 0)
                return;

            if (start > end)
                (start, end) = (end, start);

            Select(expanded.GetRange(start, end - start + 1));
        }

        public void SelectAllExpanded()
        {
            if (!m_SupportMultiSelect)
                return;

            Select(GetExpandedNodes());
        }

        public void DeselectAll()
        {
            List<TreeNode> before = new List<TreeNode>(m_Selection);
            m_Selection.Clear();
            RaiseSelectionChanged(before);
        }

        internal void RemoveFromSelection(TreeNode node)
        {
            List<TreeNode> before = new List<TreeNode>(m_Selection);
            if (m_Selection.Remove(node))
                RaiseSelectionChanged(before);
        }

        public override bool OnKeyDown(KeyboardKeys key)
        {
            // Flax uses Editor.Options.Input bindings for SelectAll, SelectInvert, and DeselectAll.
            // Keep those Editor shortcut bindings disabled until the managed options API is available.
            return base.OnKeyDown(key);
        }

        public override void OnGetFocus()
        {
            m_KeyUpdateTime = 0.0f;
            base.OnGetFocus();
        }

        public override void Update(float deltaTime)
        {
            TreeNode? node = SelectedNode;
            RootControl? root = Root;

            if (root != null && node != null && node.AutoFocus && ContainsFocus)
            {
                if (root.GetKeyDown(KeyboardKeys.ArrowUp) || root.GetKeyDown(KeyboardKeys.ArrowDown))
                    m_KeyUpdateTime = KeyUpdateTimeout;

                if (m_KeyUpdateTime >= KeyUpdateTimeout)
                {
                    bool keyUpArrow = root.GetKey(KeyboardKeys.ArrowUp);
                    bool keyDownArrow = root.GetKey(KeyboardKeys.ArrowDown);

                    if (keyDownArrow != keyUpArrow)
                    {
                        TreeNode? toSelect = FindVerticalNavigationTarget(node, keyUpArrow);
                        if (toSelect != null && toSelect.AutoFocus)
                        {
                            Select(toSelect);
                            toSelect.Focus();
                        }

                        m_KeyUpdateTime = 0.0f;
                    }
                }
                else
                {
                    m_KeyUpdateTime += deltaTime;
                }

                if (root.GetKeyDown(KeyboardKeys.ArrowRight))
                {
                    if (node.IsExpanded)
                    {
                        TreeNode? child = GetFirstVisibleChild(node);
                        if (child != null)
                        {
                            Select(child);
                            child.Focus();
                        }
                    }
                    else
                    {
                        node.Expand();
                    }
                }
                else if (root.GetKeyDown(KeyboardKeys.ArrowLeft))
                {
                    if (node.IsCollapsed)
                    {
                        if (node.Parent is TreeNode parentNode && parentNode.AutoFocus)
                        {
                            Select(parentNode);
                            parentNode.Focus();
                        }
                    }
                    else
                    {
                        node.Collapse();
                    }
                }
            }

            base.Update(deltaTime);
        }

        protected override void OnLayoutChildren()
        {
            m_Nodes.Clear();
            foreach (Control control in Children)
            {
                if (control is TreeNode node)
                    m_Nodes.Add(node);
            }

            float y = m_Margin.Top;
            float width = Math.Max(0, Width - m_Margin.Width);
            foreach (TreeNode node in m_Nodes)
            {
                if (!node.Visible)
                    continue;

                node.SetBounds(m_Margin.Left, y, width, node.Height);
                y += node.Height;
            }

            if (AutoSize)
            {
                Height = y + m_Margin.Bottom;
            }
        }

        protected override void OnChildAdded(Control control)
        {
            if (control is TreeNode node && !m_Nodes.Contains(node))
                m_Nodes.Add(node);
            base.OnChildAdded(control);
        }

        protected override void OnChildRemoved(Control control)
        {
            if (control is TreeNode node)
            {
                m_Nodes.Remove(node);
                m_Selection.Remove(node);
            }
            base.OnChildRemoved(control);
        }

        private List<TreeNode> GetExpandedNodes()
        {
            List<TreeNode> result = new List<TreeNode>();
            foreach (TreeNode node in m_Nodes)
            {
                WalkExpanded(node, result);
            }

            return result;
        }

        private TreeNode? FindVerticalNavigationTarget(TreeNode node, bool up)
        {
            List<TreeNode> expanded = GetExpandedNodes();
            int index = expanded.IndexOf(node);
            if (index < 0)
                return null;

            index += up ? -1 : 1;
            if ((uint)index >= (uint)expanded.Count)
                return null;

            return expanded[index];
        }

        private static TreeNode? GetFirstVisibleChild(TreeNode node)
        {
            foreach (Control control in node.Children)
            {
                if (control is not TreeNode child)
                    continue;
                if (child.Visible)
                    return child;
            }

            return null;
        }

        private static void WalkExpanded(TreeNode node, List<TreeNode> result)
        {
            result.Add(node);
            if (!node.IsExpanded)
                return;

            foreach (Control control in node.Children)
            {
                if (control is not TreeNode child)
                    continue;
                WalkExpanded(child, result);
            }
        }

        private void RaiseSelectionChanged(List<TreeNode> before)
        {
            if (!SequenceEqual(before, m_Selection))
                SelectedChanged?.Invoke(before, m_Selection);
        }

        private static bool SequenceEqual(IReadOnlyList<TreeNode> before, IReadOnlyList<TreeNode> after)
        {
            if (before.Count != after.Count)
                return false;

            for (int i = 0; i < before.Count; i++)
            {
                if (!ReferenceEquals(before[i], after[i]))
                    return false;
            }

            return true;
        }
    }
}
