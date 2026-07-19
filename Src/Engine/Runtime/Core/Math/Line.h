#pragma once

#include "Matrix.h"

//-------------------------------------------------------------------------
// Line Helpers
//-------------------------------------------------------------------------

namespace SE
{
    struct BoundingSphere;
    struct BoundingBox;
    struct Plane;
    struct Viewport;

    /*// Line - an infinite line defined by a start point and direction
    //-------------------------------------------------------------------------

    class Line
    {
    public:
        enum CtorStartEnd
        {
            StartEnd,
        };

        enum CtorStartDirection
        {
            StartDirection,
        };

    public:
        Line(CtorStartDirection, Float3 const &startPoint, Float3 const &direction)
            : m_startPoint(startPoint), m_direction(direction)
        {
            ENGINE_ASSERT(m_direction.IsNormalized3());
        }

        Line(CtorStartEnd, Float3 const &startPoint, Float3 const &endPoint)
            : m_startPoint(startPoint), m_direction((endPoint - startPoint).Normalize())
        {
            ENGINE_ASSERT(m_direction.IsNormalized3());
        }

        inline Float3 GetStartPoint() const { return m_startPoint; }

        inline Float3 GetDirection() const { return m_direction; }

        //-------------------------------------------------------------------------

        inline float ScalarProjectionOnLine(Float3 const &point) const
        {
            auto const dot = Float3::Dot3((point - m_startPoint), m_direction);
            return dot.ToFloat();
        }

        inline Float3 GetPointAlongLine(float distanceFromStartPoint) const
        {
            return Float3::MultiplyAdd(m_direction, Float3(distanceFromStartPoint), m_startPoint);
        }

        inline Float3 VectorProjectionOnLine(Float3 const &point, float &outScalarResolute) const
        {
            outScalarResolute = ScalarProjectionOnLine(point);
            return GetPointAlongLine(outScalarResolute);
        }

        inline Float3 GetClosestPointOnLine(Float3 const &point) const
        {
            float scalarResolute;
            return VectorProjectionOnLine(point, scalarResolute);
        }

        inline Float3 GetDistanceOnLineFromStartPoint(Float3 const &point) const
        {
            return Float3(ScalarProjectionOnLine(point));
        }

        inline float GetDistanceBetweenLineAndPoint(Float3 const &point, Float3 *pOutClosestPoint = nullptr) const
        {
            Float3 const closestPointOnLine = GetClosestPointOnLine(point);

            if (pOutClosestPoint != nullptr)
            {
                *pOutClosestPoint = closestPointOnLine;
            }

            return Float3::Distance(closestPointOnLine, point);
        }

        // Return the intersection point between two lines in 2D
        Float3 IntersectLine2D(Line const &other) const
        {
            Float3 V = m_startPoint - other.m_startPoint;
            Float3 C1 = Float3::Cross(m_direction, other.m_direction);
            Float3 C2 = Float3::Cross(other.m_direction, V);

            Float3 result;
            if (C1.IsNearZero2())
            {
                // Coincident
                if (C2.IsNearZero2())
                {
                    result = Float3::Infinity;
                }
                else // Parallel
                {
                    result = Float3::QNaN;
                }
            }
            else // Intersection point = Line1Point1 + V1 * (C2 / C1)
            {
                Float3 distance = C1.GetInverse();
                distance = C2 * distance;
                result = Float3::MultiplyAdd(m_direction, distance, m_startPoint);
            }

            return result;
        }

    protected:
        Line() = default;

    protected:
        Float3 m_startPoint;
        Float3 m_direction;
    };

    // Line Segment - a line with a fixed start and end points
    //-------------------------------------------------------------------------

    class LineSegment : public Line
    {
        friend class Ray;

    public:
        LineSegment(Float3 startPoint, Float3 endPoint)
            : Line(Line::StartEnd, startPoint, endPoint), m_endPoint(endPoint), m_length(startPoint.GetDistance3(endPoint))
        {
        }

        inline Float3 GetEndPoint() const { return m_endPoint; }

        inline float GetLength() const { return m_length.ToFloat(); }

        //-------------------------------------------------------------------------

        // The scalar project aka distance along the segment (clamped between start and end point)
        inline float ScalarProjectionOnSegment(Float3 const &point) const
        {
            auto const dot = Float3::Dot3((point - m_startPoint), m_direction);
            float distance = Math::Clamp(dot.ToFloat(), 0.0f, GetLength());
            return distance;
        }

        // Returns a point on the segment at the desired percentage between start and end points
        inline Float3 GetPointOnSegment(float percentageAlongSegment) const
        {
            ENGINE_ASSERT(percentageAlongSegment >= 0 && percentageAlongSegment <= 1.0f);
            Float3 const distance = m_length * percentageAlongSegment;
            return Float3::MultiplyAdd(m_direction, distance, m_startPoint);
        }

        inline Float3 VectorProjectionOnSegment(Float3 const &point, float &outScalarResolute) const
        {
            outScalarResolute = ScalarProjectionOnSegment(point);
            return GetPointAlongLine(outScalarResolute);
        }

        inline Float3 GetClosestPointOnSegment(Float3 const &point) const
        {
            float scalarResolute;
            return VectorProjectionOnSegment(point, scalarResolute);
        }

        // Get the distance along the segment starting at the start point. Always returns a positive value >= 0
        inline Float3 GetDistanceAlongSegment(Float3 const &point) const
        {
            return Float3(ScalarProjectionOnSegment(point));
        }

        // Returns the shortest distance between the segment and the specified point
        inline float GetDistanceFromSegmentToPoint(Float3 const &point, Float3 *pOutClosestPoint = nullptr) const
        {
            Float3 const closestPointOnSegment = GetClosestPointOnSegment(point);

            if (pOutClosestPoint != nullptr)
            {
                *pOutClosestPoint = closestPointOnSegment;
            }

            return closestPointOnSegment.GetDistance3(point);
        }

        //-------------------------------------------------------------------------

        inline LineSegment &Transform(Matrix const &transform)
        {
            m_startPoint = transform.TransformPoint(m_startPoint);
            m_endPoint = transform.TransformPoint(m_endPoint);
            m_direction = (m_endPoint - m_startPoint).GetNormalized3();
            m_length = Float3(m_startPoint.GetDistance3(m_endPoint));
            return *this;
        }

        inline LineSegment GetTransformed(Matrix const &transform) const
        {
            LineSegment line = *this;
            line.Transform(transform);
            return line;
        }

    private:
        LineSegment() = default;

    private:
        Float3 m_endPoint;
        Float3 m_length;
    };*/

    /// <summary>
    /// Represents a three dimensional line based on a point in space and a direction.
    /// </summary>
    struct SE_API_RUNTIME Ray
    {
    public:
        /// <summary>
        /// The position in three dimensional space where the ray starts.
        /// </summary>
        Float3 Position;

        /// <summary>
        /// The normalized direction in which the ray points.
        /// </summary>
        Float3 Direction;

    public:
        /// <summary>
        /// Identity ray (at zero origin pointing forwards).
        /// </summary>
        static Ray Identity;

    public:
        /// <summary>
        /// Empty constructor.
        /// </summary>
        Ray() = default;

        /// <summary>
        /// Initializes a new instance of the <see cref="Ray"/> struct.
        /// </summary>
        /// <param name="position">The ray origin position in 3D space.</param>
        /// <param name="direction">The normalized ray direction in 3D space.</param>
        Ray(const Float3& position, const Float3& direction)
            : Position(position)
            , Direction(direction)
        {
        }

    public:
        String ToString() const;

    public:
        FORCE_INLINE bool operator==(const Ray& other) const
        {
            return Position == other.Position && Direction == other.Direction;
        }

        FORCE_INLINE bool operator!=(const Ray& other) const
        {
            return Position != other.Position || Direction != other.Direction;
        }

        static bool NearEqual(const Ray& a, const Ray& b)
        {
            return Float3::NearEqual(a.Position, b.Position) && Float3::NearEqual(a.Direction, b.Direction);
        }

        static bool NearEqual(const Ray& a, const Ray& b, float epsilon)
        {
            return Float3::NearEqual(a.Position, b.Position, epsilon) && Float3::NearEqual(a.Direction, b.Direction, epsilon);
        }

    public:
        /// <summary>
        /// Gets a point at distance long ray.
        /// </summary>
        /// <param name="distance">The distance from ray origin.</param>
        /// <returns>The calculated point.</returns>
        Float3 GetPoint(float distance) const;

        /// <summary>
        /// Determines if there is an intersection between the current object and a point.
        /// </summary>
        /// <param name="point">The point to test.</param>
        /// <returns>Whether the two objects intersected.</returns>
        bool Intersects(const Float3& point) const;

        /// <summary>
        /// Determines if there is an intersection between the current object and a <see cref="Ray" />.
        /// </summary>
        /// <param name="ray">The ray to test.</param>
        /// <returns>Whether the two objects intersected.</returns>
        bool Intersects(const Ray& ray) const;

        /// <summary>
        /// Determines if there is an intersection between the current object and a <see cref="Ray" />.
        /// </summary>
        /// <param name="ray">The ray to test.</param>
        /// <param name="point">When the method completes, contains the point of intersection, or <see cref="Float3.Zero" /> if there was no intersection.
        /// </param>
        /// <returns>Whether the two objects intersected.</returns>
        bool Intersects(const Ray& ray, Float3& point) const;

        /// <summary>
        /// Determines if there is an intersection between the current object and a <see cref="Plane" />.
        /// </summary>
        /// <param name="plane">The plane to test</param>
        /// <returns>Whether the two objects intersected.</returns>
        bool Intersects(const Plane& plane) const;

        /// <summary>
        /// Determines if there is an intersection between the current object and a <see cref="Plane" />.
        /// </summary>
        /// <param name="plane">The plane to test.</param>
        /// <param name="distance">When the method completes, contains the distance of the intersection, or 0 if there was no intersection.</param>
        /// <returns>Whether the two objects intersected.</returns>
        bool Intersects(const Plane& plane, float& distance) const;

        /// <summary>
        /// Determines if there is an intersection between the current object and a <see cref="Plane" />.
        /// </summary>
        /// <param name="plane">The plane to test.</param>
        /// <param name="point">When the method completes, contains the point of intersection, or <see cref="Float3.Zero" /> if there was no intersection.</param>
        /// <returns>Whether the two objects intersected.</returns>
        bool Intersects(const Plane& plane, Float3& point) const;

        /// <summary>
        /// Determines if there is an intersection between the current object and a triangle.
        /// </summary>
        /// <param name="vertex1">The first vertex of the triangle to test.</param>
        /// <param name="vertex2">The second vertex of the triangle to test.</param>
        /// <param name="vertex3">The third vertex of the triangle to test.</param>
        /// <returns>Whether the two objects intersected.</returns>
        bool Intersects(const Float3& vertex1, const Float3& vertex2, const Float3& vertex3) const;

        /// <summary>
        /// Determines if there is an intersection between the current object and a triangle.
        /// </summary>
        /// <param name="vertex1">The first vertex of the triangle to test.</param>
        /// <param name="vertex2">The second vertex of the triangle to test.</param>
        /// <param name="vertex3">The third vertex of the triangle to test.</param>
        /// <param name="distance">When the method completes, contains the distance of the intersection, or 0 if there was no intersection.</param>
        /// <returns>Whether the two objects intersected.</returns>
        bool Intersects(const Float3& vertex1, const Float3& vertex2, const Float3& vertex3, float& distance) const;

        /// <summary>
        /// Determines if there is an intersection between the current object and a triangle.
        /// </summary>
        /// <param name="vertex1">The first vertex of the triangle to test.</param>
        /// <param name="vertex2">The second vertex of the triangle to test.</param>
        /// <param name="vertex3">The third vertex of the triangle to test.</param>
        /// <param name="point">When the method completes, contains the point of intersection, or <see cref="Float3.Zero" /> if there was no intersection.</param>
        /// <returns>Whether the two objects intersected.</returns>
        bool Intersects(const Float3& vertex1, const Float3& vertex2, const Float3& vertex3, Float3& point) const;

        /// <summary>
        /// Determines if there is an intersection between the current object and a <see cref="AABB" />.
        /// </summary>
        /// <param name="box">The box to test.</param>
        /// <returns>Whether the two objects intersected.</returns>
        bool Intersects(const BoundingBox& box) const;

        /// <summary>
        /// Determines if there is an intersection between the current object and a <see cref="AABB" />.
        /// </summary>
        /// <param name="box">The box to test.</param>
        /// <param name="distance">When the method completes, contains the distance of the intersection, or 0 if there was no intersection.</param>
        /// <returns>Whether the two objects intersected.</returns>
        bool Intersects(const BoundingBox& box, float& distance) const;

        /// <summary>
        /// Determines if there is an intersection between the current object and a <see cref="AABB" />.
        /// </summary>
        /// <param name="box">The box to test.</param>
        /// <param name="point">When the method completes, contains the point of intersection, or <see cref="Float3.Zero" /> if there was no intersection.</param>
        /// <returns>Whether the two objects intersected.</returns>
        bool Intersects(const BoundingBox& box, Float3& point) const;

        /// <summary>
        /// Determines if there is an intersection between the current object and a <see cref="Sphere" />.
        /// </summary>
        /// <param name="sphere">The sphere to test.</param>
        /// <returns>Whether the two objects intersected.</returns>
        bool Intersects(const BoundingSphere& sphere) const;

        /// <summary>
        /// Determines if there is an intersection between the current object and a <see cref="Sphere" />.
        /// </summary>
        /// <param name="sphere">The sphere to test.</param>
        /// <param name="distance">When the method completes, contains the distance of the intersection, or 0 if there was no intersection.</param>
        /// <returns>Whether the two objects intersected.</returns>
        bool Intersects(const BoundingSphere& sphere, float& distance) const;

        /// <summary>
        /// Determines if there is an intersection between the current object and a <see cref="Sphere" />.
        /// </summary>
        /// <param name="sphere">The sphere to test.</param>
        /// <param name="point">When the method completes, contains the point of intersection, or <see cref="Float3.Zero" /> if there was no intersection.</param>
        /// <returns>Whether the two objects intersected.</returns>
        bool Intersects(const BoundingSphere& sphere, Float3& point) const;
    public:
        /// <summary>
        /// Calculates a world space ray from 2d screen coordinates.
        /// </summary>
        /// <param name="x">The X coordinate on 2d screen.</param>
        /// <param name="y">The Y coordinate on 2d screen.</param>
        /// <param name="viewport">The screen viewport.</param>
        /// <param name="vp">The View*Projection matrix.</param>
        /// <returns>The resulting ray.</returns>
        static Ray GetPickRay(float x, float y, const Viewport& viewport, const Matrix& vp);
    };

}

template<>
struct TIsPODType<SE::Ray>
{
    enum { Value = true };
};

DEFINE_DEFAULT_FORMATTING(SE::Ray, "Position:{0} Direction:{1}", v.Position, v.Direction);