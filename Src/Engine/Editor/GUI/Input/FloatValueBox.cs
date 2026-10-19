using System;

namespace SE.Editor.GUI
{
    /// <summary>
    /// Floating point value editor.
    /// </summary>
    /// <seealso cref="float" />
    public class FloatValueBox : ValueBox<float>
    {
        private Utils.ValueCategory m_Category = Utils.ValueCategory.None;
        public override float GetValue()
        {
            return _value;
        }

        public override void SetValue(float value)
        {
            value = _min > _max ? value : Mathf.Clamp(value, _min, _max);
            if (Mathf.Abs(_value - value) > Mathf.Epsilon)
            {
                _value = value;
                UpdateText();
                OnValueChanged();
            }
        }

        public override float GetMinValue()
        {
            return _min;
        }

        public override void SetMinValue(float value)
        {
            if (!(Math.Abs((double)_min - value) <= Mathf.Epsilon))
            {
                SE.Debug.Assert(value <= _max, "Minimum value exceeds maximum value.");
                _min = value;
                SetValue(GetValue());
            }
        }

        public override float GetMaxValue()
        {
            return _max;
        }

        public override void SetMaxValue(float value)
        {
            if (!(Math.Abs((double)_max - value) <= Mathf.Epsilon))
            {
                SE.Debug.Assert(value >= _min, "Maximum value is below minimum value.");
                _max = value;
                SetValue(GetValue());
            }
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="FloatValueBox"/> class.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <param name="x">The x location.</param>
        /// <param name="y">The y location.</param>
        /// <param name="width">The width.</param>
        /// <param name="min">The minimum value.</param>
        /// <param name="max">The maximum value.</param>
        /// <param name="slideSpeed">The slide speed.</param>
        public FloatValueBox(float value, float x = 0, float y = 0, float width = 120,
            float min = float.MinValue, float max = float.MaxValue, float slideSpeed = 1)
            : base(min > max ? value : Mathf.Clamp(value, min, max),
                x, y, width, min, max, slideSpeed)
        {
            TryUseAutoSliderSpeed();
            UpdateText();
        }

        /// <summary>
        /// Sets the value limits.
        /// </summary>
        /// <param name="min">The minimum value (bottom range).</param>
        /// <param name="max">The maximum value (upper range).</param>
        public void SetLimits(float min, float max)
        {
            _min = min;
            _max = _min < max ? max : _min;
            SetValue(GetValue());
        }

        public void SetLimits(LimitAttribute limits)
        {
            _min = limits.Min;
            _max = Mathf.Max(_min, limits.Max);
            _slideSpeed = limits.SliderSpeed;
            TryUseAutoSliderSpeed();
            SetValue(GetValue());
        }

        /// <summary>
        /// Sets the limits from the attribute.
        /// </summary>
        /// <param name="value">The speed.</param>
        public void SetSpeed(float value)
        {
            _slideSpeed = value;
            TryUseAutoSliderSpeed();
        }

        /// <summary>
        /// Sets the limits from the other <see cref="FloatValueBox"/>.
        /// </summary>
        /// <param name="other">The other.</param>
        public void SetLimits(FloatValueBox other)
        {
            _min = other._min;
            _max = other._max;
            _slideSpeed = other._slideSpeed;
            SetValue(GetValue());
        }

        /// <summary>
        /// Gets or sets the category of the value. This can be none for just a number or a more specific one like a
        /// distance.
        /// </summary>
        public Utils.ValueCategory Category
        {
            get => m_Category;
            set
            {
                if (m_Category == value)
                {
                    return;
                }
                m_Category = value;
                UpdateText();
            }
        }

        private void TryUseAutoSliderSpeed()
        {
            float range = _max - _min;
            if (Mathf.IsOne(_slideSpeed) && range > Mathf.Epsilon * 200.0f && range < 1000000.0f)
            {
                _slideSpeed = range * 0.001f;
            }
        }

        /// <inheritdoc />
        protected override void UpdateText()
        {
            SetText(NumberFormatter.FormatFloat(_value, Category));
        }

        /// <inheritdoc />
        protected override void TryGetValue()
        {
            try { SetValue((float)ShuntingYard.Parse(Text)); }
            catch (Exception ex) { SE.Debug.Log(ex.ToString()); UpdateText(); }
        }

        /// <inheritdoc />
        protected override void ApplySliding(float delta)
        {
            SetValue(_startSlideValue + delta);
        }
    }
}
