#nullable disable annotations

using SE.GUI;

namespace SE.Editor.GUI
{
    /// <summary>
    /// A navigation bar control. Shows the current location path with UI buttons to navigate around.
    /// </summary>
    public class NavigationBar : Panel
    {
        /// <summary>
        /// The default buttons margin.
        /// </summary>
        public const float DefaultButtonsMargin = 2.0f;

        public NavigationBar() : base(ScrollBars.Horizontal)
        {
        }

        /// <summary>
        /// Updates the bar bounds and positions it after the last toolstrip button. Ensures to fit the toolstrip area (navigation bar horizontal scroll bar can be used to view the full path).
        /// </summary>
        /// <param name="toolstrip">The toolstrip.</param>
        public void UpdateBounds(ToolStrip toolstrip)
        {
            if (toolstrip == null)
            {
                return;
            }
            ToolStripButton lastToolstripButton = toolstrip.LastButton;
            Float2 parentSize = Parent.Size;
            Bounds = new Rectangle(lastToolstripButton.Right + 8.0f, 0,
                parentSize.X - X - 8.0f, toolstrip.Height);
        }

        /// <inheritdoc />
        protected override void Arrange()
        {
            base.Arrange();

            // Arrange buttons
            float x = DefaultButtonsMargin;
            for (int i = 0; i < m_Children.Count; i++)
            {
                Control child = m_Children[i];
                if (child.IsScrollable)
                {
                    child.X = x;
                    x += child.Width + DefaultButtonsMargin;
                }
            }
        }
    }

    /// <summary>
    /// A navigation bar button. Allows to change the current location and view the path.
    /// </summary>
    public class NavigationButton : Button
    {
        /// <summary>
        /// The valid drag is over flag.
        /// </summary>
        protected bool m_ValidDragOver = false;

        /// <summary>
        /// The default margin (horizontal).
        /// </summary>
        public const float DefaultMargin = 6.0f;

        public NavigationButton()
        {
            Height = 0;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="NavigationButton"/> class.
        /// </summary>
        /// <param name="x">The x position.</param>
        /// <param name="y">The y position.</param>
        /// <param name="height">The height.</param>
        public NavigationButton(float x, float y, float height)
            : base(x, y, 2.0f * DefaultMargin)
        {
            Height = height;
        }

        /// <inheritdoc />
        public override void Draw()
        {
            // Cache data
            Style style = Style.Current;
            Rectangle clientRect = new Rectangle(Float2.Zero, Size);
            Rectangle textRect = new Rectangle(4, 0, clientRect.Width - 4, clientRect.Height);

            // Draw background
            if (IsDragOver() && m_ValidDragOver)
            {
                Render2D.FillRectangle(clientRect, style.Selection);
                Render2D.DrawRectangle(clientRect, style.SelectionBorder);
            }
            else if (isPressed)
            {
                Render2D.FillRectangle(clientRect, style.BackgroundSelected);
            }
            else if (IsMouseOver)
            {
                Render2D.FillRectangle(clientRect, style.BackgroundHighlighted);
            }

            // Draw text
            Render2D.DrawText(style.FontMedium, Text, textRect,
                EnabledInHierarchy ? style.Foreground : style.ForegroundDisabled,
                TextAlignment.Near, TextAlignment.Center);
        }

        /// <inheritdoc />
        public override void PerformLayout(bool force = false)
        {
            Style style = Style.Current;

            if (style != null && style.FontMedium != null)
            {
                Width = style.FontMedium.MeasureText(Text, TextLayoutOptions.Default).X + 2.0f * DefaultMargin;
            }
        }
    }
}
