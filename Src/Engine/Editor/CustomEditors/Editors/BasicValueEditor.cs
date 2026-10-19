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
        private bool m_Refreshing;
        private CheckBox m_CheckBox;
        private TextBox m_TextBox;
        private FloatValueBox m_FloatBox;
        private ComboBox m_ComboBox;

        public override DisplayStyle Style => DisplayStyle.Inline;

        public override void Initialize(LayoutElementsContainer layout)
        {
            Type type = Values.Type;
            if (type == typeof(bool))
            {
                m_CheckBox = new CheckBox { Height = 18, Parent = layout.ContainerControl };
                m_CheckBox.StateChanged += box =>
                {
                    if (!m_Refreshing)
                    {
                        SetValue(box.Checked);
                    }
                };
            }
            else if (type == typeof(float))
            {
                m_FloatBox = new FloatValueBox(Convert.ToSingle(Values.Count > 0 ? Values[0] : 0f, CultureInfo
                    .InvariantCulture))
                {
                    Height = 18,
                    Parent = layout.ContainerControl
                };
                m_FloatBox.ValueChanged += () =>
                {
                    if (!m_Refreshing)
                    {
                        SetValue(m_FloatBox.GetValue(), m_FloatBox.IsSliding ? this : null);
                    }
                };
                m_FloatBox.SlidingEnd += ClearToken;
                if (Values.GetAttributes().OfType<LimitAttribute>().FirstOrDefault() is { } limit)
                {
                    m_FloatBox.SetLimits(limit.Min, limit.Max);
                    m_FloatBox.SetSpeed(limit.SliderSpeed);
                }
            }
            else if (type.IsEnum)
            {
                m_ComboBox = new ComboBox { Height = 18, Parent = layout.ContainerControl };
                foreach (string name in Enum.GetNames(type))
                {
                    m_ComboBox.Items.Add(name);
                }
                m_ComboBox.SelectedIndexChanged += box =>
                {
                    if (!m_Refreshing && box.SelectedIndex >= 0)
                    {
                        SetValue(Enum.Parse(type, box.SelectedItem));
                    }
                };
            }
            else
            {
                m_TextBox = new TextBox(false, 0, 0) { Height = 18, Parent = layout.ContainerControl };
                m_TextBox.EditEnd += () =>
                {
                    if (m_Refreshing)
                    {
                        return;
                    }
                    try
                    {
                        object value = type == typeof(string)
                            ? m_TextBox.Text
                            : Convert.ChangeType(m_TextBox.Text, type, CultureInfo.CurrentCulture);
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
            m_Refreshing = true;
            try
            {
                object value = Values.Count > 0 ? Values[0] : null;
                if (m_CheckBox != null)
                {
                    m_CheckBox.State = HasDifferentValues ? CheckBoxState.Intermediate : value is true
                        ? CheckBoxState.Checked : CheckBoxState.Default;
                }
                if (m_FloatBox != null && value != null && !HasDifferentValues)
                {
                    m_FloatBox.SetValue(Convert.ToSingle(value, CultureInfo.InvariantCulture));
                }
                if (m_ComboBox != null)
                {
                    m_ComboBox.SelectedIndex = HasDifferentValues || value == null ? -1 : m_ComboBox.Items
                        .IndexOf(value.ToString()!);
                }
                if (m_TextBox != null && !m_TextBox.IsEditing)
                {
                    m_TextBox.Text = HasDifferentValues ? string.Empty : Convert.ToString(value, CultureInfo
                        .CurrentCulture) ?? string.Empty;
                }
            }
            finally
            {
                m_Refreshing = false;
            }
        }
    }
}
