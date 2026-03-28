#pragma once

#include "Core/Math/Vector2.h"
#include "Core/Types/Hash.h"
#include "Core/Types/Property.h"
#include "Runtime/API.h"

namespace SE
{

    /// <summary>
    /// Describes the space around a control.
    /// </summary>
    struct SE_API_RUNTIME Margin
    {
        // static String _formatString = "Left:{0:F2} Right:{1:F2} Top:{2:F2} Bottom:{3:F2}";

        /// <summary>
        /// A <see cref="Margin" /> with all of its components set to zero.
        /// </summary>
        static Margin Zero;

        /// <summary>
        /// Holds the margin to the left.
        /// </summary>
        float Left = 0;

        /// <summary>
        /// Holds the margin to the right.
        /// </summary>
        float Right = 0;

        /// <summary>
        /// Holds the margin to the top.
        /// </summary>
        float Top = 0;

        /// <summary>
        /// Holds the margin to the bottom.
        /// </summary>
        float Bottom = 0;

        /// <summary>
        /// Gets the margin's location (Left, Top).
        /// </summary>
        Float2 GetLocation();

        /// <summary>
        /// Gets the margin's total size. Cumulative margin size (Left + Right, Top + Bottom).
        /// </summary>
        Float2 GetSize();

        /// <summary>
        /// Gets the width (left + right).
        /// </summary>
        float GetWidth();

        /// <summary>
        /// Gets the height (top + bottom).
        /// </summary>
        float GetHeight();

        Margin();

        /// <summary>
        /// Initializes a new instance of the <see cref="Margin"/> struct.
        /// </summary>
        /// <param name="value">The value.</param>
        explicit Margin(float value);

        /// <summary>
        /// Initializes a new instance of the <see cref="Margin"/> struct.
        /// </summary>
        /// <param name="left">The left.</param>
        /// <param name="right">The right.</param>
        /// <param name="top">The top.</param>
        /// <param name="bottom">The bottom.</param>
        Margin(float left, float right, float top, float bottom);

        /// <summary>
        /// Gets a value indicting whether this margin is zero.
        /// </summary>
        bool IsZero() const { return Math::IsZero(Left) && Math::IsZero(Right) && Math::IsZero(Top) && Math::IsZero(Bottom); }

        /// <summary>
        /// Shrinks the rectangle by this margin.
        /// </summary>
        /// <param name="rect">The rectangle.</param>
        void ShrinkRectangle(Rectangle rect) const;

        /// <summary>
        /// Expands the rectangle by this margin.
        /// </summary>
        /// <param name="rect">The rectangle.</param>
        void ExpandRectangle(Rectangle rect) const;

        /// <summary>
        /// Adds two margins.
        /// </summary>
        /// <param name="value">margins to add.</param>
        /// <returns>The sum of the two margins.</returns>
        Margin operator +(const Margin& value) const;

        /// <summary>
        /// Subtracts two margins.
        /// </summary>
        /// <param name="value">The first margins to subtract from.</param>
        /// <returns>The result of subtraction of the two margins.</returns>
        Margin operator -(const Margin& value) const;

        /// <summary>
        /// Tests for equality between two objects.
        /// </summary>
        /// <param name="value">The value to compare.</param>
        /// <returns><c>true</c> if <paramref name="left" /> has the same value as <paramref name="right" />; otherwise, <c>false</c>.</returns>
        bool operator ==(const Margin& value) const;

        /// <summary>
        /// Tests for inequality between two objects.
        /// </summary>
        /// <param name="value">The value to compare.</param>
        /// <returns><c>true</c> if <paramref name="left" /> has a different value than <paramref name="right" />; otherwise, <c>false</c>.</returns>
        bool operator !=(const Margin& value) const;

        /// <summary>
        /// Returns a <see cref="System.String" /> that represents this instance.
        /// </summary>
        /// <returns>A <see cref="System.String" /> that represents this instance.</returns>
        String ToString() const;
    };


    inline uint32 GetHash(const Margin &key)
    {
        uint32 hashCode = 0;
        HashCombine(hashCode, key.Left);
        HashCombine(hashCode, key.Right);
        HashCombine(hashCode, key.Top);
        HashCombine(hashCode, key.Bottom);
        return hashCode;
    }

} // SE
