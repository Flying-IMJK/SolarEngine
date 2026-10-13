#nullable disable annotations

using System;

namespace SE.GUI
{
    public class Panel : ScrollableControl
    {
        private bool m_LayoutChanged;
        private bool m_AlwaysShowScrollbars;
        private int m_LayoutUpdateLock;
        private ScrollBars m_ScrollBars;
        private float m_ScrollBarsSize = 14;
        private Margin m_ScrollMargin;
        private Color m_ScrollbarTrackColor;
        private Color m_ScrollbarThumbColor;
        private Color m_ScrollbarThumbSelectedColor;

        /// <summary>
        /// The cached scroll area bounds. Used to scroll contents of the panel control. Cached during performing layout.
        /// </summary>
        protected Rectangle m_ControlsBounds;

        /// <summary>
        /// The vertical scroll bar.
        /// </summary>
        public VScrollBar vScrollBar;

        /// <summary>
        /// The horizontal scroll bar.
        /// </summary>
        public HScrollBar hScrollBar;

        /// <summary>
        /// Gets the view bottom.
        /// </summary>
        public Float2 GetViewBottom()
        {
            return m_ViewOffset + Size;
        }

        /// <summary>
        /// Gets the cached scroll area bounds. Used to scroll contents of the panel control. Cached during performing layout.
        /// </summary>
        public Rectangle GetControlsBounds()
        {
            return m_ControlsBounds;
        }

        /// <summary>
        /// Gets or sets the scroll bars usage by this panel.
        /// </summary>
        // [EditorDisplay("Scrollbar Style"), EditorOrder(1500), Tooltip("The scroll bars usage.")]
        public ScrollBars GetScrollBars()
        {
            return m_ScrollBars;
        }

        public void SetScrollBars(ScrollBars value)
        {
            if (m_ScrollBars == value)
            {
                return;
            }

            m_ScrollBars = value;
            if (((int)value & (int)ScrollBars.Vertical) == (int)ScrollBars.Vertical)
            {
                if (vScrollBar == null)
                {
                    vScrollBar = GetChild<VScrollBar>();
                }

                if (vScrollBar == null)
                {
                    vScrollBar = new VScrollBar(this, Width - m_ScrollBarsSize, Height);
                    vScrollBar.AnchorPreset = AnchorPresets.TopLeft;

                    //VScrollBar.X += VScrollBar.Width;
                    vScrollBar.ValueChanged += () =>
                    {
                        SetViewOffset(Orientation.Vertical, vScrollBar.GetValue());
                    };
                }

                if (vScrollBar != null)
                {
                    vScrollBar.TrackColor = m_ScrollbarTrackColor;
                    vScrollBar.ThumbColor = m_ScrollbarThumbColor;
                    vScrollBar.ThumbSelectedColor = m_ScrollbarThumbSelectedColor;
                }
            }
            else if (vScrollBar != null)
            {
                vScrollBar.Dispose();
                vScrollBar = null;
            }

            if (((int)value & (int)ScrollBars.Horizontal) == (int)ScrollBars.Horizontal)
            {
                if (hScrollBar == null)
                {
                    hScrollBar = GetChild<HScrollBar>();
                }

                if (hScrollBar == null)
                {
                    hScrollBar = new HScrollBar(this, Height - m_ScrollBarsSize, Width);
                    hScrollBar.AnchorPreset = AnchorPresets.TopLeft;

                    //HScrollBar.Y += HScrollBar.Height;
                    //HScrollBar.Offsets += new Margin(0, 0, HScrollBar.Height * 0.5f, 0);
                    hScrollBar.ValueChanged += () =>
                    {
                        SetViewOffset(Orientation.Horizontal, hScrollBar.GetValue());
                    };
                }

                if (hScrollBar != null)
                {
                    hScrollBar.TrackColor = m_ScrollbarTrackColor;
                    hScrollBar.ThumbColor = m_ScrollbarThumbColor;
                    hScrollBar.ThumbSelectedColor = m_ScrollbarThumbSelectedColor;
                }
            }
            else if (hScrollBar != null)
            {
                hScrollBar.Dispose();
                hScrollBar = null;
            }
            PerformLayout();
        }

        /// <summary>
        /// Gets or sets the size of the scroll bars.
        /// </summary>
        // [EditorDisplay("Scrollbar Style"), EditorOrder(1501), Tooltip("Scroll bars size.")]
        public float GetScrollBarsSize()
        {
            return m_ScrollBarsSize;
        }

        public void SetScrollBarsSize(float value)
        {
            if (Mathf.NearEqual(m_ScrollBarsSize, value))
            {
                return;
            }

            m_ScrollBarsSize = value;
            PerformLayout();
        }

        /// <summary>
        /// Gets or sets a value indicating whether always show scrollbars. Otherwise show them only if scrolling is available.
        /// </summary>
        // [EditorDisplay("Scrollbar Style"), EditorOrder(1502), Tooltip("Whether always show scrollbars. Otherwise show them only if scrolling is available.")]
        public bool GetAlwaysShowScrollbars()
        {
            return m_AlwaysShowScrollbars;
        }

        public void SetAlwaysShowScrollbars(bool value)
        {
            if (m_AlwaysShowScrollbars != value)
            {
                m_AlwaysShowScrollbars = value;
                switch (m_ScrollBars)
                {
                    case ScrollBars.None:
                        break;
                    case ScrollBars.Horizontal:
                        hScrollBar.Visible = value;
                        break;
                    case ScrollBars.Vertical:
                        vScrollBar.Visible = value;
                        break;
                    case ScrollBars.Both:
                        hScrollBar.Visible = value;
                        vScrollBar.Visible = value;
                        break;
                    default:
                        break;
                }

                PerformLayout();
            }
        }

        /// <summary>
        /// Gets or sets the scroll margin applies to the child controls area. Can be used to expand the scroll area bounds by adding a margin.
        /// </summary>
        // [EditorDisplay("Scrollbar Style"), EditorOrder(1503), Tooltip("Scroll margin applies to the child controls area. Can be used to expand the scroll area bounds by adding a margin.")]
        public Margin GetScrollMargin()
        {
            return m_ScrollMargin;
        }

        public void SetScrollMargin(Margin value)
        {
            if (m_ScrollMargin != value)
            {
                m_ScrollMargin = value;
                PerformLayout();
            }
        }

        /// <summary>
        /// The color of the scroll bar track.
        /// </summary>
        // [EditorDisplay("Scrollbar Style"), EditorOrder(1600), ExpandGroups]
        public Color GetScrollbarTrackColor()
        {
            return m_ScrollbarTrackColor;
        }

        public void SetScrollbarTrackColor(Color value)
        {
            m_ScrollbarTrackColor = value;
            if (vScrollBar != null)
            {
                vScrollBar.TrackColor = m_ScrollbarTrackColor;
            }

            if (hScrollBar != null)
            {
                hScrollBar.TrackColor = m_ScrollbarTrackColor;
            }
        }

        /// <summary>
        /// The color of the scroll bar thumb.
        /// </summary>
        // [EditorDisplay("Scrollbar Style"), EditorOrder(1601)]
        public Color GetScrollbarThumbColor()
        {
            return m_ScrollbarThumbColor;
        }

        public void SetScrollbarThumbColor(Color value)
        {
            m_ScrollbarThumbColor = value;
            if (vScrollBar != null)
            {
                vScrollBar.ThumbColor = m_ScrollbarThumbColor;
            }

            if (hScrollBar != null)
            {
                hScrollBar.ThumbColor = m_ScrollbarThumbColor;
            }
        }

        /// <summary>
        /// The color of the scroll bar thumb when selected.
        /// </summary>
        // [EditorDisplay("Scrollbar Style"), EditorOrder(1602)]
        public Color GetScrollbarThumbSelectedColor()
        {
            return m_ScrollbarThumbSelectedColor;
        }

        public void SetScrollbarThumbSelectedColor(Color value)
        {
            m_ScrollbarThumbSelectedColor = value;
            if (vScrollBar != null)
            {
                vScrollBar.ThumbSelectedColor = m_ScrollbarThumbSelectedColor;
            }

            if (hScrollBar != null)
            {
                hScrollBar.ThumbSelectedColor = m_ScrollbarThumbSelectedColor;
            }
        }

        public Panel()
        {
            // Initialize the palette before SetScrollBars creates its controls.
            var style = Style.Current;
            m_ScrollbarTrackColor = style.BackgroundHighlighted;
            m_ScrollbarThumbColor = style.BackgroundNormal;
            m_ScrollbarThumbSelectedColor = style.BackgroundSelected;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Panel"/> class.
        /// </summary>
        /// <param name="scrollBars">The scroll bars.</param>
        /// <param name="autoFocus">True if control can accept user focus</param>
        public Panel(ScrollBars scrollBars, bool autoFocus = false) : this()
        {
            AutoFocus = autoFocus;
            SetScrollBars(scrollBars);
        }

        /// <inheritdoc />
        public override void SetViewOffset(Float2 value)
        {
            bool wasLocked = IsLayoutLocked;
            IsLayoutLocked = true;
            if (hScrollBar != null)
            {
                hScrollBar.SetValue(-value.X);
            }

            if (vScrollBar != null)
            {
                vScrollBar.SetValue(-value.Y);
            }

            IsLayoutLocked = wasLocked;
            base.SetViewOffset(value);
        }

        /// <summary>
        /// Cuts the scroll bars value smoothing and imminently goes to the target scroll value.
        /// </summary>
        public void FastScroll()
        {
            if (hScrollBar != null)
            {
                hScrollBar.FastScroll();
            }

            if (vScrollBar != null)
            {
                vScrollBar.FastScroll();
            }
        }

        /// <summary>
        /// Scrolls the view to the given control area.
        /// </summary>
        /// <param name="c">The control.</param>
        /// <param name="fastScroll">True of scroll to the item quickly without smoothing.</param>
        public void ScrollViewTo(Control c, bool fastScroll = false)
        {
            SE.Debug.Assert(c != null, "Invalid control.");

            Float2 location = c.Location;
            Float2 size = c.Size;
            while (c.HasParent && c.Parent != this)
            {
                c = c.Parent;
                location = c.PointToParent(location);
            }

            if (c.HasParent)
            {
                ScrollViewTo(new Rectangle(location, size), fastScroll);
            }
        }

        /// <summary>
        /// Scrolls the view to the given location.
        /// </summary>
        /// <param name="location">The location.</param>
        /// <param name="fastScroll">True of scroll to the item quickly without smoothing.</param>
        public void ScrollViewTo(Float2 location, bool fastScroll = false)
        {
            ScrollViewTo(new Rectangle(location, Float2.Zero), fastScroll);
        }

        /// <summary>
        /// Scrolls the view to the given area.
        /// </summary>
        /// <param name="bounds">The bounds.</param>
        /// <param name="fastScroll">True of scroll to the item quickly without smoothing.</param>
        public void ScrollViewTo(Rectangle bounds, bool fastScroll = false)
        {
            bool wasLocked = IsLayoutLocked;
            IsLayoutLocked = true;
            if (hScrollBar != null && hScrollBar.Enabled)
            {
                hScrollBar.ScrollViewTo(bounds.Left, bounds.Right, fastScroll);
            }

            if (vScrollBar != null && vScrollBar.Enabled)
            {
                vScrollBar.ScrollViewTo(bounds.Top, bounds.Bottom, fastScroll);
            }

            IsLayoutLocked = wasLocked;
            PerformLayout();
        }

        public void SetViewOffset(Orientation orientation, float value)
        {
            if (orientation == Orientation.Vertical)
            {
                m_ViewOffset.Y = -value;
            }
            else
            {
                m_ViewOffset.X = -value;
            }

            OnViewOffsetChanged();
            PerformLayout();
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            if (base.OnMouseDown(location, button))
            {
                return true;
            }

            return AutoFocus && Focus(this);
        }

        /// <inheritdoc />
        public override bool OnMouseWheel(Float2 location, float delta)
        {
            // Base
            if (base.OnMouseWheel(location, delta))
            {
                return true;
            }

            if (Input.GetKey(KeyboardKeys.Shift))
            {
                if (hScrollBar != null && hScrollBar.Enabled &&
                    hScrollBar.OnMouseWheel(hScrollBar.PointFromParent(location), delta))
                {
                    return true;
                }
            }

            // Roll back to scroll bars
            if (vScrollBar != null && vScrollBar.Enabled &&
                vScrollBar.OnMouseWheel(vScrollBar.PointFromParent(location), delta))
            {
                return true;
            }

            // No event handled
            return false;
        }

        /// <inheritdoc />
        public override void RemoveChildren()
        {
            // Keep scroll bars alive
            if (vScrollBar != null)
            {
                m_Children.Remove(vScrollBar);
            }

            if (hScrollBar != null)
            {
                m_Children.Remove(hScrollBar);
            }

            base.RemoveChildren();

            // Restore scrollbars
            if (vScrollBar != null)
            {
                m_Children.Add(vScrollBar);
            }

            if (hScrollBar != null)
            {
                m_Children.Add(hScrollBar);
            }

            PerformLayout();
        }

        /// <inheritdoc />
        public override void DisposeChildren()
        {
            // Keep scrollbars alive
            if (vScrollBar != null)
            {
                m_Children.Remove(vScrollBar);
            }

            if (hScrollBar != null)
            {
                m_Children.Remove(hScrollBar);
            }

            base.DisposeChildren();

            // Restore scrollbars
            if (vScrollBar != null)
            {
                m_Children.Add(vScrollBar);
            }

            if (hScrollBar != null)
            {
                m_Children.Add(hScrollBar);
            }

            PerformLayout();
        }

        /// <inheritdoc />
        public override void OnChildResized(Control control)
        {
            base.OnChildResized(control);
            if (control.IsScrollable)
            {
                PerformLayout();
            }
        }

        /// <inheritdoc />
        public override void Draw()
        {
            base.Draw();

            // Draw scrollbars manually (they are outside the clipping bounds)
            if (vScrollBar != null && vScrollBar.Visible)
            {
                Render2D.PushTransform(vScrollBar.CachedTransform);
                vScrollBar.Draw();
                Render2D.PopTransform();
            }
            if (hScrollBar != null && hScrollBar.Visible)
            {
                Render2D.PushTransform(hScrollBar.CachedTransform);
                hScrollBar.Draw();
                Render2D.PopTransform();
            }
        }

        /// <inheritdoc />
        public override bool ContainsPoint(Float2 location, bool precise)
        {
            if (precise && BackgroundColor.A <= 0.0f) // Go through transparency
            {
                return false;
            }

            return base.ContainsPoint(location, precise);
        }

        /// <inheritdoc />
        public override bool IntersectsChildContent(Control child, Float2 location, out Float2 childSpaceLocation)
        {
            // For not scroll bars we want to reject any collisions
            if (child != vScrollBar && child != hScrollBar)
            {
                // Check if has v scroll bar to reject points on it
                if (vScrollBar != null && vScrollBar.Enabled)
                {
                    Float2 pos = vScrollBar.PointFromParent(location);
                    if (vScrollBar.ContainsPoint(pos))
                    {
                        childSpaceLocation = Float2.Zero;
                        return false;
                    }
                }

                // Check if has h scroll bar to reject points on it
                if (hScrollBar != null && hScrollBar.Enabled)
                {
                    Float2 pos = hScrollBar.PointFromParent(location);
                    if (hScrollBar.ContainsPoint(pos))
                    {
                        childSpaceLocation = Float2.Zero;
                        return false;
                    }
                }
            }

            return base.IntersectsChildContent(child, location, out childSpaceLocation);
        }

        /// <inheritdoc />
        public override void AddChildInternal(Control child)
        {
            base.AddChildInternal(child);
            if (child.IsScrollable)
            {
                PerformLayout();
            }
        }

        /// <inheritdoc />
        public override void PerformLayout(bool force = false)
        {
            if (m_LayoutUpdateLock > 2)
            {
                return;
            }

            m_LayoutUpdateLock++;
            if (!IsLayoutLocked)
            {
                m_LayoutChanged = false;
            }

            base.PerformLayout(force);
            if (!IsLayoutLocked && m_LayoutChanged)
            {
                m_LayoutChanged = false;
                PerformLayout(true);
            }
            m_LayoutUpdateLock--;
        }

        /// <inheritdoc />
        public override Rectangle GetDesireClientArea()
        {
            Rectangle rect = new Rectangle(Float2.Zero, Size);
            if (vScrollBar != null && vScrollBar.Visible)
            {
                rect.Width -= vScrollBar.Width;
            }

            if (hScrollBar != null && hScrollBar.Visible)
            {
                rect.Height -= hScrollBar.Height;
            }

            return rect;
        }

        /// <inheritdoc />
        public override DragDropEffect OnDragMove(ref Float2 location, DragData data)
        {
            DragDropEffect result = base.OnDragMove(ref location, data);

            float width = Width;
            float height = Height;
            float minSize = 70;
            float areaSize = 25;
            float moveScale = 4.0f;
            Float2 viewOffset = -m_ViewOffset;

            if (vScrollBar != null && vScrollBar.Enabled && height > minSize)
            {
                if (new Rectangle(0, 0, width, areaSize).Contains(location))
                {
                    viewOffset.Y -= moveScale;
                }
                else if (new Rectangle(0, height - areaSize, width, areaSize).Contains(location))
                {
                    viewOffset.Y += moveScale;
                }

                viewOffset.Y = Math.Clamp(viewOffset.Y, vScrollBar.GetMinimum(), vScrollBar.GetMaximum());
                vScrollBar.SetValue(viewOffset.Y);
            }

            if (hScrollBar != null && hScrollBar.Enabled && width > minSize)
            {
                if (new Rectangle(0, 0, areaSize, height).Contains(location))
                {
                    viewOffset.X -= moveScale;
                }
                else if (new Rectangle(width - areaSize, 0, areaSize, height).Contains(location))
                {
                    viewOffset.X += moveScale;
                }

                viewOffset.X = Math.Clamp(viewOffset.X, hScrollBar.GetMinimum(), hScrollBar.GetMaximum());
                hScrollBar.SetValue(viewOffset.X);
            }

            viewOffset *= -1;
            if (viewOffset != m_ViewOffset)
            {
                m_ViewOffset = viewOffset;
                OnViewOffsetChanged();
                PerformLayout();
            }

            return result;
        }

        /// <inheritdoc />
        protected override void PerformLayoutBeforeChildren()
        {
            // Arrange controls and get scroll bounds
            ArrangeAndGetBounds();

            // Update scroll bars
            Rectangle controlsBounds = m_ControlsBounds;
            Rectangle scrollBounds = controlsBounds;
            m_ScrollMargin.ExpandRectangle(ref scrollBounds);
            if (vScrollBar != null)
            {
                float height = Height;
                bool vScrollEnabled = (controlsBounds.Bottom > height + 0.01f || controlsBounds.Y < 0.0f) && height > m_ScrollBarsSize;
                if (vScrollBar.Enabled != vScrollEnabled)
                {
                    // Set scroll bar visibility
                    vScrollBar.Enabled = vScrollEnabled;
                    vScrollBar.Visible = vScrollEnabled || m_AlwaysShowScrollbars;
                    m_LayoutChanged = true;

                    // Clear scroll state
                    vScrollBar.Reset();
                    m_ViewOffset.Y = 0;
                    OnViewOffsetChanged();

                    // Get the new bounds after changing scroll
                    ArrangeAndGetBounds();
                }

                if (vScrollEnabled)
                {
                    vScrollBar.SetScrollRange(
                        scrollBounds.Top,
                        MathF.Max(MathF.Max(0.0f, scrollBounds.Top), scrollBounds.Height - height));
                }

                vScrollBar.Bounds = new Rectangle(Width - m_ScrollBarsSize, 0, m_ScrollBarsSize, Height);
            }

            if (hScrollBar != null)
            {
                float width = Width;
                bool hScrollEnabled = controlsBounds.Right > width + 0.01f || controlsBounds.X < 0.0f && width > m_ScrollBarsSize;
                if (hScrollBar.Enabled != hScrollEnabled)
                {
                    // Set scroll bar visibility
                    hScrollBar.Enabled = hScrollEnabled;
                    hScrollBar.Visible = hScrollEnabled || m_AlwaysShowScrollbars;
                    m_LayoutChanged = true;

                    // Clear scroll state
                    hScrollBar.Reset();
                    m_ViewOffset.X = 0;
                    OnViewOffsetChanged();

                    // Get the new bounds after changing scroll
                    ArrangeAndGetBounds();
                }

                if (hScrollEnabled)
                {
                    hScrollBar.SetScrollRange(
                        scrollBounds.Left,
                        MathF.Max(MathF.Max(0.0f, scrollBounds.Left), scrollBounds.Width - width));
                }

                hScrollBar.Bounds = new Rectangle(0, Height - m_ScrollBarsSize,
                    Width - (vScrollBar != null && vScrollBar.Visible ? vScrollBar.Width : 0), m_ScrollBarsSize);
            }
        }

        /// <summary>
        /// Arranges the child controls and gets their bounds.
        /// </summary>
        protected virtual void ArrangeAndGetBounds()
        {
            Arrange();

            // Calculate scroll area bounds
            Float2 totalMin = Float2.Zero;
            Float2 totalMax = Float2.Zero;
            for (int i = 0; i < m_Children.Count; i++)
            {
                Control child = m_Children[i];
                if (child.Visible && child.IsScrollable)
                {
                    Float2 min = Float2.Zero;
                    Float2 max = child.Size;
                    Matrix3x3 transform = child.CachedTransform;
                    min = Matrix3x3.Transform2D(min, transform);
                    max = Matrix3x3.Transform2D(max, transform);
                    totalMin = Float2.Min(min, totalMin);
                    totalMax = Float2.Max(max, totalMax);
                }
            }

            // Cache result
            m_ControlsBounds = new Rectangle(totalMin, totalMax - totalMin);
        }

        /// <summary>
        /// Arranges the child controls.
        /// </summary>
        protected virtual void Arrange()
        {
            base.PerformLayoutBeforeChildren();
        }
    }
}
