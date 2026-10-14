using System.Linq;
using SE.GUI;

namespace SE.Editor
{
    [CustomEditor(typeof(string)), DefaultEditor]
    public sealed class StringEditor : CustomEditor
    {
        private TextBox _textBox = null!;
        private string _watermarkText = string.Empty;
        private Color _watermarkColor;
        private Color _defaultWatermarkColor;

        public override DisplayStyle Style => DisplayStyle.Inline;

        public override void Initialize(LayoutElementsContainer layout)
        {
            object[] attributes = Values.GetAttributes();
            bool isMultiline = attributes.Any(x => x is MultilineTextAttribute);
            _textBox = new TextBox(isMultiline, 0, 0)
            {
                Height = isMultiline ? TextBox.DefaultHeight * 4 : TextBox.DefaultHeight,
                Parent = layout.ContainerControl,
            };
            _watermarkColor = _defaultWatermarkColor = _textBox.WatermarkTextColor;
            if (attributes.OfType<WatermarkAttribute>().FirstOrDefault() is { } watermark)
            {
                _watermarkText = watermark.WatermarkText;
                _watermarkColor = watermark.WatermarkColor > 0
                    ? Color.FromRGB(watermark.WatermarkColor)
                    : SE.GUI.Style.Current.ForegroundDisabled;
                _textBox.WatermarkText = _watermarkText;
                _textBox.WatermarkTextColor = _watermarkColor;
            }
            _textBox.EditEnd += () => SetValue(_textBox.Text);
        }

        public override void Refresh()
        {
            base.Refresh();
            if (HasDifferentValues)
            {
                _textBox.Text = string.Empty;
                _textBox.WatermarkTextColor = _defaultWatermarkColor;
                _textBox.WatermarkText = "Different values";
            }
            else
            {
                _textBox.Text = (string?)Values[0] ?? string.Empty;
                _textBox.WatermarkTextColor = _watermarkColor;
                _textBox.WatermarkText = _watermarkText;
            }
        }
    }
}
