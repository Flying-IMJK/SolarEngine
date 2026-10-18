
using SE.GUI;

namespace SE.Editor.GUI
{
    public class MainMenuButton : Control
    {
        /// <summary>
        /// The button text.
        /// </summary>
        public string Text;

        /// <summary>
        /// The context menu.
        /// </summary>
        public ContextMenu ContextMenu = null;

        /// <summary>
        /// The background color when mouse is over.
        /// </summary>
        public Color BackgroundColorMouseOver = Color.Transparent;

        /// <summary>
        /// The background color when mouse is over and context menu is opened.
        /// </summary>
        public Color BackgroundColorMouseOverOpened = Color.Transparent;

        /// <summary>
        /// Initializes a new instance of the <see cref="MainMenuButton"/> class.
        /// </summary>
        /// <param name="text">The text.</param>
        public MainMenuButton(string text) : base(0, 0, 32, 16)
        {
            ContextMenu = new ContextMenu();

            Text = text;

            Style style = Style.Current;
            if (true)
            {
                BackgroundColorMouseOver = style.BackgroundHighlighted;
                BackgroundColorMouseOverOpened = style.Background;
            }
            else
            {
                BackgroundColorMouseOver = BackgroundColorMouseOverOpened = style.LightBackground * 1.3f;
            }
        }

        /// <inheritdoc />
        public override void Draw()
        {
            // Cache data
            Style style = Style.Current;
            Rectangle clientRect = new Rectangle(0, 0, Width, Height);
            bool hasChildItems = ContextMenu.HasChildren;
            bool isOpened = ContextMenu.IsOpened;
            bool enabled = EnabledInHierarchy;

            // Draw background
            if (enabled && hasChildItems && (isOpened || IsMouseOver))
            {
                Render2D.FillRectangle(clientRect,
                    isOpened ? BackgroundColorMouseOverOpened : BackgroundColorMouseOver);
            }

            // Draw text
            Render2D.DrawText(style.FontMedium, Text, clientRect,
                enabled && hasChildItems ? style.Foreground : style.ForegroundDisabled,
                TextAlignment.Center, TextAlignment.Center);
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            Focus();

            MainMenu mainMenu = Parent as MainMenu;
            if (mainMenu != null)
            {
                mainMenu.Selected = this;
            }

            return true;
        }

        /// <inheritdoc />
        public override void OnMouseEnter(Float2 location)
        {
            base.OnMouseEnter(location);

            MainMenu mainMenu = Parent as MainMenu;
            if (mainMenu != null && mainMenu.Selected != null)
            {
                mainMenu.Selected = this;
            }
        }

        /// <inheritdoc />
        public override void PerformLayout(bool force = false)
        {
            Style style = Style.Current;
            float width = 18;

            if (style.FontMedium != null)
            {
                width += style.FontMedium.MeasureText(Text, TextLayoutOptions.Default).X;
            }

            Width = width;
        }
    }
}
