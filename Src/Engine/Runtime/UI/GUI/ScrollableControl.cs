using System;

namespace SE.GUI
{
    /// <summary>
    /// A clipped container whose children can be scrolled in logical coordinates.
    /// </summary>
    public class ScrollableControl : ContainerControl
    {
        private Float2 m_ScrollOffset;
        private Float2 m_MaximumScrollOffset;

        public ScrollableControl()
        {
        }

        public ScrollableControl(Rectangle bounds)
            : base(bounds)
        {
        }

        private ScrollBars m_ScrollBars = ScrollBars.Both;

        public virtual ScrollBars ScrollBars
        {
            get => m_ScrollBars;
            set
            {
                if (m_ScrollBars == value)
                    return;
                m_ScrollBars = value;
                PerformLayout();
            }
        }
        public float ScrollSpeed { get; set; } = 40.0f;
        public Float2 ScrollOffset
        {
            get => m_ScrollOffset;
            set
            {
                Float2 clamped = new Float2(
                    Math.Clamp(value.X, 0.0f, m_MaximumScrollOffset.X),
                    Math.Clamp(value.Y, 0.0f, m_MaximumScrollOffset.Y));
                if (m_ScrollOffset == clamped)
                    return;
                m_ScrollOffset = clamped;
                OnScrollOffsetChanged();
            }
        }

        public Float2 MaximumScrollOffset => m_MaximumScrollOffset;
        internal override Float2 ChildOffset => new Float2(-m_ScrollOffset.X, -m_ScrollOffset.Y);

        /// <summary>
        /// Gets the drawable area reserved for child content. Panels override this when scrollbar overlays are visible.
        /// </summary>
        protected virtual Float2 ScrollViewportSize => Size;

        public override void PerformLayout(bool force = false)
        {
            base.PerformLayout(force);
            UpdateScrollRange();
        }

        public override bool OnMouseWheel(Float2 location, float delta)
        {
            _ = location;
            if (MathF.Abs(delta) <= float.Epsilon)
                return false;

            Float2 offset = m_ScrollOffset;
            if ((ScrollBars & ScrollBars.Vertical) != 0 && m_MaximumScrollOffset.Y > 0.0f)
                offset.Y = Math.Clamp(offset.Y - delta * ScrollSpeed, 0.0f, m_MaximumScrollOffset.Y);
            else if ((ScrollBars & ScrollBars.Horizontal) != 0 && m_MaximumScrollOffset.X > 0.0f)
                offset.X = Math.Clamp(offset.X - delta * ScrollSpeed, 0.0f, m_MaximumScrollOffset.X);
            else
                return false;

            ScrollOffset = offset;
            return true;
        }

        public override void Draw()
        {
            DrawSelf();
            if (!VisibleInHierarchy || IsDisposed)
                return;

            Float2 viewport = ScrollViewportSize;
            Rectangle clip = new Rectangle(ScreenPos, viewport);
            Render2D.PushClip(ref clip);
            DrawChildren();
            Render2D.PopClip();
        }

        protected override void OnBoundsChanged(bool locationChanged, bool sizeChanged)
        {
            base.OnBoundsChanged(locationChanged, sizeChanged);
            if (sizeChanged)
                UpdateScrollRange();
        }

        /// <summary>
        /// Called after changing the current view offset.
        /// </summary>
        protected virtual void OnScrollOffsetChanged()
        {
        }

        /// <summary>
        /// Calculates the unscrolled extent of the child content.
        /// </summary>
        protected Float2 GetScrollableContentSize()
        {
            float right = 0.0f;
            float bottom = 0.0f;
            for (int index = 0; index < Children.Count; index++)
            {
                Control child = Children[index];
                if (!child.Visible || !child.IsScrollable)
                    continue;
                right = MathF.Max(right, child.X + child.Width);
                bottom = MathF.Max(bottom, child.Y + child.Height);
            }

            return new Float2(right, bottom);
        }

        /// <summary>
        /// Updates the scrollable extent and clamps the current view offset.
        /// </summary>
        protected void UpdateScrollRange()
        {
            Float2 content = GetScrollableContentSize();
            Float2 viewport = ScrollViewportSize;

            m_MaximumScrollOffset = new Float2(
                (ScrollBars & ScrollBars.Horizontal) != 0 ? MathF.Max(0.0f, content.X - viewport.X) : 0.0f,
                (ScrollBars & ScrollBars.Vertical) != 0 ? MathF.Max(0.0f, content.Y - viewport.Y) : 0.0f);
            ScrollOffset = m_ScrollOffset;
        }
    }
}
