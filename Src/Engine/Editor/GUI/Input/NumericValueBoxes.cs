using System;
using System.Globalization;

namespace SE.Editor.GUI
{
    /// <summary>
    /// Signed integer value editor.
    /// </summary>
    public sealed class IntValueBox : ValueBox<int>
    {
        public IntValueBox(int value, float x = 0.0f, float y = 0.0f, float width = 120.0f, int min = int.MinValue, int max = int.MaxValue, float slideSpeed = 1.0f)
            : base(value, x, y, width, Math.Min(min, max), Math.Max(min, max), slideSpeed)
        {
            UpdateText();
        }

        public void SetLimits(int min, int max)
        {
            _min = Math.Min(min, max);
            _max = Math.Max(min, max);
            SetValue(_value);
        }

        protected override void UpdateText() => Text = _value.ToString(CultureInfo.InvariantCulture);

        protected override void TryGetValue()
        {
            if (int.TryParse(Text, NumberStyles.Integer, CultureInfo.InvariantCulture, out int value))
                SetValue(value);
            else
                UpdateText();
        }

        protected override void ApplySliding(float delta) => SetValue(SaturatingAdd(_startSlideValue, delta));

        private static int SaturatingAdd(int value, float delta)
        {
            long result = (long)value + (long)MathF.Round(delta);
            return (int)Math.Clamp(result, int.MinValue, int.MaxValue);
        }
    }

    /// <summary>
    /// Double precision value editor.
    /// </summary>
    public sealed class DoubleValueBox : ValueBox<double>
    {
        public DoubleValueBox(double value, float x = 0.0f, float y = 0.0f, float width = 120.0f, double min = double.MinValue, double max = double.MaxValue, float slideSpeed = 1.0f)
            : base(value, x, y, width, Math.Min(min, max), Math.Max(min, max), slideSpeed)
        {
            UpdateText();
        }

        public void SetLimits(double min, double max)
        {
            _min = Math.Min(min, max);
            _max = Math.Max(min, max);
            SetValue(_value);
        }

        protected override void UpdateText() => Text = _value.ToString("G", CultureInfo.InvariantCulture);

        protected override void TryGetValue()
        {
            if (double.TryParse(Text, NumberStyles.Float, CultureInfo.InvariantCulture, out double value))
                SetValue(value);
            else
                UpdateText();
        }

        protected override void ApplySliding(float delta) => SetValue(_startSlideValue + delta);
    }

    /// <summary>
    /// Signed 64-bit integer value editor.
    /// </summary>
    public sealed class LongValueBox : ValueBox<long>
    {
        public LongValueBox(long value, float x = 0.0f, float y = 0.0f, float width = 120.0f, long min = long.MinValue, long max = long.MaxValue, float slideSpeed = 1.0f)
            : base(value, x, y, width, Math.Min(min, max), Math.Max(min, max), slideSpeed)
        {
            UpdateText();
        }

        public void SetLimits(long min, long max)
        {
            _min = Math.Min(min, max);
            _max = Math.Max(min, max);
            SetValue(_value);
        }

        protected override void UpdateText() => Text = _value.ToString(CultureInfo.InvariantCulture);

        protected override void TryGetValue()
        {
            if (long.TryParse(Text, NumberStyles.Integer, CultureInfo.InvariantCulture, out long value))
                SetValue(value);
            else
                UpdateText();
        }

        protected override void ApplySliding(float delta)
        {
            double result = _startSlideValue + MathF.Round(delta);
            SetValue((long)Math.Clamp(result, long.MinValue, long.MaxValue));
        }
    }

    /// <summary>
    /// Unsigned 32-bit integer value editor.
    /// </summary>
    public sealed class UIntValueBox : ValueBox<uint>
    {
        public UIntValueBox(uint value, float x = 0.0f, float y = 0.0f, float width = 120.0f, uint min = uint.MinValue, uint max = uint.MaxValue, float slideSpeed = 1.0f)
            : base(value, x, y, width, Math.Min(min, max), Math.Max(min, max), slideSpeed)
        {
            UpdateText();
        }

        public void SetLimits(uint min, uint max)
        {
            _min = Math.Min(min, max);
            _max = Math.Max(min, max);
            SetValue(_value);
        }

        protected override void UpdateText() => Text = _value.ToString(CultureInfo.InvariantCulture);

        protected override void TryGetValue()
        {
            if (uint.TryParse(Text, NumberStyles.Integer, CultureInfo.InvariantCulture, out uint value))
                SetValue(value);
            else
                UpdateText();
        }

        protected override void ApplySliding(float delta)
        {
            double result = _startSlideValue + MathF.Round(delta);
            SetValue((uint)Math.Clamp(result, uint.MinValue, uint.MaxValue));
        }
    }

    /// <summary>
    /// Unsigned 64-bit integer value editor.
    /// </summary>
    public sealed class ULongValueBox : ValueBox<ulong>
    {
        public ULongValueBox(ulong value, float x = 0.0f, float y = 0.0f, float width = 120.0f, ulong min = ulong.MinValue, ulong max = ulong.MaxValue, float slideSpeed = 1.0f)
            : base(value, x, y, width, Math.Min(min, max), Math.Max(min, max), slideSpeed)
        {
            UpdateText();
        }

        public void SetLimits(ulong min, ulong max)
        {
            _min = Math.Min(min, max);
            _max = Math.Max(min, max);
            SetValue(_value);
        }

        protected override void UpdateText() => Text = _value.ToString(CultureInfo.InvariantCulture);

        protected override void TryGetValue()
        {
            if (ulong.TryParse(Text, NumberStyles.Integer, CultureInfo.InvariantCulture, out ulong value))
                SetValue(value);
            else
                UpdateText();
        }

        protected override void ApplySliding(float delta)
        {
            double result = _startSlideValue + MathF.Round(delta);
            SetValue((ulong)Math.Clamp(result, ulong.MinValue, ulong.MaxValue));
        }
    }
}
