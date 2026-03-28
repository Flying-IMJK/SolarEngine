#include "Quaternion.h"
#include "Matrix.h"
#include "Matrix3x3.h"
#include "Math.h"

#include "Core/Types/Strings/String.h"

namespace SE
{
	Quaternion Quaternion::zero(0, 0, 0, 0);
	Quaternion Quaternion::One(1, 1, 1, 1);
	Quaternion Quaternion::Identity(0, 0, 0, 1);

	Quaternion::Quaternion(const Float4& value)
		: x(value.x)
		, y(value.y)
		, z(value.z)
		, w(value.w)
	{
	}

	String Quaternion::ToString() const
	{
		return String::Format(SE_TEXT("{}"), *this);
	}

	float Quaternion::GetAngle() const
	{
		const float length = x * x + y * y + z * z;
		if (Math::IsZero(length))
			return 0.0f;
		return 2.0f * acosf(Math::Clamp(w, -1.0f, 1.0f));
	}

	Float3 Quaternion::GetAxis() const
	{
		const float length = x * x + y * y + z * z;
		if (Math::IsZero(length))
			return Float3::UnitX;
		const float inv = 1.0f / Math::Sqrt(length);
		return Float3(x * inv, y * inv, z * inv);
	}

	Float3 Quaternion::GetEuler() const
	{
		Float3 result;
		const float sqw = w * w;
		const float sqx = x * x;
		const float sqy = y * y;
		const float sqz = z * z;
		const float unit = sqx + sqy + sqz + sqw; // if normalised is one, otherwise is correction factor
		const float test = x * w - y * z;

		if (test > 0.499995f * unit)
		{
			// singularity at north pole

			// yaw pitch roll
			result.y = 2.0f * Math::ATan2(y, x);
			result.x = Math::PiDivTwo;
			result.z = 0;
		}
		else if (test < -0.499995f * unit)
		{
			// singularity at south pole

			// yaw pitch roll
			result.y = -2.0f * Math::ATan2(y, x);
			result.x = -Math::PiDivTwo;
			result.z = 0;
		}
		else
		{
			// yaw pitch roll
			const Quaternion q = Quaternion(w, z, x, y);
			result.y = Math::ATan2(2.0f * q.x * q.w + 2.0f * q.y * q.z, 1 - 2.0f * (q.z * q.z + q.w * q.w));
			result.x = Math::ASin(2.0f * (q.x * q.z - q.w * q.y));
			result.z = Math::ATan2(2.0f * q.x * q.y + 2.0f * q.z * q.w, 1 - 2.0f * (q.y * q.y + q.z * q.z));
		}

		return Math::UnwindDegrees(result * Math::RadiansToDegrees);
	}

	void Quaternion::Multiply(const Quaternion& other)
	{
		const float a = y * other.z - z * other.y;
		const float b = z * other.x - x * other.z;
		const float c = x * other.y - y * other.x;
		const float d = x * other.x + y * other.y + z * other.z;
		x = x * other.w + other.x * w + a;
		y = y * other.w + other.y * w + b;
		z = z * other.w + other.z * w + c;
		w = w * other.w - d;
	}

	Float3 Quaternion::operator*(const Float3& vector) const
	{
		return Float3::Transform(vector, *this);
	}

	void Quaternion::Multiply(const Quaternion& left, const Quaternion& right, Quaternion& result)
	{
		const float a = left.y * right.z - left.z * right.y;
		const float b = left.z * right.x - left.x * right.z;
		const float c = left.x * right.y - left.y * right.x;
		const float d = left.x * right.x + left.y * right.y + left.z * right.z;
		result.x = left.x * right.w + right.x * left.w + a;
		result.y = left.y * right.w + right.y * left.w + b;
		result.z = left.z * right.w + right.z * left.w + c;
		result.w = left.w * right.w - d;
	}

	void Quaternion::Lerp(const Quaternion& start, const Quaternion& end, float amount, Quaternion& result)
	{
		const float inverse = 1.0f - amount;
		if (Dot(start, end) >= 0.0f)
		{
			result.x = inverse * start.x + amount * end.x;
			result.y = inverse * start.y + amount * end.y;
			result.z = inverse * start.z + amount * end.z;
			result.w = inverse * start.w + amount * end.w;
		}
		else
		{
			result.x = inverse * start.x - amount * end.x;
			result.y = inverse * start.y - amount * end.y;
			result.z = inverse * start.z - amount * end.z;
			result.w = inverse * start.w - amount * end.w;
		}
		result.Normalize();
	}

	void Quaternion::RotationAxis(const Float3& axis, float angle, Quaternion& result)
	{
		Float3 normalized;
		Float3::Normalize(axis, normalized);

		const float half = angle * 0.5f;
		const float sinHalf = Math::Sin(half);
		const float cosHalf = Math::Cos(half);

		result.x = normalized.x * sinHalf;
		result.y = normalized.y * sinHalf;
		result.z = normalized.z * sinHalf;
		result.w = cosHalf;
	}

	void Quaternion::RotationCosAxis(const Float3& axis, float cos, Quaternion& result)
	{
		Float3 normalized;
		Float3::Normalize(axis, normalized);

		const float cosHalf2 = (1.0f + cos) * 0.5f;
		const float sinHalf2 = 1.0f - cosHalf2;
		const float cosHalf = Math::Sqrt(cosHalf2);
		const float sinHalf = Math::Sqrt(sinHalf2);

		result.x = normalized.x * sinHalf;
		result.y = normalized.y * sinHalf;
		result.z = normalized.z * sinHalf;
		result.w = cosHalf;
	}

	void Quaternion::RotationMatrix(const Matrix& matrix, Quaternion& result)
	{
		float sqrtV;
		float half;
		const float scale = matrix.M11 + matrix.M22 + matrix.M33;

		if (scale > 0.0f)
		{
			sqrtV = Math::Sqrt(scale + 1.0f);
			result.w = sqrtV * 0.5f;
			sqrtV = 0.5f / sqrtV;

			result.x = (matrix.M23 - matrix.M32) * sqrtV;
			result.y = (matrix.M31 - matrix.M13) * sqrtV;
			result.z = (matrix.M12 - matrix.M21) * sqrtV;
		}
		else if (matrix.M11 >= matrix.M22 && matrix.M11 >= matrix.M33)
		{
			sqrtV = Math::Sqrt(1.0f + matrix.M11 - matrix.M22 - matrix.M33);
			half = 0.5f / sqrtV;

			result = Quaternion(
				0.5f * sqrtV,
				(matrix.M12 + matrix.M21) * half,
				(matrix.M13 + matrix.M31) * half,
				(matrix.M23 - matrix.M32) * half);
		}
		else if (matrix.M22 > matrix.M33)
		{
			sqrtV = Math::Sqrt(1.0f + matrix.M22 - matrix.M11 - matrix.M33);
			half = 0.5f / sqrtV;

			result = Quaternion(
				(matrix.M21 + matrix.M12) * half,
				0.5f * sqrtV,
				(matrix.M32 + matrix.M23) * half,
				(matrix.M31 - matrix.M13) * half);
		}
		else
		{
			sqrtV = Math::Sqrt(1.0f + matrix.M33 - matrix.M11 - matrix.M22);
			half = 0.5f / sqrtV;

			result = Quaternion(
				(matrix.M31 + matrix.M13) * half,
				(matrix.M32 + matrix.M23) * half,
				0.5f * sqrtV,
				(matrix.M12 - matrix.M21) * half);
		}

		result.Normalize();
	}

	void Quaternion::RotationMatrix(const Matrix3x3& matrix, Quaternion& result)
	{
		float sqrtV;
		float half;
		const float scale = matrix.M11 + matrix.M22 + matrix.M33;

		if (scale > 0.0f)
		{
			sqrtV = Math::Sqrt(scale + 1.0f);
			result.w = sqrtV * 0.5f;
			sqrtV = 0.5f / sqrtV;

			result.x = (matrix.M23 - matrix.M32) * sqrtV;
			result.y = (matrix.M31 - matrix.M13) * sqrtV;
			result.z = (matrix.M12 - matrix.M21) * sqrtV;
		}
		else if (matrix.M11 >= matrix.M22 && matrix.M11 >= matrix.M33)
		{
			sqrtV = Math::Sqrt(1.0f + matrix.M11 - matrix.M22 - matrix.M33);
			half = 0.5f / sqrtV;

			result = Quaternion(
				0.5f * sqrtV,
				(matrix.M12 + matrix.M21) * half,
				(matrix.M13 + matrix.M31) * half,
				(matrix.M23 - matrix.M32) * half);
		}
		else if (matrix.M22 > matrix.M33)
		{
			sqrtV = Math::Sqrt(1.0f + matrix.M22 - matrix.M11 - matrix.M33);
			half = 0.5f / sqrtV;

			result = Quaternion(
				(matrix.M21 + matrix.M12) * half,
				0.5f * sqrtV,
				(matrix.M32 + matrix.M23) * half,
				(matrix.M31 - matrix.M13) * half);
		}
		else
		{
			sqrtV = Math::Sqrt(1.0f + matrix.M33 - matrix.M11 - matrix.M22);
			half = 0.5f / sqrtV;

			result = Quaternion(
				(matrix.M31 + matrix.M13) * half,
				(matrix.M32 + matrix.M23) * half,
				0.5f * sqrtV,
				(matrix.M12 - matrix.M21) * half);
		}

		result.Normalize();
	}

	void Quaternion::LookAt(const Float3& eye, const Float3& target, const Float3& up, Quaternion& result)
	{
		Matrix matrix;
		Matrix::LookAt(eye, target, up, matrix);
		RotationMatrix(matrix, result);
	}

	void Quaternion::RotationLookAt(const Float3& forward, const Float3& up, Quaternion& result)
	{
		LookAt(Float3::Zero, forward, up, result);
	}

	void Quaternion::Billboard(const Float3& objectPosition, const Float3& cameraPosition, const Float3& cameraUpVector, const Float3& cameraForwardVector, Quaternion& result)
	{
		Matrix matrix;
		Matrix::Billboard(objectPosition, cameraPosition, cameraUpVector, cameraForwardVector, matrix);
		RotationMatrix(matrix, result);
	}

	Quaternion Quaternion::FromDirection(const Float3& direction)
	{
		Quaternion orientation;
		if (Float3::Dot(direction, Float3::Up) >= 0.999f)
		{
			RotationAxis(Float3::Left, Math::PiDivTwo, orientation);
		}
		else
		{
			Float3 right, up;
			Float3::Cross(direction, Float3::Up, right);
			Float3::Cross(right, direction, up);
			LookRotation(direction, up, orientation);
		}
		return orientation;
	}

	void Quaternion::LookRotation(const Float3& forward, const Float3& up, Quaternion& result)
	{
		Float3 forwardNorm = forward;
		forwardNorm.Normalize();
		Float3 rightNorm;
		Float3::Cross(up, forwardNorm, rightNorm);
		rightNorm.Normalize();
		Float3 upNorm;
		Float3::Cross(forwardNorm, rightNorm, upNorm);

		#define m00 rightNorm.x
		#define m01 rightNorm.y
		#define m02 rightNorm.z
		#define m10 upNorm.x
		#define m11 upNorm.y
		#define m12 upNorm.z
		#define m20 forwardNorm.x
		#define m21 forwardNorm.y
		#define m22 forwardNorm.z

		const float sum = m00 + m11 + m22;
		if (sum > 0)
		{
			const float num = Math::Sqrt(sum + 1);
			const float invNumHalf = 0.5f / num;
			result.x = (m12 - m21) * invNumHalf;
			result.y = (m20 - m02) * invNumHalf;
			result.z = (m01 - m10) * invNumHalf;
			result.w = num * 0.5f;
		}
		else if (m00 >= m11 && m00 >= m22)
		{
			const float num = Math::Sqrt(1 + m00 - m11 - m22);
			const float invNumHalf = 0.5f / num;
			result.x = 0.5f * num;
			result.y = (m01 + m10) * invNumHalf;
			result.z = (m02 + m20) * invNumHalf;
			result.w = (m12 - m21) * invNumHalf;
		}
		else if (m11 > m22)
		{
			const float num = Math::Sqrt(1 + m11 - m00 - m22);
			const float invNumHalf = 0.5f / num;
			result.x = (m10 + m01) * invNumHalf;
			result.y = 0.5f * num;
			result.z = (m21 + m12) * invNumHalf;
			result.w = (m20 - m02) * invNumHalf;
		}
		else
		{
			const float num = Math::Sqrt(1 + m22 - m00 - m11);
			const float invNumHalf = 0.5f / num;
			result.x = (m20 + m02) * invNumHalf;
			result.y = (m21 + m12) * invNumHalf;
			result.z = 0.5f * num;
			result.w = (m01 - m10) * invNumHalf;
		}

#undef m00
#undef m01
#undef m02
#undef m10
#undef m11
#undef m12
#undef m20
#undef m21
#undef m22
	}

	void Quaternion::GetRotationFromTo(const Float3& from, const Float3& to, Quaternion& result, const Float3& fallbackAxis)
	{
		// Based on Stan Melax's article in Game Programming Gems

		Float3 v0 = from;
		Float3 v1 = to;
		v0.Normalize();
		v1.Normalize();

		// If dot == 1, vectors are the same
		const float d = Float3::Dot(v0, v1);
		if (d >= 1.0f)
		{
			result = Identity;
			return;
		}

		if (d < 1e-6f - 1.0f)
		{
			if (fallbackAxis != Float3::Zero)
			{
				// Rotate 180 degrees about the fallback axis
				RotationAxis(fallbackAxis, Math::PI, result);
			}
			else
			{
				// Generate an axis
				Float3 axis = Float3::Cross(Float3::UnitX, from);
				if (axis.LengthSquared() < Math::ZeroTolerance) // Pick another if colinear
					axis = Float3::Cross(Float3::UnitY, from);
				axis.Normalize();
				RotationAxis(axis, Math::PI, result);
			}
		}
		else
		{
			const float s = Math::Sqrt((1 + d) * 2);
			const float invS = 1 / s;

			Float3 c;
			Float3::Cross(v0, v1, c);

			result.x = c.x * invS;
			result.y = c.y * invS;
			result.z = c.z * invS;
			result.w = s * 0.5f;
			result.Normalize();
		}
	}

	void Quaternion::FindBetween(const Float3& from, const Float3& to, Quaternion& result)
	{
		// http://lolengine.net/blog/2014/02/24/quaternion-from-two-vectors-final
		const float normFromNormTo = Math::Sqrt(from.LengthSquared() * to.LengthSquared());
		if (normFromNormTo < Math::ZeroTolerance)
		{
			result = Identity;
			return;
		}
		const float w = normFromNormTo + Float3::Dot(from, to);
		if (w < 1.e-6f * normFromNormTo)
		{
			result = Math::Abs(from.x) > Math::Abs(from.z)
					 ? Quaternion(-from.y, from.x, 0.0f, 0.0f)
					 : Quaternion(0.0f, -from.z, from.y, 0.0f);
		}
		else
		{
			const Float3 cross = Float3::Cross(from, to);
			result = Quaternion(cross.x, cross.y, cross.z, w);
		}
		result.Normalize();
	}

	void Quaternion::Slerp(const Quaternion& start, const Quaternion& end, float amount, Quaternion& result)
	{
		float opposite;
		float inverse;
		const float dot = Dot(start, end);

		if (Math::Abs(dot) > 1.0f - Math::ZeroTolerance)
		{
			inverse = 1.0f - amount;
			opposite = amount * Math::Sign(dot);
		}
		else
		{
			const float acos1 = Math::ACos(Math::Abs(dot));
			const float invSin = 1.0f / Math::Sin(acos1);
			inverse = Math::Sin((1.0f - amount) * acos1) * invSin;
			opposite = Math::Sin(amount * acos1) * invSin * Math::Sign(dot);
		}

		result.x = inverse * start.x + opposite * end.x;
		result.y = inverse * start.y + opposite * end.y;
		result.z = inverse * start.z + opposite * end.z;
		result.w = inverse * start.w + opposite * end.w;
	}

	Quaternion Quaternion::Euler(float x, float y, float z)
	{
		const float halfRoll = z * (Math::DegreesToRadians * 0.5f);
		const float halfPitch = x * (Math::DegreesToRadians * 0.5f);
		const float halfyaw = y * (Math::DegreesToRadians * 0.5f);

		const float sinRollOver2 = Math::Sin(halfRoll);
		const float cosRollOver2 = Math::Cos(halfRoll);
		const float sinPitchOver2 = Math::Sin(halfPitch);
		const float cosPitchOver2 = Math::Cos(halfPitch);
		const float sinyawOver2 = Math::Sin(halfyaw);
		const float cosyawOver2 = Math::Cos(halfyaw);

		return Quaternion(
			cosyawOver2 * sinPitchOver2 * cosRollOver2 + sinyawOver2 * cosPitchOver2 * sinRollOver2,
			sinyawOver2 * cosPitchOver2 * cosRollOver2 - cosyawOver2 * sinPitchOver2 * sinRollOver2,
			cosyawOver2 * cosPitchOver2 * sinRollOver2 - sinyawOver2 * sinPitchOver2 * cosRollOver2,
			cosyawOver2 * cosPitchOver2 * cosRollOver2 + sinyawOver2 * sinPitchOver2 * sinRollOver2
		);
	}

	Quaternion Quaternion::Euler(const Float3& euler)
	{
		return Euler(euler.x, euler.y, euler.z);
	}

	void Quaternion::RotationYawPitchRoll(float yaw, float pitch, float roll, Quaternion& result)
	{
		const float halfRoll = roll * 0.5f;
		const float halfPitch = pitch * 0.5f;
		const float halfyaw = yaw * 0.5f;

		const float sinRollOver2 = Math::Sin(halfRoll);
		const float cosRollOver2 = Math::Cos(halfRoll);
		const float sinPitchOver2 = Math::Sin(halfPitch);
		const float cosPitchOver2 = Math::Cos(halfPitch);
		const float sinyawOver2 = Math::Sin(halfyaw);
		const float cosyawOver2 = Math::Cos(halfyaw);

		result.w = cosyawOver2 * cosPitchOver2 * cosRollOver2 + sinyawOver2 * sinPitchOver2 * sinRollOver2;
		result.x = cosyawOver2 * sinPitchOver2 * cosRollOver2 + sinyawOver2 * cosPitchOver2 * sinRollOver2;
		result.y = sinyawOver2 * cosPitchOver2 * cosRollOver2 - cosyawOver2 * sinPitchOver2 * sinRollOver2;
		result.z = cosyawOver2 * cosPitchOver2 * sinRollOver2 - sinyawOver2 * sinPitchOver2 * cosRollOver2;
	}

}

