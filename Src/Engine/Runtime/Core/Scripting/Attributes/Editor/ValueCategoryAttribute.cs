using System;

namespace SE
{
    /// <summary>
    /// Selects unit formatting for supported numeric inspector controls. Stored values retain
    /// their original units; formatting only changes how they are shown and parsed.
    /// </summary>
    [Serializable]
    [AttributeUsage(AttributeTargets.Field | AttributeTargets.Property)]
    public sealed class ValueCategoryAttribute : Attribute
    {
        /// <summary>Formatting category used by the inspector.</summary>
        public Utils.ValueCategory Category;
        /// <summary>Creates a formatting hint for the given category.</summary>
        public ValueCategoryAttribute(Utils.ValueCategory category) => Category = category;
    }
}
