#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"
#include "Color.h"
#include "Core/Types/Strings/String.h"

namespace SE
{
	// Float
	static_assert(sizeof(Float2) == 8, "Invalid Float2 type size.");

	template<>
	const Float2 Float2::Zero(0.0f);
	template<>
	const Float2 Float2::One(1.0f);
	template<>
	const Float2 Float2::Half(0.5f);
	template<>
	const Float2 Float2::UnitX(1.0f, 0.0f);
	template<>
	const Float2 Float2::UnitY(0.0f, 1.0f);
	template<>
	const Float2 Float2::Minimum(Min_float);
	template<>
	const Float2 Float2::Maximum(Max_float);

	template<>
	Float2::Vector2Base(const Int3& xy)
		: x((float)xy.x)
		, y((float)xy.y)
	{
	}

	template<>
	Float2::Vector2Base(const Int4& xy)
		: x((float)xy.x)
		, y((float)xy.y)
	{
	}

	template<>
	Float2::Vector2Base(const Float3& xy)
		: x(xy.x)
		, y(xy.y)
	{
	}

	template<>
	Float2::Vector2Base(const Float4& xy)
		: x(xy.x)
		, y(xy.y)
	{
	}

	template<>
	Float2::Vector2Base(const Double3& xy)
		: x((float)xy.x)
		, y((float)xy.y)
	{
	}

	template<>
	Float2::Vector2Base(const Double4& xy)
		: x((float)xy.x)
		, y((float)xy.y)
	{
	}

	template<>
	Float2::Vector2Base(const Color& color)
		: x(color.r)
		, y(color.g)
	{
	}

	template<>
	String Float2::ToString() const
	{
		return String::Format(SE_TEXT("{}"), *this);
	}

	template<>
	float Float2::TriangleArea(const Float2& v0, const Float2& v1, const Float2& v2)
	{
		return Math::Abs((v0.x * (v1.y - v2.y) + v1.x * (v2.y - v0.y) + v2.x * (v0.y - v1.y)) / 2);
	}

	template<>
	float Float2::Angle(const Float2& from, const Float2& to)
	{
		const float dot = Math::Clamp(Dot(Normalize(from), Normalize(to)), -1.0f, 1.0f);
		if (Math::Abs(dot) > 1.0f - Math::ZeroTolerance)
			return dot > 0.0f ? 0.0f : Math::PI;
		return Math::ACos(dot);
	}

// Double

	static_assert(sizeof(Double2) == 16, "Invalid Double2 type size.");

	template<>
	const Double2 Double2::Zero(0.0);
	template<>
	const Double2 Double2::One(1.0);
	template<>
	const Double2 Double2::UnitX(1.0, 0.0);
	template<>
	const Double2 Double2::UnitY(0.0, 1.0);
	template<>
	const Double2 Double2::Minimum(Min_double);
	template<>
	const Double2 Double2::Maximum(Max_double);

	template<>
	Double2::Vector2Base(const Int3& xy)
		: x((double)xy.x)
		, y((double)xy.y)
	{
	}

	template<>
	Double2::Vector2Base(const Int4& xy)
		: x((double)xy.x)
		, y((double)xy.y)
	{
	}

	template<>
	Double2::Vector2Base(const Float3& xy)
		: x((double)xy.x)
		, y((double)xy.y)
	{
	}

	template<>
	Double2::Vector2Base(const Float4& xy)
		: x((int32)xy.x)
		, y((double)xy.y)
	{
	}

	template<>
	Double2::Vector2Base(const Double3& xy)
		: x(xy.x)
		, y(xy.y)
	{
	}

	template<>
	Double2::Vector2Base(const Double4& xy)
		: x(xy.x)
		, y(xy.y)
	{
	}

	template<>
	Double2::Vector2Base(const Color& color)
		: x((double)color.r)
		, y((double)color.g)
	{
	}

	template<>
	String Double2::ToString() const
	{
		return String::Format(SE_TEXT("{}"), *this);
	}

	template<>
	double Double2::TriangleArea(const Double2& v0, const Double2& v1, const Double2& v2)
	{
		return Math::Abs((v0.x * (v1.y - v2.y) + v1.x * (v2.y - v0.y) + v2.x * (v0.y - v1.y)) / 2);
	}

	template<>
	double Double2::Angle(const Double2& from, const Double2& to)
	{
		const double dot = Math::Clamp(Dot(Normalize(from), Normalize(to)), -1.0, 1.0);
		if (Math::Abs(dot) > 1.0 - Math::ZeroTolerance)
			return dot > 0.0 ? 0.0 : Math::PI;
		return Math::ACos(dot);
	}

	// Int

	static_assert(sizeof(Int2) == 8, "Invalid Int2 type size.");

	template<>
	const Int2 Int2::Zero(0);
	template<>
	const Int2 Int2::One(1);
	template<>
	const Int2 Int2::UnitX(1, 0);
	template<>
	const Int2 Int2::UnitY(0, 1);
	template<>
	const Int2 Int2::Minimum(Min_int32);
	template<>
	const Int2 Int2::Maximum(Max_int32);

	template<>
	Int2::Vector2Base(const Int3& xy)
		: x(xy.x)
		, y(xy.y)
	{
	}

	template<>
	Int2::Vector2Base(const Int4& xy)
		: x(xy.x)
		, y(xy.y)
	{
	}

	template<>
	Int2::Vector2Base(const Float3& xy)
		: x((int32)xy.x)
		, y((int32)xy.y)
	{
	}

	template<>
	Int2::Vector2Base(const Float4& xy)
		: x((int32)xy.x)
		, y((int32)xy.y)
	{
	}

	template<>
	Int2::Vector2Base(const Double3& xy)
		: x((int32)xy.x)
		, y((int32)xy.y)
	{
	}

	template<>
	Int2::Vector2Base(const Double4& xy)
		: x((int32)xy.x)
		, y((int32)xy.y)
	{
	}

	template<>
	Int2::Vector2Base(const Color& color)
		: x((int32)color.r)
		, y((int32)color.g)
	{
	}

	template<>
	String Int2::ToString() const
	{
		return String::Format(SE_TEXT("{}"), *this);
	}

	template<>
	int32 Int2::TriangleArea(const Int2& v0, const Int2& v1, const Int2& v2)
	{
		return Math::Abs((v0.x * (v1.y - v2.y) + v1.x * (v2.y - v0.y) + v2.x * (v0.y - v1.y)) / 2);
	}

	template<>
	int32 Int2::Angle(const Int2& from, const Int2& to)
	{
		return 0;
	}

}