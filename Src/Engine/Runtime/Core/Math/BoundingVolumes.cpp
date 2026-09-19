#include "BoundingVolumes.h"
#include "Transform.h"
#include "Line.h"

#include "Runtime/Core/Types/Strings/String.h"

//-------------------------------------------------------------------------

namespace SE
{
    const BoundingBox BoundingBox::Empty = BoundingBox(Float3(Max_float), Float3(Max_float));
    const BoundingBox BoundingBox::Zero(Float3(0.0f));

    String BoundingBox::ToString() const
    {
        return String::Format(SE_TEXT("{}"), *this);
    }

    void BoundingBox::GetCorners(Float3 corners[8]) const
    {
        corners[0] = Float3((float)Minimum.X, (float)Maximum.Y, (float)Maximum.Z);
        corners[1] = Float3((float)Maximum.X, (float)Maximum.Y, (float)Maximum.Z);
        corners[2] = Float3((float)Maximum.X, (float)Minimum.Y, (float)Maximum.Z);
        corners[3] = Float3((float)Minimum.X, (float)Minimum.Y, (float)Maximum.Z);
        corners[4] = Float3((float)Minimum.X, (float)Maximum.Y, (float)Minimum.Z);
        corners[5] = Float3((float)Maximum.X, (float)Maximum.Y, (float)Minimum.Z);
        corners[6] = Float3((float)Maximum.X, (float)Minimum.Y, (float)Minimum.Z);
        corners[7] = Float3((float)Minimum.X, (float)Minimum.Y, (float)Minimum.Z);
    }

    void BoundingBox::GetCorners(Double3 corners[8]) const
    {
        corners[0] = Double3(Minimum.X, Maximum.Y, Maximum.Z);
        corners[1] = Double3(Maximum.X, Maximum.Y, Maximum.Z);
        corners[2] = Double3(Maximum.X, Minimum.Y, Maximum.Z);
        corners[3] = Double3(Minimum.X, Minimum.Y, Maximum.Z);
        corners[4] = Double3(Minimum.X, Maximum.Y, Minimum.Z);
        corners[5] = Double3(Maximum.X, Maximum.Y, Minimum.Z);
        corners[6] = Double3(Maximum.X, Minimum.Y, Minimum.Z);
        corners[7] = Double3(Minimum.X, Minimum.Y, Minimum.Z);
    }

    BoundingBox BoundingBox::MakeOffsetted(const Float3& offset) const
    {
        BoundingBox result;
        result.Minimum = Minimum + offset;
        result.Maximum = Maximum + offset;
        return result;
    }

    void BoundingBox::FromPoints(const Float3* points, int32 pointsCount, BoundingBox& result)
    {
        ASSERT(points && pointsCount > 0);
        Float3 min = points[0];
        Float3 max = points[0];
        for (int32 i = 1; i < pointsCount; i++)
        {
            Float3::Min(min, points[i], min);
            Float3::Max(max, points[i], max);
        }
        result = BoundingBox(min, max);
    }

    void BoundingBox::FromPoints(const Double3* points, int32 pointsCount, BoundingBox& result)
    {
        ASSERT(points && pointsCount > 0);
        Double3 min = points[0];
        Double3 max = points[0];
        for (int32 i = 1; i < pointsCount; i++)
        {
            Double3::Min(min, points[i], min);
            Double3::Max(max, points[i], max);
        }
        result = BoundingBox((Float3)min, (Float3)max);
    }

    void BoundingBox::FromSphere(const BoundingSphere& sphere, BoundingBox& result)
    {
        result = BoundingBox(
            Float3(sphere.Center.X - sphere.Radius, sphere.Center.Y - sphere.Radius, sphere.Center.Z - sphere.Radius),
            Float3(sphere.Center.X + sphere.Radius, sphere.Center.Y + sphere.Radius, sphere.Center.Z + sphere.Radius)
        );
    }

    BoundingBox BoundingBox::FromSphere(const BoundingSphere& sphere)
    {
        BoundingBox result;
        FromSphere(sphere, result);
        return result;
    }

    BoundingBox BoundingBox::Transform(const BoundingBox& box, const Matrix& matrix)
    {
        BoundingBox result;
        Transform(box, matrix, result);
        return result;
    }

    BoundingBox BoundingBox::MakeOffsetted(const BoundingBox& box, const Float3& offset)
    {
        BoundingBox result;
        result.Minimum = box.Minimum + offset;
        result.Maximum = box.Maximum + offset;
        return result;
    }

    BoundingBox BoundingBox::MakeScaled(const BoundingBox& box, float scale)
    {
        Float3 size;
        Float3::Subtract(box.Maximum, box.Minimum, size);
        Float3 sizeHalf = size * 0.5f;
        const Float3 center = box.Minimum + sizeHalf;
        sizeHalf = sizeHalf * scale;
        return BoundingBox(center - sizeHalf, center + sizeHalf);
    }

    void BoundingBox::Transform(const BoundingBox& box, const Matrix& matrix, BoundingBox& result)
    {
        // Reference: http://dev.theomader.com/transform-bounding-boxes/

        const auto right = matrix.GetRight();
        const auto xa = right * box.Minimum.X;
        const auto xb = right * box.Maximum.X;

        const auto up = matrix.GetUp();
        const auto ya = up * box.Minimum.Y;
        const auto yb = up * box.Maximum.Y;

        const auto backward = matrix.GetBackward();
        const auto za = backward * box.Minimum.Z;
        const auto zb = backward * box.Maximum.Z;

        const auto translation = matrix.GetTranslation();
        const auto min = Float3::Min(xa, xb) + Float3::Min(ya, yb) + Float3::Min(za, zb) + translation;
        const auto max = Float3::Max(xa, xb) + Float3::Max(ya, yb) + Float3::Max(za, zb) + translation;
        result = BoundingBox(min, max);
    }

    void BoundingBox::Transform(const BoundingBox& box, const ::SE::Transform& transform, BoundingBox& result)
    {
        // Reference: http://dev.theomader.com/transform-bounding-boxes/

        const auto right = Float3::Transform(Float3::Right, transform.Orientation);
        const auto xa = right * box.Minimum.X;
        const auto xb = right * box.Maximum.X;

        const auto up = Float3::Transform(Float3::Up, transform.Orientation);
        const auto ya = up * box.Minimum.Y;
        const auto yb = up * box.Maximum.Y;

        const auto backward = Float3::Transform(Float3::Backward, transform.Orientation);
        const auto za = backward * box.Minimum.Z;
        const auto zb = backward * box.Maximum.Z;

        const auto min = Float3::Min(xa, xb) + Float3::Min(ya, yb) + Float3::Min(za, zb) + transform.Translation;
        const auto max = Float3::Max(xa, xb) + Float3::Max(ya, yb) + Float3::Max(za, zb) + transform.Translation;
        result = BoundingBox(min, max);
    }

    bool BoundingBox::Intersects(const Ray& ray) const
    {
        float distance;
        return CollisionsHelper::RayIntersectsBox(ray, *this, distance);
    }

    bool BoundingBox::Intersects(const Ray& ray, float& distance) const
    {
        return CollisionsHelper::RayIntersectsBox(ray, *this, distance);
    }

    bool BoundingBox::Intersects(const Ray& ray, float& distance, Float3& normal) const
    {
        return CollisionsHelper::RayIntersectsBox(ray, *this, distance, normal);
    }

    bool BoundingBox::Intersects(const Ray& ray, Float3& point) const
    {
        return CollisionsHelper::RayIntersectsBox(ray, *this, point);
    }

    PlaneIntersectionType BoundingBox::Intersects(const Plane& plane) const
    {
        return CollisionsHelper::PlaneIntersectsBox(plane, *this);
    }

    bool BoundingBox::Intersects(const BoundingBox& box) const
    {
        return CollisionsHelper::BoxIntersectsBox(*this, box);
    }

    bool BoundingBox::Intersects(const BoundingSphere& sphere) const
    {
        return CollisionsHelper::BoxIntersectsSphere(*this, sphere);
    }

    ContainmentType BoundingBox::Contains(const Float3& point) const
    {
        return CollisionsHelper::BoxContainsPoint(*this, point);
    }

    ContainmentType BoundingBox::Contains(const BoundingBox& box) const
    {
        return CollisionsHelper::BoxContainsBox(*this, box);
    }

    ContainmentType BoundingBox::Contains(const BoundingSphere& sphere) const
    {
        return CollisionsHelper::BoxContainsSphere(*this, sphere);
    }

    float BoundingBox::Distance(const Float3& point) const
    {
        return CollisionsHelper::DistanceBoxPoint(*this, point);
    }

    float BoundingBox::Distance(const BoundingBox& box) const
    {
        return CollisionsHelper::DistanceBoxBox(*this, box);
    }

    //-------------------------------------------------------------------------


    //-------------------------------------------------------------------------

    String BoundingFrustum::ToString() const
    {
        return String::Format(SE_TEXT("{}"), *this);
    }

    void BoundingFrustum::SetMatrix(const Matrix& matrix)
    {
        // Set matrix
        _matrix = matrix;

        // Source:
        // http://www.chadvernon.com/blog/resources/directx9/frustum-culling/

        // Left plane
        _pLeft.Normal.X = matrix.M14 + matrix.M11;
        _pLeft.Normal.Y = matrix.M24 + matrix.M21;
        _pLeft.Normal.Z = matrix.M34 + matrix.M31;
        _pLeft.D = matrix.M44 + matrix.M41;
        _pLeft.Normalize();

        // Right plane
        _pRight.Normal.X = matrix.M14 - matrix.M11;
        _pRight.Normal.Y = matrix.M24 - matrix.M21;
        _pRight.Normal.Z = matrix.M34 - matrix.M31;
        _pRight.D = matrix.M44 - matrix.M41;
        _pRight.Normalize();

        // Top plane
        _pTop.Normal.X = matrix.M14 - matrix.M12;
        _pTop.Normal.Y = matrix.M24 - matrix.M22;
        _pTop.Normal.Z = matrix.M34 - matrix.M32;
        _pTop.D = matrix.M44 - matrix.M42;
        _pTop.Normalize();

        // Bottom plane
        _pBottom.Normal.X = matrix.M14 + matrix.M12;
        _pBottom.Normal.Y = matrix.M24 + matrix.M22;
        _pBottom.Normal.Z = matrix.M34 + matrix.M32;
        _pBottom.D = matrix.M44 + matrix.M42;
        _pBottom.Normalize();

        // Near plane
        _pNear.Normal.X = matrix.M13;
        _pNear.Normal.Y = matrix.M23;
        _pNear.Normal.Z = matrix.M33;
        _pNear.D = matrix.M43;
        _pNear.Normalize();

        // Far plane
        _pFar.Normal.X = matrix.M14 - matrix.M13;
        _pFar.Normal.Y = matrix.M24 - matrix.M23;
        _pFar.Normal.Z = matrix.M34 - matrix.M33;
        _pFar.D = matrix.M44 - matrix.M43;
        _pFar.Normalize();
    }

    Plane BoundingFrustum::GetPlane(int32 index) const
    {
        if (index > 5)
            return Plane();
        return _planes[index];
    }

    static Float3 Get3PlanesInterPoint(const Plane& p1, const Plane& p2, const Plane& p3)
    {
        const Float3 n2Xn3 = Float3::Cross(p2.Normal, p3.Normal);
        const Float3 n3Xn1 = Float3::Cross(p3.Normal, p1.Normal);
        const Float3 n1Xn2 = Float3::Cross(p1.Normal, p2.Normal);
        const float div1 = Float3::Dot(p1.Normal, n2Xn3);
        const float div2 = Float3::Dot(p2.Normal, n3Xn1);
        const float div3 = Float3::Dot(p3.Normal, n1Xn2);
        if (Math::IsZero(div1 * div2 * div3))
            return Float3::Zero;
        return n2Xn3 * (-p1.D / div1) - n3Xn1 * (p2.D / div2) - n1Xn2 * (p3.D / div3);
    }

    void BoundingFrustum::GetCorners(Float3 corners[8]) const
    {
        corners[0] = Get3PlanesInterPoint(_pNear, _pBottom, _pRight);
        corners[1] = Get3PlanesInterPoint(_pNear, _pTop, _pRight);
        corners[2] = Get3PlanesInterPoint(_pNear, _pTop, _pLeft);
        corners[3] = Get3PlanesInterPoint(_pNear, _pBottom, _pLeft);
        corners[4] = Get3PlanesInterPoint(_pFar, _pBottom, _pRight);
        corners[5] = Get3PlanesInterPoint(_pFar, _pTop, _pRight);
        corners[6] = Get3PlanesInterPoint(_pFar, _pTop, _pLeft);
        corners[7] = Get3PlanesInterPoint(_pFar, _pBottom, _pLeft);
    }

    void BoundingFrustum::GetCorners(Double3 corners[8]) const
    {
        corners[0] = Get3PlanesInterPoint(_pNear, _pBottom, _pRight);
        corners[1] = Get3PlanesInterPoint(_pNear, _pTop, _pRight);
        corners[2] = Get3PlanesInterPoint(_pNear, _pTop, _pLeft);
        corners[3] = Get3PlanesInterPoint(_pNear, _pBottom, _pLeft);
        corners[4] = Get3PlanesInterPoint(_pFar, _pBottom, _pRight);
        corners[5] = Get3PlanesInterPoint(_pFar, _pTop, _pRight);
        corners[6] = Get3PlanesInterPoint(_pFar, _pTop, _pLeft);
        corners[7] = Get3PlanesInterPoint(_pFar, _pBottom, _pLeft);
    }

    void BoundingFrustum::GetBox(BoundingBox& result) const
    {
        Float3 corners[8];
        GetCorners(corners);
        BoundingBox::FromPoints(corners, 8, result);
    }

    void BoundingFrustum::GetSphere(BoundingSphere& result) const
    {
        Float3 corners[8];
        GetCorners(corners);
        BoundingSphere::FromPoints(corners, 8, result);
    }

    float BoundingFrustum::GetWidthAtDepth(float depth) const
    {
        const float hAngle = Math::PI / 2.0f - Math::ACos((float)Float3::Dot(_pNear.Normal, _pLeft.Normal));
        return Math::Tan(hAngle) * depth * 2.0f;
    }

    float BoundingFrustum::GetHeightAtDepth(float depth) const
    {
        const float vAngle = Math::PI / 2.0f - Math::ACos((float)Float3::Dot(_pNear.Normal, _pTop.Normal));
        return Math::Tan(vAngle) * depth * 2.0f;
    }

    ContainmentType BoundingFrustum::Contains(const Float3& point) const
    {
        PlaneIntersectionType result = PlaneIntersectionType::Front;
        for (int32 i = 0; i < 6; i++)
        {
            const PlaneIntersectionType planeResult = _planes[i].Intersects(point);
            switch (planeResult)
            {
            case PlaneIntersectionType::Back:
                return ContainmentType::Disjoint;
            case PlaneIntersectionType::Intersecting:
                result = PlaneIntersectionType::Intersecting;
                break;
            }
        }
        switch (result)
        {
        case PlaneIntersectionType::Intersecting:
            return ContainmentType::Intersects;
        default:
            return ContainmentType::Contains;
        }
    }

    ContainmentType BoundingFrustum::Contains(const BoundingSphere& sphere) const
    {
        auto result = PlaneIntersectionType::Front;
        for (int32 i = 0; i < 6; i++)
        {
            const PlaneIntersectionType planeResult = _planes[i].Intersects(sphere);
            switch (planeResult)
            {
            case PlaneIntersectionType::Back:
                return ContainmentType::Disjoint;
            case PlaneIntersectionType::Intersecting:
                result = PlaneIntersectionType::Intersecting;
                break;
            }
        }
        switch (result)
        {
        case PlaneIntersectionType::Intersecting:
            return ContainmentType::Intersects;
        default:
            return ContainmentType::Contains;
        }
    }

    bool BoundingFrustum::Intersects(const BoundingSphere& sphere) const
    {
        // 没有大小的Sphere 表示无限
        if (sphere.Radius <= 0.0f)
        {
            return true;
        }

        for (int32 i = 0; i < 6; i++)
        {
            const float distance = Float3::Dot(_planes[i].Normal, sphere.Center) + _planes[i].D;
            if (distance < -sphere.Radius)
                return false;
        }
        return true;
    }

    //-------------------------------------------------------------------------

    const BoundingSphere BoundingSphere::Empty(Float3(0, 0, 0), 0);

    String BoundingSphere::ToString() const
    {
        return String::Format(SE_TEXT("{}"), *this);
    }

    bool BoundingSphere::Intersects(const Ray& ray) const
    {
        float distance;
        return CollisionsHelper::RayIntersectsSphere(ray, *this, distance);
    }

    bool BoundingSphere::Intersects(const Ray& ray, float& distance) const
    {
        return CollisionsHelper::RayIntersectsSphere(ray, *this, distance);
    }

    bool BoundingSphere::Intersects(const Ray& ray, float& distance, Float3& normal) const
    {
        return CollisionsHelper::RayIntersectsSphere(ray, *this, distance, normal);
    }

    bool BoundingSphere::Intersects(const Ray& ray, Float3& point) const
    {
        return CollisionsHelper::RayIntersectsSphere(ray, *this, point);
    }

    PlaneIntersectionType BoundingSphere::Intersects(const Plane& plane) const
    {
        return CollisionsHelper::PlaneIntersectsSphere(plane, *this);
    }

    bool BoundingSphere::Intersects(const Float3& vertex1, const Float3& vertex2, const Float3& vertex3) const
    {
        return CollisionsHelper::SphereIntersectsTriangle(*this, vertex1, vertex2, vertex3);
    }

    bool BoundingSphere::Intersects(const BoundingBox& box) const
    {
        return CollisionsHelper::BoxIntersectsSphere(box, *this);
    }

    bool BoundingSphere::Intersects(const BoundingSphere& sphere) const
    {
        return CollisionsHelper::SphereIntersectsSphere(*this, sphere);
    }

    ContainmentType BoundingSphere::Contains(const Float3& point) const
    {
        return CollisionsHelper::SphereContainsPoint(*this, point);
    }

    ContainmentType BoundingSphere::Contains(const Float3& vertex1, const Float3& vertex2, const Float3& vertex3) const
    {
        return CollisionsHelper::SphereContainsTriangle(*this, vertex1, vertex2, vertex3);
    }

    ContainmentType BoundingSphere::Contains(const BoundingBox& box) const
    {
        return CollisionsHelper::SphereContainsBox(*this, box);
    }

    ContainmentType BoundingSphere::Contains(const BoundingSphere& sphere) const
    {
        return CollisionsHelper::SphereContainsSphere(*this, sphere);
    }

    BoundingBox BoundingSphere::GetBoundingBox() const
    {
        BoundingBox result;
        BoundingBox::FromSphere(*this, result);
        return result;
    }

    void BoundingSphere::GetBoundingBox(BoundingBox& result) const
    {
        BoundingBox::FromSphere(*this, result);
    }

    void BoundingSphere::FromPoints(const Float3* points, int32 pointsCount, BoundingSphere& result)
    {
        ASSERT(points && pointsCount > 0);

        // Find the center of all points
        Float3 center = Float3::Zero;
        for (int32 i = 0; i < pointsCount; i++)
            Float3::Add(points[i], center, center);
        center /= (float)pointsCount;

        // Find the radius of the sphere
        float radius = 0.0f;
        for (int32 i = 0; i < pointsCount; i++)
        {
            const float distance = Float3::DistanceSquared(center, points[i]);
            if (distance > radius)
                radius = distance;
        }

        // Construct the sphere
        result.Center = center;
        result.Radius = Math::Sqrt(radius);
    }

    void BoundingSphere::FromPoints(const Double3* points, int32 pointsCount, BoundingSphere& result)
    {
        ASSERT(points && pointsCount > 0);

        // Find the center of all points
        Double3 center = Double3::Zero;
        for (int32 i = 0; i < pointsCount; i++)
            Double3::Add(points[i], center, center);
        center /= (double)pointsCount;

        // Find the radius of the sphere
        double radius = 0.0;
        for (int32 i = 0; i < pointsCount; i++)
        {
            const double distance = Double3::DistanceSquared(center, points[i]);
            if (distance > radius)
                radius = distance;
        }

        // Construct the sphere
        result.Center = center;
        result.Radius = (float)Math::Sqrt(radius);
    }

    void BoundingSphere::FromBox(const BoundingBox& box, BoundingSphere& result)
    {
        ASSERT(!box.Minimum.IsNanOrInfinity() && !box.Maximum.IsNanOrInfinity());
        const float x = box.Maximum.X - box.Minimum.X;
        const float y = box.Maximum.Y - box.Minimum.Y;
        const float z = box.Maximum.Z - box.Minimum.Z;
        result.Center.X = box.Minimum.X + x * 0.5f;
        result.Center.Y = box.Minimum.Y + y * 0.5f;
        result.Center.Z = box.Minimum.Z + z * 0.5f;
        result.Radius = Math::Sqrt(x * x + y * y + z * z) * 0.5f;
    }

    void BoundingSphere::Merge(const BoundingSphere& value1, const BoundingSphere& value2, BoundingSphere& result)
    {
        // Pre-exit if one of the bounding sphere by assuming that a merge with an empty sphere is equivalent at taking the non-empty sphere
        if (value1 == Empty)
        {
            result = value2;
            return;
        }
        if (value2 == Empty)
        {
            result = value1;
            return;
        }

        const Float3 difference = value2.Center - value1.Center;
        const float length = difference.Length();
        const float radius = value1.Radius;
        const float radius2 = value2.Radius;

        if (radius + radius2 >= length)
        {
            if (radius - radius2 >= length)
            {
                result = value1;
                return;
            }

            if (radius2 - radius >= length)
            {
                result = value2;
                return;
            }
        }

        const Float3 vector = difference * (1.0f / length);
        const float min = Math::Min(-radius, length - radius2);
        const float max = (Math::Max(radius, length + radius2) - min) * 0.5f;

        result.Center = value1.Center + vector * (max + min);
        result.Radius = max;
    }

    void BoundingSphere::Merge(const BoundingSphere& value1, const Float3& value2, BoundingSphere& result)
    {
        const Float3 difference = value2 - value1.Center;
        const float length = difference.Length();
        const float radius = value1.Radius;
        if (radius >= length)
        {
            result = value1;
            return;
        }

        const Float3 vector = difference * (1.0f / length);
        const float min = Math::Min(-radius, length);
        const float max = (Math::Max(radius, length) - min) * 0.5f;

        result.Center = value1.Center + vector * (max + min);
        result.Radius = max;
    }

    void BoundingSphere::Transform(const BoundingSphere& sphere, const Matrix& matrix, BoundingSphere& result)
    {
        Float3::Transform(sphere.Center, matrix, result.Center);
        result.Radius = sphere.Radius * matrix.GetScaleVector().GetAbsolute().MaxValue();
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////
    
    OrientedBoundingBox::OrientedBoundingBox(const BoundingBox& bb)
    {
        const Float3 center = bb.Minimum + (bb.Maximum - bb.Minimum) * 0.5f;
        Extents = bb.Maximum - center;
        Transformation = ::SE::Transform(center);
    }

    OrientedBoundingBox::OrientedBoundingBox(const Float3& extents, const Matrix& transformation)
        : Extents(extents)
    {
        transformation.Decompose(Transformation);
    }

    OrientedBoundingBox::OrientedBoundingBox(const Float3& extents, const Matrix3x3& rotationScale, const Float3& translation)
        : Extents(extents)
        , Transformation(translation, rotationScale)
    {
    }

    OrientedBoundingBox::OrientedBoundingBox(const Float3& minimum, const Float3& maximum)
    {
        const Float3 center = minimum + (maximum - minimum) * 0.5f;
        Extents = maximum - center;
        Transformation = ::SE::Transform(center);
    }

    OrientedBoundingBox::OrientedBoundingBox(Float3 points[], int32 pointCount)
    {
        ASSERT(points && pointCount > 0);
        Float3 minimum = points[0];
        Float3 maximum = points[0];
        for (int32 i = 1; i < pointCount; i++)
        {
            Float3::Min(minimum, points[i], minimum);
            Float3::Max(maximum, points[i], maximum);
        }
        const Float3 center = minimum + (maximum - minimum) * 0.5f;
        Extents = maximum - center;
        Transformation = ::SE::Transform(center);
    }

    String OrientedBoundingBox::ToString() const
    {
        return String::Format(SE_TEXT("{}"), *this);
    }

    void OrientedBoundingBox::GetCorners(Float3 corners[8]) const
    {
        const Float3 xv = Transformation.LocalToWorldVector(Float3((float)Extents.X, 0, 0));
        const Float3 yv = Transformation.LocalToWorldVector(Float3(0, (float)Extents.Y, 0));
        const Float3 zv = Transformation.LocalToWorldVector(Float3(0, 0, (float)Extents.Z));

        const Float3 center = Transformation.Translation;

        corners[0] = center + xv + yv + zv;
        corners[1] = center + xv + yv - zv;
        corners[2] = center - xv + yv - zv;
        corners[3] = center - xv + yv + zv;
        corners[4] = center + xv - yv + zv;
        corners[5] = center + xv - yv - zv;
        corners[6] = center - xv - yv - zv;
        corners[7] = center - xv - yv + zv;
    }

    void OrientedBoundingBox::GetCorners(Double3 corners[8]) const
    {
        const Double3 xv = Transformation.LocalToWorldVector(Float3((float)Extents.X, 0, 0));
        const Double3 yv = Transformation.LocalToWorldVector(Float3(0, (float)Extents.Y, 0));
        const Double3 zv = Transformation.LocalToWorldVector(Float3(0, 0, (float)Extents.Z));

        const Double3 center = Transformation.Translation;

        corners[0] = center + xv + yv + zv;
        corners[1] = center + xv + yv - zv;
        corners[2] = center - xv + yv - zv;
        corners[3] = center - xv + yv + zv;
        corners[4] = center + xv - yv + zv;
        corners[5] = center + xv - yv - zv;
        corners[6] = center - xv - yv - zv;
        corners[7] = center - xv - yv + zv;
    }

    Float3 OrientedBoundingBox::GetSize() const
    {
        const Float3 xv = Transformation.LocalToWorldVector(Float3(Extents.X * 2, 0, 0));
        const Float3 yv = Transformation.LocalToWorldVector(Float3(0, Extents.Y * 2, 0));
        const Float3 zv = Transformation.LocalToWorldVector(Float3(0, 0, Extents.Z * 2));
        return Float3(xv.Length(), yv.Length(), zv.Length());
    }

    Float3 OrientedBoundingBox::GetSizeSquared() const
    {
        const Float3 xv = Transformation.LocalToWorldVector(Float3(Extents.X * 2, 0, 0));
        const Float3 yv = Transformation.LocalToWorldVector(Float3(0, Extents.Y * 2, 0));
        const Float3 zv = Transformation.LocalToWorldVector(Float3(0, 0, Extents.Z * 2));
        return Float3(xv.LengthSquared(), yv.LengthSquared(), zv.LengthSquared());
    }

    BoundingBox OrientedBoundingBox::GetBoundingBox() const
    {
        BoundingBox result;
        Float3 corners[8];
        GetCorners(corners);
        BoundingBox::FromPoints(corners, 8, result);
        return result;
    }

    void OrientedBoundingBox::GetBoundingBox(BoundingBox& result) const
    {
        Float3 corners[8];
        GetCorners(corners);

        BoundingBox::FromPoints(corners, 8, result);
    }

    void OrientedBoundingBox::Transform(const Matrix& matrix)
    {
        ::SE::Transform transform;
        matrix.Decompose(transform);
        Transformation = transform.LocalToWorld(Transformation);
    }

    void OrientedBoundingBox::Transform(const ::SE::Transform& transform)
    {
        Transformation = transform.LocalToWorld(Transformation);
    }

    ContainmentType OrientedBoundingBox::Contains(const Float3& point, float* distance) const
    {
        // Transform the point into the obb coordinates
        Float3 locPoint;
        Transformation.WorldToLocal(point, locPoint);
        locPoint.X = Math::Abs(locPoint.X);
        locPoint.Y = Math::Abs(locPoint.Y);
        locPoint.Z = Math::Abs(locPoint.Z);

        if (distance)
        {
            // Get minimum distance to edge in local space
            Float3 tmp;
            Float3::Subtract(Extents, locPoint, tmp);
            const float minDstToEdgeLocal = tmp.GetAbsolute().MinValue();

            // Transform distance to world space
            Float3 dstVec = Float3::UnitX * minDstToEdgeLocal;
            Transformation.LocalToWorldVector(dstVec, dstVec);
            *distance = dstVec.Length();
        }

        // Simple axes-aligned BB check
        if (locPoint.X < Extents.X && locPoint.Y < Extents.Y && locPoint.Z < Extents.Z)
            return ContainmentType::Contains;
        if (Float3::NearEqual(locPoint, Extents))
            return ContainmentType::Intersects;
        return ContainmentType::Disjoint;
    }

    ContainmentType OrientedBoundingBox::Contains(const BoundingSphere& sphere, bool ignoreScale) const
    {
        // Transform sphere center into the obb coordinates
        Float3 locCenter;
        Transformation.WorldToLocal(sphere.Center, locCenter);

        float locRadius;
        if (ignoreScale)
        {
            locRadius = sphere.Radius;
        }
        else
        {
            // Transform sphere radius into the obb coordinates
            Float3 vRadius = Float3::UnitX * sphere.Radius;
            Transformation.LocalToWorldVector(vRadius, vRadius);
            locRadius = vRadius.Length();
        }

        // Perform regular BoundingBox to BoundingSphere containment check
        const Float3 minusExtents = -Extents;
        Float3 vector;
        Float3::Clamp(locCenter, minusExtents, Extents, vector);
        const float distance = Float3::DistanceSquared(locCenter, vector);

        if (distance > locRadius * locRadius)
            return ContainmentType::Disjoint;
        if (minusExtents.X + locRadius <= locCenter.X && locCenter.X <= Extents.X - locRadius && (Extents.X - minusExtents.X > locRadius && minusExtents.Y + locRadius <= locCenter.Y) && (locCenter.Y <= Extents.Y - locRadius && Extents.Y - minusExtents.Y > locRadius && (minusExtents.Z + locRadius <= locCenter.Z && locCenter.Z <= Extents.Z - locRadius && Extents.Z - minusExtents.Z > locRadius)))
            return ContainmentType::Contains;
        return ContainmentType::Intersects;
    }

    bool OrientedBoundingBox::Intersects(const Ray& ray, Float3& point) const
    {
        // Put ray in box space
        Ray bRay;
        Transformation.WorldToLocalVector(ray.Direction, bRay.Direction);
        Transformation.WorldToLocal(ray.Position, bRay.Position);

        // Perform a regular ray to BoundingBox check
        const BoundingBox bb(-Extents, Extents);
        const bool intersects = CollisionsHelper::RayIntersectsBox(bRay, bb, point);

        // Put the result intersection back to world
        if (intersects)
            Transformation.LocalToWorld(point, point);

        return intersects;
    }

    bool OrientedBoundingBox::Intersects(const Ray& ray, float& distance) const
    {
        Float3 point;
        const bool result = Intersects(ray, point);
        distance = Float3::Distance(ray.Position, point);
        return result;
    }

    bool OrientedBoundingBox::Intersects(const Ray& ray, float& distance, Float3& normal) const
    {
        // Put ray in box space
        Ray bRay;
        Transformation.WorldToLocalVector(ray.Direction, bRay.Direction);
        Transformation.WorldToLocal(ray.Position, bRay.Position);

        // Perform a regular ray to BoundingBox check
        const BoundingBox bb(-Extents, Extents);
        if (CollisionsHelper::RayIntersectsBox(bRay, bb, distance, normal))
        {
            // Put the result intersection back to world
            Transformation.LocalToWorldVector(normal, normal);
            normal.Normalize();
            return true;
        }

        return false;
    }

}
