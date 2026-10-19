using SE.GUI;

namespace SE.Editor
{
    [CustomEditor(typeof(bool)), DefaultEditor]
    public sealed class BooleanEditor : CustomEditor
    {
        private CheckBox m_CheckBox = null!;

        public override DisplayStyle Style => DisplayStyle.Inline;

        public override void Initialize(LayoutElementsContainer layout)
        {
            m_CheckBox = new CheckBox { Height = 18, Parent = layout.ContainerControl };
            m_CheckBox.StateChanged += box => SetValue(box.Checked);
        }

        public override void Refresh()
        {
            base.Refresh();
            if (HasDifferentValues)
            {
                m_CheckBox.Intermediate = true;
            }
            else if (Values[0] is bool value)
            {
                m_CheckBox.Checked = value;
            }
        }
    }
}
