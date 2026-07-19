#pragma once

#include "Runtime/Core/Types/Variable.h"
#include "Runtime/Core/Platform/Compiler.h"

#include <algorithm>
#include <cmath>
#include <limits>


namespace SE::Math
{
	enum class AngleUnit
	{
		AU_DEGREE,
		AU_RADIAN
	};

	static constexpr AngleUnit k_AngleUnit = AngleUnit::AU_DEGREE;
	static constexpr float POS_INFINITY = std::numeric_limits<float>::infinity();
	static constexpr float NEG_INFINITY = -std::numeric_limits<float>::infinity();
	static constexpr float PI = 3.14159265358979323846264338327950288f;
	static constexpr float ONE_OVER_PI = 1.0f / PI;
	static constexpr float TWO_PI = 2.0f * PI;
	static constexpr float HALF_PI = 0.5f * PI;
	static constexpr float FDeg2Rad = PI / 180.0f;
	static constexpr float FRad2Deg = 180.0f / PI;
	static constexpr float PiDivTwo = PI / 2;
	static constexpr float PiDivFour = PI / 4;
	static constexpr float EPSILON = 1e-6f;
	// The value for which all absolute numbers smaller than are considered equal to zero.
	static constexpr float ZeroTolerance = 1e-6f;
	static constexpr float ZeroToleranceDouble = 1e-16;

	// Converts radians to degrees.
	static constexpr float  RadiansToDegrees (180.0f / PI);
	// Converts degrees to radians.
	static constexpr float  DegreesToRadians (PI / 180.0f);

	static constexpr float Float_EPSILON = FLT_EPSILON;
	static constexpr float Double_EPSILON = DBL_EPSILON;

	FORCE_INLINE bool Cmp(float x, float y)
	{
		return fabsf(x - y) < FLT_EPSILON * fmaxf(1.0f, fmaxf(fabsf(x), fabsf(y)));
	};

	FORCE_INLINE float Abs(float a) { return fabsf(a); }
	FORCE_INLINE double Abs(double a) { return fabs(a); }
	FORCE_INLINE int8 Abs(int8 a) { return (int8)abs(a); }
	FORCE_INLINE int16 Abs(int16 a) { return (int16)abs(a); }
	FORCE_INLINE int32 Abs(int32 a) { return labs(a); }
	FORCE_INLINE int64 Abs(int64 a) { return llabs(a); }

	FORCE_INLINE bool IsNan(float f) { return std::isnan(f); }

	FORCE_INLINE float Sqrt(float fValue) { return std::sqrt(fValue); }

	FORCE_INLINE float InvSqrt(float value) { return 1.f / Sqrt(value); }

	FORCE_INLINE bool IsNearEqual(double value, double comparand, double epsilon = EPSILON)
	{
		return fabs(value - comparand) <= epsilon;
	}

	FORCE_INLINE bool IsZero(float a)
	{
		return Abs(a) < ZeroTolerance;
	}

	// Determines whether the specified value is close to one (1.0)
	// @param a The integer value
	// @returns True if the specified value is close to one (1.0). otherwise false
	FORCE_INLINE bool IsOne(int32 a)
	{
		return a == 1;
	}

	// Determines whether the specified value is close to one (1.0f)
	// @param a The floating value
	// @returns True if the specified value is close to one (1.0f). otherwise false
	FORCE_INLINE bool IsOne(float a)
	{
		return IsZero(a - 1.0f);
	}

	// Performs smooth (cubic Hermite) interpolation between 0 and 1
	// @param amount Value between 0 and 1 indicating interpolation amount
	FORCE_INLINE float SmoothStep(float amount)
	{
		return amount <= 0 ? 0 : amount >= 1 ? 1 : amount * amount * (3 - 2 * amount);
	}

	template <typename T>
	FORCE_INLINE T Min(T a, T b) { return std::min(a, b);}

	template <typename T>
	FORCE_INLINE T Min(T a, T b, T c) { return std::max(std::max(a, b), c);}

	template <typename T>
	FORCE_INLINE T Max(T a, T b) { return std::max(a, b);}

	template <typename T>
	FORCE_INLINE T Max(T a, T b, T c) { return std::max(std::max(a, b), c);}

	template <typename T>
	FORCE_INLINE T AbsMin(T a, T b) { return Abs(a) <= Abs(b) ? a : b; }

	template <typename T>
	FORCE_INLINE T AbsMax(T a, T b) { return Abs(a) >= Abs(b) ? a : b; }

	template <typename T>
	FORCE_INLINE T Clamp(T value, T lowerBound, T upperBound)
	{
		if (lowerBound > upperBound)
		{
			return value;
		}
		return Min(Max(value, lowerBound), upperBound);
	}

	template<class T>
	FORCE_INLINE T Saturate(const T value)
	{
		return value < 0 ? 0 : value < 1 ? value : 1;
	}

	template <typename T>
	FORCE_INLINE bool RangeInclusive(T value, T lowerBound, T upperBound)
	{
		if (lowerBound > upperBound)
		{
			return true;
		}
		return value >= lowerBound && value <= upperBound;
	}

	template <typename T>
	FORCE_INLINE bool RangeExclusive(T value, T lowerBound, T upperBound)
	{
		if (lowerBound > upperBound)
		{
			return true;
		}
		return value < lowerBound || value > upperBound;
	}

	template <typename T>
	FORCE_INLINE T Lerp(T A, T B, float t) { return A + (B - A) * t; }

	FORCE_INLINE bool IsNearZero(float value, float epsilon = EPSILON)
	{
		return fabsf(value) <= epsilon;
	}

	FORCE_INLINE float Trunc(float value) { return trunc(value); }

	// Decomposes a float into integer and remainder portions, remainder is return and the integer result is stored in the integer portion
	FORCE_INLINE float ModF(float value, float &integerPortion) { return modff(value, &integerPortion); }

	// Returns the floating point remainder of x/y
	FORCE_INLINE float FMod(float x, float y) { return fmodf(x, y); }

	FORCE_INLINE int32 FMod(int32 x, int32 y) { return (int32)fmodf(x, y); }

	FORCE_INLINE float Ceil(float value) { return ceilf(value); }

	FORCE_INLINE int32 CeilToInt(float value) { return (int32)ceilf(value); }

	FORCE_INLINE float Floor(float value) { return floorf(value); }

	FORCE_INLINE double Floor(double value) { return floor(value); }

	FORCE_INLINE int32 FloorToInt(float value) { return (int32)floorf(value); }

	FORCE_INLINE float Round(float value) { return roundf(value); }

	FORCE_INLINE int32 RoundToInt(float value) { return (int32)roundf(value); }

	FORCE_INLINE float Log(float value) { return logf(value); }
	FORCE_INLINE float Log2f(float value) { return log2f(value); }

	// Note: returns true for 0
	FORCE_INLINE bool IsPowerOf2(int32 x) { return (x & (x - 1)) == 0; }

	FORCE_INLINE float Exp(const float value)
	{
		return expf(value);
	}

	FORCE_INLINE float Exp2(const float value)
	{
		return exp2f(value);
	}

	// 是否为奇数
	template<typename T>
	FORCE_INLINE bool IsOdd( T n )
	{
		static_assert(std::numeric_limits<T>::is_integer, "Integer type required");
		return n % 2 != 0;
	}

	// 是否为偶数
	template<typename T>
	FORCE_INLINE bool IsEven( T n )
	{
		static_assert(std::numeric_limits<T>::is_integer, "Integer type required.");
		return n % 2 == 0;
	}

	/// <summary>
	/// Returns signed fractional part of a float.
	/// </summary>
	/// <param name="value">Floating point value to convert.</param>
	/// <returns>A float between [0 ; 1) for nonnegative input. A float between [-1; 0) for negative input.</returns>
	FORCE_INLINE float Fractional(float value)
	{
		return value - Trunc(value);
	}

	/// <summary>
	/// Aligns value up to match desire alignment.
	/// </summary>
	/// <param name="value">The value.</param>
	/// <param name="alignment">The alignment.</param>
	/// <returns>Aligned value (multiple of alignment).</returns>
	template<typename T>
	T AlignUp(T value, T alignment)
	{
		T mask = alignment - 1;
		return (T)(value + mask & ~mask);
	}

	/// <summary>
	/// Aligns value down to match desire alignment.
	/// </summary>
	/// <param name="value">The value.</param>
	/// <param name="alignment">The alignment.</param>
	/// <returns>Aligned value (multiple of alignment).</returns>
	template<typename T>
	T AlignDown(T value, T alignment)
	{
		T mask = alignment - 1;
		return (T)(value & ~mask);
	}

	// Given a heading which may be outside the +/- PI range, 'unwind' it back into that range
	template<typename T>
	static T UnwindRadians(T a)
	{
		while (a > PI)
			a -= TWO_PI;
		while (a < -PI)
			a += TWO_PI;
		return a;
	}

	// Utility to ensure angle is between +/- 180 degrees by unwinding
	template<typename T>
	static T UnwindDegrees(T a)
	{
		while (a > 180)
			a -= 360;
		while (a < -180)
			a += 360;
		return a;
	}

	// Round floating point value up to 1 decimal place
	template<typename T>
	FORCE_INLINE T RoundTo1DecimalPlace(T value)
	{
		return (T)round((double)value * 10) / (T)10;
	}

	// Round floating point value up to 2 decimal places
	template<typename T>
	FORCE_INLINE T RoundTo2DecimalPlaces(T value)
	{
		return (T)round((double)value * 100.0) / (T)100;
	}

	// Round floating point value up to 3 decimal places
	template<typename T>
	FORCE_INLINE T RoundTo3DecimalPlaces(T value)
	{
		return (T)round((double)value * 1000.0) / (T)1000;
	}

	FORCE_INLINE uint32 GetClosestPowerOfTwo(uint32 x)
	{
		uint32 i = 1;
		while (i < x)
			i += i;
		if (4 * x < 3 * i)
			i >>= 1;
		return i;
	}

	FORCE_INLINE uint32 GetUpperPowerOfTwo(uint32 x)
	{
		uint32 i = 1;
		while (i < x)
			i += i;
		return i;
	}

	FORCE_INLINE uint32 GetLowerPowerOfTwo(uint32 x)
	{
		uint32 i = 1;
		while (i <= x)
			i += i;
		return i >> 1;
	}

	FORCE_INLINE int32 GetUpperPowerOfTwo(int32 x)
	{
		uint32 i = 1;
		while (i < x)
			i += i;
		return i;
	}

	FORCE_INLINE int32 GetLowerPowerOfTwo(int32 x)
	{
		uint32 i = 1;
		while (i <= x)
			i += i;
		return i >> 1;
	}

	FORCE_INLINE uint64 NextPowerOfTwo(uint64 x)
	{
		--x;
		x |= x >> 1;
		x |= x >> 2;
		x |= x >> 4;
		x |= x >> 8;
		x |= x >> 16;
		return ++x;
	}

	FORCE_INLINE uint32 NextPowerOfTwo(uint32 x)
	{
		--x;
		x |= x >> 1;
		x |= x >> 2;
		x |= x >> 4;
		x |= x >> 8;
		x |= x >> 16;
		return ++x;
	}

	FORCE_INLINE int64 NextPowerOfTwo(int64 x)
	{
		--x;
		x |= x >> 1;
		x |= x >> 2;
		x |= x >> 4;
		x |= x >> 8;
		x |= x >> 16;
		return ++x;
	}

	FORCE_INLINE int32 NextPowerOfTwo(int32 x)
	{
		--x;
		x |= x >> 1;
		x |= x >> 2;
		x |= x >> 4;
		x |= x >> 8;
		x |= x >> 16;
		return ++x;
	}

	// 返回32位整数 二进制为设置的个数
	FORCE_INLINE int32 CountBits(uint32 x)
	{
		// [Reference: https://stackoverflow.com/questions/109023/how-to-count-the-number-of-set-bits-in-a-32-bit-integer]
#ifdef __GNUC_
		return __builtin_popcount(x);
#elif _MSC_VER && PLATFORM_SIMD_SSE4_2
		return __popcnt(x);
#else
		x = x - ((x >> 1) & 0x55555555);
		x = (x & 0x33333333) + ((x >> 2) & 0x33333333);
		x = (x + (x >> 4)) & 0x0F0F0F0F;
		return (x * 0x01010101) >> 24;
#endif
	}

	// Divides two integers and rounds up
	template<class T>
	FORCE_INLINE T DivideAndRoundUp(T dividend, T divisor)
	{
		return (dividend + divisor - 1) / divisor;
	}

	template<class T>
	FORCE_INLINE T DivideAndRoundDown(T dividend, T divisor)
	{
		return dividend / divisor;
	}

	// Returns a value indicating the sign of a number
	// @returns A number that indicates the sign of value
	FORCE_INLINE float Sign(float v)
	{
		return v > 0.0f ? 1.0f : v < 0.0f ? -1.0f : 0.0f;
	}

	FORCE_INLINE float Sin(float value) { return sinf(value); }
	FORCE_INLINE float Cos(float value) { return cosf(value); }
	FORCE_INLINE float Tan(float value) { return tanf(value); }

	FORCE_INLINE float ASin(float value) { return asinf(value); }
	FORCE_INLINE float ACos(float value) { return acosf(value); }
	FORCE_INLINE float ATan(float value) { return atanf(value); }
	FORCE_INLINE float ATan2(float y, float x) { return atan2f(y, x); }

	FORCE_INLINE float Cosec(float value) { return 1.0f / sinf(value); }
	FORCE_INLINE float Sec(float value) { return 1.0f / cosf(value); }
	FORCE_INLINE float Cot(float value) { return 1.0f / tanf(PiDivTwo - value); }

	FORCE_INLINE float Remap(float value, float fromMin, float fromMax, float toMin, float toMax)
	{
		return (value - fromMin) / (fromMax - fromMin) * (toMax - toMin) + toMin;
	}

	// x ^ y
	FORCE_INLINE float Pow(float x, float y) { return powf(x, y); }
}
