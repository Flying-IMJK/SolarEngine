using System;
using SE.GUI;

namespace SE.Editor.GUI.Dialogs
{
    // The hue/saturation wheel follows Flax's pointer conversion. SolarEngine
    // draws the wheel with gradient spokes because it has no HSWheelMaterial.
    public sealed class ColorSelectorWithSliders : ContainerControl
    {
        private readonly Rectangle _wheel;
        private readonly Rectangle _valueSlider;
        private readonly Rectangle _alphaSlider;
        private Color _color;
        private int _dragMode;

        public event Action<Color>? ColorChanged;
        public event Action? SlidingStart;
        public event Action? SlidingEnd;
        public Color Color
        {
            get => _color;
            set
            {
                if (_color == value) return;
                _color = value;
                ColorChanged?.Invoke(value);
            }
        }
        public bool IsSliding => _dragMode != 0;

        public ColorSelectorWithSliders(float wheelSize, float sliderThickness)
            : base(0, 0, wheelSize + 2 * sliderThickness + 25, wheelSize)
        {
            AutoFocus = true;
            _wheel = new Rectangle(0, 0, wheelSize, wheelSize);
            _valueSlider = new Rectangle(wheelSize + 10, 0, sliderThickness, wheelSize);
            _alphaSlider = new Rectangle(_valueSlider.Right + 15, 0, sliderThickness, wheelSize);
        }

        public override void Draw()
        {
            base.Draw();
            Float2 center = _wheel.Center;
            float radius = _wheel.Width * 0.5f;
            for (int angle = 0; angle < 360; angle += 2)
            {
                float radians = angle * Mathf.DegreesToRadians;
                var end = center + new Float2(Mathf.Cos(radians), -Mathf.Sin(radians)) * radius;
                Render2D.DrawLine(center, end, Color.White, Color.FromHSV(angle, 1, 1), 2.5f);
            }
            Float3 hsv = _color.ToHSV();
            float pointerAngle = hsv.X * Mathf.DegreesToRadians;
            var pointer = center + new Float2(Mathf.Cos(pointerAngle), -Mathf.Sin(pointerAngle)) *
                (hsv.Y * radius);
            var pointerRect = new Rectangle(pointer - new Float2(4), new Float2(8));
            Render2D.FillRectangle(pointerRect, Color.FromHSV(hsv.X, hsv.Y, 1));
            Render2D.DrawRectangle(pointerRect, Color.Black);

            var hueColor = Color.FromHSV(hsv.X, hsv.Y, 1);
            Render2D.FillRectangle(_valueSlider, hueColor, hueColor, Color.Black, Color.Black);
            float valueY = _valueSlider.Y + _valueSlider.Height * (1 - hsv.Z);
            Render2D.DrawRectangle(new Rectangle(_valueSlider.X - 2, valueY - 2,
                _valueSlider.Width + 4, 4), Color.White, 2);

            Render2D.FillRectangle(_alphaSlider, Color.White);
            float checker = _alphaSlider.Width * 0.5f;
            Render2D.PushClip(_alphaSlider);
            for (int x = 0; x < 2; x++)
                for (int y = 0; y < Mathf.CeilToInt(_alphaSlider.Height / checker); y++)
                    if ((x + y) % 2 == 0)
                        Render2D.FillRectangle(new Rectangle(_alphaSlider.X + checker * x,
                            _alphaSlider.Y + checker * y, checker, checker), Color.Gray);
            Render2D.PopClip();
            var opaque = _color;
            opaque.A = 1;
            Render2D.FillRectangle(_alphaSlider, opaque, opaque, Color.Transparent, Color.Transparent);
            float alphaY = _alphaSlider.Y + _alphaSlider.Height * (1 - _color.A);
            Render2D.DrawRectangle(new Rectangle(_alphaSlider.X - 2, alphaY - 2,
                _alphaSlider.Width + 4, 4), Color.White, 2);
        }

        private void UpdateMouse(Float2 location)
        {
            if (_dragMode == 1)
            {
                Float2 delta = location - _wheel.Center;
                float degrees = Mathf.Atan2(-delta.Y, delta.X) * Mathf.RadiansToDegrees;
                if (degrees < 0) degrees += 360;
                Float3 hsv = _color.ToHSV();
                hsv.X = degrees;
                hsv.Y = Mathf.Saturate(delta.Length / (_wheel.Width * 0.5f));
                if ((_color == Color.Black || _color == Color.Transparent) && hsv.Z <= 0.001f)
                    hsv.Z = 1;
                Color = Color.FromHSV(hsv, _color.A);
            }
            else if (_dragMode == 2)
            {
                Float3 hsv = _color.ToHSV();
                hsv.Z = 1 - Mathf.Saturate((location.Y - _valueSlider.Y) / _valueSlider.Height);
                Color = Color.FromHSV(hsv, _color.A);
            }
            else if (_dragMode == 3)
            {
                var value = _color;
                value.A = 1 - Mathf.Saturate((location.Y - _alphaSlider.Y) / _alphaSlider.Height);
                Color = value;
            }
        }

        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            if (button != MouseButton.Left) return base.OnMouseDown(location, button);
            _dragMode = _wheel.Contains(location) ? 1 : _valueSlider.Contains(location) ? 2 :
                _alphaSlider.Contains(location) ? 3 : 0;
            if (_dragMode == 0) return base.OnMouseDown(location, button);
            Focus();
            StartMouseCapture();
            SlidingStart?.Invoke();
            UpdateMouse(location);
            return true;
        }

        public override void OnMouseMove(Float2 location)
        {
            if (_dragMode != 0) UpdateMouse(location);
            base.OnMouseMove(location);
        }

        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Left && _dragMode != 0)
            {
                UpdateMouse(location);
                EndMouseCapture();
                EndSliding();
                return true;
            }
            return base.OnMouseUp(location, button);
        }

        public override void OnLostFocus()
        {
            EndSliding();
            base.OnLostFocus();
        }

        public override void OnEndMouseCapture()
        {
            EndSliding();
            base.OnEndMouseCapture();
        }

        private void EndSliding()
        {
            if (_dragMode == 0) return;
            _dragMode = 0;
            SlidingEnd?.Invoke();
        }
    }
}
