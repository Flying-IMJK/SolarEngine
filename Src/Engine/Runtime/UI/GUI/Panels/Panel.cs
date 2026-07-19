using System;

namespace SE.GUI
{
    /// <summary>
    /// A general-purpose managed container control.
    /// </summary>
    public class Panel : ScrollableControl
    {
        private readonly VScrollBar _verticalScrollBar;
        private readonly HScrollBar _horizontalScrollBar;
        private float _scrollBarsSize = 16.0f;
        private Float2 _viewportSize;
        private bool _syncingScrollBars;

        public Panel()
            : this(Rectangle.Empty)
        {
        }

        public Panel(Rectangle bounds)
            : base(bounds)
        {
            _verticalScrollBar = new VScrollBar { IsScrollable = false, Visible = false };
            _horizontalScrollBar = new HScrollBar { IsScrollable = false, Visible = false };
            _verticalScrollBar.ValueChanged += OnVerticalScrollBarValueChanged;
            _horizontalScrollBar.ValueChanged += OnHorizontalScrollBarValueChanged;
            AddChild(_verticalScrollBar);
            AddChild(_horizontalScrollBar);
        }

        /// <summary>
        /// Gets the vertical scrollbar managed by this panel.
        /// </summary>
        public VScrollBar VerticalScrollBar => _verticalScrollBar;

        /// <summary>
        /// Gets the horizontal scrollbar managed by this panel.
        /// </summary>
        public HScrollBar HorizontalScrollBar => _horizontalScrollBar;

        /// <summary>
        /// Gets or sets the reserved size of a visible scrollbar.
        /// </summary>
        public float ScrollBarsSize
        {
            get => _scrollBarsSize;
            set
            {
                float result = MathF.Max(1.0f, value);
                if (MathF.Abs(_scrollBarsSize - result) <= float.Epsilon)
                    return;
                _scrollBarsSize = result;
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
        public Float2 ViewBottom => ScrollOffset + _viewportSize;

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

        protected override Float2 ScrollViewportSize => _viewportSize;

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
            else if (bounds.Right > target.X + _viewportSize.X)
                target.X = bounds.Right - _viewportSize.X;

            if (bounds.Y < target.Y)
                target.Y = bounds.Y;
            else if (bounds.Bottom > target.Y + _viewportSize.Y)
                target.Y = bounds.Bottom - _viewportSize.Y;
            ScrollOffset = target;
        }

        /// <inheritdoc />
        public override void Draw()
        {
            DrawSelf();
            if (!VisibleInHierarchy || IsDisposed)
                return;

            Rectangle clip = new Rectangle(ScreenPos, _viewportSize);
            Render2D.PushClip(ref clip);
            for (int index = 0; index < Children.Count; index++)
            {
                Control child = Children[index];
                if (!ReferenceEquals(child, _verticalScrollBar) && !ReferenceEquals(child, _horizontalScrollBar))
                    child.Draw();
            }
            Render2D.PopClip();

            _verticalScrollBar.Draw();
            _horizontalScrollBar.Draw();
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
                float width = MathF.Max(0.0f, Width - (showVertical ? _scrollBarsSize : 0.0f));
                float height = MathF.Max(0.0f, Height - (showHorizontal ? _scrollBarsSize : 0.0f));
                showHorizontal = horizontalEnabled && (AlwaysShowScrollbars || content.X > width);
                showVertical = verticalEnabled && (AlwaysShowScrollbars || content.Y > height);
            }

            _viewportSize = new Float2(
                MathF.Max(0.0f, Width - (showVertical ? _scrollBarsSize : 0.0f)),
                MathF.Max(0.0f, Height - (showHorizontal ? _scrollBarsSize : 0.0f)));
            _verticalScrollBar.Visible = showVertical;
            _horizontalScrollBar.Visible = showHorizontal;
            _verticalScrollBar.SetBounds(_viewportSize.X, 0.0f, showVertical ? _scrollBarsSize : 0.0f, _viewportSize.Y);
            _horizontalScrollBar.SetBounds(0.0f, _viewportSize.Y, _viewportSize.X, showHorizontal ? _scrollBarsSize : 0.0f);
            _verticalScrollBar.SetContentSize(_viewportSize.Y, content.Y);
            _horizontalScrollBar.SetContentSize(_viewportSize.X, content.X);
        }

        protected override void OnScrollOffsetChanged()
        {
            if (_syncingScrollBars)
                return;

            _syncingScrollBars = true;
            _verticalScrollBar.Value = ScrollOffset.Y;
            _horizontalScrollBar.Value = ScrollOffset.X;
            _syncingScrollBars = false;
        }

        protected override void OnChildAdded(Control control)
        {
            base.OnChildAdded(control);
            if (ReferenceEquals(control, _verticalScrollBar) || ReferenceEquals(control, _horizontalScrollBar))
                return;

            SetChildIndex(_verticalScrollBar, Children.Count - 1);
            SetChildIndex(_horizontalScrollBar, Children.Count - 1);
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
            if (!_syncingScrollBars)
                ScrollOffset = new Float2(ScrollOffset.X, scrollBar.Value);
        }

        private void OnHorizontalScrollBarValueChanged(Slider scrollBar)
        {
            if (!_syncingScrollBars)
                ScrollOffset = new Float2(scrollBar.Value, ScrollOffset.Y);
        }
    }
}
