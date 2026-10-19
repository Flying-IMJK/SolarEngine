using System;
using SE.GUI;

namespace SE.Editor
{
    [CustomEditor(typeof(Guid)), DefaultEditor]
    public sealed class GuidEditor : CustomEditor
    {
        private TextBox m_TextBox = null!;
        private Color m_DefaultWatermarkColor;

        public override DisplayStyle Style => DisplayStyle.Inline;

        public override void Initialize(LayoutElementsContainer layout)
        {
            m_TextBox = new TextBox(false, 0, 0) { Parent = layout.ContainerControl };
            m_DefaultWatermarkColor = m_TextBox.WatermarkTextColor;
            m_TextBox.EditEnd += () =>
            {
                if (Guid.TryParse(m_TextBox.Text, out Guid value))
                {
                    SetValue(value);
                }
            };
        }

        public override void Refresh()
        {
            base.Refresh();
            if (HasDifferentValues)
            {
                m_TextBox.Text = string.Empty;
                m_TextBox.WatermarkTextColor = m_DefaultWatermarkColor;
                m_TextBox.WatermarkText = "Different values";
            }
            else
            {
                m_TextBox.Text = ((Guid)Values[0]!).ToString("D");
                m_TextBox.WatermarkText = string.Empty;
            }
        }
    }
}
