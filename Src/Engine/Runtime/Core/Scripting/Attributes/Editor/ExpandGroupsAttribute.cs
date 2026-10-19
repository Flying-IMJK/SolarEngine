using System;

namespace SE
{
    /// <summary>Expands the property groups containing the annotated member.</summary>
    [AttributeUsage(AttributeTargets.Field | AttributeTargets.Property | AttributeTargets.Delegate
        | AttributeTargets.Event | AttributeTargets.Method)]
    public sealed class ExpandGroupsAttribute : Attribute
    {
    }
}
