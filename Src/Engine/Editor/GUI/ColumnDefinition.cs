using System;
using SE.GUI;

namespace SE.Editor.GUI
{
    /// <summary>
    /// Describes the appearance and sizing policy for a <see cref="Table"/> column.
    /// </summary>
    public sealed class ColumnDefinition
    {
        public delegate string ValueFormatDelegate(object value);

        public bool UseExpandCollapseMode { get; set; }
        public TextAlignment CellAlignment { get; set; } = TextAlignment.Far;
        public string Title { get; set; } = string.Empty;
        public Font? TitleFont { get; set; }
        public Color TitleColor { get; set; } = Color.White;
        public Color TitleBackgroundColor { get; set; } = Style.Current.BackgroundNormal;
        public float MinSize { get; set; } = 10.0f;
        public float MinSizePercentage { get; set; }
        public float MaxSize { get; set; } = float.MaxValue;
        public float MaxSizePercentage { get; set; } = 1.0f;
        public ValueFormatDelegate? FormatValue { get; set; }

        public float ClampColumnSize(float value, float tableSize)
        {
            if (tableSize <= 0.0f)
                return 0.0f;

            float normalized = Math.Clamp(value, MinSizePercentage, MaxSizePercentage);
            float width = Math.Clamp(normalized * tableSize, MinSize, MaxSize);
            return width / tableSize;
        }
    }
}
