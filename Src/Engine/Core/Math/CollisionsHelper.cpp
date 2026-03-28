
#include "CollisionsHelper.h"

#include "BoundingVolumes.h"
#include "Vector2.h"
#include "Vector3.h"
#include "Rectangle.h"
#include "Plane.h"
#include "Line.h"

namespace SE
{
    void CollisionsHelper::ClosestPointPointLine(const Float2& point, const Float2& p0, const Float2& p1, Float2& result)
    {
        const Float2 p = point - p0;
        Float2 n = p1 - p0;

        const float length = n.Length();
        if (length < 1e-10f)
        {
            // Both points are the same, just give any
            result = p0;
            return;
        }
        n /= length;

        const float dot = Float2::Dot(n, p);
        if (dot <= 0.0f)
        {
            // Before first point
            result = p0;
        }
        else if (dot >= length)
        {
            // After first point
            result = p1;
        }
        else
        {
            // Inside
            result = p0 + n * dot;
        }
    }

    Float2 CollisionsHelper::ClosestPointPointLine(const Float2& point, const Float2& p0, const Float2& p1)
    {
        Float2 result;
        ClosestPointPointLine(point, p0, p1, result);
        return result;
    }

    void CollisionsHelper::ClosestPointPointLine(const Float3& point, const Float3& p0, const Float3& p1, Float3& result)
    {
        const Float3 p = point - p0;
        Float3 n = p1 - p0;
        const float length = n.Length();
        if (length < 1e-10f)
        {
            result = p0;
            return;
        }
        n /= length;
        const float dot = Float3::Dot(n, p);
        if (dot <= 0.0f)
        {
            result = p0;
            return;
        }
        if (dot >= length)
        {
            result = p1;
            return;
        }
        result = p0 + n * dot;
    }

    Float3 CollisionsHelper::ClosestPointPointLine(const Float3& point, const Float3& p0, const Float3& p1)
    {
        Float3 result;
        ClosestPointPointLine(point, p0, p1, result);
        return result;
    }

    void CollisionsHelper::ClosestPointPointTriangle(const Float3& point, const Float3& vertex1, const Float3& vertex2, const Float3& vertex3, Float3& result)
    {
        // Source: float-Time Collision Detection by Christer Ericson
        // Consterence: Page 136

        // Check if P in vertex region outside A
        const Float3 ab = vertex2 - vertex1;
        const Float3 ac = vertex3 - vertex1;
        const Float3 ap = point - vertex1;

        const float d1 = Float3::Dot(ab, ap);
        const float d2 = Float3::Dot(ac, ap);
        if (d1 <= 0.0f && d2 <= 0.0f)
        {
            result = vertex1; //Barycentric coordinates (1,0,0)
            return;
        }

        // Check if P in vertex region outside B
        const Float3 bp = point - vertex2;
        const float d3 = Float3::Dot(ab, bp);
        const float d4 = Float3::Dot(ac, bp);
        if (d3 >= 0.0f && d4 <= d3)
        {
            result = vertex2; // Barycentric coordinates (0,1,0)
            return;
        }

        // Check if P in edge region of AB, if so return projection of P onto AB
        const float vc = d1 * d4 - d3 * d2;
        if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f)
        {
            const float v = d1 / (d1 - d3);
            result = vertex1 + v * ab; //Barycentric coordinates (1-v,v,0)
            return;
        }

        //Check if P in vertex region outside C
        const Float3 cp = point - vertex3;
        const float d5 = Float3::Dot(ab, cp);
        const float d6 = Float3::Dot(ac, cp);
        if (d6 >= 0.0f && d5 <= d6)
        {
            result = vertex3; //Barycentric coordinates (0,0,1)
            return;
        }

        //Check if P in edge region of AC, if so return projection of P onto AC
        const float vb = d5 * d2 - d1 * d6;
        if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f)
        {
            const float w = d2 / (d2 - d6);
            result = vertex1 + w * ac; //Barycentric coordinates (1-w,0,w)
            return;
        }

        //Check if P in edge region of BC, if so return projection of P onto BC
        const float va = d3 * d6 - d5 * d4;
        if (va <= 0.0f && d4 - d3 >= 0.0f && d5 - d6 >= 0.0f)
        {
            const float w = (d4 - d3) / (d4 - d3 + (d5 - d6));
            result = vertex2 + w * (vertex3 - vertex2); //Barycentric coordinates (0,1-w,w)
            return;
        }

        //P inside face region. Compute Q through its Barycentric coordinates (u,v,w)
        const float denom = 1.0f / (va + vb + vc);
        const float v2 = vb * denom;
        const float w2 = vc * denom;
        result = vertex1 + ab * v2 + ac * w2; //= u*vertex1 + v*vertex2 + w*vertex3, u = va * denom = 1.0f - v - w
    }

    Float3 CollisionsHelper::ClosestPointPointTriangle(const Float3& point, const Float3& vertex1, const Float3& vertex2, const Float3& vertex3)
    {
        Float3 result;
        ClosestPointPointTriangle(point, vertex1, vertex2, vertex3, result);
        return result;
    }

    void CollisionsHelper::ClosestPointPlanePoint(const Plane& plane, const Float3& point, Float3& result)
    {
        // Source: float-Time Collision Detection by Christer Ericson
        // Consterence: Page 126
        const float dot = Float3::Dot(plane.Normal, point);
        const float t = dot - plane.D;
        result = point - t * plane.Normal;
    }

    Float3 CollisionsHelper::ClosestPointPlanePoint(const Plane& plane, const Float3& point)
    {
        Float3 result;
        ClosestPointPlanePoint(plane, point, result);
        return result;
    }

    void CollisionsHelper::ClosestPointBoxPoint(const BoundingBox& box, const Float3& point, Float3& result)
    {
        // Source: float-Time Collision Detection by Christer Ericson
        // Consterence: Page 130
        Float3 temp;
        Float3::Max(point, box.Minimum, temp);
        Float3::Min(temp, box.Maximum, result);
    }

    Float3 CollisionsHelper::ClosestPointBoxPoint(const BoundingBox& box, const Float3& point)
    {
        Float3 result;
        ClosestPointBoxPoint(box, point, result);
        return result;
    }

    void CollisionsHelper::ClosestPointRectanglePoint(const Rectangle& rect, const Float2& point, Float2& result)
    {
        Float2 temp, end;
        Float2::Add(rect.Location, rect.Size, end);
        Float2::Max(point, rect.Location, temp);
        Float2::Min(temp, end, result);
    }

    Float2 CollisionsHelper::ClosestPointRectanglePoint(const Rectangle& rect, const Float2& point)
    {
        Float2 result;
        ClosestPointRectanglePoint(rect, point, result);
        return result;
    }

    void CollisionsHelper::ClosestPointSpherePoint(const BoundingSphere& sphere, const Float3& point, Float3& result)
    {
        // Source: Jorgy343
        // Consterence: None

        //Get the unit direction from the sphere's center to the point.
        Float3::Subtract(point, sphere.Center, result);
        result.Normalize();

        //Multiply the unit direction by the sphere's radius to get a vector
        //the length of the sphere.
        result *= sphere.Radius;

        //Add the sphere's center to the direction to get a point on the sphere.
        result += sphere.Center;
    }

    Float3 CollisionsHelper::ClosestPointSpherePoint(const BoundingSphere& sphere, const Float3& point)
    {
        Float3 result;
        ClosestPointSpherePoint(sphere, point, result);
        return result;
    }

    void CollisionsHelper::ClosestPointSphereSphere(const BoundingSphere& sphere1, const BoundingSphere& sphere2, Float3& result)
    {
        // Source: Jorgy343
        // Consterence: None

        //Get the unit direction from the first sphere's center to the second sphere's center.
        Float3::Subtract(sphere2.Center, sphere1.Center, result);
        result.Normalize();

        //Multiply the unit direction by the first sphere's radius to get a vector
        //the length of the first sphere.
        result *= sphere1.Radius;

        //Add the first sphere's center to the direction to get a point on the first sphere.
        result += sphere1.Center;
    }

    Float3 CollisionsHelper::ClosestPointSphereSphere(const BoundingSphere& sphere1, const BoundingSphere& sphere2)
    {
        Float3 result;
        ClosestPointSphereSphere(sphere1, sphere2, result);
        return result;
    }

    float CollisionsHelper::DistancePlanePoint(const Plane& plane, const Float3& point)
    {
        // Source: float-Time Collision Detection by Christer Ericson
        // Consterence: Page 127
        const float dot = Float3::Dot(plane.Normal, point);
        return dot - plane.D;
    }

    float CollisionsHelper::DistanceBoxPoint(const BoundingBox& box, const Float3& point)
    {
        // Source: float-Time Collision Detection by Christer Ericson
        // Consterence: Page 131

        float distance = 0.0f;

        if (point.x < box.Minimum.x)
            distance += (box.Minimum.x - point.x) * (box.Minimum.x - point.x);
        if (point.x > box.Maximum.x)
            distance += (point.x - box.Maximum.x) * (point.x - box.Maximum.x);

        if (point.y < box.Minimum.y)
            distance += (box.Minimum.y - point.y) * (box.Minimum.y - point.y);
        if (point.y > box.Maximum.y)
            distance += (point.y - box.Maximum.y) * (point.y - box.Maximum.y);

        if (point.z < box.Minimum.z)
            distance += (box.Minimum.z - point.z) * (box.Minimum.z - point.z);
        if (point.z > box.Maximum.z)
            distance += (point.z - box.Maximum.z) * (point.z - box.Maximum.z);

        return Math::Sqrt(distance);
    }

    float CollisionsHelper::DistanceBoxBox(const BoundingBox& box1, const BoundingBox& box2)
    {
        // Source:
        // Consterence:

        float distance = 0.0f;

        // Distance for X
        if (box1.Minimum.x > box2.Maximum.x)
        {
            const float delta = box2.Maximum.x - box1.Minimum.x;
            distance += delta * delta;
        }
        else if (box2.Minimum.x > box1.Maximum.x)
        {
            const float delta = box1.Maximum.x - box2.Minimum.x;
            distance += delta * delta;
        }

        // Distance for Y
        if (box1.Minimum.y > box2.Maximum.y)
        {
            const float delta = box2.Maximum.y - box1.Minimum.y;
            distance += delta * delta;
        }
        else if (box2.Minimum.y > box1.Maximum.y)
        {
            const float delta = box1.Maximum.y - box2.Minimum.y;
            distance += delta * delta;
        }

        // Distance for Z
        if (box1.Minimum.z > box2.Maximum.z)
        {
            const float delta = box2.Maximum.z - box1.Minimum.z;
            distance += delta * delta;
        }
        else if (box2.Minimum.z > box1.Maximum.z)
        {
            const float delta = box1.Maximum.z - box2.Minimum.z;
            distance += delta * delta;
        }

        return Math::Sqrt(distance);
    }

    float CollisionsHelper::DistanceSpherePoint(const BoundingSphere& sphere, const Float3& point)
    {
        // Source: Jorgy343
        // Consterence: None
        float distance = Float3::Distance(sphere.Center, point);
        distance -= sphere.Radius;
        return Math::Max<float>(distance, 0.0f);
    }

    float CollisionsHelper::DistanceSphereSphere(const BoundingSphere& sphere1, const BoundingSphere& sphere2)
    {
        // Source: Jorgy343
        // Consterence: None
        float distance = Float3::Distance(sphere1.Center, sphere2.Center);
        distance -= sphere1.Radius + sphere2.Radius;
        return Math::Max<float>(distance, 0.0f);
    }

    bool CollisionsHelper::RayIntersectsPoint(const Ray& ray, const Float3& point)
    {
        // Source: RayIntersectsSphere
        // Consterence: None

        Float3 m;
        Float3::Subtract(ray.Position, point, m);

        //Same thing as RayIntersectsSphere except that the radius of the sphere (point)
        //is the epsilon for zero.
        const float b = Float3::Dot(m, ray.Direction);
        const float c = Float3::Dot(m, m) - Math::ZeroTolerance;

        if (c > 0.0f && b > 0.0f)
            return false;

        const float discriminant = b * b - c;

        if (discriminant < 0.0f)
            return false;

        return true;
    }

    bool CollisionsHelper::RayIntersectsRay(const Ray& ray1, const Ray& ray2, Float3& point)
    {
        // Source: float-Time Rendering, Third Edition
        // Consterence: Page 780

        Float3 cross;

        Float3::Cross(ray1.Direction, ray2.Direction, cross);
        float denominator = cross.Length();

        // Lines are parallel
        if (Math::IsZero(denominator))
        {
            // Lines are parallel and on top of each other
            if (Math::IsNearEqual(ray2.Position.x, ray1.Position.x) &&
                Math::IsNearEqual(ray2.Position.y, ray1.Position.y) &&
                Math::IsNearEqual(ray2.Position.z, ray1.Position.z))
            {
                point = Float3::Zero;
                return true;
            }
        }

        denominator = denominator * denominator;

        // 3x3 matrix for the first ray
        const float m11 = ray2.Position.x - ray1.Position.x;
        const float m12 = ray2.Position.y - ray1.Position.y;
        const float m13 = ray2.Position.z - ray1.Position.z;
        float m21 = ray2.Direction.x;
        float m22 = ray2.Direction.y;
        float m23 = ray2.Direction.z;
        const float m31 = cross.x;
        const float m32 = cross.y;
        const float m33 = cross.z;

        // Determinant of first matrix
        const float dets =
                m11 * m22 * m33 +
                m12 * m23 * m31 +
                m13 * m21 * m32 -
                m11 * m23 * m32 -
                m12 * m21 * m33 -
                m13 * m22 * m31;

        // 3x3 matrix for the second ray
        m21 = ray1.Direction.x;
        m22 = ray1.Direction.y;
        m23 = ray1.Direction.z;

        // Determinant of the second matrix
        const float dett =
                m11 * m22 * m33 +
                m12 * m23 * m31 +
                m13 * m21 * m32 -
                m11 * m23 * m32 -
                m12 * m21 * m33 -
                m13 * m22 * m31;

        // t values of the point of intersection
        const float s = dets / denominator;
        const float t = dett / denominator;

        // The points of intersection.
        const Float3 point1 = ray1.Position + s * ray1.Direction;
        const Float3 point2 = ray2.Position + t * ray2.Direction;

        // If the points are not equal, no intersection has occurred
        if (!Math::IsNearEqual(point2.x, point1.x) ||
            !Math::IsNearEqual(point2.y, point1.y) ||
            !Math::IsNearEqual(point2.z, point1.z))
        {
            point = Float3::Zero;
            return false;
        }

        point = point1;
        return true;
    }

    bool CollisionsHelper::RayIntersectsPlane(const Ray& ray, const Plane& plane, float& distance)
    {
        // Source: float-Time Collision Detection by Christer Ericson
        // Consterence: Page 175

        const float direction = Float3::Dot(plane.Normal, ray.Direction);

        if (Math::IsZero(direction))
        {
            distance = 0.0f;
            return false;
        }

        const float position = Float3::Dot(plane.Normal, ray.Position);
        distance = (-plane.D - position) / direction;

        if (distance < Plane::DistanceEpsilon)
        {
            distance = 0.0f;
            return false;
        }

        return true;
    }

    bool CollisionsHelper::RayIntersectsPlane(const Ray& ray, const Plane& plane, Float3& point)
    {
        // Source: float-Time Collision Detection by Christer Ericson
        // Consterence: Page 175

        float distance;
        if (!RayIntersectsPlane(ray, plane, distance))
        {
            point = Float3::Zero;
            return false;
        }

        point = ray.Position + ray.Direction * distance;
        return true;
    }

    bool CollisionsHelper::RayIntersectsTriangle(const Ray& ray, const Float3& vertex1, const Float3& vertex2, const Float3& vertex3, float& distance)
    {
        /*
        //bool Collision::RayIntersectsTriangle(const Ray& ray, const Float3& vertex1, const Float3& vertex2, const Float3& vertex3, float& distance)
    bool Collision::RayIntersectsTriangle(const Ray& ray, const Float3& a, const Float3& b, const Float3& c, float& distance)
    {
        // todo: optimize this
        Float3 p = ray.Position;

        //int IntersectSegmentTriangle(Point p, Point q, Point a, Point b, Point c, float &u, float &v, float &w, float &t)

        Float3 ab = b - a;
        Float3 ac = c - a;
        Float3 qp = ray.Direction;

        // Compute triangle normal. Can be precalculated or cached if
        // intersecting multiple segments against the same triangle
        Float3 n = Float3::Cross(ab, ac);

        // Compute denominator d. If d <= 0, segment is parallel to or points
        // away from triangle, so exit early
        float d = Float3::Dot(qp, n);
        if (d <= 0.0f)
            return false;

        // Compute intersection t value of pq with plane of triangle. A ray
        // intersects iff 0 <= t. Segment intersects iff 0 <= t <= 1. Delay
        // dividing by d until intersection has been found to pierce triangle
        Float3 ap = p - a;
        float t = Float3::Dot(ap, n);
        if (t < 0.0f || t > d)
            return false;
        // For segment; exclude this code line for a ray test

        // Compute barycentric coordinate components and test if within bounds
        Float3 e = Float3::Cross(qp, ap);
        Float3 v = Float3::Dot(ac, e);
        if (v < 0.0f || v > d)
            return false;
        Float3 w = -Float3::Dot(ab, e);
        if (w < 0.0f || v + w > d)
            return false;

        // Segment/ray intersects triangle. Perform delayed division and
        // compute the last barycentric coordinate component
        float ood = 1.0f / d;
        t *= ood;
        v *= ood;
        w *= ood;
        Float3 u = Float3::One - v - w;
        return true;
    */

        // Source: Fast Minimum Storage Ray / Triangle Intersection
        // Consterence: http://www.cs.virginia.edu/~gfx/Courses/2003/ImageSynthesis/papers/Acceleration/Fast%20MinimumStorage%20RayTriangle%20Intersection.pdf

        distance = 0.0f;

        // Compute vectors along two edges of the triangle
        Float3 edge1, edge2;

        // Edge 1
        edge1.x = vertex2.x - vertex1.x;
        edge1.y = vertex2.y - vertex1.y;
        edge1.z = vertex2.z - vertex1.z;

        // Edge2
        edge2.x = vertex3.x - vertex1.x;
        edge2.y = vertex3.y - vertex1.y;
        edge2.z = vertex3.z - vertex1.z;

        // Cross product of ray direction and edge2 - first part of determinant
        Float3 directionCrossEdge2;
        directionCrossEdge2.x = ray.Direction.y * edge2.z - ray.Direction.z * edge2.y;
        directionCrossEdge2.y = ray.Direction.z * edge2.x - ray.Direction.x * edge2.z;
        directionCrossEdge2.z = ray.Direction.x * edge2.y - ray.Direction.y * edge2.x;

        // Compute the determinant (dot product of edge1 and the first part of determinant)
        const float determinant = edge1.x * directionCrossEdge2.x + edge1.y * directionCrossEdge2.y + edge1.z * directionCrossEdge2.z;

        // If the ray is parallel to the triangle plane, there is no collision
        // This also means that we are not culling, the ray may hit both the
        // back and the front of the triangle.
        if (Math::IsZero(determinant))
        {
            return false;
        }

        const float inverseDeterminant = 1.0f / determinant;

        // Calculate the U parameter of the intersection point
        Float3 distanceVector;
        distanceVector.x = ray.Position.x - vertex1.x;
        distanceVector.y = ray.Position.y - vertex1.y;
        distanceVector.z = ray.Position.z - vertex1.z;

        float triangleU = distanceVector.x * directionCrossEdge2.x + distanceVector.y * directionCrossEdge2.y + distanceVector.z * directionCrossEdge2.z;
        triangleU *= inverseDeterminant;

        // Make sure it is inside the triangle
        if (triangleU < 0.0f || triangleU > 1.0f)
        {
            return false;
        }

        // Calculate the V parameter of the intersection point
        Float3 distanceCrossEdge1;
        distanceCrossEdge1.x = distanceVector.y * edge1.z - distanceVector.z * edge1.y;
        distanceCrossEdge1.y = distanceVector.z * edge1.x - distanceVector.x * edge1.z;
        distanceCrossEdge1.z = distanceVector.x * edge1.y - distanceVector.y * edge1.x;

        float triangleV = ray.Direction.x * distanceCrossEdge1.x + ray.Direction.y * distanceCrossEdge1.y + ray.Direction.z * distanceCrossEdge1.z;
        triangleV *= inverseDeterminant;

        // Make sure it is inside the triangle
        if (triangleV < 0.0f || triangleU + triangleV > 1.0f)
        {
            return false;
        }

        // Compute the distance along the ray to the triangle
        float rayDistance = edge2.x * distanceCrossEdge1.x + edge2.y * distanceCrossEdge1.y + edge2.z * distanceCrossEdge1.z;
        rayDistance *= inverseDeterminant;

        // Check if the triangle is behind the ray origin
        if (rayDistance < 0.0f)
        {
            return false;
        }

        distance = rayDistance;
        return true;
    }

    bool CollisionsHelper::RayIntersectsTriangle(const Ray& ray, const Float3& vertex1, const Float3& vertex2, const Float3& vertex3, float& distance, Float3& normal)
    {
        // Source: Fast Minimum Storage Ray / Triangle Intersection
        // Consterence: http://www.cs.virginia.edu/~gfx/Courses/2003/ImageSynthesis/papers/Acceleration/Fast%20MinimumStorage%20RayTriangle%20Intersection.pdf

        distance = 0.0f;
        normal = Float3::Up;

        // Compute vectors along two edges of the triangle
        Float3 edge1, edge2;

        // Edge 1
        edge1.x = vertex2.x - vertex1.x;
        edge1.y = vertex2.y - vertex1.y;
        edge1.z = vertex2.z - vertex1.z;

        // Edge2
        edge2.x = vertex3.x - vertex1.x;
        edge2.y = vertex3.y - vertex1.y;
        edge2.z = vertex3.z - vertex1.z;

        // Cross product of ray direction and edge2 - first part of determinant
        Float3 directionCrossEdge2;
        directionCrossEdge2.x = ray.Direction.y * edge2.z - ray.Direction.z * edge2.y;
        directionCrossEdge2.y = ray.Direction.z * edge2.x - ray.Direction.x * edge2.z;
        directionCrossEdge2.z = ray.Direction.x * edge2.y - ray.Direction.y * edge2.x;

        // Compute the determinant (dot product of edge1 and the first part of determinant)
        const float determinant = edge1.x * directionCrossEdge2.x + edge1.y * directionCrossEdge2.y + edge1.z * directionCrossEdge2.z;

        // If the ray is parallel to the triangle plane, there is no collision
        // This also means that we are not culling, the ray may hit both the
        // back and the front of the triangle.
        if (Math::IsZero(determinant))
        {
            return false;
        }

        const float inverseDeterminant = 1.0f / determinant;

        // Calculate the U parameter of the intersection point
        Float3 distanceVector;
        distanceVector.x = ray.Position.x - vertex1.x;
        distanceVector.y = ray.Position.y - vertex1.y;
        distanceVector.z = ray.Position.z - vertex1.z;

        float triangleU = distanceVector.x * directionCrossEdge2.x + distanceVector.y * directionCrossEdge2.y + distanceVector.z * directionCrossEdge2.z;
        triangleU *= inverseDeterminant;

        // Make sure it is inside the triangle
        if (triangleU < 0.0f || triangleU > 1.0f)
        {
            return false;
        }

        // Calculate the V parameter of the intersection point
        Float3 distanceCrossEdge1;
        distanceCrossEdge1.x = distanceVector.y * edge1.z - distanceVector.z * edge1.y;
        distanceCrossEdge1.y = distanceVector.z * edge1.x - distanceVector.x * edge1.z;
        distanceCrossEdge1.z = distanceVector.x * edge1.y - distanceVector.y * edge1.x;

        float triangleV = ray.Direction.x * distanceCrossEdge1.x + ray.Direction.y * distanceCrossEdge1.y + ray.Direction.z * distanceCrossEdge1.z;
        triangleV *= inverseDeterminant;

        // Make sure it is inside the triangle
        if (triangleV < 0.0f || triangleU + triangleV > 1.0f)
        {
            return false;
        }

        // Compute the distance along the ray to the triangle
        float rayDistance = edge2.x * distanceCrossEdge1.x + edge2.y * distanceCrossEdge1.y + edge2.z * distanceCrossEdge1.z;
        rayDistance *= inverseDeterminant;

        // Check if the triangle is behind the ray origin
        if (rayDistance < 0.0f)
        {
            return false;
        }

        // Calculate hit normal (handle both triangle sides)
        const Float3 vd0 = vertex2 - vertex1;
        const Float3 vd1 = vertex3 - vertex1;
        const Float3 n1 = Float3::Normalize(vd0 ^ vd1);
        const Float3 n2 = Float3::Normalize(vd1 ^ vd0);
        // TODO: optimize it
        const float BiasAdjust = 0.01f;
        const Float3 center = (vertex1 + vertex2 + vertex3) * 0.333333f;
        if (Float3::DistanceSquared(center + n1 * BiasAdjust, ray.Position) < Float3::DistanceSquared(center + n2 * BiasAdjust, ray.Position))
            normal = n1;
        else
            normal = n2;
        //*normal = Float3::Normalize(vd0 ^ vd1) * -1;

        distance = rayDistance;

        return true;
    }

    bool CollisionsHelper::RayIntersectsTriangle(const Ray& ray, const Float3& vertex1, const Float3& vertex2, const Float3& vertex3, Float3& point)
    {
        float distance;
        if (!RayIntersectsTriangle(ray, vertex1, vertex2, vertex3, distance))
        {
            point = Float3::Zero;
            return false;
        }
        point = ray.Position + ray.Direction * distance;
        return true;
    }

    bool CollisionsHelper::RayIntersectsBox(const Ray& ray, const BoundingBox& box, float& distance)
    {
        // Source: float-Time Collision Detection by Christer Ericson
        // Consterence: Page 179

        distance = 0.0f;
        float tmax = Max_float;

        if (Math::IsZero(ray.Direction.x))
        {
            if (ray.Position.x < box.Minimum.x || ray.Position.x > box.Maximum.x)
            {
                distance = 0.0f;
                return false;
            }
        }
        else
        {
            const float inverse = 1.0f / ray.Direction.x;
            float t1 = (box.Minimum.x - ray.Position.x) * inverse;
            float t2 = (box.Maximum.x - ray.Position.x) * inverse;

            if (t1 > t2)
            {
                const float temp = t1;
                t1 = t2;
                t2 = temp;
            }

            distance = Math::Max(t1, distance);
            tmax = Math::Min(t2, tmax);

            if (distance > tmax)
            {
                distance = 0.0f;
                return false;
            }
        }

        if (Math::IsZero(ray.Direction.y))
        {
            if (ray.Position.y < box.Minimum.y || ray.Position.y > box.Maximum.y)
            {
                distance = 0.0f;
                return false;
            }
        }
        else
        {
            const float inverse = 1.0f / ray.Direction.y;
            float t1 = (box.Minimum.y - ray.Position.y) * inverse;
            float t2 = (box.Maximum.y - ray.Position.y) * inverse;

            if (t1 > t2)
            {
                const float temp = t1;
                t1 = t2;
                t2 = temp;
            }

            distance = Math::Max(t1, distance);
            tmax = Math::Min(t2, tmax);

            if (distance > tmax)
            {
                distance = 0.0f;
                return false;
            }
        }

        if (Math::IsZero(ray.Direction.z))
        {
            if (ray.Position.z < box.Minimum.z || ray.Position.z > box.Maximum.z)
            {
                distance = 0.0f;
                return false;
            }
        }
        else
        {
            const float inverse = 1.0f / ray.Direction.z;
            float t1 = (box.Minimum.z - ray.Position.z) * inverse;
            float t2 = (box.Maximum.z - ray.Position.z) * inverse;

            if (t1 > t2)
            {
                const float temp = t1;
                t1 = t2;
                t2 = temp;
            }

            distance = Math::Max(t1, distance);
            tmax = Math::Min(t2, tmax);

            if (distance > tmax)
            {
                distance = 0.0f;
                return false;
            }
        }

        return true;
    }

    bool CollisionsHelper::RayIntersectsBox(const Ray& ray, const BoundingBox& box, float& distance, Float3& normal)
    {
        if (!RayIntersectsBox(ray, box, distance))
        {
            normal = Float3::Up;
            return false;
        }

        // TODO: optimize this

        const Float3 point = ray.Position + ray.Direction * distance;
        Float3 size;
        Float3::Subtract(box.Maximum, box.Minimum, size);
        const Float3 center = box.Minimum + size * 0.5f;
        const Float3 localPoint = point - center;

        float dMin = Max_float;

        float d = Math::Abs(size.x - Math::Abs(localPoint.x));
        if (d < dMin)
        {
            dMin = d;
            normal = Float3(Math::Sign(localPoint.x), 0, 0);
        }

        d = Math::Abs(size.y - Math::Abs(localPoint.y));
        if (d < dMin)
        {
            dMin = d;
            normal = Float3(0, Math::Sign(localPoint.y), 0);
        }

        d = Math::Abs(size.z - Math::Abs(localPoint.z));
        if (d < dMin)
        {
            normal = Float3(0, 0, Math::Sign(localPoint.z));
        }

        return true;
    }

    bool CollisionsHelper::RayIntersectsBox(const Ray& ray, const BoundingBox& box, Float3& point)
    {
        float distance;
        if (!RayIntersectsBox(ray, box, distance))
        {
            point = Float3::Zero;
            return false;
        }

        point = ray.Position + ray.Direction * distance;
        return true;
    }

    bool CollisionsHelper::RayIntersectsSphere(const Ray& ray, const BoundingSphere& sphere, float& distance)
    {
        // Source: float-Time Collision Detection by Christer Ericson
        // Consterence: Page 177

        Float3 m;
        Float3::Subtract(ray.Position, sphere.Center, m);

        const float b = Float3::Dot(m, ray.Direction);
        const float c = Float3::Dot(m, m) - sphere.Radius * sphere.Radius;

        if (c > 0.0f && b > 0.0f)
        {
            distance = 0.0f;
            return false;
        }

        const float discriminant = b * b - c;

        if (discriminant < 0.0f)
        {
            distance = 0.0f;
            return false;
        }

        distance = -b - Math::Sqrt(discriminant);

        if (distance < 0.0f)
            distance = 0.0f;

        return true;
    }

    bool CollisionsHelper::RayIntersectsSphere(const Ray& ray, const BoundingSphere& sphere, float& distance, Float3& normal)
    {
        if (!RayIntersectsSphere(ray, sphere, distance))
        {
            normal = Float3::Up;
            return false;
        }
        const Float3 point = ray.Position + ray.Direction * distance;
        normal = Float3::Normalize(point - sphere.Center);
        return true;
    }

    bool CollisionsHelper::RayIntersectsSphere(const Ray& ray, const BoundingSphere& sphere, Float3& point)
    {
        float distance;
        if (!RayIntersectsSphere(ray, sphere, distance))
        {
            point = Float3::Zero;
            return false;
        }
        point = ray.Position + ray.Direction * distance;
        return true;
    }

    PlaneIntersectionType CollisionsHelper::PlaneIntersectsPoint(const Plane& plane, const Float3& point)
    {
        const float distance = Float3::Dot(plane.Normal, point) + plane.D;
        if (distance > Plane::DistanceEpsilon)
            return PlaneIntersectionType::Front;
        if (distance < Plane::DistanceEpsilon)
            return PlaneIntersectionType::Back;
        return PlaneIntersectionType::Intersecting;
    }

    bool CollisionsHelper::PlaneIntersectsPlane(const Plane& plane1, const Plane& plane2)
    {
        Float3 direction;
        Float3::Cross(plane1.Normal, plane2.Normal, direction);

        // If direction is the zero vector, the planes are parallel and possibly
        // coincident. It is not an intersection. The dot product will tell us.
        const float denominator = Float3::Dot(direction, direction);

        return !Math::IsZero(denominator);
    }

    bool CollisionsHelper::PlaneIntersectsPlane(const Plane& plane1, const Plane& plane2, Ray& line)
    {
        // Source: float-Time Collision Detection by Christer Ericson
        // Consterence: Page 207

        Float3 direction;
        Float3::Cross(plane1.Normal, plane2.Normal, direction);

        // If direction is the zero vector, the planes are parallel and possibly coincident. It is not an intersection. The dot product will tell us.
        const float denominator = Float3::Dot(direction, direction);

        // We assume the planes are normalized, theconstore the denominator
        // only serves as a parallel and coincident check. Otherwise we need
        // to divide the point by the denominator.
        if (Math::IsZero(denominator))
        {
            return false;
        }

        Float3 point;
        const Float3 temp = plane1.D * plane2.Normal - plane2.D * plane1.Normal;
        Float3::Cross(temp, direction, point);

        line.Position = point;
        line.Direction = direction;
        line.Direction.Normalize();

        return true;
    }

    PlaneIntersectionType CollisionsHelper::PlaneIntersectsTriangle(const Plane& plane, const Float3& vertex1, const Float3& vertex2, const Float3& vertex3)
    {
        // Source: float-Time Collision Detection by Christer Ericson
        // Consterence: Page 207

        const PlaneIntersectionType test1 = PlaneIntersectsPoint(plane, vertex1);
        const PlaneIntersectionType test2 = PlaneIntersectsPoint(plane, vertex2);
        const PlaneIntersectionType test3 = PlaneIntersectsPoint(plane, vertex3);

        if (test1 == PlaneIntersectionType::Front && test2 == PlaneIntersectionType::Front && test3 == PlaneIntersectionType::Front)
            return PlaneIntersectionType::Front;

        if (test1 == PlaneIntersectionType::Back && test2 == PlaneIntersectionType::Back && test3 == PlaneIntersectionType::Back)
            return PlaneIntersectionType::Back;

        return PlaneIntersectionType::Intersecting;
    }

    PlaneIntersectionType CollisionsHelper::PlaneIntersectsBox(const Plane& plane, const BoundingBox& box)
    {
        // Source: float-Time Collision Detection by Christer Ericson
        // Consterence: Page 161

        Float3 min;
        Float3 max;

        max.x = plane.Normal.x >= 0.0f ? box.Minimum.x : box.Maximum.x;
        max.y = plane.Normal.y >= 0.0f ? box.Minimum.y : box.Maximum.y;
        max.z = plane.Normal.z >= 0.0f ? box.Minimum.z : box.Maximum.z;
        min.x = plane.Normal.x >= 0.0f ? box.Maximum.x : box.Minimum.x;
        min.y = plane.Normal.y >= 0.0f ? box.Maximum.y : box.Minimum.y;
        min.z = plane.Normal.z >= 0.0f ? box.Maximum.z : box.Minimum.z;

        float distance = Float3::Dot(plane.Normal, max);
        if (distance + plane.D > Plane::DistanceEpsilon)
            return PlaneIntersectionType::Front;
        distance = Float3::Dot(plane.Normal, min);
        if (distance + plane.D < Plane::DistanceEpsilon)
            return PlaneIntersectionType::Back;
        return PlaneIntersectionType::Intersecting;
    }

    PlaneIntersectionType CollisionsHelper::PlaneIntersectsSphere(const Plane& plane, const BoundingSphere& sphere)
    {
        // Source: float-Time Collision Detection by Christer Ericson
        // Consterence: Page 160

        float distance = Float3::Dot(plane.Normal, sphere.Center);
        distance += plane.D;

        if (distance > sphere.Radius)
            return PlaneIntersectionType::Front;
        if (distance < -sphere.Radius)
            return PlaneIntersectionType::Back;
        return PlaneIntersectionType::Intersecting;
    }

    bool CollisionsHelper::BoxIntersectsBox(const BoundingBox& box1, const BoundingBox& box2)
    {
        if (box1.Minimum.x > box2.Maximum.x || box2.Minimum.x > box1.Maximum.x)
            return false;
        if (box1.Minimum.y > box2.Maximum.y || box2.Minimum.y > box1.Maximum.y)
            return false;
        if (box1.Minimum.z > box2.Maximum.z || box2.Minimum.z > box1.Maximum.z)
            return false;
        return true;
    }

    bool CollisionsHelper::BoxIntersectsSphere(const BoundingBox& box, const BoundingSphere& sphere)
    {
        // Source: float-Time Collision Detection by Christer Ericson
        // Consterence: Page 166

        Float3 vector;
        Float3::Clamp(sphere.Center, box.Minimum, box.Maximum, vector);
        const float distance = Float3::DistanceSquared(sphere.Center, vector);
        return distance <= sphere.Radius * sphere.Radius;
    }

    bool CollisionsHelper::SphereIntersectsTriangle(const BoundingSphere& sphere, const Float3& vertex1, const Float3& vertex2, const Float3& vertex3)
    {
        // Source: float-Time Collision Detection by Christer Ericson
        // Consterence: Page 167

        Float3 point;
        ClosestPointPointTriangle(sphere.Center, vertex1, vertex2, vertex3, point);
        const Float3 v = point - sphere.Center;

        const float dot = Float3::Dot(v, v);

        return dot <= sphere.Radius * sphere.Radius;
    }

    bool CollisionsHelper::SphereIntersectsSphere(const BoundingSphere& sphere1, const BoundingSphere& sphere2)
    {
        const float radiisum = sphere1.Radius + sphere2.Radius;
        return Float3::DistanceSquared(sphere1.Center, sphere2.Center) <= radiisum * radiisum;
    }

    ContainmentType CollisionsHelper::BoxContainsPoint(const BoundingBox& box, const Float3& point)
    {
        if (box.Minimum.x <= point.x && box.Maximum.x >= point.x &&
            box.Minimum.y <= point.y && box.Maximum.y >= point.y &&
            box.Minimum.z <= point.z && box.Maximum.z >= point.z)
        {
            return ContainmentType::Contains;
        }

        return ContainmentType::Disjoint;
    }

    ContainmentType CollisionsHelper::BoxContainsBox(const BoundingBox& box1, const BoundingBox& box2)
    {
        if (box1.Maximum.x < box2.Minimum.x || box1.Minimum.x > box2.Maximum.x)
            return ContainmentType::Disjoint;

        if (box1.Maximum.y < box2.Minimum.y || box1.Minimum.y > box2.Maximum.y)
            return ContainmentType::Disjoint;

        if (box1.Maximum.z < box2.Minimum.z || box1.Minimum.z > box2.Maximum.z)
            return ContainmentType::Disjoint;

        if (box1.Minimum.x <= box2.Minimum.x && (box2.Maximum.x <= box1.Maximum.x &&
                box1.Minimum.y <= box2.Minimum.y && box2.Maximum.y <= box1.Maximum.y) &&
            box1.Minimum.z <= box2.Minimum.z && box2.Maximum.z <= box1.Maximum.z)
        {
            return ContainmentType::Contains;
        }

        return ContainmentType::Intersects;
    }

    ContainmentType CollisionsHelper::BoxContainsSphere(const BoundingBox& box, const BoundingSphere& sphere)
    {
        Float3 vector;
        Float3::Clamp(sphere.Center, box.Minimum, box.Maximum, vector);
        const float distance = Float3::DistanceSquared(sphere.Center, vector);

        if (distance > sphere.Radius * sphere.Radius)
            return ContainmentType::Disjoint;

        if (box.Minimum.x + sphere.Radius <= sphere.Center.x && sphere.Center.x <= box.Maximum.x - sphere.Radius && (box.Maximum.x - box.Minimum.x > sphere.Radius &&
            box.Minimum.y + sphere.Radius <= sphere.Center.y) && (sphere.Center.y <= box.Maximum.y - sphere.Radius && box.Maximum.y - box.Minimum.y > sphere.Radius &&
            (box.Minimum.z + sphere.Radius <= sphere.Center.z && sphere.Center.z <= box.Maximum.z - sphere.Radius && box.Maximum.z - box.Minimum.z > sphere.Radius)))
        {
            return ContainmentType::Contains;
        }

        return ContainmentType::Intersects;
    }

    ContainmentType CollisionsHelper::SphereContainsPoint(const BoundingSphere& sphere, const Float3& point)
    {
        if (Float3::DistanceSquared(point, sphere.Center) <= sphere.Radius * sphere.Radius)
            return ContainmentType::Contains;
        return ContainmentType::Disjoint;
    }

    ContainmentType CollisionsHelper::SphereContainsTriangle(const BoundingSphere& sphere, const Float3& vertex1, const Float3& vertex2, const Float3& vertex3)
    {
        // Source: Jorgy343
        // Consterence: None

        const ContainmentType test1 = SphereContainsPoint(sphere, vertex1);
        const ContainmentType test2 = SphereContainsPoint(sphere, vertex2);
        const ContainmentType test3 = SphereContainsPoint(sphere, vertex3);

        if (test1 == ContainmentType::Contains && test2 == ContainmentType::Contains && test3 == ContainmentType::Contains)
            return ContainmentType::Contains;

        if (SphereIntersectsTriangle(sphere, vertex1, vertex2, vertex3))
            return ContainmentType::Intersects;

        return ContainmentType::Disjoint;
    }

    ContainmentType CollisionsHelper::SphereContainsBox(const BoundingSphere& sphere, const BoundingBox& box)
    {
        Float3 vector;

        if (!BoxIntersectsSphere(box, sphere))
            return ContainmentType::Disjoint;

        const float radiusSquared = sphere.Radius * sphere.Radius;

        vector.x = sphere.Center.x - box.Minimum.x;
        vector.y = sphere.Center.y - box.Maximum.y;
        vector.z = sphere.Center.z - box.Maximum.z;
        if (vector.LengthSquared() > radiusSquared)
            return ContainmentType::Intersects;

        vector.x = sphere.Center.x - box.Maximum.x;
        vector.y = sphere.Center.y - box.Maximum.y;
        vector.z = sphere.Center.z - box.Maximum.z;
        if (vector.LengthSquared() > radiusSquared)
            return ContainmentType::Intersects;

        vector.x = sphere.Center.x - box.Maximum.x;
        vector.y = sphere.Center.y - box.Minimum.y;
        vector.z = sphere.Center.z - box.Maximum.z;
        if (vector.LengthSquared() > radiusSquared)
            return ContainmentType::Intersects;

        vector.x = sphere.Center.x - box.Minimum.x;
        vector.y = sphere.Center.y - box.Minimum.y;
        vector.z = sphere.Center.z - box.Maximum.z;
        if (vector.LengthSquared() > radiusSquared)
            return ContainmentType::Intersects;

        vector.x = sphere.Center.x - box.Minimum.x;
        vector.y = sphere.Center.y - box.Maximum.y;
        vector.z = sphere.Center.z - box.Minimum.z;
        if (vector.LengthSquared() > radiusSquared)
            return ContainmentType::Intersects;

        vector.x = sphere.Center.x - box.Maximum.x;
        vector.y = sphere.Center.y - box.Maximum.y;
        vector.z = sphere.Center.z - box.Minimum.z;
        if (vector.LengthSquared() > radiusSquared)
            return ContainmentType::Intersects;

        vector.x = sphere.Center.x - box.Maximum.x;
        vector.y = sphere.Center.y - box.Minimum.y;
        vector.z = sphere.Center.z - box.Minimum.z;
        if (vector.LengthSquared() > radiusSquared)
            return ContainmentType::Intersects;

        vector.x = sphere.Center.x - box.Minimum.x;
        vector.y = sphere.Center.y - box.Minimum.y;
        vector.z = sphere.Center.z - box.Minimum.z;
        if (vector.LengthSquared() > radiusSquared)
            return ContainmentType::Intersects;

        return ContainmentType::Contains;
    }

    ContainmentType CollisionsHelper::SphereContainsSphere(const BoundingSphere& sphere1, const BoundingSphere& sphere2)
    {
        const float distance = Float3::Distance(sphere1.Center, sphere2.Center);
        if (sphere1.Radius + sphere2.Radius < distance)
            return ContainmentType::Disjoint;
        if (sphere1.Radius - sphere2.Radius < distance)
            return ContainmentType::Intersects;
        return ContainmentType::Contains;
    }

    bool CollisionsHelper::FrustumIntersectsBox(const BoundingFrustum& frustum, const BoundingBox& box)
    {
        return FrustumContainsBox(frustum, box) != ContainmentType::Disjoint;
    }

    ContainmentType CollisionsHelper::FrustumContainsBox(const BoundingFrustum& frustum, const BoundingBox& box)
    {
        auto result = ContainmentType::Contains;
        for (int32 i = 0; i < 6; i++)
        {
            Plane plane = frustum._planes[i];

            Float3 p = box.Minimum;
            if (plane.Normal.x >= 0)
                p.x = box.Maximum.x;
            if (plane.Normal.y >= 0)
                p.y = box.Maximum.y;
            if (plane.Normal.z >= 0)
                p.z = box.Maximum.z;
            if (Float3::Dot(plane.Normal, p) + plane.D < Plane::DistanceEpsilon)
                return ContainmentType::Disjoint;

            p = box.Maximum;
            if (plane.Normal.x >= 0)
                p.x = box.Minimum.x;
            if (plane.Normal.y >= 0)
                p.y = box.Minimum.y;
            if (plane.Normal.z >= 0)
                p.z = box.Minimum.z;
            if (Float3::Dot(plane.Normal, p) + plane.D < Plane::DistanceEpsilon)
                result = ContainmentType::Intersects;
        }
        return result;
    }

    bool CollisionsHelper::LineIntersectsLine(const Float2& l1p1, const Float2& l1p2, const Float2& l2p1, const Float2& l2p2)
    {
        float q = (l1p1.y - l2p1.y) * (l2p2.x - l2p1.x) - (l1p1.x - l2p1.x) * (l2p2.y - l2p1.y);
        const float d = (l1p2.x - l1p1.x) * (l2p2.y - l2p1.y) - (l1p2.y - l1p1.y) * (l2p2.x - l2p1.x);

        if (Math::IsZero(d))
            return false;

        const float r = q / d;
        q = (l1p1.y - l2p1.y) * (l1p2.x - l1p1.x) - (l1p1.x - l2p1.x) * (l1p2.y - l1p1.y);
        const float s = q / d;

        return !(r < 0 || r > 1 || s < 0 || s > 1);
    }

    bool CollisionsHelper::LineIntersectsRect(const Float2& p1, const Float2& p2, const Rectangle& rect)
    {
        /*Float2 c = rect.Location; // Box center-point
        Float2 e= rect.Size * 0.5f; // Box halflength extents
        Float2 m=(p2+p1)*0.5f; // Segment midpoint
        Float2 d=p1-m; // Segment halflength vector
        m = m - c; // Translate box and segment to origin

        // Try world coordinate axes as separating axes
        float adx = Math::Abs(d.x);
        if (Math::Abs(m.x) > e.x + adx) return false;
        float ady = Math::Abs(d.y);
        if (Math::Abs(m.y) > e.y + ady) return false;

        // Add in an epsilon term to counteract arithmetic errors when segment is
        // (near) parallel to a coordinate axis (see text for detail)
        adx += ZeroTolerance; ady += ZeroTolerance;

        // Try cross products of segment direction vector with coordinate axes
        if (Math::Abs(m.y * d.x - m.x * d.y) > e.x * ady + e.y * adx) return false;
        if (Math::Abs(m.x * d.y - m.y * d.x) > e.x * ady + e.y * adx) return false;

        // No separating axis found; segment must be overlapping BoundingBox
        return true;*/

        // TODO: optimize it
        const Float2 pA(rect.GetRight(), rect.GetY());
        const Float2 pB(rect.GetRight(), rect.GetBottom());
        const Float2 pC(rect.GetX(), rect.GetBottom());
        return LineIntersectsLine(p1, p2, rect.Location, pA) ||
                LineIntersectsLine(p1, p2, pA, pB) ||
                LineIntersectsLine(p1, p2, pB, pC) ||
                LineIntersectsLine(p1, p2, pC, rect.Location) ||
                (rect.Contains(p1) && rect.Contains(p2));

        /*float minX = Math::Min(p1.x, p2.x);
        float maxX = Math::Max(p1.x, p2.x);
        float minY = Math::Min(p1.y, p2.y);
        float maxY = Math::Max(p1.y, p2.y);*/

        /*float l = rect.GetLeft();
        float r = rect.GetRight();
        float t = rect.GetTop();
        float b = rect.GetBottom();

        // Calculate m and c for the equation for the line (y = mx+c)
        float m = (p2.y - p1.y) / (p2.x - p1.x);
        float c = p1.y - (m * p1.x);

        // if the line is going up from right to left then the top intersect point is on the left
        float top_intersection, bottom_intersection;
        if (m > 0)
        {
            top_intersection = (m*l + c);
            bottom_intersection = (m*r + c);
        }
        // otherwise it's on the right
        else
        {
            top_intersection = (m*r + c);
            bottom_intersection = (m*l + c);
        }

        // work out the top and bottom extents for the triangle
        float toptrianglepoint, bottomtrianglepoint;
        if (y0 < y1)
        {
            toptrianglepoint = p1.y;
            bottomtrianglepoint = p2.y;
        }
        else
        {
            toptrianglepoint = p2.y;
            bottomtrianglepoint = p1.y;
        }

        // and calculate the overlap between those two bounds
        float topoverlap = top_intersection > toptrianglepoint ? top_intersection : toptrianglepoint;
        float botoverlap = bottom_intersection < bottomtrianglepoint ? bottom_intersection : bottomtrianglepoint;

        // (topoverlap<botoverlap) :
        // if the intersection isn't the right way up then we have no overlap

        // (!((botoverlap<t) || (topoverlap>b)) :
        // If the bottom overlap is higher than the top of the rectangle or the top overlap is
        // lower than the bottom of the rectangle we don't have intersection. So return the negative
        // of that. Much faster than checking each of the points is within the bounds of the rectangle.
        return (topoverlap < botoverlap) && (!((botoverlap < t) || (topoverlap > b)));*/
    }

    Float2 CollisionsHelper::LineHitsBox(const Float3& lineStart, const Float3& lineEnd, const Float3& boxMin, const Float3& boxMax)
    {
        const Float3 invDirection = 1.0f / (lineEnd - lineStart);
        const Float3 enterIntersection = (boxMin - lineStart) * invDirection;
        const Float3 exitIntersection = (boxMax - lineStart) * invDirection;
        const Float3 minIntersections = Float3::Min(enterIntersection, exitIntersection);
        const Float3 maxIntersections = Float3::Max(enterIntersection, exitIntersection);
        return Float2(Math::Saturate(minIntersections.MaxValue()), Math::Saturate(maxIntersections.MinValue()));
    }

    bool CollisionsHelper::IsPointInTriangle(const Float2& point, const Float2& a, const Float2& b, const Float2& c)
    {
        const Float2 an = a - point;
        const Float2 bn = b - point;
        const Float2 cn = c - point;

        const bool orientation = Float2::Cross(an, bn) > 0;

        if (Float2::Cross(bn, cn) > 0 != orientation)
            return false;
        return Float2::Cross(cn, an) > 0 == orientation;
    }
} // SE