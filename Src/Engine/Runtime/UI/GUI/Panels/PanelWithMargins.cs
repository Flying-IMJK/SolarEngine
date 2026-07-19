using System;

namespace SE.GUI
{
    /// <summary>
    /// Base panel for layouts that arrange controls within margins.
    /// </summary>
    public class PanelWithMargins : Panel
    {
        private Margin _margin = new Margin(2.0f);
        private float _spacing = 2.0f;
        private Float2 _offset;
        private bool _autoSize = true;

        public PanelWithMargins()
            : base(new Rectangle(0.0f, 0.0f, 64.0f, 64.0f))
        {
            AutoFocus = false;
        }

        public PanelWithMargins(Rectangle bounds)
            : this()
        {
            Bounds = bounds;
        }

        public Margin Margin
        {
            get => _margin;
            set
            {
                if (_margin == value)
                    return;
                _margin = value;
                PerformLayout();
            }
        }

        public float LeftMargin
        {
            get => _margin.Left;
            set => SetMargin(value, _margin.Right, _margin.Top, _margin.Bottom);
        }

        public float RightMargin
        {
            get => _margin.Right;
            set => SetMargin(_margin.Left, value, _margin.Top, _margin.Bottom);
        }

        public float TopMargin
        {
            get => _margin.Top;
            set => SetMargin(_margin.Left, _margin.Right, value, _margin.Bottom);
        }

        public float BottomMargin
        {
            get => _margin.Bottom;
            set => SetMargin(_margin.Left, _margin.Right, _margin.Top, value);
        }

        public float Spacing
        {
            get => _spacing;
            set
            {
                if (_spacing == value)
                    return;
                _spacing = value;
                PerformLayout();
            }
        }

        public Float2 Offset
        {
            get => _offset;
            set
            {
                if (_offset == value)
                    return;
                _offset = value;
                PerformLayout();
            }
        }

        public bool AutoSize
        {
            get => _autoSize;
            set
            {
                if (_autoSize == value)
                    return;
                _autoSize = value;
                PerformLayout();
            }
        }

        protected Rectangle ContentBounds => new Rectangle(
            _margin.Left + _offset.X,
            _margin.Top + _offset.Y,
            MathF.Max(0.0f, Width - _margin.Width),
            MathF.Max(0.0f, Height - _margin.Height));

        private void SetMargin(float left, float right, float top, float bottom)
        {
            Margin = new Margin(left, right, top, bottom);
        }
    }
}
