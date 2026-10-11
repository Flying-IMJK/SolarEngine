#nullable disable annotations

using System;

namespace SE.Editor
{
    /// <summary>
    /// Clickable property name label that fires mouse events.
    /// </summary>
    public class ClickablePropertyNameLabel : PropertyNameLabel
    {
        /// <summary>
        /// Mouse action delegate.
        /// </summary>
        public delegate void MouseDelegate(ClickablePropertyNameLabel label, Float2 location);

        /// <summary>The mouse left button clicks on the label.</summary>
        public MouseDelegate MouseLeftClick;

        /// <summary>The mouse right button clicks on the label.</summary>
        public MouseDelegate MouseRightClick;

        /// <summary>The mouse left button double clicks on the label.</summary>
        public MouseDelegate MouseLeftDoubleClick;

        /// <summary>The mouse right button double clicks on the label.</summary>
        public MouseDelegate MouseRightDoubleClick;

        public ClickablePropertyNameLabel(string name) : base(name)
        {
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Left)
            {
                if (MouseLeftClick != null)
                {
                    MouseLeftClick(this, location);
                    return true;
                }
            }
            else if (button == MouseButton.Right)
            {
                if (MouseRightClick != null)
                {
                    MouseRightClick(this, location);
                    return true;
                }
            }
            return base.OnMouseUp(location, button);
        }

        /// <inheritdoc />
        public override bool OnMouseDoubleClick(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Left)
            {
                if (MouseLeftDoubleClick != null)
                {
                    MouseLeftDoubleClick(this, location);
                    return true;
                }
            }
            else if (button == MouseButton.Right)
            {
                if (MouseRightDoubleClick != null)
                {
                    MouseRightDoubleClick(this, location);
                    return true;
                }
            }
            return base.OnMouseDoubleClick(location, button);
        }
    }
}
