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
            Minimum = min;
            Maximum = Math.Max(Minimum, max);
            SetValue(Value);
        }

        public void SetSpeed(float value)
        {
            SliderSpeed = value;
            TryUseAutoSliderSpeed();
        }

        public void SetLimits(FloatValueBox other)
        {
            Minimum = other.Minimum;
            Maximum = other.Maximum;
            SliderSpeed = other.SliderSpeed;
            SetValue(Value);
        }

        protected override void UpdateText()
        {
            Text = CurrentValue.ToString(CultureInfo.InvariantCulture);
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
            SetValue(StartSlideValue + delta);
        }

        private void TryUseAutoSliderSpeed()
        {
            float range = Maximum - Minimum;
            if (Math.Abs(SliderSpeed - 1.0f) <= float.Epsilon && range > float.Epsilon * 200.0f && range < 1000000.0f)
                SliderSpeed = range * 0.001f;
        }
    }
}
