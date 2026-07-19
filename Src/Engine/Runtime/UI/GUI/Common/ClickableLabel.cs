using System;

namespace SE.GUI
{
    /// <summary>
    /// A text label that reports pointer clicks.
    /// </summary>
    public class ClickableLabel : Label
    {
        private bool _isPressed;

        public ClickableLabel()
        {
        }

        public ClickableLabel(Rectangle bounds, string text = "")
            : base(bounds, text)
        {
        }

        public event Action<ClickableLabel>? Clicked;

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
    }
}
