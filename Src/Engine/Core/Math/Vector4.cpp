
#include "Vector4.h"
#include "Vector2.h"
#include "Vector3.h"
#include "Color.h"
#include "Matrix.h"
#include "Rectangle.h"
#include "Core/Types/Strings/String.h"

// Float

namespace SE
{

	static_assert(sizeof(Float4) == 16, "Invalid Float4 type size.");

	template<>
	const Float4 Float4::Zero(0.0f);
	template<>
	const Float4 Float4::One(1.0f);
	template<>
	const Float4 Float4::Half(0.5f);
	template<>
	const Float4 Float4::UnitX(1.0f, 0.0f, 0.0f, 0.0f);
	template<>
	const Float4 Float4::UnitY(0.0f, 1.0f, 0.0f, 0.0f);
	template<>
	const Float4 Float4::UnitZ(0.0f, 0.0f, 1.0f, 0.0f);
	template<>
	const Float4 Float4::UnitW(0.0f, 0.0f, 0.0f, 1.0f);
	template<>
	const Float4 Float4::Minimum(Min_float);
	template<>
	const Float4 Float4::Maximum(Max_float);

	template<>
	Float4::Vector4Base(const Float2& xy, float z, float w)
		: x(xy.x)
		, y(xy.y)
		, z(z)
		, w(w)
	{
	}

	template<>
	Float4::Vector4Base(const Float2& xy, const Float2& zw)
		: x(xy.x)
		, y(xy.y)
		, z(zw.x)
		, w(zw.y)
	{
	}

	template<>
	Float4::Vector4Base(const Float3& xyz, float w)
		: x(xyz.x)
		, y(xyz.y)
		, z(xyz.z)
		, w(w)
	{
	}

	template<>
	Float4::Vector4Base(const Int2& xy, float z, float w)
		: x((float)xy.x)
		, y((float)xy.y)
		, z(z)
		, w(w)
	{
	}

	template<>
	Float4::Vector4Base(const Int3& xyz, float w)
		: x((float)xyz.x)
		, y((float)xyz.y)
		, z((float)xyz.z)
		, w(w)
	{
	}

	template<>
	Float4::Vector4Base(const Double2& xy, float z, float w)
		: x((float)xy.x)
		, y((float)xy.y)
		, z(z)
		, w(w)
	{
	}

	template<>
	Float4::Vector4Base(const Double2& xy, const Double2& zw)
		: x((float)xy.x)
		, y((float)xy.y)
		, z((float)zw.x)
		, w((float)zw.y)
	{
	}

	template<>
	Float4::Vector4Base(const Double3& xyz, float w)
		: x((float)xyz.x)
		, y((float)xyz.y)
		, z((float)xyz.z)
		, w(w)
	{
	}

	template<>
	Float4::Vector4Base(const Color& color)
		: x(color.r)
		, y(color.g)
		, z(color.b)
		, w(color.a)
	{
	}

	template<>
	Float4::Vector4Base(const Rectangle& rect)
		: x(rect.Location.x)
		, y(rect.Location.y)
		, z(rect.Size.x)
		, w(rect.Size.y)
	{
	}

	template<>
	String Float4::ToString() const
	{
		return String::Format(SE_TEXT("{}"), *this);
	}

	template<>
	Float4 Float4::Transform(const Float4& v, const Matrix& m)
	{
/*		return Float4(
			m.Values[0][0] * v.Raw[0] + m.Values[1][0] * v.Raw[1] + m.Values[2][0] * v.Raw[2] + m.Values[3][0] * v.Raw[3],
			m.Values[0][1] * v.Raw[0] + m.Values[1][1] * v.Raw[1] + m.Values[2][1] * v.Raw[2] + m.Values[3][1] * v.Raw[3],
			m.Values[0][2] * v.Raw[0] + m.Values[1][2] * v.Raw[1] + m.Values[2][2] * v.Raw[2] + m.Values[3][2] * v.Raw[3],
			m.Values[0][3] * v.Raw[0] + m.Values[1][3] * v.Raw[1] + m.Values[2][3] * v.Raw[2] + m.Values[3][3] * v.Raw[3]
		);*/
		return Float4::Zero;
	}

// Double

	static_assert(sizeof(Double4) == 32, "Invalid Double4 type size.");

	template<>
	const Double4 Double4::Zero(0.0);
	template<>
	const Double4 Double4::One(1.0);
	template<>
	const Double4 Double4::UnitX(1.0, 0.0, 0.0, 0.0);
	template<>
	const Double4 Double4::UnitY(0.0, 1.0, 0.0, 0.0);
	template<>
	const Double4 Double4::UnitZ(0.0, 0.0, 1.0, 0.0);
	template<>
	const Double4 Double4::UnitW(0.0, 0.0, 0.0, 1.0);
	template<>
	const Double4 Double4::Minimum(Min_double);
	template<>
	const Double4 Double4::Maximum(Max_double);

	template<>
	Double4::Vector4Base(const Float2& xy, double z, double w)
		: x((double)xy.x)
		, y((double)xy.y)
		, z(z)
		, w(w)
	{
	}

	template<>
	Double4::Vector4Base(const Float2& xy, const Float2& zw)
		: x((double)xy.x)
		, y((double)xy.y)
		, z((double)zw.x)
		, w((double)zw.y)
	{
	}

	template<>
	Double4::Vector4Base(const Float3& xyz, double w)
		: x((double)xyz.x)
		, y((double)xyz.y)
		, z((double)xyz.z)
		, w(w)
	{
	}

	template<>
	Double4::Vector4Base(const Int2& xy, double z, double w)
		: x((double)xy.x)
		, y((double)xy.y)
		, z(z)
		, w(w)
	{
	}

	template<>
	Double4::Vector4Base(const Int3& xyz, double w)
		: x((double)xyz.x)
		, y((double)xyz.y)
		, z((double)xyz.z)
		, w(w)
	{
	}

	template<>
	Double4::Vector4Base(const Double2& xy, double z, double w)
		: x(xy.x)
		, y(xy.y)
		, z(z)
		, w(w)
	{
	}

	template<>
	Double4::Vector4Base(const Double2& xy, const Double2& zw)
		: x(xy.x)
		, y(xy.y)
		, z(zw.x)
		, w(zw.y)
	{
	}

	template<>
	Double4::Vector4Base(const Double3& xyz, double w)
		: x((double)xyz.x)
		, y((double)xyz.y)
		, z((double)xyz.z)
		, w(w)
	{
	}

	template<>
	Double4::Vector4Base(const Color& color)
		: x((double)color.r)
		, y((double)color.g)
		, z((double)color.b)
		, w((double)color.a)
	{
	}

	template<>
	Double4::Vector4Base(const Rectangle& rect)
		: x((double)rect.Location.x)
		, y((double)rect.Location.y)
		, z((double)rect.Size.x)
		, w((double)rect.Size.y)
	{
	}

	template<>
	String Double4::ToString() const
	{
		return String::Format(SE_TEXT("{}"), *this);
	}

	template<>
	Double4 Double4::Transform(const Double4& v, const Matrix& m)
	{
//		return Double4(
//			m.Values[0][0] * v.Raw[0] + m.Values[1][0] * v.Raw[1] + m.Values[2][0] * v.Raw[2] + m.Values[3][0] * v.Raw[3],
//			m.Values[0][1] * v.Raw[0] + m.Values[1][1] * v.Raw[1] + m.Values[2][1] * v.Raw[2] + m.Values[3][1] * v.Raw[3],
//			m.Values[0][2] * v.Raw[0] + m.Values[1][2] * v.Raw[1] + m.Values[2][2] * v.Raw[2] + m.Values[3][2] * v.Raw[3],
//			m.Values[0][3] * v.Raw[0] + m.Values[1][3] * v.Raw[1] + m.Values[2][3] * v.Raw[2] + m.Values[3][3] * v.Raw[3]
//		);
		return Double4::Zero;
	}

// Int

	static_assert(sizeof(Int4) == 16, "Invalid Int4 type size.");

	template<>
	const Int4 Int4::Zero(0);
	template<>
	const Int4 Int4::One(1);
	template<>
	const Int4 Int4::UnitX(1, 0, 0, 0);
	template<>
	const Int4 Int4::UnitY(0, 1, 0, 0);
	template<>
	const Int4 Int4::UnitZ(0, 0, 1, 0);
	template<>
	const Int4 Int4::UnitW(0, 0, 0, 1);
	template<>
	const Int4 Int4::Minimum(Min_int32);
	template<>
	const Int4 Int4::Maximum(Max_int32);

	template<>
	Int4::Vector4Base(const Float2& xy, int32 z, int32 w)
		: x((int32)xy.x)
		, y((int32)xy.y)
		, z(z)
		, w(w)
	{
	}

	template<>
	Int4::Vector4Base(const Float2& xy, const Float2& zw)
		: x((int32)xy.x)
		, y((int32)xy.y)
		, z((int32)zw.x)
		, w((int32)zw.y)
	{
	}

	template<>
	Int4::Vector4Base(const Float3& xyz, int32 w)
		: x((int32)xyz.x)
		, y((int32)xyz.y)
		, z((int32)xyz.z)
		, w(w)
	{
	}

	template<>
	Int4::Vector4Base(const Int2& xy, int32 z, int32 w)
		: x(xy.x)
		, y(xy.y)
		, z(z)
		, w(w)
	{
	}

	template<>
	Int4::Vector4Base(const Int3& xyz, int32 w)
		: x(xyz.x)
		, y(xyz.y)
		, z(xyz.z)
		, w(w)
	{
	}

	template<>
	Int4::Vector4Base(const Double2& xy, int32 z, int32 w)
		: x((int32)xy.x)
		, y((int32)xy.y)
		, z(z)
		, w(w)
	{
	}

	template<>
	Int4::Vector4Base(const Double2& xy, const Double2& zw)
		: x((int32)xy.x)
		, y((int32)xy.y)
		, z((int32)zw.x)
		, w((int32)zw.y)
	{
	}

	template<>
	Int4::Vector4Base(const Double3& xyz, int32 w)
		: x((int32)xyz.x)
		, y((int32)xyz.y)
		, z((int32)xyz.z)
		, w(w)
	{
	}

	template<>
	Int4::Vector4Base(const Color& color)
		: x((int32)color.r)
		, y((int32)color.g)
		, z((int32)color.b)
		, w((int32)color.a)
	{
	}

	template<>
	Int4::Vector4Base(const Rectangle& rect)
		: x((int32)rect.Location.x)
		, y((int32)rect.Location.y)
		, z((int32)rect.Size.x)
		, w((int32)rect.Size.y)
	{
	}

	template<>
	String Int4::ToString() const
	{
		return String::Format(SE_TEXT("{}"), *this);
	}

	template<>
	Int4 Int4::Transform(const Int4& v, const Matrix& m)
	{
		return v;
	}

}