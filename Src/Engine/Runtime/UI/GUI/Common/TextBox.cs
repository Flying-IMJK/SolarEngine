namespace SE.GUI
{
    /// <summary>
    /// Basic editable text box that renders its text through the managed font bindings.
    /// </summary>
    public class TextBox : TextBoxBase
    {
        public TextBox()
        {
        }

        public TextBox(Rectangle bounds, bool isMultiline = false)
            : base(bounds, isMultiline)
        {
            BackgroundColor = Style.Current.BackgroundNormal;
        }

        public Font? Font { get; set; }
        public Color TextColor { get; set; } = Style.Current.Foreground;
        public string PlaceholderText { get; set; } = string.Empty;
        public Color PlaceholderTextColor { get; set; } = Style.Current.ForegroundDisabled;
        public TextAlignment HorizontalAlignment { get; set; } = TextAlignment.Near;
        public TextAlignment VerticalAlignment { get; set; } = TextAlignment.Center;
        public TextWrapping TextWrapping { get; set; } = TextWrapping.NoWrap;

        protected override void OnDraw()
        {
            base.OnDraw();

            string text = string.IsNullOrEmpty(Text) ? PlaceholderText : Text;
            Font? font = Font ?? Style.Current.FontMedium;
            if (ReferenceEquals(font, null) || string.IsNullOrEmpty(text))
                return;

            Rectangle bounds = ScreenBounds;
            Color color = string.IsNullOrEmpty(Text) ? PlaceholderTextColor : TextColor;
            Render2D.RenderText(font, text, ref bounds, ref color, HorizontalAlignment, VerticalAlignment, TextWrapping);
        }
    }
}
