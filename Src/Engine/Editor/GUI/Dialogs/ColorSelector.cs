using System;
using SE.GUI;

namespace SE.Editor.GUI.Dialogs
{
    // The hue/saturation wheel follows Flax's pointer conversion. SolarEngine
    // draws the wheel with gradient spokes because it has no HSWheelMaterial.
    public sealed class ColorSelectorWithSliders : ContainerControl
    {
        private readonly Rectangle m_Wheel;
        private readonly Rectangle m_ValueSlider;
        private readonly Rectangle m_AlphaSlider;
        private Color m_Color;
        private int m_DragMode;

        public event Action<Color> ColorChanged;
        public event Action SlidingStart;
        public event Action SlidingEnd;
        public Color Color
        {
            get => m_Color;
            set
            {
                if (m_Color == value)
                {
                    return;
                }
                m_Color = value;
                ColorChanged?.Invoke(value);
            }
        }
        public bool IsSliding => m_DragMode != 0;

        public ColorSelectorWithSliders(float wheelSize, float sliderThickness)
            : base(0, 0, wheelSize + 2 * sliderThickness + 25, wheelSize)
        {
            AutoFocus = true;
            m_Wheel = new Rectangle(0, 0, wheelSize, wheelSize);
            m_ValueSlider = new Rectangle(wheelSize + 10, 0, sliderThickness, wheelSize);
            m_AlphaSlider = new Rectangle(m_ValueSlider.Right + 15, 0, sliderThickness, wheelSize);
        }

        public override void Draw()
        {
            base.Draw();
            Float2 center = m_Wheel.Center;
            float radius = m_Wheel.Width * 0.5f;
            for (int angle = 0; angle < 360; angle += 2)
            {
                float radians = angle * Mathf.DegreesToRadians;
                Float2 end = center + new Float2(Mathf.Cos(radians), -Mathf.Sin(radians)) * radius;
                Render2D.DrawLine(center, end, Color.White, Color.FromHSV(angle, 1, 1), 2.5f);
            }
            Float3 hsv = m_Color.ToHSV();
            float pointerAngle = hsv.X * Mathf.DegreesToRadians;
            Float2 pointer = center + new Float2(Mathf.Cos(pointerAngle), -Mathf.Sin(pointerAngle)) *
                (hsv.Y * radius);
            Rectangle pointerRect = new Rectangle(pointer - new Float2(4), new Float2(8));
            Render2D.FillRectangle(pointerRect, Color.FromHSV(hsv.X, hsv.Y, 1));
            Render2D.DrawRectangle(pointerRect, Color.Black);

            Color hueColor = Color.FromHSV(hsv.X, hsv.Y, 1);
            Render2D.FillRectangle(m_ValueSlider, hueColor, hueColor, Color.Black, Color.Black);
            float valueY = m_ValueSlider.Y + m_ValueSlider.Height * (1 - hsv.Z);
            Render2D.DrawRectangle(new Rectangle(m_ValueSlider.X - 2, valueY - 2,
                m_ValueSlider.Width + 4, 4), Color.White, 2);

            Render2D.FillRectangle(m_AlphaSlider, Color.White);
            float checker = m_AlphaSlider.Width * 0.5f;
            Render2D.PushClip(m_AlphaSlider);
            for (int x = 0; x < 2; x++)
            {
                for (int y = 0; y < Mathf.CeilToInt(m_AlphaSlider.Height / checker); y++)
                {
                    if ((x + y) % 2 == 0)
                    {
                        Render2D.FillRectangle(new Rectangle(m_AlphaSlider.X + checker * x,
                                                    m_AlphaSlider.Y + checker * y, checker, checker), Color.Gray);
                    }
                }
            }
            Render2D.PopClip();
            Color opaque = m_Color;
            opaque.A = 1;
            Render2D.FillRectangle(m_AlphaSlider, opaque, opaque, Color.Transparent, Color.Transparent);
            float alphaY = m_AlphaSlider.Y + m_AlphaSlider.Height * (1 - m_Color.A);
            Render2D.DrawRectangle(new Rectangle(m_AlphaSlider.X - 2, alphaY - 2,
                m_AlphaSlider.Width + 4, 4), Color.White, 2);
        }

        private void UpdateMouse(Float2 location)
        {
            if (m_DragMode == 1)
            {
                Float2 delta = location - m_Wheel.Center;
                float degrees = Mathf.Atan2(-delta.Y, delta.X) * Mathf.RadiansToDegrees;
                if (degrees < 0)
                {
                    degrees += 360;
                }
                Float3 hsv = m_Color.ToHSV();
                hsv.X = degrees;
                hsv.Y = Mathf.Saturate(delta.Length / (m_Wheel.Width * 0.5f));
                if ((m_Color == Color.Black || m_Color == Color.Transparent) && hsv.Z <= 0.001f)
                {
                    hsv.Z = 1;
                }
                Color = Color.FromHSV(hsv, m_Color.A);
            }
            else if (m_DragMode == 2)
            {
                Float3 hsv = m_Color.ToHSV();
                hsv.Z = 1 - Mathf.Saturate((location.Y - m_ValueSlider.Y) / m_ValueSlider.Height);
                Color = Color.FromHSV(hsv, m_Color.A);
            }
            else if (m_DragMode == 3)
            {
                Color value = m_Color;
                value.A = 1 - Mathf.Saturate((location.Y - m_AlphaSlider.Y) / m_AlphaSlider.Height);
                Color = value;
            }
        }

        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            if (button != MouseButton.Left)
            {
                return base.OnMouseDown(location, button);
            }
            m_DragMode = m_Wheel.Contains(location) ? 1 : m_ValueSlider.Contains(location) ? 2 :
                m_AlphaSlider.Contains(location) ? 3 : 0;
            if (m_DragMode == 0)
            {
                return base.OnMouseDown(location, button);
            }
            Focus();
            StartMouseCapture();
            SlidingStart?.Invoke();
            UpdateMouse(location);
            return true;
        }

        public override void OnMouseMove(Float2 location)
        {
            if (m_DragMode != 0)
            {
                UpdateMouse(location);
            }
            base.OnMouseMove(location);
        }

        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Left && m_DragMode != 0)
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
            if (m_DragMode == 0)
            {
                return;
            }
            m_DragMode = 0;
            SlidingEnd?.Invoke();
        }
    }
}
