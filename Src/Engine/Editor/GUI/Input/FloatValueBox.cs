using System;
using System.Globalization;

namespace SE.Editor.GUI
{
    public sealed class FloatValueBox : ValueBox<float>
    {
        public FloatValueBox(
            float value,
            float x = 0,
            float y = 0,
            float width = 120,
            float min = float.MinValue,
            float max = float.MaxValue,
            float slideSpeed = 1)
            : base(Math.Clamp(value, min, max), x, y, width, min, max, slideSpeed)
        {
            TryUseAutoSliderSpeed();
            UpdateText();
        }

        public void SetLimits(float min, float max)
        {
            _min = min;
            _max = Math.Max(_min, max);
            SetValue(Value);
        }

        public void SetSpeed(float value)
        {
            _slideSpeed = value;
            TryUseAutoSliderSpeed();
        }

        public void SetLimits(FloatValueBox other)
        {
            _min = other._min;
            _max = other._max;
            _slideSpeed = other._slideSpeed;
            SetValue(Value);
        }

        protected override void UpdateText()
        {
            Text = _value.ToString(CultureInfo.InvariantCulture);
        }

        protected override void TryGetValue()
        {
            if (float.TryParse(Text, NumberStyles.Float, CultureInfo.InvariantCulture, out float value))
                SetValue(value);
            else
                UpdateText();
        }

        protected override void ApplySliding(float delta)
        {
            SetValue(_startSlideValue + delta);
        }

        private void TryUseAutoSliderSpeed()
        {
            float range = _max - _min;
            if (Math.Abs(_slideSpeed - 1.0f) <= float.Epsilon && range > float.Epsilon * 200.0f && range < 1000000.0f)
                _slideSpeed = range * 0.001f;
        }
    }
}
