

using SE.GUI;

namespace SE.Editor
{
    /// <summary>
    /// <see cref="CustomEditor"/> properties list control.
    /// </summary>
    /// <seealso cref="SE.GUI.PanelWithMargins" />
    [HideInEditor]
    public class PropertiesList : PanelWithMargins
    {
        // TODO: sync splitter for whole presenter

        private const float SplitterPadding = 15;
        private const float EditorsMinWidthRatio = 0.4f;

        /// <summary>
        /// The splitter size (in pixels).
        /// </summary>
        public const int SplitterSize = 2;

        private PropertiesListElement m_Element;
        private float m_SplitterValue;
        private Rectangle m_SplitterRect;
        private bool m_SplitterClicked, m_MouseOverSplitter;
        private bool m_CursorChanged;
        private bool m_HasCustomSplitterValue;

        /// <summary>
        /// Gets or sets the splitter value (always in range [0; 1]).
        /// </summary>
        /// <value>
        /// The splitter value (always in range [0; 1]).
        /// </value>
        public float SplitterValue
        {
            get => m_SplitterValue;
            set
            {
                value = Mathf.Clamp(value, 0.05f, 0.95f);
                if (!Mathf.NearEqual(m_SplitterValue, value))
                {
                    m_SplitterValue = value;
                    UpdateSplitRect();
                    PerformLayout(true);
                }
            }
        }

        /// <summary>
        /// Gets the properties list element. It's a parent object for this control.
        /// </summary>
        public PropertiesListElement Element => m_Element;

        /// <summary>
        /// Initializes a new instance of the <see cref="PropertiesList"/> class.
        /// </summary>
        /// <param name="element">The element.</param>
        public PropertiesList(PropertiesListElement element)
        {
            ClipChildren = false;
            m_Element = element;
            m_SplitterValue = 0.4f;
            Margin = new Margin();
            Spacing = 2.0f;
            UpdateSplitRect();
        }

        private void AutoSizeSplitter(bool ignoreCustomSplitterValue = false)
        {
            if (m_HasCustomSplitterValue && !ignoreCustomSplitterValue)
            {
                return;
            }

            Font font = Style.Current.FontMedium;
            if (font == null || Width <= 0)
            {
                return;
            }

            float largestWidth = 0f;
            for (int i = 0; i < m_Element.Labels.Count; i++)
            {
                Label currentLabel = m_Element.Labels[i];
                Float2 dimensions = font.MeasureText(currentLabel.Text, TextLayoutOptions.Default);
                float width = dimensions.X + currentLabel.Margin.Left + SplitterPadding;

                largestWidth = Mathf.Max(largestWidth, width);
            }

            SplitterValue = Mathf.Clamp(largestWidth / Width, 0, 1 - EditorsMinWidthRatio);
        }

        private void UpdateSplitRect()
        {
            m_SplitterRect = new Rectangle(Mathf.Clamp(m_SplitterValue * Width - SplitterSize * 0.5f, 0.0f, Width),
                0, SplitterSize, Height);
            LeftMargin = m_SplitterValue * Width + _spacing;
        }

        private void StartTracking()
        {
            // Start move
            m_SplitterClicked = true;

            // Start capturing mouse
            StartMouseCapture();
        }

        private void EndTracking()
        {
            if (m_SplitterClicked)
            {
                // Clear flag
                m_SplitterClicked = false;

                // End capturing mouse
                EndMouseCapture();
            }
        }

        /// <inheritdoc />
        public override void Draw()
        {
            base.Draw();

            Style style = Style.Current;

            // Draw splitter
            Render2D.FillRectangle(m_SplitterRect, m_SplitterClicked ? style.BackgroundSelected
                : m_MouseOverSplitter ? style.BackgroundHighlighted : style.Background * 0.8f);
        }

        /// <inheritdoc />
        public override void OnLostFocus()
        {
            EndTracking();

            base.OnLostFocus();
        }

        /// <inheritdoc />
        public override void OnMouseMove(Float2 location)
        {
            m_MouseOverSplitter = m_SplitterRect.Contains(location);

            if (m_SplitterClicked)
            {
                SplitterValue = location.X / Width;
                Cursor = CursorType.SizeWE;
                m_CursorChanged = true;
                m_HasCustomSplitterValue = true;
            }
            else if (m_MouseOverSplitter)
            {
                Cursor = CursorType.SizeWE;
                m_CursorChanged = true;
            }
            else if (m_CursorChanged)
            {
                Cursor = CursorType.Default;
                m_CursorChanged = false;
            }

            base.OnMouseMove(location);
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Left)
            {
                if (m_SplitterRect.Contains(location))
                {
                    // Start moving splitter
                    StartTracking();
                    return false;
                }
            }

            return base.OnMouseDown(location, button);
        }

        /// <inheritdoc />
        public override bool OnMouseDoubleClick(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Left && m_SplitterRect.Contains(location))
            {
                if (m_SplitterClicked)
                {
                    EndTracking();
                }

                AutoSizeSplitter(true);
                return true;
            }

            return base.OnMouseDoubleClick(location, button);
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (m_SplitterClicked)
            {
                EndTracking();
                return true;
            }

            return base.OnMouseUp(location, button);
        }

        /// <inheritdoc />
        public override void OnMouseLeave()
        {
            // Clear flag
            m_MouseOverSplitter = false;

            if (m_CursorChanged)
            {
                Cursor = CursorType.Default;
                m_CursorChanged = false;
            }

            base.OnMouseLeave();
        }

        /// <inheritdoc />
        public override void OnEndMouseCapture()
        {
            // Clear flag
            m_SplitterClicked = false;
        }

        /// <inheritdoc />
        protected override void OnSizeChanged()
        {
            base.OnSizeChanged();

            // Refresh
            UpdateSplitRect();
            PerformLayout(true);
            // SolarEngine has no interface option for this; use the enabled Flax behavior.
            if (m_Element != null)
            {
                AutoSizeSplitter();
            }
        }

        /// <inheritdoc />
        protected override void PerformLayoutBeforeChildren()
        {
            base.PerformLayoutBeforeChildren();

            // Pre-set width of all controls
            float w = Mathf.Max(0, Width - _margin.Width);
            for (int i = 0; i < m_Children.Count; i++)
            {
                Control c = m_Children[i];
                if (!(c is PropertyNameLabel))
                {
                    c.Width = w;
                }
            }
        }

        /// <inheritdoc />
        protected override void PerformLayoutAfterChildren()
        {
            // Place non-label controls from top to down
            float y = _margin.Top;
            float w = Mathf.Max(0, Width - _margin.Width);
            bool firstItem = true;
            for (int i = 0; i < m_Children.Count; i++)
            {
                Control c = m_Children[i];
                if (!(c is PropertyNameLabel))
                {
                    Rectangle rect = new Rectangle(_margin.Left, y, w, c.Height);
                    if (c.Visible)
                    {
                        if (firstItem)
                        {
                            firstItem = false;
                        }
                        else
                        {
                            rect.Y += _spacing;
                        }
                    }
                    else if (!firstItem)
                    {
                        rect.Y += _spacing;
                    }
                    c.Bounds = rect;
                    if (c.Visible)
                    {
                        y = c.Bottom;
                    }
                }
            }
            y += _margin.Bottom;

            // Place labels accordingly to their respective controls placement
            float namesWidth = m_SplitterValue * Width;
            int count = m_Element.Labels.Count;
            float[] yStarts = new float[count + 1];
            for (int i = 0; i < count; i++)
            {
                PropertyNameLabel label = m_Element.Labels[i];
                PropertiesList container = label.FirstChildControlContainer ?? this;

                if (label.FirstChildControlIndex < 0)
                {
                    yStarts[i] = 0;
                }
                else if (container.ChildrenCount <= label.FirstChildControlIndex)
                {
                    yStarts[i] = y;
                }
                else if (label.FirstChildControlContainer != null)
                {
                    Control firstChild = label.FirstChildControlContainer.Children[label.FirstChildControlIndex];
                    yStarts[i] = firstChild.PointToParent(this, Float2.Zero).Y;
                    if (i == count - 1)
                    {
                        yStarts[i + 1] = firstChild.Parent.Bottom;
                    }
                }
                else
                {
                    Control firstChild = m_Children[label.FirstChildControlIndex];
                    yStarts[i] = firstChild.Top;
                    if (i == count - 1)
                    {
                        yStarts[i + 1] = firstChild.Bottom;
                    }
                }

            }
            for (int i = 0; i < count; i++)
            {
                PropertyNameLabel label = m_Element.Labels[i];

                Rectangle rect = new Rectangle(0, yStarts[i], namesWidth, yStarts[i + 1] - yStarts[i]);
                if (i != count - 1)
                {
                    rect.Height -= _spacing;
                }
                //label.Parent = this;
                label.Bounds = rect;
            }

            if (_autoSize)
            {
                Height = y;
            }
        }
    }
}
