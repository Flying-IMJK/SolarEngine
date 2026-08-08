using SE.GUI;

namespace SE.Editor.GUI
{
    /// <summary>
    /// Table column descriptor.
    /// </summary>
    public class ColumnDefinition
    {
        public delegate string ValueFormatDelegate(object value);

        public bool UseExpandCollapseMode;
        public TextAlignment CellAlignment = TextAlignment.Far;
        public string? Title;
        public Font? TitleFont;
        public Color TitleColor = Color.White;
        public Color TitleBackgroundColor = Color.Brown;
        public float MinSize = 10.0f;
        public float MinSizePercentage = 0.0f;
        public float MaxSize = float.MaxValue;
        public float MaxSizePercentage = 1.0f;
        public ValueFormatDelegate? FormatValue;

        public float ClampColumnSize(float value, float tableSize)
        {
            float width = Mathf.Clamp(value, MinSizePercentage, MaxSizePercentage) * tableSize;
            return Mathf.Clamp(width, MinSize, MaxSize) / tableSize;
        }
    }
}
