using System;

namespace SE
{
    /// <summary>
    /// Configures collection presentation and editing in the inspector. Limits and flags are
    /// applied by the collection editor; they do not constrain changes made outside it.
    /// </summary>
    [AttributeUsage(AttributeTargets.Field | AttributeTargets.Property | AttributeTargets.Class)]
    public sealed class CollectionAttribute : Attribute
    {
        /// <summary>Requested collection layout style.</summary>
        public enum DisplayType { Default, Header, Inline }
        /// <summary>Collection layout style.</summary>
        public DisplayType Display;
        /// <summary>Disallows inspector edits to collection structure and elements.</summary>
        public bool ReadOnly;
        /// <summary>Allows moving elements in the inspector.</summary>
        public bool CanReorderItems = true;
        /// <summary>Allows changing the collection length in the inspector.</summary>
        public bool CanResize = true;
        /// <summary>Rejects null element values entered through the inspector.</summary>
        public bool NotNullItems;
        /// <summary>Editor alias used for each element, when present.</summary>
        public string OverrideEditorTypeName;
        /// <summary>Vertical space inserted between element rows.</summary>
        public float Spacing;
        /// <summary>Minimum inspector length.</summary>
        public int MinCount;
        /// <summary>Maximum inspector length; zero uses the editor default.</summary>
        public int MaxCount;
        /// <summary>Optional collection background tint.</summary>
        public Color? BackgroundColor;
    }
}
