using System;

namespace SE.GUI
{
    /// <summary>
    /// Determines how the progress region is drawn.
    /// </summary>
    public enum ProgressBarMethod
    {
        Stretch,
        Clip,
    }

    /// <summary>
    /// Determines which edge a progress region grows from.
    /// </summary>
    public enum ProgressBarOrigin
    {
        HorizontalLeft,
        HorizontalRight,
        VerticalTop,
        VerticalBottom,
    }

    /// <summary>
    /// Displays a normalized range value as a progress region.
    /// </summary>
    public class ProgressBar : ContainerControl
    {
        private float _minimum;
        private float _maximum = 100.0f;
        private float _value;
        private float _current;

        public ProgressBar()
            : this(0.0f, 0.0f, 120.0f)
        {
        }

        public ProgressBar(float x, float y, float width, float height = 28.0f)
            : base(new Rectangle(x, y, width, height))
        {
            AutoFocus = false;
            BackgroundColor = Style.Current.Background;
            BarColor = Style.Current.BackgroundHighlighted;
            BarMargin = new Margin(1.0f);
        }

        public float SmoothingScale { get; set; } = 1.0f;
        public bool UseSmoothing => SmoothingScale > 0.0f;
        public ProgressBarMethod Method { get; set; } = ProgressBarMethod.Stretch;
        public ProgressBarOrigin Origin { get; set; } = ProgressBarOrigin.HorizontalLeft;
        public Margin BarMargin { get; set; }
        public Color BarColor { get; set; }
        public IBrush? BarBrush { get; set; }

        public float Minimum
        {
            get => _minimum;
            set
            {
                if (value > _maximum)
                    throw new ArgumentOutOfRangeException(nameof(value));
                _minimum = value;
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
                _maximum = value;
                Value = _value;
            }
        }

        public float Value
        {
            get => _value;
            set
            {
                float clamped = Math.Clamp(value, _minimum, _maximum);
                if (MathF.Abs(clamped - _value) <= float.Epsilon)
                    return;

                _value = clamped;
                if (!UseSmoothing)
                    _current = clamped;
            }
        }

        protected override void OnUpdate(float deltaTime)
        {
            if (!UseSmoothing || deltaTime >= 1.0f / 20.0f)
            {
                _current = _value;
                return;
            }

            float factor = Math.Clamp(deltaTime * 5.0f * SmoothingScale, 0.0f, 1.0f);
            _current += (_value - _current) * factor;
            if (MathF.Abs(_value - _current) <= 0.01f)
                _current = _value;
        }

        protected override void OnDraw()
        {
            base.OnDraw();
            if (_maximum <= _minimum)
                return;

            float normalized = Math.Clamp((_current - _minimum) / (_maximum - _minimum), 0.0f, 1.0f);
            if (normalized <= 0.0f)
                return;

            Rectangle bar = GetProgressBounds(normalized);
            if (Method == ProgressBarMethod.Stretch)
            {
                BarMargin.ShrinkRectangle(ref bar);
                DrawBar(bar);
                return;
            }

            Rectangle full = ScreenBounds;
            BarMargin.ShrinkRectangle(ref full);
            Render2D.PushClip(ref bar);
            DrawBar(full);
            Render2D.PopClip();
        }

        private Rectangle GetProgressBounds(float normalized)
        {
            Rectangle bounds = ScreenBounds;
            return Origin switch
            {
                ProgressBarOrigin.HorizontalRight => new Rectangle(bounds.X + bounds.Width * (1.0f - normalized), bounds.Y, bounds.Width * normalized, bounds.Height),
                ProgressBarOrigin.VerticalTop => new Rectangle(bounds.X, bounds.Y, bounds.Width, bounds.Height * normalized),
                ProgressBarOrigin.VerticalBottom => new Rectangle(bounds.X, bounds.Y + bounds.Height * (1.0f - normalized), bounds.Width, bounds.Height * normalized),
                _ => new Rectangle(bounds.X, bounds.Y, bounds.Width * normalized, bounds.Height),
            };
        }

        private void DrawBar(Rectangle bounds)
        {
            if (bounds.Width <= 0.0f || bounds.Height <= 0.0f)
                return;

            Color color = BarColor;
            if (BarBrush != null)
                BarBrush.Draw(bounds, color);
            else
                Render2D.FillRectangle(ref bounds, ref color);
        }
    }
}
