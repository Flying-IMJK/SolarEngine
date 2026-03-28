#include "Plane.h"
#include "Vector4.h"
#include "Quaternion.h"
#include "Matrix.h"
#include "CollisionsHelper.h"
#include "Core/Types/Strings/String.h"

namespace SE
{
    const float Plane::DistanceEpsilon = 0.0001f;
    const float Plane::NormalEpsilon = 1.0f / 65535.0f;

    Plane::Plane(const Float3& point1, const Float3& point2, const Float3& point3)
    {
        Float3 cross;

        const Float3 t1 = point2 - point1;
        const Float3 t2 = point3 - point1;

        Float3::Cross(t1, t2, cross);
        const float invPyth = cross.InvLength();

        Normal = cross * invPyth;
        D = -(Normal.x * point1.x + Normal.y * point1.y + Normal.z * point1.z);
    }

    String Plane::ToString() const
    {
        return String::Format(SE_TEXT("{}"), *this);
    }

    void Plane::Normalize()
    {
        const float length = Normal.Length();
        if (!Math::IsZero(length))
        {
            const float rcp = 1.0f / length;
            Normal *= rcp;
            D *= rcp;
        }
    }

    Float3 Plane::Intersection(const Plane& inPlane1, const Plane& inPlane2, const Plane& inPlane3)
    {
        // intersection point with 3 planes
        //  {
        //      x = -( c2*b1*d3-c2*b3*d1+b3*c1*d2+c3*b2*d1-b1*c3*d2-c1*b2*d3)/
        //           (-c2*b3*a1+c3*b2*a1-b1*c3*a2-c1*b2*a3+b3*c1*a2+c2*b1*a3), 
        //      y =  ( c3*a2*d1-c3*a1*d2-c2*a3*d1+d2*c1*a3-a2*c1*d3+c2*d3*a1)/
        //           (-c2*b3*a1+c3*b2*a1-b1*c3*a2-c1*b2*a3+b3*c1*a2+c2*b1*a3), 
        //      z = -(-a2*b1*d3+a2*b3*d1-a3*b2*d1+d3*b2*a1-d2*b3*a1+d2*b1*a3)/
        //           (-c2*b3*a1+c3*b2*a1-b1*c3*a2-c1*b2*a3+b3*c1*a2+c2*b1*a3)
        //  }

        // TODO: convet into cros products, dot products etc. ???

        const float bc1 = inPlane1.Normal.y * inPlane3.Normal.z - inPlane3.Normal.y * inPlane1.Normal.z;
        const float bc2 = inPlane2.Normal.y * inPlane1.Normal.z - inPlane1.Normal.y * inPlane2.Normal.z;
        const float bc3 = inPlane3.Normal.y * inPlane2.Normal.z - inPlane2.Normal.y * inPlane3.Normal.z;

        const float ad1 = inPlane1.Normal.x * inPlane3.D - inPlane3.Normal.x * inPlane1.D;
        const float ad2 = inPlane2.Normal.x * inPlane1.D - inPlane1.Normal.x * inPlane2.D;
        const float ad3 = inPlane3.Normal.x * inPlane2.D - inPlane2.Normal.x * inPlane3.D;

        const float x = -(inPlane1.D * bc3 + inPlane2.D * bc1 + inPlane3.D * bc2);
        const float y = -(inPlane1.Normal.z * ad3 + inPlane2.Normal.z * ad1 + inPlane3.Normal.z * ad2);
        const float z = +(inPlane1.Normal.y * ad3 + inPlane2.Normal.y * ad1 + inPlane3.Normal.y * ad2);
        const float w = -(inPlane1.Normal.x * bc3 + inPlane2.Normal.x * bc1 + inPlane3.Normal.x * bc2);

        // better to have detectable invalid values than to have reaaaaaaally big values
        if (w > -NormalEpsilon && w < NormalEpsilon)
        {
            return Float3(NAN);
        }
        return Float3(x / w, y / w, z / w);
    }

    PlaneIntersectionType Plane::Intersects(const Float3& point) const
    {
        return CollisionsHelper::PlaneIntersectsPoint(*this, point);
    }

    bool Plane::Intersects(const Ray& ray) const
    {
        float distance;
        return CollisionsHelper::RayIntersectsPlane(ray, *this, distance);
    }

    bool Plane::Intersects(const Ray& ray, float& distance) const
    {
        return CollisionsHelper::RayIntersectsPlane(ray, *this, distance);
    }

    bool Plane::Intersects(const Ray& ray, Float3& point) const
    {
        return CollisionsHelper::RayIntersectsPlane(ray, *this, point);
    }

    bool Plane::Intersects(const Plane& plane) const
    {
        return CollisionsHelper::PlaneIntersectsPlane(*this, plane);
    }

    bool Plane::Intersects(const Plane& plane, Ray& line) const
    {
        return CollisionsHelper::PlaneIntersectsPlane(*this, plane, line);
    }

    PlaneIntersectionType Plane::Intersects(const Float3& vertex1, const Float3& vertex2, const Float3& vertex3) const
    {
        return CollisionsHelper::PlaneIntersectsTriangle(*this, vertex1, vertex2, vertex3);
    }

    PlaneIntersectionType Plane::Intersects(const BoundingBox& box) const
    {
        return CollisionsHelper::PlaneIntersectsBox(*this, box);
    }

    PlaneIntersectionType Plane::Intersects(const BoundingSphere& sphere) const
    {
        return CollisionsHelper::PlaneIntersectsSphere(*this, sphere);
    }

    void Plane::Multiply(const Plane& value, float scale, Plane& result)
    {
        result.Normal.x = value.Normal.x * scale;
        result.Normal.y = value.Normal.y * scale;
        result.Normal.z = value.Normal.z * scale;
        result.D = value.D * scale;
    }

    Plane Plane::Multiply(const Plane& value, float scale)
    {
        return Plane(value.Normal * scale, value.D * scale);
    }

    void Plane::Dot(const Plane& left, const Float4& right, float& result)
    {
        result = left.Normal.x * right.x + left.Normal.y * right.y + left.Normal.z * right.z + left.D * right.w;
    }

    float Plane::Dot(const Plane& left, const Float4& right)
    {
        return left.Normal.x * right.x + left.Normal.y * right.y + left.Normal.z * right.z + left.D * right.w;
    }

    void Plane::DotCoordinate(const Plane& left, const Float3& right, float& result)
    {
        result = left.Normal.x * right.x + left.Normal.y * right.y + left.Normal.z * right.z + left.D;
    }

    float Plane::DotCoordinate(const Plane& left, const Float3& right)
    {
        return left.Normal.x * right.x + left.Normal.y * right.y + left.Normal.z * right.z + left.D;
    }

    void Plane::DotNormal(const Plane& left, const Float3& right, float& result)
    {
        result = left.Normal.x * right.x + left.Normal.y * right.y + left.Normal.z * right.z;
    }

    float Plane::DotNormal(const Plane& left, const Float3& right)
    {
        return left.Normal.x * right.x + left.Normal.y * right.y + left.Normal.z * right.z;
    }

    void Plane::Normalize(const Plane& plane, Plane& result)
    {
        const float magnitude = 1.0f / Math::Sqrt(plane.Normal.x * plane.Normal.x + plane.Normal.y * plane.Normal.y + plane.Normal.z * plane.Normal.z);
        result.Normal.x = plane.Normal.x * magnitude;
        result.Normal.y = plane.Normal.y * magnitude;
        result.Normal.z = plane.Normal.z * magnitude;
        result.D = plane.D * magnitude;
    }

    Plane Plane::Normalize(const Plane& plane)
    {
        const float magnitude = 1.0f / Math::Sqrt(plane.Normal.x * plane.Normal.x + plane.Normal.y * plane.Normal.y + plane.Normal.z * plane.Normal.z);
        return Plane(plane.Normal * magnitude, plane.D * magnitude);
    }

    void Plane::Transform(const Plane& plane, const Quaternion& rotation, Plane& result)
    {
        const float x2 = rotation.x + rotation.x;
        const float y2 = rotation.y + rotation.y;
        const float z2 = rotation.z + rotation.z;
        const float wx = rotation.w * x2;
        const float wy = rotation.w * y2;
        const float wz = rotation.w * z2;
        const float xx = rotation.x * x2;
        const float xy = rotation.x * y2;
        const float xz = rotation.x * z2;
        const float yy = rotation.y * y2;
        const float yz = rotation.y * z2;
        const float zz = rotation.z * z2;

        const float x = plane.Normal.x;
        const float y = plane.Normal.y;
        const float z = plane.Normal.z;

        result.Normal.x = x * (1.0f - yy - zz) + y * (xy - wz) + z * (xz + wy);
        result.Normal.y = x * (xy + wz) + y * (1.0f - xx - zz) + z * (yz - wx);
        result.Normal.z = x * (xz - wy) + y * (yz + wx) + z * (1.0f - xx - yy);
        result.D = plane.D;
    }

    Plane Plane::Transform(const Plane& plane, const Quaternion& rotation)
    {
        Plane result;
        Transform(plane, rotation, result);
        return result;
    }

    void Plane::Transform(const Plane& plane, const Matrix& transformation, Plane& result)
    {
        const float x = plane.Normal.x;
        const float y = plane.Normal.y;
        const float z = plane.Normal.z;
        const float d = plane.D;

        Matrix inverse;
        Matrix::Invert(transformation, inverse);

        result.Normal.x = x * inverse.M11 + y * inverse.M12 + z * inverse.M13 + d * inverse.M14;
        result.Normal.y = x * inverse.M21 + y * inverse.M22 + z * inverse.M23 + d * inverse.M24;
        result.Normal.z = x * inverse.M31 + y * inverse.M32 + z * inverse.M33 + d * inverse.M34;
        result.D = x * inverse.M41 + y * inverse.M42 + z * inverse.M43 + d * inverse.M44;
    }

    Plane Plane::Transform(const Plane& plane, const Matrix& transformation)
    {
        Plane result;
        Transform(plane, transformation, result);
        return result;
    }
}