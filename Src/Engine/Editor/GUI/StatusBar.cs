using SE.GUI;

namespace SE.Editor.GUI
{
    /// <summary>
    /// Status strip GUI control.
    /// </summary>
    /// <seealso cref="ContainerControl" />
    public class StatusBar : ContainerControl
    {
        /// <summary>
        /// The default height.
        /// </summary>
        public const int DefaultHeight = 22;

        /// <summary>
        /// Gets or sets the color of the status strip.
        /// </summary>
        public Color StatusColor
        {
            get
            {
                return BackgroundColor;
            }
            set
            {
                BackgroundColor = value;
            }
        }

        /// <summary>
        /// Gets or sets the status text.
        /// </summary>
        public string Text = string.Empty;

        /// <summary>
        /// Gets or sets the status text color
        /// </summary>
        public Color TextColor = Style.Current.Foreground;

        /// <summary>
        /// Initializes a new instance of the <see cref="StatusBar"/> class.
        /// </summary>
        public StatusBar()
        {
            AutoFocus = false;
            AnchorPreset = AnchorPresets.HorizontalStretchBottom;
        }

        /// <inheritdoc />
        public override void Draw()
        {
            base.Draw();

            Style style = Style.Current;

            // Match the thickness and color of the docked window splitters.
            Render2D.FillRectangle(new Rectangle(0, 0, Width, SplitPanel.SplitterSize), style.LightBackground);

            // Draw size grip
            WindowRootControl window = Root as WindowRootControl;
            if (window != null && !window.IsMaximized())
            {
                Render2D.DrawSprite(style.StatusBarSizeGrip, new Rectangle(Width - 12, 10, 12, 12), style.Foreground);
            }

            // Draw status text
            Render2D.RenderText(style.FontSmall, Text, new Rectangle(4, 0, Width - 20, Height), TextColor, TextAlignment.Near, TextAlignment.Center);
        }
    }
}
