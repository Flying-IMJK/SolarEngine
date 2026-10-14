using System;

namespace SE
{
    /// <summary>Displays a float or int editor value as a slider within a range.</summary>
    [Serializable]
    [AttributeUsage(AttributeTargets.Field | AttributeTargets.Property)]
    public sealed class RangeAttribute : Attribute
    {
        public float Min;
        public float Max;

        public RangeAttribute(float min, float max)
        {
            Min = min;
            Max = max;
        }
    }
}
