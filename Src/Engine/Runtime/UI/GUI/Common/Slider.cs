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
        private float _minimum;
        private float _maximum = 100.0f;
        private float _value = 100.0f;
        private Float2 _thumbSize = new Float2(16.0f, 16.0f);
        private Rectangle _thumbRect;
        private bool _isSliding;
        private bool _mouseOverThumb;

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
            get => _direction;
            set
            {
                if (_direction == value)
                    return;
                _direction = value;
                UpdateThumb();
            }
        }
        private SliderDirection _direction = SliderDirection.HorizontalRight;

        public float Minimum
        {
            get => _minimum;
            set
            {
                if (value > _maximum)
                    throw new ArgumentOutOfRangeException(nameof(value));
                _minimum = WholeNumbers ? MathF.Round(value) : value;
                Value = _value;
            }
        }

        public float Maximum
        {
            get => _maximum;
            set
            {
                if (value < _minimum)
                    throw new ArgumentOutOfRangeException(nameof(value));
                _maximum = WholeNumbers ? MathF.Round(value) : value;
                Value = _value;
            }
        }

        public float Value
        {
            get => _value;
            set
            {
                float result = Math.Clamp(value, Minimum, Maximum);
                if (WholeNumbers)
                    result = MathF.Round(result);
                if (MathF.Abs(result - _value) <= float.Epsilon)
                    return;

                _value = result;
                UpdateThumb();
                ValueChanged?.Invoke(this);
            }
        }

        public bool WholeNumbers { get; set; }
        public bool FillTrack { get; set; } = true;
        public int TrackThickness { get; set; } = 2;
        public Float2 ThumbSize
        {
            get => _thumbSize;
            set
            {
                _thumbSize = new Float2(MathF.Max(0.0f, value.X), MathF.Max(0.0f, value.Y));
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
        public bool IsSliding => _isSliding;
        public Rectangle ThumbBounds => _thumbRect;

        public override bool OnMouseDown(Float2 location, int button)
        {
            if (button != 1)
                return false;

            if (_thumbRect.Contains(location))
            {
                _isSliding = true;
                Root?.StartTrackingMouse(this);
                SlidingStart?.Invoke(this);
                return true;
            }

            SetValueFromLocation(location);
            return true;
        }

        public override void OnMouseMove(Float2 location)
        {
            _mouseOverThumb = _thumbRect.Contains(location);
            if (_isSliding)
                SetValueFromLocation(location);
        }

        public override bool OnMouseUp(Float2 location, int button)
        {
            if (button != 1 || !_isSliding)
                return false;

            SetValueFromLocation(location);
            _isSliding = false;
            Root?.EndTrackingMouse();
            SlidingEnd?.Invoke(this);
            return true;
        }

        public override void ClearState()
        {
            bool wasSliding = _isSliding;
            _isSliding = false;
            _mouseOverThumb = false;
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

            Rectangle thumb = new Rectangle(ScreenPos + _thumbRect.Location, _thumbRect.Size);
            Color color = _isSliding ? ThumbColorSelected : _mouseOverThumb ? ThumbColorHighlighted : ThumbColor;
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
            float thumb = horizontal ? _thumbSize.X : _thumbSize.Y;
            float length = MathF.Max(0.0f, (horizontal ? Width : Height) - thumb);
            if (length <= 0.0f || _maximum <= _minimum)
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
            float range = _maximum - _minimum;
            float normalized = range <= 0.0f ? 0.0f : Math.Clamp((_value - _minimum) / range, 0.0f, 1.0f);
            if (Direction is SliderDirection.HorizontalLeft or SliderDirection.VerticalUp)
                normalized = 1.0f - normalized;

            float thumb = horizontal ? _thumbSize.X : _thumbSize.Y;
            float position = normalized * MathF.Max(0.0f, (horizontal ? Width : Height) - thumb);
            _thumbRect = horizontal
                ? new Rectangle(position, (Height - _thumbSize.Y) * 0.5f, _thumbSize.X, _thumbSize.Y)
                : new Rectangle((Width - _thumbSize.X) * 0.5f, position, _thumbSize.X, _thumbSize.Y);
        }

        private Rectangle GetTrackBounds()
        {
            bool horizontal = Direction is SliderDirection.HorizontalRight or SliderDirection.HorizontalLeft;
            if (horizontal)
                return new Rectangle(ScreenPos.X + _thumbSize.X * 0.5f, ScreenPos.Y + (Height - TrackThickness) * 0.5f, MathF.Max(0.0f, Width - _thumbSize.X), TrackThickness);

            return new Rectangle(ScreenPos.X + (Width - TrackThickness) * 0.5f, ScreenPos.Y + _thumbSize.Y * 0.5f, TrackThickness, MathF.Max(0.0f, Height - _thumbSize.Y));
        }

        private Rectangle GetFillBounds(Rectangle track)
        {
            bool horizontal = Direction is SliderDirection.HorizontalRight or SliderDirection.HorizontalLeft;
            float center = horizontal ? ScreenPos.X + _thumbRect.X + _thumbRect.Width * 0.5f : ScreenPos.Y + _thumbRect.Y + _thumbRect.Height * 0.5f;
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
