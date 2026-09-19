using System;

namespace SE
{
    /// <summary>
    /// Prevents an item from being animated by the scene animation system.
    /// </summary>
    [Serializable]
    [AttributeUsage(AttributeTargets.Property | AttributeTargets.Field | AttributeTargets.Method)]
    public sealed class NoAnimateAttribute : Attribute
    {
    }
}
