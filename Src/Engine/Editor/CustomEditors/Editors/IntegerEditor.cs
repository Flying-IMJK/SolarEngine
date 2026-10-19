using System;
using System.Linq;
using SE.Editor.GUI;

namespace SE.Editor
{
    [CustomEditor(typeof(int)), DefaultEditor]
    public sealed class IntegerEditor : CustomEditor
    {
        private IntValueBox m_Box;
        private SliderControl m_Slider;

        public override DisplayStyle Style => DisplayStyle.Inline;

        public override void Initialize(LayoutElementsContainer layout)
        {
            object[] attributes = Values.GetAttributes();
            if (attributes.OfType<RangeAttribute>().FirstOrDefault() is { } range)
            {
                m_Slider = new SliderControl(0) { Parent = layout.ContainerControl };
                m_Slider.SetLimits(range);
                m_Slider.ValueChanged += OnValueChanged;
                m_Slider.SlidingEnd += ClearToken;
                return;
            }
            m_Box = new IntValueBox { Parent = layout.ContainerControl };
            if (attributes.OfType<LimitAttribute>().FirstOrDefault() is { } limit)
            {
                m_Box.SetLimits(limit);
            }
            m_Box.ValueChanged += OnValueChanged;
            m_Box.SlidingEnd += ClearToken;
        }

        private void OnValueChanged()
        {
            bool sliding = m_Slider?.IsSliding ?? m_Box!.IsSliding;
            int value = m_Slider != null ? (int)m_Slider.Value : m_Box!.GetValue();
            SetValue(value, sliding ? this : null);
        }

        public override void Refresh()
        {
            base.Refresh();
            if (HasDifferentValues)
            {
                return;
            }
            int value = Convert.ToInt32(Values[0]);
            if (m_Slider != null)
            {
                m_Slider.Value = value;
            }
            else
            {
                m_Box!.SetValue(value);
            }
        }
    }

    public abstract class SignedIntegerValueEditor : CustomEditor
    {
        private LongValueBox m_Box = null!;

        public override DisplayStyle Style => DisplayStyle.Inline;

        public override void Initialize(LayoutElementsContainer layout)
        {
            GetLimits(out long min, out long max);
            m_Box = new LongValueBox { Parent = layout.ContainerControl };
            if (Values.GetAttributes().OfType<LimitAttribute>().FirstOrDefault() is { } limit)
            {
                m_Box.SetLimits(limit);
            }
            m_Box.SetLimits(Math.Max(m_Box.GetMinValue(), min), Math.Min(m_Box.GetMaxValue(), max));
            m_Box.ValueChanged += () => SetValue(ConvertValue(m_Box.GetValue()), m_Box.IsSliding ? this : null);
            m_Box.SlidingEnd += ClearToken;
        }

        public override void Refresh()
        {
            base.Refresh();
            if (!HasDifferentValues)
            {
                m_Box.SetValue(GetValue(Values[0]!));
            }
        }

        protected abstract void GetLimits(out long min, out long max);
        protected abstract long GetValue(object value);
        protected abstract object ConvertValue(long value);
    }

    [CustomEditor(typeof(sbyte)), DefaultEditor]
    public sealed class SByteEditor : SignedIntegerValueEditor
    {
        protected override void GetLimits(out long min, out long max) { min = sbyte.MinValue; max = sbyte.MaxValue; }
        protected override long GetValue(object value) => (sbyte)value;
        protected override object ConvertValue(long value) => (sbyte)value;
    }

    [CustomEditor(typeof(short)), DefaultEditor]
    public sealed class ShortEditor : SignedIntegerValueEditor
    {
        protected override void GetLimits(out long min, out long max) { min = short.MinValue; max = short.MaxValue; }
        protected override long GetValue(object value) => (short)value;
        protected override object ConvertValue(long value) => (short)value;
    }

    [CustomEditor(typeof(long)), DefaultEditor]
    public sealed class LongEditor : SignedIntegerValueEditor
    {
        protected override void GetLimits(out long min, out long max) { min = long.MinValue; max = long.MaxValue; }
        protected override long GetValue(object value) => (long)value;
        protected override object ConvertValue(long value) => value;
    }

    public abstract class UnsignedIntegerValueEditor : CustomEditor
    {
        private ULongValueBox m_Box = null!;

        public override DisplayStyle Style => DisplayStyle.Inline;

        public override void Initialize(LayoutElementsContainer layout)
        {
            GetLimits(out ulong min, out ulong max);
            m_Box = new ULongValueBox { Parent = layout.ContainerControl };
            if (Values.GetAttributes().OfType<LimitAttribute>().FirstOrDefault() is { } limit)
            {
                m_Box.SetLimits(limit);
            }
            m_Box.SetLimits(Math.Max(m_Box.GetMinValue(), min), Math.Min(m_Box.GetMaxValue(), max));
            m_Box.ValueChanged += () => SetValue(ConvertValue(m_Box.GetValue()), m_Box.IsSliding ? this : null);
            m_Box.SlidingEnd += ClearToken;
        }

        public override void Refresh()
        {
            base.Refresh();
            if (!HasDifferentValues)
            {
                m_Box.SetValue(GetValue(Values[0]!));
            }
        }

        protected abstract void GetLimits(out ulong min, out ulong max);
        protected abstract ulong GetValue(object value);
        protected abstract object ConvertValue(ulong value);
    }

    [CustomEditor(typeof(byte)), DefaultEditor]
    public sealed class ByteEditor : UnsignedIntegerValueEditor
    {
        protected override void GetLimits(out ulong min, out ulong max) { min = byte.MinValue; max = byte.MaxValue; }
        protected override ulong GetValue(object value) => (byte)value;
        protected override object ConvertValue(ulong value) => (byte)value;
    }

    [CustomEditor(typeof(char)), DefaultEditor]
    public sealed class CharEditor : UnsignedIntegerValueEditor
    {
        protected override void GetLimits(out ulong min, out ulong max) { min = char.MinValue; max = char.MaxValue; }
        protected override ulong GetValue(object value) => (char)value;
        protected override object ConvertValue(ulong value) => (char)value;
    }

    [CustomEditor(typeof(ushort)), DefaultEditor]
    public sealed class UShortEditor : UnsignedIntegerValueEditor
    {
        protected override void GetLimits(out ulong min, out ulong max)
        {
            min = ushort.MinValue; max = ushort
            .MaxValue;
        }
        protected override ulong GetValue(object value) => (ushort)value;
        protected override object ConvertValue(ulong value) => (ushort)value;
    }

    [CustomEditor(typeof(uint)), DefaultEditor]
    public sealed class UintEditor : UnsignedIntegerValueEditor
    {
        protected override void GetLimits(out ulong min, out ulong max) { min = uint.MinValue; max = uint.MaxValue; }
        protected override ulong GetValue(object value) => (uint)value;
        protected override object ConvertValue(ulong value) => (uint)value;
    }

    [CustomEditor(typeof(ulong)), DefaultEditor]
    public sealed class ULongEditor : UnsignedIntegerValueEditor
    {
        protected override void GetLimits(out ulong min, out ulong max) { min = ulong.MinValue; max = ulong.MaxValue; }
        protected override ulong GetValue(object value) => (ulong)value;
        protected override object ConvertValue(ulong value) => value;
    }
}
