#include "Plane.h"
#include "Vector4.h"
#include "Quaternion.h"
#include "Matrix.h"
#include "CollisionsHelper.h"
#include "Runtime/Core/Types/Strings/String.h"

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
        D = -(Normal.X * point1.X + Normal.Y * point1.Y + Normal.Z * point1.Z);
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

        const float bc1 = inPlane1.Normal.Y * inPlane3.Normal.Z - inPlane3.Normal.Y * inPlane1.Normal.Z;
        const float bc2 = inPlane2.Normal.Y * inPlane1.Normal.Z - inPlane1.Normal.Y * inPlane2.Normal.Z;
        const float bc3 = inPlane3.Normal.Y * inPlane2.Normal.Z - inPlane2.Normal.Y * inPlane3.Normal.Z;

        const float ad1 = inPlane1.Normal.X * inPlane3.D - inPlane3.Normal.X * inPlane1.D;
        const float ad2 = inPlane2.Normal.X * inPlane1.D - inPlane1.Normal.X * inPlane2.D;
        const float ad3 = inPlane3.Normal.X * inPlane2.D - inPlane2.Normal.X * inPlane3.D;

        const float x = -(inPlane1.D * bc3 + inPlane2.D * bc1 + inPlane3.D * bc2);
        const float y = -(inPlane1.Normal.Z * ad3 + inPlane2.Normal.Z * ad1 + inPlane3.Normal.Z * ad2);
        const float z = +(inPlane1.Normal.Y * ad3 + inPlane2.Normal.Y * ad1 + inPlane3.Normal.Y * ad2);
        const float w = -(inPlane1.Normal.X * bc3 + inPlane2.Normal.X * bc1 + inPlane3.Normal.X * bc2);

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
        result.Normal.X = value.Normal.X * scale;
        result.Normal.Y = value.Normal.Y * scale;
        result.Normal.Z = value.Normal.Z * scale;
        result.D = value.D * scale;
    }

    Plane Plane::Multiply(const Plane& value, float scale)
    {
        return Plane(value.Normal * scale, value.D * scale);
    }

    void Plane::Dot(const Plane& left, const Float4& right, float& result)
    {
        result = left.Normal.X * right.X + left.Normal.Y * right.Y + left.Normal.Z * right.Z + left.D * right.W;
    }

    float Plane::Dot(const Plane& left, const Float4& right)
    {
        return left.Normal.X * right.X + left.Normal.Y * right.Y + left.Normal.Z * right.Z + left.D * right.W;
    }

    void Plane::DotCoordinate(const Plane& left, const Float3& right, float& result)
    {
        result = left.Normal.X * right.X + left.Normal.Y * right.Y + left.Normal.Z * right.Z + left.D;
    }

    float Plane::DotCoordinate(const Plane& left, const Float3& right)
    {
        return left.Normal.X * right.X + left.Normal.Y * right.Y + left.Normal.Z * right.Z + left.D;
    }

    void Plane::DotNormal(const Plane& left, const Float3& right, float& result)
    {
        result = left.Normal.X * right.X + left.Normal.Y * right.Y + left.Normal.Z * right.Z;
    }

    float Plane::DotNormal(const Plane& left, const Float3& right)
    {
        return left.Normal.X * right.X + left.Normal.Y * right.Y + left.Normal.Z * right.Z;
    }

    void Plane::Normalize(const Plane& plane, Plane& result)
    {
        const float magnitude = 1.0f / Math::Sqrt(plane.Normal.X * plane.Normal.X + plane.Normal.Y * plane.Normal.Y + plane.Normal.Z * plane.Normal.Z);
        result.Normal.X = plane.Normal.X * magnitude;
        result.Normal.Y = plane.Normal.Y * magnitude;
        result.Normal.Z = plane.Normal.Z * magnitude;
        result.D = plane.D * magnitude;
    }

    Plane Plane::Normalize(const Plane& plane)
    {
        const float magnitude = 1.0f / Math::Sqrt(plane.Normal.X * plane.Normal.X + plane.Normal.Y * plane.Normal.Y + plane.Normal.Z * plane.Normal.Z);
        return Plane(plane.Normal * magnitude, plane.D * magnitude);
    }

    void Plane::Transform(const Plane& plane, const Quaternion& rotation, Plane& result)
    {
        const float x2 = rotation.X + rotation.X;
        const float y2 = rotation.Y + rotation.Y;
        const float z2 = rotation.Z + rotation.Z;
        const float wx = rotation.W * x2;
        const float wy = rotation.W * y2;
        const float wz = rotation.W * z2;
        const float xx = rotation.X * x2;
        const float xy = rotation.X * y2;
        const float xz = rotation.X * z2;
        const float yy = rotation.Y * y2;
        const float yz = rotation.Y * z2;
        const float zz = rotation.Z * z2;

        const float x = plane.Normal.X;
        const float y = plane.Normal.Y;
        const float z = plane.Normal.Z;

        result.Normal.X = x * (1.0f - yy - zz) + y * (xy - wz) + z * (xz + wy);
        result.Normal.Y = x * (xy + wz) + y * (1.0f - xx - zz) + z * (yz - wx);
        result.Normal.Z = x * (xz - wy) + y * (yz + wx) + z * (1.0f - xx - yy);
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
        const float x = plane.Normal.X;
        const float y = plane.Normal.Y;
        const float z = plane.Normal.Z;
        const float d = plane.D;

        Matrix inverse;
        Matrix::Invert(transformation, inverse);

        result.Normal.X = x * inverse.M11 + y * inverse.M12 + z * inverse.M13 + d * inverse.M14;
        result.Normal.Y = x * inverse.M21 + y * inverse.M22 + z * inverse.M23 + d * inverse.M24;
        result.Normal.Z = x * inverse.M31 + y * inverse.M32 + z * inverse.M33 + d * inverse.M34;
        result.D = x * inverse.M41 + y * inverse.M42 + z * inverse.M43 + d * inverse.M44;
    }

    Plane Plane::Transform(const Plane& plane, const Matrix& transformation)
    {
        Plane result;
        Transform(plane, transformation, result);
        return result;
    }
}
