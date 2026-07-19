#pragma once

#include "Runtime/Core/Math/Rectangle.h"

namespace SE
{
    /// <summary>
    /// Specifies the alignment of the text along horizontal or vertical direction in the layout box.
    /// </summary>
    SE_ENUM(API)
    enum class TextAlignment
    {
        /// <summary>
        /// Align text near the edge.
        /// </summary>
        Near = 0,

        /// <summary>
        /// Align text to the center.
        /// </summary>
        Center,

        /// <summary>
        /// Align text to the far edge.
        /// </summary>
        Far,
    };

    /// <summary>
    /// Specifies text wrapping to be used in a particular multiline paragraph.
    /// </summary>
    SE_ENUM(API)
    enum class TextWrapping
    {
        /// <summary>
        /// No text wrapping.
        /// </summary>
        NoWrap = 0,

        /// <summary>
        /// Wrap only whole words that overflow.
        /// </summary>
        WrapWords,

        /// <summary>
        /// Wrap single characters that overflow.
        /// </summary>
        WrapChars,
    };

    /// <summary>
    /// Structure which describes text layout properties.
    /// </summary>
    SE_STRUCT(API)
    struct TextLayoutOptions
    {
        /// <summary>
        /// The layout rectangle (text bounds).
        /// </summary>
        SE_PROPERTY(API)
        Rectangle Bounds;

        /// <summary>
        /// The horizontal alignment mode.
        /// </summary>
        SE_PROPERTY(API)
        TextAlignment HorizontalAlignment;

        /// <summary>
        /// The vertical alignment mode.
        /// </summary>
        SE_PROPERTY(API)
        TextAlignment VerticalAlignment;

        /// <summary>
        /// The text wrapping mode.
        /// </summary>
        SE_PROPERTY(API)
        TextWrapping TextWrapping = TextWrapping::NoWrap;

        /// <summary>
        /// The text scale factor. Default is 1.
        /// </summary>
        SE_PROPERTY(API)
        float Scale = 1.0f;

        /// <summary>
        /// Base line gap scale. Default is 1.
        /// </summary>
        SE_PROPERTY(API)
        float BaseLinesGapScale = 1.0f;

        /// <summary>
        /// Initializes a new instance of the <see cref="TextLayoutOptions"/> struct.
        /// </summary>
        TextLayoutOptions()
        {
            Bounds = Rectangle(0, 0, Max_float, Max_float);
            HorizontalAlignment = TextAlignment::Near;
            VerticalAlignment = TextAlignment::Near;
            TextWrapping = TextWrapping::NoWrap;
            Scale = 1.0f;
            BaseLinesGapScale = 1.0f;
        }

        FORCE_INLINE bool operator==(const TextLayoutOptions& other) const
        {
            return Bounds == other.Bounds
                    && HorizontalAlignment == other.HorizontalAlignment
                    && VerticalAlignment == other.VerticalAlignment
                    && TextWrapping == other.TextWrapping
                    && Math::IsNearEqual(Scale, other.Scale)
                    && Math::IsNearEqual(BaseLinesGapScale, other.BaseLinesGapScale);
        }

        FORCE_INLINE bool operator!=(const TextLayoutOptions& other) const
        {
            return !operator==(other);
        }

        static TextLayoutOptions Default()
        {
            TextLayoutOptions layoutOptions;
            layoutOptions.Bounds = Rectangle(0, 0, Max_float, Max_float);
            layoutOptions.Scale = 1.0f;
            layoutOptions.BaseLinesGapScale = 1.0f;
            return layoutOptions;
        }

    };

}
