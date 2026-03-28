#pragma once

#include "Math.h"
#include "Matrix.h"
#include "Plane.h"

namespace SE
{
    struct Ray;
    enum class OverlapResult
    {
        NoOverlap,
        Overlap,
        FullyEnclosed,
    };

    //-------------------------------------------------------------------------
    struct BoundingBox;
    struct BoundingSphere;
    struct Plane;

    //-------------------------------------------------------------------------
    // Axis Aligned Bounding Box
    //-------------------------------------------------------------------------

    struct SE_API_CORE BoundingBox
    {
    public:
        /// <summary>
        /// A <see cref="AABB"/> which represents an empty space.
        /// </summary>
        static const BoundingBox Empty;

        /// <summary>
        /// A <see cref="AABB"/> located at zero point with zero size.
        /// </summary>
        static const BoundingBox Zero;

    public:
        /// <summary>
        /// The minimum point of the box.
        /// </summary>
        Float3 Minimum;

        /// <summary>
        /// The maximum point of the box.
        /// </summary>
        Float3 Maximum;

    public:
        /// <summary>
        /// Empty constructor.
        /// </summary>
        BoundingBox() = default;

        /// <summary>
        /// Initializes a new instance of the <see cref="AABB"/> struct.
        /// </summary>
        /// <param name="point">The location of the empty bounding box.</param>
        BoundingBox(const Float3& point)
            : Minimum(point)
            , Maximum(point)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="AABB"/> struct.
        /// </summary>
        /// <param name="minimum">The minimum vertex of the bounding box.</param>
        /// <param name="maximum">The maximum vertex of the bounding box.</param>
        BoundingBox(const Float3& minimum, const Float3& maximum)
            : Minimum(minimum)
            , Maximum(maximum)
        {
        }

    public:
        String ToString() const;

    public:
        /// <summary>
        /// Gets the eight corners of the bounding box.
        /// </summary>
        /// <param name="corners">An array of points representing the eight corners of the bounding box.</param>
        void GetCorners(Float3 corners[8]) const;

        /// <summary>
        /// Gets the eight corners of the bounding box.
        /// </summary>
        /// <param name="corners">An array of points representing the eight corners of the bounding box.</param>
        void GetCorners(Double3 corners[8]) const;

        /// <summary>
        /// Calculates volume of the box.
        /// </summary>
        /// <returns>The box volume.</returns>
        float GetVolume() const
        {
            Float3 size;
            Float3::Subtract(Maximum, Minimum, size);
            return size.x * size.y * size.z;
        }

        /// <summary>
        /// Calculates the size of the box.
        /// </summary>
        /// <returns>The box size.</returns>
        Float3 GetSize() const
        {
            Float3 size;
            Float3::Subtract(Maximum, Minimum, size);
            return size;
        }

        /// <summary>
        /// Calculates the size of the box.
        /// </summary>
        /// <param name="result">The result box size.</param>
        void GetSize(Float3& result) const
        {
            Float3::Subtract(Maximum, Minimum, result);
        }

        /// <summary>
        /// Sets the size of the box.
        /// </summary>
        /// <param name="value">The box size to set.</param>
        void SetSize(const Float3& value)
        {
            Float3 center;
            GetCenter(center);
            const Float3 sizeHalf = value * 0.5f;
            Minimum = center - sizeHalf;
            Maximum = center + sizeHalf;
        }

        /// <summary>
        /// Gets the center point location.
        /// </summary>
        /// <returns>The box center.</returns>
        Float3 GetCenter() const
        {
            return Minimum + (Maximum - Minimum) * 0.5f;
        }

        /// <summary>
        /// Gets the center point location.
        /// </summary>
        /// <param name="result">The result box center.</param>
        void GetCenter(Float3& result) const
        {
            result = Minimum + (Maximum - Minimum) * 0.5f;
        }

        /// <summary>
        /// Sets the center point location.
        /// </summary>
        /// <param name="value">The box center to set.</param>
        void SetCenter(const Float3& value)
        {
            const Float3 sizeHalf = GetSize() * 0.5f;
            Minimum = value - sizeHalf;
            Maximum = value + sizeHalf;
        }

    public:
        static bool NearEqual(const BoundingBox& a, const BoundingBox& b)
        {
            return Float3::NearEqual(a.Minimum, b.Minimum) && Float3::NearEqual(a.Maximum, b.Maximum);
        }

        static bool NearEqual(const BoundingBox& a, const BoundingBox& b, float epsilon)
        {
            return Float3::NearEqual(a.Minimum, b.Minimum, epsilon) && Float3::NearEqual(a.Maximum, b.Maximum, epsilon);
        }

    public:
        /// <summary>
        /// Merges the box with a point.
        /// </summary>
        /// <param name="point">The point to add to the box bounds.</param>
        void Merge(const Float3& point)
        {
            Float3::Min(Minimum, point, Minimum);
            Float3::Max(Maximum, point, Maximum);
        }

        /// <summary>
        /// Merges the box with the other box.
        /// </summary>
        /// <param name="box">The other box to add to the box bounds.</param>
        void Merge(const BoundingBox& box)
        {
            Float3::Min(Minimum, box.Minimum, Minimum);
            Float3::Max(Maximum, box.Maximum, Maximum);
        }

        /// <summary>
        /// Creates the bounding box that is offseted by the given vector. Adds the offset value to minimum and maximum points.
        /// </summary>
        /// <param name="offset">The offset.</param>
        /// <returns>The result.</returns>
        BoundingBox MakeOffsetted(const Float3& offset) const;

    public:
        FORCE_INLINE bool operator==(const BoundingBox& other) const
        {
            return Minimum == other.Minimum && Maximum == other.Maximum;
        }

        FORCE_INLINE bool operator!=(const BoundingBox& other) const
        {
            return Minimum != other.Minimum || Maximum != other.Maximum;
        }

        FORCE_INLINE BoundingBox operator*(const Matrix& matrix) const
        {
            BoundingBox result;
            Transform(*this, matrix, result);
            return result;
        }

    public:
        /// <summary>
        /// Constructs a Bounding Box that fully contains the given pair of points.
        /// </summary>
        /// <param name="pointA">The first point that will be contained by the box.</param>
        /// <param name="pointB">The second point that will be contained by the box.</param>
        /// <param name="result">The constructed bounding box.</param>
        static void FromPoints(const Float3& pointA, const Float3& pointB, BoundingBox& result)
        {
            Float3::Min(pointA, pointB, result.Minimum);
            Float3::Max(pointA, pointB, result.Maximum);
        }

        /// <summary>
        /// Constructs a Bounding Box that fully contains the given points.
        /// </summary>
        /// <param name="points">The points that will be contained by the box.</param>
        /// <param name="pointsCount">The amount of points to use.</param>
        /// <param name="result">The constructed bounding box.</param>
        static void FromPoints(const Float3* points, int32 pointsCount, BoundingBox& result);

        /// <summary>
        /// Constructs a Bounding Box that fully contains the given points.
        /// </summary>
        /// <param name="points">The points that will be contained by the box.</param>
        /// <param name="pointsCount">The amount of points to use.</param>
        /// <param name="result">The constructed bounding box.</param>
        static void FromPoints(const Double3* points, int32 pointsCount, BoundingBox& result);

        /// <summary>
        /// Constructs a Bounding Box from a given sphere.
        /// </summary>
        /// <param name="sphere">The sphere that will designate the extents of the box.</param>
        /// <param name="result">The constructed bounding box.</param>
        static void FromSphere(const BoundingSphere& sphere, BoundingBox& result);

        /// <summary>
        /// Constructs a Bounding Box from a given sphere.
        /// </summary>
        /// <param name="sphere">The sphere that will designate the extents of the box.</param>
        /// <returns>The constructed bounding box.</returns>
        static BoundingBox FromSphere(const BoundingSphere& sphere);

        /// <summary>
        /// Constructs a Bounding Box that is as large as the total combined area of the two specified boxes.
        /// </summary>
        /// <param name="value1">The first box to merge.</param>
        /// <param name="value2">The second box to merge.</param>
        /// <param name="result">The constructed bounding box.</param>
        static void Merge(const BoundingBox& value1, const BoundingBox& value2, BoundingBox& result)
        {
            Float3::Min(value1.Minimum, value2.Minimum, result.Minimum);
            Float3::Max(value1.Maximum, value2.Maximum, result.Maximum);
        }

        /// <summary>
        /// Transforms the bounding box using the specified matrix.
        /// </summary>
        /// <param name="box">The box.</param>
        /// <param name="matrix">The matrix.</param>
        /// <returns>The result transformed box.</returns>
        static BoundingBox Transform(const BoundingBox& box, const Matrix& matrix);

        /// <summary>
        /// Creates the bounding box that is offseted by the given vector. Adds the offset value to minimum and maximum points.
        /// </summary>
        /// <param name="box">The box.</param>
        /// <param name="offset">The bounds offset.</param>
        /// <returns>The offsetted bounds.</returns>
        static BoundingBox MakeOffsetted(const BoundingBox& box, const Float3& offset);

        /// <summary>
        /// Creates the bounding box that is scaled by the given factor. Applies scale to the size of the bounds.
        /// </summary>
        /// <param name="box">The box.</param>
        /// <param name="scale">The bounds scale.</param>
        /// <returns>The scaled bounds.</returns>
        static BoundingBox MakeScaled(const BoundingBox& box, float scale);

        /// <summary>
        /// Transforms the bounding box using the specified matrix.
        /// </summary>
        /// <param name="box">The box.</param>
        /// <param name="matrix">The matrix.</param>
        /// <param name="result">The result transformed box.</param>
        static void Transform(const BoundingBox& box, const Matrix& matrix, BoundingBox& result);

        /// <summary>
        /// Transforms the bounding box using the specified transformation.
        /// </summary>
        /// <param name="box">The box.</param>
        /// <param name="transform">The transformation.</param>
        /// <param name="result">The result transformed box.</param>
        static void Transform(const BoundingBox& box, const ::SE::Transform& transform, BoundingBox& result);

    public:
        /// <summary>
        /// Determines if there is an intersection between the current object and a Ray.
        /// </summary>
        /// <param name="ray">The ray to test.</param>
        /// <returns>Whether the two objects intersected.</returns>
        bool Intersects(const Ray& ray) const;

        /// <summary>
        /// Determines if there is an intersection between the current object and a Ray.
        /// </summary>
        /// <param name="ray">The ray to test.</param>
        /// <param name="distance">When the method completes, contains the distance of the intersection, or 0 if there was no intersection.</param>
        /// <returns> Whether the two objects intersected.</returns>
        bool Intersects(const Ray& ray, float& distance) const;

        /// <summary>
        /// Determines if there is an intersection between the current object and a Ray.
        /// </summary>
        /// <param name="ray">The ray to test.</param>
        /// <param name="distance">When the method completes, contains the distance of the intersection, or 0 if there was no intersection.</param>
        /// <param name="normal">When the method completes, contains the intersection surface normal vector, or Float3::Up if there was no intersection.</param>
        /// <returns>Whether the two objects intersected.</returns>
        bool Intersects(const Ray& ray, float& distance, Float3& normal) const;

        /// <summary>
        /// Determines if there is an intersection between the current object and a Ray.
        /// </summary>
        /// <param name="ray">The ray to test.</param>
        /// <param name="point">When the method completes, contains the point of intersection, or <see cref="Float3.Zero"/> if there was no intersection.</param>
        /// <returns>Whether the two objects intersected.</returns>
        bool Intersects(const Ray& ray, Float3& point) const;

        /// <summary>
        /// Determines if there is an intersection between the current object and a Plane.
        /// </summary>
        /// <param name="plane">The plane to test.</param>
        /// <returns>Whether the two objects intersected.</returns>
        PlaneIntersectionType Intersects(const Plane& plane) const;

        /// <summary>
        /// Determines if there is an intersection between the current object and a Bounding Box.
        /// </summary>
        /// <param name="box">The box to test.</param>
        /// <returns>Whether the two objects intersected.</returns>
        bool Intersects(const BoundingBox& box) const;

        /// <summary>
        /// Determines if there is an intersection between the current object and a Bounding Sphere.
        /// </summary>
        /// <param name="sphere">The sphere to test.</param>
        /// <returns>Whether the two objects intersected.</returns>
        bool Intersects(const BoundingSphere& sphere) const;

        /// <summary>
        /// Determines whether the current objects contains a point.
        /// </summary>
        /// <param name="point">The point to test.</param>
        /// <returns>The type of containment the two objects have.</returns>
        ContainmentType Contains(const Float3& point) const;

        /// <summary>
        /// Determines whether the current objects contains a Bounding Box.
        /// </summary>
        /// <param name="box">The box to test.</param>
        /// <returns>The type of containment the two objects have.</returns>
        ContainmentType Contains(const BoundingBox& box) const;

        /// <summary>
        /// Determines whether the current objects contains a Bounding Sphere.
        /// </summary>
        /// <param name="sphere">The sphere to test.</param>
        /// <returns>The type of containment the two objects have.</returns>
        ContainmentType Contains(const BoundingSphere& sphere) const;

        /// <summary>
        /// Determines the distance between a Bounding Box and a point.
        /// </summary>
        /// <param name="point">The point to test.</param>
        /// <returns>The distance between bounding box and a point.</returns>
        float Distance(const Float3& point) const;

        /// <summary>
        /// Determines the distance between two Bounding Boxed.
        /// </summary>
        /// <param name="box">The bounding box to test.</param>
        /// <returns>The distance between bounding boxes.</returns>
        float Distance(const BoundingBox& box) const;
    };

    /*//-------------------------------------------------------------------------
    // Oriented Bounding Box
    //-------------------------------------------------------------------------

    struct SE_API_CORE OBB
    {
        OBB() = default;
        OBB(VectorSIMD center, VectorSIMD extents, Quaternion orientation = Quaternion::Identity);
        OBB(VectorSIMD const *pPoints, uint32_t numPoints);
        explicit OBB(AABB const &aabb);
        explicit OBB(AABB const &aabb, Transform const &transform);

        //-------------------------------------------------------------------------

        inline bool IsValid() const { return m_Extents.IsGreaterThanEqual3(VectorSIMD::Zero); }

        inline void Reset()
        {
            m_Orientation = Quaternion::Identity;
            m_Center = m_Extents = VectorSIMD::Zero;
        }

        inline void GetCorners(VectorSIMD corners[8]) const
        {
            for (int32_t i = 0; i < 8; ++i)
            {
                corners[i] = m_Center + m_Orientation.RotateVector(m_Extents * VectorSIMD::BoxCorners[i]);
            }
        }

        inline AABB GetAABB() const;

        // Positioning
        //-------------------------------------------------------------------------

        inline void SetCenter(VectorSIMD const &newCenter) { m_Center = newCenter; }

        inline void SetOrientation(Quaternion const &newOrientation) { m_Orientation = newOrientation; }

        inline void Translate(VectorSIMD const &deltaVectorSIMD) { m_Center += deltaVectorSIMD; }

        inline void Rotate(Quaternion const &deltaRotation) { m_Orientation = deltaRotation * m_Orientation; }

        void ApplyTransform(Transform const &transform);

        void ApplyScale(VectorSIMD const &scale);

        inline OBB GetTransformed(Transform const &transform) const
        {
            OBB result = *this;
            result.ApplyTransform(transform);
            return result;
        }

        // Queries
        //-------------------------------------------------------------------------

        inline bool ContainsPoint(VectorSIMD const &point) const;

        //-------------------------------------------------------------------------

        inline OverlapResult OverlapTest(OBB const &other) const;
        bool Overlaps(OBB const &box) const; // Slightly faster than the full overlap test

        //-------------------------------------------------------------------------

        inline OverlapResult OverlapTest(AABB const &aabb) const { return OverlapTest(OBB(aabb)); }
        inline bool Overlaps(AABB const &aabb) const { return OverlapTest(OBB(aabb)) != OverlapResult::NoOverlap; }

    public:
        Quaternion m_Orientation = Quaternion::Identity;
        VectorSIMD m_Center = VectorSIMD::UnitW;
        VectorSIMD m_Extents = VectorSIMD::Zero;
    };*/

    /// <summary>
    /// Defines a frustum which can be used in frustum culling, zoom to Extents (zoom to fit) operations, (matrix, frustum, camera) interchange, and many kind of intersection testing.
    /// </summary>
    struct SE_API_CORE BoundingFrustum
    {
        friend CollisionsHelper;
    private:
        Matrix _matrix;

        union
        {
            struct
            {
                Plane _pNear;
                Plane _pFar;
                Plane _pLeft;
                Plane _pRight;
                Plane _pTop;
                Plane _pBottom;
            };
            Plane _planes[6];
        };

    public:
        /// <summary>
        /// Empty constructor.
        /// </summary>
        BoundingFrustum() = default;

        /// <summary>
        /// Initializes a new instance of the <see cref="BoundingFrustum"/> struct.
        /// </summary>
        /// <param name="matrix">The combined matrix that usually takes View * Projection matrix.</param>
        BoundingFrustum(const Matrix& matrix)
        {
            SetMatrix(matrix);
        }

    public:
        String ToString() const;

    public:
        /// <summary>
        /// Gets the matrix that describes this bounding frustum.
        /// </summary>
        FORCE_INLINE const Matrix& GetMatrix() const
        {
            return _matrix;
        }

        /// <summary>
        /// Gets the matrix that describes this bounding frustum.
        /// </summary>
        FORCE_INLINE Matrix GetMatrix()
        {
            return _matrix;
        }

        /// <summary>
        /// Gets the inverted matrix to that describes this bounding frustum.
        /// </summary>
        /// <param name="result">The result matrix.</param>
        FORCE_INLINE void GetInvMatrix(Matrix& result) const
        {
            Matrix::Invert(_matrix, result);
        }

        /// <summary>
        /// Sets the matrix (made from view and projection matrices) that describes this bounding frustum.
        /// </summary>
        /// <param name="view">The view matrix.</param>
        /// <param name="projection">The projection matrix.</param>
        void SetMatrix(const Matrix& view, const Matrix& projection)
        {
            Matrix viewProjection;
            Matrix::Multiply(view, projection, viewProjection);
            SetMatrix(viewProjection);
        }

        /// <summary>
        /// Sets the matrix that describes this bounding frustum.
        /// </summary>
        /// <param name="matrix">The matrix (View*Projection).</param>
        void SetMatrix(const Matrix& matrix);

        /// <summary>
        /// Gets the near.
        /// </summary>
        FORCE_INLINE Plane GetNear() const
        {
            return _pNear;
        }

        /// <summary>
        /// Gets the far plane of the BoundingFrustum.
        /// </summary>
        FORCE_INLINE Plane GetFar() const
        {
            return _pFar;
        }

        /// <summary>
        /// Gets the left plane of the BoundingFrustum.
        /// </summary>
        FORCE_INLINE Plane GetLeft() const
        {
            return _pLeft;
        }

        /// <summary>
        /// Gets the right plane of the BoundingFrustum.
        /// </summary>
        FORCE_INLINE Plane GetRight() const
        {
            return _pRight;
        }

        /// <summary>
        /// Gets the top plane of the BoundingFrustum.
        /// </summary>
        FORCE_INLINE Plane GetTop() const
        {
            return _pTop;
        }

        /// <summary>
        /// Gets the bottom plane of the BoundingFrustum.
        /// </summary>
        FORCE_INLINE Plane GetBottom() const
        {
            return _pBottom;
        }

        /// <summary>
        /// Gets the one of the 6 planes related to this frustum.
        /// </summary>
        /// <param name="index">The index where 0 for Left, 1 for Right, 2 for Top, 3 for Bottom, 4 for Near, 5 for Far.</param>
        /// <returns>The plane.</returns>
        Plane GetPlane(int32 index) const;

        /// <summary>
        /// Gets the the 8 corners of the frustum: Near1 (near right down corner), Near2 (near right top corner), Near3 (near Left top corner), Near4 (near Left down corner), Far1 (far right down corner), Far2 (far right top corner), Far3 (far left top corner), Far4 (far left down corner).
        /// </summary>
        /// <param name="corners">The corners.</param>
        void GetCorners(Float3 corners[8]) const;

        /// <summary>
        /// Gets the the 8 corners of the frustum: Near1 (near right down corner), Near2 (near right top corner), Near3 (near Left top corner), Near4 (near Left down corner), Far1 (far right down corner), Far2 (far right top corner), Far3 (far left top corner), Far4 (far left down corner).
        /// </summary>
        /// <param name="corners">The corners.</param>
        void GetCorners(Double3 corners[8]) const;

        /// <summary>
        /// Gets bounding box that contains whole frustum.
        /// </summary>
        /// <param name="result">The result box.</param>
        void GetBox(BoundingBox& result) const;

        /// <summary>
        /// Gets bounding sphere that contains whole frustum.
        /// </summary>
        /// <param name="result">The result sphere.</param>
        void GetSphere(BoundingSphere& result) const;

        /// <summary>
        /// Determines whether this frustum is orthographic.
        /// </summary>
        FORCE_INLINE bool IsOrthographic() const
        {
            return _pLeft.Normal == -_pRight.Normal && _pTop.Normal == -_pBottom.Normal;
        }

        /// <summary>
        /// Gets the width of the frustum at specified depth.
        /// </summary>
        /// <param name="depth">The depth at which to calculate frustum width.</param>
        /// <returns>The width of the frustum at the specified depth.</returns>
        float GetWidthAtDepth(float depth) const;

        /// <summary>
        /// Gets the height of the frustum at specified depth.
        /// </summary>
        /// <param name="depth">The depth at which to calculate frustum height.</param>
        /// <returns>The height of the frustum at the specified depth.</returns>
        float GetHeightAtDepth(float depth) const;

    public:
        FORCE_INLINE bool operator==(const BoundingFrustum& other) const
        {
            return _matrix == other._matrix;
        }

        FORCE_INLINE bool operator!=(const BoundingFrustum& other) const
        {
            return _matrix != other._matrix;
        }

    public:
        /// <summary>
        /// Checks whether a point lays inside, intersects or lays outside the frustum.
        /// </summary>
        /// <param name="point">The point.</param>
        /// <returns>The type of the containment.</returns>
        ContainmentType Contains(const Float3& point) const;

        /// <summary>
        /// Determines the intersection relationship between the frustum and a bounding box.
        /// </summary>
        /// <param name="box">The box.</param>
        /// <returns>The type of the containment.</returns>
        FORCE_INLINE ContainmentType Contains(const BoundingBox& box) const
        {
            return CollisionsHelper::FrustumContainsBox(*this, box);
        }

        /// <summary>
        /// Determines the intersection relationship between the frustum and a bounding sphere.
        /// </summary>
        /// <param name="sphere">The sphere.</param>
        /// <returns>The type of the containment.</returns>
        ContainmentType Contains(const BoundingSphere& sphere) const;

        /// <summary>
        /// Checks whether the current BoundingFrustum intersects a Sphere.
        /// </summary>
        /// <param name="sphere">The sphere.</param>
        /// <returns>True if the current BoundingFrustum intersects a Sphere, otherwise false.</returns>
        bool Intersects(const BoundingSphere& sphere) const;

        /// <summary>
        /// Checks whether the current BoundingFrustum intersects a AABB.
        /// </summary>
        /// <param name="box">The box</param>
        /// <returns>True if the current BoundingFrustum intersects a AABB, otherwise false.</returns>
        FORCE_INLINE bool Intersects(const BoundingBox& box) const
        {
            return CollisionsHelper::FrustumContainsBox(*this, box) != ContainmentType::Disjoint;
        }
    };


    struct SE_API_CORE BoundingSphere
    {
    public:
        /// <summary>
        /// An empty bounding sphere (Center = 0 and Radius = 0).
        /// </summary>
        static const BoundingSphere Empty;

    public:
        /// <summary>
        /// The center of the sphere in three dimensional space.
        /// </summary>
        Float3 Center;

        /// <summary>
        /// The radius of the sphere.
        /// </summary>
        float Radius;

    public:
        /// <summary>
        /// Empty constructor.
        /// </summary>
        BoundingSphere() = default;

        /// <summary>
        /// Initializes a new instance of the <see cref="Sphere"/> struct.
        /// </summary>
        /// <param name="center">The center of the sphere in three dimensional space.</param>
        /// <param name="radius">The radius of the sphere.</param>
        BoundingSphere(const Float3& center, float radius)
            : Center(center)
            , Radius(radius)
        {
        }

    public:
        String ToString() const;

    public:
        FORCE_INLINE bool operator==(const BoundingSphere& other) const
        {
            return Center == other.Center && Radius == other.Radius;
        }

        FORCE_INLINE bool operator!=(const BoundingSphere& other) const
        {
            return Center != other.Center || Radius != other.Radius;
        }

    public:
        static bool NearEqual(const BoundingSphere& a, const BoundingSphere& b)
        {
            return Float3::NearEqual(a.Center, b.Center) && Math::IsNearEqual(a.Radius, b.Radius);
        }

        static bool NearEqual(const BoundingSphere& a, const BoundingSphere& b, float epsilon)
        {
            return Float3::NearEqual(a.Center, b.Center, epsilon) && Math::IsNearEqual(a.Radius, b.Radius, epsilon);
        }

    public:
        /// <summary>
        /// Determines if there is an intersection between the current object and a Ray.
        /// </summary>
        /// <param name="ray">The ray to test.</param>
        /// <returns>Whether the two objects intersected.</returns>
        bool Intersects(const Ray& ray) const;

        /// <summary>
        /// Determines if there is an intersection between the current object and a Ray.
        /// </summary>
        /// <param name="ray">The ray to test.</param>
        /// <param name="distance">When the method completes, contains the distance of the intersection, or 0 if there was no intersection.</param>
        /// <returns>Whether the two objects intersected.</returns>
        bool Intersects(const Ray& ray, float& distance) const;

        /// <summary>
        /// Determines if there is an intersection between the current object and a Ray.
        /// </summary>
        /// <param name="ray">The ray to test.</param>
        /// <param name="distance">When the method completes, contains the distance of the intersection, or 0 if there was no intersection.</param>
        /// <param name="normal">When the method completes, contains the intersection surface normal vector, or Float3::Up if there was no intersection.</param>
        /// <returns>Whether the two objects intersected.</returns>
        bool Intersects(const Ray& ray, float& distance, Float3& normal) const;

        /// <summary>
        /// Determines if there is an intersection between the current object and a Ray.
        /// </summary>
        /// <param name="ray">The ray to test.</param>
        /// <param name="point">When the method completes, contains the point of intersection, or Float3::Zero if there was no intersection.</param>
        /// <returns>Whether the two objects intersected.</returns>
        bool Intersects(const Ray& ray, Float3& point) const;

        /// <summary>
        /// Determines if there is an intersection between the current object and a Plane.
        /// </summary>
        /// <param name="plane">The plane to test.</param>
        /// <returns>Whether the two objects intersected.</returns>
        PlaneIntersectionType Intersects(const Plane& plane) const;

        /// <summary>
        /// Determines if there is an intersection between the current object and a triangle.
        /// </summary>
        /// <param name="vertex1">The first vertex of the triangle to test.</param>
        /// <param name="vertex2">The second vertex of the triangle to test.</param>
        /// <param name="vertex3">The third vertex of the triangle to test.</param>
        /// <returns>Whether the two objects intersected.</returns>
        bool Intersects(const Float3& vertex1, const Float3& vertex2, const Float3& vertex3) const;

        /// <summary>
        /// Determines if there is an intersection between the current object and a Bounding Box.
        /// </summary>
        /// <param name="box">The box to test.</param>
        /// <returns>Whether the two objects intersected.</returns>
        bool Intersects(const BoundingBox& box) const;

        /// <summary>
        /// Determines if there is an intersection between the current object and a Bounding Sphere.
        /// </summary>
        /// <param name="sphere">The sphere to test.</param>
        /// <returns>Whether the two objects intersected.</returns>
        bool Intersects(const BoundingSphere& sphere) const;

        /// <summary>
        /// Determines whether the current objects contains a point.
        /// </summary>
        /// <param name="point">The point to test.</param>
        /// <returns> The type of containment the two objects have.</returns>
        ContainmentType Contains(const Float3& point) const;

        /// <summary>
        /// Determines whether the current objects contains a triangle.
        /// </summary>
        /// <param name="vertex1">The first vertex of the triangle to test.</param>
        /// <param name="vertex2">The second vertex of the triangle to test.</param>
        /// <param name="vertex3">The third vertex of the triangle to test.</param>
        /// <returns>The type of containment the two objects have.</returns>
        ContainmentType Contains(const Float3& vertex1, const Float3& vertex2, const Float3& vertex3) const;

        /// <summary>
        /// Determines whether the current objects contains a Bounding Box
        /// </summary>
        /// <param name="box">The box to test.</param>
        /// <returns>The type of containment the two objects have.</returns>
        ContainmentType Contains(const BoundingBox& box) const;

        /// <summary>
        /// Determines whether the current objects contains a Bounding Sphere.
        /// </summary>
        /// <param name="sphere">The sphere to test.</param>
        /// <returns>The type of containment the two objects have.</returns>
        ContainmentType Contains(const BoundingSphere& sphere) const;

    public:
        /// <summary>
        /// Gets the box which contains whole sphere.
        /// </summary>
        /// <returns>The box.</returns>
        BoundingBox GetBoundingBox() const;

        /// <summary>
        /// Gets the box which contains whole sphere.
        /// </summary>
        /// <param name="result">The result box.</param>
        void GetBoundingBox(BoundingBox& result) const;

        /// <summary>
        /// Constructs a Sphere that fully contains the given points
        /// </summary>
        /// <param name="points">The points that will be contained by the sphere.</param>
        /// <param name="pointsCount">The amount of points to use.</param>
        /// <param name="result">When the method completes, contains the newly constructed bounding sphere.</param>
        static void FromPoints(const Float3* points, int32 pointsCount, BoundingSphere& result);

        /// <summary>
        /// Constructs a Sphere that fully contains the given points
        /// </summary>
        /// <param name="points">The points that will be contained by the sphere.</param>
        /// <param name="pointsCount">The amount of points to use.</param>
        /// <param name="result">When the method completes, contains the newly constructed bounding sphere.</param>
        static void FromPoints(const Double3* points, int32 pointsCount, BoundingSphere& result);

        /// <summary>
        /// Constructs a Bounding Sphere from a given box.
        /// </summary>
        /// <param name="box">The box that will designate the extents of the sphere.</param>
        /// <param name="result">When the method completes, the newly constructed bounding sphere.</param>
        static void FromBox(const BoundingBox& box, BoundingSphere& result);

        /// <summary>
        /// Constructs a Sphere that is the as large as the total combined area of the two specified spheres
        /// </summary>
        /// <param name="value1">The first sphere to merge.</param>
        /// <param name="value2">The second sphere to merge.</param>
        /// <param name="result">When the method completes, contains the newly constructed bounding sphere.</param>
        static void Merge(const BoundingSphere& value1, const BoundingSphere& value2, BoundingSphere& result);

        /// <summary>
        /// Constructs a Sphere that is the as large as the total combined area of the specified sphere and the point.
        /// </summary>
        /// <param name="value1">The sphere to merge.</param>
        /// <param name="value2">The point to merge.</param>
        /// <param name="result">When the method completes, contains the newly constructed bounding sphere.</param>
        static void Merge(const BoundingSphere& value1, const Float3& value2, BoundingSphere& result);

        /// <summary>
        /// Transforms the bounding sphere using the specified matrix.
        /// </summary>
        /// <param name="sphere">The sphere.</param>
        /// <param name="matrix">The matrix.</param>
        /// <param name="result">The result transformed sphere.</param>
        static void Transform(const BoundingSphere& sphere, const Matrix& matrix, BoundingSphere& result);
    };


}

template<>
struct TIsPODType<SE::BoundingBox>
{
    enum { Value = true };
};

DEFINE_DEFAULT_FORMATTING(SE::BoundingBox, "Minimum:{0} Maximum:{1}", v.Minimum, v.Maximum);

template<>
struct TIsPODType<SE::BoundingFrustum>
{
    enum { Value = true };
};

DEFINE_DEFAULT_FORMATTING(SE::BoundingFrustum, "{}", v.GetMatrix());

template<>
struct TIsPODType<SE::BoundingSphere>
{
    enum { Value = true };
};

DEFINE_DEFAULT_FORMATTING(SE::BoundingSphere, "Center:{0} Radius:{1}", v.Center, v.Radius);