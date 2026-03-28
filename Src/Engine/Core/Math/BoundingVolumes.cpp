#include "BoundingVolumes.h"
#include "Transform.h"

#include "Core/Types/Strings/String.h"

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
        corners[0] = Float3((float)Minimum.x, (float)Maximum.y, (float)Maximum.z);
        corners[1] = Float3((float)Maximum.x, (float)Maximum.y, (float)Maximum.z);
        corners[2] = Float3((float)Maximum.x, (float)Minimum.y, (float)Maximum.z);
        corners[3] = Float3((float)Minimum.x, (float)Minimum.y, (float)Maximum.z);
        corners[4] = Float3((float)Minimum.x, (float)Maximum.y, (float)Minimum.z);
        corners[5] = Float3((float)Maximum.x, (float)Maximum.y, (float)Minimum.z);
        corners[6] = Float3((float)Maximum.x, (float)Minimum.y, (float)Minimum.z);
        corners[7] = Float3((float)Minimum.x, (float)Minimum.y, (float)Minimum.z);
    }

    void BoundingBox::GetCorners(Double3 corners[8]) const
    {
        corners[0] = Double3(Minimum.x, Maximum.y, Maximum.z);
        corners[1] = Double3(Maximum.x, Maximum.y, Maximum.z);
        corners[2] = Double3(Maximum.x, Minimum.y, Maximum.z);
        corners[3] = Double3(Minimum.x, Minimum.y, Maximum.z);
        corners[4] = Double3(Minimum.x, Maximum.y, Minimum.z);
        corners[5] = Double3(Maximum.x, Maximum.y, Minimum.z);
        corners[6] = Double3(Maximum.x, Minimum.y, Minimum.z);
        corners[7] = Double3(Minimum.x, Minimum.y, Minimum.z);
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
            Float3(sphere.Center.x - sphere.Radius, sphere.Center.y - sphere.Radius, sphere.Center.z - sphere.Radius),
            Float3(sphere.Center.x + sphere.Radius, sphere.Center.y + sphere.Radius, sphere.Center.z + sphere.Radius)
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
        const auto xa = right * box.Minimum.x;
        const auto xb = right * box.Maximum.x;

        const auto up = matrix.GetUp();
        const auto ya = up * box.Minimum.y;
        const auto yb = up * box.Maximum.y;

        const auto backward = matrix.GetBackward();
        const auto za = backward * box.Minimum.z;
        const auto zb = backward * box.Maximum.z;

        const auto translation = matrix.GetTranslation();
        const auto min = Float3::Min(xa, xb) + Float3::Min(ya, yb) + Float3::Min(za, zb) + translation;
        const auto max = Float3::Max(xa, xb) + Float3::Max(ya, yb) + Float3::Max(za, zb) + translation;
        result = BoundingBox(min, max);
    }

    void BoundingBox::Transform(const BoundingBox& box, const ::SE::Transform& transform, BoundingBox& result)
    {
        // Reference: http://dev.theomader.com/transform-bounding-boxes/

        const auto right = Float3::Transform(Float3::Right, transform.Orientation);
        const auto xa = right * box.Minimum.x;
        const auto xb = right * box.Maximum.x;

        const auto up = Float3::Transform(Float3::Up, transform.Orientation);
        const auto ya = up * box.Minimum.y;
        const auto yb = up * box.Maximum.y;

        const auto backward = Float3::Transform(Float3::Backward, transform.Orientation);
        const auto za = backward * box.Minimum.z;
        const auto zb = backward * box.Maximum.z;

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

    /*OBB::OBB(AABB const &aabb)
        : m_Orientation(Quaternion::Identity), m_Center(aabb.GetCenter()), m_Extents(aabb.GetExtents())
    {
    }

    OBB::OBB(AABB const &aabb, Transform const &transform)
    {
        VectorSIMD const center(aabb.GetCenter());

        m_Center = transform.TransformPoint(center).ToFloat3();
        m_Orientation = transform.GetRotation();
        m_Extents = aabb.GetExtents();
    }

    OBB::OBB(VectorSIMD center, VectorSIMD extents, Quaternion orientation)
        : m_Orientation(orientation), m_Center(center), m_Extents(extents)
    {
        ENGINE_ASSERT(extents.IsGreaterThanEqual3(VectorSIMD::Zero) && orientation.IsNormalized());
    }

    // Copied from DirectXMath:
    //-----------------------------------------------------------------------------
    // Find the approximate minimum oriented bounding box containing a set of
    // points.  Exact computation of minimum oriented bounding box is possible but
    // is slower and requires a more complex algorithm.
    // The algorithm works by computing the inertia tensor of the points and then
    // using the eigenvectors of the inertia tensor as the axes of the box.
    // Computing the inertia tensor of the convex hull of the points will usually
    // result in better bounding box but the computation is more complex.
    // Exact computation of the minimum oriented bounding box is possible but the
    // best know algorithm is O(N^3) and is significantly more complex to implement.
    //
    // WARNING: this doesnt handle symmetric point clounds very well!!!!
    //-----------------------------------------------------------------------------

    OBB::OBB(VectorSIMD const *pPoints, uint32_t numPoints)
    {
        ENGINE_ASSERT(pPoints != nullptr && numPoints > 0);

        // Compute the center of mass and inertia tensor of the points.
        VectorSIMD CenterOfMass = VectorSIMD::Zero;
        for (size_t i = 0; i < numPoints; ++i)
        {
            CenterOfMass = CenterOfMass + pPoints[i];
        }

        CenterOfMass = CenterOfMass * VectorSIMD(1.0f / numPoints);

        // Compute the inertia tensor of the points around the center of mass.
        // Using the center of mass is not strictly necessary, but will hopefully
        // improve the stability of finding the eigenvectors.
        VectorSIMD XX_YY_ZZ = VectorSIMD::Zero;
        VectorSIMD XY_XZ_YZ = VectorSIMD::Zero;

        for (size_t i = 0; i < numPoints; ++i)
        {
            VectorSIMD point = pPoints[i] - CenterOfMass;
            XX_YY_ZZ = XX_YY_ZZ + (point * point);

            VectorSIMD XXY = point.Swizzle<0, 0, 1, 3>();
            VectorSIMD YZZ = point.Swizzle<1, 2, 2, 3>();

            XY_XZ_YZ = XY_XZ_YZ + (XXY * YZZ);
        }

        // Compute the eigenvectors of the inertia tensor.
        VectorSIMD v1, v2, v3;
        VectorSIMD::CalculateEigenVectorsFromCovarianceMatrix(XX_YY_ZZ.GetX(), XX_YY_ZZ.GetY(), XX_YY_ZZ.GetZ(), XY_XZ_YZ.GetX(), XY_XZ_YZ.GetY(), XY_XZ_YZ.GetZ(), v1, v2, v3);

        // Put them in a matrix.
        Matrix R;
        R.m_rows[0] = v1.GetWithW0();
        R.m_rows[1] = v2.GetWithW0();
        R.m_rows[2] = v3.GetWithW0();
        R.m_rows[3] = VectorSIMD::UnitW;

        // Multiply by -1 to convert the matrix into a right handed coordinate
        // system (determinant ~= 1) in case the eigenvectors form a left handed
        // coordinate system (determinant ~= -1) because XMQuaternionRotationMatrix only
        // works on right handed matrices.
        VectorSIMD det = R.GetDeterminant();
        if (det.IsLessThan4(VectorSIMD::Zero))
        {
            R.m_rows[0].Negate();
            R.m_rows[1].Negate();
            R.m_rows[2].Negate();
        }

        // Get the rotation quaternion from the matrix.
        Quaternion vOrientation = R.GetRotation();

        // Make sure it is normal (in case the vectors are slightly non-orthogonal).
        vOrientation.Normalize();

        // Rebuild the rotation matrix from the quaternion.
        R = Matrix(vOrientation);

        // Build the rotation into the rotated space.
        Matrix InverseR = R.GetTransposed();

        // Find the minimum OBB using the eigenvectors as the axes.
        VectorSIMD vMin, vMax;
        vMin = vMax = InverseR.RotateVector(pPoints[0]);
        for (size_t i = 1; i < numPoints; ++i)
        {
            VectorSIMD rotatedPoint = InverseR.RotateVector(pPoints[i]);
            vMin = VectorSIMD::Min(vMin, rotatedPoint);
            vMax = VectorSIMD::Max(vMax, rotatedPoint);
        }

        m_Extents = (vMax - vMin) * VectorSIMD::Half;
        m_Orientation = vOrientation;

        // Rotate the center into world space.
        VectorSIMD vCenter = vMin + m_Extents;
        vCenter = R.RotateVector(vCenter);
        m_Center = vCenter;
    }

    void OBB::ApplyTransform(Transform const &transform)
    {
        Transform currentTransform(m_Orientation, m_Center);
        currentTransform = currentTransform * transform;

        m_Center = currentTransform.GetTranslation();
        m_Orientation = currentTransform.GetRotation();
        m_Extents = (m_Extents * currentTransform.GetScale()).Abs();
    }

    void OBB::ApplyScale(VectorSIMD const &scale)
    {
        ENGINE_ASSERT(!scale.IsAnyEqualToZero3());
        m_Extents *= scale;
    }

    bool OBB::Overlaps(OBB const &box) const
    {
        // Build the 3x3 rotation matrix that defines the orientation of B relative to A.

        Quaternion Q = m_Orientation * box.m_Orientation.GetConjugate();
        Matrix R(Q);

        // Compute the translation of B relative to A.
        VectorSIMD t = m_Orientation.RotateVectorInverse(box.m_Center - m_Center);

        // h(A) = extents of A.
        // h(B) = extents of B.
        //
        // a(u) = axes of A = (1,0,0), (0,1,0), (0,0,1)
        // b(u) = axes of B relative to A = (r00,r10,r20), (r01,r11,r21), (r02,r12,r22)
        //
        // For each possible separating axis l:
        //   d(A) = sum (for i = u,v,m_w) h(A)(i) * abs( a(i) dot l )
        //   d(B) = sum (for i = u,v,m_w) h(B)(i) * abs( b(i) dot l )
        //   if abs( t dot l ) > d(A) + d(B) then disjoint
        //

        // Rows. Note R[0,1,2]X.m_w = 0.
        VectorSIMD const R0X = R[0];
        VectorSIMD const R1X = R[1];
        VectorSIMD const R2X = R[2];

        R = R.Transpose();

        // Columns. Note RX[0,1,2].m_w = 0.
        VectorSIMD const RX0 = R[0];
        VectorSIMD const RX1 = R[1];
        VectorSIMD const RX2 = R[2];

        VectorSIMD const NRX0 = RX0.GetNegated();
        VectorSIMD const NRX1 = RX1.GetNegated();
        VectorSIMD const NRX2 = RX2.GetNegated();

        // Absolute value of rows.
        VectorSIMD const AR0X = R0X.GetAbs();
        VectorSIMD const AR1X = R1X.GetAbs();
        VectorSIMD const AR2X = R2X.GetAbs();

        // Absolute value of columns.
        VectorSIMD const ARX0 = RX0.GetAbs();
        VectorSIMD const ARX1 = RX1.GetAbs();
        VectorSIMD const ARX2 = RX2.GetAbs();

        // Test each of the 15 possible separating axes.
        VectorSIMD d, d_A, d_B;

        // l = a(u) = (1, 0, 0)
        // t dot l = t.m_x
        // d(A) = h(A).m_x
        // d(B) = h(B) dot abs(r00, r01, r02)
        d = t.GetSplatX();
        d_A = m_Extents.GetSplatX();
        d_B = box.m_Extents.Dot3(AR0X);
        VectorSIMD NoIntersection = d.Abs().GreaterThan((d_A + d_B).Abs());

#define NO_INTERSECTION_TEST NoIntersection = SIMD::Int::Or(NoIntersection, d.Abs().GreaterThan((d_A + d_B).Abs()));

        // l = a(v) = (0, 1, 0)
        // t dot l = t.m_y
        // d(A) = h(A).m_y
        // d(B) = h(B) dot abs(r10, r11, r12)
        d = t.GetSplatY();
        d_A = m_Extents.GetSplatY();
        d_B = box.m_Extents.Dot3(AR1X);
        NO_INTERSECTION_TEST

        // l = a(m_w) = (0, 0, 1)
        // t dot l = t.m_z
        // d(A) = h(A).m_z
        // d(B) = h(B) dot abs(r20, r21, r22)
        d = t.GetSplatZ();
        d_A = m_Extents.GetSplatZ();
        d_B = box.m_Extents.Dot3(AR2X);
        NO_INTERSECTION_TEST

        // l = b(u) = (r00, r10, r20)
        // d(A) = h(A) dot abs(r00, r10, r20)
        // d(B) = h(B).m_x
        d = t.Dot3(RX0);
        d_A = m_Extents.Dot3(ARX0);
        d_B = box.m_Extents.GetSplatX();
        NO_INTERSECTION_TEST

        // l = b(v) = (r01, r11, r21)
        // d(A) = h(A) dot abs(r01, r11, r21)
        // d(B) = h(B).m_y
        d = t.Dot3(RX1);
        d_A = m_Extents.Dot3(ARX1);
        d_B = box.m_Extents.GetSplatY();
        NO_INTERSECTION_TEST

        // l = b(m_w) = (r02, r12, r22)
        // d(A) = h(A) dot abs(r02, r12, r22)
        // d(B) = h(B).m_z
        d = t.Dot3(RX2);
        d_A = m_Extents.Dot3(ARX2);
        d_B = box.m_Extents.GetSplatZ();
        NO_INTERSECTION_TEST

        // l = a(u) m_x b(u) = (0, -r20, r10)
        // d(A) = h(A) dot abs(0, r20, r10)
        // d(B) = h(B) dot abs(0, r02, r01)
        d = t.Dot3(VectorSIMD::Permute<3, 6, 1, 0>(RX0, NRX0));
        d_A = m_Extents.Dot3(ARX0.Swizzle<3, 2, 1, 0>());
        d_B = box.m_Extents.Dot3(AR0X.Swizzle<3, 2, 1, 0>());
        NO_INTERSECTION_TEST

        // l = a(u) m_x b(v) = (0, -r21, r11)
        // d(A) = h(A) dot abs(0, r21, r11)
        // d(B) = h(B) dot abs(r02, 0, r00)
        d = t.Dot3(VectorSIMD::Permute<3, 6, 1, 0>(RX1, NRX1));
        d_A = m_Extents.Dot3(ARX1.Swizzle<3, 2, 1, 0>());
        d_B = box.m_Extents.Dot3(AR0X.Swizzle<2, 3, 0, 1>());
        NO_INTERSECTION_TEST

        // l = a(u) m_x b(m_w) = (0, -r22, r12)
        // d(A) = h(A) dot abs(0, r22, r12)
        // d(B) = h(B) dot abs(r01, r00, 0)
        d = t.Dot3(VectorSIMD::Permute<3, 6, 1, 0>(RX2, NRX2));
        d_A = m_Extents.Dot3(ARX2.Swizzle<3, 2, 1, 0>());
        d_B = box.m_Extents.Dot3(AR0X.Swizzle<1, 0, 3, 2>());
        NO_INTERSECTION_TEST

        // l = a(v) m_x b(u) = (r20, 0, -r00)
        // d(A) = h(A) dot abs(r20, 0, r00)
        // d(B) = h(B) dot abs(0, r12, r11)
        d = t.Dot3(VectorSIMD::Permute<2, 3, 4, 1>(RX0, NRX0));
        d_A = m_Extents.Dot3(ARX0.Swizzle<2, 3, 0, 1>());
        d_B = box.m_Extents.Dot3(AR1X.Swizzle<3, 2, 1, 0>());

        // l = a(v) m_x b(v) = (r21, 0, -r01)
        // d(A) = h(A) dot abs(r21, 0, r01)
        // d(B) = h(B) dot abs(r12, 0, r10)
        d = t.Dot3(VectorSIMD::Permute<2, 3, 4, 1>(RX1, NRX1));
        d_A = m_Extents.Dot3(ARX1.Swizzle<2, 3, 0, 1>());
        d_B = box.m_Extents.Dot3(AR1X.Swizzle<2, 3, 0, 1>());
        NO_INTERSECTION_TEST

        // l = a(v) m_x b(m_w) = (r22, 0, -r02)
        // d(A) = h(A) dot abs(r22, 0, r02)
        // d(B) = h(B) dot abs(r11, r10, 0)
        d = t.Dot3(VectorSIMD::Permute<2, 3, 4, 1>(RX2, NRX2));
        d_A = m_Extents.Dot3(ARX2.Swizzle<2, 3, 0, 1>());
        d_B = box.m_Extents.Dot3(AR1X.Swizzle<1, 0, 3, 2>());
        NO_INTERSECTION_TEST

        // l = a(m_w) m_x b(u) = (-r10, r00, 0)
        // d(A) = h(A) dot abs(r10, r00, 0)
        // d(B) = h(B) dot abs(0, r22, r21)
        d = t.Dot3(VectorSIMD::Permute<5, 0, 3, 2>(RX0, NRX0));
        d_A = m_Extents.Dot3(ARX0.Swizzle<1, 0, 3, 2>());
        d_B = box.m_Extents.Dot3(AR2X.Swizzle<3, 2, 1, 0>());
        NO_INTERSECTION_TEST

        // l = a(m_w) m_x b(v) = (-r11, r01, 0)
        // d(A) = h(A) dot abs(r11, r01, 0)
        // d(B) = h(B) dot abs(r22, 0, r20)
        d = t.Dot3(VectorSIMD::Permute<5, 0, 3, 2>(RX1, NRX1));
        d_A = m_Extents.Dot3(ARX1.Swizzle<1, 0, 3, 2>());
        d_B = box.m_Extents.Dot3(AR2X.Swizzle<2, 3, 0, 1>());
        NO_INTERSECTION_TEST

        // l = a(m_w) m_x b(m_w) = (-r12, r02, 0)
        // d(A) = h(A) dot abs(r12, r02, 0)
        // d(B) = h(B) dot abs(r21, r20, 0)
        d = t.Dot3(VectorSIMD::Permute<5, 0, 3, 2>(RX2, NRX2));
        d_A = m_Extents.Dot3(ARX2.Swizzle<1, 0, 3, 2>());
        d_B = box.m_Extents.Dot3(AR2X.Swizzle<1, 0, 3, 2>());
        NO_INTERSECTION_TEST

#undef NO_INTERSECTION_TEST

        // No separating axis found, boxes must intersect.
        return SIMD::Int::NotEqual(NoIntersection, SIMD::g_trueMask) ? true : false;
    }*/

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
        _pLeft.Normal.x = matrix.M14 + matrix.M11;
        _pLeft.Normal.y = matrix.M24 + matrix.M21;
        _pLeft.Normal.z = matrix.M34 + matrix.M31;
        _pLeft.D = matrix.M44 + matrix.M41;
        _pLeft.Normalize();

        // Right plane
        _pRight.Normal.x = matrix.M14 - matrix.M11;
        _pRight.Normal.y = matrix.M24 - matrix.M21;
        _pRight.Normal.z = matrix.M34 - matrix.M31;
        _pRight.D = matrix.M44 - matrix.M41;
        _pRight.Normalize();

        // Top plane
        _pTop.Normal.x = matrix.M14 - matrix.M12;
        _pTop.Normal.y = matrix.M24 - matrix.M22;
        _pTop.Normal.z = matrix.M34 - matrix.M32;
        _pTop.D = matrix.M44 - matrix.M42;
        _pTop.Normalize();

        // Bottom plane
        _pBottom.Normal.x = matrix.M14 + matrix.M12;
        _pBottom.Normal.y = matrix.M24 + matrix.M22;
        _pBottom.Normal.z = matrix.M34 + matrix.M32;
        _pBottom.D = matrix.M44 + matrix.M42;
        _pBottom.Normalize();

        // Near plane
        _pNear.Normal.x = matrix.M13;
        _pNear.Normal.y = matrix.M23;
        _pNear.Normal.z = matrix.M33;
        _pNear.D = matrix.M43;
        _pNear.Normalize();

        // Far plane
        _pFar.Normal.x = matrix.M14 - matrix.M13;
        _pFar.Normal.y = matrix.M24 - matrix.M23;
        _pFar.Normal.z = matrix.M34 - matrix.M33;
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
        const float x = box.Maximum.x - box.Minimum.x;
        const float y = box.Maximum.y - box.Minimum.y;
        const float z = box.Maximum.z - box.Minimum.z;
        result.Center.x = box.Minimum.x + x * 0.5f;
        result.Center.y = box.Minimum.y + y * 0.5f;
        result.Center.z = box.Minimum.z + z * 0.5f;
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
}