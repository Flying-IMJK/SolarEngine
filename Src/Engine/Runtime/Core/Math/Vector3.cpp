#include "Vector3.h"
#include "Vector2.h"
#include "Vector4.h"
#include "Color.h"
#include "Quaternion.h"
#include "Matrix.h"
#include "Matrix3x3.h"
#include "Transform.h"
#include "Runtime/Core/Types/Strings/String.h"

namespace SE
{

	// Float
	static_assert(sizeof(Float3) == 12, "Invalid Float3 type size.");

	template<>
	const Float3 Float3::Zero(0.0f);
	template<>
	const Float3 Float3::One(1.0f);
	template<>
	const Float3 Float3::Half(0.5f);
	template<>
	const Float3 Float3::UnitX(1.0f, 0.0f, 0.0f);
	template<>
	const Float3 Float3::UnitY(0.0f, 1.0f, 0.0f);
	template<>
	const Float3 Float3::UnitZ(0.0f, 0.0f, 1.0f);
	template<>
	const Float3 Float3::Up(0.0f, 1.0f, 0.0f);
	template<>
	const Float3 Float3::Down(0.0f, -1.0f, 0.0f);
	template<>
	const Float3 Float3::Left(-1.0f, 0.0f, 0.0f);
	template<>
	const Float3 Float3::Right(1.0f, 0.0f, 0.0f);
	template<>
	const Float3 Float3::Forward(0.0f, 0.0f, 1.0f);
	template<>
	const Float3 Float3::Backward(0.0f, 0.0f, -1.0f);
	template<>
	const Float3 Float3::Minimum(Min_float);
	template<>
	const Float3 Float3::Maximum(Max_float);

	template<>
	Float3::Vector3Base(const Float2& xy, float z)
		: x(xy.x)
		, y(xy.y)
		, z(z)
	{
	}

	template<>
	Float3::Vector3Base(const Double2& xy, float z)
		: x((float)xy.x)
		, y((float)xy.y)
		, z(0)
	{
	}

	template<>
	Float3::Vector3Base(const Int2& xy, float z)
		: x((float)xy.x)
		, y((float)xy.y)
		, z(z)
	{
	}

	template<>
	Float3::Vector3Base(const Int3& xyz)
		: x((float)xyz.x)
		, y((float)xyz.y)
		, z((float)xyz.z)
	{
	}

	template<>
	Float3::Vector3Base(const Int4& xyz)
		: x((float)xyz.x)
		, y((float)xyz.y)
		, z((float)xyz.z)
	{
	}

	template<>
	Float3::Vector3Base(const Float4& xyz)
		: x((float)xyz.x)
		, y((float)xyz.y)
		, z((float)xyz.z)
	{
	}

	template<>
	Float3::Vector3Base(const Double4& xyz)
		: x((float)xyz.x)
		, y((float)xyz.y)
		, z((float)xyz.z)
	{
	}

	template<>
	Float3::Vector3Base(const Color& color)
		: x(color.r)
		, y(color.g)
		, z(color.b)
	{
	}

	template<>
	String Float3::ToString() const
	{
		return String::Format(SE_TEXT("{}"), *this);
	}

	template<>
	void Float3::Hermite(const Float3& value1, const Float3& tangent1, const Float3& value2, const Float3& tangent2, float amount, Float3& result)
	{
		const float squared = amount * amount;
		const float cubed = amount * squared;
		const float part1 = 2.0f * cubed - 3.0f * squared + 1.0f;
		const float part2 = -2.0f * cubed + 3.0f * squared;
		const float part3 = cubed - 2.0f * squared + amount;
		const float part4 = cubed - squared;
		result.x = value1.x * part1 + value2.x * part2 + tangent1.x * part3 + tangent2.x * part4;
		result.y = value1.y * part1 + value2.y * part2 + tangent1.y * part3 + tangent2.y * part4;
		result.z = value1.z * part1 + value2.z * part2 + tangent1.z * part3 + tangent2.z * part4;
	}

	template<>
	void Float3::Reflect(const Float3& vector, const Float3& normal, Float3& result)
	{
/*		const Float3 dot = vector.x * normal.x + vector.y * normal.y + vector.z * normal.z;
		result.x = vector.x - 2.0f * dot * normal.x;
		result.y = vector.y - 2.0f * dot * normal.y;
		result.z = vector.z - 2.0f * dot * normal.z;*/
	}

	template<>
	void Float3::Transform(const Float3& vector, const Quaternion& rotation, Float3& result)
	{
		const float x = rotation.x + rotation.x;
		const float y = rotation.y + rotation.y;
		const float z = rotation.z + rotation.z;
		const float wx = rotation.w * x;
		const float wy = rotation.w * y;
		const float wz = rotation.w * z;
		const float xx = rotation.x * x;
		const float xy = rotation.x * y;
		const float xz = rotation.x * z;
		const float yy = rotation.y * y;
		const float yz = rotation.y * z;
		const float zz = rotation.z * z;
		result = Float3(
			vector.x * (1.0f - yy - zz) + vector.y * (xy - wz) + vector.z * (xz + wy),
			vector.x * (xy + wz) + vector.y * (1.0f - xx - zz) + vector.z * (yz - wx),
			vector.x * (xz - wy) + vector.y * (yz + wx) + vector.z * (1.0f - xx - yy));
	}

	template<>
	Float3 Float3::Transform(const Float3& vector, const Quaternion& rotation)
	{
		const float x = rotation.x + rotation.x;
		const float y = rotation.y + rotation.y;
		const float z = rotation.z + rotation.z;
		const float wx = rotation.w * x;
		const float wy = rotation.w * y;
		const float wz = rotation.w * z;
		const float xx = rotation.x * x;
		const float xy = rotation.x * y;
		const float xz = rotation.x * z;
		const float yy = rotation.y * y;
		const float yz = rotation.y * z;
		const float zz = rotation.z * z;
		return Float3(
			vector.x * (1.0f - yy - zz) + vector.y * (xy - wz) + vector.z * (xz + wy),
			vector.x * (xy + wz) + vector.y * (1.0f - xx - zz) + vector.z * (yz - wx),
			vector.x * (xz - wy) + vector.y * (yz + wx) + vector.z * (1.0f - xx - yy));
	}

	template<>
	void Float3::Transform(const Float3& vector, const Matrix& transform, Float4& result)
	{
		result = Float4(
			vector.x * transform.M11 + vector.y * transform.M21 + vector.z * transform.M31 + transform.M41,
			vector.x * transform.M12 + vector.y * transform.M22 + vector.z * transform.M32 + transform.M42,
			vector.x * transform.M13 + vector.y * transform.M23 + vector.z * transform.M33 + transform.M43,
			vector.x * transform.M14 + vector.y * transform.M24 + vector.z * transform.M34 + transform.M44);
	}

	template<>
	void Float3::Transform(const Float3& vector, const Matrix& transform, Float3& result)
	{
		result = Float3(
			vector.x * transform.M11 + vector.y * transform.M21 + vector.z * transform.M31 + transform.M41,
			vector.x * transform.M12 + vector.y * transform.M22 + vector.z * transform.M32 + transform.M42,
			vector.x * transform.M13 + vector.y * transform.M23 + vector.z * transform.M33 + transform.M43);
	}

	template<>
	void Float3::Transform(const Float3& vector, const Matrix3x3& transform, Float3& result)
	{
		result = Float3(
			vector.x * transform.M11 + vector.y * transform.M21 + vector.z * transform.M31,
			vector.x * transform.M12 + vector.y * transform.M22 + vector.z * transform.M32,
			vector.x * transform.M13 + vector.y * transform.M23 + vector.z * transform.M33);
	}

	template<>
	void Float3::Transform(const Float3& vector, const ::SE::Transform& transform, Float3& result)
	{
#if USE_LARGE_WORLDS
		Vector3 tmp;
    transform.LocalToWorld(vector, tmp);
    result = tmp;
#else
		transform.LocalToWorld(vector, result);
#endif
	}

	template<>
	Float3 Float3::Transform(const Float3& vector, const Matrix& transform)
	{
		return Float3(
			vector.x * transform.M11 + vector.y * transform.M21 + vector.z * transform.M31 + transform.M41,
			vector.x * transform.M12 + vector.y * transform.M22 + vector.z * transform.M32 + transform.M42,
			vector.x * transform.M13 + vector.y * transform.M23 + vector.z * transform.M33 + transform.M43);
		return Float3::Zero;
	}

	template<>
	Float3 Float3::Transform(const Float3& vector, const ::SE::Transform& transform)
	{
		Float3 result = Float3::Zero;
		transform.LocalToWorld(vector, result);
		return result;
	}

	template<>
	void Float3::TransformCoordinate(const Float3& coordinate, const Matrix& transform, Float3& result)
	{
		Float4 v;
		v.x = coordinate.x * transform.M11 + coordinate.y * transform.M21 + coordinate.z * transform.M31 + transform.M41;
		v.y = coordinate.x * transform.M12 + coordinate.y * transform.M22 + coordinate.z * transform.M32 + transform.M42;
		v.z = coordinate.x * transform.M13 + coordinate.y * transform.M23 + coordinate.z * transform.M33 + transform.M43;
		v.w = 1.0f / (coordinate.x * transform.M14 + coordinate.y * transform.M24 + coordinate.z * transform.M34 + transform.M44);
		result = Float3(v.x * v.w, v.y * v.w, v.z * v.w);
	}

	template<>
	void Float3::TransformNormal(const Float3& normal, const Matrix& transform, Float3& result)
	{
		result = Float3(
			normal.x * transform.M11 + normal.y * transform.M21 + normal.z * transform.M31,
			normal.x * transform.M12 + normal.y * transform.M22 + normal.z * transform.M32,
			normal.x * transform.M13 + normal.y * transform.M23 + normal.z * transform.M33);
	}

	template<>
	Float3 Float3::Project(const Float3& vector, const Float3& onNormal)
	{
		const float sqrMag = Dot(onNormal, onNormal);
		if (sqrMag < Math::ZeroTolerance)
			return Zero;
		return onNormal * Dot(vector, onNormal) / sqrMag;
	}

	template<>
	void Float3::Project(const Float3& vector, float x, float y, float width, float height, float minZ, float maxZ, const Matrix& worldViewProjection, Float3& result)
	{
		Float3 v;
		TransformCoordinate(vector, worldViewProjection, v);
		result = Float3((1.0f + v.x) * 0.5f * width + x, (1.0f - v.y) * 0.5f * height + y, v.z * (maxZ - minZ) + minZ);
	}

	template<>
	void Float3::Unproject(const Float3& vector, float x, float y, float width, float height, float minZ, float maxZ, const Matrix& worldViewProjection, Float3& result)
	{
		Matrix matrix;
		Matrix::Invert(worldViewProjection, matrix);
		const Float3 v((vector.x - x) / width * 2.0f - 1.0f, -((vector.y - y) / height * 2.0f - 1.0f), (vector.z - minZ) / (maxZ - minZ));
		TransformCoordinate(v, matrix, result);
	}

	template<>
	void Float3::CreateOrthonormalBasis(Float3& xAxis, Float3& yAxis, Float3& zAxis)
	{
		xAxis -= (xAxis | zAxis) / (zAxis | zAxis) * zAxis;
		yAxis -= (yAxis | zAxis) / (zAxis | zAxis) * zAxis;
		if (xAxis.LengthSquared() < Math::ZeroTolerance)
			xAxis = yAxis ^ zAxis;
		if (yAxis.LengthSquared() < Math::ZeroTolerance)
			yAxis = xAxis ^ zAxis;
		xAxis.Normalize();
		yAxis.Normalize();
		zAxis.Normalize();
	}

	template<>
	void Float3::FindBestAxisVectors(Float3& firstAxis, Float3& secondAxis) const
	{
		const float absX = Math::Abs(x);
		const float absY = Math::Abs(y);
		const float absZ = Math::Abs(z);
		if (absZ > absX && absZ > absY)
			firstAxis = Float3(1, 0, 0);
		else
			firstAxis = Float3(0, 0, 1);
		firstAxis = (firstAxis - *this * (firstAxis | *this)).GetNormalized();
		secondAxis = firstAxis ^ *this;
	}

	template<>
	float Float3::TriangleArea(const Float3& v0, const Float3& v1, const Float3& v2)
	{
		return ((v2 - v0) ^ (v1 - v0)).Length() * 0.5f;
	}

	template<>
	float Float3::Angle(const Float3& from, const Float3& to)
	{
		const float dot = Math::Clamp(Dot(Normalize(from), Normalize(to)), -1.0f, 1.0f);
		if (Math::Abs(dot) > 1.0f - Math::ZeroTolerance)
			return dot > 0.0f ? 0.0f : Math::PI;
		return Math::ACos(dot);
	}

// Double

	static_assert(sizeof(Double3) == 24, "Invalid Double3 type size.");

	template<>
	const Double3 Double3::Zero(0.0);
	template<>
	const Double3 Double3::One(1.0);
	template<>
	const Double3 Double3::Half(0.5);
	template<>
	const Double3 Double3::UnitX(1.0, 0.0, 0.0);
	template<>
	const Double3 Double3::UnitY(0.0, 1.0, 0.0);
	template<>
	const Double3 Double3::UnitZ(0.0, 0.0, 1.0);
	template<>
	const Double3 Double3::Up(0.0, 1.0, 0.0);
	template<>
	const Double3 Double3::Down(0.0, -1.0, 0.0);
	template<>
	const Double3 Double3::Left(-1.0, 0.0, 0.0);
	template<>
	const Double3 Double3::Right(1.0, 0.0, 0.0);
	template<>
	const Double3 Double3::Forward(0.0, 0.0, 1.0);
	template<>
	const Double3 Double3::Backward(0.0, 0.0, -1.0);
	template<>
	const Double3 Double3::Minimum(Min_double);
	template<>
	const Double3 Double3::Maximum(Max_double);

	template<>
	Double3::Vector3Base(const Float2& xy, double z)
		: x((double)xy.x)
		, y((double)xy.y)
		, z(z)
	{
	}

	template<>
	Double3::Vector3Base(const Double2& xy, double z)
		: x(xy.x)
		, y(xy.y)
		, z(0)
	{
	}

	template<>
	Double3::Vector3Base(const Int2& xy, double z)
		: x((double)xy.x)
		, y((double)xy.y)
		, z(z)
	{
	}

	template<>
	Double3::Vector3Base(const Int3& xyz)
		: x((double)xyz.x)
		, y((double)xyz.y)
		, z((double)xyz.z)
	{
	}

	template<>
	Double3::Vector3Base(const Int4& xyz)
		: x((double)xyz.x)
		, y((double)xyz.y)
		, z((double)xyz.z)
	{
	}

	template<>
	Double3::Vector3Base(const Float4& xyz)
		: x((double)xyz.x)
		, y((double)xyz.y)
		, z((double)xyz.z)
	{
	}

	template<>
	Double3::Vector3Base(const Double4& xyz)
		: x(xyz.x)
		, y(xyz.y)
		, z(xyz.z)
	{
	}

	template<>
	Double3::Vector3Base(const Color& color)
		: x((double)color.r)
		, y((double)color.g)
		, z((double)color.b)
	{
	}

	template<>
	String Double3::ToString() const
	{
		return String::Format(SE_TEXT("{}"), *this);
	}

	template<>
	void Double3::Hermite(const Double3& value1, const Double3& tangent1, const Double3& value2, const Double3& tangent2, double amount, Double3& result)
	{
		const double squared = amount * amount;
		const double cubed = amount * squared;
		const double part1 = 2.0 * cubed - 3.0 * squared + 1.0;
		const double part2 = -2.0 * cubed + 3.0 * squared;
		const double part3 = cubed - 2.0 * squared + amount;
		const double part4 = cubed - squared;
		result.x = value1.x * part1 + value2.x * part2 + tangent1.x * part3 + tangent2.x * part4;
		result.y = value1.y * part1 + value2.y * part2 + tangent1.y * part3 + tangent2.y * part4;
		result.z = value1.z * part1 + value2.z * part2 + tangent1.z * part3 + tangent2.z * part4;
	}

	template<>
	void Double3::Reflect(const Double3& vector, const Double3& normal, Double3& result)
	{
		const double dot = vector.x * normal.x + vector.y * normal.y + vector.z * normal.z;
		result.x = vector.x - 2.0 * dot * normal.x;
		result.y = vector.y - 2.0 * dot * normal.y;
		result.z = vector.z - 2.0 * dot * normal.z;
	}

	template<>
	void Double3::Transform(const Double3& vector, const Quaternion& rotation, Double3& result)
	{
		const float x = rotation.x + rotation.x;
		const float y = rotation.y + rotation.y;
		const float z = rotation.z + rotation.z;
		const float wx = rotation.w * x;
		const float wy = rotation.w * y;
		const float wz = rotation.w * z;
		const float xx = rotation.x * x;
		const float xy = rotation.x * y;
		const float xz = rotation.x * z;
		const float yy = rotation.y * y;
		const float yz = rotation.y * z;
		const float zz = rotation.z * z;
		result = Double3(
			vector.x * (1.0f - yy - zz) + vector.y * (xy - wz) + vector.z * (xz + wy),
			vector.x * (xy + wz) + vector.y * (1.0f - xx - zz) + vector.z * (yz - wx),
			vector.x * (xz - wy) + vector.y * (yz + wx) + vector.z * (1.0f - xx - yy));
	}

	template<>
	Double3 Double3::Transform(const Double3& vector, const Quaternion& rotation)
	{
		const float x = rotation.x + rotation.x;
		const float y = rotation.y + rotation.y;
		const float z = rotation.z + rotation.z;
		const float wx = rotation.w * x;
		const float wy = rotation.w * y;
		const float wz = rotation.w * z;
		const float xx = rotation.x * x;
		const float xy = rotation.x * y;
		const float xz = rotation.x * z;
		const float yy = rotation.y * y;
		const float yz = rotation.y * z;
		const float zz = rotation.z * z;
		return Double3(
			vector.x * double(1.0f - yy - zz) + vector.y * double(xy - wz) + vector.z * double(xz + wy),
			vector.x * double(xy + wz) + vector.y * double(1.0f - xx - zz) + vector.z * double(yz - wx),
			vector.x * double(xz - wy) + vector.y * double(yz + wx) + vector.z * double(1.0f - xx - yy));
		return Double3::Zero;
	}

	template<>
	void Double3::Transform(const Double3& vector, const Matrix& transform, Double4& result)
	{
		result = Double4(
			vector.x * transform.M11 + vector.y * transform.M21 + vector.z * transform.M31 + transform.M41,
			vector.x * transform.M12 + vector.y * transform.M22 + vector.z * transform.M32 + transform.M42,
			vector.x * transform.M13 + vector.y * transform.M23 + vector.z * transform.M33 + transform.M43,
			vector.x * transform.M14 + vector.y * transform.M24 + vector.z * transform.M34 + transform.M44);
	}

	template<>
	void Double3::Transform(const Double3& vector, const Matrix& transform, Double3& result)
	{
		result = Double3(
			vector.x * transform.M11 + vector.y * transform.M21 + vector.z * transform.M31 + transform.M41,
			vector.x * transform.M12 + vector.y * transform.M22 + vector.z * transform.M32 + transform.M42,
			vector.x * transform.M13 + vector.y * transform.M23 + vector.z * transform.M33 + transform.M43);
	}

	template<>
	void Double3::Transform(const Double3& vector, const Matrix3x3& transform, Double3& result)
	{
		result = Double3(
			vector.x * transform.M11 + vector.y * transform.M21 + vector.z * transform.M31,
			vector.x * transform.M12 + vector.y * transform.M22 + vector.z * transform.M32,
			vector.x * transform.M13 + vector.y * transform.M23 + vector.z * transform.M33);
	}

	template<>
	void Double3::Transform(const Double3& vector, const ::SE::Transform& transform, Double3& result)
	{
#if USE_LARGE_WORLDS
		transform.LocalToWorld(vector, result);
#else
		Float3 tmp;
		transform.LocalToWorld(vector, tmp);
		result = tmp;
#endif
	}

	template<>
	Double3 Double3::Transform(const Double3& vector, const Matrix& transform)
	{
		return Double3(
			vector.x * transform.M11 + vector.y * transform.M21 + vector.z * transform.M31 + transform.M41,
			vector.x * transform.M12 + vector.y * transform.M22 + vector.z * transform.M32 + transform.M42,
			vector.x * transform.M13 + vector.y * transform.M23 + vector.z * transform.M33 + transform.M43);
		return Double3::Zero;
	}

	template<>
	Double3 Double3::Transform(const Double3& vector, const ::SE::Transform& transform)
	{
		Float3 result;
		transform.LocalToWorld(vector, result);
		return result;
	}

	template<>
	void Double3::TransformCoordinate(const Double3& coordinate, const Matrix& transform, Double3& result)
	{
		Double4 v = Double4::Zero;
		v.x = coordinate.x * transform.M11 + coordinate.y * transform.M21 + coordinate.z * transform.M31 + transform.M41;
		v.y = coordinate.x * transform.M12 + coordinate.y * transform.M22 + coordinate.z * transform.M32 + transform.M42;
		v.z = coordinate.x * transform.M13 + coordinate.y * transform.M23 + coordinate.z * transform.M33 + transform.M43;
		v.w = 1.0 / (coordinate.x * transform.M14 + coordinate.y * transform.M24 + coordinate.z * transform.M34 + transform.M44);
		result = Double3(v.x * v.w, v.y * v.w, v.z * v.w);
	}

	template<>
	void Double3::TransformNormal(const Double3& normal, const Matrix& transform, Double3& result)
	{
		result = Double3(
			normal.x * transform.M11 + normal.y * transform.M21 + normal.z * transform.M31,
			normal.x * transform.M12 + normal.y * transform.M22 + normal.z * transform.M32,
			normal.x * transform.M13 + normal.y * transform.M23 + normal.z * transform.M33);
	}

	template<>
	Double3 Double3::Project(const Double3& vector, const Double3& onNormal)
	{
		const Float3 sqrMag = Dot(onNormal, onNormal);
		if (sqrMag < Math::ZeroTolerance)
			return Zero;
		return onNormal * Dot(vector, onNormal) / sqrMag;
	}

	template<>
	void Double3::Project(const Double3& vector, float x, float y, float width, float height, float minZ, float maxZ, const Matrix& worldViewProjection, Double3& result)
	{
		Double3 v;
		TransformCoordinate(vector, worldViewProjection, v);
		result = Double3((1.0f + v.x) * 0.5f * width + x, (1.0f - v.y) * 0.5f * height + y, v.z * (maxZ - minZ) + minZ);
	}

	template<>
	void Double3::Unproject(const Double3& vector, float x, float y, float width, float height, float minZ, float maxZ, const Matrix& worldViewProjection, Double3& result)
	{
		Matrix matrix;
		Matrix::Invert(worldViewProjection, matrix);
		const Double3 v((vector.x - x) / width * 2.0f - 1.0f, -((vector.y - y) / height * 2.0f - 1.0f), (vector.z - minZ) / (maxZ - minZ));
		TransformCoordinate(v, matrix, result);
	}

	template<>
	void Double3::CreateOrthonormalBasis(Double3& xAxis, Double3& yAxis, Double3& zAxis)
	{
		xAxis -= (xAxis | zAxis) / (zAxis | zAxis) * zAxis;
		yAxis -= (yAxis | zAxis) / (zAxis | zAxis) * zAxis;
		if (xAxis.LengthSquared() < Math::ZeroTolerance)
			xAxis = yAxis ^ zAxis;
		if (yAxis.LengthSquared() < Math::ZeroTolerance)
			yAxis = xAxis ^ zAxis;
		xAxis.Normalize();
		yAxis.Normalize();
		zAxis.Normalize();
	}

	template<>
	void Double3::FindBestAxisVectors(Double3& firstAxis, Double3& secondAxis) const
	{
		const double absX = Math::Abs(x);
		const double absY = Math::Abs(y);
		const double absZ = Math::Abs(z);
		if (absZ > absX && absZ > absY)
			firstAxis = Double3(1, 0, 0);
		else
			firstAxis = Double3(0, 0, 1);
		firstAxis = (firstAxis - *this * (firstAxis | *this)).GetNormalized();
		secondAxis = firstAxis ^ *this;
	}

	template<>
	double Double3::TriangleArea(const Double3& v0, const Double3& v1, const Double3& v2)
	{
		return ((v2 - v0) ^ (v1 - v0)).Length() * 0.5;
	}

	template<>
	double Double3::Angle(const Double3& from, const Double3& to)
	{
		const double dot = Math::Clamp(Dot(Normalize(from), Normalize(to)), -1.0, 1.0);
		if (Math::Abs(dot) > 1.0 - Math::ZeroTolerance)
			return dot > 0.0 ? 0.0 : Math::PI;
		return Math::ACos(dot);
	}

	// Int

	static_assert(sizeof(Int3) == 12, "Invalid Int3 type size.");

	template<>
	const Int3 Int3::Zero(0);
	template<>
	const Int3 Int3::One(1);
	template<>
	const Int3 Int3::Half(1);
	template<>
	const Int3 Int3::UnitX(1, 0, 0);
	template<>
	const Int3 Int3::UnitY(0, 1, 0);
	template<>
	const Int3 Int3::UnitZ(0, 0, 1);
	template<>
	const Int3 Int3::Up(0, 1, 0);
	template<>
	const Int3 Int3::Down(0, -1, 0);
	template<>
	const Int3 Int3::Left(-1, 0, 0);
	template<>
	const Int3 Int3::Right(1, 0, 0);
	template<>
	const Int3 Int3::Forward(0, 0, 1);
	template<>
	const Int3 Int3::Backward(0, 0, -1);
	template<>
	const Int3 Int3::Minimum(Min_int32);
	template<>
	const Int3 Int3::Maximum(Max_int32);

	template<>
	Int3::Vector3Base(const Float2& xy, int32 z)
		: x((int32)xy.x)
		, y((int32)xy.y)
		, z(z)
	{
	}

	template<>
	Int3::Vector3Base(const Double2& xy, int32 z)
		: x((int32)xy.x)
		, y((int32)xy.y)
		, z(0)
	{
	}

	template<>
	Int3::Vector3Base(const Int2& xy, int32 z)
		: x(xy.x)
		, y(xy.y)
		, z(z)
	{
	}

	template<>
	Int3::Vector3Base(const Int3& xyz)
		: x(xyz.x)
		, y(xyz.y)
		, z(xyz.z)
	{
	}

	template<>
	Int3::Vector3Base(const Int4& xyz)
		: x(xyz.x)
		, y(xyz.y)
		, z(xyz.z)
	{
	}

	template<>
	Int3::Vector3Base(const Float4& xyz)
		: x((int32)xyz.x)
		, y((int32)xyz.y)
		, z((int32)xyz.z)
	{
	}

	template<>
	Int3::Vector3Base(const Double4& xyz)
		: x((int32)xyz.x)
		, y((int32)xyz.y)
		, z((int32)xyz.z)
	{
	}

	template<>
	Int3::Vector3Base(const Color& color)
		: x((int32)color.r)
		, y((int32)color.g)
		, z((int32)color.b)
	{
	}

	template<>
	String Int3::ToString() const
	{
		return String::Format(SE_TEXT("{}"), *this);
	}

	template<>
	void Int3::Hermite(const Int3& value1, const Int3& tangent1, const Int3& value2, const Int3& tangent2, int32 amount, Int3& result)
	{
		result = value1;
	}

	template<>
	void Int3::Reflect(const Int3& vector, const Int3& normal, Int3& result)
	{
		result = vector;
	}

	template<>
	void Int3::Transform(const Int3& vector, const Quaternion& rotation, Int3& result)
	{
		result = vector;
	}

	template<>
	Int3 Int3::Transform(const Int3& vector, const Quaternion& rotation)
	{
		return vector;
	}

	template<>
	void Int3::Transform(const Int3& vector, const Matrix& transform, Int4& result)
	{
		result = Int4(vector);
	}

	template<>
	void Int3::Transform(const Int3& vector, const Matrix& transform, Int3& result)
	{
		result = vector;
	}

	template<>
	void Int3::Transform(const Int3& vector, const Matrix3x3& transform, Int3& result)
	{
		result = vector;
	}

	template<>
	void Int3::Transform(const Int3& vector, const ::SE::Transform& transform, Int3& result)
	{
		result = vector;
	}

	template<>
	Int3 Int3::Transform(const Int3& vector, const Matrix& transform)
	{
		return vector;
	}

	template<>
	Int3 Int3::Transform(const Int3& vector, const ::SE::Transform& transform)
	{
		return vector;
	}

	template<>
	void Int3::TransformCoordinate(const Int3& coordinate, const Matrix& transform, Int3& result)
	{
		result = coordinate;
	}

	template<>
	void Int3::TransformNormal(const Int3& normal, const Matrix& transform, Int3& result)
	{
		result = normal;
	}

	template<>
	Int3 Int3::Project(const Int3& vector, const Int3& onNormal)
	{
		return Zero;
	}

	template<>
	void Int3::Project(const Int3& vector, float x, float y, float width, float height, float minZ, float maxZ, const Matrix& worldViewProjection, Int3& result)
	{
		result = vector;
	}

	template<>
	void Int3::Unproject(const Int3& vector, float x, float y, float width, float height, float minZ, float maxZ, const Matrix& worldViewProjection, Int3& result)
	{
		result = vector;
	}

	template<>
	void Int3::CreateOrthonormalBasis(Int3& xAxis, Int3& yAxis, Int3& zAxis)
	{
	}

	template<>
	void Int3::FindBestAxisVectors(Int3& firstAxis, Int3& secondAxis) const
	{
	}

	template<>
	int32 Int3::TriangleArea(const Int3& v0, const Int3& v1, const Int3& v2)
	{
		return 0;
	}

	template<>
	int32 Int3::Angle(const Int3& from, const Int3& to)
	{
		return 0;
	}

}