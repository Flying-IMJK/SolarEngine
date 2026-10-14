using System;

namespace SE
{
    /// <summary>Adds vertical space before the annotated field or property in GenericEditor.</summary>
    [Serializable]
    [AttributeUsage(AttributeTargets.Field | AttributeTargets.Property)]
    public sealed class SpaceAttribute : Attribute
    {
        public float Height;

        public SpaceAttribute(float height = 10.0f) => Height = height;
    }
}
