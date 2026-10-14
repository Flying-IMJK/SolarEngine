using System;

namespace SE
{
    /// <summary>Sets how enum item names are displayed in the editor.</summary>
    [AttributeUsage(AttributeTargets.Field | AttributeTargets.Property | AttributeTargets.Enum)]
    public sealed class EnumDisplayAttribute : Attribute
    {
        public enum FormatMode
        {
            Default = 0,
            None = 1,
        }

        public FormatMode Mode;

        public EnumDisplayAttribute(FormatMode mode) => Mode = mode;
    }
}
