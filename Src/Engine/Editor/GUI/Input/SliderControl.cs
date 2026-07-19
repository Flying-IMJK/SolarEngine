using System;
using System.Globalization;
using SE.GUI;

namespace SE.Editor.GUI
{
    /// <summary>
    /// Combines a normalized slider and numeric text editor into one float control.
    /// </summary>
    public sealed class SliderControl : ContainerControl
    {
        private const float TextBoxWidth = 48.0f;
        private bool _updating;
        private float _minimum;
        private float _maximum;
        private float _value;

        public SliderControl(float value, float x = 0.0f, float y = 0.0f, float width = 120.0f, float min = 0.0f, float max = 100.0f)
            : base(new Rectangle(x, y, width, 18.0f))
        {
            _minimum = Math.Min(min, max);
            _maximum = Math.Max(min, max);
            Slider = new Slider(MathF.Max(0.0f, width - TextBoxWidth), Height);
            Editor = new FloatValueBox(value, 0.0f, 0.0f, TextBoxWidth, _minimum, _maximum, 0.0f);
            AddChild(Slider);
            AddChild(Editor);
            Slider.Minimum = 0.0f;
            Slider.Maximum = 1.0f;
            Slider.ValueChanged += _ => OnSliderValueChanged();
            Slider.SlidingStart += _ => SlidingStart?.Invoke(this);
            Slider.SlidingEnd += _ => SlidingEnd?.Invoke(this);
            Editor.BoxValueChanged += _ => OnEditorValueChanged();
            _value = float.NaN;
            Value = value;
        }

        public event Action<SliderControl>? ValueChanged;
        public event Action<SliderControl>? SlidingStart;
        public event Action<SliderControl>? SlidingEnd;

        public Slider Slider { get; }
        public FloatValueBox Editor { get; }
        public bool IsSliding => Slider.IsSliding;

        public float Minimum
        {
            get => _minimum;
            set
            {
                _minimum = Math.Min(value, _maximum);
                Editor.SetLimits(_minimum, _maximum);
                Value = _value;
            }
        }

        public float Maximum
        {
            get => _maximum;
            set
            {
                _maximum = Math.Max(_minimum, value);
                Editor.SetLimits(_minimum, _maximum);
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
                _updating = true;
                Editor.Value = clamped;
                Slider.Value = _maximum <= _minimum ? 0.0f : (clamped - _minimum) / (_maximum - _minimum);
                _updating = false;
                ValueChanged?.Invoke(this);
            }
        }

        protected override void OnLayoutChildren()
        {
            float editorWidth = MathF.Min(TextBoxWidth, Width);
            Slider.SetBounds(0.0f, 0.0f, MathF.Max(0.0f, Width - editorWidth), Height);
            Editor.SetBounds(Width - editorWidth, 0.0f, editorWidth, Height);
        }

        private void OnSliderValueChanged()
        {
            if (!_updating)
                Value = _minimum + Slider.Value * (_maximum - _minimum);
        }

        private void OnEditorValueChanged()
        {
            if (!_updating)
                Value = Editor.Value;
        }
    }
}
