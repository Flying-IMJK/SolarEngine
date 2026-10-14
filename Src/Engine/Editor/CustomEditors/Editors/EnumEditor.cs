using System;
using System.Linq;
using SE.Editor.GUI;

namespace SE.Editor
{
    [CustomEditor(typeof(Enum)), DefaultEditor]
    public class EnumEditor : CustomEditor
    {
        protected EnumComboBox? element;

        public override DisplayStyle Style => DisplayStyle.Inline;

        public override void Initialize(LayoutElementsContainer layout)
        {
            var mode = Values.GetAttributes().OfType<EnumDisplayAttribute>().FirstOrDefault()?.Mode ??
                EnumDisplayAttribute.FormatMode.Default;
            if (HasDifferentTypes)
                return;
            Type type = Values.Type != typeof(object) || Values[0] == null
                ? Values.Type
                : Values[0]!.GetType();
            element = new EnumComboBox(type, mode)
            {
                Height = ComboBox.DefaultHeight,
                Parent = layout.ContainerControl,
            };
            element.ValueChanged += OnValueChanged;
        }

        protected virtual void OnValueChanged()
        {
            if (element != null)
                SetValue(element.EnumTypeValue);
        }

        public override void Refresh()
        {
            base.Refresh();
            if (!HasDifferentTypes && !HasDifferentValues && element != null && Values[0] != null)
                element.EnumTypeValue = Values[0]!;
        }
    }
}
