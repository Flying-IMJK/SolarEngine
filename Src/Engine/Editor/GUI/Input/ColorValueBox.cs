using System;
using System.Globalization;
using SE.GUI;

namespace SE.Editor.GUI
{
    /// <summary>
    /// Text-based color editor using <c>#RRGGBB</c> or <c>#RRGGBBAA</c> notation.
    /// </summary>
    public sealed class ColorValueBox : TextBox
    {
        private Color _value;

        public ColorValueBox(Color value, float x = 0.0f, float y = 0.0f, float width = 120.0f)
            : base(new Rectangle(x, y, width, 18.0f))
        {
            Value = value;
        }

        public event Action<ColorValueBox>? ValueChanged;

        public Color Value
        {
            get => _value;
            set
            {
                if (_value == value)
                    return;
                _value = value;
                BackgroundColor = value;
                Text = Format(value);
                ValueChanged?.Invoke(this);
            }
        }

        public override void EndEdit()
        {
            base.EndEdit();
            if (TryParse(Text, out Color color))
                Value = color;
            else
                Text = Format(_value);
        }

        private static string Format(Color color)
        {
            int r = (int)MathF.Round(Math.Clamp(color.R, 0.0f, 1.0f) * 255.0f);
            int g = (int)MathF.Round(Math.Clamp(color.G, 0.0f, 1.0f) * 255.0f);
            int b = (int)MathF.Round(Math.Clamp(color.B, 0.0f, 1.0f) * 255.0f);
            int a = (int)MathF.Round(Math.Clamp(color.A, 0.0f, 1.0f) * 255.0f);
            return $"#{r:X2}{g:X2}{b:X2}{a:X2}";
        }

        private static bool TryParse(string text, out Color color)
        {
            color = default;
            if (string.IsNullOrWhiteSpace(text))
                return false;
            string value = text.Trim().TrimStart('#');
            if (value.Length != 6 && value.Length != 8)
                return false;

            if (!int.TryParse(value[..2], NumberStyles.HexNumber, CultureInfo.InvariantCulture, out int r) ||
                !int.TryParse(value.Substring(2, 2), NumberStyles.HexNumber, CultureInfo.InvariantCulture, out int g) ||
                !int.TryParse(value.Substring(4, 2), NumberStyles.HexNumber, CultureInfo.InvariantCulture, out int b) ||
                (value.Length == 8 && !int.TryParse(value.Substring(6, 2), NumberStyles.HexNumber, CultureInfo.InvariantCulture, out int a)))
                return false;

            int alpha = value.Length == 8 ? int.Parse(value.Substring(6, 2), NumberStyles.HexNumber, CultureInfo.InvariantCulture) : 255;
            color = new Color(r, g, b, alpha);
            return true;
        }
    }
}
