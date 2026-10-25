
using SE.GUI;

namespace SE.Editor.GUI
{
    /// <summary>
    /// Search box control which can gather text search input from the user.
    /// </summary>
    public class SearchBox : TextBox
    {
        /// <summary>
        /// A button that clears the search bar.
        /// </summary>
        public Button ClearSearchButton;

        public SearchBox() : base(false, 0, 0)
        {
            ClearSearchButton = null;
        }

        /// <summary>
        /// Init search box
        /// </summary>
        public SearchBox(bool isMultiline, float x, float y, float width = 120.0f)
            : base(isMultiline, x, y, width)
        {
            WatermarkText = "Search...";

            ClearSearchButton = new Button();
            ClearSearchButton.Parent = this;
            ClearSearchButton.Width = 14.0f;
            ClearSearchButton.Height = 14.0f;
            ClearSearchButton.AnchorPreset = AnchorPresets.TopRight;
            ClearSearchButton.Text = string.Empty;
            ClearSearchButton.TooltipText = "Cancel Search.";
            ClearSearchButton.BackgroundColor = TextColor;
            ClearSearchButton.BorderColor = Color.Transparent;
            ClearSearchButton.BackgroundColorHighlighted = Style.Current.ForegroundGrey;
            ClearSearchButton.BorderColorHighlighted = Color.Transparent;
            ClearSearchButton.BackgroundColorSelected = Style.Current.ForegroundGrey;
            ClearSearchButton.BorderColorSelected = Color.Transparent;
            ClearSearchButton.BackgroundBrush = new SpriteBrush(EditorIcons.Cross16);
            ClearSearchButton.Visible = false;

            ClearSearchButton.LocalY = ClearSearchButton.LocalY + 2;
            ClearSearchButton.LocalX = ClearSearchButton.LocalX - 2;
            ClearSearchButton.Clicked += Clear;
            ClearSearchButton.HoverBegin += () =>
            {
                _changeCursor = false;
                Cursor = CursorType.Default;
            };
            ClearSearchButton.HoverEnd += () =>
            {
                _changeCursor = true;
            };

            TextChanged += () =>
            {
                ClearSearchButton.Visible = Text.Length != 0;
            };
        }
    }
}
