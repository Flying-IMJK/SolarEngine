using System;
using System.Linq;
using SE.Editor.GUI;

namespace SE.Editor
{
    [CustomEditor(typeof(float)), DefaultEditor]
    public sealed class FloatEditor : CustomEditor
    {
        private FloatValueBox? _box;
        private SliderControl? _slider;
        public FloatValueBox? Element => _box;

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
            _box = new FloatValueBox(0) { Parent = layout.ContainerControl };
            if (attributes.OfType<ValueCategoryAttribute>().FirstOrDefault() is { } category &&
                category.Category != Utils.ValueCategory.None)
            {
                _box.Category = category.Category;
                if (LinkedLabel != null)
                    LinkedLabel.SetupContextMenu += (label, menu, editor) =>
                    {
                        menu.AddSeparator();
                        var button = menu.AddButton("Show formatted", item =>
                            _box.Category = item.Checked ? category.Category : Utils.ValueCategory.None);
                        button.AutoCheck = true;
                        button.Checked = _box.Category != Utils.ValueCategory.None;
                    };
            }
            if (attributes.OfType<LimitAttribute>().FirstOrDefault() is { } limit)
                _box.SetLimits(limit);
            _box.ValueChanged += OnValueChanged;
            _box.SlidingEnd += ClearToken;
        }

        private void OnValueChanged()
        {
            bool sliding = _slider?.IsSliding ?? _box!.IsSliding;
            float value = _slider?.Value ?? _box!.GetValue();
            SetValue(value, sliding ? this : null);
        }

        public override void Refresh()
        {
            base.Refresh();
            if (HasDifferentValues) return;
            float value = Convert.ToSingle(Values[0]);
            if (_slider != null) _slider.Value = value;
            else _box!.SetValue(value);
        }
    }

    [CustomEditor(typeof(double)), DefaultEditor]
    public sealed class DoubleEditor : CustomEditor
    {
        private DoubleValueBox _box = null!;

        public override DisplayStyle Style => DisplayStyle.Inline;

        public override void Initialize(LayoutElementsContainer layout)
        {
            _box = new DoubleValueBox(0) { Parent = layout.ContainerControl };
            if (Values.GetAttributes().OfType<ValueCategoryAttribute>().FirstOrDefault() is { } category &&
                category.Category != Utils.ValueCategory.None)
            {
                _box.Category = category.Category;
                if (LinkedLabel != null)
                    LinkedLabel.SetupContextMenu += (label, menu, editor) =>
                    {
                        menu.AddSeparator();
                        var button = menu.AddButton("Show formatted", item =>
                            _box.Category = item.Checked ? category.Category : Utils.ValueCategory.None);
                        button.AutoCheck = true;
                        button.Checked = _box.Category != Utils.ValueCategory.None;
                    };
            }
            if (Values.GetAttributes().OfType<LimitAttribute>().FirstOrDefault() is { } limit)
                _box.SetLimits(limit);
            _box.ValueChanged += () => SetValue(_box.GetValue(), _box.IsSliding ? this : null);
            _box.SlidingEnd += ClearToken;
        }

        public override void Refresh()
        {
            base.Refresh();
            if (!HasDifferentValues && Values[0] != null)
                _box.SetValue(Convert.ToDouble(Values[0]));
        }
    }
}
