using System;

namespace SE
{
    /// <summary>
    /// Controls an item's display group and name in the editor.
    /// </summary>
    [Serializable]
    [AttributeUsage(AttributeTargets.Field | AttributeTargets.Property | AttributeTargets.Delegate | AttributeTargets.Event | AttributeTargets.Method)]
    public sealed class EditorDisplayAttribute : Attribute
    {
        /// <summary>
        /// Inlines the annotated property into its parent editor layout.
        /// </summary>
        public const string InlineStyle = "__inline__";

        /// <summary>
        /// The editor group name.
        /// </summary>
        public string? Group;

        /// <summary>
        /// The editor display name.
        /// </summary>
        public string? Name;

        private EditorDisplayAttribute()
        {
        }

        /// <summary>
        /// Initializes a new editor display attribute.
        /// </summary>
        public EditorDisplayAttribute(string? group = null, string? name = null)
        {
            Group = group;
            Name = name;
        }
    }
}
