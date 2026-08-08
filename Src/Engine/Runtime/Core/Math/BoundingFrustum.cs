// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#if USE_LARGE_WORLDS
using Real = System.Double;
#else
using Real = System.Single;
#endif

// -----------------------------------------------------------------------------
// Original code from SharpDX project. https://github.com/sharpdx/SharpDX/
// Greetings to Alexandre Mutel. Original code published with the following license:
// -----------------------------------------------------------------------------
// Copyright (c) 2010-2014 SharpDX - Alexandre Mutel
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace SE
{
    /// <summary>
    /// Defines a frustum which can be used in frustum culling, zoom to Extents (zoom to fit) operations, (matrix, frustum, camera) interchange, and many kind of intersection testing.
    /// </summary>
    [Serializable]
    [StructLayout(LayoutKind.Sequential, Pack = 4)]
    public struct BoundingFrustum : IEquatable<BoundingFrustum>
    {
        private Matrix m_PMatrix;
        private Plane m_PNear;
        private Plane m_PFar;
        private Plane m_PLeft;
        private Plane m_PRight;
        private Plane m_PTop;
        private Plane m_PBottom;

        /// <summary>
        /// Gets or sets the Matrix that describes this bounding frustum.
        /// </summary>
        public Matrix Matrix
        {
            get => m_PMatrix;
            set
            {
                m_PMatrix = value;
                GetPlanesFromMatrix(ref m_PMatrix, out m_PNear, out m_PFar, out m_PLeft, out m_PRight, out m_PTop, out m_PBottom);
            }
        }

        /// <summary>
        /// Gets the near plane of the BoundingFrustum.
        /// </summary>
        public Plane Near => m_PNear;

        /// <summary>
        /// Gets the far plane of the BoundingFrustum.
        /// </summary>
        public Plane Far => m_PFar;

        /// <summary>
        /// Gets the left plane of the BoundingFrustum.
        /// </summary>
        public Plane Left => m_PLeft;

        /// <summary>
        /// Gets the right plane of the BoundingFrustum.
        /// </summary>
        public Plane Right => m_PRight;

        /// <summary>
        /// Gets the top plane of the BoundingFrustum.
        /// </summary>
        public Plane Top => m_PTop;

        /// <summary>
        /// Gets the bottom plane of the BoundingFrustum.
        /// </summary>
        public Plane Bottom => m_PBottom;

        /// <summary>
        /// Creates a new instance of BoundingFrustum.
        /// </summary>
        /// <param name="matrix">Combined matrix that usually takes view × projection matrix.</param>
        public BoundingFrustum(Matrix matrix)
        {
            m_PMatrix = matrix;
            GetPlanesFromMatrix(ref m_PMatrix, out m_PNear, out m_PFar, out m_PLeft, out m_PRight, out m_PTop, out m_PBottom);
        }

        /// <summary>
        /// Creates a new instance of BoundingFrustum.
        /// </summary>
        /// <param name="matrix">Combined matrix that usually takes view × projection matrix.</param>
        public BoundingFrustum(ref Matrix matrix)
        {
            m_PMatrix = matrix;
            GetPlanesFromMatrix(ref m_PMatrix, out m_PNear, out m_PFar, out m_PLeft, out m_PRight, out m_PTop, out m_PBottom);
        }

        /// <summary>
        /// Returns a hash code for this instance.
        /// </summary>
        /// <returns>A hash code for this instance, suitable for use in hashing algorithms and data structures like a hash table.</returns>
        public override int GetHashCode()
        {
            return m_PMatrix.GetHashCode();
        }

        /// <summary>
        /// Determines whether the specified <see cref="BoundingFrustum" /> is equal to this instance.
        /// </summary>
        /// <param name="other">The <see cref="BoundingFrustum" /> to compare with this instance.</param>
        /// <returns><c>true</c> if the specified <see cref="BoundingFrustum" /> is equal to this instance; otherwise, <c>false</c>.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public bool Equals(ref BoundingFrustum other)
        {
            return m_PMatrix == other.m_PMatrix;
        }

        /// <summary>
        /// Determines whether the specified <see cref="BoundingFrustum" /> is equal to this instance.
        /// </summary>
        /// <param name="other">The <see cref="BoundingFrustum" /> to compare with this instance.</param>
        /// <returns><c>true</c> if the specified <see cref="BoundingFrustum" /> is equal to this instance; otherwise, <c>false</c>.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public bool Equals(BoundingFrustum other)
        {
            return Equals(ref other);
        }

        /// <summary>
        /// Determines whether the specified <see cref="System.Object" /> is equal to this instance.
        /// </summary>
        /// <param name="obj">The <see cref="System.Object" /> to compare with this instance.</param>
        /// <returns><c>true</c> if the specified <see cref="System.Object" /> is equal to this instance; otherwise, <c>false</c>.</returns>
        public override bool Equals(object obj)
        {
            return obj is BoundingFrustum other && Equals(ref other);
        }

        /// <summary>
        /// Implements the operator ==.
        /// </summary>
        /// <param name="left">The left.</param>
        /// <param name="right">The right.</param>
        /// <returns>The result of the operator.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool operator ==(BoundingFrustum left, BoundingFrustum right)
        {
            return left.Equals(ref right);
        }

        /// <summary>
        /// Implements the operator !=.
        /// </summary>
        /// <param name="left">The left.</param>
        /// <param name="right">The right.</param>
        /// <returns>The result of the operator.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool operator !=(BoundingFrustum left, BoundingFrustum right)
        {
            return !left.Equals(ref right);
        }

        /// <summary>
        /// Returns one of the 6 planes related to this frustum.
        /// </summary>
        /// <param name="index">Plane index where 0 fro Left, 1 for Right, 2 for Top, 3 for Bottom, 4 for Near, 5 for Far</param>
        /// <returns>The frustum plane.</returns>
        public Plane GetPlane(int index)
        {
            switch (index)
            {
            case 0: return m_PLeft;
            case 1: return m_PRight;
            case 2: return m_PTop;
            case 3: return m_PBottom;
            case 4: return m_PNear;
            case 5: return m_PFar;
            default: return new Plane();
            }
        }

        private static void GetPlanesFromMatrix(ref Matrix matrix, out Plane near, out Plane far, out Plane left, out Plane right, out Plane top, out Plane bottom)
        {
            //http://www.chadvernon.com/blog/resources/directx9/frustum-culling/

            // Left plane
            left.Normal.X = matrix.M14 + matrix.M11;
            left.Normal.Y = matrix.M24 + matrix.M21;
            left.Normal.Z = matrix.M34 + matrix.M31;
            left.D = matrix.M44 + matrix.M41;
            left.Normalize();

            // Right plane
            right.Normal.X = matrix.M14 - matrix.M11;
            right.Normal.Y = matrix.M24 - matrix.M21;
            right.Normal.Z = matrix.M34 - matrix.M31;
            right.D = matrix.M44 - matrix.M41;
            right.Normalize();

            // Top plane
            top.Normal.X = matrix.M14 - matrix.M12;
            top.Normal.Y = matrix.M24 - matrix.M22;
            top.Normal.Z = matrix.M34 - matrix.M32;
            top.D = matrix.M44 - matrix.M42;
            top.Normalize();

            // Bottom plane
            bottom.Normal.X = matrix.M14 + matrix.M12;
            bottom.Normal.Y = matrix.M24 + matrix.M22;
            bottom.Normal.Z = matrix.M34 + matrix.M32;
            bottom.D = matrix.M44 + matrix.M42;
            bottom.Normalize();

            // Near plane
            near.Normal.X = matrix.M13;
            near.Normal.Y = matrix.M23;
            near.Normal.Z = matrix.M33;
            near.D = matrix.M43;
            near.Normalize();

            // Far plane
            far.Normal.X = matrix.M14 - matrix.M13;
            far.Normal.Y = matrix.M24 - matrix.M23;
            far.Normal.Z = matrix.M34 - matrix.M33;
            far.D = matrix.M44 - matrix.M43;
            far.Normalize();
        }

        private static Vector3 Get3PlanesInterPoint(ref Plane p1, ref Plane p2, ref Plane p3)
        {
            Vector3.Cross(ref p2.Normal, ref p3.Normal, out var n2Xn3);
            Vector3.Cross(ref p3.Normal, ref p1.Normal, out var n3Xn1);
            Vector3.Cross(ref p1.Normal, ref p2.Normal, out var n1Xn2);
            var div1 = Vector3.Dot(ref p1.Normal, ref n2Xn3);
            var div2 = Vector3.Dot(ref p2.Normal, ref n3Xn1);
            var div3 = Vector3.Dot(ref p3.Normal, ref n1Xn2);
            if (Mathf.IsZero(div1 * div2 * div3))
                return Vector3.Zero;
            return n2Xn3 * (-p1.D / div1) - n3Xn1 * (p2.D / div2) - n1Xn2 * (p3.D / div3);
        }

        /// <summary>
        /// Creates a new frustum relaying on perspective camera parameters
        /// </summary>
        /// <param name="cameraPos">The camera pos.</param>
        /// <param name="lookDir">The look dir.</param>
        /// <param name="upDir">Up dir.</param>
        /// <param name="fov">The fov.</param>
        /// <param name="znear">The Z near.</param>
        /// <param name="zfar">The Z far.</param>
        /// <param name="aspect">The aspect.</param>
        /// <returns>The bounding frustum calculated from perspective camera</returns>
        public static BoundingFrustum FromCamera(Vector3 cameraPos, Vector3 lookDir, Vector3 upDir, float fov, float znear, float zfar, float aspect)
        {
            //http://knol.google.com/k/view-frustum

            lookDir = Vector3.Normalize(lookDir);
            upDir = Vector3.Normalize(upDir);

            Vector3 nearCenter = cameraPos + lookDir * znear;
            Vector3 farCenter = cameraPos + lookDir * zfar;
            var nearHalfHeight = (float)(znear * Math.Tan(fov / 2f));
            var farHalfHeight = (float)(zfar * Math.Tan(fov / 2f));
            float nearHalfWidth = nearHalfHeight * aspect;
            float farHalfWidth = farHalfHeight * aspect;

            Vector3 rightDir = Vector3.Normalize(Vector3.Cross(upDir, lookDir));
            Vector3 near1 = nearCenter - nearHalfHeight * upDir + nearHalfWidth * rightDir;
            Vector3 near2 = nearCenter + nearHalfHeight * upDir + nearHalfWidth * rightDir;
            Vector3 near3 = nearCenter + nearHalfHeight * upDir - nearHalfWidth * rightDir;
            Vector3 near4 = nearCenter - nearHalfHeight * upDir - nearHalfWidth * rightDir;
            Vector3 far1 = farCenter - farHalfHeight * upDir + farHalfWidth * rightDir;
            Vector3 far2 = farCenter + farHalfHeight * upDir + farHalfWidth * rightDir;
            Vector3 far3 = farCenter + farHalfHeight * upDir - farHalfWidth * rightDir;
            Vector3 far4 = farCenter - farHalfHeight * upDir - farHalfWidth * rightDir;

            var result = new BoundingFrustum
            {
                m_PNear = new Plane(near1, near2, near3),
                m_PFar = new Plane(far3, far2, far1),
                m_PLeft = new Plane(near4, near3, far3),
                m_PRight = new Plane(far1, far2, near2),
                m_PTop = new Plane(near2, far2, far3),
                m_PBottom = new Plane(far4, far1, near1)
            };

            result.m_PNear.Normalize();
            result.m_PFar.Normalize();
            result.m_PLeft.Normalize();
            result.m_PRight.Normalize();
            result.m_PTop.Normalize();
            result.m_PBottom.Normalize();

            result.m_PMatrix = Matrix.LookAt(cameraPos, cameraPos + lookDir * 10, upDir) * Matrix.PerspectiveFov(fov, aspect, znear, zfar);

            return result;
        }

        /// <summary>
        /// Returns the 8 corners of the frustum, element0 is Near1 (near right down corner)
        /// , element1 is Near2 (near right top corner)
        /// , element2 is Near3 (near Left top corner)
        /// , element3 is Near4 (near Left down corner)
        /// , element4 is Far1 (far right down corner)
        /// , element5 is Far2 (far right top corner)
        /// , element6 is Far3 (far left top corner)
        /// , element7 is Far4 (far left down corner)
        /// </summary>
        /// <returns>The 8 corners of the frustum</returns>
        public Vector3[] GetCorners()
        {
            var corners = new Vector3[8];
            GetCorners(corners);
            return corners;
        }

        /// <summary>
        /// Returns the 8 corners of the frustum, element0 is Near1 (near right down corner)
        /// , element1 is Near2 (near right top corner)
        /// , element2 is Near3 (near Left top corner)
        /// , element3 is Near4 (near Left down corner)
        /// , element4 is Far1 (far right down corner)
        /// , element5 is Far2 (far right top corner)
        /// , element6 is Far3 (far left top corner)
        /// , element7 is Far4 (far left down corner)
        /// </summary>
        /// <returns>The 8 corners of the frustum</returns>
        public void GetCorners(Vector3[] corners)
        {
            corners[0] = Get3PlanesInterPoint(ref m_PNear, ref m_PBottom, ref m_PRight); //Near1
            corners[1] = Get3PlanesInterPoint(ref m_PNear, ref m_PTop, ref m_PRight); //Near2
            corners[2] = Get3PlanesInterPoint(ref m_PNear, ref m_PTop, ref m_PLeft); //Near3
            corners[3] = Get3PlanesInterPoint(ref m_PNear, ref m_PBottom, ref m_PLeft); //Near3
            corners[4] = Get3PlanesInterPoint(ref m_PFar, ref m_PBottom, ref m_PRight); //Far1
            corners[5] = Get3PlanesInterPoint(ref m_PFar, ref m_PTop, ref m_PRight); //Far2
            corners[6] = Get3PlanesInterPoint(ref m_PFar, ref m_PTop, ref m_PLeft); //Far3
            corners[7] = Get3PlanesInterPoint(ref m_PFar, ref m_PBottom, ref m_PLeft); //Far3
        }

        /// <summary>
        /// Checks whether a point lay inside, intersects or lay outside the frustum.
        /// </summary>
        /// <param name="point">The point.</param>
        /// <returns>Type of the containment</returns>
        public ContainmentType Contains(ref Vector3 point)
        {
            var result = PlaneIntersectionType.Front;
            var planeResult = PlaneIntersectionType.Front;
            for (var i = 0; i < 6; i++)
            {
                switch (i)
                {
                case 0:
                    planeResult = m_PNear.Intersects(ref point);
                    break;
                case 1:
                    planeResult = m_PFar.Intersects(ref point);
                    break;
                case 2:
                    planeResult = m_PLeft.Intersects(ref point);
                    break;
                case 3:
                    planeResult = m_PRight.Intersects(ref point);
                    break;
                case 4:
                    planeResult = m_PTop.Intersects(ref point);
                    break;
                case 5:
                    planeResult = m_PBottom.Intersects(ref point);
                    break;
                }
                switch (planeResult)
                {
                case PlaneIntersectionType.Back: return ContainmentType.Disjoint;
                case PlaneIntersectionType.Intersecting:
                    result = PlaneIntersectionType.Intersecting;
                    break;
                }
            }
            switch (result)
            {
            case PlaneIntersectionType.Intersecting: return ContainmentType.Intersects;
            default: return ContainmentType.Contains;
            }
        }

        /// <summary>
        /// Checks whether a point lay inside, intersects or lay outside the frustum.
        /// </summary>
        /// <param name="point">The point.</param>
        /// <returns>Type of the containment</returns>
        public ContainmentType Contains(Vector3 point)
        {
            return Contains(ref point);
        }

        private void GetBoxToPlanePVertexNVertex(ref BoundingBox box, ref Vector3 planeNormal, out Vector3 p, out Vector3 n)
        {
            p = box.Minimum;
            if (planeNormal.X >= 0)
                p.X = box.Maximum.X;
            if (planeNormal.Y >= 0)
                p.Y = box.Maximum.Y;
            if (planeNormal.Z >= 0)
                p.Z = box.Maximum.Z;

            n = box.Maximum;
            if (planeNormal.X >= 0)
                n.X = box.Minimum.X;
            if (planeNormal.Y >= 0)
                n.Y = box.Minimum.Y;
            if (planeNormal.Z >= 0)
                n.Z = box.Minimum.Z;
        }

        /// <summary>
        /// Determines the intersection relationship between the frustum and a bounding box.
        /// </summary>
        /// <param name="box">The box.</param>
        /// <returns>Type of the containment</returns>
        public ContainmentType Contains(ref BoundingBox box)
        {
            var result = ContainmentType.Contains;
            for (var i = 0; i < 6; i++)
            {
                var plane = GetPlane(i);
                GetBoxToPlanePVertexNVertex(ref box, ref plane.Normal, out var p, out var n);
                if (CollisionsHelper.PlaneIntersectsPoint(ref plane, ref p) == PlaneIntersectionType.Back)
                    return ContainmentType.Disjoint;

                if (CollisionsHelper.PlaneIntersectsPoint(ref plane, ref n) == PlaneIntersectionType.Back)
                    result = ContainmentType.Intersects;
            }
            return result;
        }

        /// <summary>
        /// Determines the intersection relationship between the frustum and a bounding box.
        /// </summary>
        /// <param name="box">The box.</param>
        /// <returns>Type of the containment</returns>
        public ContainmentType Contains(BoundingBox box)
        {
            return Contains(ref box);
        }

        /// <summary>
        /// Determines the intersection relationship between the frustum and a bounding box.
        /// </summary>
        /// <param name="box">The box.</param>
        /// <param name="result">Type of the containment.</param>
        public void Contains(ref BoundingBox box, out ContainmentType result)
        {
            result = Contains(ref box);
        }

        /// <summary>
        /// Determines the intersection relationship between the frustum and a bounding sphere.
        /// </summary>
        /// <param name="sphere">The sphere.</param>
        /// <returns>Type of the containment</returns>
        public ContainmentType Contains(ref BoundingSphere sphere)
        {
            var result = PlaneIntersectionType.Front;
            var planeResult = PlaneIntersectionType.Front;
            for (var i = 0; i < 6; i++)
            {
                switch (i)
                {
                case 0:
                    planeResult = m_PNear.Intersects(ref sphere);
                    break;
                case 1:
                    planeResult = m_PFar.Intersects(ref sphere);
                    break;
                case 2:
                    planeResult = m_PLeft.Intersects(ref sphere);
                    break;
                case 3:
                    planeResult = m_PRight.Intersects(ref sphere);
                    break;
                case 4:
                    planeResult = m_PTop.Intersects(ref sphere);
                    break;
                case 5:
                    planeResult = m_PBottom.Intersects(ref sphere);
                    break;
                }
                switch (planeResult)
                {
                case PlaneIntersectionType.Back: return ContainmentType.Disjoint;
                case PlaneIntersectionType.Intersecting:
                    result = PlaneIntersectionType.Intersecting;
                    break;
                }
            }
            switch (result)
            {
            case PlaneIntersectionType.Intersecting: return ContainmentType.Intersects;
            default: return ContainmentType.Contains;
            }
        }

        /// <summary>
        /// Determines the intersection relationship between the frustum and a bounding sphere.
        /// </summary>
        /// <param name="sphere">The sphere.</param>
        /// <returns>Type of the containment</returns>
        public ContainmentType Contains(BoundingSphere sphere)
        {
            return Contains(ref sphere);
        }

        /// <summary>
        /// Determines the intersection relationship between the frustum and a bounding sphere.
        /// </summary>
        /// <param name="sphere">The sphere.</param>
        /// <param name="result">Type of the containment.</param>
        public void Contains(ref BoundingSphere sphere, out ContainmentType result)
        {
            result = Contains(ref sphere);
        }

        /// <summary>
        /// Checks whether the current BoundingFrustum intersects a BoundingSphere.
        /// </summary>
        /// <param name="sphere">The sphere.</param>
        /// <returns>Type of the containment</returns>
        public bool Intersects(ref BoundingSphere sphere)
        {
            return Contains(ref sphere) != ContainmentType.Disjoint;
        }

        /// <summary>
        /// Checks whether the current BoundingFrustum intersects a BoundingSphere.
        /// </summary>
        /// <param name="sphere">The sphere.</param>
        /// <param name="result">Set to <c>true</c> if the current BoundingFrustum intersects a BoundingSphere.</param>
        public void Intersects(ref BoundingSphere sphere, out bool result)
        {
            result = Contains(ref sphere) != ContainmentType.Disjoint;
        }

        /// <summary>
        /// Checks whether the current BoundingFrustum intersects a BoundingBox.
        /// </summary>
        /// <param name="box">The box.</param>
        /// <returns><c>true</c> if the current BoundingFrustum intersects a BoundingSphere.</returns>
        public bool Intersects(ref BoundingBox box)
        {
            return Contains(ref box) != ContainmentType.Disjoint;
        }

        /// <summary>
        /// Checks whether the current BoundingFrustum intersects a BoundingBox.
        /// </summary>
        /// <param name="box">The box.</param>
        /// <param name="result"><c>true</c> if the current BoundingFrustum intersects a BoundingSphere.</param>
        public void Intersects(ref BoundingBox box, out bool result)
        {
            result = Contains(ref box) != ContainmentType.Disjoint;
        }

        private PlaneIntersectionType PlaneIntersectsPoints(ref Plane plane, Vector3[] points)
        {
            PlaneIntersectionType result = CollisionsHelper.PlaneIntersectsPoint(ref plane, ref points[0]);
            for (var i = 1; i < points.Length; i++)
                if (CollisionsHelper.PlaneIntersectsPoint(ref plane, ref points[i]) != result)
                    return PlaneIntersectionType.Intersecting;
            return result;
        }

        /// <summary>
        /// Checks whether the current BoundingFrustum intersects the specified Plane.
        /// </summary>
        /// <param name="plane">The plane.</param>
        /// <returns>Plane intersection type.</returns>
        public PlaneIntersectionType Intersects(ref Plane plane)
        {
            return PlaneIntersectsPoints(ref plane, GetCorners());
        }

        /// <summary>
        /// Checks whether the current BoundingFrustum intersects the specified Plane.
        /// </summary>
        /// <param name="plane">The plane.</param>
        /// <param name="result">Plane intersection type.</param>
        public void Intersects(ref Plane plane, out PlaneIntersectionType result)
        {
            result = PlaneIntersectsPoints(ref plane, GetCorners());
        }

        /// <summary>
        /// Get the width of the frustum at specified depth.
        /// </summary>
        /// <param name="depth">the depth at which to calculate frustum width.</param>
        /// <returns>With of the frustum at the specified depth</returns>
        public float GetWidthAtDepth(float depth)
        {
            var hAngle = (float)(Math.PI / 2.0 - Math.Acos(Vector3.Dot(m_PNear.Normal, m_PLeft.Normal)));
            return (float)(Math.Tan(hAngle) * depth * 2);
        }

        /// <summary>
        /// Get the height of the frustum at specified depth.
        /// </summary>
        /// <param name="depth">the depth at which to calculate frustum height.</param>
        /// <returns>Height of the frustum at the specified depth</returns>
        public float GetHeightAtDepth(float depth)
        {
            var vAngle = (float)(Math.PI / 2.0 - Math.Acos(Vector3.Dot(m_PNear.Normal, m_PTop.Normal)));
            return (float)(Math.Tan(vAngle) * depth * 2);
        }

        private BoundingFrustum GetInsideOutClone()
        {
            BoundingFrustum frustum = this;
            frustum.m_PNear.Normal = -frustum.m_PNear.Normal;
            frustum.m_PFar.Normal = -frustum.m_PFar.Normal;
            frustum.m_PLeft.Normal = -frustum.m_PLeft.Normal;
            frustum.m_PRight.Normal = -frustum.m_PRight.Normal;
            frustum.m_PTop.Normal = -frustum.m_PTop.Normal;
            frustum.m_PBottom.Normal = -frustum.m_PBottom.Normal;
            return frustum;
        }

        /// <summary>
        /// Checks whether the current BoundingFrustum intersects the specified Ray.
        /// </summary>
        /// <param name="ray">The ray.</param>
        /// <returns><c>true</c> if the current BoundingFrustum intersects the specified Ray.</returns>
        public bool Intersects(ref Ray ray)
        {
            return Intersects(ref ray, out _, out _);
        }

        /// <summary>
        /// Checks whether the current BoundingFrustum intersects the specified Ray.
        /// </summary>
        /// <param name="ray">The Ray to check for intersection with.</param>
        /// <param name="inDistance">The distance at which the ray enters the frustum if there is an intersection and the ray starts outside the frustum.</param>
        /// <param name="outDistance">The distance at which the ray exits the frustum if there is an intersection.</param>
        /// <returns><c>true</c> if the current BoundingFrustum intersects the specified Ray.</returns>
        public bool Intersects(ref Ray ray, out Real? inDistance, out Real? outDistance)
        {
            if (Contains(ray.Position) != ContainmentType.Disjoint)
            {
                Real nearstPlaneDistance = Real.MaxValue;
                for (var i = 0; i < 6; i++)
                {
                    Plane plane = GetPlane(i);
                    if (CollisionsHelper.RayIntersectsPlane(ref ray, ref plane, out Real distance) && (distance < nearstPlaneDistance))
                        nearstPlaneDistance = distance;
                }

                inDistance = nearstPlaneDistance;
                outDistance = null;
                return true;
            }
            //We will find the two points at which the ray enters and exists the frustum
            //These two points make a line which center inside the frustum if the ray intersects it
            //Or outside the frustum if the ray intersects frustum planes outside it.
            Real minDist = Real.MaxValue;
            Real maxDist = Real.MinValue;
            for (var i = 0; i < 6; i++)
            {
                Plane plane = GetPlane(i);
                if (CollisionsHelper.RayIntersectsPlane(ref ray, ref plane, out Real distance))
                {
                    minDist = Mathf.Min(minDist, distance);
                    maxDist = Mathf.Max(maxDist, distance);
                }
            }

            Vector3 minPoint = ray.Position + ray.Direction * minDist;
            Vector3 maxPoint = ray.Position + ray.Direction * maxDist;
            Vector3 center = (minPoint + maxPoint) / 2f;
            if (Contains(ref center) != ContainmentType.Disjoint)
            {
                inDistance = minDist;
                outDistance = maxDist;
                return true;
            }
            inDistance = null;
            outDistance = null;
            return false;
        }

        /// <summary>
        /// Get the distance which when added to camera position along the lookat direction will do the effect of zoom to extents (zoom to fit) operation, so all the passed points will fit in the current view.
        /// if the returned value is positive, the camera will move toward the lookat direction (ZoomIn).
        /// if the returned value is negative, the camera will move in the reverse direction of the lookat direction (ZoomOut).
        /// </summary>
        /// <param name="points">The points.</param>
        /// <returns>The zoom to fit distance</returns>
        public Real GetZoomToExtentsShiftDistance(Vector3[] points)
        {
            var vAngle = (float)(Math.PI / 2.0 - Math.Acos(Vector3.Dot(m_PNear.Normal, m_PTop.Normal)));
            var vSin = (float)Math.Sin(vAngle);
            var hAngle = (float)(Math.PI / 2.0 - Math.Acos(Vector3.Dot(m_PNear.Normal, m_PLeft.Normal)));
            var hSin = (float)Math.Sin(hAngle);
            float horizontalToVerticalMapping = vSin / hSin;

            BoundingFrustum ioFrustrum = GetInsideOutClone();

            var maxPointDist = Real.MinValue;
            for (var i = 0; i < points.Length; i++)
            {
                var pointDist = CollisionsHelper.DistancePlanePoint(ref ioFrustrum.m_PTop, ref points[i]);
                pointDist = Mathf.Max(pointDist, CollisionsHelper.DistancePlanePoint(ref ioFrustrum.m_PBottom, ref points[i]));
                pointDist = Mathf.Max(pointDist, CollisionsHelper.DistancePlanePoint(ref ioFrustrum.m_PLeft, ref points[i]) * horizontalToVerticalMapping);
                pointDist = Mathf.Max(pointDist, CollisionsHelper.DistancePlanePoint(ref ioFrustrum.m_PRight, ref points[i]) * horizontalToVerticalMapping);
                maxPointDist = Mathf.Max(maxPointDist, pointDist);
            }
            return -maxPointDist / vSin;
        }

        /// <summary>
        /// Get the distance which when added to camera position along the lookat direction will do the effect of zoom to extents (zoom to fit) operation, so all the passed points will fit in the current view.
        /// if the returned value is positive, the camera will move toward the lookat direction (ZoomIn).
        /// if the returned value is negative, the camera will move in the reverse direction of the lookat direction (ZoomOut).
        /// </summary>
        /// <param name="boundingBox">The bounding box.</param>
        /// <returns>The zoom to fit distance</returns>
        public Real GetZoomToExtentsShiftDistance(ref BoundingBox boundingBox)
        {
            return GetZoomToExtentsShiftDistance(boundingBox.GetCorners());
        }

        /// <summary>
        /// Get the vector shift which when added to camera position will do the effect of zoom to extents (zoom to fit) operation, so all the passed points will fit in the current view.
        /// </summary>
        /// <param name="points">The points.</param>
        /// <returns>The zoom to fit vector</returns>
        public Vector3 GetZoomToExtentsShiftVector(Vector3[] points)
        {
            return GetZoomToExtentsShiftDistance(points) * m_PNear.Normal;
        }

        /// <summary>
        /// Get the vector shift which when added to camera position will do the effect of zoom to extents (zoom to fit) operation, so all the passed points will fit in the current view.</summary>
        /// <param name="boundingBox">The bounding box.</param>
        /// <returns>The zoom to fit vector</returns>
        public Vector3 GetZoomToExtentsShiftVector(ref BoundingBox boundingBox)
        {
            return GetZoomToExtentsShiftDistance(boundingBox.GetCorners()) * m_PNear.Normal;
        }

        /// <summary>
        /// Indicate whether the current BoundingFrustum is Orthographic.
        /// </summary>
        public bool IsOrthographic => (m_PLeft.Normal == -m_PRight.Normal) && (m_PTop.Normal == -m_PBottom.Normal);
    }
}
