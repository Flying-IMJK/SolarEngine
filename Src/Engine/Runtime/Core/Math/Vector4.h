#pragma once

#include "Math.h"
#include "Runtime/Core/Formatting.h"
#include "Runtime/Core/Types/Variable.h"
#include "Runtime/Core/TypeSystem/TypeMacro.h"

namespace SE
{
	/// <summary>
	/// Represents a four dimensional mathematical vector with 32-bit precision (per-component).
	/// </summary>
    SE_STRUCT(Template, API())
	template<typename T>
	struct Vector4Base
	{
		static SE_API_RUNTIME struct ScriptingTypeInitializer TypeInitializer;
		union
		{
			struct
			{
				/// <summary>
				/// The X component.
				/// </summary>
                SE_FIELD(API())
				T X;

				/// <summary>
				/// The Y component.
				/// </summary>
                SE_FIELD(API())
				T Y;

				/// <summary>
				/// The Z component.
				/// </summary>
                SE_FIELD(API())
				T Z;

				/// <summary>
				/// The W component.
				/// </summary>
                SE_FIELD(API())
				T W;
			};

			/// <summary>
			/// The raw vector values (in XYZW order).
			/// </summary>
			T Raw[4];
		};

	public:
		// Vector with all components equal 0
		static SE_API_RUNTIME const Vector4Base<T> Zero;

		// Vector with all components equal 1
		static SE_API_RUNTIME const Vector4Base<T> One;

		// Vector with all components equal 0.5
		static SE_API_RUNTIME const Vector4Base<T> Half;

		// Vector X=1, Y=0, Z=0, W=0
		static SE_API_RUNTIME const Vector4Base<T> UnitX;

		// Vector X=0, Y=1, Z=0, W=0
		static SE_API_RUNTIME const Vector4Base<T> UnitY;

		// Vector X=0, Y=0, Z=1, W=0
		static SE_API_RUNTIME const Vector4Base<T> UnitZ;

		// Vector X=0, Y=0, Z=0, W=1
		static SE_API_RUNTIME const Vector4Base<T> UnitW;

		// Vector with all components equal maximum value.
		static SE_API_RUNTIME const Vector4Base<T> Minimum;

		// Vector with all components equal minimum value.
		static SE_API_RUNTIME const Vector4Base<T> Maximum;

	public:
		/// <summary>
		/// Empty constructor.
		/// </summary>
		Vector4Base() = default;

		Vector4Base(T xyzw)
			: X(xyzw)
			, Y(xyzw)
			, Z(xyzw)
			, W(xyzw)
		{
		}

		explicit Vector4Base(const T* xyzw)
			: X(xyzw[0])
			, Y(xyzw[1])
			, Z(xyzw[2])
			, W(xyzw[3])
		{
		}

		Vector4Base(T x, T y, T z, T w)
			: X(x)
			, Y(y)
			, Z(z)
			, W(w)
		{
		}

		template<typename U = T, typename TEnableIf<TNot<TIsTheSame<T, U>>::Value>::Type...>
		Vector4Base(const Vector4Base<U>& xyzw)
			: X((T)xyzw.X)
			, Y((T)xyzw.Y)
			, Z((T)xyzw.Z)
			, W((T)xyzw.W)
		{
		}

		SE_API_RUNTIME explicit Vector4Base(const Float2& xy, T z = 0, T w = 0);
		SE_API_RUNTIME explicit Vector4Base(const Float2& xy, const Float2& zw);
		SE_API_RUNTIME explicit Vector4Base(const Float3& xyz, T w = 0);
		SE_API_RUNTIME explicit Vector4Base(const Int2& xy, T z = 0, T w = 0);
		SE_API_RUNTIME explicit Vector4Base(const Int3& xyz, T w = 0);
		SE_API_RUNTIME explicit Vector4Base(const Double2& xy, T z = 0, T w = 0);
		SE_API_RUNTIME explicit Vector4Base(const Double2& xy, const Double2& zw);
		SE_API_RUNTIME explicit Vector4Base(const Double3& xyz, T w = 0);
		SE_API_RUNTIME explicit Vector4Base(const Color& color);
		SE_API_RUNTIME explicit Vector4Base(const Rectangle& rect);

	public:
		SE_API_RUNTIME String ToString() const;

	public:
		// Gets a value indicting whether this vector is zero.
		bool IsZero() const
		{
			return Math::IsZero(X) && Math::IsZero(Y) && Math::IsZero(Z) && Math::IsZero(W);
		}

		// Gets a value indicting whether any vector component is zero.
		bool IsAnyZero() const
		{
			return Math::IsZero(X) || Math::IsZero(Y) || Math::IsZero(Z) || Math::IsZero(W);
		}

		// Gets a value indicting whether this vector is one.
		bool IsOne() const
		{
			return Math::IsOne(X) && Math::IsOne(Y) && Math::IsOne(Z) && Math::IsOne(W);
		}

		/// <summary>
		/// Returns the average arithmetic of all the components.
		/// </summary>
		T AverageArithmetic() const
		{
			return (X + Y + Z + W) * 0.25f;
		}

		/// <summary>
		/// Gets the sum of all vector components values.
		/// </summary>
		T SumValues() const
		{
			return X + Y + Z + W;
		}

		/// <summary>
		/// Returns the minimum value of all the components.
		/// </summary>
		T MinValue() const
		{
			return Math::Min(X, Y, Z, W);
		}

		/// <summary>
		/// Returns the maximum value of all the components.
		/// </summary>
		T MaxValue() const
		{
			return Math::Max(X, Y, Z, W);
		}

		/// <summary>
		/// Returns true if vector has one or more components is not a number (NaN).
		/// </summary>
		bool IsNaN() const
		{
			return isnan(X) || isnan(Y) || isnan(Z) || isnan(W);
		}

		/// <summary>
		/// Returns true if vector has one or more components equal to +/- infinity.
		/// </summary>
		bool IsInfinity() const
		{
			return isinf(X) || isinf(Y) || isinf(Z) || isinf(W);
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
		Vector4Base GetAbsolute() const
		{
			return Vector4Base(Math::Abs(X), Math::Abs(Y), Math::Abs(Z), Math::Abs(W));
		}

		/// <summary>
		/// Calculates a vector with values being opposite to values of that vector.
		/// </summary>
		Vector4Base GetNegative() const
		{
			return Vector4Base(-X, -Y, -Z, -W);
		}

	public:
		Vector4Base operator+(const Vector4Base& b) const
		{
			return Vector4Base(X + b.X, Y + b.Y, Z + b.Z, W + b.W);
		}

		Vector4Base operator-(const Vector4Base& b) const
		{
			return Vector4Base(X - b.X, Y - b.Y, Z - b.Z, W - b.W);
		}

		Vector4Base operator*(const Vector4Base& b) const
		{
			return Vector4Base(X * b.X, Y * b.Y, Z * b.Z, W * b.W);
		}

		Vector4Base operator/(const Vector4Base& b) const
		{
			return Vector4Base(X / b.X, Y / b.Y, Z / b.Z, W / b.W);
		}

		Vector4Base operator-() const
		{
			return Vector4Base(-X, -Y, -Z, -W);
		}

		Vector4Base operator+(T b) const
		{
			return Vector4Base(X + b, Y + b, Z + b, W + b);
		}

		Vector4Base operator-(T b) const
		{
			return Vector4Base(X - b, Y - b, Z - b, W - b);
		}

		Vector4Base operator*(T b) const
		{
			return Vector4Base(X * b, Y * b, Z * b, W * b);
		}

		Vector4Base operator/(T b) const
		{
			return Vector4Base(X / b, Y / b, Z / b, W / b);
		}

		Vector4Base operator+(typename TOtherFloat<T>::Type a) const
		{
			T b = (T)a;
			return Vector4Base(X + b, Y + b, Z + b, W + b);
		}

		Vector4Base operator-(typename TOtherFloat<T>::Type a) const
		{
			T b = (T)a;
			return Vector4Base(X - b, Y - b, Z - b, W - b);
		}

		Vector4Base operator*(typename TOtherFloat<T>::Type a) const
		{
			T b = (T)a;
			return Vector4Base(X * b, Y * b, Z * b, W * b);
		}

		Vector4Base operator/(typename TOtherFloat<T>::Type a) const
		{
			T b = (T)a;
			return Vector4Base(X / b, Y / b, Z / b, W / b);
		}

		Vector4Base& operator+=(const Vector4Base& b)
		{
			X += b.X;
			Y += b.Y;
			Z += b.Z;
			W += b.W;
			return *this;
		}

		Vector4Base& operator-=(const Vector4Base& b)
		{
			X -= b.X;
			Y -= b.Y;
			Z -= b.Z;
			W -= b.W;
			return *this;
		}

		Vector4Base& operator*=(const Vector4Base& b)
		{
			X *= b.X;
			Y *= b.Y;
			Z *= b.Z;
			W *= b.W;
			return *this;
		}

		Vector4Base& operator/=(const Vector4Base& b)
		{
			X /= b.X;
			Y /= b.Y;
			Z /= b.Z;
			W /= b.W;
			return *this;
		}

		Vector4Base& operator+=(T b)
		{
			X += b;
			Y += b;
			Z += b;
			W += b;
			return *this;
		}

		Vector4Base& operator-=(T b)
		{
			X -= b;
			Y -= b;
			Z -= b;
			W -= b;
			return *this;
		}

		Vector4Base& operator*=(T b)
		{
			X *= b;
			Y *= b;
			Z *= b;
			W *= b;
			return *this;
		}

		Vector4Base& operator/=(T b)
		{
			X /= b;
			Y /= b;
			Z /= b;
			W /= b;
			return *this;
		}

		bool operator==(const Vector4Base& b) const
		{
			return X == b.X && Y == b.Y && Z == b.Z && W == b.W;
		}

		bool operator!=(const Vector4Base& b) const
		{
			return X != b.X || Y != b.Y || Z != b.Z || W != b.W;
		}

		bool operator>(const Vector4Base& b) const
		{
			return X > b.X && Y > b.Y && Z > b.Z && W > b.W;
		}

		bool operator>=(const Vector4Base& b) const
		{
			return X >= b.X && Y >= b.Y && Z >= b.Z && W >= b.W;
		}

		bool operator<(const Vector4Base& b) const
		{
			return X < b.X && Y < b.Y && Z < b.Z && W < b.W;
		}

		bool operator<=(const Vector4Base& b) const
		{
			return X <= b.X && Y <= b.Y && Z <= b.Z && W <= b.W;
		}

	public:
		static bool NearEqual(const Vector4Base& a, const Vector4Base& b)
		{
			return Math::IsNearEqual(a.X, b.X) && Math::IsNearEqual(a.Y, b.Y) && Math::IsNearEqual(a.Z, b.Z) && Math::IsNearEqual(a.W, b.W);
		}

		static bool NearEqual(const Vector4Base& a, const Vector4Base& b, T epsilon)
		{
			return Math::IsNearEqual(a.X, b.X, epsilon) && Math::IsNearEqual(a.Y, b.Y, epsilon) && Math::IsNearEqual(a.Z, b.Z, epsilon) && Math::IsNearEqual(a.W, b.W, epsilon);
		}

	public:
		static void Add(const Vector4Base& a, const Vector4Base& b, Vector4Base& result)
		{
			result = Vector4Base(a.X + b.X, a.Y + b.Y, a.Z + b.Z, a.W + b.W);
		}

		static void Subtract(const Vector4Base& a, const Vector4Base& b, Vector4Base& result)
		{
			result = Vector4Base(a.X - b.X, a.Y - b.Y, a.Z - b.Z, a.W - b.W);
		}

		static void Multiply(const Vector4Base& a, const Vector4Base& b, Vector4Base& result)
		{
			result = Vector4Base(a.X * b, a.Y * b, a.Z * b, a.W * b);
		}

		static void Divide(const Vector4Base& a, const Vector4Base& b, Vector4Base& result)
		{
			result = Vector4Base(a.X / b.X, a.Y / b.Y, a.Z / b.Z, a.W / b.W);
		}

	public:
		static Vector4Base Mod(const Vector4Base& a, const Vector4Base& b)
		{
			return Vector4Base(Math::FMod(a.X, b.X), Math::FMod(a.Y, b.Y), Math::FMod(a.Z, b.Z), Math::FMod(a.W, b.W));
		}

		static Vector4Base Floor(const Vector4Base& v)
		{
			return Vector4Base(Math::Floor(v.X), Math::Floor(v.Y), Math::Floor(v.Z), Math::Floor(v.W));
		}

		static Vector4Base Frac(const Vector4Base& v)
		{
			return Vector4Base(v.X - (int32)v.X, v.Y - (int32)v.Y, v.Z - (int32)v.Z, v.W - (int32)v.W);
		}

		static Vector4Base Round(const Vector4Base& v)
		{
			return Vector4Base(Math::Round(v.X), Math::Round(v.Y), Math::Round(v.Z), Math::Round(v.W));
		}

		static Vector4Base Ceil(const Vector4Base& v)
		{
			return Vector4Base(Math::Ceil(v.X), Math::Ceil(v.Y), Math::Ceil(v.Z), Math::Ceil(v.W));
		}

		static Vector4Base Abs(const Vector4Base& v)
		{
			return Vector4Base(Math::Abs(v.X), Math::Abs(v.Y), Math::Abs(v.Z), Math::Abs(v.W));
		}

	public:
		// Restricts a value to be within a specified range
		// @param v The value to clamp
		// @param min The minimum value,
		// @param max The maximum value
		// @returns Clamped value
		static Vector4Base Clamp(const Vector4Base& v, const Vector4Base& min, const Vector4Base& max)
		{
			Vector4Base result;
			Clamp(v, min, max, result);
			return result;
		}

		// Restricts a value to be within a specified range
		// @param v The value to clamp
		// @param min The minimum value,
		// @param max The maximum value
		// @param result When the method completes, contains the clamped value
		static void Clamp(const Vector4Base& v, const Vector4Base& min, const Vector4Base& max, Vector4Base& result)
		{
			result = Vector4Base(Math::Clamp(v.X, min.X, max.X), Math::Clamp(v.Y, min.Y, max.Y), Math::Clamp(v.Z, min.Z, max.Z), Math::Clamp(v.W, min.W, max.W));
		}

		// Performs a linear interpolation between two vectors
		// @param start Start vector
		// @param end End vector
		// @param amount Value between 0 and 1 indicating the weight of end
		// @param result When the method completes, contains the linear interpolation of the two vectors
		static void Lerp(const Vector4Base& start, const Vector4Base& end, T amount, Vector4Base& result)
		{
			result.X = Math::Lerp(start.X, end.X, amount);
			result.Y = Math::Lerp(start.Y, end.Y, amount);
			result.Z = Math::Lerp(start.Z, end.Z, amount);
			result.W = Math::Lerp(start.W, end.W, amount);
		}

		// <summary>
		// Performs a linear interpolation between two vectors.
		// </summary>
		// @param start Start vector,
		// @param end End vector,
		// @param amount Value between 0 and 1 indicating the weight of @paramref end"/>,
		// @returns The linear interpolation of the two vectors
		static Vector4Base Lerp(const Vector4Base& start, const Vector4Base& end, T amount)
		{
			Vector4Base result;
			Lerp(start, end, amount, result);
			return result;
		}

		SE_API_RUNTIME static Vector4Base Transform(const Vector4Base& v, const Matrix& m);
	};

	template<typename T>
	inline Vector4Base<T> operator+(T a, const Vector4Base<T>& b)
	{
		return b + a;
	}

	template<typename T>
	inline Vector4Base<T> operator-(T a, const Vector4Base<T>& b)
	{
		return Vector4Base<T>(a) - b;
	}

	template<typename T>
	inline Vector4Base<T> operator*(T a, const Vector4Base<T>& b)
	{
		return b * a;
	}

	template<typename T>
	inline Vector4Base<T> operator/(T a, const Vector4Base<T>& b)
	{
		return Vector4Base<T>(a) / b;
	}

	template<typename T>
	inline Vector4Base<T> operator+(typename TOtherFloat<T>::Type a, const Vector4Base<T>& b)
	{
		return b + a;
	}

	template<typename T>
	inline Vector4Base<T> operator-(typename TOtherFloat<T>::Type a, const Vector4Base<T>& b)
	{
		return Vector4Base<T>(a) - b;
	}

	template<typename T>
	inline Vector4Base<T> operator*(typename TOtherFloat<T>::Type a, const Vector4Base<T>& b)
	{
		return b * a;
	}

	template<typename T>
	inline Vector4Base<T> operator/(typename TOtherFloat<T>::Type a, const Vector4Base<T>& b)
	{
		return Vector4Base<T>(a) / b;
	}

	template<typename T>
	inline uint32 GetHash(const Vector4Base<T>& key)
	{
		return (((((*(uint32*)&key.X * 397) ^ *(uint32*)&key.Y) * 397) ^ *(uint32*)&key.Z) * 397) ^*(uint32*)&key.W;
	}
}

template<>
struct TIsPODType<SE::Float4>
{
    enum { Value = true };
};

DEFINE_DEFAULT_FORMATTING(SE::Float4, "X:{0} Y:{1} Z:{2} W:{3}", v.X, v.Y, v.Z, v.W);

template<>
struct TIsPODType<SE::Double4>
{
    enum { Value = true };
};

DEFINE_DEFAULT_FORMATTING(SE::Double4, "X:{0} Y:{1} Z:{2} W:{3}", v.X, v.Y, v.Z, v.W)

template<>
struct TIsPODType<SE::Int4>
{
    enum { Value = true };
};

DEFINE_DEFAULT_FORMATTING(SE::Int4, "X:{0} Y:{1} Z:{2} W:{3}", v.X, v.Y, v.Z, v.W);

#if !defined(_MSC_VER) || defined(__clang__)
// Forward specializations for Clang
template<> SE_API_RUNTIME const SE::Float4 SE::Float4::Zero;
template<> SE_API_RUNTIME const SE::Float4 SE::Float4::One;
template<> SE_API_RUNTIME const SE::Float4 SE::Float4::UnitX;
template<> SE_API_RUNTIME const SE::Float4 SE::Float4::UnitY;
template<> SE_API_RUNTIME const SE::Float4 SE::Float4::UnitZ;
template<> SE_API_RUNTIME const SE::Float4 SE::Float4::UnitW;
template<> SE_API_RUNTIME const SE::Float4 SE::Float4::Minimum;
template<> SE_API_RUNTIME const SE::Float4 SE::Float4::Maximum;

template<> SE_API_RUNTIME const SE::Double4 SE::Double4::Zero;
template<> SE_API_RUNTIME const SE::Double4 SE::Double4::One;
template<> SE_API_RUNTIME const SE::Double4 SE::Double4::UnitX;
template<> SE_API_RUNTIME const SE::Double4 SE::Double4::UnitY;
template<> SE_API_RUNTIME const SE::Double4 SE::Double4::UnitZ;
template<> SE_API_RUNTIME const SE::Double4 SE::Double4::UnitW;
template<> SE_API_RUNTIME const SE::Double4 SE::Double4::Minimum;
template<> SE_API_RUNTIME const SE::Double4 SE::Double4::Maximum;

template<> SE_API_RUNTIME const SE::Int4 SE::Int4::Zero;
template<> SE_API_RUNTIME const SE::Int4 SE::Int4::One;
template<> SE_API_RUNTIME const SE::Int4 SE::Int4::UnitX;
template<> SE_API_RUNTIME const SE::Int4 SE::Int4::UnitY;
template<> SE_API_RUNTIME const SE::Int4 SE::Int4::UnitZ;
template<> SE_API_RUNTIME const SE::Int4 SE::Int4::UnitW;
template<> SE_API_RUNTIME const SE::Int4 SE::Int4::Minimum;
template<> SE_API_RUNTIME const SE::Int4 SE::Int4::Maximum;
#endif
