#nullable disable annotations
using System;

namespace SE.GUI
{
    /// <summary>
    /// The label that contains events for mouse interaction.
    /// </summary>
    public class ClickableLabel : Label
    {
        private bool m_LeftClick = false;
        private bool m_IsRightDown = false;

        /// <summary>
        /// The double click event.
        /// </summary>
        public Action DoubleClick;

        /// <summary>
        /// The left mouse button click event.
        /// </summary>
        public Action LeftClick;

        /// <summary>
        /// The right mouse button click event.
        /// </summary>
        public Action RightClick;

        /// <inheritdoc />
        public override bool OnMouseDoubleClick(Float2 location, MouseButton button)
        {
            if (DoubleClick != null)
            {
                DoubleClick();
            }

            return base.OnMouseDoubleClick(location, button);
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Left)
            {
                m_LeftClick = true;
            }
            else if (button == MouseButton.Right)
            {
                m_IsRightDown = true;
            }

            return base.OnMouseDown(location, button);
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Left && m_LeftClick)
            {
                m_LeftClick = false;
                if (LeftClick != null)
                {
                    LeftClick();
                }
            }
            else if (button == MouseButton.Right && m_IsRightDown)
            {
                m_IsRightDown = false;
                if (RightClick != null)
                {
                    RightClick();
                }
            }

            return base.OnMouseUp(location, button);
        }

        /// <inheritdoc />
        public override void OnMouseLeave()
        {
            m_LeftClick = false;
            m_IsRightDown = false;

            base.OnMouseLeave();
        }
    }
}
