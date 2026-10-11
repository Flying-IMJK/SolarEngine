#nullable disable annotations

using System;
using System.Collections.Generic;
using SE.GUI;

namespace SE.Editor.GUI
{
    public class TreeNode : ContainerControl
    {
        /// <summary>
        /// The default drag insert position margin.
        /// </summary>
        public const float DefaultDragInsertPositionMargin = 3.0f;

        /// <summary>
        /// The default node offset on Y axis.
        /// </summary>
        public const float DefaultNodeOffsetY = 0.0f;

        private Tree m_Tree;
        private bool m_IsExpanded;
        private bool m_CanChangeOrder;
        private float m_AnimationProgress;
        private float m_CachedHeight;
        private bool m_MouseOverArrow;
        private bool m_MouseOverHeader;
        private float m_XOffset;
        private float m_TextWidth;
        private float m_HeaderHeight = 16.0f;
        private Rectangle m_HeaderRect;
        private SpriteHandle m_IconCollapsed;
        private SpriteHandle m_IconOpened;
        private Margin m_TextMargin = new Margin(2.0f);
        private string m_Text = string.Empty;
        private bool m_TextChanged;
        private bool m_IsMouseDown;
        private float m_MouseDownTime;
        private Float2 m_MouseDownPos;
        private DragItemPositioning m_DragOverMode;
        private bool m_IsDragOverHeader;
        private static ulong m_DragEndFrame;

        public TreeNode()
        {
            m_AnimationProgress = 1.0f;
            m_CachedHeight = m_HeaderHeight;
            m_IconCollapsed = SpriteHandle.Invalid;
            m_IconOpened = SpriteHandle.Invalid;
            m_MouseDownTime = -1.0f;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="TreeNode"/> class.
        /// </summary>
        /// <param name="canChangeOrder">Enable/disable changing node order in parent tree node.</param>
        public TreeNode(bool canChangeOrder) : this(canChangeOrder, SpriteHandle.Invalid, SpriteHandle.Invalid)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="TreeNode"/> class.
        /// </summary>
        /// <param name="canChangeOrder">Enable/disable changing node order in parent tree node.</param>
        /// <param name="iconCollapsed">The icon for node collapsed.</param>
        /// <param name="iconOpened">The icon for node opened.</param>
        public TreeNode(bool canChangeOrder, SpriteHandle iconCollapsed, SpriteHandle iconOpened) : base(0, 0, 64, 16)
        {
            m_CanChangeOrder = canChangeOrder;
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
            TextFont = new FontReference(style.FontSmall);
        }

        /// <summary>
        /// Gets or sets the text.
        /// </summary>
        public string Text
        {
            get => m_Text;
            set
            {
                m_Text = value;
                m_TextChanged = true;
                PerformLayout();
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether this node is expanded.
        /// </summary>
        public bool IsExpanded
        {
            get => m_IsExpanded;
            set
            {
                if (value)
                {
                    Expand(true);
                }
                else
                {
                    Collapse(true);
                }
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether this node is collapsed.
        /// </summary>
        public bool IsCollapsed
        {
            get => m_IsExpanded;
            set
            {
                if (value)
                {
                    Collapse(true);
                }
                else
                {
                    Expand(true);
                }
            }
        }

        /// <summary>
        /// Gets a value indicating whether the node is collapsed in the hierarchy (is collapsed or any of its parents is collapsed).
        /// </summary>
        public bool IsCollapsedInHierarchy => IsCollapsed || (Parent is TreeNode parent && parent.IsCollapsedInHierarchy);

        /// <summary>
        /// Gets or sets the text margin.
        /// </summary>
        public Margin TextMargin
        {
            get => m_TextMargin;
            set
            {
                m_TextMargin = value;
                PerformLayout();
            }
        }

        /// <summary>
        /// Gets or sets the color of the text.
        /// </summary>
        public Color TextColor;

        /// <summary>
        /// Gets or sets the font used to render text.
        /// </summary>
        public FontReference TextFont = new FontReference();

        /// <summary>
        /// Gets or sets the color of the background when tree node is selected.
        /// </summary>
        public Color BackgroundColorSelected;

        /// <summary>
        /// Gets or sets the color of the background when tree node is highlighted.
        /// </summary>
        public Color BackgroundColorHighlighted;

        /// <summary>
        /// Gets or sets the color of the background when tree node is selected but not focused.
        /// </summary>
        public Color BackgroundColorSelectedUnfocused;

        /// <summary>
        /// Gets the parent tree control.
        /// </summary>
        public Tree ParentTree 
        {
            get 
            {
                if (m_Tree == null)
                {
                    if (Parent is TreeNode node)
                    {
                        m_Tree = node.ParentTree;
                    }
                    else if (Parent is Tree tree)
                    {
                        m_Tree = tree;
                    }
                }

                return m_Tree;
            }
        }

        /// <summary>
        /// Gets a value indicating whether this node is root.
        /// </summary>
        public bool IsRoot => Parent is not TreeNode;

        /// <summary>
        /// Gets the minimum width of the node sub-tree.
        /// </summary>
        public float MinimumWidth
        {
            get
            {
                UpdateTextWidth();
                float minWidth = m_XOffset + m_TextWidth + 6.0f + 16.0f;
                if (m_IconCollapsed.IsValid)
                {
                    minWidth += 16.0f;
                }

                if (m_IsExpanded || m_AnimationProgress < 1.0f)
                {
                    for (int i = 0; i < m_Children.Count; i++)
                    {
                        if (m_Children[i] is TreeNode node && node.Visible)
                        {
                            minWidth = Math.Max(minWidth, node.MinimumWidth);
                        }
                    }
                }
                return minWidth;
            }
        }

        /// <summary>
        /// The indent applied to the child nodes.
        /// </summary>
        public float ChildrenIndent = 12.0f;

        /// <summary>
        /// The height of the tree node header area.
        /// </summary>
        public float HeaderHeight
        {
            get => m_HeaderHeight;
            set
            {
                if (!Mathf.NearEqual(m_HeaderHeight, value))
                {
                    m_HeaderHeight = value;
                    PerformLayout();
                }
            }
        }

        /// <summary>
        /// Gets or sets the color of the icon.
        /// </summary>
        public Color IconColor = Color.White;

        /// <summary>
        /// Gets the arrow rectangle.
        /// </summary>
        public Rectangle ArrowRect
        {
            get
            {
                if (CustomArrowRect.HasValue)
                {
                    return CustomArrowRect.Value;
                }

                return new Rectangle(m_XOffset + 2.0f + m_TextMargin.Left, 2.0f, 12.0f, 12.0f);
            }
        }

        /// <summary>
        /// Gets the header rectangle.
        /// </summary>
        public Rectangle HeaderRect => m_HeaderRect;

        /// <summary>
        /// Gets the header text rectangle.
        /// </summary>
        public Rectangle TextRect => CalculateTextRect();

        /// <summary>
        /// Custom arrow rectangle within node.
        /// </summary>
        public Rectangle? CustomArrowRect;

        /// <summary>
        /// Gets the drag over action type.
        /// </summary>
        public DragItemPositioning DragOverMode => m_DragOverMode;

        /// <summary>
        /// Gets a value indicating whether this node has any visible child. Returns false if it has no children.
        /// </summary>
        public bool HasAnyVisibleChild
        {
            get
            {
                for (int i = 0; i < m_Children.Count; i++)
                {
                    if (m_Children[i] is TreeNode node && node.Visible)
                    {
                        return true;
                    }
                }

                return false;
            }
        }

        /// <summary>
        /// Expand node.
        /// </summary>
        /// <param name="noAnimation">True if skip node expanding animation.</param>
        public void Expand(bool noAnimation = false)
        {
            // Parents first
            ExpandAllParents(noAnimation);

            // Change state
            if (m_IsExpanded && m_AnimationProgress >= 1.0f)
            {
                return;
            }

            bool previousState = m_IsExpanded;
            m_IsExpanded = true;
            if (noAnimation)
            {
                m_AnimationProgress = 1.0f;
            }
            else if (previousState != m_IsExpanded)
            {
                m_AnimationProgress = 1.0f - m_AnimationProgress;
            }

            // Update
            OnExpandedChanged();
            OnExpandAnimationChanged();
        }

        /// <summary>
        /// Collapse node.
        /// </summary>
        /// <param name="noAnimation">True if skip node expanding animation.</param>
        public void Collapse(bool noAnimation = false)
        {
            // Change state
            if (!m_IsExpanded && m_AnimationProgress >= 1.0f)
            {
                return;
            }

            bool previousState = m_IsExpanded;
            m_IsExpanded = false;
            if (noAnimation)
            {
                m_AnimationProgress = 1.0f;
            }
            else if (previousState != m_IsExpanded)
            {
                m_AnimationProgress = 1.0f - m_AnimationProgress;
            }

            // Update
            OnExpandedChanged();
            OnExpandAnimationChanged();
        }

        /// <summary>
        /// Expand node and all the children.
        /// </summary>
        /// <param name="noAnimation">True if skip node expanding animation.</param>
        public void ExpandAll(bool noAnimation = false)
        {
            bool wasLayoutLocked = IsLayoutLocked;
            IsLayoutLocked = true;
            Expand(noAnimation);
            for (int i = 0; i < m_Children.Count; i++)
            {
                if (m_Children[i] is TreeNode node)
                {
                    node.ExpandAll(noAnimation);
                }
            }

            IsLayoutLocked = wasLayoutLocked;
            PerformLayout();
        }

        /// <summary>
        /// Collapse node and all the children.
        /// </summary>
        /// <param name="noAnimation">True if skip node expanding animation.</param>
        public void CollapseAll(bool noAnimation = false)
        {
            bool wasLayoutLocked = IsLayoutLocked;
            IsLayoutLocked = true;
            Collapse(noAnimation);
            for (int i = 0; i < m_Children.Count; i++)
            {
                if (m_Children[i] is TreeNode node)
                {
                    node.CollapseAll(noAnimation);
                }
            }

            IsLayoutLocked = wasLayoutLocked;
            PerformLayout();
        }

        /// <summary>
        /// Ensure that all node parents are expanded.
        /// </summary>
        /// <param name="noAnimation">True if skip node expanding animation.</param>
        public void ExpandAllParents(bool noAnimation = false)
        {
            if (Parent is TreeNode parent)
            {
                parent.Expand(noAnimation);
            }
        }

        /// <summary>
        /// Ends open/close animation by force.
        /// </summary>
        public void EndAnimation()
        {
            if (m_AnimationProgress < 1.0f)
            {
                m_AnimationProgress = 1.0f;
                OnExpandAnimationChanged();
            }
        }

        /// <summary>
        /// Select node in the tree.
        /// </summary>
        public void Select()
        {
            ParentTree.Select(this);
        }

        /// <inheritdoc />
        public override void Update(float deltaTime)
        {
            if (m_AnimationProgress < 1.0f)
            {
                bool isDeltaSlow = deltaTime > 1.0f / 20.0f;
                if (isDeltaSlow)
                {
                    m_AnimationProgress = 1.0f;
                }
                else
                {
                    const float openCloseAnimationTime = 0.1f;
                    m_AnimationProgress += deltaTime / openCloseAnimationTime;
                    if (m_AnimationProgress > 1.0f)
                    {
                        m_AnimationProgress = 1.0f;
                    }
                }

                OnExpandAnimationChanged();
            }

            const float longPressTimeSeconds = 0.6f;
            if (m_IsMouseDown && Time.GetUnscaledGameTime() - m_MouseDownTime > longPressTimeSeconds)
            {
                OnLongPress();
            }

            if (m_IsExpanded)
            {
                base.Update(deltaTime);
            }
        }

        /// <inheritdoc />
        public override void Draw()
        {
            Style style = Style.Current;
            Tree tree = ParentTree;
            if (tree == null)
            {
                return;
            }

            bool isSelected = tree.Selection.Contains(this);
            bool isFocused = tree.ContainsFocus;
            float left = m_XOffset + 16.0f;
            Rectangle textRect = new Rectangle(left, 0, Width - left, m_HeaderHeight);
            m_TextMargin.ShrinkRectangle(ref textRect);

            if (isSelected || m_MouseOverHeader)
            {
                Render2D.FillRectangle(m_HeaderRect, isSelected && isFocused
                    ? BackgroundColorSelected
                    : m_MouseOverHeader ? BackgroundColorHighlighted : BackgroundColorSelectedUnfocused);
            }

            if (HasAnyVisibleChild)
            {
                Render2D.DrawSprite(m_IsExpanded ? style.ArrowDown : style.ArrowRight, ArrowRect,
                    m_MouseOverHeader ? style.Foreground : style.ForegroundGrey);
            }

            if (m_IconCollapsed.IsValid)
            {
                Render2D.DrawSprite(m_IsExpanded ? m_IconOpened : m_IconCollapsed,
                    new Rectangle(textRect.Left, 0, 16, 16), IconColor);
                textRect.X += 18.0f;
                textRect.Width -= 18.0f;
            }

            Color textColor = CacheTextColor();
            Render2D.RenderText(TextFont.GetFont(), m_Text, textRect, textColor,
                TextAlignment.Near, TextAlignment.Center);

            if (IsDragOver() && m_Tree.DraggedOverNode == this)
            {
                switch (m_DragOverMode)
                {
                    case DragItemPositioning.At:
                        Render2D.FillRectangle(textRect, style.Selection);
                        Render2D.DrawRectangle(textRect, style.SelectionBorder);
                        break;
                    case DragItemPositioning.Above:
                        Render2D.DrawRectangle(new Rectangle(textRect.X,
                            textRect.Top - DefaultDragInsertPositionMargin * 0.5f - DefaultNodeOffsetY - m_TextMargin.Top,
                            textRect.Width, DefaultDragInsertPositionMargin), style.SelectionBorder);
                        break;
                    case DragItemPositioning.Below:
                        Render2D.DrawRectangle(new Rectangle(textRect.X,
                            textRect.Bottom + m_TextMargin.Bottom - DefaultDragInsertPositionMargin * 0.5f,
                            textRect.Width, DefaultDragInsertPositionMargin), style.SelectionBorder);
                        break;
                }
            }

            if (tree != null)
            {
                TreeNode parentNode = Parent as TreeNode;
                bool thisNodeIsLast = false;
                while (parentNode != null && parentNode != tree.GetChild(0))
                {
                    float bottomOffset = 0;
                    float topOffset = 0;

                    if (Parent == parentNode && this == Parent.GetChild(0))
                    {
                        topOffset = 2;
                    }

                    if (thisNodeIsLast && parentNode.Children.Count == 1)
                    {
                        bottomOffset = topOffset != 0 ? 4 : 2;
                    }

                    if (Parent == parentNode && this == Parent.GetChild(Parent.Children.Count - 1) && !m_IsExpanded)
                    {
                        thisNodeIsLast = true;
                        bottomOffset = topOffset != 0 ? 4 : 2;
                    }

                    float leftOffset = 9;
                    if (m_IconCollapsed.IsValid)
                    {
                        leftOffset += 18;
                    }

                    Rectangle parentTextRect = parentNode.TextRect;
                    Rectangle parentHeaderRect = parentNode.HeaderRect;
                    Rectangle lineRect = new Rectangle(parentTextRect.Left - leftOffset, parentHeaderRect.Top + topOffset, 1, parentHeaderRect.Height - bottomOffset);

                    Color color = isSelected ? style.ForegroundGrey : style.LightBackground;
                    Render2D.FillRectangle(lineRect, color);
                    parentNode = parentNode.Parent as TreeNode;
                }
            }

            if (m_IsExpanded)
            {
                if (ClipChildren)
                {
                    Render2D.PushClip(new Rectangle(0, m_HeaderHeight, Width, Height - m_HeaderHeight));
                    base.Draw();
                    Render2D.PopClip();
                }
                else
                {
                    base.Draw();
                }
            }
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            UpdateMouseOverFlags(location);
            if (m_MouseOverHeader)
            {
                if (button == MouseButton.Left)
                {
                    m_IsMouseDown = true;
                    m_MouseDownPos = location;
                    m_MouseDownTime = 0; // Time.UnscaledGameTime
                }
                Focus();
                return true;
            }

            if (m_IsExpanded)
            {
                return base.OnMouseDown(location, button);
            }

            Focus();
            return true;
        }

        /// <inheritdoc />
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
                {
                    return true;
                }

                if (!m_MouseOverArrow)
                {
                    Tree tree = ParentTree!;
                    RootControl window = tree.Root!;
                    if (window.GetKey(KeyboardKeys.Shift))
                    {
                        tree.SelectRange(this);
                    }
                    else if (window.GetKey(KeyboardKeys.Control))
                    {
                        tree.AddOrRemoveSelection(this);
                    }
                    else if (button == MouseButton.Right && tree.Selection.Contains(this))
                    {
                        // Do nothing
                    }
                    else
                    {
                        tree.Select(this);
                    }
                }

                if (m_MouseOverArrow && HasAnyVisibleChild)
                {
                    if (ParentTree.Root.GetKey(KeyboardKeys.Alt))
                    {
                        if (m_IsExpanded)
                        {
                            CollapseAll();
                        }
                        else
                        {
                            ExpandAll();
                        }
                    }
                    else
                    {
                        if (m_IsExpanded)
                        {
                            Collapse();
                        }
                        else
                        {
                            Expand();
                        }
                    }
                }

                if (button == MouseButton.Right && TestHeaderHit(ref location))
                {
                    ParentTree.OnRightClickInternal(this, ref location);
                }

                Focus();
                return true;
            }

            if (button == MouseButton.Right && TestHeaderHit(ref location))
            {
                ParentTree.OnRightClickInternal(this, ref location);
            }

            return base.OnMouseUp(location, button);
        }

        /// <inheritdoc />
        public override bool OnMouseDoubleClick(Float2 location, MouseButton button)
        {
            if (TestHeaderHit(ref location))
            {
                return OnMouseDoubleClickHeader(ref location, button);
            }

            if (m_AnimationProgress >= 1.0f)
            {
                return base.OnMouseDoubleClick(location, button);
            }

            return false;
        }

        /// <inheritdoc />
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
            if (m_AnimationProgress >= 1.0f)
            {
                if (m_IsExpanded)
                {
                    base.OnMouseMove(location);
                }
            }
        }

        /// <inheritdoc />
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

        /// <inheritdoc />
        public override bool OnKeyDown(KeyboardKeys key)
        {
            if (m_IsExpanded)
            {
                return base.OnKeyDown(key);
            }

            return false;
        }

        /// <inheritdoc />
        public override void OnKeyUp(KeyboardKeys key)
        {
            if (m_IsExpanded)
            {
                base.OnKeyUp(key);
            }
        }

        /// <inheritdoc />
        public override void OnChildResized(Control control)
        {
            if (!m_IsExpanded && control is TreeNode)
            {
                return;
            }

            PerformLayout();
            base.OnChildResized(control);
        }

        /// <inheritdoc />
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
                    {
                        ParentTree.DraggedOverNode = this;
                    }

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
            DragDropEffect result = base.OnDragMove(ref location, data);
            ClearDragPositioning();
            if (result == DragDropEffect.None)
            {
                UpdateDragPositioning(ref location);
                bool isDragOverHeader = TestHeaderHit(ref location);
                if (isDragOverHeader)
                {
                    if (ParentTree != null)
                    {
                        ParentTree.DraggedOverNode = this;
                    }

                    if (ArrowRect.Contains(location) && HasAnyVisibleChild)
                    {
                        Expand(true);
                    }

                    if (!m_IsDragOverHeader)
                    {
                        result = OnDragEnterHeader(data);
                    }
                    else
                    {
                        result = OnDragMoveHeader(data);
                    }
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
            DragDropEffect result = base.OnDragDrop(ref location, data);
            if (result == DragDropEffect.None)
            {
                UpdateDragPositioning(ref location);
                m_DragEndFrame = Engine.FrameCount;
                if (TestHeaderHit(ref location))
                {
                    result = OnDragDropHeader(data);
                }
            }
            m_IsDragOverHeader = false;
            ClearDragPositioning();
            return result;
        }

        /// <inheritdoc />
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

        /// <inheritdoc />
        public override bool OnTestTooltipOverControl(Float2 location)
        {
            return TestHeaderHit(ref location) && ShowTooltip();
        }

        /// <inheritdoc />
        public override void PerformLayout(bool force = false)
        {
            if (IsLayoutLocked && !force)
            {
                return;
            }

            bool wasLocked = IsLayoutLocked;
            if (!wasLocked)
            {
                LockChildrenRecursive();
            }

            ContainerControl parent = Parent;
            float width = parent is TreeNode ? parent.Width : Width;
            if (m_IsExpanded || m_AnimationProgress < 1.0f)
            {
                Width = width;
                PerformLayoutBeforeChildren();
                for (int i = 0; i < m_Children.Count; i++)
                {
                    m_Children[i].PerformLayout(true);
                }

                PerformLayoutAfterChildren();
            }
            else
            {
                m_CachedHeight = m_HeaderHeight;
                Size = new Float2(width, m_HeaderHeight);
            }

            if (!wasLocked)
            {
                UnlockChildrenRecursive();
            }
        }

        /// <inheritdoc />
        public override int CompareTo(Control other)
        {
            if (other is TreeNode node)
            {
                return string.Compare(Text, node.Text, StringComparison.Ordinal);
            }

            return base.CompareTo(other);
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            if (ParentTree != null)
            {
                ParentTree.Selection.Remove(this);
            }

            base.OnDestroy();
        }

        /// <summary>
        /// Called when drag and drop enters the node header area.
        /// </summary>
        /// <param name="data">The data.</param>
        /// <returns>Drag action response.</returns>
        protected virtual DragDropEffect OnDragEnterHeader(DragData data)
        {
            return DragDropEffect.None;
        }

        /// <summary>
        /// Called when drag and drop moves over the node header area.
        /// </summary>
        /// <param name="data">The data.</param>
        /// <returns>Drag action response.</returns>
        protected virtual DragDropEffect OnDragMoveHeader(DragData data)
        {
            return DragDropEffect.None;
        }

        /// <summary>
        /// Called when drag and drop performs over the node header area.
        /// </summary>
        /// <param name="data">The data.</param>
        /// <returns>Drag action response.</returns>
        protected virtual DragDropEffect OnDragDropHeader(DragData data)
        {
            return DragDropEffect.None;
        }

        /// <summary>
        /// Called when drag and drop leaves the node header area.
        /// </summary>
        protected virtual void OnDragLeaveHeader()
        {
        }

        /// <summary>
        /// Begins the drag drop operation.
        /// </summary>
        protected virtual void BeginDragDrop()
        {
        }

        /// <summary>
        /// Called when mouse double clicks header.
        /// </summary>
        /// <param name="location">The mouse location.</param>
        /// <param name="button">The button.</param>
        /// <returns>True if event has been handled.</returns>
        protected virtual bool OnMouseDoubleClickHeader(ref Float2 location, MouseButton button)
        {
            if (HasAnyVisibleChild)
            {
                if (m_IsExpanded)
                {
                    Collapse();
                }
                else
                {
                    Expand();
                }
            }
            return true;
        }

        /// <summary>
        /// Called when mouse is pressing node header for a long time.
        /// </summary>
        protected virtual void OnLongPress()
        {
        }

        /// <summary>
        /// Called when expanded/collapsed state changes.
        /// </summary>
        protected virtual void OnExpandedChanged()
        {
        }

        /// <summary>
        /// Called when expand/collapse animation progress changes.
        /// </summary>
        protected virtual void OnExpandAnimationChanged()
        {
            if (ParentTree != null)
            {
                ParentTree.PerformLayout();
            }
            else if (Parent != null)
            {
                Parent.PerformLayout();
            }
            else
            {
                PerformLayout();
            }
        }

        /// <summary>
        /// Tests the header hit.
        /// </summary>
        /// <param name="location">The location.</param>
        /// <returns>True if hits it.</returns>
        protected virtual bool TestHeaderHit(ref Float2 location)
        {
            return m_HeaderRect.Contains(location);
        }

        /// <summary>
        /// Caches the color of the text for this node. Called during update before children nodes but after parent node so it can reuse parent tree node data.
        /// </summary>
        /// <returns>Text color.</returns>
        protected virtual Color CacheTextColor()
        {
            return Enabled ? TextColor : TextColor * 0.6f;
        }

        /// <summary>
        /// Updates the cached width of the text.
        /// </summary>
        protected void UpdateTextWidth()
        {
            if (m_TextChanged)
            {
                Font font = TextFont.GetFont();
                if (font != null)
                {
                    TextLayoutOptions layout = new TextLayoutOptions();
                    m_TextWidth = font.MeasureText(m_Text, layout).X;
                    m_TextChanged = false;
                }
            }
        }

        /// <inheritdoc />
        protected override void DrawChildren()
        {
            List<Control> children = m_Children;
            if (children.Count == 0)
            {
                return;
            }

            if (CullChildren)
            {
                Rectangle globalClipping = default;
                Matrix3x3 globalTransform = default;
                Render2D.PeekClip(ref globalClipping);
                Render2D.PeekTransform(ref globalTransform);

                Rectangle firstChildGlobalRect = GetChildGlobalRectangle(children[0], ref globalTransform);
                int firstVisibleChild = Math.Clamp(
                    (int)Math.Floor((globalClipping.Y - firstChildGlobalRect.Top) / m_HeaderHeight) + 1,
                    0, children.Count - 1);
                if (GetChildGlobalRectangle(children[firstVisibleChild], ref globalTransform).Top > globalClipping.Top ||
                    !children[firstVisibleChild].Visible)
                {
                    for (; firstVisibleChild > 0; firstVisibleChild--)
                    {
                        Control child = children[firstVisibleChild];
                        if (!child.Visible)
                        {
                            continue;
                        }

                        if (GetChildGlobalRectangle(child, ref globalTransform).Top < globalClipping.Top)
                        {
                            break;
                        }
                    }
                }

                for (int i = firstVisibleChild; i < children.Count; i++)
                {
                    Control child = children[i];
                    if (!child.Visible)
                    {
                        continue;
                    }

                    Rectangle childGlobalRect = GetChildGlobalRectangle(child, ref globalTransform);
                    if (!globalClipping.Intersects(childGlobalRect))
                    {
                        break;
                    }

                    Render2D.PushTransform(child.GetCachedTransform());
                    child.Draw();
                    Render2D.PopTransform();
                }
            }
            else
            {
                for (int i = 0; i < children.Count; i++)
                {
                    Control child = children[i];
                    if (child.Visible)
                    {
                        Render2D.PushTransform(child.GetCachedTransform());
                        child.Draw();
                        Render2D.PopTransform();
                    }
                }
            }
        }

        private static Rectangle GetChildGlobalRectangle(Control control, ref Matrix3x3 globalTransform)
        {
            Matrix3x3 cachedTransform = control.GetCachedTransform();
            Matrix3x3.Multiply(ref cachedTransform, ref globalTransform, out Matrix3x3 globalChildTransform);
            return new Rectangle(globalChildTransform.M31, globalChildTransform.M32,
                control.Width * globalChildTransform.M11, control.Height * globalChildTransform.M22);
        }

        /// <inheritdoc />
        protected override void OnSizeChanged()
        {
            base.OnSizeChanged();
            m_HeaderRect = new Rectangle(0, 0, Width, m_HeaderHeight);
        }

        /// <inheritdoc />
        protected override bool CanNavigateChild(Control child)
        {
            if (IsCollapsed && child is TreeNode)
            {
                return false;
            }

            return base.CanNavigateChild(child);
        }

        /// <inheritdoc />
        protected override void OnParentChangedInternal()
        {
            m_Tree = null;
            base.OnParentChangedInternal();
        }

        /// <inheritdoc />
        protected override void PerformLayoutBeforeChildren()
        {
            if (m_IsExpanded)
            {
                float xOffset = m_XOffset + ChildrenIndent;
                for (int i = 0; i < m_Children.Count; i++)
                {
                    if (m_Children[i] is TreeNode node)
                    {
                        node.m_XOffset = xOffset;
                    }
                }
            }

            base.PerformLayoutBeforeChildren();
        }

        /// <inheritdoc />
        protected override void PerformLayoutAfterChildren()
        {
            float y = m_HeaderHeight;
            float height = m_HeaderHeight;
            float xOffset = m_XOffset + ChildrenIndent;
            if (m_IsExpanded || m_AnimationProgress < 1.0f)
            {
                y -= m_CachedHeight * (m_IsExpanded ? 1.0f - m_AnimationProgress : m_AnimationProgress);
                for (int i = 0; i < m_Children.Count; i++)
                {
                    if (m_Children[i] is not TreeNode node || !node.Visible)
                    {
                        continue;
                    }

                    node.m_XOffset = xOffset;
                    node.Location = new Float2(0, y);
                    float nodeHeight = node.Height + DefaultNodeOffsetY;
                    y += nodeHeight;
                    height += nodeHeight;
                }
            }

            m_CachedHeight = height;
            Height = Math.Max(m_HeaderHeight, y);
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

        /// <summary>
        /// Updates the drag over mode based on the given mouse location.
        /// </summary>
        /// <param name="location">The location.</param>
        private void UpdateDragPositioning(ref Float2 location)
        {
            Rectangle above = new Rectangle(m_HeaderRect.X,
                m_HeaderRect.Y - DefaultDragInsertPositionMargin - DefaultNodeOffsetY,
                m_HeaderRect.Width, DefaultDragInsertPositionMargin * 2.0f);
            Rectangle below = new Rectangle(m_HeaderRect.Y,
                m_HeaderRect.Bottom - DefaultDragInsertPositionMargin,
                m_HeaderRect.Width, DefaultDragInsertPositionMargin * 2.0f);
            if (above.Contains(location))
            {
                m_DragOverMode = DragItemPositioning.Above;
            }
            else if ((IsCollapsed || !HasAnyVisibleChild) && below.Contains(location))
            {
                m_DragOverMode = DragItemPositioning.Below;
            }
            else
            {
                m_DragOverMode = DragItemPositioning.At;
            }

            Tree tree = ParentTree;
            if (m_DragOverMode == DragItemPositioning.None)
            {
                if (tree != null && (object)tree.DraggedOverNode == this)
                {
                    tree.DraggedOverNode = null;
                }
            }
            else if (tree != null)
            {
                tree.DraggedOverNode = this;
            }
        }

        private void ClearDragPositioning()
        {
            m_DragOverMode = DragItemPositioning.None;
            Tree tree = ParentTree;
            if (tree != null && (object)tree.DraggedOverNode == this)
            {
                tree.DraggedOverNode = null;
            }
        }

        private void UpdateMouseOverFlags(Float2 location)
        {
            m_MouseOverArrow = HasAnyVisibleChild && ArrowRect.Contains(location);
            m_MouseOverHeader = new Rectangle(0, 0, Width, m_HeaderHeight - 1.0f).Contains(location);
            if (m_MouseOverHeader)
            {
                for (int i = 0; i < Children.Count; i++)
                {
                    if (!Children[i].IsScrollable &&
                        IntersectsChildContent(GetChild(i), location, out _))
                    {
                        m_MouseOverHeader = false;
                        break;
                    }
                }
            }
        }
    }
}
