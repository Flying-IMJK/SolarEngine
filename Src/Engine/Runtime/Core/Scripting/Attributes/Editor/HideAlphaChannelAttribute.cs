using System;

namespace SE
{
    /// <summary>Hides alpha controls when editing a color field or property, preserving its alpha value.</summary>
    [AttributeUsage(AttributeTargets.Field | AttributeTargets.Property)]
    public sealed class HideAlphaChannelAttribute : Attribute
    {
    }
}
