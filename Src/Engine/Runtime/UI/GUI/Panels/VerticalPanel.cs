namespace SE.GUI
{
    /// <summary>
    /// This panel arranges child controls vertically.
    /// </summary>
    public class VerticalPanel : PanelWithMargins
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="VerticalPanel"/> class.
        /// </summary>
        public VerticalPanel()
        {
        }

        /// <inheritdoc />
        protected override void PerformLayoutBeforeChildren()
        {
            base.PerformLayoutBeforeChildren();

            // Pre-set width of all controls
            float w = Width - _margin.Width;
            for (int i = 0; i < m_Children.Count; i++)
            {
                Control c = m_Children[i];
                if (c.Visible && Mathf.IsZero(c.AnchorMin.X) && Mathf.IsZero(c.AnchorMax.X))
                {
                    c.Width = w;
                }
            }
        }

        /// <inheritdoc />
        protected override void PerformLayoutAfterChildren()
        {
            // Sort controls vertically
            float top = _margin.Top;
            float bottom = _margin.Bottom;
            float w = Width - _margin.Width;
            bool hasAnyTop = false;
            bool hasAnyBottom = false;
            for (int i = 0; i < m_Children.Count; i++)
            {
                Control c = m_Children[i];
                if (c.Visible)
                {
                    float h = c.Height;
                    if (Mathf.IsZero(c.AnchorMin.Y) && Mathf.IsZero(c.AnchorMax.Y))
                    {
                        c.Bounds = new Rectangle(_margin.Left + _offset.X, top + _offset.Y, w, h);
                        top = c.Bottom + _spacing;
                        hasAnyTop = true;
                    }
                    else if (Mathf.IsOne(c.AnchorMin.Y) && Mathf.IsOne(c.AnchorMax.Y))
                    {
                        bottom += h + _spacing;
                        c.Bounds = new Rectangle(_margin.Left + _offset.X, Height - bottom + _offset.Y, w, h);
                        hasAnyBottom = true;
                    }
                }
            }
            if (hasAnyTop)
            {
                top -= _spacing;
            }
            if (hasAnyBottom)
            {
                bottom -= _spacing;
            }

            // Update size
            if (_autoSize)
            {
                Height = top + bottom;
            }
        }
    }
}
