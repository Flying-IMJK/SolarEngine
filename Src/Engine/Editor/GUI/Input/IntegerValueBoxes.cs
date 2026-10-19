using System;

namespace SE.Editor.GUI
{
    [HideInEditor]
    public class IntValueBox : ValueBox<int>
    {
        public IntValueBox(int value = 0, float x = 0, float y = 0, float width = 120,
            int min = int.MinValue, int max = int.MaxValue, float slideSpeed = 1)
            : base(Mathf.Clamp(value, min, max), x, y, width, min, max, slideSpeed) => UpdateText();

        public override int GetValue() => _value;
        public override void SetValue(int value)
        {
            value = Mathf.Clamp(value, _min, _max);
            if (_value == value)
            {
                return;
            }
            _value = value;
            UpdateText();
            OnValueChanged();
        }
        public override int GetMinValue() => _min;
        public override void SetMinValue(int value)
        {
            if (value > _max)
            {
                throw new ArgumentException();
            }
            _min = value;
            SetValue(_value);
        }
        public override int GetMaxValue() => _max;
        public override void SetMaxValue(int value)
        {
            if (value < _min)
            {
                throw new ArgumentException();
            }
            _max = value;
            SetValue(_value);
        }
        public void SetLimits(int min, int max)
        {
            _min = min;
            _max = Math.Max(min, max);
            SetValue(_value);
        }
        public void SetLimits(LimitAttribute limits)
        {
            _min = limits.Min == float.MinValue ? int.MinValue : (int)limits.Min;
            _max = Math.Max(_min, limits.Max == float.MaxValue ? int.MaxValue : (int)limits.Max);
            _slideSpeed = limits.SliderSpeed;
            SetValue(_value);
        }
        public void SetLimits(RangeAttribute limits)
        {
            _min = limits.Min == float.MinValue ? int.MinValue : (int)limits.Min;
            _max = Math.Max(_min, limits.Max == float.MaxValue ? int.MaxValue : (int)limits.Max);
            SetValue(_value);
        }
        protected override void UpdateText() => SetText(_value.ToString());
        protected override void TryGetValue()
        {
            try { SetValue((int)ShuntingYard.Parse(Text)); }
            catch (Exception ex) { SE.Debug.Log(ex.ToString()); UpdateText(); }
        }
        protected override void ApplySliding(float delta) => SetValue(_startSlideValue + (int)delta);
    }

    [HideInEditor]
    public class LongValueBox : ValueBox<long>
    {
        public LongValueBox(long value = 0, float x = 0, float y = 0, float width = 120,
            long min = long.MinValue, long max = long.MaxValue, float slideSpeed = 1)
            : base(Mathf.Clamp(value, min, max), x, y, width, min, max, slideSpeed) => UpdateText();

        public override long GetValue() => _value;
        public override void SetValue(long value)
        {
            value = Mathf.Clamp(value, _min, _max);
            if (_value == value)
            {
                return;
            }
            _value = value;
            UpdateText();
            OnValueChanged();
        }
        public override long GetMinValue() => _min;
        public override void SetMinValue(long value)
        {
            if (value > _max)
            {
                throw new ArgumentException();
            }
            _min = value;
            SetValue(_value);
        }
        public override long GetMaxValue() => _max;
        public override void SetMaxValue(long value)
        {
            if (value < _min)
            {
                throw new ArgumentException();
            }
            _max = value;
            SetValue(_value);
        }
        public void SetLimits(long min, long max)
        {
            _min = Math.Min(min, max);
            _max = Math.Max(min, max);
            SetValue(_value);
        }
        public void SetLimits(LimitAttribute limits)
        {
            _min = limits.Min == int.MinValue ? long.MinValue : (long)limits.Min;
            _max = Math.Max(_min, limits.Max == int.MaxValue ? long.MaxValue : (long)limits.Max);
            _slideSpeed = limits.SliderSpeed;
            SetValue(_value);
        }
        protected override void UpdateText() => SetText(_value.ToString());
        protected override void TryGetValue()
        {
            if (long.TryParse(Text, out long value))
            {
                SetValue(value);
            }
            else
            {
                UpdateText();
            }
        }
        protected override void ApplySliding(float delta) => SetValue(_startSlideValue + (long)delta);
    }

    [HideInEditor]
    public class ULongValueBox : ValueBox<ulong>
    {
        public ULongValueBox(ulong value = 0, float x = 0, float y = 0, float width = 120,
            ulong min = ulong.MinValue, ulong max = ulong.MaxValue, float slideSpeed = 1)
            : base(Mathf.Clamp(value, min, max), x, y, width, min, max, slideSpeed) => UpdateText();

        public override ulong GetValue() => _value;
        public override void SetValue(ulong value)
        {
            value = Mathf.Clamp(value, _min, _max);
            if (_value == value)
            {
                return;
            }
            _value = value;
            UpdateText();
            OnValueChanged();
        }
        public override ulong GetMinValue() => _min;
        public override void SetMinValue(ulong value)
        {
            if (value > _max)
            {
                throw new ArgumentException();
            }
            _min = value;
            SetValue(_value);
        }
        public override ulong GetMaxValue() => _max;
        public override void SetMaxValue(ulong value)
        {
            if (value < _min)
            {
                throw new ArgumentException();
            }
            _max = value;
            SetValue(_value);
        }
        public void SetLimits(ulong min, ulong max)
        {
            _min = Math.Min(min, max);
            _max = Math.Max(min, max);
            SetValue(_value);
        }
        public void SetLimits(LimitAttribute limits)
        {
            _min = limits.Min < 0.0f ? 0 : (ulong)limits.Min;
            _max = Math.Max(_min, limits.Max == float.MaxValue ? ulong.MaxValue : (ulong)limits.Max);
            _slideSpeed = limits.SliderSpeed;
            SetValue(_value);
        }
        protected override void UpdateText() => SetText(_value.ToString());
        protected override void TryGetValue()
        {
            try { SetValue((ulong)ShuntingYard.Parse(Text)); }
            catch (Exception ex) { SE.Debug.Log(ex.ToString()); UpdateText(); }
        }
        protected override void ApplySliding(float delta)
        {
            if (delta < 0 && _startSlideValue > delta)
            {
                SetValue((ulong)Mathf.Max(0, (long)_startSlideValue + (long)delta));
            }
            else if (delta < 0)
            {
                SetValue(_startSlideValue - (ulong)(-delta));
            }
            else
            {
                SetValue(_startSlideValue + (ulong)delta);
            }
        }
    }
}
