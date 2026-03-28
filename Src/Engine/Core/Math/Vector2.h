#pragma once

#include "Math.h"
#include "Core/Utilities/Formatting.h"
#include "Core/Templates.h"

namespace SE
{
	/// <summary>
	/// Represents a two dimensional mathematical vector.
	/// </summary>
	template<typename T>
	struct Vector2Base
	{
		typedef T Real;
		static SE_API_CORE struct ScriptingTypeInitializer TypeInitializer;

		union
		{
			struct
			{
				/// <summary>
				/// The X component of the vector.
				/// </summary>
				T x;

				/// <summary>
				/// The Y component of the vector.
				/// </summary>
				T y;
			};

			/// <summary>
			/// The raw vector values (in XY order).
			/// </summary>
			T Raw[2];
		};

	public:
		// Vector with all components equal 0
		static SE_API_CORE const Vector2Base<T> Zero;

		// Vector with all components equal 1
		static SE_API_CORE const Vector2Base<T> One;

		// Vector with all components equal 0.5
		static SE_API_CORE const Vector2Base<T> Half;

		// Vector X=1, Y=0
		static SE_API_CORE const Vector2Base<T> UnitX;

		// Vector X=0, Y=1
		static SE_API_CORE const Vector2Base<T> UnitY;

		// Vector with all components equal maximum value.
		static SE_API_CORE const Vector2Base<T> Minimum;

		// Vector with all components equal minimum value.
		static SE_API_CORE const Vector2Base<T> Maximum;

	public:
		/// <summary>
		/// Empty constructor.
		/// </summary>
		Vector2Base() = default;

		inline Vector2Base(T xy)
			: x(xy)
			, y(xy)
		{
		}

		inline explicit Vector2Base(const T* xy)
			: x(xy[0])
			, y(xy[1])
		{
		}

		inline Vector2Base(T x, T y)
			: x(x)
			, y(y)
		{
		}

		template<typename U = T, typename TEnableIf<TNot<TIsTheSame<T, U>>::Value>::Type...>
		inline Vector2Base(const Vector2Base<U>& xy)
			: x((T)xy.x)
			, y((T)xy.y)
		{
		}

		SE_API_CORE explicit Vector2Base(const Int3& xy);
		SE_API_CORE explicit Vector2Base(const Int4& xy);
		SE_API_CORE explicit Vector2Base(const Float3& xy);
		SE_API_CORE explicit Vector2Base(const Float4& xy);
		SE_API_CORE explicit Vector2Base(const Double3& xy);
		SE_API_CORE explicit Vector2Base(const Double4& xy);
		SE_API_CORE explicit Vector2Base(const Color& color);

	public:
		SE_API_CORE String ToString() const;

	public:
		// Gets a value indicting whether this instance is normalized.
		bool IsNormalized() const
		{
			return Math::IsOne(x * x + y * y);
		}

		// Gets a value indicting whether this vector is zero.
		bool IsZero() const
		{
			return Math::IsZero(x) && Math::IsZero(y);
		}

		// Gets a value indicting whether any vector component is zero.
		bool IsAnyZero() const
		{
			return Math::IsZero(x) || Math::IsZero(y);
		}

		// Gets a value indicting whether this vector is zero.
		bool IsOne() const
		{
			return Math::IsOne(x) && Math::IsOne(y);
		}

		// Calculates the length of the vector.
		T Length() const
		{
			return Math::Sqrt(x * x + y * y);
		}

		// Calculates the squared length of the vector.
		T LengthSquared() const
		{
			return x * x + y * y;
		}

		// Calculates inverted length of the vector (1 / length).
		T InvLength() const
		{
			return 1.0f / Length();
		}

		/// <summary>
		/// Returns the average arithmetic of all the components.
		/// </summary>
		T AverageArithmetic() const
		{
			return (x + y) * 0.5f;
		}

		/// <summary>
		/// Gets the sum of all vector components values.
		/// </summary>
		T SumValues() const
		{
			return x + y;
		}

		/// <summary>
		/// Gets the multiplication result of all vector components values.
		/// </summary>
		T MulValues() const
		{
			return x * y;
		}

		/// <summary>
		/// Returns the minimum value of all the components.
		/// </summary>
		T MinValue() const
		{
			return Math::Min(x, y);
		}

		/// <summary>
		/// Returns the maximum value of all the components.
		/// </summary>
		T MaxValue() const
		{
			return Math::Max(x, y);
		}

		/// <summary>
		/// Returns true if vector has one or more components is not a number (NaN).
		/// </summary>
		bool IsNaN() const
		{
			return isnan(x) || isnan(y);
		}

		/// <summary>
		/// Returns true if vector has one or more components equal to +/- infinity.
		/// </summary>
		bool IsInfinity() const
		{
			return isinf(x) || isinf(y);
		}

		/// <summary>
		/// Returns true if vector has one or more components equal to +/- infinity or NaN.
		/// </summary>
		bool IsNanOrInfinity() const
		{
			return IsInfinity() || IsNaN();
		}

		/// <summary>
		/// Calculates a vector with values being absolute values of that vector.
		/// </summary>
		Vector2Base GetAbsolute() const
		{
			return Vector2Base(Math::Abs(x), Math::Abs(y));
		}

		/// <summary>
		/// Calculates a vector with values being opposite to values of that vector.
		/// </summary>
		Vector2Base GetNegative() const
		{
			return Vector2Base(-x, -y);
		}

		/// <summary>
		/// Calculates a normalized vector that has length equal to 1.
		/// </summary>
		Vector2Base GetNormalized() const
		{
			Vector2Base result(x, y);
			result.Normalize();
			return result;
		}

	public:
		/// <summary>
		/// Performs vector normalization (scales vector up to unit length).
		/// </summary>
		void Normalize()
		{
			const T length = Math::Sqrt(x * x + y * y);
			if (length >= Math::ZeroTolerance)
			{
				const T invLength = (T)1.0f / length;
				x *= invLength;
				y *= invLength;
			}
		}

	public:
		Vector2Base operator+(const Vector2Base& b) const
		{
			return Vector2Base(x + b.x, y + b.y);
		}

		Vector2Base operator-(const Vector2Base& b) const
		{
			return Vector2Base(x - b.x, y - b.y);
		}

		Vector2Base operator*(const Vector2Base& b) const
		{
			return Vector2Base(x * b.x, y * b.y);
		}

		Vector2Base operator/(const Vector2Base& b) const
		{
			return Vector2Base(x / b.x, y / b.y);
		}

		Vector2Base operator-() const
		{
			return Vector2Base(-x, -y);
		}

		Vector2Base operator+(T b) const
		{
			return Vector2Base(x + b, y + b);
		}

		Vector2Base operator-(T b) const
		{
			return Vector2Base(x - b, y - b);
		}

		Vector2Base operator*(T b) const
		{
			return Vector2Base(x * b, y * b);
		}

		Vector2Base operator/(T b) const
		{
			return Vector2Base(x / b, y / b);
		}

		Vector2Base operator+(typename TOtherFloat<T>::Type a) const
		{
			T b = (T)a;
			return Vector2Base(x + b, y + b);
		}

		Vector2Base operator-(typename TOtherFloat<T>::Type a) const
		{
			T b = (T)a;
			return Vector2Base(x - b, y - b);
		}

		Vector2Base operator*(typename TOtherFloat<T>::Type a) const
		{
			T b = (T)a;
			return Vector2Base(x * b, y * b);
		}

		Vector2Base operator/(typename TOtherFloat<T>::Type a) const
		{
			T b = (T)a;
			return Vector2Base(x / b, y / b);
		}

		Vector2Base& operator+=(const Vector2Base& b)
		{
			x += b.x;
			y += b.y;
			return *this;
		}

		Vector2Base& operator-=(const Vector2Base& b)
		{
			x -= b.x;
			y -= b.y;
			return *this;
		}

		Vector2Base& operator*=(const Vector2Base& b)
		{
			x *= b.x;
			y *= b.y;
			return *this;
		}

		Vector2Base& operator/=(const Vector2Base& b)
		{
			x /= b.x;
			y /= b.y;
			return *this;
		}

		Vector2Base& operator+=(T b)
		{
			x += b;
			y += b;
			return *this;
		}

		Vector2Base& operator-=(T b)
		{
			x -= b;
			y -= b;
			return *this;
		}

		Vector2Base& operator*=(T b)
		{
			x *= b;
			y *= b;
			return *this;
		}

		Vector2Base& operator/=(T b)
		{
			x /= b;
			y /= b;
			return *this;
		}

		bool operator==(const Vector2Base& b) const
		{
			return x == b.x && y == b.y;
		}

		bool operator!=(const Vector2Base& b) const
		{
			return x != b.x || y != b.y;
		}

		bool operator>(const Vector2Base& b) const
		{
			return x > b.x && y > b.y;
		}

		bool operator>=(const Vector2Base& b) const
		{
			return x >= b.x && y >= b.y;
		}

		bool operator<(const Vector2Base& b) const
		{
			return x < b.x && y < b.y;
		}

		bool operator<=(const Vector2Base& b) const
		{
			return x <= b.x && y <= b.y;
		}

	public:
		static bool NearEqual(const Vector2Base& a, const Vector2Base& b)
		{
			return Math::IsNearEqual(a.x, b.x) && Math::IsNearEqual(a.y, b.y);
		}

		static bool NearEqual(const Vector2Base& a, const Vector2Base& b, T epsilon)
		{
			return Math::IsNearEqual(a.x, b.x, epsilon) && Math::IsNearEqual(a.y, b.y, epsilon);
		}

	public:
		static T Dot(const Vector2Base& a, const Vector2Base& b)
		{
			return a.x * b.x + a.y * b.y;
		}

		static T Cross(const Vector2Base& a, const Vector2Base& b)
		{
			return a.x * b.y - a.y * b.x;
		}

		static void Add(const Vector2Base& a, const Vector2Base& b, Vector2Base& result)
		{
			result = Vector2Base(a.x + b.x, a.y + b.y);
		}

		static void Subtract(const Vector2Base& a, const Vector2Base& b, Vector2Base& result)
		{
			result = Vector2Base(a.x - b.x, a.y - b.y);
		}

		static void Multiply(const Vector2Base& a, const Vector2Base& b, Vector2Base& result)
		{
			result = Vector2Base(a.x * b.x, a.y * b.y);
		}

		static void Divide(const Vector2Base& a, const Vector2Base& b, Vector2Base& result)
		{
			result = Vector2Base(a.x / b.x, a.y / b.y);
		}

		static void Min(const Vector2Base& a, const Vector2Base& b, Vector2Base& result)
		{
			result = Vector2Base(a.x < b.x ? a.x : b.x, a.y < b.y ? a.y : b.y);
		}

		static void Max(const Vector2Base& a, const Vector2Base& b, Vector2Base& result)
		{
			result = Vector2Base(a.x > b.x ? a.x : b.x, a.y > b.y ? a.y : b.y);
		}

	public:
		static Vector2Base Min(const Vector2Base& a, const Vector2Base& b)
		{
			return Vector2Base(a.x < b.x ? a.x : b.x, a.y < b.y ? a.y : b.y);
		}

		static Vector2Base Max(const Vector2Base& a, const Vector2Base& b)
		{
			return Vector2Base(a.x > b.x ? a.x : b.x, a.y > b.y ? a.y : b.y);
		}

		static Vector2Base Mod(const Vector2Base& a, const Vector2Base& b)
		{
			return Vector2Base(Math::FMod(a.x, b.x), Math::FMod(a.y, b.y));
		}

		static Vector2Base Floor(const Vector2Base& v)
		{
			return Vector2Base(Math::Floor(v.x), Math::Floor(v.y));
		}

		static Vector2Base Frac(const Vector2Base& v)
		{
			return Vector2Base(v.x - (int32)v.x, v.y - (int32)v.y);
		}

		static Vector2Base Round(const Vector2Base& v)
		{
			return Vector2Base(Math::Round(v.x), Math::Round(v.y));
		}

		static Vector2Base Ceil(const Vector2Base& v)
		{
			return Vector2Base(Math::Ceil(v.x), Math::Ceil(v.y));
		}

		static Vector2Base Abs(const Vector2Base& v)
		{
			return Vector2Base(Math::Abs(v.x), Math::Abs(v.y));
		}

	public:
		// Clamp vector values within given range
		// @param v Vector to clamp
		// @param min Minimum value
		// @param max Maximum value
		// @returns Clamped vector
		static Vector2Base Clamp(const Vector2Base& v, const Vector2Base& min, const Vector2Base& max)
		{
			Vector2Base result;
			Clamp(v, min, max, result);
			return result;
		}

		// Restricts a value to be within a specified range
		// @param v The value to clamp
		// @param min The minimum value,
		// @param max The maximum value
		// @param result When the method completes, contains the clamped value
		static void Clamp(const Vector2Base& v, const Vector2Base& min, const Vector2Base& max, Vector2Base& result)
		{
			result = Vector2Base(Math::Clamp(v.x, min.x, max.x), Math::Clamp(v.y, min.y, max.y));
		}

		// Calculates distance between two points in 2D
		// @param a 1st point
		// @param b 2nd point
		// @returns Distance
		static T Distance(const Vector2Base& a, const Vector2Base& b)
		{
			const T x = a.x - b.x;
			const T y = a.y - b.y;
			return Math::Sqrt(x * x + y * y);
		}

		// Calculates the squared distance between two points in 2D
		// @param a 1st point
		// @param b 2nd point
		// @returns Distance
		static T DistanceSquared(const Vector2Base& a, const Vector2Base& b)
		{
			const T x = a.x - b.x;
			const T y = a.y - b.y;
			return x * x + y * y;
		}

		// Performs vector normalization (scales vector up to unit length).
		static Vector2Base Normalize(const Vector2Base& v)
		{
			Vector2Base r = v;
			const T length = Math::Sqrt(r.x * r.x + r.y * r.y);
			if (length >= Math::ZeroTolerance)
			{
				const T inv = (T)1.0f / length;
				r.x *= inv;
				r.y *= inv;
			}
			return r;
		}

		// Performs a linear interpolation between two vectors
		// @param start Start vector
		// @param end End vector
		// @param amount Value between 0 and 1 indicating the weight of end
		// @param result When the method completes, contains the linear interpolation of the two vectors
		static void Lerp(const Vector2Base& start, const Vector2Base& end, T amount, Vector2Base& result)
		{
			result.x = Math::Lerp(start.x, end.x, amount);
			result.y = Math::Lerp(start.y, end.y, amount);
		}

		// <summary>
		// Performs a linear interpolation between two vectors.
		// </summary>
		// @param start Start vector,
		// @param end End vector,
		// @param amount Value between 0 and 1 indicating the weight of @paramref end"/>,
		// @returns The linear interpolation of the two vectors
		static Vector2Base Lerp(const Vector2Base& start, const Vector2Base& end, T amount)
		{
			Vector2Base result;
			Lerp(start, end, amount, result);
			return result;
		}

	public:
		/// <summary>
		/// Calculates the area of the triangle.
		/// </summary>
		/// <param name="v0">The first triangle vertex.</param>
		/// <param name="v1">The second triangle vertex.</param>
		/// <param name="v2">The third triangle vertex.</param>
		/// <returns>The triangle area.</returns>
		SE_API_CORE static T TriangleArea(const Vector2Base& v0, const Vector2Base& v1, const Vector2Base& v2);

		/// <summary>
		/// Calculates the angle (in radians) between from and to. This is always the smallest value.
		/// </summary>
		/// <param name="from">The first vector.</param>
		/// <param name="to">The second vector.</param>
		/// <returns>The angle (in radians).</returns>
		SE_API_CORE static T Angle(const Vector2Base& from, const Vector2Base& to);
	};

	template<typename T>
	inline Vector2Base<T> operator+(T a, const Vector2Base<T>& b)
	{
		return b + a;
	}

	template<typename T>
	inline Vector2Base<T> operator-(T a, const Vector2Base<T>& b)
	{
		return Vector2Base<T>(a) - b;
	}

	template<typename T>
	inline Vector2Base<T> operator*(T a, const Vector2Base<T>& b)
	{
		return b * a;
	}

	template<typename T>
	inline Vector2Base<T> operator/(T a, const Vector2Base<T>& b)
	{
		return Vector2Base<T>(a) / b;
	}

	template<typename T>
	inline Vector2Base<T> operator+(typename TOtherFloat<T>::Type a, const Vector2Base<T>& b)
	{
		return b + a;
	}

	template<typename T>
	inline Vector2Base<T> operator-(typename TOtherFloat<T>::Type a, const Vector2Base<T>& b)
	{
		return Vector2Base<T>(a) - b;
	}

	template<typename T>
	inline Vector2Base<T> operator*(typename TOtherFloat<T>::Type a, const Vector2Base<T>& b)
	{
		return b * a;
	}

	template<typename T>
	inline Vector2Base<T> operator/(typename TOtherFloat<T>::Type a, const Vector2Base<T>& b)
	{
		return Vector2Base<T>(a) / b;
	}

	template<typename T>
	inline uint32 GetHash(const Vector2Base<T>& key)
	{
		return (*(uint32*)&key.x * 397) ^ *(uint32*)&key.y;
	}
}

template<>
struct TIsPODType<SE::Float2>
{
    enum { Value = true };
};

DEFINE_DEFAULT_FORMATTING(SE::Float2, "X:{0} Y:{1}", v.x, v.y)

template<>
struct TIsPODType<SE::Double2>
{
    enum { Value = true };
};

DEFINE_DEFAULT_FORMATTING(SE::Double2, "X:{0} Y:{1}", v.x, v.y)

template<>
struct TIsPODType<SE::Int2>
{
    enum { Value = true };
};

DEFINE_DEFAULT_FORMATTING(SE::Int2, "X:{0} Y:{1}", v.x, v.y)

#if !defined(_MSC_VER) || defined(__clang__)
// Forward specializations for Clang
template<> SE_API_CORE const SE::Float2 SE::Float2::Zero;
template<> SE_API_CORE const SE::Float2 SE::Float2::One;
template<> SE_API_CORE const SE::Float2 SE::Float2::UnitX;
template<> SE_API_CORE const SE::Float2 SE::Float2::UnitY;
template<> SE_API_CORE const SE::Float2 SE::Float2::Minimum;
template<> SE_API_CORE const SE::Float2 SE::Float2::Maximum;

template<> SE_API_CORE const SE::Double2 SE::Double2::Zero;
template<> SE_API_CORE const SE::Double2 SE::Double2::One;
template<> SE_API_CORE const SE::Double2 SE::Double2::UnitX;
template<> SE_API_CORE const SE::Double2 SE::Double2::UnitY;
template<> SE_API_CORE const SE::Double2 SE::Double2::Minimum;
template<> SE_API_CORE const SE::Double2 SE::Double2::Maximum;

template<> SE_API_CORE const SE::Int2 SE::Int2::Zero;
template<> SE_API_CORE const SE::Int2 SE::Int2::One;
template<> SE_API_CORE const SE::Int2 SE::Int2::UnitX;
template<> SE_API_CORE const SE::Int2 SE::Int2::UnitY;
template<> SE_API_CORE const SE::Int2 SE::Int2::Minimum;
template<> SE_API_CORE const SE::Int2 SE::Int2::Maximum;

#endif
