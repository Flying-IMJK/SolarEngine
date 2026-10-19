using System.Linq;
using SE.GUI;

namespace SE.Editor
{
    [CustomEditor(typeof(string)), DefaultEditor]
    public sealed class StringEditor : CustomEditor
    {
        private TextBox m_TextBox = null!;
        private string m_WatermarkText = string.Empty;
        private Color m_WatermarkColor;
        private Color m_DefaultWatermarkColor;

        public override DisplayStyle Style => DisplayStyle.Inline;

        public override void Initialize(LayoutElementsContainer layout)
        {
            object[] attributes = Values.GetAttributes();
            bool isMultiline = attributes.Any(x => x is MultilineTextAttribute);
            m_TextBox = new TextBox(isMultiline, 0, 0)
            {
                Height = isMultiline ? TextBox.DefaultHeight * 4 : TextBox.DefaultHeight,
                Parent = layout.ContainerControl,
            };
            m_WatermarkColor = m_DefaultWatermarkColor = m_TextBox.WatermarkTextColor;
            if (attributes.OfType<WatermarkAttribute>().FirstOrDefault() is { } watermark)
            {
                m_WatermarkText = watermark.WatermarkText;
                m_WatermarkColor = watermark.WatermarkColor > 0
                    ? Color.FromRGB(watermark.WatermarkColor)
                    : SE.GUI.Style.Current.ForegroundDisabled;
                m_TextBox.WatermarkText = m_WatermarkText;
                m_TextBox.WatermarkTextColor = m_WatermarkColor;
            }
            m_TextBox.EditEnd += () => SetValue(m_TextBox.Text);
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
                m_TextBox.Text = (string)Values[0] ?? string.Empty;
                m_TextBox.WatermarkTextColor = m_WatermarkColor;
                m_TextBox.WatermarkText = m_WatermarkText;
            }
        }
    }
}
