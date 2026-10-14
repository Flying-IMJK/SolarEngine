using System;

namespace SE
{
    /// <summary>
    /// Selects a CustomEditor type for the annotated value type or member. The editor type must
    /// derive from SE.Editor.CustomEditor and have a parameterless constructor.
    /// </summary>
    [AttributeUsage(AttributeTargets.Class | AttributeTargets.Enum | AttributeTargets.Struct | AttributeTargets.Field |
        AttributeTargets.Property | AttributeTargets.Delegate | AttributeTargets.Event | AttributeTargets.Method)]
    public sealed class CustomEditorAttribute : Attribute
    {
        public readonly Type Type;
        public CustomEditorAttribute(Type type) => Type = type;
    }
}
