using System;
using SE.GUI;

namespace SE.Editor.GUI.Dialogs
{
    // The hue/saturation wheel follows Flax's pointer conversion. SolarEngine
    // draws the wheel with a gradient triangle fan because it has no HSWheelMaterial.
    public sealed class ColorSelectorWithSliders : ContainerControl
    {
        private readonly Rectangle m_Wheel;
        private readonly Float2[] m_WheelVertices;
        private readonly Color[] m_WheelColors;
        private readonly Rectangle m_ValueSlider;
        private readonly Rectangle m_AlphaSlider;
        private readonly bool m_UseAlpha;
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

        public ColorSelectorWithSliders(float wheelSize, float sliderThickness, bool useAlpha)
            : base(0, 0, wheelSize + (useAlpha ? 2 * sliderThickness + 25 : sliderThickness + 10), wheelSize)
        {
            m_UseAlpha = useAlpha;
            AutoFocus = true;
            m_Wheel = new Rectangle(0, 0, wheelSize, wheelSize);
            m_ValueSlider = new Rectangle(wheelSize + 10, 0, sliderThickness, wheelSize);
            m_AlphaSlider = new Rectangle(m_ValueSlider.Right + 15, 0, sliderThickness, wheelSize);

            const int wheelSegments = 180;
            m_WheelVertices = new Float2[wheelSegments * 3];
            m_WheelColors = new Color[wheelSegments * 3];
            Float2 center = m_Wheel.Center;
            float radius = m_Wheel.Width * 0.5f;
            Float2 previous = center + new Float2(radius, 0);
            Color previousColor = Color.FromHSV(0, 1, 1);
            for (int i = 0; i < wheelSegments; i++)
            {
                // Wrap the final endpoint to zero so the fan closes at the exact starting position.
                float angle = (i + 1) % wheelSegments * (360.0f / wheelSegments);
                float radians = angle * Mathf.DegreesToRadians;
                Float2 end = center + new Float2(Mathf.Cos(radians), -Mathf.Sin(radians)) * radius;
                Color endColor = Color.FromHSV(angle, 1, 1);
                int vertex = i * 3;
                m_WheelVertices[vertex] = center;
                m_WheelVertices[vertex + 1] = previous;
                m_WheelVertices[vertex + 2] = end;
                m_WheelColors[vertex] = Color.White;
                m_WheelColors[vertex + 1] = previousColor;
                m_WheelColors[vertex + 2] = endColor;
                previous = end;
                previousColor = endColor;
            }
        }

        public override void Draw()
        {
            base.Draw();
            Render2D.FillTriangles(m_WheelVertices, m_WheelColors, true);
            Float2 center = m_Wheel.Center;
            float radius = m_Wheel.Width * 0.5f;
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

            if (m_UseAlpha)
            {
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
                m_UseAlpha && m_AlphaSlider.Contains(location) ? 3 : 0;
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
