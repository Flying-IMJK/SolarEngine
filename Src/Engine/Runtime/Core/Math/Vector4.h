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
	SE_STRUCT(Template)
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
				SE_PROPERTY(API)
				T x;

				/// <summary>
				/// The Y component.
				/// </summary>
				SE_PROPERTY(API)
				T y;

				/// <summary>
				/// The Z component.
				/// </summary>
				SE_PROPERTY(API)
				T z;

				/// <summary>
				/// The W component.
				/// </summary>
				SE_PROPERTY(API)
				T w;
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
			: x(xyzw)
			, y(xyzw)
			, z(xyzw)
			, w(xyzw)
		{
		}

		explicit Vector4Base(const T* xyzw)
			: x(xyzw[0])
			, y(xyzw[1])
			, z(xyzw[2])
			, w(xyzw[3])
		{
		}

		Vector4Base(T x, T y, T z, T w)
			: x(x)
			, y(y)
			, z(z)
			, w(w)
		{
		}

		template<typename U = T, typename TEnableIf<TNot<TIsTheSame<T, U>>::Value>::Type...>
		Vector4Base(const Vector4Base<U>& xyzw)
			: x((T)xyzw.x)
			, y((T)xyzw.y)
			, z((T)xyzw.z)
			, w((T)xyzw.w)
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
			return Math::IsZero(x) && Math::IsZero(y) && Math::IsZero(z) && Math::IsZero(w);
		}

		// Gets a value indicting whether any vector component is zero.
		bool IsAnyZero() const
		{
			return Math::IsZero(x) || Math::IsZero(y) || Math::IsZero(z) || Math::IsZero(w);
		}

		// Gets a value indicting whether this vector is one.
		bool IsOne() const
		{
			return Math::IsOne(x) && Math::IsOne(y) && Math::IsOne(z) && Math::IsOne(w);
		}

		/// <summary>
		/// Returns the average arithmetic of all the components.
		/// </summary>
		T AverageArithmetic() const
		{
			return (x + y + z + w) * 0.25f;
		}

		/// <summary>
		/// Gets the sum of all vector components values.
		/// </summary>
		T SumValues() const
		{
			return x + y + z + w;
		}

		/// <summary>
		/// Returns the minimum value of all the components.
		/// </summary>
		T MinValue() const
		{
			return Math::Min(x, y, z, w);
		}

		/// <summary>
		/// Returns the maximum value of all the components.
		/// </summary>
		T MaxValue() const
		{
			return Math::Max(x, y, z, w);
		}

		/// <summary>
		/// Returns true if vector has one or more components is not a number (NaN).
		/// </summary>
		bool IsNaN() const
		{
			return isnan(x) || isnan(y) || isnan(z) || isnan(w);
		}

		/// <summary>
		/// Returns true if vector has one or more components equal to +/- infinity.
		/// </summary>
		bool IsInfinity() const
		{
			return isinf(x) || isinf(y) || isinf(z) || isinf(w);
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
			return Vector4Base(Math::Abs(x), Math::Abs(y), Math::Abs(z), Math::Abs(w));
		}

		/// <summary>
		/// Calculates a vector with values being opposite to values of that vector.
		/// </summary>
		Vector4Base GetNegative() const
		{
			return Vector4Base(-x, -y, -z, -w);
		}

	public:
		Vector4Base operator+(const Vector4Base& b) const
		{
			return Vector4Base(x + b.x, y + b.y, z + b.z, w + b.w);
		}

		Vector4Base operator-(const Vector4Base& b) const
		{
			return Vector4Base(x - b.x, y - b.y, z - b.z, w - b.w);
		}

		Vector4Base operator*(const Vector4Base& b) const
		{
			return Vector4Base(x * b.x, y * b.y, z * b.z, w * b.w);
		}

		Vector4Base operator/(const Vector4Base& b) const
		{
			return Vector4Base(x / b.x, y / b.y, z / b.z, w / b.w);
		}

		Vector4Base operator-() const
		{
			return Vector4Base(-x, -y, -z, -w);
		}

		Vector4Base operator+(T b) const
		{
			return Vector4Base(x + b, y + b, z + b, w + b);
		}

		Vector4Base operator-(T b) const
		{
			return Vector4Base(x - b, y - b, z - b, w - b);
		}

		Vector4Base operator*(T b) const
		{
			return Vector4Base(x * b, y * b, z * b, w * b);
		}

		Vector4Base operator/(T b) const
		{
			return Vector4Base(x / b, y / b, z / b, w / b);
		}

		Vector4Base operator+(typename TOtherFloat<T>::Type a) const
		{
			T b = (T)a;
			return Vector4Base(x + b, y + b, z + b, w + b);
		}

		Vector4Base operator-(typename TOtherFloat<T>::Type a) const
		{
			T b = (T)a;
			return Vector4Base(x - b, y - b, z - b, w - b);
		}

		Vector4Base operator*(typename TOtherFloat<T>::Type a) const
		{
			T b = (T)a;
			return Vector4Base(x * b, y * b, z * b, w * b);
		}

		Vector4Base operator/(typename TOtherFloat<T>::Type a) const
		{
			T b = (T)a;
			return Vector4Base(x / b, y / b, z / b, w / b);
		}

		Vector4Base& operator+=(const Vector4Base& b)
		{
			x += b.x;
			y += b.y;
			z += b.z;
			w += b.w;
			return *this;
		}

		Vector4Base& operator-=(const Vector4Base& b)
		{
			x -= b.x;
			y -= b.y;
			z -= b.z;
			w -= b.w;
			return *this;
		}

		Vector4Base& operator*=(const Vector4Base& b)
		{
			x *= b.x;
			y *= b.y;
			z *= b.z;
			w *= b.w;
			return *this;
		}

		Vector4Base& operator/=(const Vector4Base& b)
		{
			x /= b.x;
			y /= b.y;
			z /= b.z;
			w /= b.w;
			return *this;
		}

		Vector4Base& operator+=(T b)
		{
			x += b;
			y += b;
			z += b;
			w += b;
			return *this;
		}

		Vector4Base& operator-=(T b)
		{
			x -= b;
			y -= b;
			z -= b;
			w -= b;
			return *this;
		}

		Vector4Base& operator*=(T b)
		{
			x *= b;
			y *= b;
			z *= b;
			w *= b;
			return *this;
		}

		Vector4Base& operator/=(T b)
		{
			x /= b;
			y /= b;
			z /= b;
			w /= b;
			return *this;
		}

		bool operator==(const Vector4Base& b) const
		{
			return x == b.x && y == b.y && z == b.z && w == b.w;
		}

		bool operator!=(const Vector4Base& b) const
		{
			return x != b.x || y != b.y || z != b.z || w != b.w;
		}

		bool operator>(const Vector4Base& b) const
		{
			return x > b.x && y > b.y && z > b.z && w > b.w;
		}

		bool operator>=(const Vector4Base& b) const
		{
			return x >= b.x && y >= b.y && z >= b.z && w >= b.w;
		}

		bool operator<(const Vector4Base& b) const
		{
			return x < b.x && y < b.y && z < b.z && w < b.w;
		}

		bool operator<=(const Vector4Base& b) const
		{
			return x <= b.x && y <= b.y && z <= b.z && w <= b.w;
		}

	public:
		static bool NearEqual(const Vector4Base& a, const Vector4Base& b)
		{
			return Math::IsNearEqual(a.x, b.x) && Math::IsNearEqual(a.y, b.y) && Math::IsNearEqual(a.z, b.z) && Math::IsNearEqual(a.w, b.w);
		}

		static bool NearEqual(const Vector4Base& a, const Vector4Base& b, T epsilon)
		{
			return Math::IsNearEqual(a.x, b.x, epsilon) && Math::IsNearEqual(a.y, b.y, epsilon) && Math::IsNearEqual(a.z, b.z, epsilon) && Math::IsNearEqual(a.w, b.w, epsilon);
		}

	public:
		static void Add(const Vector4Base& a, const Vector4Base& b, Vector4Base& result)
		{
			result = Vector4Base(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w);
		}

		static void Subtract(const Vector4Base& a, const Vector4Base& b, Vector4Base& result)
		{
			result = Vector4Base(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w);
		}

		static void Multiply(const Vector4Base& a, const Vector4Base& b, Vector4Base& result)
		{
			result = Vector4Base(a.x * b, a.y * b, a.z * b, a.w * b);
		}

		static void Divide(const Vector4Base& a, const Vector4Base& b, Vector4Base& result)
		{
			result = Vector4Base(a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w);
		}

	public:
		static Vector4Base Mod(const Vector4Base& a, const Vector4Base& b)
		{
			return Vector4Base(Math::FMod(a.x, b.x), Math::FMod(a.y, b.y), Math::FMod(a.z, b.z), Math::FMod(a.w, b.w));
		}

		static Vector4Base Floor(const Vector4Base& v)
		{
			return Vector4Base(Math::Floor(v.x), Math::Floor(v.y), Math::Floor(v.z), Math::Floor(v.w));
		}

		static Vector4Base Frac(const Vector4Base& v)
		{
			return Vector4Base(v.x - (int32)v.x, v.y - (int32)v.y, v.z - (int32)v.z, v.w - (int32)v.w);
		}

		static Vector4Base Round(const Vector4Base& v)
		{
			return Vector4Base(Math::Round(v.x), Math::Round(v.y), Math::Round(v.z), Math::Round(v.w));
		}

		static Vector4Base Ceil(const Vector4Base& v)
		{
			return Vector4Base(Math::Ceil(v.x), Math::Ceil(v.y), Math::Ceil(v.z), Math::Ceil(v.w));
		}

		static Vector4Base Abs(const Vector4Base& v)
		{
			return Vector4Base(Math::Abs(v.x), Math::Abs(v.y), Math::Abs(v.z), Math::Abs(v.w));
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
			result = Vector4Base(Math::Clamp(v.x, min.x, max.x), Math::Clamp(v.y, min.y, max.y), Math::Clamp(v.z, min.z, max.z), Math::Clamp(v.w, min.w, max.w));
		}

		// Performs a linear interpolation between two vectors
		// @param start Start vector
		// @param end End vector
		// @param amount Value between 0 and 1 indicating the weight of end
		// @param result When the method completes, contains the linear interpolation of the two vectors
		static void Lerp(const Vector4Base& start, const Vector4Base& end, T amount, Vector4Base& result)
		{
			result.x = Math::Lerp(start.x, end.x, amount);
			result.y = Math::Lerp(start.y, end.y, amount);
			result.z = Math::Lerp(start.z, end.z, amount);
			result.w = Math::Lerp(start.w, end.w, amount);
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
		return (((((*(uint32*)&key.x * 397) ^ *(uint32*)&key.y) * 397) ^ *(uint32*)&key.z) * 397) ^*(uint32*)&key.w;
	}
}

template<>
struct TIsPODType<SE::Float4>
{
    enum { Value = true };
};

DEFINE_DEFAULT_FORMATTING(SE::Float4, "X:{0} Y:{1} Z:{2} W:{3}", v.x, v.y, v.z, v.w);

template<>
struct TIsPODType<SE::Double4>
{
    enum { Value = true };
};

DEFINE_DEFAULT_FORMATTING(SE::Double4, "X:{0} Y:{1} Z:{2} W:{3}", v.x, v.y, v.z, v.w)

template<>
struct TIsPODType<SE::Int4>
{
    enum { Value = true };
};

DEFINE_DEFAULT_FORMATTING(SE::Int4, "X:{0} Y:{1} Z:{2} W:{3}", v.x, v.y, v.z, v.w);

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
