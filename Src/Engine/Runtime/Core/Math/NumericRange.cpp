#include "NumericRange.h"

#include "Runtime/Core/Logging/Logging.h"

//-------------------------------------------------------------------------

namespace SE
{
	FloatRange::FloatRange(float min, float max)
		: begin(min), end(max)
	{
		ENGINE_ASSERT(min <= max);
	}

	bool FloatRange::IsValid() const
	{
		ENGINE_ASSERT(IsSet());
		return end >= begin;
	}

	bool FloatRange::Overlaps(FloatRange const& rhs) const
	{
		ENGINE_ASSERT(IsSetAndValid());
		return Math::Max(begin, rhs.begin) <= Math::Min(end, rhs.end);
	}

	bool FloatRange::ContainsInclusive(FloatRange const& rhs) const
	{
		ENGINE_ASSERT(IsSetAndValid() && rhs.IsSetAndValid());
		return begin <= rhs.begin && end >= rhs.end;
	}

	bool FloatRange::ContainsInclusive(float const& v) const
	{
		ENGINE_ASSERT(IsSetAndValid());
		return v >= begin && v <= end;
	}

	bool FloatRange::ContainsExclusive(FloatRange const& rhs) const
	{
		ENGINE_ASSERT(IsSetAndValid() && rhs.IsSetAndValid());
		return begin < rhs.begin && end > rhs.end;
	}

	bool FloatRange::ContainsExclusive(float const& v) const
	{
		ENGINE_ASSERT(IsSetAndValid());
		return v > begin && v < end;
	}

	float FloatRange::GetClampedValue(float const& v) const
	{
		ENGINE_ASSERT(IsSetAndValid());
		return Math::Clamp(v, begin, end);
	}

	float FloatRange::GetPercentageThrough(float const& v) const
	{
		ENGINE_ASSERT(IsSet());
		float const length = GetLength();
		float percentageThrough = 0.0f;
		if (length != 0)
		{
			percentageThrough = (v - begin) / length;
		}
		return percentageThrough;
	}

	float FloatRange::GetValueForPercentageThrough(float const percentageThrough) const
	{
		ENGINE_ASSERT(IsSet());
		return (GetLength() * percentageThrough) + begin;
	}

	void FloatRange::MakeValid()
	{
		ENGINE_ASSERT(IsSet());

		if (!IsValid())
		{
			float originalEnd = end;
			end = begin;
			begin = originalEnd;
		}
	}

	void FloatRange::GrowRange(float newValue)
	{
		if (IsSet())
		{
			ENGINE_ASSERT(IsValid());
			begin = Math::Min(begin, newValue);
			end = Math::Max(end, newValue);
		}
		else
		{
			begin = end = newValue;
		}
	}

	void FloatRange::Merge(FloatRange const& rhs)
	{
		ENGINE_ASSERT(IsSetAndValid() && rhs.IsSetAndValid());
		begin = Math::Min(begin, rhs.begin);
		end = Math::Max(end, rhs.end);
	}

	IntRange::IntRange(int32 min, int32 max)
		: begin(min), end(max)
	{
		ENGINE_ASSERT(IsValid());
	}

	bool IntRange::IsValid() const
	{
		ENGINE_ASSERT(IsSet());
		return end >= begin;
	}

	bool IntRange::Overlaps(IntRange const& rhs) const
	{
		ENGINE_ASSERT(IsSetAndValid());
		return Math::Max(begin, rhs.begin) <= Math::Min(end, rhs.end);
	}

	bool IntRange::ContainsInclusive(IntRange const& rhs) const
	{
		ENGINE_ASSERT(IsSetAndValid() && rhs.IsSetAndValid());
		return begin <= rhs.begin && end >= rhs.end;
	}

	bool IntRange::ContainsInclusive(int32 const& v) const
	{
		ENGINE_ASSERT(IsSetAndValid());
		return v >= begin && v <= end;
	}

	bool IntRange::ContainsExclusive(IntRange const& rhs) const
	{
		ENGINE_ASSERT(IsSetAndValid() && rhs.IsSetAndValid());
		return begin < rhs.begin && end > rhs.end;
	}

	bool IntRange::ContainsExclusive(int32 const& v) const
	{
		ENGINE_ASSERT(IsSetAndValid());
		return v > begin && v < end;
	}

	void IntRange::MakeValid()
	{
		ENGINE_ASSERT(IsSet());

		if (!IsValid())
		{
			int32 originalEnd = end;
			end = begin;
			begin = originalEnd;
		}
	}

	void IntRange::GrowRange(int32 newValue)
	{
		if (IsSet())
		{
			ENGINE_ASSERT(IsValid());
			begin = Math::Min(begin, newValue);
			end = Math::Max(end, newValue);
		}
		else
		{
			begin = end = newValue;
		}
	}

	void IntRange::Merge(IntRange const& rhs)
	{
		ENGINE_ASSERT(IsSetAndValid() && rhs.IsSetAndValid());
		begin = Math::Min(begin, rhs.begin);
		end = Math::Max(end, rhs.end);
	}
}
