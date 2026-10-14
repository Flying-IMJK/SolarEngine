using System;
using SE.GUI;

namespace SE.Editor
{
    [CustomEditor(typeof(Guid)), DefaultEditor]
    public sealed class GuidEditor : CustomEditor
    {
        private TextBox _textBox = null!;
        private Color _defaultWatermarkColor;

        public override DisplayStyle Style => DisplayStyle.Inline;

        public override void Initialize(LayoutElementsContainer layout)
        {
            _textBox = new TextBox(false, 0, 0) { Parent = layout.ContainerControl };
            _defaultWatermarkColor = _textBox.WatermarkTextColor;
            _textBox.EditEnd += () =>
            {
                if (Guid.TryParse(_textBox.Text, out Guid value))
                    SetValue(value);
            };
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
                _textBox.Text = ((Guid)Values[0]!).ToString("D");
                _textBox.WatermarkText = string.Empty;
            }
        }
    }
}
