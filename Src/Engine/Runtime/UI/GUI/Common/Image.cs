using System;

namespace SE.GUI
{
    /// <summary>
    /// Displays a managed brush and optionally reports clicks.
    /// </summary>
    public class Image : ContainerControl
    {
        private bool _isPressed;

        public Image()
        {
            AutoFocus = false;
        }

        public Image(Rectangle bounds)
            : this()
        {
            Bounds = bounds;
        }

        public event Action<Image, int>? Clicked;

        public IBrush? Brush { get; set; }
        public Margin Margin { get; set; }
        public Color Color { get; set; } = Color.White;
        public Color MouseOverColor { get; set; } = Color.White;
        public Color DisabledTint { get; set; } = new Color(128, 128, 128);
        public bool KeepAspectRatio { get; set; } = true;

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
                Clicked?.Invoke(this, button);
            return true;
        }

        public override void ClearState()
        {
            _isPressed = false;
            base.ClearState();
        }

        protected override void OnDraw()
        {
            base.OnDraw();
            if (Brush == null)
                return;

            Rectangle bounds = ScreenBounds;
            Margin.ShrinkRectangle(ref bounds);
            if (bounds.Width <= 0.0f || bounds.Height <= 0.0f)
                return;

            Color tint = Color;
            if (!EnabledInHierarchy)
                tint *= DisabledTint;
            else if (IsMouseOver)
                tint *= MouseOverColor;
            Brush.Draw(bounds, tint);
        }
    }
}
