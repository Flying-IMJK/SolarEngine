using System;
using SE.GUI;

namespace SE.Editor.GUI
{
    /// <summary>
    /// A label that exposes left, right and double-click notifications.
    /// </summary>
    public class ClickableLabel : Label
    {
        private bool _leftDown;
        private bool _rightDown;

        public ClickableLabel()
        {
        }

        public ClickableLabel(Rectangle bounds, string text = "")
            : base(bounds, text)
        {
        }

        public event Action<ClickableLabel>? DoubleClicked;
        public event Action<ClickableLabel>? LeftClicked;
        public event Action<ClickableLabel>? RightClicked;

        public override bool OnMouseDown(Float2 location, int button)
        {
            if (button == 1)
                _leftDown = true;
            else if (button == 3)
                _rightDown = true;
            return button == 1 || button == 3;
        }

        public override bool OnMouseUp(Float2 location, int button)
        {
            bool inBounds = location.X >= 0.0f && location.Y >= 0.0f && location.X <= Width && location.Y <= Height;
            if (button == 1 && _leftDown)
            {
                _leftDown = false;
                if (inBounds)
                    LeftClicked?.Invoke(this);
                return true;
            }
            if (button == 3 && _rightDown)
            {
                _rightDown = false;
                if (inBounds)
                    RightClicked?.Invoke(this);
                return true;
            }
            return false;
        }

        public override bool OnMouseDoubleClick(Float2 location, int button)
        {
            if (button != 1)
                return false;
            DoubleClicked?.Invoke(this);
            return true;
        }

        public override void OnMouseLeave()
        {
            _leftDown = false;
            _rightDown = false;
        }
    }
}
