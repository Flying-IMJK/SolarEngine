using System;

namespace SE.GUI
{
    /// <summary>
    /// The direction in which a slider value increases.
    /// </summary>
    public enum SliderDirection
    {
        HorizontalRight,
        HorizontalLeft,
        VerticalUp,
        VerticalDown,
    }

    /// <summary>
    /// A range control with a draggable thumb.
    /// </summary>
    public class Slider : ContainerControl
    {
        private float m_Minimum;
        private float m_Maximum = 100.0f;
        private float m_Value = 100.0f;
        private Float2 m_ThumbSize = new Float2(16.0f, 16.0f);
        private Rectangle m_ThumbRect;
        private bool m_IsSliding;
        private bool m_MouseOverThumb;

        public Slider()
            : this(120.0f, 30.0f)
        {
        }

        public Slider(float width, float height)
            : base(new Rectangle(0.0f, 0.0f, width, height))
        {
            AutoFocus = false;
            TrackLineColor = Style.Current.BackgroundHighlighted;
            TrackFillLineColor = Style.Current.Foreground;
            ThumbColor = Style.Current.BackgroundNormal;
            ThumbColorHighlighted = Style.Current.BackgroundHighlighted;
            ThumbColorSelected = Style.Current.BackgroundSelected;
            UpdateThumb();
        }

        public event Action<Slider>? SlidingStart;
        public event Action<Slider>? SlidingEnd;
        public event Action<Slider>? ValueChanged;

        public SliderDirection Direction
        {
            get => m_Direction;
            set
            {
                if (m_Direction == value)
                    return;
                m_Direction = value;
                UpdateThumb();
            }
        }
        private SliderDirection m_Direction = SliderDirection.HorizontalRight;

        public float Minimum
        {
            get => m_Minimum;
            set
            {
                if (value > m_Maximum)
                    throw new ArgumentOutOfRangeException(nameof(value));
                m_Minimum = WholeNumbers ? MathF.Round(value) : value;
                Value = m_Value;
            }
        }

        public float Maximum
        {
            get => m_Maximum;
            set
            {
                if (value < m_Minimum)
                    throw new ArgumentOutOfRangeException(nameof(value));
                m_Maximum = WholeNumbers ? MathF.Round(value) : value;
                Value = m_Value;
            }
        }

        public float Value
        {
            get => m_Value;
            set
            {
                float result = Math.Clamp(value, Minimum, Maximum);
                if (WholeNumbers)
                    result = MathF.Round(result);
                if (MathF.Abs(result - m_Value) <= float.Epsilon)
                    return;

                m_Value = result;
                UpdateThumb();
                ValueChanged?.Invoke(this);
            }
        }

        public bool WholeNumbers { get; set; }
        public bool FillTrack { get; set; } = true;
        public int TrackThickness { get; set; } = 2;
        public Float2 ThumbSize
        {
            get => m_ThumbSize;
            set
            {
                m_ThumbSize = new Float2(MathF.Max(0.0f, value.X), MathF.Max(0.0f, value.Y));
                UpdateThumb();
            }
        }

        public Color TrackLineColor { get; set; }
        public Color TrackFillLineColor { get; set; }
        public Color ThumbColor { get; set; }
        public Color ThumbColorHighlighted { get; set; }
        public Color ThumbColorSelected { get; set; }
        public IBrush? TrackBrush { get; set; }
        public IBrush? FillTrackBrush { get; set; }
        public IBrush? ThumbBrush { get; set; }
        public bool IsSliding => m_IsSliding;
        public Rectangle ThumbBounds => m_ThumbRect;

        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            if (button != MouseButton.Left)
                return false;

            if (m_ThumbRect.Contains(location))
            {
                m_IsSliding = true;
                Root?.StartTrackingMouse(this);
                SlidingStart?.Invoke(this);
                return true;
            }

            SetValueFromLocation(location);
            return true;
        }

        public override void OnMouseMove(Float2 location)
        {
            m_MouseOverThumb = m_ThumbRect.Contains(location);
            if (m_IsSliding)
                SetValueFromLocation(location);
        }

        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (button != MouseButton.Left || !m_IsSliding)
                return false;

            SetValueFromLocation(location);
            m_IsSliding = false;
            Root?.EndTrackingMouse();
            SlidingEnd?.Invoke(this);
            return true;
        }

        public override void ClearState()
        {
            bool wasSliding = m_IsSliding;
            m_IsSliding = false;
            m_MouseOverThumb = false;
            if (wasSliding)
                SlidingEnd?.Invoke(this);
            base.ClearState();
        }

        protected override void OnDraw()
        {
            base.OnDraw();

            Rectangle track = GetTrackBounds();
            DrawBrush(TrackBrush, track, TrackLineColor);
            if (FillTrack)
                DrawBrush(FillTrackBrush, GetFillBounds(track), TrackFillLineColor);

            Rectangle thumb = new Rectangle(ScreenPos + m_ThumbRect.Location, m_ThumbRect.Size);
            Color color = m_IsSliding ? ThumbColorSelected : m_MouseOverThumb ? ThumbColorHighlighted : ThumbColor;
            DrawBrush(ThumbBrush, thumb, color);
        }

        protected override void OnBoundsChanged(bool locationChanged, bool sizeChanged)
        {
            base.OnBoundsChanged(locationChanged, sizeChanged);
            if (sizeChanged)
                UpdateThumb();
        }

        private void SetValueFromLocation(Float2 location)
        {
            bool horizontal = Direction is SliderDirection.HorizontalRight or SliderDirection.HorizontalLeft;
            float thumb = horizontal ? m_ThumbSize.X : m_ThumbSize.Y;
            float length = MathF.Max(0.0f, (horizontal ? Width : Height) - thumb);
            if (length <= 0.0f || m_Maximum <= m_Minimum)
                return;

            float coordinate = (horizontal ? location.X : location.Y) - thumb * 0.5f;
            float normalized = Math.Clamp(coordinate / length, 0.0f, 1.0f);
            if (Direction is SliderDirection.HorizontalLeft or SliderDirection.VerticalUp)
                normalized = 1.0f - normalized;
            Value = Minimum + (Maximum - Minimum) * normalized;
        }

        /// <summary>
        /// Recalculates the thumb rectangle after range or size changes.
        /// </summary>
        protected void UpdateThumb()
        {
            bool horizontal = Direction is SliderDirection.HorizontalRight or SliderDirection.HorizontalLeft;
            float range = m_Maximum - m_Minimum;
            float normalized = range <= 0.0f ? 0.0f : Math.Clamp((m_Value - m_Minimum) / range, 0.0f, 1.0f);
            if (Direction is SliderDirection.HorizontalLeft or SliderDirection.VerticalUp)
                normalized = 1.0f - normalized;

            float thumb = horizontal ? m_ThumbSize.X : m_ThumbSize.Y;
            float position = normalized * MathF.Max(0.0f, (horizontal ? Width : Height) - thumb);
            m_ThumbRect = horizontal
                ? new Rectangle(position, (Height - m_ThumbSize.Y) * 0.5f, m_ThumbSize.X, m_ThumbSize.Y)
                : new Rectangle((Width - m_ThumbSize.X) * 0.5f, position, m_ThumbSize.X, m_ThumbSize.Y);
        }

        private Rectangle GetTrackBounds()
        {
            bool horizontal = Direction is SliderDirection.HorizontalRight or SliderDirection.HorizontalLeft;
            if (horizontal)
                return new Rectangle(ScreenPos.X + m_ThumbSize.X * 0.5f, ScreenPos.Y + (Height - TrackThickness) * 0.5f, MathF.Max(0.0f, Width - m_ThumbSize.X), TrackThickness);

            return new Rectangle(ScreenPos.X + (Width - TrackThickness) * 0.5f, ScreenPos.Y + m_ThumbSize.Y * 0.5f, TrackThickness, MathF.Max(0.0f, Height - m_ThumbSize.Y));
        }

        private Rectangle GetFillBounds(Rectangle track)
        {
            bool horizontal = Direction is SliderDirection.HorizontalRight or SliderDirection.HorizontalLeft;
            float center = horizontal ? ScreenPos.X + m_ThumbRect.X + m_ThumbRect.Width * 0.5f : ScreenPos.Y + m_ThumbRect.Y + m_ThumbRect.Height * 0.5f;
            if (Direction is SliderDirection.HorizontalLeft or SliderDirection.VerticalUp)
            {
                if (horizontal)
                    return new Rectangle(center, track.Y, MathF.Max(0.0f, track.Right - center), track.Height);
                return new Rectangle(track.X, center, track.Width, MathF.Max(0.0f, track.Bottom - center));
            }

            if (horizontal)
                return new Rectangle(track.X, track.Y, MathF.Max(0.0f, center - track.X), track.Height);
            return new Rectangle(track.X, track.Y, track.Width, MathF.Max(0.0f, center - track.Y));
        }

        private static void DrawBrush(IBrush? brush, Rectangle bounds, Color color)
        {
            if (bounds.Width <= 0.0f || bounds.Height <= 0.0f)
                return;

            if (brush != null)
                brush.Draw(bounds, color);
            else
                Render2D.FillRectangle(ref bounds, ref color);
        }
    }
}
