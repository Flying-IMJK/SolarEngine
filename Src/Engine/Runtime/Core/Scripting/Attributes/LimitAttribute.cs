using System;

namespace SE
{
    /// <summary>
    /// Restricts an integer or floating-point value to an editor range.
    /// </summary>
    [Serializable]
    [AttributeUsage(AttributeTargets.Field | AttributeTargets.Property)]
    public sealed class LimitAttribute : Attribute
    {
        /// <summary>
        /// The minimum value.
        /// </summary>
        public float Min;

        /// <summary>
        /// The maximum value.
        /// </summary>
        public float Max;

        /// <summary>
        /// The slider editing speed.
        /// </summary>
        public float SliderSpeed;

        private LimitAttribute()
        {
            Min = 0.0f;
            Max = 100.0f;
            SliderSpeed = 1.0f;
        }

        /// <summary>
        /// Initializes a new limit attribute.
        /// </summary>
        public LimitAttribute(float min, float max = float.MaxValue, float sliderSpeed = 1.0f)
        {
            Min = min;
            Max = max;
            SliderSpeed = sliderSpeed;
        }
    }
}
