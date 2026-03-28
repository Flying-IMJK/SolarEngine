#pragma once
#include "ValueBox.h"

namespace SE::Editor
{
    /// <summary>
    /// Floating point value editor.
    /// </summary>
    /// <seealso cref="float" />
    class FloatValueBox : public ValueBox<float>
    {
    public:
        float GetValue() override { return _value; }
        void SetValue(float value) override;

        float GetMinValue() override { return _min;  }
        void SetMinValue(float value) override;

        float GetMaxValue() override { return _max;  }
        void SetMaxValue(float value) override;

        /// <summary>
        /// Initializes a new instance of the <see cref="FloatValueBox"/> class.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <param name="x">The x location.</param>
        /// <param name="y">The y location.</param>
        /// <param name="width">The width.</param>
        /// <param name="min">The minimum value.</param>
        /// <param name="max">The maximum value.</param>
        /// <param name="slideSpeed">The slide speed.</param>
        FloatValueBox(float value, float x = 0, float y = 0, float width = 120, float min = Min_float, float max = Max_float, float slideSpeed = 1);

        /// <summary>
        /// Sets the value limits.
        /// </summary>
        /// <param name="min">The minimum value (bottom range).</param>
        /// <param name="max">The maximum value (upper range).</param>
        void SetLimits(float min, float max);

        /// <summary>
        /// Sets the limits from the attribute.
        /// </summary>
        /// <param name="value">The speed.</param>
        void SetSpeed(float value);

        /// <summary>
        /// Sets the limits from the other <see cref="FloatValueBox"/>.
        /// </summary>
        /// <param name="other">The other.</param>
        void SetLimits(FloatValueBox other);

        /// <summary>
        /// Gets or sets the category of the value. This can be none for just a number or a more specific one like a distance.
        /// </summary>
        /*public Utils.ValueCategory Category
        {
            get => _category;
            set
            {
                if (_category == value)
                    return;
                _category = value;
                UpdateText();
            }
        }*/

    private:
        void TryUseAutoSliderSpeed();

    protected:
        /// <inheritdoc />
        void UpdateText() override;

        /// <inheritdoc />
        void TryGetValue() override;

        /// <inheritdoc />
        void ApplySliding(float delta) override
        {
            SetValue(_startSlideValue + delta);
        }
    };

} // SE

