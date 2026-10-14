using System;

namespace SE
{
    /// <summary>Uses a multiline text box for a string field or property.</summary>
    [AttributeUsage(AttributeTargets.Field | AttributeTargets.Property)]
    public sealed class MultilineTextAttribute : Attribute
    {
    }
}
