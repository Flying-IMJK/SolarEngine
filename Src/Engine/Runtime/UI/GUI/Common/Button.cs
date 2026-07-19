using System;

namespace SE.GUI
{
    /// <summary>
    /// A managed clickable control with optional text rendering.
    /// </summary>
    public class Button : ContainerControl
    {
        private bool _isPressed;

        public Button()
            : this(Rectangle.Empty, string.Empty)
        {
        }

        public Button(Rectangle bounds, string text = "")
            : base(bounds)
        {
            Text = text;
            BackgroundColor = Style.Current.BackgroundNormal;
            BackgroundColorHighlighted = Style.Current.BackgroundHighlighted;
            BackgroundColorPressed = Style.Current.BackgroundSelected;
        }

        public event Action<Button>? Clicked;

        public string Text { get; set; }
        public Font? Font { get; set; }
        public Color TextColor { get; set; } = Style.Current.Foreground;
        public Color BackgroundColorHighlighted { get; set; }
        public Color BackgroundColorPressed { get; set; }
        public TextAlignment HorizontalAlignment { get; set; } = TextAlignment.Center;
        public TextAlignment VerticalAlignment { get; set; } = TextAlignment.Center;
        public TextWrapping TextWrapping { get; set; } = TextWrapping.NoWrap;
        public bool IsPressed => _isPressed;

        public override bool OnMouseDown(Float2 location, int button)
        {
            if (button != 1)
                return false;

            _isPressed = true;
            Root?.StartTrackingMouse(this);
            return true;
        }

        public override bool OnMouseUp(Float2 location, int button)
        {
            if (button != 1 || !_isPressed)
                return false;

            _isPressed = false;
            Root?.EndTrackingMouse();
            if (location.X >= 0.0f && location.Y >= 0.0f && location.X <= Width && location.Y <= Height)
                Clicked?.Invoke(this);
            return true;
        }

        public override void ClearState()
        {
            _isPressed = false;
            base.ClearState();
        }

        protected override void OnDraw()
        {
            Color background = _isPressed
                ? BackgroundColorPressed
                : IsMouseOver ? BackgroundColorHighlighted : BackgroundColor;
            if (!EnabledInHierarchy)
                background.A *= 0.5f;

            if (background.A > 0.0f)
            {
                Rectangle bounds = ScreenBounds;
                Render2D.FillRectangle(ref bounds, ref background);
            }

            Font? font = Font ?? Style.Current.FontMedium;
            if (ReferenceEquals(font, null) || string.IsNullOrEmpty(Text))
                return;

            Rectangle textBounds = ScreenBounds;
            Color textColor = TextColor;
            if (!EnabledInHierarchy)
                textColor.A *= 0.6f;
            Render2D.RenderText(font, Text, ref textBounds, ref textColor, HorizontalAlignment, VerticalAlignment, TextWrapping);
        }
    }
}
