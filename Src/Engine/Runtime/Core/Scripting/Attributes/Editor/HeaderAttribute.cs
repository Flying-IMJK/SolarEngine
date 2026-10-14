using System;

namespace SE
{
    /// <summary>Adds a header before the annotated field or property in GenericEditor.</summary>
    [Serializable]
    [AttributeUsage(AttributeTargets.Field | AttributeTargets.Property)]
    public sealed class HeaderAttribute : Attribute
    {
        public string Text = string.Empty;
        public int FontSize;
        public uint Color;

        public HeaderAttribute() { }

        public HeaderAttribute(string text, int fontSize = -1, uint color = 0)
        {
            Text = text;
            FontSize = fontSize;
            Color = color;
        }
    }
}
