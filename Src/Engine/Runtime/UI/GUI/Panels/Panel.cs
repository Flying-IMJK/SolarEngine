using System;

namespace SE.GUI
{
    /// <summary>
    /// A general-purpose managed container control.
    /// </summary>
    public class Panel : ScrollableControl
    {
        private readonly VScrollBar m_VerticalScrollBar;
        private readonly HScrollBar m_HorizontalScrollBar;
        private float m_ScrollBarsSize = 16.0f;
        private Float2 m_ViewportSize;
        private bool m_SyncingScrollBars;

        public Panel()
            : this(Rectangle.Empty)
        {
        }

        public Panel(Rectangle bounds)
            : base(bounds)
        {
            m_VerticalScrollBar = new VScrollBar { IsScrollable = false, Visible = false };
            m_HorizontalScrollBar = new HScrollBar { IsScrollable = false, Visible = false };
            m_VerticalScrollBar.ValueChanged += OnVerticalScrollBarValueChanged;
            m_HorizontalScrollBar.ValueChanged += OnHorizontalScrollBarValueChanged;
            AddChild(m_VerticalScrollBar);
            AddChild(m_HorizontalScrollBar);
        }

        /// <summary>
        /// Gets the vertical scrollbar managed by this panel.
        /// </summary>
        public VScrollBar VerticalScrollBar => m_VerticalScrollBar;

        /// <summary>
        /// Gets the horizontal scrollbar managed by this panel.
        /// </summary>
        public HScrollBar HorizontalScrollBar => m_HorizontalScrollBar;

        /// <summary>
        /// Gets or sets the reserved size of a visible scrollbar.
        /// </summary>
        public float ScrollBarsSize
        {
            get => m_ScrollBarsSize;
            set
            {
                float result = MathF.Max(1.0f, value);
                if (MathF.Abs(m_ScrollBarsSize - result) <= float.Epsilon)
                    return;
                m_ScrollBarsSize = result;
                PerformLayout();
            }
        }

        /// <summary>
        /// Gets or sets whether enabled scrollbars are shown even when all content fits.
        /// </summary>
        public bool AlwaysShowScrollbars { get; set; }

        /// <summary>
        /// Gets or sets the extra logical content area included at the right and bottom edges.
        /// </summary>
        public Margin ScrollMargin { get; set; }

        /// <summary>
        /// Gets the visible bottom-right point in unscrolled panel coordinates.
        /// </summary>
        public Float2 ViewBottom => ScrollOffset + m_ViewportSize;

        /// <summary>
        /// Gets the unscrolled bounds occupied by scrollable child controls.
        /// </summary>
        public Rectangle ControlsBounds
        {
            get
            {
                Float2 size = GetContentSizeWithMargin();
                return new Rectangle(0.0f, 0.0f, size.X, size.Y);
            }
        }

        protected override Float2 ScrollViewportSize => m_ViewportSize;

        /// <summary>
        /// Moves the view so that the given child control is visible.
        /// </summary>
        public void ScrollViewTo(Control control)
        {
            ArgumentNullException.ThrowIfNull(control);
            Rectangle screenBounds = control.ScreenBounds;
            Rectangle localBounds = new Rectangle(screenBounds.Location - ScreenPos + ScrollOffset, screenBounds.Size);
            ScrollViewTo(localBounds);
        }

        /// <summary>
        /// Moves the view to the supplied unscrolled content point.
        /// </summary>
        public void ScrollViewTo(Float2 location)
        {
            ScrollOffset = location;
        }

        /// <summary>
        /// Moves the view just enough to reveal the supplied unscrolled content rectangle.
        /// </summary>
        public void ScrollViewTo(Rectangle bounds)
        {
            Float2 target = ScrollOffset;
            if (bounds.X < target.X)
                target.X = bounds.X;
            else if (bounds.Right > target.X + m_ViewportSize.X)
                target.X = bounds.Right - m_ViewportSize.X;

            if (bounds.Y < target.Y)
                target.Y = bounds.Y;
            else if (bounds.Bottom > target.Y + m_ViewportSize.Y)
                target.Y = bounds.Bottom - m_ViewportSize.Y;
            ScrollOffset = target;
        }

        /// <inheritdoc />
        public override void Draw()
        {
            DrawSelf();
            if (!VisibleInHierarchy || IsDisposed)
                return;

            Rectangle clip = new Rectangle(ScreenPos, m_ViewportSize);
            Render2D.PushClip(ref clip);
            for (int index = 0; index < Children.Count; index++)
            {
                Control child = Children[index];
                if (!ReferenceEquals(child, m_VerticalScrollBar) && !ReferenceEquals(child, m_HorizontalScrollBar))
                    child.Draw();
            }
            Render2D.PopClip();

            m_VerticalScrollBar.Draw();
            m_HorizontalScrollBar.Draw();
        }

        protected override void OnLayoutChildren()
        {
            Float2 content = GetContentSizeWithMargin();
            bool horizontalEnabled = (ScrollBars & ScrollBars.Horizontal) != 0;
            bool verticalEnabled = (ScrollBars & ScrollBars.Vertical) != 0;
            bool showHorizontal = horizontalEnabled && AlwaysShowScrollbars;
            bool showVertical = verticalEnabled && AlwaysShowScrollbars;

            for (int iteration = 0; iteration < 2; iteration++)
            {
                float width = MathF.Max(0.0f, Width - (showVertical ? m_ScrollBarsSize : 0.0f));
                float height = MathF.Max(0.0f, Height - (showHorizontal ? m_ScrollBarsSize : 0.0f));
                showHorizontal = horizontalEnabled && (AlwaysShowScrollbars || content.X > width);
                showVertical = verticalEnabled && (AlwaysShowScrollbars || content.Y > height);
            }

            m_ViewportSize = new Float2(
                MathF.Max(0.0f, Width - (showVertical ? m_ScrollBarsSize : 0.0f)),
                MathF.Max(0.0f, Height - (showHorizontal ? m_ScrollBarsSize : 0.0f)));
            m_VerticalScrollBar.Visible = showVertical;
            m_HorizontalScrollBar.Visible = showHorizontal;
            m_VerticalScrollBar.SetBounds(m_ViewportSize.X, 0.0f, showVertical ? m_ScrollBarsSize : 0.0f, m_ViewportSize.Y);
            m_HorizontalScrollBar.SetBounds(0.0f, m_ViewportSize.Y, m_ViewportSize.X, showHorizontal ? m_ScrollBarsSize : 0.0f);
            m_VerticalScrollBar.SetContentSize(m_ViewportSize.Y, content.Y);
            m_HorizontalScrollBar.SetContentSize(m_ViewportSize.X, content.X);
        }

        protected override void OnScrollOffsetChanged()
        {
            if (m_SyncingScrollBars)
                return;

            m_SyncingScrollBars = true;
            m_VerticalScrollBar.Value = ScrollOffset.Y;
            m_HorizontalScrollBar.Value = ScrollOffset.X;
            m_SyncingScrollBars = false;
        }

        protected override void OnChildAdded(Control control)
        {
            base.OnChildAdded(control);
            if (ReferenceEquals(control, m_VerticalScrollBar) || ReferenceEquals(control, m_HorizontalScrollBar))
                return;

            SetChildIndex(m_VerticalScrollBar, Children.Count - 1);
            SetChildIndex(m_HorizontalScrollBar, Children.Count - 1);
        }

        private Float2 GetContentSizeWithMargin()
        {
            Float2 content = GetScrollableContentSize();
            return new Float2(
                MathF.Max(0.0f, content.X + ScrollMargin.Left + ScrollMargin.Right),
                MathF.Max(0.0f, content.Y + ScrollMargin.Top + ScrollMargin.Bottom));
        }

        private void OnVerticalScrollBarValueChanged(Slider scrollBar)
        {
            if (!m_SyncingScrollBars)
                ScrollOffset = new Float2(ScrollOffset.X, scrollBar.Value);
        }

        private void OnHorizontalScrollBarValueChanged(Slider scrollBar)
        {
            if (!m_SyncingScrollBars)
                ScrollOffset = new Float2(scrollBar.Value, ScrollOffset.Y);
        }
    }
}
