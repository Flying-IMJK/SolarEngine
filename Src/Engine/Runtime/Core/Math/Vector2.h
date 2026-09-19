#pragma once

#include "Math.h"
#include "Runtime/Core/Formatting.h"
#include "Runtime/Core/Templates.h"
#include "Runtime/Core/TypeSystem/IType.h"
#include "Runtime/Core/Types/Variable.h"

namespace SE
{
	/// <summary>
	/// Represents a two dimensional mathematical vector.
	/// </summary>
    SE_STRUCT(Template, API())
	template<typename T>
	struct Vector2Base
	{
		typedef T Real;
		static SE_API_RUNTIME struct ScriptingTypeInitializer TypeInitializer;

		union
		{
			struct
			{
				/// <summary>
				/// The X component of the vector.
				/// </summary>
                SE_FIELD(API())
				T X;

				/// <summary>
				/// The Y component of the vector.
				/// </summary>
                SE_FIELD(API())
				T Y;
			};

			/// <summary>
			/// The raw vector values (in XY order).
			/// </summary>
			T Raw[2];
		};

	public:
		// Vector with all components equal 0
		static SE_API_RUNTIME const Vector2Base<T> Zero;

		// Vector with all components equal 1
		static SE_API_RUNTIME const Vector2Base<T> One;

		// Vector with all components equal 0.5
		static SE_API_RUNTIME const Vector2Base<T> Half;

		// Vector X=1, Y=0
		static SE_API_RUNTIME const Vector2Base<T> UnitX;

		// Vector X=0, Y=1
		static SE_API_RUNTIME const Vector2Base<T> UnitY;

		// Vector with all components equal maximum value.
		static SE_API_RUNTIME const Vector2Base<T> Minimum;

		// Vector with all components equal minimum value.
		static SE_API_RUNTIME const Vector2Base<T> Maximum;

	public:
		/// <summary>
		/// Empty constructor.
		/// </summary>
		Vector2Base() = default;

		inline Vector2Base(T xy)
			: X(xy)
			, Y(xy)
		{
		}

		inline explicit Vector2Base(const T* xy)
			: X(xy[0])
			, Y(xy[1])
		{
		}

		inline Vector2Base(T x, T y)
			: X(x)
			, Y(y)
		{
		}

		template<typename U = T, typename TEnableIf<TNot<TIsTheSame<T, U>>::Value>::Type...>
		inline Vector2Base(const Vector2Base<U>& xy)
			: X((T)xy.X)
			, Y((T)xy.Y)
		{
		}

		SE_API_RUNTIME explicit Vector2Base(const Int3& xy);
		SE_API_RUNTIME explicit Vector2Base(const Int4& xy);
		SE_API_RUNTIME explicit Vector2Base(const Float3& xy);
		SE_API_RUNTIME explicit Vector2Base(const Float4& xy);
		SE_API_RUNTIME explicit Vector2Base(const Double3& xy);
		SE_API_RUNTIME explicit Vector2Base(const Double4& xy);
		SE_API_RUNTIME explicit Vector2Base(const Color& color);

	public:
		SE_API_RUNTIME String ToString() const;

	public:
		// Gets a value indicting whether this instance is normalized.
		bool IsNormalized() const
		{
			return Math::IsOne(X * X + Y * Y);
		}

		// Gets a value indicting whether this vector is zero.
		bool IsZero() const
		{
			return Math::IsZero(X) && Math::IsZero(Y);
		}

		// Gets a value indicting whether any vector component is zero.
		bool IsAnyZero() const
		{
			return Math::IsZero(X) || Math::IsZero(Y);
		}

		// Gets a value indicting whether this vector is zero.
		bool IsOne() const
		{
			return Math::IsOne(X) && Math::IsOne(Y);
		}

		// Calculates the length of the vector.
		T Length() const
		{
			return Math::Sqrt(X * X + Y * Y);
		}

		// Calculates the squared length of the vector.
		T LengthSquared() const
		{
			return X * X + Y * Y;
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
			return (X + Y) * 0.5f;
		}

		/// <summary>
		/// Gets the sum of all vector components values.
		/// </summary>
		T SumValues() const
		{
			return X + Y;
		}

		/// <summary>
		/// Gets the multiplication result of all vector components values.
		/// </summary>
		T MulValues() const
		{
			return X * Y;
		}

		/// <summary>
		/// Returns the minimum value of all the components.
		/// </summary>
		T MinValue() const
		{
			return Math::Min(X, Y);
		}

		/// <summary>
		/// Returns the maximum value of all the components.
		/// </summary>
		T MaxValue() const
		{
			return Math::Max(X, Y);
		}

		/// <summary>
		/// Returns true if vector has one or more components is not a number (NaN).
		/// </summary>
		bool IsNaN() const
		{
			return isnan(X) || isnan(Y);
		}

		/// <summary>
		/// Returns true if vector has one or more components equal to +/- infinity.
		/// </summary>
		bool IsInfinity() const
		{
			return isinf(X) || isinf(Y);
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
			return Vector2Base(Math::Abs(X), Math::Abs(Y));
		}

		/// <summary>
		/// Calculates a vector with values being opposite to values of that vector.
		/// </summary>
		Vector2Base GetNegative() const
		{
			return Vector2Base(-X, -Y);
		}

		/// <summary>
		/// Calculates a normalized vector that has length equal to 1.
		/// </summary>
		Vector2Base GetNormalized() const
		{
			Vector2Base result(X, Y);
			result.Normalize();
			return result;
		}

	public:
		/// <summary>
		/// Performs vector normalization (scales vector up to unit length).
		/// </summary>
		void Normalize()
		{
			const T length = Math::Sqrt(X * X + Y * Y);
			if (length >= Math::ZeroTolerance)
			{
				const T invLength = (T)1.0f / length;
				X *= invLength;
				Y *= invLength;
			}
		}

	public:
		Vector2Base operator+(const Vector2Base& b) const
		{
			return Vector2Base(X + b.X, Y + b.Y);
		}

		Vector2Base operator-(const Vector2Base& b) const
		{
			return Vector2Base(X - b.X, Y - b.Y);
		}

		Vector2Base operator*(const Vector2Base& b) const
		{
			return Vector2Base(X * b.X, Y * b.Y);
		}

		Vector2Base operator/(const Vector2Base& b) const
		{
			return Vector2Base(X / b.X, Y / b.Y);
		}

		Vector2Base operator-() const
		{
			return Vector2Base(-X, -Y);
		}

		Vector2Base operator+(T b) const
		{
			return Vector2Base(X + b, Y + b);
		}

		Vector2Base operator-(T b) const
		{
			return Vector2Base(X - b, Y - b);
		}

		Vector2Base operator*(T b) const
		{
			return Vector2Base(X * b, Y * b);
		}

		Vector2Base operator/(T b) const
		{
			return Vector2Base(X / b, Y / b);
		}

		Vector2Base operator+(typename TOtherFloat<T>::Type a) const
		{
			T b = (T)a;
			return Vector2Base(X + b, Y + b);
		}

		Vector2Base operator-(typename TOtherFloat<T>::Type a) const
		{
			T b = (T)a;
			return Vector2Base(X - b, Y - b);
		}

		Vector2Base operator*(typename TOtherFloat<T>::Type a) const
		{
			T b = (T)a;
			return Vector2Base(X * b, Y * b);
		}

		Vector2Base operator/(typename TOtherFloat<T>::Type a) const
		{
			T b = (T)a;
			return Vector2Base(X / b, Y / b);
		}

		Vector2Base& operator+=(const Vector2Base& b)
		{
			X += b.X;
			Y += b.Y;
			return *this;
		}

		Vector2Base& operator-=(const Vector2Base& b)
		{
			X -= b.X;
			Y -= b.Y;
			return *this;
		}

		Vector2Base& operator*=(const Vector2Base& b)
		{
			X *= b.X;
			Y *= b.Y;
			return *this;
		}

		Vector2Base& operator/=(const Vector2Base& b)
		{
			X /= b.X;
			Y /= b.Y;
			return *this;
		}

		Vector2Base& operator+=(T b)
		{
			X += b;
			Y += b;
			return *this;
		}

		Vector2Base& operator-=(T b)
		{
			X -= b;
			Y -= b;
			return *this;
		}

		Vector2Base& operator*=(T b)
		{
			X *= b;
			Y *= b;
			return *this;
		}

		Vector2Base& operator/=(T b)
		{
			X /= b;
			Y /= b;
			return *this;
		}

		bool operator==(const Vector2Base& b) const
		{
			return X == b.X && Y == b.Y;
		}

		bool operator!=(const Vector2Base& b) const
		{
			return X != b.X || Y != b.Y;
		}

		bool operator>(const Vector2Base& b) const
		{
			return X > b.X && Y > b.Y;
		}

		bool operator>=(const Vector2Base& b) const
		{
			return X >= b.X && Y >= b.Y;
		}

		bool operator<(const Vector2Base& b) const
		{
			return X < b.X && Y < b.Y;
		}

		bool operator<=(const Vector2Base& b) const
		{
			return X <= b.X && Y <= b.Y;
		}

	public:
		static bool NearEqual(const Vector2Base& a, const Vector2Base& b)
		{
			return Math::IsNearEqual(a.X, b.X) && Math::IsNearEqual(a.Y, b.Y);
		}

		static bool NearEqual(const Vector2Base& a, const Vector2Base& b, T epsilon)
		{
			return Math::IsNearEqual(a.X, b.X, epsilon) && Math::IsNearEqual(a.Y, b.Y, epsilon);
		}

	public:
		static T Dot(const Vector2Base& a, const Vector2Base& b)
		{
			return a.X * b.X + a.Y * b.Y;
		}

		static T Cross(const Vector2Base& a, const Vector2Base& b)
		{
			return a.X * b.Y - a.Y * b.X;
		}

		static void Add(const Vector2Base& a, const Vector2Base& b, Vector2Base& result)
		{
			result = Vector2Base(a.X + b.X, a.Y + b.Y);
		}

		static void Subtract(const Vector2Base& a, const Vector2Base& b, Vector2Base& result)
		{
			result = Vector2Base(a.X - b.X, a.Y - b.Y);
		}

		static void Multiply(const Vector2Base& a, const Vector2Base& b, Vector2Base& result)
		{
			result = Vector2Base(a.X * b.X, a.Y * b.Y);
		}

		static void Divide(const Vector2Base& a, const Vector2Base& b, Vector2Base& result)
		{
			result = Vector2Base(a.X / b.X, a.Y / b.Y);
		}

		static void Min(const Vector2Base& a, const Vector2Base& b, Vector2Base& result)
		{
			result = Vector2Base(a.X < b.X ? a.X : b.X, a.Y < b.Y ? a.Y : b.Y);
		}

		static void Max(const Vector2Base& a, const Vector2Base& b, Vector2Base& result)
		{
			result = Vector2Base(a.X > b.X ? a.X : b.X, a.Y > b.Y ? a.Y : b.Y);
		}

	public:
		static Vector2Base Min(const Vector2Base& a, const Vector2Base& b)
		{
			return Vector2Base(a.X < b.X ? a.X : b.X, a.Y < b.Y ? a.Y : b.Y);
		}

		static Vector2Base Max(const Vector2Base& a, const Vector2Base& b)
		{
			return Vector2Base(a.X > b.X ? a.X : b.X, a.Y > b.Y ? a.Y : b.Y);
		}

		static Vector2Base Mod(const Vector2Base& a, const Vector2Base& b)
		{
			return Vector2Base(Math::FMod(a.X, b.X), Math::FMod(a.Y, b.Y));
		}

		static Vector2Base Floor(const Vector2Base& v)
		{
			return Vector2Base(Math::Floor(v.X), Math::Floor(v.Y));
		}

		static Vector2Base Frac(const Vector2Base& v)
		{
			return Vector2Base(v.X - (int32)v.X, v.Y - (int32)v.Y);
		}

		static Vector2Base Round(const Vector2Base& v)
		{
			return Vector2Base(Math::Round(v.X), Math::Round(v.Y));
		}

		static Vector2Base Ceil(const Vector2Base& v)
		{
			return Vector2Base(Math::Ceil(v.X), Math::Ceil(v.Y));
		}

		static Vector2Base Abs(const Vector2Base& v)
		{
			return Vector2Base(Math::Abs(v.X), Math::Abs(v.Y));
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
			result = Vector2Base(Math::Clamp(v.X, min.X, max.X), Math::Clamp(v.Y, min.Y, max.Y));
		}

		// Calculates distance between two points in 2D
		// @param a 1st point
		// @param b 2nd point
		// @returns Distance
		static T Distance(const Vector2Base& a, const Vector2Base& b)
		{
			const T x = a.X - b.X;
			const T y = a.Y - b.Y;
			return Math::Sqrt(x * x + y * y);
		}

		// Calculates the squared distance between two points in 2D
		// @param a 1st point
		// @param b 2nd point
		// @returns Distance
		static T DistanceSquared(const Vector2Base& a, const Vector2Base& b)
		{
			const T x = a.X - b.X;
			const T y = a.Y - b.Y;
			return x * x + y * y;
		}

		// Performs vector normalization (scales vector up to unit length).
		static Vector2Base Normalize(const Vector2Base& v)
		{
			Vector2Base r = v;
			const T length = Math::Sqrt(r.X * r.X + r.Y * r.Y);
			if (length >= Math::ZeroTolerance)
			{
				const T inv = (T)1.0f / length;
				r.X *= inv;
				r.Y *= inv;
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
			result.X = Math::Lerp(start.X, end.X, amount);
			result.Y = Math::Lerp(start.Y, end.Y, amount);
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
		SE_API_RUNTIME static T TriangleArea(const Vector2Base& v0, const Vector2Base& v1, const Vector2Base& v2);

		/// <summary>
		/// Calculates the angle (in radians) between from and to. This is always the smallest value.
		/// </summary>
		/// <param name="from">The first vector.</param>
		/// <param name="to">The second vector.</param>
		/// <returns>The angle (in radians).</returns>
		SE_API_RUNTIME static T Angle(const Vector2Base& from, const Vector2Base& to);
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
		return (*(uint32*)&key.X * 397) ^ *(uint32*)&key.Y;
	}

}

template<>
struct TIsPODType<SE::Float2>
{
    enum { Value = true };
};

DEFINE_DEFAULT_FORMATTING(SE::Float2, "X:{0} Y:{1}", v.X, v.Y)

template<>
struct TIsPODType<SE::Double2>
{
    enum { Value = true };
};

DEFINE_DEFAULT_FORMATTING(SE::Double2, "X:{0} Y:{1}", v.X, v.Y)

template<>
struct TIsPODType<SE::Int2>
{
    enum { Value = true };
};

DEFINE_DEFAULT_FORMATTING(SE::Int2, "X:{0} Y:{1}", v.X, v.Y)

#if !defined(_MSC_VER) || defined(__clang__)
// Forward specializations for Clang
template<> SE_API_RUNTIME const SE::Float2 SE::Float2::Zero;
template<> SE_API_RUNTIME const SE::Float2 SE::Float2::One;
template<> SE_API_RUNTIME const SE::Float2 SE::Float2::UnitX;
template<> SE_API_RUNTIME const SE::Float2 SE::Float2::UnitY;
template<> SE_API_RUNTIME const SE::Float2 SE::Float2::Minimum;
template<> SE_API_RUNTIME const SE::Float2 SE::Float2::Maximum;

template<> SE_API_RUNTIME const SE::Double2 SE::Double2::Zero;
template<> SE_API_RUNTIME const SE::Double2 SE::Double2::One;
template<> SE_API_RUNTIME const SE::Double2 SE::Double2::UnitX;
template<> SE_API_RUNTIME const SE::Double2 SE::Double2::UnitY;
template<> SE_API_RUNTIME const SE::Double2 SE::Double2::Minimum;
template<> SE_API_RUNTIME const SE::Double2 SE::Double2::Maximum;

template<> SE_API_RUNTIME const SE::Int2 SE::Int2::Zero;
template<> SE_API_RUNTIME const SE::Int2 SE::Int2::One;
template<> SE_API_RUNTIME const SE::Int2 SE::Int2::UnitX;
template<> SE_API_RUNTIME const SE::Int2 SE::Int2::UnitY;
template<> SE_API_RUNTIME const SE::Int2 SE::Int2::Minimum;
template<> SE_API_RUNTIME const SE::Int2 SE::Int2::Maximum;

#endif
