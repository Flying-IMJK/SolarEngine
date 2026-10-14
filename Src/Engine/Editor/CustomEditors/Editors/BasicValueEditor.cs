using System;
using System.Globalization;
using System.Linq;
using SE.Editor.GUI;
using SE.GUI;

namespace SE.Editor
{
    [HideInEditor]
    internal sealed class BasicValueEditor : CustomEditor
    {
        private bool _refreshing;
        private CheckBox? _checkBox;
        private TextBox? _textBox;
        private FloatValueBox? _floatBox;
        private ComboBox? _comboBox;

        public override DisplayStyle Style => DisplayStyle.Inline;

        public override void Initialize(LayoutElementsContainer layout)
        {
            Type type = Values.Type;
            if (type == typeof(bool))
            {
                _checkBox = new CheckBox { Height = 18, Parent = layout.ContainerControl };
                _checkBox.StateChanged += box =>
                {
                    if (!_refreshing)
                        SetValue(box.Checked);
                };
            }
            else if (type == typeof(float))
            {
                _floatBox = new FloatValueBox(Convert.ToSingle(Values.Count > 0 ? Values[0] : 0f, CultureInfo.InvariantCulture))
                {
                    Height = 18,
                    Parent = layout.ContainerControl
                };
                _floatBox.ValueChanged += () =>
                {
                    if (!_refreshing)
                        SetValue(_floatBox.GetValue(), _floatBox.IsSliding ? this : null);
                };
                _floatBox.SlidingEnd += ClearToken;
                if (Values.GetAttributes().OfType<LimitAttribute>().FirstOrDefault() is { } limit)
                {
                    _floatBox.SetLimits(limit.Min, limit.Max);
                    _floatBox.SetSpeed(limit.SliderSpeed);
                }
            }
            else if (type.IsEnum)
            {
                _comboBox = new ComboBox { Height = 18, Parent = layout.ContainerControl };
                foreach (string name in Enum.GetNames(type))
                    _comboBox.Items.Add(name);
                _comboBox.SelectedIndexChanged += box =>
                {
                    if (!_refreshing && box.SelectedIndex >= 0)
                        SetValue(Enum.Parse(type, box.SelectedItem));
                };
            }
            else
            {
                _textBox = new TextBox(false, 0, 0) { Height = 18, Parent = layout.ContainerControl };
                _textBox.EditEnd += () =>
                {
                    if (_refreshing)
                        return;
                    try
                    {
                        object value = type == typeof(string)
                            ? _textBox.Text
                            : Convert.ChangeType(_textBox.Text, type, CultureInfo.CurrentCulture);
                        SetValue(value);
                    }
                    catch (FormatException)
                    {
                        Refresh();
                    }
                    catch (OverflowException)
                    {
                        Refresh();
                    }
                };
            }
        }

        public override void Refresh()
        {
            base.Refresh();
            _refreshing = true;
            try
            {
                object? value = Values.Count > 0 ? Values[0] : null;
                if (_checkBox != null)
                    _checkBox.State = HasDifferentValues ? CheckBoxState.Intermediate : value is true ? CheckBoxState.Checked : CheckBoxState.Default;
                if (_floatBox != null && value != null && !HasDifferentValues)
                    _floatBox.SetValue(Convert.ToSingle(value, CultureInfo.InvariantCulture));
                if (_comboBox != null)
                    _comboBox.SelectedIndex = HasDifferentValues || value == null ? -1 : _comboBox.Items.IndexOf(value.ToString()!);
                if (_textBox != null && !_textBox.IsEditing)
                    _textBox.Text = HasDifferentValues ? string.Empty : Convert.ToString(value, CultureInfo.CurrentCulture) ?? string.Empty;
            }
            finally
            {
                _refreshing = false;
            }
        }
    }
}
