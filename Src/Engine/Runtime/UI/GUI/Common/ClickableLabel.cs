using System;

namespace SE.GUI
{
    /// <summary>
    /// A text label that reports pointer clicks.
    /// </summary>
    public class ClickableLabel : Label
    {
        private bool m_IsPressed;

        public ClickableLabel()
        {
        }

        public ClickableLabel(Rectangle bounds, string text = "")
            : base(bounds, text)
        {
        }

        public event Action<ClickableLabel>? Clicked;

        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            if (button != MouseButton.Left)
                return false;

            m_IsPressed = true;
            Root?.StartTrackingMouse(this);
            return true;
        }

        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (button != MouseButton.Left || !m_IsPressed)
                return false;

            m_IsPressed = false;
            Root?.EndTrackingMouse();
            if (location.X >= 0.0f && location.Y >= 0.0f && location.X <= Width && location.Y <= Height)
                Clicked?.Invoke(this);
            return true;
        }

        public override void ClearState()
        {
            m_IsPressed = false;
            base.ClearState();
        }
    }
}
