using System.Collections.Generic;
using SE.GUI;
namespace SE.Editor.GUI
{
    public enum DragItemPositioning
    {
        None = 0,
        At,
        Above,
        Below,
    }

    public class TreeNode : ContainerControl
    {
        private const int LeftMouseButton = 1;
        private const int RightMouseButton = 3;
        public const float DefaultDragInsertPositionMargin = 3.0f;
        public const float DefaultNodeOffsetY = 0.0f;

        private readonly List<TreeNode> _nodes = new List<TreeNode>();
        private readonly Label _headerText;
        private bool _isExpanded;
        private string _text = string.Empty;
        private float _headerHeight = 16.0f;
        private GuiMargin _textMargin = new GuiMargin(2.0f);
        private DragItemPositioning _dragOverMode;

        public TreeNode(bool canChangeOrder = false)
            : base(new Rectangle(0, 0, 200, 16))
        {
            CanChangeOrder = canChangeOrder;
            _headerText = new Label(new Rectangle(0, 0, 200, _headerHeight), string.Empty)
            {
                AutoFocus = false,
                Enabled = false,
            };
            AddChild(_headerText);
            SetBounds(0, 0, 200, _headerHeight);
        }

        public IReadOnlyList<TreeNode> Nodes => _nodes;
        public bool CanChangeOrder { get; }
        public float ChildrenIndent { get; set; } = 12.0f;
        public float AnimationProgress { get; private set; }
        public bool HasAnyVisibleChild => _nodes.Exists(static node => node.Visible);
        public DragItemPositioning DragOverMode => _dragOverMode;
        public Tree? ParentTree => FindParentTree();
        public bool IsRoot => Parent is Tree;
        public float MinimumWidth => CalculateMinimumWidth();
        public GuiRect ArrowRect => new GuiRect(0, 0, _headerHeight, _headerHeight);
        public GuiRect HeaderRect => new GuiRect(0, 0, Width, _headerHeight);
        public GuiRect TextRect => new GuiRect(_textMargin.Left, 0, Width - _textMargin.Width, _headerHeight);

        public string Text
        {
            get => _text;
            set
            {
                _text = value;
                _headerText.Text = value;
            }
        }

        public bool IsExpanded
        {
            get => _isExpanded;
            set
            {
                if (value)
                    Expand(true);
                else
                    Collapse(true);
            }
        }

        public bool IsCollapsed
        {
            get => !_isExpanded;
            set => IsExpanded = !value;
        }

        public bool IsCollapsedInHierarchy
        {
            get
            {
                TreeNode? node = this;
                while (node != null)
                {
                    if (!node.IsExpanded && node.Nodes.Count > 0)
                        return true;
                    node = node.Parent as TreeNode;
                }

                return false;
            }
        }

        public GuiMargin TextMargin
        {
            get => _textMargin;
            set
            {
                _textMargin = value;
                PerformLayout();
            }
        }

        public float HeaderHeight
        {
            get => _headerHeight;
            set
            {
                _headerHeight = value;
                PerformLayout();
            }
        }

        public TreeNode AddNode(TreeNode node)
        {
            _nodes.Add(node);
            AddChild(node);
            PerformLayout();
            RequestTreeLayout();
            return node;
        }

        public void Expand(bool noAnimation = false)
        {
            if (_isExpanded)
                return;

            _isExpanded = true;
            AnimationProgress = noAnimation ? 1.0f : 0.0f;
            OnExpandedChanged();
            PerformLayout();
            RequestTreeLayout();
        }

        public void Collapse(bool noAnimation = false)
        {
            if (!_isExpanded)
                return;

            _isExpanded = false;
            AnimationProgress = noAnimation ? 0.0f : 1.0f;
            OnExpandedChanged();
            PerformLayout();
            RequestTreeLayout();
        }

        public void ExpandAll(bool noAnimation = false)
        {
            Expand(noAnimation);
            foreach (TreeNode child in _nodes)
            {
                child.ExpandAll(noAnimation);
            }
        }

        public void CollapseAll(bool noAnimation = false)
        {
            Collapse(noAnimation);
            foreach (TreeNode child in _nodes)
            {
                child.CollapseAll(noAnimation);
            }
        }

        public void ExpandAllParents(bool noAnimation = false)
        {
            TreeNode? parent = Parent as TreeNode;
            while (parent != null)
            {
                parent.Expand(noAnimation);
                parent = parent.Parent as TreeNode;
            }
        }

        public void EndAnimation()
        {
            AnimationProgress = _isExpanded ? 1.0f : 0.0f;
            OnExpandAnimationChanged();
        }

        public void Select()
        {
            ParentTree?.Select(this);
        }

        public virtual DragDropEffect OnDragEnter(DragData data, GuiPoint location)
        {
            UpdateDragPositioning(location);
            return _dragOverMode == DragItemPositioning.At ? OnDragEnterHeader(data) : DragDropEffect.Move;
        }

        public virtual DragDropEffect OnDragMove(DragData data, GuiPoint location)
        {
            UpdateDragPositioning(location);
            return _dragOverMode == DragItemPositioning.At ? OnDragMoveHeader(data) : DragDropEffect.Move;
        }

        public virtual DragDropEffect OnDragDrop(DragData data, GuiPoint location)
        {
            UpdateDragPositioning(location);
            DragDropEffect effect = _dragOverMode == DragItemPositioning.At ? OnDragDropHeader(data) : DragDropEffect.Move;
            ClearDragPositioning();
            return effect;
        }

        public override void OnDragLeave()
        {
            ClearDragPositioning();
            OnDragLeaveHeader();
        }

        public override bool OnMouseDown(Float2 location, int button)
        {
            GuiPoint point = new GuiPoint(location.X, location.Y);
            if (button == LeftMouseButton && HeaderRect.Contains(point))
            {
                ParentTree?.Select(this);
                if (ArrowRect.Contains(point) && Nodes.Count > 0)
                    IsExpanded = !IsExpanded;
                return true;
            }

            if (button == RightMouseButton && HeaderRect.Contains(point))
            {
                ParentTree?.OnRightClickInternal(this, point);
                return true;
            }

            return base.OnMouseDown(location, button);
        }

        public override bool OnMouseDoubleClick(Float2 location, int button)
        {
            if (button != LeftMouseButton)
                return base.OnMouseDoubleClick(location, button);

            GuiPoint point = new GuiPoint(location.X, location.Y);
            if (!HeaderRect.Contains(point))
                return false;

            if (!OnMouseDoubleClickHeader(point) && Nodes.Count > 0)
                IsExpanded = !IsExpanded;
            return true;
        }

        public override SE.GUI.DragDropEffect OnDragEnter(Float2 location, SE.GUI.DragData data)
        {
            return ToRuntimeEffect(OnDragEnter(ToEditorDragData(data), new GuiPoint(location.X, location.Y)));
        }

        public override SE.GUI.DragDropEffect OnDragMove(Float2 location, SE.GUI.DragData data)
        {
            return ToRuntimeEffect(OnDragMove(ToEditorDragData(data), new GuiPoint(location.X, location.Y)));
        }

        public override SE.GUI.DragDropEffect OnDragDrop(Float2 location, SE.GUI.DragData data)
        {
            return ToRuntimeEffect(OnDragDrop(ToEditorDragData(data), new GuiPoint(location.X, location.Y)));
        }

        protected virtual DragDropEffect OnDragEnterHeader(DragData data) => DragDropEffect.None;
        protected virtual DragDropEffect OnDragMoveHeader(DragData data) => DragDropEffect.None;
        protected virtual DragDropEffect OnDragDropHeader(DragData data) => DragDropEffect.None;
        protected virtual void OnDragLeaveHeader() { }
        protected virtual void BeginDragDrop() { }
        protected virtual bool OnMouseDoubleClickHeader(GuiPoint location) => false;
        protected virtual void OnLongPress() { }
        protected virtual void OnExpandedChanged() { }
        protected virtual void OnExpandAnimationChanged() { }

        protected override void OnLayoutChildren()
        {
            _headerText.SetBounds(_textMargin.Left, 0, Width - _textMargin.Width, _headerHeight);

            float y = _headerHeight + DefaultNodeOffsetY;
            foreach (TreeNode child in _nodes)
            {
                child.Visible = _isExpanded;
                if (!_isExpanded)
                    continue;

                child.SetBounds(ChildrenIndent, y, Width - ChildrenIndent, child.Height);
                y += child.Height;
            }

            Height = y;
        }

        private Tree? FindParentTree()
        {
            SE.GUI.Control? current = Parent;
            while (current != null)
            {
                if (current is Tree tree)
                    return tree;
                current = current.Parent;
            }

            return null;
        }

        private float CalculateMinimumWidth()
        {
            float width = Text.Length * 7.0f + _textMargin.Width;
            foreach (TreeNode child in _nodes)
            {
                width = System.Math.Max(width, ChildrenIndent + child.MinimumWidth);
            }

            return width;
        }

        private void UpdateDragPositioning(GuiPoint location)
        {
            if (location.Y < DefaultDragInsertPositionMargin)
                _dragOverMode = DragItemPositioning.Above;
            else if (location.Y > HeaderHeight - DefaultDragInsertPositionMargin)
                _dragOverMode = DragItemPositioning.Below;
            else
                _dragOverMode = DragItemPositioning.At;

            Tree? tree = ParentTree;
            if (tree != null)
                tree.DraggedOverNode = this;
        }

        private void ClearDragPositioning()
        {
            _dragOverMode = DragItemPositioning.None;
            Tree? tree = ParentTree;
            if (tree != null && ReferenceEquals(tree.DraggedOverNode, this))
                tree.DraggedOverNode = null;
        }

        private void RequestTreeLayout()
        {
            ParentTree?.PerformLayout();
        }

        private static DragData ToEditorDragData(SE.GUI.DragData data)
        {
            if (data is DragData editorData)
                return editorData;
            if (data is SE.GUI.DragDataText textData)
                return new DragDataText(textData.Text);

            return new UnsupportedDragData(data);
        }

        private static SE.GUI.DragDropEffect ToRuntimeEffect(DragDropEffect effect)
        {
            return effect switch
            {
                DragDropEffect.Copy => SE.GUI.DragDropEffect.Copy,
                DragDropEffect.Move => SE.GUI.DragDropEffect.Move,
                DragDropEffect.Link => SE.GUI.DragDropEffect.Link,
                _ => SE.GUI.DragDropEffect.None,
            };
        }

        private sealed class UnsupportedDragData : DragData
        {
            public UnsupportedDragData(SE.GUI.DragData source)
            {
                Source = source;
            }

            public SE.GUI.DragData Source { get; }
            public override SE.GUI.DragDataType DataType => Source.DataType;
        }
    }
}
