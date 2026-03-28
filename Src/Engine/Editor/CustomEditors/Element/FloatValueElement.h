#pragma once
#include "IFloatValueElement.h"
#include "Editor/CustomEditors/LayoutElement.h"
#include "Editor/GUI/Input/FloatValueBox.h"

namespace SE::Editor
{

    /// <summary>
    /// The floating point value element.
    /// </summary>
    /// <seealso cref="FlaxEditor.CustomEditors.LayoutElement" />
    class FloatValueElement : public LayoutElement, public IFloatValueElement
    {
        SE_CLASS(FloatValueElement, LayoutElement)
    public:
        /// <summary>
        /// The float value box.
        /// </summary>
        FloatValueBox* ValueBox;

        /// <summary>
        /// Initializes a new instance of the <see cref="FloatValueElement"/> class.
        /// </summary>
        FloatValueElement();

        /*
        /// <summary>
        /// Sets the editor limits from member <see cref="LimitAttribute"/>.
        /// </summary>
        /// <param name="member">The member.</param>
        void SetLimits(MemberInfo member)
        {
            // Try get limit attribute for value min/max range setting and slider speed
            if (member != null)
            {
                var attributes = member.GetCustomAttributes(true);
                var limit = attributes.FirstOrDefault(x => x is LimitAttribute);
                if (limit != null)
                {
                    ValueBox.SetLimits((LimitAttribute)limit);
                }
            }
        }

        /// <summary>
        /// Sets the editor value category.
        /// </summary>
        /// <param name="category">The category.</param>
        void SetCategory(Utils.ValueCategory category)
        {
            ValueBox.Category = category;
        }

        /// <summary>
        /// Sets the editor limits from member <see cref="LimitAttribute"/>.
        /// </summary>
        /// <param name="limit">The limit.</param>
        void SetLimits(LimitAttribute limit)
        {
            if (limit != null)
            {
                ValueBox.SetLimits(limit);
            }
        }

        /// <summary>
        /// Sets the editor limits from the other <see cref="FloatValueElement"/>.
        /// </summary>
        /// <param name="other">The other.</param>
        void SetLimits(FloatValueElement other)
        {
            if (other != null)
            {
                ValueBox.SetLimits(other.ValueBox);
            }
        }
        */

        Control* GetControl() override;

        float GetValue() override;
        void SetValue(float value) override;
        bool IsSliding() override;
    };
} // SE
