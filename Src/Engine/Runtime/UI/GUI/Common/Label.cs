using System;

namespace SE.GUI
{
    /// <summary>
    /// Draws a single text value using a managed <see cref="SE.Font"/>.
    /// </summary>
    public class Label : ContainerControl
    {
        public Label()
        {
            AutoFocus = false;
            IsScrollable = false;
        }

        public Label(Rectangle bounds, string text = "")
            : base(bounds)
        {
            AutoFocus = false;
            IsScrollable = false;
            Text = text;
        }

        public string Text { get; set; } = string.Empty;
        public Font? Font { get; set; }
        public Color TextColor { get; set; } = Color.White;
        public TextAlignment HorizontalAlignment { get; set; } = TextAlignment.Near;
        public TextAlignment VerticalAlignment { get; set; } = TextAlignment.Near;
        public TextWrapping TextWrapping { get; set; } = TextWrapping.NoWrap;

        protected override void OnDraw()
        {
            base.OnDraw();

            Font? font = Font ?? Style.Current.FontMedium;
            if (ReferenceEquals(font, null) || string.IsNullOrEmpty(Text))
                return;

            Rectangle bounds = ScreenBounds;
            Color color = TextColor;
            Render2D.RenderText(font, Text, ref bounds, ref color, HorizontalAlignment, VerticalAlignment, TextWrapping);
        }
    }
}
