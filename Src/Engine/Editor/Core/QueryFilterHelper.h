#pragma once
#include "Core/Types/Property.h"
#include "Core/Types/Strings/String.h"

namespace SE::Editor
{
    /// <summary>
    /// Helper class to filter items based on a input filter query.
    /// </summary>
    class QueryFilterHelper
    {
        struct Range;
    public:
        /// <summary>
        /// The minimum text match length.
        /// </summary>
        static constexpr  int MinLength = 1;

        /// <summary>
        /// Matches the specified text with the filter.
        /// </summary>
        /// <param name="filter">The filter.</param>
        /// <param name="text">The text.</param>
        /// <returns>True if text has one or more matches, otherwise false.</returns>
        static bool Match(String& filter, String& text);

        /// <summary>
        /// Matches the specified text with the filter.
        /// </summary>
        /// <param name="filter">The filter.</param>
        /// <param name="text">The text.</param>
        /// <param name="matches">The found matches.</param>
        /// <returns>True if text has one or more matches, otherwise false.</returns>
        static bool Match(String& filter, String& text, List<Range>& matches);

        /// <summary>
        /// Describes sub range of the text.
        /// </summary>
        struct Range
        {
        public:
            /// <summary>
            /// The start index of the range.
            /// </summary>
            int StartIndex;

            /// <summary>
            /// The length.
            /// </summary>
            int Length;

            /// <summary>
            /// The end index of the range.
            /// </summary>
            PRO_GET(EndIndex, Range, int, __GetEndIndex);

            Range() = default;

            /// <summary>
            /// Initializes a new instance of the <see cref="Range"/> struct.
            /// </summary>
            /// <param name="start">The start.</param>
            /// <param name="length">The length.</param>
            Range(int start, int length)
            {
                StartIndex = start;
                Length = length;
            }

            /// <summary>
            /// Tests for equality between two objects.
            /// </summary>
            /// <param name="right">The second value to compare.</param>
            /// <returns><c>true</c> if <paramref name="left"/> has the same value as <paramref name="right"/>; otherwise, <c>false</c>.</returns>
            bool operator ==(Range right)
            {
                return Equals(right);
            }

            /// <summary>
            /// Tests for equality between two objects.
            /// </summary>
            /// <param name="right">The second value to compare.</param>
            /// <returns><c>true</c> if <paramref name="left"/> has the same value as <paramref name="right"/>; otherwise, <c>false</c>.</returns>
            bool operator !=(Range right)
            {
                return !Equals(right);
            }

            /// <summary>
            /// Compares this object with the other instance.
            /// </summary>
            /// <param name="other">The other object.</param>
            /// <returns>True if objects are equal.</returns>
            bool Equals(Range other)
            {
                return StartIndex == other.StartIndex && Length == other.Length;
            }

        private:
            int __GetEndIndex() { return StartIndex + Length; }
        };
    };

    String ToString(QueryFilterHelper::Range& range);
} // SE

