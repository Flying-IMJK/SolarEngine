using SE.GUI;

namespace SE.Editor
{
    [CustomEditor(typeof(bool)), DefaultEditor]
    public sealed class BooleanEditor : CustomEditor
    {
        private CheckBox _checkBox = null!;

        public override DisplayStyle Style => DisplayStyle.Inline;

        public override void Initialize(LayoutElementsContainer layout)
        {
            _checkBox = new CheckBox { Height = 18, Parent = layout.ContainerControl };
            _checkBox.StateChanged += box => SetValue(box.Checked);
        }

        public override void Refresh()
        {
            base.Refresh();
            if (HasDifferentValues)
                _checkBox.Intermediate = true;
            else if (Values[0] is bool value)
                _checkBox.Checked = value;
        }
    }
}
