namespace SE.GUI
{
    public class ScrollableControl : ContainerControl
    {
        /// <summary>
        /// The view offset. Useful to offset contents of the container (used by the scrollbars and drop panels).
        /// </summary>
        protected Float2 m_ViewOffset;

        /// <summary>
        /// Called when view offset gets changed.
        /// </summary>
        protected virtual void OnViewOffsetChanged()
        {
        }

        /// <inheritdoc />
        protected override void DrawChildren()
        {
            // Draw all visible child controls
            bool hasViewOffset = !m_ViewOffset.IsZero;
            for (int i = 0; i < m_Children.Count; i++)
            {
                Control child = m_Children[i];

                if (child.Visible)
                {
                    Matrix3x3 transform = child.CachedTransform;
                    if (hasViewOffset && child.IsScrollable)
                    {
                        transform.M31 += m_ViewOffset.X;
                        transform.M32 += m_ViewOffset.Y;
                    }

                    Render2D.PushTransform(transform);
                    child.Draw();
                    Render2D.PopTransform();
                }
            }
        }

        public ScrollableControl()
        {
        }

        /// <summary>
        /// Gets current view offset for all the controls (used by the scroll bars).
        /// </summary>
        public Float2 GetViewOffset()
        {
            return m_ViewOffset;
        }

        /// <summary>
        /// Sets the view offset.
        /// </summary>
        /// <param name="value">The value.</param>
        public virtual void SetViewOffset(Float2 value)
        {
            m_ViewOffset = value;
            OnViewOffsetChanged();
        }

        /// <inheritdoc />
        public override bool IntersectsChildContent(Control child, Float2 location, out Float2 childSpaceLocation)
        {
            // Apply offset on scrollable controls
            if (child.IsScrollable)
            {
                location -= m_ViewOffset;
            }

            return child.IntersectsContent(location, out childSpaceLocation);
        }

        /// <inheritdoc />
        public override bool IntersectsContent(Float2 locationParent, out Float2 location)
        {
            // Little workaround to prevent applying offset when performing intersection test with this scrollable
            // control.
            // Note that overriden PointFromParent applies view offset.
            location = base.PointFromParent(locationParent);
            return ContainsPoint(location);
        }

        /// <inheritdoc />
        public override Float2 PointToParent(Float2 location)
        {
            return base.PointToParent(location) + m_ViewOffset;
        }

        /// <inheritdoc />
        public override Float2 PointFromParent(Float2 location)
        {
            return base.PointFromParent(location) - m_ViewOffset;
        }
    }
}
