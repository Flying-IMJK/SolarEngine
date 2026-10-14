using System;

namespace SE
{
    /// <summary>Sets the watermark displayed by a string editor.</summary>
    [Serializable]
    [AttributeUsage(AttributeTargets.Field | AttributeTargets.Property)]
    public class WatermarkAttribute : Attribute
    {
        public string WatermarkText;
        public uint WatermarkColor;

        public WatermarkAttribute(string text)
        {
            WatermarkText = text;
        }

        public WatermarkAttribute(string text, uint color)
        {
            WatermarkText = text;
            WatermarkColor = color;
        }
    }
}
