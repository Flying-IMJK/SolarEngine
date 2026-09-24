using System;
using System.Collections.Generic;
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

    /// <summary>
    /// Managed counterpart of the native tree node control.
    /// </summary>
    public class TreeNode : ContainerControl
    {
        public const float DefaultDragInsertPositionMargin = 3.0f;
        public const float DefaultNodeOffsetY = 0.0f;

        private readonly List<TreeNode> m_Nodes = new List<TreeNode>();
        protected readonly Label m_HeaderText;
        private readonly SpriteHandle m_IconCollapsed;
        private readonly SpriteHandle m_IconOpened;
        private Tree? m_Tree;
        private Tree? m_LastTree;
        private bool m_IsExpanded;
        private float m_AnimationProgress;
        private float m_CachedHeight;
        private float m_XOffset;
        private float m_TextWidth;
        private bool m_TextChanged;
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
            : this(canChangeOrder, SpriteHandle.Invalid, SpriteHandle.Invalid)
        {
        }

        public TreeNode(bool canChangeOrder, SpriteHandle iconCollapsed, SpriteHandle iconOpened)
            : base(new Rectangle(0, 0, 64, 16))
        {
            m_AnimationProgress = 1.0f;
            m_CachedHeight = m_HeaderHeight;
            m_IconCollapsed = iconCollapsed;
            m_IconOpened = iconOpened;
            m_MouseDownTime = -1.0f;

            Style style = Style.Current;
            TextColor = style.TextColor;
            BackgroundColorSelected = style.BackgroundSelected;
            BackgroundColorHighlighted = style.BackgroundHighlighted;
            BackgroundColorSelectedUnfocused = style.LightBackground;
            TextFont = new FontReference(style.FontSmall!);

            m_HeaderText = new Label(new Rectangle(0, 0, 64, m_HeaderHeight), string.Empty)
            {
                AutoFocus = false,
                Enabled = false,
                HorizontalAlignment = TextAlignment.Near,
                VerticalAlignment = TextAlignment.Center,
                TextWrapping = TextWrapping.NoWrap,
            };
            ParentChanged += OnParentChanged;
            AddChild(m_HeaderText);
        }

        public float ChildrenIndent { get; set; } = 12.0f;
        public bool HasAnyVisibleChild => m_Nodes.Exists(static node => node.Visible);
        public DragItemPositioning DragOverMode => m_DragOverMode;
        public Tree? ParentTree => GetParentTree();
        public bool IsRoot => Parent is not TreeNode;
        public float MinimumWidth => CalculateMinimumWidth();
        public Rectangle HeaderRect => new Rectangle(0, 0, Width, m_HeaderHeight);
        public Rectangle ArrowRect => CustomArrowRect ?? new Rectangle(m_XOffset + 2.0f + m_TextMargin.Left, 2.0f, 12.0f, 12.0f);
        public Rectangle TextRect => CalculateTextRect();
        public Rectangle? CustomArrowRect { get; set; }
        public Color TextColor { get; set; }
        public FontReference TextFont { get; set; }
        public Color BackgroundColorSelected { get; set; }
        public Color BackgroundColorHighlighted { get; set; }
        public Color BackgroundColorSelectedUnfocused { get; set; }
        public Color IconColor { get; set; } = Color.White;

        public string Text
        {
            get => m_Text;
            set
            {
                m_Text = value ?? string.Empty;
                m_TextChanged = true;
                m_HeaderText.Text = m_Text;
                PerformLayout();
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
            set
            {
                if (value)
                    Collapse(true);
                else
                    Expand(true);
            }
        }

        public bool IsCollapsedInHierarchy => IsCollapsed || (Parent is TreeNode parent && parent.IsCollapsedInHierarchy);

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
                if (Mathf.NearEqual(m_HeaderHeight, value))
                    return;
                m_HeaderHeight = value;
                PerformLayout();
            }
        }

        public void Expand(bool noAnimation = false)
        {
            // Expanding the direct parent recursively continues the parent chain.
            ExpandAllParents(noAnimation);
            if (m_IsExpanded && m_AnimationProgress >= 1.0f)
                return;

            bool previousState = m_IsExpanded;
            m_IsExpanded = true;
            if (noAnimation)
                m_AnimationProgress = 1.0f;
            else if (previousState != m_IsExpanded)
                m_AnimationProgress = 1.0f - m_AnimationProgress;

            OnExpandedChanged();
            OnExpandAnimationChanged();
        }

        public void Collapse(bool noAnimation = false)
        {
            if (!m_IsExpanded && m_AnimationProgress >= 1.0f)
                return;

            bool previousState = m_IsExpanded;
            m_IsExpanded = false;
            if (noAnimation)
                m_AnimationProgress = 1.0f;
            else if (previousState != m_IsExpanded)
                m_AnimationProgress = 1.0f - m_AnimationProgress;

            OnExpandedChanged();
            OnExpandAnimationChanged();
        }

        public void ExpandAll(bool noAnimation = false)
        {
            bool wasLayoutLocked = IsLayoutLocked;
            IsLayoutLocked = true;
            Expand(noAnimation);
            foreach (TreeNode child in m_Nodes)
                child.ExpandAll(noAnimation);
            IsLayoutLocked = wasLayoutLocked;
            PerformLayout();
        }

        public void CollapseAll(bool noAnimation = false)
        {
            bool wasLayoutLocked = IsLayoutLocked;
            IsLayoutLocked = true;
            Collapse(noAnimation);
            foreach (TreeNode child in m_Nodes)
                child.CollapseAll(noAnimation);
            IsLayoutLocked = wasLayoutLocked;
            PerformLayout();
        }

        public void ExpandAllParents(bool noAnimation = false)
        {
            if (Parent is TreeNode parent)
                parent.Expand(noAnimation);
        }

        public void EndAnimation()
        {
            if (m_AnimationProgress < 1.0f)
            {
                m_AnimationProgress = 1.0f;
                OnExpandAnimationChanged();
            }
        }

        public void Select()
        {
            ParentTree?.Select(this);
        }

        public override void Update(float deltaTime)
        {
            if (m_AnimationProgress < 1.0f)
            {
                if (deltaTime > 1.0f / 20.0f)
                    m_AnimationProgress = 1.0f;
                else
                    m_AnimationProgress = Math.Min(1.0f, m_AnimationProgress + deltaTime / 0.1f);
                OnExpandAnimationChanged();
            }

            const float longPressTimeSeconds = 0.6f;
            if (m_IsMouseDown && Time.GetUnscaledGameTime() - m_MouseDownTime > longPressTimeSeconds)
                OnLongPress();
            if (m_IsExpanded)
                base.Update(deltaTime);
        }

        public override void Draw()
        {
            if (!VisibleInHierarchy || IsDisposed)
                return;

            Style style = Style.Current;
            Tree? tree = ParentTree;
            bool isSelected = false;
            if (tree != null)
            {
                for (int i = 0; i < tree.Selection.Count; i++)
                {
                    if (ReferenceEquals(tree.Selection[i], this))
                    {
                        isSelected = true;
                        break;
                    }
                }
            }
            bool isFocused = tree?.ContainsFocus == true;
            Rectangle headerRect = ToScreen(HeaderRect);

            if (isSelected || m_MouseOverHeader)
            {
                Color background = isSelected && isFocused
                    ? BackgroundColorSelected
                    : m_MouseOverHeader ? BackgroundColorHighlighted : BackgroundColorSelectedUnfocused;
                Render2D.FillRectangle(headerRect, background);
            }

            if (HasAnyVisibleChild)
            {
                SpriteHandle arrow = m_IsExpanded ? style.ArrowDown : style.ArrowRight;
                Rectangle arrowRect = ToScreen(ArrowRect);
                Color arrowColor = m_MouseOverHeader ? style.Foreground : style.ForegroundGrey;
                Render2D.DrawSprite(arrow, arrowRect, arrowColor);
            }

            Rectangle textRect = TextRect;
            if (m_IconCollapsed.IsValid)
            {
                SpriteHandle icon = m_IsExpanded ? m_IconOpened : m_IconCollapsed;
                Rectangle iconRect = ToScreen(new Rectangle(textRect.Left - 18.0f, 0, 16, 16));
                Color iconColor = IconColor;
                Render2D.DrawSprite(icon, iconRect, iconColor);
            }

            m_HeaderText.TextColor = CacheTextColor();
            m_HeaderText.Font = TextFont.GetFont();
            m_HeaderText.Draw();

            if (m_DragOverMode != DragItemPositioning.None && tree?.DraggedOverNode == this)
            {
                Rectangle screenTextRect = ToScreen(textRect);
                switch (m_DragOverMode)
                {
                    case DragItemPositioning.At:
                        Render2D.FillRectangle(screenTextRect, style.Selection);
                        Render2D.DrawRectangle(screenTextRect, style.SelectionBorder, 1.0f);
                        break;
                    case DragItemPositioning.Above:
                        Render2D.DrawRectangle(new Rectangle(screenTextRect.X, screenTextRect.Top - DefaultDragInsertPositionMargin * 0.5f - DefaultNodeOffsetY - m_TextMargin.Top, screenTextRect.Width, DefaultDragInsertPositionMargin), style.SelectionBorder, 1.0f);
                        break;
                    case DragItemPositioning.Below:
                        Render2D.DrawRectangle(new Rectangle(screenTextRect.X, screenTextRect.Bottom + m_TextMargin.Bottom - DefaultDragInsertPositionMargin * 0.5f, screenTextRect.Width, DefaultDragInsertPositionMargin), style.SelectionBorder, 1.0f);
                        break;
                }
            }

            DrawGuideLines(tree, isSelected, style);
            if (m_IsExpanded)
            {
                foreach (Control child in Children)
                {
                    if (!ReferenceEquals(child, m_HeaderText) && child.Visible)
                        child.Draw();
                }
            }
        }

        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            UpdateMouseOverFlags(location);
            if (m_MouseOverHeader)
            {
                if (button == MouseButton.Left)
                {
                    m_IsMouseDown = true;
                    m_MouseDownPos = location;
                    // Preserve the active native implementation; its time query is commented out.
                    m_MouseDownTime = 0.0f;
                }
                Focus();
                return true;
            }

            if (m_IsExpanded)
                return base.OnMouseDown(location, button);
            Focus();
            return true;
        }

        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            UpdateMouseOverFlags(location);
            if (button == MouseButton.Left && m_IsMouseDown)
            {
                m_IsMouseDown = false;
                m_MouseDownTime = -1.0f;
            }

            if (m_MouseOverHeader)
            {
                if (button == MouseButton.Left && Engine.FrameCount - m_DragEndFrame < 10)
                    return true;

                Tree? tree = ParentTree;
                if (!m_MouseOverArrow && tree != null)
                {
                    RootControl? window = tree.Root;
                    if (window?.GetKey(KeyboardKeys.Shift) == true)
                        tree.SelectRange(this);
                    else if (window?.GetKey(KeyboardKeys.Control) == true)
                        tree.AddOrRemoveSelection(this);
                    else
                    {
                        bool isSelected = false;
                        if (button == MouseButton.Right)
                        {
                            for (int i = 0; i < tree.Selection.Count; i++)
                            {
                                if (ReferenceEquals(tree.Selection[i], this))
                                {
                                    isSelected = true;
                                    break;
                                }
                            }
                        }

                        if (!isSelected)
                            tree.Select(this);
                    }
                }

                if (m_MouseOverArrow && HasAnyVisibleChild)
                {
                    if (ParentTree?.Root?.GetKey(KeyboardKeys.Alt) == true)
                    {
                        if (m_IsExpanded)
                            CollapseAll();
                        else
                            ExpandAll();
                    }
                    else if (m_IsExpanded)
                        Collapse();
                    else
                        Expand();
                }

                if (button == MouseButton.Right && TestHeaderHit(ref location))
                    tree?.OnRightClickInternal(this, location);
                Focus();
                return true;
            }

            if (button == MouseButton.Right && TestHeaderHit(ref location))
                ParentTree?.OnRightClickInternal(this, location);
            return base.OnMouseUp(location, button);
        }

        public override bool OnMouseDoubleClick(Float2 location, MouseButton button)
        {
            if (TestHeaderHit(ref location))
                return OnMouseDoubleClickHeader(ref location, button);
            if (m_AnimationProgress >= 1.0f)
                return base.OnMouseDoubleClick(location, button);
            return false;
        }

        public override void OnMouseMove(Float2 location)
        {
            UpdateMouseOverFlags(location);
            if (m_IsMouseDown && Float2.Distance(m_MouseDownPos, location) > 10.0f)
            {
                m_IsMouseDown = false;
                m_MouseDownTime = -1.0f;
                BeginDragDrop();
                return;
            }
            if (m_AnimationProgress >= 1.0f && m_IsExpanded)
                base.OnMouseMove(location);
        }

        public override void OnMouseLeave()
        {
            m_MouseOverArrow = false;
            m_MouseOverHeader = false;
            if (m_IsMouseDown)
            {
                m_IsMouseDown = false;
                m_MouseDownTime = -1.0f;
                BeginDragDrop();
            }
            base.OnMouseLeave();
        }

        public override bool OnKeyDown(KeyboardKeys key) => m_IsExpanded && base.OnKeyDown(key);
        public override bool OnKeyUp(KeyboardKeys key) => m_IsExpanded && base.OnKeyUp(key);

        public override DragDropEffect OnDragEnter(ref Float2 location, DragData data)
        {
            DragDropEffect result = base.OnDragEnter(ref location, data);
            m_DragOverMode = DragItemPositioning.None;
            if (result == DragDropEffect.None)
            {
                UpdateDragPositioning(ref location);
                m_IsDragOverHeader = TestHeaderHit(ref location);
                if (m_IsDragOverHeader)
                {
                    if (ParentTree != null)
                        ParentTree.DraggedOverNode = this;
                    if (ArrowRect.Contains(location) && HasAnyVisibleChild)
                        Expand(true);
                    result = OnDragEnterHeader(data);
                }
                if (result == DragDropEffect.None)
                    m_DragOverMode = DragItemPositioning.None;
            }
            return result;
        }

        public override DragDropEffect OnDragMove(ref Float2 location, DragData data)
        {
            DragDropEffect result = base.OnDragMove(ref location, data);
            ClearDragPositioning();
            if (result == DragDropEffect.None)
            {
                UpdateDragPositioning(ref location);
                bool isDragOverHeader = TestHeaderHit(ref location);
                if (isDragOverHeader)
                {
                    if (ParentTree != null)
                        ParentTree.DraggedOverNode = this;
                    if (ArrowRect.Contains(location) && HasAnyVisibleChild)
                        Expand(true);
                    result = !m_IsDragOverHeader ? OnDragEnterHeader(data) : OnDragMoveHeader(data);
                }
                else if (m_IsDragOverHeader)
                    OnDragLeaveHeader();
                m_IsDragOverHeader = isDragOverHeader;
                if (result == DragDropEffect.None)
                    m_DragOverMode = DragItemPositioning.None;
            }
            return result;
        }

        public override DragDropEffect OnDragDrop(ref Float2 location, DragData data)
        {
            DragDropEffect result = base.OnDragDrop(ref location, data);
            if (result == DragDropEffect.None)
            {
                UpdateDragPositioning(ref location);
                m_DragEndFrame = Engine.FrameCount;
                if (TestHeaderHit(ref location))
                    result = OnDragDropHeader(data);
            }
            m_IsDragOverHeader = false;
            ClearDragPositioning();
            return result;
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

        public override void PerformLayout(bool force = false)
        {
            SyncNodesFromChildren();
            if (IsLayoutLocked && !force)
                return;

            bool wasLocked = IsLayoutLocked;
            if (!wasLocked)
                LockChildrenRecursive();

            float width = Parent is TreeNode parent ? parent.Width : Width;
            if (m_IsExpanded || m_AnimationProgress < 1.0f)
            {
                SetBounds(X, Y, width, Height);
                PerformLayoutBeforeChildren();
                foreach (Control child in Children)
                    child.PerformLayout(true);
                PerformLayoutAfterChildren();
            }
            else
            {
                m_CachedHeight = m_HeaderHeight;
                SetBounds(X, Y, width, m_HeaderHeight);
                LayoutHeader();
            }

            if (!wasLocked)
                UnlockChildrenRecursive();
        }

        public override int Compare(Control? other)
        {
            return other is TreeNode node ? string.Compare(Text, node.Text, StringComparison.Ordinal) : 0;
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

        protected virtual void OnExpandAnimationChanged()
        {
            if (ParentTree != null)
                ParentTree.PerformLayout();
            else if (Parent != null)
                Parent.PerformLayout();
            else
                PerformLayout();
        }

        protected virtual bool TestHeaderHit(ref Float2 location) => HeaderRect.Contains(ref location);

        protected virtual Color CacheTextColor()
        {
            return Enabled ? TextColor : TextColor * 0.6f;
        }

        protected override void OnChildAdded(Control control)
        {
            if (control is TreeNode node && !m_Nodes.Contains(node))
                m_Nodes.Add(node);
            control.SizeChanged += OnChildSizeChanged;
            base.OnChildAdded(control);
        }

        protected override void OnChildRemoved(Control control)
        {
            if (control is TreeNode node)
                m_Nodes.Remove(node);
            control.SizeChanged -= OnChildSizeChanged;
            base.OnChildRemoved(control);
        }

        protected override void OnBoundsChanged(bool locationChanged, bool sizeChanged)
        {
            base.OnBoundsChanged(locationChanged, sizeChanged);
            if (sizeChanged)
                LayoutHeader();
        }

        protected override void OnDispose()
        {
            ParentChanged -= OnParentChanged;
            m_LastTree?.RemoveFromSelection(this);
            base.OnDispose();
        }

        private void PerformLayoutBeforeChildren()
        {
            if (m_IsExpanded)
            {
                float xOffset = m_XOffset + ChildrenIndent;
                foreach (TreeNode node in m_Nodes)
                    node.m_XOffset = xOffset;
            }
            LayoutHeader();
        }

        private void PerformLayoutAfterChildren()
        {
            float y = m_HeaderHeight;
            float height = m_HeaderHeight;
            float xOffset = m_XOffset + ChildrenIndent;
            if (m_IsExpanded || m_AnimationProgress < 1.0f)
            {
                y -= m_CachedHeight * (m_IsExpanded ? 1.0f - m_AnimationProgress : m_AnimationProgress);
                foreach (TreeNode node in m_Nodes)
                {
                    if (!node.Visible)
                        continue;
                    node.m_XOffset = xOffset;
                    node.SetBounds(0, y, Width, node.Height);
                    float nodeHeight = node.Height + DefaultNodeOffsetY;
                    y += nodeHeight;
                    height += nodeHeight;
                }
            }

            m_CachedHeight = height;
            SetBounds(X, Y, Width, Math.Max(m_HeaderHeight, y));
            LayoutHeader();
        }

        private void LayoutHeader()
        {
            if (m_HeaderText == null)
                return;
            Rectangle textRect = TextRect;
            m_HeaderText.TextColor = CacheTextColor();
            m_HeaderText.Font = TextFont?.GetFont();
            m_HeaderText.SetBounds(textRect);
        }

        private Tree? GetParentTree()
        {
            if (m_Tree == null)
            {
                if (Parent is TreeNode node)
                    m_Tree = node.ParentTree;
                else if (Parent is Tree tree)
                    m_Tree = tree;
                if (m_Tree != null)
                    m_LastTree = m_Tree;
            }
            return m_Tree;
        }

        private float CalculateMinimumWidth()
        {
            UpdateTextWidth();
            float minWidth = m_XOffset + m_TextWidth + 6.0f + 16.0f;
            if (m_IconCollapsed.IsValid)
                minWidth += 16.0f;
            if (m_IsExpanded || m_AnimationProgress < 1.0f)
            {
                foreach (TreeNode node in m_Nodes)
                {
                    if (node.Visible)
                        minWidth = Math.Max(minWidth, node.MinimumWidth);
                }
            }
            return minWidth;
        }

        private void UpdateTextWidth()
        {
            if (!m_TextChanged)
                return;
            Font? font = TextFont?.GetFont();
            if (font != null)
            {
                TextLayoutOptions layout = new TextLayoutOptions();
                m_TextWidth = font.MeasureText(m_Text, layout).X;
                m_TextChanged = false;
            }
        }

        private Rectangle CalculateTextRect()
        {
            float left = m_XOffset + 16.0f;
            Rectangle textRect = new Rectangle(left, 0, Width - left, m_HeaderHeight);
            m_TextMargin.ShrinkRectangle(ref textRect);
            if (m_IconCollapsed.IsValid)
            {
                textRect.X += 18.0f;
                textRect.Width -= 18.0f;
            }
            return textRect;
        }

        private void UpdateDragPositioning(ref Float2 location)
        {
            Rectangle header = HeaderRect;
            Rectangle above = new Rectangle(header.X, header.Y - DefaultDragInsertPositionMargin - DefaultNodeOffsetY, header.Width, DefaultDragInsertPositionMargin * 2.0f);
            Rectangle below = new Rectangle(header.X, header.Bottom - DefaultDragInsertPositionMargin, header.Width, DefaultDragInsertPositionMargin * 2.0f);
            if (above.Contains(location))
                m_DragOverMode = DragItemPositioning.Above;
            else if ((IsCollapsed || !HasAnyVisibleChild) && below.Contains(location))
                m_DragOverMode = DragItemPositioning.Below;
            else
                m_DragOverMode = DragItemPositioning.At;

            Tree? tree = ParentTree;
            if (m_DragOverMode == DragItemPositioning.None)
            {
                if (tree != null && ReferenceEquals(tree.DraggedOverNode, this))
                    tree.DraggedOverNode = null;
            }
            else if (tree != null)
                tree.DraggedOverNode = this;
        }

        private void ClearDragPositioning()
        {
            m_DragOverMode = DragItemPositioning.None;
            Tree? tree = ParentTree;
            if (tree != null && ReferenceEquals(tree.DraggedOverNode, this))
                tree.DraggedOverNode = null;
        }

        private void UpdateMouseOverFlags(Float2 location)
        {
            m_MouseOverArrow = HasAnyVisibleChild && ArrowRect.Contains(location);
            m_MouseOverHeader = new Rectangle(0, 0, Width, m_HeaderHeight - 1.0f).Contains(location);
            if (!m_MouseOverHeader)
                return;
            foreach (Control child in Children)
            {
                if (!child.IsScrollable && child.EnabledInHierarchy && child.VisibleInHierarchy && child.Bounds.Contains(location))
                {
                    m_MouseOverHeader = false;
                    break;
                }
            }
        }

        private void OnChildSizeChanged(Control control)
        {
            if (!m_IsExpanded && control is TreeNode)
                return;
            PerformLayout();
        }

        private void SyncNodesFromChildren()
        {
            m_Nodes.Clear();
            foreach (Control child in Children)
            {
                if (child is TreeNode node)
                    m_Nodes.Add(node);
            }
        }

        private void OnParentChanged(Control control)
        {
            _ = control;
            m_Tree = null;
        }

        private Rectangle ToScreen(Rectangle local)
        {
            Float2 screen = ScreenPos;
            local.X += screen.X;
            local.Y += screen.Y;
            return local;
        }

        private void DrawGuideLines(Tree? tree, bool isSelected, Style style)
        {
            if (tree == null || tree.Children.Count == 0)
                return;

            TreeNode? parentNode = Parent as TreeNode;
            bool thisNodeIsLast = false;
            while (parentNode != null && !ReferenceEquals(parentNode, tree.GetChild(0)))
            {
                float bottomOffset = 0.0f;
                float topOffset = 0.0f;
                if (ReferenceEquals(Parent, parentNode) && parentNode.m_Nodes.Count > 0 && ReferenceEquals(this, parentNode.m_Nodes[0]))
                    topOffset = 2.0f;
                if (thisNodeIsLast && parentNode.m_Nodes.Count == 1)
                    bottomOffset = topOffset != 0.0f ? 4.0f : 2.0f;
                if (ReferenceEquals(Parent, parentNode) && parentNode.m_Nodes.Count > 0 && ReferenceEquals(this, parentNode.m_Nodes[parentNode.m_Nodes.Count - 1]) && !m_IsExpanded)
                {
                    thisNodeIsLast = true;
                    bottomOffset = topOffset != 0.0f ? 4.0f : 2.0f;
                }

                float leftOffset = m_IconCollapsed.IsValid ? 27.0f : 9.0f;
                Rectangle parentText = parentNode.ToScreen(parentNode.TextRect);
                Rectangle parentHeader = parentNode.ToScreen(parentNode.HeaderRect);
                Rectangle line = new Rectangle(parentText.Left - leftOffset, parentHeader.Top + topOffset, 1.0f, parentHeader.Height - bottomOffset);
                Render2D.FillRectangle(line, isSelected ? style.ForegroundGrey : style.LightBackground);
                parentNode = parentNode.Parent as TreeNode;
            }
        }
    }
}
