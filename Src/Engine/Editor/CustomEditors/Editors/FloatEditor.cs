using System;
using System.Linq;
using SE.Editor.GUI;

namespace SE.Editor
{
    [CustomEditor(typeof(float)), DefaultEditor]
    public sealed class FloatEditor : CustomEditor
    {
        private FloatValueBox m_Box;
        private SliderControl m_Slider;
        public FloatValueBox Element => m_Box;

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
            m_Box = new FloatValueBox(0) { Parent = layout.ContainerControl };
            if (attributes.OfType<ValueCategoryAttribute>().FirstOrDefault() is { } category &&
                category.Category != Utils.ValueCategory.None)
            {
                m_Box.Category = category.Category;
                if (LinkedLabel != null)
                {
                    LinkedLabel.SetupContextMenu += (label, menu, editor) =>
                                        {
                                            menu.AddSeparator();
                                            ContextMenuButton button = menu.AddButton("Show formatted", item =>
                                                m_Box.Category = item.Checked ? category.Category : Utils
                                                    .ValueCategory.None);
                                            button.AutoCheck = true;
                                            button.Checked = m_Box.Category != Utils.ValueCategory.None;
                                        };
                }
            }
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
            float value = m_Slider?.Value ?? m_Box!.GetValue();
            SetValue(value, sliding ? this : null);
        }

        public override void Refresh()
        {
            base.Refresh();
            if (HasDifferentValues)
            {
                return;
            }
            float value = Convert.ToSingle(Values[0]);
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

    [CustomEditor(typeof(double)), DefaultEditor]
    public sealed class DoubleEditor : CustomEditor
    {
        private DoubleValueBox m_Box = null!;

        public override DisplayStyle Style => DisplayStyle.Inline;

        public override void Initialize(LayoutElementsContainer layout)
        {
            m_Box = new DoubleValueBox(0) { Parent = layout.ContainerControl };
            if (Values.GetAttributes().OfType<ValueCategoryAttribute>().FirstOrDefault() is { } category &&
                category.Category != Utils.ValueCategory.None)
            {
                m_Box.Category = category.Category;
                if (LinkedLabel != null)
                {
                    LinkedLabel.SetupContextMenu += (label, menu, editor) =>
                                        {
                                            menu.AddSeparator();
                                            ContextMenuButton button = menu.AddButton("Show formatted", item =>
                                                m_Box.Category = item.Checked ? category.Category : Utils
                                                    .ValueCategory.None);
                                            button.AutoCheck = true;
                                            button.Checked = m_Box.Category != Utils.ValueCategory.None;
                                        };
                }
            }
            if (Values.GetAttributes().OfType<LimitAttribute>().FirstOrDefault() is { } limit)
            {
                m_Box.SetLimits(limit);
            }
            m_Box.ValueChanged += () => SetValue(m_Box.GetValue(), m_Box.IsSliding ? this : null);
            m_Box.SlidingEnd += ClearToken;
        }

        public override void Refresh()
        {
            base.Refresh();
            if (!HasDifferentValues && Values[0] != null)
            {
                m_Box.SetValue(Convert.ToDouble(Values[0]));
            }
        }
    }
}
