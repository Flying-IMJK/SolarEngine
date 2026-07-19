#include "Line.h"

#include "Plane.h"
#include "Runtime/Core/Types/Strings/String.h"
#include "Runtime/Graphics/Viewport.h"

namespace SE
{
	Ray Ray::Identity(Float3(0, 0, 0), Float3(0, 0, 1.0f));

	String Ray::ToString() const
	{
		return String::Format(SE_TEXT("{}"), *this);
	}

	Float3 Ray::GetPoint(float distance) const
	{
		return Position + Direction * distance;
	}

	bool Ray::Intersects(const Float3& point) const
	{
		return CollisionsHelper::RayIntersectsPoint(*this, point);
	}

	bool Ray::Intersects(const Ray& ray) const
	{
		Float3 point;
		return CollisionsHelper::RayIntersectsRay(*this, ray, point);
	}

	bool Ray::Intersects(const Ray& ray, Float3& point) const
	{
		return CollisionsHelper::RayIntersectsRay(*this, ray, point);
	}

	bool Ray::Intersects(const Plane& plane) const
	{
		float distance;
		return CollisionsHelper::RayIntersectsPlane(*this, plane, distance);
	}

	bool Ray::Intersects(const Plane& plane, float& distance) const
	{
		return CollisionsHelper::RayIntersectsPlane(*this, plane, distance);
	}

	bool Ray::Intersects(const Plane& plane, Float3& point) const
	{
		return CollisionsHelper::RayIntersectsPlane(*this, plane, point);
	}

	bool Ray::Intersects(const Float3& vertex1, const Float3& vertex2, const Float3& vertex3) const
	{
		float distance;
		return CollisionsHelper::RayIntersectsTriangle(*this, vertex1, vertex2, vertex3, distance);
	}

	bool Ray::Intersects(const Float3& vertex1, const Float3& vertex2, const Float3& vertex3, float& distance) const
	{
		return CollisionsHelper::RayIntersectsTriangle(*this, vertex1, vertex2, vertex3, distance);
	}

	bool Ray::Intersects(const Float3& vertex1, const Float3& vertex2, const Float3& vertex3, Float3& point) const
	{
		return CollisionsHelper::RayIntersectsTriangle(*this, vertex1, vertex2, vertex3, point);
	}

	bool Ray::Intersects(const BoundingBox& box) const
	{
		float distance;
		return CollisionsHelper::RayIntersectsBox(*this, box, distance);
	}

	bool Ray::Intersects(const BoundingBox& box, float& distance) const
	{
		return CollisionsHelper::RayIntersectsBox(*this, box, distance);
	}

	bool Ray::Intersects(const BoundingBox& box, Float3& point) const
	{
		return CollisionsHelper::RayIntersectsBox(*this, box, point);
	}

	bool Ray::Intersects(const BoundingSphere& sphere) const
	{
		float distance;
		return CollisionsHelper::RayIntersectsSphere(*this, sphere, distance);
	}

	bool Ray::Intersects(const BoundingSphere& sphere, float& distance) const
	{
		return CollisionsHelper::RayIntersectsSphere(*this, sphere, distance);
	}

	bool Ray::Intersects(const BoundingSphere& sphere, Float3& point) const
	{
		return CollisionsHelper::RayIntersectsSphere(*this, sphere, point);
	}

	Ray Ray::GetPickRay(float x, float y, const Viewport& viewport, const Matrix& vp)
	{
		Float3 nearPoint(x, y, 0.0f);
		Float3 farPoint(x, y, 1.0f);

		nearPoint = Float3::Unproject(nearPoint, viewport.x, viewport.y, viewport.width, viewport.height, viewport.minDepth, viewport.maxDepth, vp);
		farPoint = Float3::Unproject(farPoint, viewport.x, viewport.y, viewport.width, viewport.height, viewport.minDepth, viewport.maxDepth, vp);

		Float3 direction = farPoint - nearPoint;
		direction.Normalize();

		return Ray(nearPoint, direction);
	}


}
