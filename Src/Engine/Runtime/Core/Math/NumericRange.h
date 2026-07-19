#pragma once

#include "Runtime/API.h"
#include "Math.h"
#include "Runtime/Core/Logging/Logging.h"

//-------------------------------------------------------------------------

namespace SE
{
    struct SE_API_RUNTIME FloatRange
    {
        FloatRange() = default;

        inline FloatRange(float value)
            : begin(value), end(value)
        {
        }

        FloatRange(float min, float max);

        // Reset the range to an invalid value
        FORCE_INLINE void Clear() { *this = FloatRange(); }

        // Is the range initialized
        FORCE_INLINE bool IsSet() const { return begin != FLT_MAX; }

        // Is the range contained valid i.e. is the end greater than the start
        bool IsValid() const;

        // Is the range set and valid
        FORCE_INLINE bool IsSetAndValid() const { return IsSet() && IsValid(); }

        // Get the length of the range
        FORCE_INLINE float GetLength() const { return end - begin; }

        // Get the midpoint of the range
        FORCE_INLINE float GetMidpoint() const { return begin + ((end - begin) / 2); }

        // Does this range overlap the specified range
        bool Overlaps(FloatRange const &rhs) const;

        // Shifts the range by the supplied delta
        FORCE_INLINE void ShiftRange(float delta)
        {
            begin += delta;
            end += delta;
        }

        // Does the range [begin, end] contain the specified range
        bool ContainsInclusive(FloatRange const &rhs) const;

        // Does the range [begin, end] contain the specified value
        bool ContainsInclusive(float const &v) const;

        // Does the range (min, max) contain the specified range
        bool ContainsExclusive(FloatRange const &rhs) const;

        // Does the range (min, max) contain the specified value
        bool ContainsExclusive(float const &v) const;

        // Get a value clamped to this range i.e. Clamp to [begin, end]
        float GetClampedValue(float const &v) const;

        // Get the percentage through this range that specified value lies at. This is not clamped and returns a value between [-FLT_MAX, FLT_MAX]
        float GetPercentageThrough(float const &v) const;

        // Get the percentage through this range that specified value lies at. This is clamped between [0, 1]
        FORCE_INLINE float GetPercentageThroughClamped(float const &v) const
        {
            return Math::Clamp(GetPercentageThrough(v), 0.0f, 1.0f);
        }

        // Get the value in this range at the specified percentage through. Unclamped so returns [-FLT_MAX, FLT_MAX]
        float GetValueForPercentageThrough(float const percentageThrough) const;

        // Get the value in this range at the specified percentage through. Clamped to [begin, end]
        // inline float GetValueForPercentageThroughClamped(Percentage const percentageThrough) const
        // {
        //     return GetValueForPercentageThrough(percentageThrough.GetClamped(false));
        // }

        // Ensure the range is valid if the values are set incorrectly
        void MakeValid();

        // Insert a new value into the range and grow it if necessary
        void GrowRange(float newValue);

        // Grow this range with the supplied range
        void Merge(FloatRange const &rhs);

        // Get the combined range of this and the supplied range
        FORCE_INLINE FloatRange GetMerged(FloatRange const &rhs) const
        {
            FloatRange mergedRange = *this;
            mergedRange.Merge(rhs);
            return mergedRange;
        }

        FORCE_INLINE bool operator==(FloatRange const &rhs) const
        {
            return begin == rhs.begin && end == rhs.end;
        }

        FORCE_INLINE bool operator!=(FloatRange const &rhs) const
        {
            return begin != rhs.begin || end != rhs.end;
        }

    public:
        float begin = FLT_MAX;
        float end = -FLT_MAX;
    };

    //-------------------------------------------------------------------------

    struct SE_API_RUNTIME IntRange
    {
        IntRange() = default;

        inline IntRange(int32 value)
            : begin(value), end(value)
        {
        }

        IntRange(int32 min, int32 max);

        // Reset the range to an invalid value
        FORCE_INLINE void Clear() { *this = IntRange(); }

        // Is the range initialized
        FORCE_INLINE bool IsSet() const { return begin != INT_MAX; }

        // Is the range contained valid i.e. is the end greater than the start
        bool IsValid() const;

        // Is the range set and valid
        FORCE_INLINE bool IsSetAndValid() const { return IsSet() && IsValid(); }

        // Get the length of the range
        FORCE_INLINE int32 GetLength() const { return end - begin; }

        // Get the midpoint of the range
        FORCE_INLINE int32 GetMidpoint() const { return begin + ((end - begin) / 2); }

        // Does this range overlap the specified range
        bool Overlaps(IntRange const &rhs) const;

        // Shifts the range by the supplied delta
        FORCE_INLINE void ShiftRange(int32 delta)
        {
			begin += delta;
			end += delta;
        }

        // Does the range [begin, end] contain the specified range
        bool ContainsInclusive(IntRange const &rhs) const;

        // Does the range [begin, end] contain the specified value
        bool ContainsInclusive(int32 const &v) const;

        // Does the range (min, max) contain the specified range
        bool ContainsExclusive(IntRange const &rhs) const;

        // Does the range (min, max) contain the specified value
        bool ContainsExclusive(int32 const &v) const;

        // Get a value clamped to this range i.e. Clamp to [begin, end]
        inline int32 GetClampedValue(int32 const &v) const
        {
            ENGINE_ASSERT(IsSetAndValid());
            return Math::Clamp(v, begin, end);
        }

        // Get the percentage through this range that specified value lies at. This is not clamped and returns a value between [-FLT_MAX, FLT_MAX]
        // inline Percentage GetPercentageThrough(int32 const &v) const
        // {
        //     ENGINE_ASSERT(IsSet());
        //     int32 const length = GetLength();
        //     Percentage percentageThrough = 0.0f;
        //     if (length != 0)
        //     {
        //         percentageThrough = Percentage(float(v - m_begin) / length);
        //     }
        //     return percentageThrough;
        // }

        // Get the percentage through this range that specified value lies at. This is clamped between [0, 1]
        // inline Percentage GetPercentageThroughClamped(int32 const &v) const
        // {
        //     return Math::Clamp(GetPercentageThrough(v).ToFloat(), 0.0f, 1.0f);
        // }

        // Get the value in this range at the specified percentage through. Unclamped so returns [-FLT_MAX, FLT_MAX]
        // inline int32 GetValueForPercentageThrough(Percentage const percentageThrough) const
        // {
        //     ENGINE_ASSERT(IsSet());
        //     return Math::RoundToInt((GetLength() * percentageThrough) + m_begin);
        // }

        // Get the value in this range at the specified percentage through. Clamped to [begin, end]
        // inline int32 GetValueForPercentageThroughClamped(Percentage const percentageThrough) const
        // {
        //     return GetValueForPercentageThrough(percentageThrough.GetClamped(false));
        // }

        // Ensure the range is valid if the values are set incorrectly
        void MakeValid();

        // Insert a new value into the range and grow it if necessary
        void GrowRange(int32 newValue);

        // Grow this range with the supplied range
        void Merge(IntRange const &rhs);

        // Get the combined range of this and the supplied range
        FORCE_INLINE IntRange GetMerged(IntRange const &rhs) const
        {
            IntRange mergedRange = *this;
            mergedRange.Merge(rhs);
            return mergedRange;
        }

        FORCE_INLINE bool operator==(IntRange const &rhs) const
        {
            return begin == rhs.begin && end == rhs.end;
        }

        FORCE_INLINE bool operator!=(IntRange const &rhs) const
        {
            return begin != rhs.begin || end != rhs.end;
        }

    public:
        int32 begin = INT_MAX;
        int32 end = INT_MIN;
    };
}