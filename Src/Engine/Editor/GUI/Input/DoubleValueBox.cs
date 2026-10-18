using System;

namespace SE.Editor.GUI
{
    [HideInEditor]
    public class DoubleValueBox : ValueBox<double>
    {
        private Utils.ValueCategory _category = Utils.ValueCategory.None;
        public Utils.ValueCategory Category
        {
            get => _category;
            set
            {
                if (_category == value) return;
                _category = value;
                UpdateText();
            }
        }
        public DoubleValueBox(double value = 0, float x = 0, float y = 0, float width = 120,
            double min = double.MinValue, double max = double.MaxValue, float slideSpeed = 1)
            : base(Mathd.Clamp(value, min, max), x, y, width, min, max, slideSpeed) => UpdateText();

        public override double GetValue() => _value;
        public override void SetValue(double value)
        {
            value = Mathd.Clamp(value, _min, _max);
            if (Math.Abs(_value - value) <= Mathd.Epsilon) return;
            _value = value;
            UpdateText();
            OnValueChanged();
        }
        public override double GetMinValue() => _min;
        public override void SetMinValue(double value)
        {
            if (value > _max) throw new ArgumentException();
            _min = value;
            SetValue(_value);
        }
        public override double GetMaxValue() => _max;
        public override void SetMaxValue(double value)
        {
            if (value < _min) throw new ArgumentException();
            _max = value;
            SetValue(_value);
        }
        public void SetLimits(double min, double max)
        {
            _min = min;
            _max = Math.Max(min, max);
            SetValue(_value);
        }
        public void SetLimits(LimitAttribute limits)
        {
            _min = limits.Min;
            _max = Math.Max(_min, (double)limits.Max);
            _slideSpeed = limits.SliderSpeed;
            SetValue(_value);
        }
        protected override void UpdateText() => SetText(NumberFormatter.FormatFloat(_value, Category));
        protected override void TryGetValue()
        {
            try { SetValue(ShuntingYard.Parse(Text)); }
            catch (Exception ex) { SE.Debug.Log(ex.ToString()); UpdateText(); }
        }
        protected override void ApplySliding(float delta) => SetValue(_startSlideValue + delta);
    }
}
