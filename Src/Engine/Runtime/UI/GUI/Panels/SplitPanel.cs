using System;

namespace SE.GUI
{
    /// <summary>
    /// GUI control that contains two child panels and the splitter between them.
    /// </summary>
    public class SplitPanel : ContainerControl
    {
        private Orientation m_Orientation;
        private float m_SplitterValue;
        private Rectangle m_SplitterRect;
        private bool m_SplitterClicked;
        private bool m_MouseOverSplitter;
        private bool m_CursorChanged;

        private void UpdateSplitRect()
        {
            if (m_Orientation == Orientation.Horizontal)
            {
                float split = (int)MathF.Round(m_SplitterValue * Width, MidpointRounding.AwayFromZero);
                float position = split - SplitterSizeHalf;
                float maximum = Width;
                if (!(0.0f > maximum))
                {
                    position = position < 0.0f ? 0.0f : position;
                    position = maximum < position ? maximum : position;
                }
                m_SplitterRect = new Rectangle(position, 0, SplitterSize, Height);
            }
            else
            {
                float split = (int)MathF.Round(m_SplitterValue * Height, MidpointRounding.AwayFromZero);
                float position = split - SplitterSizeHalf;
                float maximum = Height;
                if (!(0.0f > maximum))
                {
                    position = position < 0.0f ? 0.0f : position;
                    position = maximum < position ? maximum : position;
                }
                m_SplitterRect = new Rectangle(0, position, Width, SplitterSize);
            }
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

        /// <summary>
        /// The splitter size (in pixels).
        /// </summary>
        public const int SplitterSize = 4;

        /// <summary>
        /// The splitter half size (in pixels).
        /// </summary>
        public const int SplitterSizeHalf = SplitterSize / 2;

        /// <summary>
        /// The first panel (left or upper based on Orientation).
        /// </summary>
        public Panel Panel1;

        /// <summary>
        /// The second panel.
        /// </summary>
        public Panel Panel2;

        /// <summary>
        /// Gets or sets the panel orientation.
        /// </summary>
        /// <value>
        /// The orientation.
        /// </value>
        public Orientation Orientation
        {
            get => m_Orientation;
            set
            {
                if (m_Orientation != value)
                {
                    m_Orientation = value;
                    UpdateSplitRect();
                    PerformLayout();
                }
            }
        }

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
                value = value < 0 ? 0 : value < 1 ? value : 1;
                if (!(Math.Abs((double)m_SplitterValue - value) <= Mathf.Epsilon))
                {
                    // Set new value
                    m_SplitterValue = value;

                    // Calculate rectangle and update panels
                    UpdateSplitRect();
                    PerformLayout();
                }
            }
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="SplitPanel"/> class.
        /// </summary>
        /// <param name="orientation">The orientation.</param>
        /// <param name="panel1Scroll">The panel1 scroll bars.</param>
        /// <param name="panel2Scroll">The panel2 scroll bars.</param>
        public SplitPanel(
            Orientation orientation = Orientation.Horizontal,
            ScrollBars panel1Scroll = ScrollBars.Both,
            ScrollBars panel2Scroll = ScrollBars.Both)
        {
            AutoFocus = false;

            m_Orientation = orientation;
            m_SplitterValue = 0.5f;

            Panel1 = new Panel(panel1Scroll);
            Panel2 = new Panel(panel2Scroll);

            Panel1.Parent = this;
            Panel2.Parent = this;

            UpdateSplitRect();
        }

        /// <inheritdoc />
        public override void Draw()
        {
            base.Draw();

            // Draw splitter
            Style style = Style.Current;
            Render2D.FillRectangle(m_SplitterRect,
                m_SplitterClicked ? style.BackgroundSelected :
                m_MouseOverSplitter ? style.BackgroundHighlighted : style.LightBackground);
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
                SplitterValue = m_Orientation == Orientation.Horizontal ? location.X / Width : location.Y / Height;
                Cursor = m_Orientation == Orientation.Horizontal ? CursorType.SizeWE : CursorType.SizeNS;
                m_CursorChanged = true;
            }
            else if (m_MouseOverSplitter)
            {
                Cursor = m_Orientation == Orientation.Horizontal ? CursorType.SizeWE : CursorType.SizeNS;
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
            if (button == MouseButton.Left && m_SplitterRect.Contains(location))
            {
                // Start moving splitter
                StartTracking();
                Focus();
                return true;
            }

            return base.OnMouseDown(location, button);
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
            EndTracking();
        }

        /// <inheritdoc />
        protected override void OnSizeChanged()
        {
            base.OnSizeChanged();

            UpdateSplitRect();
            PerformLayout();
        }

        /// <inheritdoc />
        protected override void PerformLayoutBeforeChildren()
        {
            base.PerformLayoutBeforeChildren();

            if (m_Orientation == Orientation.Horizontal)
            {
                float split = (int)MathF.Round(m_SplitterValue * Width, MidpointRounding.AwayFromZero);
                Panel1.Bounds = new Rectangle(0, 0, split - SplitterSizeHalf, Height);
                Panel2.Bounds = new Rectangle(split + SplitterSizeHalf, 0, Width - split - SplitterSizeHalf, Height);
            }
            else
            {
                float split = (int)MathF.Round(m_SplitterValue * Height, MidpointRounding.AwayFromZero);
                Panel1.Bounds = new Rectangle(0, 0, Width, split - SplitterSizeHalf);
                Panel2.Bounds = new Rectangle(0, split + SplitterSizeHalf, Width, Height - split - SplitterSizeHalf);
            }
        }
    }
}
