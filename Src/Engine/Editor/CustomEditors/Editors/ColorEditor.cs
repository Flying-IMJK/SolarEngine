using SE.Editor.GUI;

namespace SE.Editor
{
    [CustomEditor(typeof(Color)), DefaultEditor]
    public sealed class ColorEditor : CustomEditor
    {
        private ColorValueBox m_Box = null!;
        public override DisplayStyle Style => DisplayStyle.Inline;

        public override void Initialize(LayoutElementsContainer layout)
        {
            m_Box = new ColorValueBox { Parent = layout.ContainerControl };
            m_Box.ValueChanged += () => SetValue(m_Box.Value, m_Box.IsSliding ? this : null);
        }

        public override void Refresh()
        {
            base.Refresh();
            if (HasDifferentValues)
            {
                return;
            }
            object value = Values[0];
            if (value is Color color)
            {
                m_Box.Value = color;
            }
            else if (value is Color32 color32)
            {
                m_Box.Value = color32;
            }
            else if (value is Float4 float4)
            {
                m_Box.Value = float4;
            }
            else if (value is Double4 double4)
            {
                m_Box.Value = (Float4)double4;
            }
        }
    }
}
