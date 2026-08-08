using System;
using SE.GUI;

namespace SE.Editor.GUI
{
    /// <summary>
    /// A label that exposes left, right and double-click notifications.
    /// </summary>
    public class ClickableLabel : Label
    {
        private bool m_LeftDown;
        private bool m_RightDown;

        public Action? DoubleClick;
        public Action? LeftClick;
        public Action? RightClick;

        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Left)
                m_LeftDown = true;
            else if (button == MouseButton.Right)
                m_RightDown = true;

            return base.OnMouseDown(location, button);
        }

        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Left && m_LeftDown)
            {
                m_LeftDown = false;
                LeftClick?.Invoke();
            }
            else if (button == MouseButton.Right && m_RightDown)
            {
                m_RightDown = false;
                RightClick?.Invoke();
            }

            return base.OnMouseUp(location, button);
        }

        public override bool OnMouseDoubleClick(Float2 location, MouseButton button)
        {
            DoubleClick?.Invoke();
            return base.OnMouseDoubleClick(location, button);
        }

        public override void OnMouseLeave()
        {
            m_LeftDown = false;
            m_RightDown = false;
            base.OnMouseLeave();
        }
    }
}
