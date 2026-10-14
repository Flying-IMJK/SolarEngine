using System;

namespace SE
{
    /// <summary>
    /// Resolves a CustomEditor by its assembly-qualified or loaded-assembly type name.
    /// The resolved type must derive from SE.Editor.CustomEditor and be constructible.
    /// </summary>
    [AttributeUsage(AttributeTargets.Class | AttributeTargets.Enum | AttributeTargets.Struct | AttributeTargets.Field |
        AttributeTargets.Property | AttributeTargets.Delegate | AttributeTargets.Event | AttributeTargets.Method)]
    public sealed class CustomEditorAliasAttribute : Attribute
    {
        public readonly string TypeName;
        public CustomEditorAliasAttribute(string typeName) => TypeName = typeName;
    }
}
