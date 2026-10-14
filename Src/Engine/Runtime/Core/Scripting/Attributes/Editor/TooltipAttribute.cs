using System;

namespace SE
{
    /// <summary>Supplies the tooltip text for the annotated editor member.</summary>
    [Serializable]
    [AttributeUsage(AttributeTargets.All)]
    public sealed class TooltipAttribute : Attribute
    {
        public string Text;

        public TooltipAttribute(string text) => Text = text;
    }
}
