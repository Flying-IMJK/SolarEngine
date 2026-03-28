#pragma once

#include "Math.h"
#include "Core/Utilities/Formatting.h"
#include "Core/Templates.h"

namespace SE
{
	struct Matrix3x3;

	/// <summary>
	/// Represents a three dimensional mathematical vector.
	/// </summary>
	template<typename T>
	struct Vector3Base
	{
		union
		{
			struct
			{
				/// <summary>
				/// The X component.
				/// </summary>
				T x;

				/// <summary>
				/// The Y component.
				/// </summary>
				T y;

				/// <summary>
				/// The Z component.
				/// </summary>
				T z;
			};

			/// <summary>
			/// The raw vector values (in XYZ order).
			/// </summary>
			T Raw[3];
		};

	public:
		// Vector with all components equal zero (0, 0, 0).
		static SE_API_CORE const Vector3Base<T> Zero;

		// Vector with all components equal one (1, 1, 1).
		static SE_API_CORE const Vector3Base<T> One;

		// Vector with all components equal half (0.5, 0.5, 0.5).
		static SE_API_CORE const Vector3Base<T> Half;

		// The X unit vector (1, 0, 0).
		static SE_API_CORE const Vector3Base<T> UnitX;

		// The Y unit vector (0, 1, 0).
		static SE_API_CORE const Vector3Base<T> UnitY;

		// The Z unit vector (0, 0, 1).
		static SE_API_CORE const Vector3Base<T> UnitZ;

		// A unit vector designating up (0, 1, 0).
		static SE_API_CORE const Vector3Base<T> Up;

		// A unit vector designating down (0, -1, 0).
		static SE_API_CORE const Vector3Base<T> Down;

		// A unit vector designating a (-1, 0, 0).
		static SE_API_CORE const Vector3Base<T> Left;

		// A unit vector designating b (1, 0, 0).
		static SE_API_CORE const Vector3Base<T> Right;

		// A unit vector designating forward in a a-handed coordinate system (0, 0, 1).
		static SE_API_CORE const Vector3Base<T> Forward;

		// A unit vector designating backward in a a-handed coordinate system (0, 0, -1).
		static SE_API_CORE const Vector3Base<T> Backward;

		// Vector with all components equal maximum value.
		static SE_API_CORE const Vector3Base<T> Minimum;

		// Vector with all components equal minimum value.
		static SE_API_CORE const Vector3Base<T> Maximum;

	public:
		/// <summary>
		/// Empty constructor.
		/// </summary>
		Vector3Base() = default;

	    Vector3Base(T xyz)
			: x(xyz)
			, y(xyz)
			, z(xyz)
		{
		}

		explicit Vector3Base(const T* xyz)
			: x(xyz[0])
			, y(xyz[1])
			, z(xyz[2])
		{
		}

		Vector3Base(T x, T y, T z)
			: x(x)
			, y(y)
			, z(z)
		{
		}

		template<typename U = T, typename TEnableIf<TNot<TIsTheSame<T, U>>::Value>::Type...>
		Vector3Base(const Vector3Base<U>& xyz)
			: x((T)xyz.x)
			, y((T)xyz.y)
			, z((T)xyz.z)
		{
		}

		SE_API_CORE Vector3Base(const Float2& xy, T z = 0);
		SE_API_CORE Vector3Base(const Double2& xy, T z = 0);
		SE_API_CORE Vector3Base(const Int2& xy, T z = 0);
		SE_API_CORE Vector3Base(const Int3& xyz);
		SE_API_CORE explicit Vector3Base(const Float4& xyz);
		SE_API_CORE explicit Vector3Base(const Double4& xyz);
		SE_API_CORE explicit Vector3Base(const Int4& xyz);
		SE_API_CORE explicit Vector3Base(const Color& color);

	public:
		SE_API_CORE String ToString() const;

	public:
		// Gets a value indicting whether this instance is normalized.
		bool IsNormalized() const
		{
			return Math::IsOne(x * x + y * y + z * z);
		}

		// Gets a value indicting whether this vector is zero.
		bool IsZero() const
		{
			return Math::IsZero(x) && Math::IsZero(y) && Math::IsZero(z);
		}

		// Gets a value indicting whether any vector component is zero.
		bool IsAnyZero() const
		{
			return Math::IsZero(x) || Math::IsZero(y) || Math::IsZero(z);
		}

		// Gets a value indicting whether this vector is one.
		bool IsOne() const
		{
			return Math::IsOne(x) && Math::IsOne(y) && Math::IsOne(z);
		}

		// Calculates the length of the vector.
		T Length() const
		{
			return Math::Sqrt(x * x + y * y + z * z);
		}

		// Calculates the squared length of the vector.
		T LengthSquared() const
		{
			return x * x + y * y + z * z;
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
			return (x + y + z) * 0.333333334f;
		}

		/// <summary>
		/// Gets the sum of all vector components values.
		/// </summary>
		T SumValues() const
		{
			return x + y + z;
		}

		/// <summary>
		/// Returns the minimum value of all the components.
		/// </summary>
		T MinValue() const
		{
			return Math::Min(x, y, z);
		}

		/// <summary>
		/// Returns the maximum value of all the components.
		/// </summary>
		T MaxValue() const
		{
			return Math::Max(x, y, z);
		}

		/// <summary>
		/// Returns true if vector has one or more components is not a number (NaN).
		/// </summary>
		bool IsNaN() const
		{
			return isnan(x) || isnan(y) || isnan(z);
		}

		/// <summary>
		/// Returns true if vector has one or more components equal to +/- infinity.
		/// </summary>
		bool IsInfinity() const
		{
			return isinf(x) || isinf(y) || isinf(z);
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
		Vector3Base GetAbsolute() const
		{
			return Vector3Base(Math::Abs(x), Math::Abs(y), Math::Abs(z));
		}

		/// <summary>
		/// Calculates a vector with values being opposite to values of that vector.
		/// </summary>
		Vector3Base GetNegative() const
		{
			return Vector3Base(-x, -y, -z);
		}

		/// <summary>
		/// Calculates a normalized vector that has length equal to 1.
		/// </summary>
		Vector3Base GetNormalized() const
		{
			Vector3Base result(x, y, z);
			result.Normalize();
			return result;
		}

	public:
		/// <summary>
		/// Performs vector normalization (scales vector up to unit length).
		/// </summary>
		void Normalize()
		{
			const T length = Math::Sqrt(x * x + y * y + z * z);
			if (length >= Math::ZeroTolerance)
			{
				const T inv = (T)1.0f / length;
				x *= inv;
				y *= inv;
				z *= inv;
			}
		}

		/// <summary>
		/// Performs fast vector normalization (scales vector up to unit length).
		/// </summary>
		void NormalizeFast()
		{
			const T inv = 1.0f / Math::Sqrt(x * x + y * y + z * z);
			x *= inv;
			y *= inv;
			z *= inv;
		}

	public:
		Vector3Base operator+(const Vector3Base& b) const
		{
			return Vector3Base(x + b.x, y + b.y, z + b.z);
		}

		Vector3Base operator-(const Vector3Base& b) const
		{
			return Vector3Base(x - b.x, y - b.y, z - b.z);
		}

		Vector3Base operator*(const Vector3Base& b) const
		{
			return Vector3Base(x * b.x, y * b.y, z * b.z);
		}

		Vector3Base operator/(const Vector3Base& b) const
		{
			return Vector3Base(x / b.x, y / b.y, z / b.z);
		}

		Vector3Base operator-() const
		{
			return Vector3Base(-x, -y, -z);
		}

		Vector3Base operator+(T b) const
		{
			return Vector3Base(x + b, y + b, z + b);
		}

		Vector3Base operator-(T b) const
		{
			return Vector3Base(x - b, y - b, z - b);
		}

		Vector3Base operator*(T b) const
		{
			return Vector3Base(x * b, y * b, z * b);
		}

		Vector3Base operator/(T b) const
		{
			return Vector3Base(x / b, y / b, z / b);
		}

		Vector3Base operator+(typename TOtherFloat<T>::Type a) const
		{
			T b = (T)a;
			return Vector3Base(x + b, y + b, z + b);
		}

		Vector3Base operator-(typename TOtherFloat<T>::Type a) const
		{
			T b = (T)a;
			return Vector3Base(x - b, y - b, z - b);
		}

		Vector3Base operator*(typename TOtherFloat<T>::Type a) const
		{
			T b = (T)a;
			return Vector3Base(x * (T)b, y * b, z * b);
		}

		Vector3Base operator/(typename TOtherFloat<T>::Type a) const
		{
			T b = (T)a;
			return Vector3Base(x / b, y / b, z / b);
		}

		Vector3Base operator^(const Vector3Base& b) const
		{
			return Cross(*this, b);
		}

		T operator|(const Vector3Base& b) const
		{
			return Dot(*this, b);
		}

		Vector3Base& operator+=(const Vector3Base& b)
		{
			x += b.x;
			y += b.y;
			z += b.z;
			return *this;
		}

		Vector3Base& operator-=(const Vector3Base& b)
		{
			x -= b.x;
			y -= b.y;
			z -= b.z;
			return *this;
		}

		Vector3Base& operator*=(const Vector3Base& b)
		{
			x *= b.x;
			y *= b.y;
			z *= b.z;
			return *this;
		}

		Vector3Base& operator/=(const Vector3Base& b)
		{
			x /= b.x;
			y /= b.y;
			z /= b.z;
			return *this;
		}

		Vector3Base& operator+=(T b)
		{
			x += b;
			y += b;
			z += b;
			return *this;
		}

		Vector3Base& operator-=(T b)
		{
			x -= b;
			y -= b;
			z -= b;
			return *this;
		}

		Vector3Base& operator*=(T b)
		{
			x *= b;
			y *= b;
			z *= b;
			return *this;
		}

		Vector3Base& operator/=(T b)
		{
			x /= b;
			y /= b;
			z /= b;
			return *this;
		}

		bool operator==(const Vector3Base& b) const
		{
			return x == b.x && y == b.y && z == b.z;
		}

		bool operator!=(const Vector3Base& b) const
		{
			return x != b.x || y != b.y || z != b.z;
		}

		bool operator>(const Vector3Base& b) const
		{
			return x > b.x && y > b.y && z > b.z;
		}

		bool operator>=(const Vector3Base& b) const
		{
			return x >= b.x && y >= b.y && z >= b.z;
		}

		bool operator<(const Vector3Base& b) const
		{
			return x < b.x && y < b.y && z < b.z;
		}

		bool operator<=(const Vector3Base& b) const
		{
			return x <= b.x && y <= b.y && z <= b.z;
		}

	public:
		static bool NearEqual(const Vector3Base& a, const Vector3Base& b)
		{
			return Math::IsNearEqual(a.x, b.x) && Math::IsNearEqual(a.y, b.y) && Math::IsNearEqual(a.z, b.z);
		}

		static bool NearEqual(const Vector3Base& a, const Vector3Base& b, T epsilon)
		{
			return Math::IsNearEqual(a.x, b.x, epsilon) && Math::IsNearEqual(a.y, b.y, epsilon) && Math::IsNearEqual(a.z, b.z, epsilon);
		}

	public:
		static void Add(const Vector3Base& a, const Vector3Base& b, Vector3Base& result)
		{
			result = Vector3Base(a.x + b.x, a.y + b.y, a.z + b.z);
		}

		static void Subtract(const Vector3Base& a, const Vector3Base& b, Vector3Base& result)
		{
			result = Vector3Base(a.x - b.x, a.y - b.y, a.z - b.z);
		}

		static void Multiply(const Vector3Base& a, const Vector3Base& b, Vector3Base& result)
		{
			result = Vector3Base(a.x * b.x, a.y * b.y, a.z * b.z);
		}

		static void Divide(const Vector3Base& a, const Vector3Base& b, Vector3Base& result)
		{
			result = Vector3Base(a.x / b.x, a.y / b.y, a.z / b.z);
		}

		static void Min(const Vector3Base& a, const Vector3Base& b, Vector3Base& result)
		{
			result = Vector3Base(a.x < b.x ? a.x : b.x, a.y < b.y ? a.y : b.y, a.z < b.z ? a.z : b.z);
		}

		static void Max(const Vector3Base& a, const Vector3Base& b, Vector3Base& result)
		{
			result = Vector3Base(a.x > b.x ? a.x : b.x, a.y > b.y ? a.y : b.y, a.z > b.z ? a.z : b.z);
		}

	public:
		static Vector3Base Min(const Vector3Base& a, const Vector3Base& b)
		{
			return Vector3Base(a.x < b.x ? a.x : b.x, a.y < b.y ? a.y : b.y, a.z < b.z ? a.z : b.z);
		}

		static Vector3Base Max(const Vector3Base& a, const Vector3Base& b)
		{
			return Vector3Base(a.x > b.x ? a.x : b.x, a.y > b.y ? a.y : b.y, a.z > b.z ? a.z : b.z);
		}

		static Vector3Base Mod(const Vector3Base& a, const Vector3Base& b)
		{
			return Vector3Base(Math::FMod(a.x, b.x), Math::FMod(a.y, b.y), Math::FMod(a.z, b.z));
		}

		static Vector3Base Floor(const Vector3Base& v)
		{
			return Vector3Base(Math::Floor(v.x), Math::Floor(v.y), Math::Floor(v.z));
		}

		static Vector3Base Frac(const Vector3Base& v)
		{
			return Vector3Base(v.x - (int32)v.x, v.y - (int32)v.y, v.z - (int32)v.z);
		}

		static Vector3Base Round(const Vector3Base& v)
		{
			return Vector3Base(Math::Round(v.x), Math::Round(v.y), Math::Round(v.z));
		}

		static Vector3Base Ceil(const Vector3Base& v)
		{
			return Vector3Base(Math::Ceil(v.x), Math::Ceil(v.y), Math::Ceil(v.z));
		}

		static Vector3Base Abs(const Vector3Base& v)
		{
			return Vector3Base(Math::Abs(v.x), Math::Abs(v.y), Math::Abs(v.z));
		}

	public:
		// Restricts a value to be within a specified range
		// @param v The value to clamp
		// @param min The minimum value,
		// @param max The maximum value
		// @returns Clamped value
		static Vector3Base Clamp(const Vector3Base& v, const Vector3Base& min, const Vector3Base& max)
		{
			Vector3Base result;
			Clamp(v, min, max, result);
			return result;
		}

		// Restricts a value to be within a specified range
		// @param v The value to clamp
		// @param min The minimum value,
		// @param max The maximum value
		// @param result When the method completes, contains the clamped value
		static void Clamp(const Vector3Base& v, const Vector3Base& min, const Vector3Base& max, Vector3Base& result)
		{
			result = Vector3Base(Math::Clamp(v.x, min.x, max.x), Math::Clamp(v.y, min.y, max.y), Math::Clamp(v.z, min.z, max.z));
		}

		/// <summary>
		/// Makes sure that Length of the output vector is always below max and above 0.
		/// </summary>
		/// <param name="v">Input Vector.</param>
		/// <param name="max">Max Length</param>
		static Vector3Base ClampLength(const Vector3Base& v, float max)
		{
			return ClampLength(v, 0, max);
		}

		/// <summary>
		/// Makes sure that Length of the output vector is always below max and above min.
		/// </summary>
		/// <param name="v">Input Vector.</param>
		/// <param name="min">Min Length</param>
		/// <param name="max">Max Length</param>
		static Vector3Base ClampLength(const Vector3Base& v, float min, float max)
		{
			Vector3Base result;
			ClampLength(v, min, max, result);
			return result;
		}

		/// <summary>
		/// Makes sure that Length of the output vector is always below max and above min.
		/// </summary>
		/// <param name="v">Input Vector.</param>
		/// <param name="min">Min Length</param>
		/// <param name="max">Max Length</param>
		/// <param name="result">The result vector.</param>
		static void ClampLength(const Vector3Base& v, float min, float max, Vector3Base& result)
		{
			result = v;
			T lenSq = result.LengthSquared();
			if (lenSq > max * max)
			{
				T scaleFactor = max / (T)Math::Sqrt(lenSq);
				result.x *= scaleFactor;
				result.y *= scaleFactor;
				result.z *= scaleFactor;
			}
			if (lenSq < min * min)
			{
				T scaleFactor = min / (T)Math::Sqrt(lenSq);
				result.x *= scaleFactor;
				result.y *= scaleFactor;
				result.z *= scaleFactor;
			}
		}

		// Calculates the distance between two vectors
		// @param a The first vector
		// @param b The second vector
		// @returns The distance between the two vectors
		static T Distance(const Vector3Base& a, const Vector3Base& b)
		{
			const T x = a.x - b.x;
			const T y = a.y - b.y;
			const T z = a.z - b.z;
			return Math::Sqrt(x * x + y * y + z * z);
		}

		// Calculates the squared distance between two vectors
		// @param a The first vector
		// @param b The second vector
		// @returns The squared distance between the two vectors
		static T DistanceSquared(const Vector3Base& a, const Vector3Base& b)
		{
			const T x = a.x - b.x;
			const T y = a.y - b.y;
			const T z = a.z - b.z;
			return x * x + y * y + z * z;
		}

		// Performs vector normalization (scales vector up to unit length).
		static Vector3Base Normalize(const Vector3Base& v)
		{
			Vector3Base r = v;
			const T length = Math::Sqrt(r.x * r.x + r.y * r.y + r.z * r.z);
			if (length >= Math::ZeroTolerance)
			{
				const T inv = (T)1.0f / length;
				r.x *= inv;
				r.y *= inv;
				r.z *= inv;
			}
			return r;
		}

		// Performs vector normalization (scales vector up to unit length). This is a faster version that does not performs check for length equal 0 (it assumes that input vector is not empty).
		// @param inout Input vector to normalize (cannot be zero).
		// @returns Output vector that is normalized (has unit length)
		static Vector3Base NormalizeFast(const Vector3Base& v)
		{
			const T inv = 1.0f / v.Length();
			return Vector3Base(v.x * inv, v.y * inv, v.z * inv);
		}

		// Performs vector normalization (scales vector up to unit length)
		// @param inout Input vector to normalize
		// @param output Output vector that is normalized (has unit length)
		static inline void Normalize(const Vector3Base& input, Vector3Base& result)
		{
			result = Normalize(input);
		}

		// dot product with another vector
		inline static T Dot(const Vector3Base& a, const Vector3Base& b)
		{
			return a.x * b.x + a.y * b.y + a.z * b.z;
		}

		// Calculates the cross product of two vectors
		// @param a First source vector
		// @param b Second source vector
		// @param result When the method completes, contains the cross product of the two vectors
		static void Cross(const Vector3Base& a, const Vector3Base& b, Vector3Base& result)
		{
			result = Vector3Base(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
		}

		// Calculates the cross product of two vectors
		// @param a First source vector
		// @param b Second source vector
		// @returns Cross product of the two vectors
		static Vector3Base Cross(const Vector3Base& a, const Vector3Base& b)
		{
			return Vector3Base(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
		}

		// Performs a linear interpolation between two vectors
		// @param start Start vector
		// @param end End vector
		// @param amount Value between 0 and 1 indicating the weight of end
		// @param result When the method completes, contains the linear interpolation of the two vectors
		static void Lerp(const Vector3Base& start, const Vector3Base& end, T amount, Vector3Base& result)
		{
			result.x = Math::Lerp(start.x, end.x, amount);
			result.y = Math::Lerp(start.y, end.y, amount);
			result.z = Math::Lerp(start.z, end.z, amount);
		}

		// <summary>
		// Performs a linear interpolation between two vectors.
		// </summary>
		static Vector3Base Lerp(const Vector3Base& start, const Vector3Base& end, T amount)
		{
			Vector3Base result;
			Lerp(start, end, amount, result);
			return result;
		}

		// Performs a cubic interpolation between two vectors
		// @param start Start vector
		// @param end End vector
		// @param amount Value between 0 and 1 indicating the weight of end
		// @param result When the method completes, contains the cubic interpolation of the two vectors
		static void SmoothStep(const Vector3Base& start, const Vector3Base& end, T amount, Vector3Base& result)
		{
			amount = Math::SmoothStep(amount);
			Lerp(start, end, amount, result);
		}

		/// <summary>
		/// Moves a value current towards target.
		/// </summary>
		/// <param name="current">The position to move from.</param>
		/// <param name="target">The position to move towards.</param>
		/// <param name="maxDistanceDelta">The maximum distance that can be applied to the value.</param>
		/// <returns>The new position.</returns>
		static Vector3Base MoveTowards(Vector3Base current, Vector3Base target, float maxDistanceDelta)
		{
			const Vector3Base to = target - current;
			const T distanceSq = to.LengthSquared();
			if (distanceSq == 0 || (maxDistanceDelta >= 0 && distanceSq <= maxDistanceDelta * maxDistanceDelta))
				return target;
			const T scale = maxDistanceDelta / Math::Sqrt(distanceSq);
			return Vector3Base(current.x + to.x * scale, current.y + to.y * scale, current.z + to.z * scale);
		}

		// Performs a Hermite spline interpolation.
		// @param value1 First source position vector
		// @param tangent1 First source tangent vector
		// @param value2 Second source position vector
		// @param tangent2 Second source tangent vector
		// @param amount Weighting factor,
		// @param result When the method completes, contains the result of the Hermite spline interpolation,
		static SE_API_CORE void Hermite(const Vector3Base& value1, const Vector3Base& tangent1, const Vector3Base& value2, const Vector3Base& tangent2, T amount, Vector3Base& result);

		// Returns the reflection of a vector off a surface that has the specified normal
		// @param vector The source vector
		// @param normal Normal of the surface
		// @param result When the method completes, contains the reflected vector
		static SE_API_CORE void Reflect(const Vector3Base& vector, const Vector3Base& normal, Vector3Base& result);

		// Transforms a 3D vector by the given Quaternion rotation
		// @param vector The vector to rotate
		// @param rotation The Quaternion rotation to apply
		// @param result When the method completes, contains the transformed Vector3
		static SE_API_CORE void Transform(const Vector3Base& vector, const Quaternion& rotation, Vector3Base& result);

		// Transforms a 3D vector by the given Quaternion rotation
		// @param vector The vector to rotate
		// @param rotation The Quaternion rotation to apply
		// @returns The transformed Vector3
		static SE_API_CORE Vector3Base Transform(const Vector3Base& vector, const Quaternion& rotation);

		// Transforms a 3D vector by the given matrix
		// @param vector The source vector
		// @param transform The transformation matrix
		// @param result When the method completes, contains the transformed Vector3
		static SE_API_CORE void Transform(const Vector3Base& vector, const Matrix& transform, Vector3Base& result);

		// Transforms a 3D vector by the given matrix
		// @param vector The source vector
		// @param transform The transformation matrix
		// @param result When the method completes, contains the transformed Vector3
		static SE_API_CORE void Transform(const Vector3Base& vector, const Matrix3x3& transform, Vector3Base& result);

		// Transforms a 3D vector by the given transformation
		// @param vector The source vector
		// @param transform The transformation
		// @param result When the method completes, contains the transformed Vector3
		static SE_API_CORE void Transform(const Vector3Base& vector, const ::SE::Transform& transform, Vector3Base& result);

		// Transforms a 3D vector by the given matrix
		// @param vector The source vector
		// @param transform The transformation matrix
		// @returns Transformed Vector3
		static SE_API_CORE Vector3Base Transform(const Vector3Base& vector, const Matrix& transform);

		// Transforms a 3D vector by the given transformation
		// @param vector The source vector
		// @param transform The transformation
		// @returns Transformed Vector3
		static SE_API_CORE Vector3Base Transform(const Vector3Base& vector, const ::SE::Transform& transform);

		// Transforms a 3D vector by the given matrix
		// @param vector The source vector
		// @param transform The transformation matrix
		// @param result When the method completes, contains the transformed Vector4
		static SE_API_CORE void Transform(const Vector3Base& vector, const Matrix& transform, Vector4Base<T>& result);

		// Performs a coordinate transformation using the given matrix
		// @param coordinate The coordinate vector to transform
		// @param transform The transformation matrix
		// @param result When the method completes, contains the transformed coordinates
		static SE_API_CORE void TransformCoordinate(const Vector3Base& coordinate, const Matrix& transform, Vector3Base& result);

		// Performs a normal transformation using the given matrix
		// @param normal The normal vector to transform
		// @param transform The transformation matrix
		// @param result When the method completes, contains the transformed normal
		static SE_API_CORE void TransformNormal(const Vector3Base& normal, const Matrix& transform, Vector3Base& result);

		/// <summary>
		/// Projects a vector onto another vector.
		/// </summary>
		/// <param name="vector">The vector to project.</param>
		/// <param name="onNormal">The projection normal vector.</param>
		/// <returns>The projected vector.</returns>
		static SE_API_CORE Vector3Base Project(const Vector3Base& vector, const Vector3Base& onNormal);

		/// <summary>
		/// Projects a vector onto a plane defined by a normal orthogonal to the plane.
		/// </summary>
		/// <param name="vector">The vector to project.</param>
		/// <param name="planeNormal">The plane normal vector.</param>
		/// <returns>The projected vector.</returns>
		static Vector3Base ProjectOnPlane(const Vector3Base& vector, const Vector3Base& planeNormal)
		{
			return vector - Project(vector, planeNormal);
		}

		// Projects a 3D vector from object space into screen space
		// @param vector The vector to project
		// @param x The X position of the viewport
		// @param y The Y position of the viewport
		// @param width The width of the viewport
		// @param height The height of the viewport
		// @param minZ The minimum depth of the viewport
		// @param maxZ The maximum depth of the viewport
		// @param worldViewProjection The combined world-view-projection matrix
		// @param result When the method completes, contains the vector in screen space
		static SE_API_CORE void Project(const Vector3Base& vector, float x, float y, float width, float height, float minZ, float maxZ, const Matrix& worldViewProjection, Vector3Base& result);

		// Projects a 3D vector from object space into screen space
		// @param vector The vector to project
		// @param x The X position of the viewport
		// @param y The Y position of the viewport
		// @param width The width of the viewport
		// @param height The height of the viewport
		// @param minZ The minimum depth of the viewport
		// @param maxZ The maximum depth of the viewport
		// @param worldViewProjection The combined world-view-projection matrix
		// @returns The vector in screen space
		static Vector3Base Project(const Vector3Base& vector, float x, float y, float width, float height, float minZ, float maxZ, const Matrix& worldViewProjection)
		{
			Vector3Base result;
			Project(vector, x, y, width, height, minZ, maxZ, worldViewProjection, result);
			return result;
		}

		// Projects a 3D vector from screen space into object space
		// @param vector The vector to project
		// @param x The X position of the viewport
		// @param y The Y position of the viewport
		// @param width The width of the viewport
		// @param height The height of the viewport
		// @param minZ The minimum depth of the viewport
		// @param maxZ The maximum depth of the viewport
		// @param worldViewProjection The combined world-view-projection matrix
		// @param result When the method completes, contains the vector in object space
		static SE_API_CORE void Unproject(const Vector3Base& vector, float x, float y, float width, float height, float minZ, float maxZ, const Matrix& worldViewProjection, Vector3Base& result);

		// Projects a 3D vector from screen space into object space
		// @param vector The vector to project
		// @param x The X position of the viewport
		// @param y The Y position of the viewport
		// @param width The width of the viewport
		// @param height The height of the viewport
		// @param minZ The minimum depth of the viewport
		// @param maxZ The maximum depth of the viewport
		// @param worldViewProjection The combined world-view-projection matrix
		// @returns The vector in object space
		static Vector3Base Unproject(const Vector3Base& vector, float x, float y, float width, float height, float minZ, float maxZ, const Matrix& worldViewProjection)
		{
			Vector3Base result;
			Unproject(vector, x, y, width, height, minZ, maxZ, worldViewProjection, result);
			return result;
		}

		/// <summary>
		/// Creates an orthonormal basis from a basis with at least two orthogonal vectors.
		/// </summary>
		/// <param name="xAxis">The X axis.</param>
		/// <param name="yAxis">The y axis.</param>
		/// <param name="zAxis">The z axis.</param>
		static SE_API_CORE void CreateOrthonormalBasis(Vector3Base& xAxis, Vector3Base& yAxis, Vector3Base& zAxis);

		/// <summary>
		/// Finds the best arbitrary axis vectors to represent U and V axes of a plane, by using this vector as the normal of the plane.
		/// </summary>
		/// <param name="firstAxis">The reference to first axis.</param>
		/// <param name="secondAxis">The reference to second axis.</param>
		SE_API_CORE void FindBestAxisVectors(Vector3Base& firstAxis, Vector3Base& secondAxis) const;

		/// <summary>
		/// Calculates the area of the triangle.
		/// </summary>
		/// <param name="v0">The first triangle vertex.</param>
		/// <param name="v1">The second triangle vertex.</param>
		/// <param name="v2">The third triangle vertex.</param>
		/// <returns>The triangle area.</returns>
		static SE_API_CORE T TriangleArea(const Vector3Base& v0, const Vector3Base& v1, const Vector3Base& v2);

		/// <summary>
		/// Calculates the angle (in radians) between from and to. This is always the smallest value.
		/// </summary>
		/// <param name="from">The first vector.</param>
		/// <param name="to">The second vector.</param>
		/// <returns>The angle (in radians).</returns>
		static SE_API_CORE T Angle(const Vector3Base& from, const Vector3Base& to);
	};

	template<typename T>
	inline Vector3Base<T> operator+(T a, const Vector3Base<T>& b)
	{
		return b + a;
	}

	template<typename T>
	inline Vector3Base<T> operator-(T a, const Vector3Base<T>& b)
	{
		return Vector3Base<T>(a) - b;
	}

	template<typename T>
	inline Vector3Base<T> operator*(T a, const Vector3Base<T>& b)
	{
		return b * a;
	}

	template<typename T>
	inline Vector3Base<T> operator/(T a, const Vector3Base<T>& b)
	{
		return Vector3Base<T>(a) / b;
	}

	template<typename T>
	inline Vector3Base<T> operator+(typename TOtherFloat<T>::Type a, const Vector3Base<T>& b)
	{
		return b + a;
	}

	template<typename T>
	inline Vector3Base<T> operator-(typename TOtherFloat<T>::Type a, const Vector3Base<T>& b)
	{
		return Vector3Base<T>(a) - b;
	}

	template<typename T>
	inline Vector3Base<T> operator*(typename TOtherFloat<T>::Type a, const Vector3Base<T>& b)
	{
		return b * a;
	}

	template<typename T>
	inline Vector3Base<T> operator/(typename TOtherFloat<T>::Type a, const Vector3Base<T>& b)
	{
		return Vector3Base<T>(a) / b;
	}

	template<typename T>
	inline uint32 GetHash(const Vector3Base<T>& key)
	{
		return (((*(uint32*)&key.x * 397) ^ *(uint32*)&key.y) * 397) ^ *(uint32*)&key.z;
	}
}

namespace SE::Math
{
    template<typename T>
    inline static bool NearEqual(const Vector3Base<T>& a, const Vector3Base<T>& b)
    {
        return Vector3Base<T>::NearEqual(a, b);
    }

    template<typename T>
    inline static Vector3Base<T> UnwindDegrees(const Vector3Base<T>& v)
    {
        return Vector3Base<T>(UnwindDegrees(v.x), UnwindDegrees(v.y), UnwindDegrees(v.z));
    }
}

template<>
struct TIsPODType<SE::Float3>
{
    enum { Value = true };
};

DEFINE_DEFAULT_FORMATTING(SE::Float3, "X:{0} Y:{1} Z:{2}", v.x, v.y, v.z);

template<>
struct TIsPODType<SE::Double3>
{
    enum { Value = true };
};

DEFINE_DEFAULT_FORMATTING(SE::Double3, "X:{0} Y:{1} Z:{2}", v.x, v.y, v.z);

template<>
struct TIsPODType<SE::Int3>
{
    enum { Value = true };
};

DEFINE_DEFAULT_FORMATTING(SE::Int3, "X:{0} Y:{1} Z:{2}", v.x, v.y, v.z);

#if !defined(_MSC_VER) || defined(__clang__)
// Forward specializations for Clang
template<> SE_API_CORE const SE::Float3 SE::Float3::Zero;
template<> SE_API_CORE const SE::Float3 SE::Float3::One;
template<> SE_API_CORE const SE::Float3 SE::Float3::Half;
template<> SE_API_CORE const SE::Float3 SE::Float3::UnitX;
template<> SE_API_CORE const SE::Float3 SE::Float3::UnitY;
template<> SE_API_CORE const SE::Float3 SE::Float3::UnitZ;
template<> SE_API_CORE const SE::Float3 SE::Float3::Up;
template<> SE_API_CORE const SE::Float3 SE::Float3::Down;
template<> SE_API_CORE const SE::Float3 SE::Float3::Left;
template<> SE_API_CORE const SE::Float3 SE::Float3::Right;
template<> SE_API_CORE const SE::Float3 SE::Float3::Forward;
template<> SE_API_CORE const SE::Float3 SE::Float3::Backward;
template<> SE_API_CORE const SE::Float3 SE::Float3::Minimum;
template<> SE_API_CORE const SE::Float3 SE::Float3::Maximum;

template<> SE_API_CORE const SE::Double3 SE::Double3::Zero;
template<> SE_API_CORE const SE::Double3 SE::Double3::One;
template<> SE_API_CORE const SE::Double3 SE::Double3::Half;
template<> SE_API_CORE const SE::Double3 SE::Double3::UnitX;
template<> SE_API_CORE const SE::Double3 SE::Double3::UnitY;
template<> SE_API_CORE const SE::Double3 SE::Double3::UnitZ;
template<> SE_API_CORE const SE::Double3 SE::Double3::Up;
template<> SE_API_CORE const SE::Double3 SE::Double3::Down;
template<> SE_API_CORE const SE::Double3 SE::Double3::Left;
template<> SE_API_CORE const SE::Double3 SE::Double3::Right;
template<> SE_API_CORE const SE::Double3 SE::Double3::Forward;
template<> SE_API_CORE const SE::Double3 SE::Double3::Backward;
template<> SE_API_CORE const SE::Double3 SE::Double3::Minimum;
template<> SE_API_CORE const SE::Double3 SE::Double3::Maximum;

template<> SE_API_CORE const SE::Int3 SE::Int3::Zero;
template<> SE_API_CORE const SE::Int3 SE::Int3::One;
template<> SE_API_CORE const SE::Int3 SE::Int3::Half;
template<> SE_API_CORE const SE::Int3 SE::Int3::UnitX;
template<> SE_API_CORE const SE::Int3 SE::Int3::UnitY;
template<> SE_API_CORE const SE::Int3 SE::Int3::UnitZ;
template<> SE_API_CORE const SE::Int3 SE::Int3::Up;
template<> SE_API_CORE const SE::Int3 SE::Int3::Down;
template<> SE_API_CORE const SE::Int3 SE::Int3::Left;
template<> SE_API_CORE const SE::Int3 SE::Int3::Right;
template<> SE_API_CORE const SE::Int3 SE::Int3::Forward;
template<> SE_API_CORE const SE::Int3 SE::Int3::Backward;
template<> SE_API_CORE const SE::Int3 SE::Int3::Minimum;
template<> SE_API_CORE const SE::Int3 SE::Int3::Maximum;
#endif
