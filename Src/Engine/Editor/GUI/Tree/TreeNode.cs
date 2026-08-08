using System.Collections.Generic;
using System.Linq;
using SE;
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
        private const MouseButton LeftMouseButton = MouseButton.Left;
        private const MouseButton RightMouseButton = MouseButton.Right;
        public const float DefaultDragInsertPositionMargin = 3.0f;
        public const float DefaultNodeOffsetY = 0.0f;

        private readonly List<TreeNode> m_Nodes = new List<TreeNode>();
        private readonly Label m_HeaderText;
        private bool m_IsExpanded;
        private string m_Text = string.Empty;
        private float m_HeaderHeight = 16.0f;
        private Margin m_TextMargin = new Margin(2.0f);
        private DragItemPositioning m_DragOverMode;
        private bool m_MouseOverArrow;
        private bool m_MouseOverHeader;
        private bool m_IsMouseDown;
        private float m_MouseDownTime;
        private Float2 m_MouseDownPos;

        private bool m_IsDragOverHeader;
        private static ulong m_DragEndFrame;

        public TreeNode(bool canChangeOrder = false)
            : base(new Rectangle(0, 0, 200, 16))
        {
            CanChangeOrder = canChangeOrder;
            AnimationProgress = 1.0f;
            m_MouseDownTime = -1.0f;
            m_HeaderText = new Label(new Rectangle(0, 0, 200, m_HeaderHeight), string.Empty)
            {
                AutoFocus = false,
                Enabled = false,
            };
            AddChild(m_HeaderText);
            SetBounds(0, 0, 200, m_HeaderHeight);
        }

        public IReadOnlyList<TreeNode> Nodes => m_Nodes;
        public bool CanChangeOrder { get; }
        public float ChildrenIndent { get; set; } = 12.0f;
        public float AnimationProgress { get; private set; }
        public bool HasAnyVisibleChild => m_Nodes.Exists(static node => node.Visible);
        public DragItemPositioning DragOverMode => m_DragOverMode;
        public Tree? ParentTree => FindParentTree();
        public bool IsRoot => Parent is Tree;
        public float MinimumWidth => CalculateMinimumWidth();
        public Rectangle ArrowRect => new Rectangle(0, 0, m_HeaderHeight, m_HeaderHeight);
        public Rectangle HeaderRect => new Rectangle(0, 0, Width, m_HeaderHeight);
        public Rectangle TextRect => new Rectangle(m_TextMargin.Left, 0, Width - m_TextMargin.Width, m_HeaderHeight);

        public string Text
        {
            get => m_Text;
            set
            {
                m_Text = value;
                m_HeaderText.Text = value;
            }
        }

        public bool IsExpanded
        {
            get => m_IsExpanded;
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
            get => !m_IsExpanded;
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

        public Margin TextMargin
        {
            get => m_TextMargin;
            set
            {
                m_TextMargin = value;
                PerformLayout();
            }
        }

        public float HeaderHeight
        {
            get => m_HeaderHeight;
            set
            {
                m_HeaderHeight = value;
                PerformLayout();
            }
        }

        public TreeNode AddNode(TreeNode node)
        {
            m_Nodes.Add(node);
            AddChild(node);
            PerformLayout();
            RequestTreeLayout();
            return node;
        }

        /// <summary>
        /// Removes and disposes all child nodes. Dynamic editor trees use this when
        /// a filesystem refresh rebuilds a branch.
        /// </summary>
        public void ClearNodes()
        {
            TreeNode[] nodes = m_Nodes.ToArray();
            m_Nodes.Clear();
            foreach (TreeNode node in nodes)
            {
                RemoveChild(node);
                node.Dispose();
            }
            PerformLayout();
            RequestTreeLayout();
        }

        public void Expand(bool noAnimation = false)
        {
            ExpandAllParents(noAnimation);
            if (m_IsExpanded)
                return;

            m_IsExpanded = true;
            AnimationProgress = noAnimation ? 1.0f : 0.0f;
            OnExpandedChanged();
            PerformLayout();
            RequestTreeLayout();
        }

        public void Collapse(bool noAnimation = false)
        {
            if (!m_IsExpanded)
                return;

            m_IsExpanded = false;
            AnimationProgress = noAnimation ? 0.0f : 1.0f;
            OnExpandedChanged();
            PerformLayout();
            RequestTreeLayout();
        }

        public void ExpandAll(bool noAnimation = false)
        {
            Expand(noAnimation);
            foreach (TreeNode child in m_Nodes)
            {
                child.ExpandAll(noAnimation);
            }
        }

        public void CollapseAll(bool noAnimation = false)
        {
            Collapse(noAnimation);
            foreach (TreeNode child in m_Nodes)
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
            AnimationProgress = m_IsExpanded ? 1.0f : 0.0f;
            OnExpandAnimationChanged();
        }

        public void Select()
        {
            ParentTree?.Select(this);
        }

        public override void Update(float deltaTime)
        {
            const float longPressTimeSeconds = 0.6f;
            if (m_IsMouseDown && Time.GetUnscaledGameTime() - m_MouseDownTime > longPressTimeSeconds)
                OnLongPress();

            if (m_IsExpanded)
                base.Update(deltaTime);
        }

        public override void OnDragLeave()
        {
            if (m_IsDragOverHeader)
            {
                m_IsDragOverHeader = false;
                OnDragLeaveHeader();
            }

            ClearDragPositioning();
            base.OnDragLeave();
        }

        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            UpdateMouseOverFlags(location);

            // Check if mouse hits bar and node isn't a root
            if (m_MouseOverHeader)
            {
                // Check if left button goes down
                if (button == MouseButton.Left)
                {
                    m_IsMouseDown = true;
                    m_MouseDownPos = location;
                    m_MouseDownTime = Time.GetUnscaledGameTime();
                }

                // Handled
                Root?.Focus(this);
                return true;
            }

            // Base
            if (m_IsExpanded)
                return base.OnMouseDown(location, button);

            // Handled
            Root?.Focus(this);
            return true;
        }

        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            UpdateMouseOverFlags(location);

            // Clear flag for left button
            if (button == MouseButton.Left && m_IsMouseDown)
            {
                m_IsMouseDown = false;
                m_MouseDownTime = -1.0f;
            }

            // Check if mouse hits bar and node isn't a root
            if (m_MouseOverHeader)
            {
                // Skip mouse up event right after drag drop ends
                if (button == MouseButton.Left && Engine.FrameCount - m_DragEndFrame < 10)
                    return true;

                // Prevent from selecting node when user is just clicking at an arrow
                Tree? tree = ParentTree;
                if (!m_MouseOverArrow && tree != null)
                {
                    RootControl? window = tree.Root;
                    if (window?.GetKey(KeyboardKeys.Shift) == true)
                    {
                        // Select range
                        tree.SelectRange(this);
                    }
                    else if (window?.GetKey(KeyboardKeys.Control) == true)
                    {
                        // Add/Remove
                        tree.AddOrRemoveSelection(this);
                    }
                    else if (button == MouseButton.Right && tree.Selection.Contains(this))
                    {
                        // Do nothing
                    }
                    else
                    {
                        // Select
                        tree.Select(this);
                    }
                }

                // Check if mouse hits arrow
                if (m_MouseOverArrow && HasAnyVisibleChild)
                {
                    if (Root?.GetKey(KeyboardKeys.Alt) == true)
                    {
                        if (m_IsExpanded)
                            CollapseAll();
                        else
                            ExpandAll();
                    }
                    else
                    {
                        if (m_IsExpanded)
                            Collapse();
                        else
                            Expand();
                    }
                }

                // Check if mouse hits bar
                if (button == MouseButton.Right && TestHeaderHit(ref location))
                    tree?.OnRightClickInternal(this, location);

                // Handled
                Root?.Focus(this);
                return true;
            }

            // Check if mouse hits bar
            if (button == MouseButton.Right && TestHeaderHit(ref location))
                ParentTree?.OnRightClickInternal(this, location);

            // Base
            return base.OnMouseUp(location, button);
        }

        public override bool OnMouseDoubleClick(Float2 location, MouseButton button)
        {
            if (TestHeaderHit(ref location))
                return OnMouseDoubleClickHeader(ref location, button);

            if (AnimationProgress >= 1.0f)
                return base.OnMouseDoubleClick(location, button);

            return false;
        }

        public override void OnMouseMove(Float2 location)
        {
            UpdateMouseOverFlags(location);

            // Check if start drag and drop
            if (m_IsMouseDown && Float2.Distance(m_MouseDownPos, location) > 10.0f)
            {
                // Clear flag
                m_IsMouseDown = false;
                m_MouseDownTime = -1.0f;

                // Start
                BeginDragDrop();
                return;
            }

            // Check if animation has been finished
            if (AnimationProgress >= 1.0f && m_IsExpanded)
                base.OnMouseMove(location);
        }

        public override void OnMouseLeave()
        {
            // Clear flags
            m_MouseOverArrow = false;
            m_MouseOverHeader = false;

            // Check if start drag and drop
            if (m_IsMouseDown)
            {
                // Clear flag
                m_IsMouseDown = false;
                m_MouseDownTime = -1.0f;

                // Start
                BeginDragDrop();
            }

            base.OnMouseLeave();
        }

        public override bool OnKeyDown(KeyboardKeys key)
        {
            if (m_IsExpanded)
                return base.OnKeyDown(key);
            return false;
        }

        public override bool OnKeyUp(KeyboardKeys key)
        {
            if (m_IsExpanded)
                return base.OnKeyUp(key);
            return false;
        }

        /// <inheritdoc />
        public override DragDropEffect OnDragEnter(ref Float2 location, DragData data)
        {
            var result = base.OnDragEnter(ref location, data);

            // Check if no children handled that event
            m_DragOverMode = DragItemPositioning.None;
            if (result == DragDropEffect.None)
            {
                UpdateDragPositioning(ref location);

                // Check if mouse is over header
                m_IsDragOverHeader = TestHeaderHit(ref location);
                if (m_IsDragOverHeader)
                {
                    if (ParentTree != null)
                        ParentTree.DraggedOverNode = this;

                    // Expand node if mouse goes over arrow
                    if (ArrowRect.Contains(location) && HasAnyVisibleChild)
                    {
                        Expand(true);
                    }

                    result = OnDragEnterHeader(data);
                }

                if (result == DragDropEffect.None)
                {
                    m_DragOverMode = DragItemPositioning.None;
                }
            }

            return result;
        }

        /// <inheritdoc />
        public override DragDropEffect OnDragMove(ref Float2 location, DragData data)
        {
            var result = base.OnDragMove(ref location, data);

            // Check if no children handled that event
            ClearDragPositioning();
            if (result == DragDropEffect.None)
            {
                UpdateDragPositioning(ref location);

                // Check if mouse is over header
                bool isDragOverHeader = TestHeaderHit(ref location);
                if (isDragOverHeader)
                {
                    if (ParentTree != null)
                        ParentTree.DraggedOverNode = this;

                    // Expand node if mouse goes over arrow
                    if (ArrowRect.Contains(location) && HasAnyVisibleChild)
                        Expand(true);

                    if (!m_IsDragOverHeader)
                        result = OnDragEnterHeader(data);
                    else
                        result = OnDragMoveHeader(data);
                }
                else if (m_IsDragOverHeader)
                {
                    OnDragLeaveHeader();
                }
                m_IsDragOverHeader = isDragOverHeader;

                if (result == DragDropEffect.None)
                {
                    m_DragOverMode = DragItemPositioning.None;
                }
            }

            return result;
        }

        /// <inheritdoc />
        public override DragDropEffect OnDragDrop(ref Float2 location, DragData data)
        {
            var result = base.OnDragDrop(ref location, data);

            // Check if no children handled that event
            if (result == DragDropEffect.None)
            {
                UpdateDragPositioning(ref location);
                m_DragEndFrame = Engine.FrameCount;

                // Check if mouse is over header
                if (TestHeaderHit(ref location))
                {
                    result = OnDragDropHeader(data);
                }
            }

            // Clear cache
            m_IsDragOverHeader = false;
            ClearDragPositioning();

            return result;
        }

        protected virtual DragDropEffect OnDragEnterHeader(DragData data) => DragDropEffect.None;
        protected virtual DragDropEffect OnDragMoveHeader(DragData data) => DragDropEffect.None;
        protected virtual DragDropEffect OnDragDropHeader(DragData data) => DragDropEffect.None;
        protected virtual void OnDragLeaveHeader() { }
        protected virtual void BeginDragDrop() { }
        protected virtual bool OnMouseDoubleClickHeader(ref Float2 location, MouseButton button)
        {
            _ = location;
            _ = button;

            if (HasAnyVisibleChild)
            {
                if (m_IsExpanded)
                    Collapse();
                else
                    Expand();
            }

            return true;
        }
        protected virtual void OnLongPress() { }
        protected virtual void OnExpandedChanged() { }
        protected virtual void OnExpandAnimationChanged() { }
        protected virtual bool TestHeaderHit(ref Float2 location) => HeaderRect.Contains(ref location);

        protected override void OnLayoutChildren()
        {
            m_HeaderText.SetBounds(m_TextMargin.Left, 0, Width - m_TextMargin.Width, m_HeaderHeight);

            float y = m_HeaderHeight + DefaultNodeOffsetY;
            foreach (TreeNode child in m_Nodes)
            {
                child.Visible = m_IsExpanded;
                if (!m_IsExpanded)
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
            float width = Text.Length * 7.0f + m_TextMargin.Width;
            foreach (TreeNode child in m_Nodes)
            {
                width = System.Math.Max(width, ChildrenIndent + child.MinimumWidth);
            }

            return width;
        }

        private void UpdateDragPositioning(ref Float2 location)
        {
            if (location.Y < DefaultDragInsertPositionMargin) m_DragOverMode = DragItemPositioning.Above;
            else if ((!m_IsExpanded || !HasAnyVisibleChild) && location.Y > HeaderHeight - DefaultDragInsertPositionMargin)
            {
                m_DragOverMode = DragItemPositioning.Below;
            }
            else
            {
                m_DragOverMode = DragItemPositioning.At;
            }

            Tree? tree = ParentTree;
            if (tree != null)
            {
                tree.DraggedOverNode = this;
            }
        }

        private void ClearDragPositioning()
        {
            m_DragOverMode = DragItemPositioning.None;
            Tree? tree = ParentTree;
            if (tree != null && ReferenceEquals(tree.DraggedOverNode, this))
                tree.DraggedOverNode = null;
        }

        private void RequestTreeLayout()
        {
            ParentTree?.PerformLayout();
        }

        private void UpdateMouseOverFlags(Float2 location)
        {
            // Cache flags
            m_MouseOverArrow = HasAnyVisibleChild && ArrowRect.Contains(location);
            m_MouseOverHeader = new Rectangle(0, 0, Width, m_HeaderHeight - 1).Contains(location);
            if (m_MouseOverHeader)
            {
                // Allow non-scrollable controls to stay on top of the header and override the mouse behaviour
                for (int i = 0; i < Children.Count; i++)
                {
                    Control child = Children[i];
                    if (!child.IsScrollable && child.EnabledInHierarchy && child.VisibleInHierarchy && child.Bounds.Contains(location))
                    {
                        m_MouseOverHeader = false;
                        break;
                    }
                }
            }
        }
    }
}
