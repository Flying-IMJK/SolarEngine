using System;
using System.Linq;
using SE.Editor.GUI;

namespace SE.Editor
{
    [CustomEditor(typeof(int)), DefaultEditor]
    public sealed class IntegerEditor : CustomEditor
    {
        private IntValueBox? _box;
        private SliderControl? _slider;

        public override DisplayStyle Style => DisplayStyle.Inline;

        public override void Initialize(LayoutElementsContainer layout)
        {
            object[] attributes = Values.GetAttributes();
            if (attributes.OfType<RangeAttribute>().FirstOrDefault() is { } range)
            {
                _slider = new SliderControl(0) { Parent = layout.ContainerControl };
                _slider.SetLimits(range);
                _slider.ValueChanged += OnValueChanged;
                _slider.SlidingEnd += ClearToken;
                return;
            }
            _box = new IntValueBox { Parent = layout.ContainerControl };
            if (attributes.OfType<LimitAttribute>().FirstOrDefault() is { } limit)
                _box.SetLimits(limit);
            _box.ValueChanged += OnValueChanged;
            _box.SlidingEnd += ClearToken;
        }

        private void OnValueChanged()
        {
            bool sliding = _slider?.IsSliding ?? _box!.IsSliding;
            int value = _slider != null ? (int)_slider.Value : _box!.GetValue();
            SetValue(value, sliding ? this : null);
        }

        public override void Refresh()
        {
            base.Refresh();
            if (HasDifferentValues) return;
            int value = Convert.ToInt32(Values[0]);
            if (_slider != null) _slider.Value = value;
            else _box!.SetValue(value);
        }
    }

    public abstract class SignedIntegerValueEditor : CustomEditor
    {
        private LongValueBox _box = null!;

        public override DisplayStyle Style => DisplayStyle.Inline;

        public override void Initialize(LayoutElementsContainer layout)
        {
            GetLimits(out long min, out long max);
            _box = new LongValueBox { Parent = layout.ContainerControl };
            if (Values.GetAttributes().OfType<LimitAttribute>().FirstOrDefault() is { } limit)
                _box.SetLimits(limit);
            _box.SetLimits(Math.Max(_box.GetMinValue(), min), Math.Min(_box.GetMaxValue(), max));
            _box.ValueChanged += () => SetValue(ConvertValue(_box.GetValue()), _box.IsSliding ? this : null);
            _box.SlidingEnd += ClearToken;
        }

        public override void Refresh()
        {
            base.Refresh();
            if (!HasDifferentValues)
                _box.SetValue(GetValue(Values[0]!));
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
        private ULongValueBox _box = null!;

        public override DisplayStyle Style => DisplayStyle.Inline;

        public override void Initialize(LayoutElementsContainer layout)
        {
            GetLimits(out ulong min, out ulong max);
            _box = new ULongValueBox { Parent = layout.ContainerControl };
            if (Values.GetAttributes().OfType<LimitAttribute>().FirstOrDefault() is { } limit)
                _box.SetLimits(limit);
            _box.SetLimits(Math.Max(_box.GetMinValue(), min), Math.Min(_box.GetMaxValue(), max));
            _box.ValueChanged += () => SetValue(ConvertValue(_box.GetValue()), _box.IsSliding ? this : null);
            _box.SlidingEnd += ClearToken;
        }

        public override void Refresh()
        {
            base.Refresh();
            if (!HasDifferentValues)
                _box.SetValue(GetValue(Values[0]!));
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
        protected override void GetLimits(out ulong min, out ulong max) { min = ushort.MinValue; max = ushort.MaxValue; }
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
