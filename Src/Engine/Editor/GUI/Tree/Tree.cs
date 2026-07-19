using System;
using System.Collections.Generic;
using SE.GUI;
namespace SE.Editor.GUI
{
    public sealed class Tree : ContainerControl
    {
        public const float KeyUpdateTimeout = 0.25f;

        private readonly List<TreeNode> _nodes = new List<TreeNode>();
        private readonly List<TreeNode> _selection = new List<TreeNode>();
        private GuiMargin _margin;

        public Tree(bool supportMultiSelect = false)
            : base(new Rectangle(0, 0, 240, 320))
        {
            SupportMultiSelect = supportMultiSelect;
            AutoSize = true;
            SetBounds(0, 0, 240, 320);
        }

        public event Action<IReadOnlyList<TreeNode>, IReadOnlyList<TreeNode>>? SelectedChanged;
        public event Action<TreeNode, GuiPoint>? RightClick;

        public IReadOnlyList<TreeNode> Nodes => _nodes;
        public IReadOnlyList<TreeNode> Selection => _selection;
        public TreeNode? SelectedNode => _selection.Count > 0 ? _selection[0] : null;
        public TreeNode? DraggedOverNode { get; set; }
        public bool SupportMultiSelect { get; }

        public GuiMargin Margin
        {
            get => _margin;
            set
            {
                _margin = value;
                PerformLayout();
            }
        }

        public bool AutoSize { get; set; }

        public TreeNode AddNode(TreeNode node)
        {
            _nodes.Add(node);
            AddChild(node);
            PerformLayout();
            return node;
        }

        public void OnRightClickInternal(TreeNode node, GuiPoint location)
        {
            RightClick?.Invoke(node, location);
        }

        public void Select(TreeNode node)
        {
            Select(new[] { node });
        }

        public void Select(IEnumerable<TreeNode> nodes)
        {
            List<TreeNode> before = new List<TreeNode>(_selection);
            _selection.Clear();
            foreach (TreeNode node in nodes)
            {
                if (!_selection.Contains(node))
                    _selection.Add(node);
                if (!SupportMultiSelect)
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
            if (!SupportMultiSelect)
            {
                Select(node);
                return;
            }

            List<TreeNode> before = new List<TreeNode>(_selection);
            if (!_selection.Remove(node))
                _selection.Add(node);
            RaiseSelectionChanged(before);
        }

        public void SelectRange(TreeNode endNode)
        {
            if (!SupportMultiSelect || SelectedNode == null)
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
            if (!SupportMultiSelect)
                return;

            Select(GetExpandedNodes());
        }

        public void DeselectAll()
        {
            List<TreeNode> before = new List<TreeNode>(_selection);
            _selection.Clear();
            RaiseSelectionChanged(before);
        }

        protected override void OnLayoutChildren()
        {
            float y = _margin.Top;
            float width = Math.Max(0, Width - _margin.Width);
            foreach (TreeNode node in _nodes)
            {
                if (!node.Visible)
                    continue;

                node.SetBounds(_margin.Left, y, width, node.Height);
                y += node.Height;
            }

            if (AutoSize)
            {
                Height = y + _margin.Bottom;
            }
        }

        private List<TreeNode> GetExpandedNodes()
        {
            List<TreeNode> result = new List<TreeNode>();
            foreach (TreeNode node in _nodes)
            {
                WalkExpanded(node, result);
            }

            return result;
        }

        private static void WalkExpanded(TreeNode node, List<TreeNode> result)
        {
            result.Add(node);
            if (!node.IsExpanded)
                return;

            foreach (TreeNode child in node.Nodes)
            {
                WalkExpanded(child, result);
            }
        }

        private void RaiseSelectionChanged(List<TreeNode> before)
        {
            if (!SequenceEqual(before, _selection))
                SelectedChanged?.Invoke(before, _selection);
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
