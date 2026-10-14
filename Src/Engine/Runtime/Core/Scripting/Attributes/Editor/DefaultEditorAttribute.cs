using System;

namespace SE
{
    /// <summary>Marks a registered custom editor as an engine default editor.</summary>
    [AttributeUsage(AttributeTargets.Class | AttributeTargets.Enum | AttributeTargets.Struct | AttributeTargets.Field |
        AttributeTargets.Property | AttributeTargets.Delegate | AttributeTargets.Event | AttributeTargets.Method)]
    public sealed class DefaultEditorAttribute : Attribute
    {
    }
}
