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
        private bool m_Updating;
        private float m_Minimum;
        private float m_Maximum;
        private float m_Value;

        public SliderControl(float value, float x = 0.0f, float y = 0.0f, float width = 120.0f, float min = 0.0f, float max = 100.0f)
            : base(new Rectangle(x, y, width, 18.0f))
        {
            m_Minimum = Math.Min(min, max);
            m_Maximum = Math.Max(min, max);
            Slider = new Slider(MathF.Max(0.0f, width - TextBoxWidth), Height);
            Editor = new FloatValueBox(value, 0.0f, 0.0f, TextBoxWidth, m_Minimum, m_Maximum, 0.0f);
            AddChild(Slider);
            AddChild(Editor);
            Slider.Minimum = 0.0f;
            Slider.Maximum = 1.0f;
            Slider.ValueChanged += _ => OnSliderValueChanged();
            Slider.SlidingStart += _ => SlidingStart?.Invoke(this);
            Slider.SlidingEnd += _ => SlidingEnd?.Invoke(this);
            Editor.BoxValueChanged += _ => OnEditorValueChanged();
            m_Value = float.NaN;
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
            get => m_Minimum;
            set
            {
                m_Minimum = Math.Min(value, m_Maximum);
                Editor.SetLimits(m_Minimum, m_Maximum);
                Value = m_Value;
            }
        }

        public float Maximum
        {
            get => m_Maximum;
            set
            {
                m_Maximum = Math.Max(m_Minimum, value);
                Editor.SetLimits(m_Minimum, m_Maximum);
                Value = m_Value;
            }
        }

        public float Value
        {
            get => m_Value;
            set
            {
                float clamped = Math.Clamp(value, m_Minimum, m_Maximum);
                if (MathF.Abs(clamped - m_Value) <= float.Epsilon)
                    return;

                m_Value = clamped;
                m_Updating = true;
                Editor.Value = clamped;
                Slider.Value = m_Maximum <= m_Minimum ? 0.0f : (clamped - m_Minimum) / (m_Maximum - m_Minimum);
                m_Updating = false;
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
            if (!m_Updating)
                Value = m_Minimum + Slider.Value * (m_Maximum - m_Minimum);
        }

        private void OnEditorValueChanged()
        {
            if (!m_Updating)
                Value = Editor.Value;
        }
    }
}
