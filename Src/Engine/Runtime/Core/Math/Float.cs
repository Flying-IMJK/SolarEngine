// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

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
// -----------------------------------------------------------------------------
// Original code from SlimMath project. http://code.google.com/p/slimmath/
// Greetings to SlimDX Group. Original code published with the following license:
// -----------------------------------------------------------------------------
/*
* Copyright (c) 2007-2011 SlimDX Group
* 
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
* 
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
* 
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE.
*/

using System;
using System.Globalization;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace SE
{
    [Serializable]
#if SE_EDITOR
    [System.ComponentModel.TypeConverter(typeof(TypeConverters.Float2Converter))]
#endif
    public partial struct Float2 : IEquatable<Float2>, IFormattable
    {
        private static readonly string _formatString = "X:{0:F2} Y:{1:F2}";

        /// <summary>
        /// The size of the <see cref="Float2" /> type, in bytes.
        /// </summary>
        public static readonly int SizeInBytes = Marshal.SizeOf(typeof(Float2));

        /// <summary>
        /// A <see cref="Float2" /> with all of its components set to zero.
        /// </summary>
        public static readonly Float2 Zero;

        /// <summary>
        /// The X unit <see cref="Float2" /> (1, 0).
        /// </summary>
        public static readonly Float2 UnitX = new Float2(1.0f, 0.0f);

        /// <summary>
        /// The Y unit <see cref="Float2" /> (0, 1).
        /// </summary>
        public static readonly Float2 UnitY = new Float2(0.0f, 1.0f);

        /// <summary>
        /// A <see cref="Float2" /> with all of its components set to half.
        /// </summary>
        public static readonly Float2 Half = new Float2(0.5f, 0.5f);

        /// <summary>
        /// A <see cref="Float2" /> with all of its components set to one.
        /// </summary>
        public static readonly Float2 One = new Float2(1.0f, 1.0f);

        /// <summary>
        /// A <see cref="Float2" /> with all components equal to <see cref="float.MinValue"/>.
        /// </summary>
        public static readonly Float2 Minimum = new Float2(float.MinValue);

        /// <summary>
        /// A <see cref="Float2" /> with all components equal to <see cref="float.MaxValue"/>.
        /// </summary>
        public static readonly Float2 Maximum = new Float2(float.MaxValue);

        /// <summary>
        /// Initializes a new instance of the <see cref="Float2" /> struct.
        /// </summary>
        /// <param name="value">The value that will be assigned to all components.</param>
        public Float2(float value)
        {
            X = value;
            Y = value;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Float2" /> struct.
        /// </summary>
        /// <param name="x">Initial value for the X component of the vector.</param>
        /// <param name="y">Initial value for the Y component of the vector.</param>
        public Float2(float x, float y)
        {
            X = x;
            Y = y;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Float2" /> struct.
        /// </summary>
        /// <param name="value">A vector containing the values with which to initialize the X, Y and Z components.</param>
        public Float2(Vector2 value)
        {
            X = (float)value.X;
            Y = (float)value.Y;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Float2" /> struct.
        /// </summary>
        /// <param name="value">A vector containing the values with which to initialize the X, Y and Z components.</param>
        public Float2(Double2 value)
        {
            X = (float)value.X;
            Y = (float)value.Y;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Float2" /> struct.
        /// </summary>
        /// <param name="value">A vector containing the values with which to initialize the X and Y components.</param>
        public Float2(Float3 value)
        {
            X = value.X;
            Y = value.Y;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Float2" /> struct.
        /// </summary>
        /// <param name="value">A vector containing the values with which to initialize the X and Y components.</param>
        public Float2(Float4 value)
        {
            X = value.X;
            Y = value.Y;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Float2" /> struct.
        /// </summary>
        /// <param name="values">The values to assign to the X and Y components of the vector. This must be an array with two elements.</param>
        /// <exception cref="ArgumentNullException">Thrown when <paramref name="values" /> is <c>null</c>.</exception>
        /// <exception cref="ArgumentOutOfRangeException">Thrown when <paramref name="values" /> contains more or less than two elements.</exception>
        public Float2(float[] values)
        {
            if (values == null)
                throw new ArgumentNullException(nameof(values));
            if (values.Length != 2)
                throw new ArgumentOutOfRangeException(nameof(values),
                    "There must be two and only two input values for Float2.");
            X = values[0];
            Y = values[1];
        }

        /// <summary>
        /// Gets a value indicting whether this instance is normalized.
        /// </summary>
        public bool IsNormalized => Mathf.IsOne(X * X + Y * Y);

        /// <summary>
        /// Gets a value indicting whether this vector is zero
        /// </summary>
        public bool IsZero => Mathf.IsZero(X) && Mathf.IsZero(Y);

        /// <summary>
        /// Gets a minimum component value
        /// </summary>
        public float MinValue => Mathf.Min(X, Y);

        /// <summary>
        /// Gets a maximum component value
        /// </summary>
        public float MaxValue => Mathf.Max(X, Y);

        /// <summary>
        /// Gets an arithmetic average value of all vector components.
        /// </summary>
        public float AvgValue => (X + Y) * (1.0f / 2.0f);

        /// <summary>
        /// Gets a sum of the component values.
        /// </summary>
        public float ValuesSum => X + Y;

        /// <summary>
        /// Gets a vector with values being absolute values of that vector.
        /// </summary>
        public Float2 Absolute => new Float2(Mathf.Abs(X), Mathf.Abs(Y));

        /// <summary>
        /// Gets a vector with values being opposite to values of that vector.
        /// </summary>
        public Float2 Negative => new Float2(-X, -Y);

        /// <summary>
        /// Gets or sets the component at the specified index.
        /// </summary>
        /// <value>The value of the X or Y component, depending on the index.</value>
        /// <param name="index">The index of the component to access. Use 0 for the X component and 1 for the Y component.</param>
        /// <returns>The value of the component at the specified index.</returns>
        /// <exception cref="System.ArgumentOutOfRangeException">Thrown when the <paramref name="index" /> is out of the range [0,1].</exception>
        public float this[int index]
        {
            get
            {
                switch (index)
                {
                    case 0: return X;
                    case 1: return Y;
                }

                throw new ArgumentOutOfRangeException(nameof(index), "Indices for Float2 run from 0 to 1, inclusive.");
            }
            set
            {
                switch (index)
                {
                    case 0:
                        X = value;
                        break;
                    case 1:
                        Y = value;
                        break;
                    default:
                        throw new ArgumentOutOfRangeException(nameof(index),
                            "Indices for Float2 run from 0 to 1, inclusive.");
                }
            }
        }

        /// <summary>
        /// Calculates the length of the vector.
        /// </summary>
        /// <returns>The length of the vector.</returns>
        /// <remarks><see cref="Float2.LengthSquared" /> may be preferred when only the relative length is needed and speed is of the essence.</remarks>
        public float Length => (float)Math.Sqrt(X * X + Y * Y);

        /// <summary>
        /// Calculates the squared length of the vector.
        /// </summary>
        /// <returns>The squared length of the vector.</returns>
        /// <remarks>This method may be preferred to <see cref="Float2.Length" /> when only a relative length is needed and speed is of the essence.</remarks>
        public float LengthSquared => X * X + Y * Y;

        /// <summary>
        /// Gets the normalized vector. Returned vector has length equal 1.
        /// </summary>
        public Float2 Normalized
        {
            get
            {
                Float2 result = this;
                result.Normalize();
                return result;
            }
        }

        /// <summary>
        /// Converts the vector into a unit vector.
        /// </summary>
        public void Normalize()
        {
            float length = Length;
            if (length >= Mathf.Epsilon)
            {
                float inv = 1.0f / length;
                X *= inv;
                Y *= inv;
            }
        }

        /// <summary>
        /// Creates an array containing the elements of the vector.
        /// </summary>
        public float[] ToArray()
        {
            return new[] { X, Y };
        }

        /// <summary>
        /// Adds two vectors.
        /// </summary>
        /// <param name="left">The first vector to add.</param>
        /// <param name="right">The second vector to add.</param>
        /// <param name="result">When the method completes, contains the sum of the two vectors.</param>
        public static void Add(ref Float2 left, ref Float2 right, out Float2 result)
        {
            result = new Float2(left.X + right.X, left.Y + right.Y);
        }

        /// <summary>
        /// Adds two vectors.
        /// </summary>
        /// <param name="left">The first vector to add.</param>
        /// <param name="right">The second vector to add.</param>
        /// <returns>The sum of the two vectors.</returns>
        public static Float2 Add(Float2 left, Float2 right)
        {
            return new Float2(left.X + right.X, left.Y + right.Y);
        }

        /// <summary>
        /// Performs a component-wise addition.
        /// </summary>
        /// <param name="left">The input vector</param>
        /// <param name="right">The scalar value to be added to elements</param>
        /// <param name="result">The vector with added scalar for each element.</param>
        public static void Add(ref Float2 left, ref float right, out Float2 result)
        {
            result = new Float2(left.X + right, left.Y + right);
        }

        /// <summary>
        /// Performs a component-wise addition.
        /// </summary>
        /// <param name="left">The input vector</param>
        /// <param name="right">The scalar value to be added to elements</param>
        /// <returns>The vector with added scalar for each element.</returns>
        public static Float2 Add(Float2 left, float right)
        {
            return new Float2(left.X + right, left.Y + right);
        }

        /// <summary>
        /// Subtracts two vectors.
        /// </summary>
        /// <param name="left">The first vector to subtract.</param>
        /// <param name="right">The second vector to subtract.</param>
        /// <param name="result">When the method completes, contains the difference of the two vectors.</param>
        public static void Subtract(ref Float2 left, ref Float2 right, out Float2 result)
        {
            result = new Float2(left.X - right.X, left.Y - right.Y);
        }

        /// <summary>
        /// Subtracts two vectors.
        /// </summary>
        /// <param name="left">The first vector to subtract.</param>
        /// <param name="right">The second vector to subtract.</param>
        /// <returns>The difference of the two vectors.</returns>
        public static Float2 Subtract(Float2 left, Float2 right)
        {
            return new Float2(left.X - right.X, left.Y - right.Y);
        }

        /// <summary>
        /// Performs a component-wise subtraction.
        /// </summary>
        /// <param name="left">The input vector</param>
        /// <param name="right">The scalar value to be subtracted from elements</param>
        /// <param name="result">The vector with subtracted scalar for each element.</param>
        public static void Subtract(ref Float2 left, ref float right, out Float2 result)
        {
            result = new Float2(left.X - right, left.Y - right);
        }

        /// <summary>
        /// Performs a component-wise subtraction.
        /// </summary>
        /// <param name="left">The input vector</param>
        /// <param name="right">The scalar value to be subtracted from elements</param>
        /// <returns>The vector with subtracted scalar for each element.</returns>
        public static Float2 Subtract(Float2 left, float right)
        {
            return new Float2(left.X - right, left.Y - right);
        }

        /// <summary>
        /// Performs a component-wise subtraction.
        /// </summary>
        /// <param name="left">The scalar value to be subtracted from elements</param>
        /// <param name="right">The input vector</param>
        /// <param name="result">The vector with subtracted scalar for each element.</param>
        public static void Subtract(ref float left, ref Float2 right, out Float2 result)
        {
            result = new Float2(left - right.X, left - right.Y);
        }

        /// <summary>
        /// Performs a component-wise subtraction.
        /// </summary>
        /// <param name="left">The scalar value to be subtracted from elements</param>
        /// <param name="right">The input vector</param>
        /// <returns>The vector with subtracted scalar for each element.</returns>
        public static Float2 Subtract(float left, Float2 right)
        {
            return new Float2(left - right.X, left - right.Y);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <param name="result">When the method completes, contains the scaled vector.</param>
        public static void Multiply(ref Float2 value, float scale, out Float2 result)
        {
            result = new Float2(value.X * scale, value.Y * scale);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The scaled vector.</returns>
        public static Float2 Multiply(Float2 value, float scale)
        {
            return new Float2(value.X * scale, value.Y * scale);
        }

        /// <summary>
        /// Multiplies a vector with another by performing component-wise multiplication.
        /// </summary>
        /// <param name="left">The first vector to multiply.</param>
        /// <param name="right">The second vector to multiply.</param>
        /// <param name="result">When the method completes, contains the multiplied vector.</param>
        public static void Multiply(ref Float2 left, ref Float2 right, out Float2 result)
        {
            result = new Float2(left.X * right.X, left.Y * right.Y);
        }

        /// <summary>
        /// Multiplies a vector with another by performing component-wise multiplication.
        /// </summary>
        /// <param name="left">The first vector to multiply.</param>
        /// <param name="right">The second vector to multiply.</param>
        /// <returns>The multiplied vector.</returns>
        public static Float2 Multiply(Float2 left, Float2 right)
        {
            return new Float2(left.X * right.X, left.Y * right.Y);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <param name="result">When the method completes, contains the scaled vector.</param>
        public static void Divide(ref Float2 value, float scale, out Float2 result)
        {
            result = new Float2(value.X / scale, value.Y / scale);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The scaled vector.</returns>
        public static Float2 Divide(Float2 value, float scale)
        {
            return new Float2(value.X / scale, value.Y / scale);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <param name="value">The vector to scale.</param>
        /// <param name="result">When the method completes, contains the scaled vector.</param>
        public static void Divide(float scale, ref Float2 value, out Float2 result)
        {
            result = new Float2(scale / value.X, scale / value.Y);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The scaled vector.</returns>
        public static Float2 Divide(float scale, Float2 value)
        {
            return new Float2(scale / value.X, scale / value.Y);
        }

        /// <summary>
        /// Reverses the direction of a given vector.
        /// </summary>
        /// <param name="value">The vector to negate.</param>
        /// <param name="result">When the method completes, contains a vector facing in the opposite direction.</param>
        public static void Negate(ref Float2 value, out Float2 result)
        {
            result = new Float2(-value.X, -value.Y);
        }

        /// <summary>
        /// Reverses the direction of a given vector.
        /// </summary>
        /// <param name="value">The vector to negate.</param>
        /// <returns>A vector facing in the opposite direction.</returns>
        public static Float2 Negate(Float2 value)
        {
            return new Float2(-value.X, -value.Y);
        }

        /// <summary>
        /// Returns a <see cref="Float2" /> containing the 2D Cartesian coordinates of a point specified in Barycentric
        /// coordinates relative to a 2D triangle.
        /// </summary>
        /// <param name="value1">A <see cref="Float2" /> containing the 2D Cartesian coordinates of vertex 1 of the triangle.</param>
        /// <param name="value2">A <see cref="Float2" /> containing the 2D Cartesian coordinates of vertex 2 of the triangle.</param>
        /// <param name="value3">A <see cref="Float2" /> containing the 2D Cartesian coordinates of vertex 3 of the triangle.</param>
        /// <param name="amount1">Barycentric coordinate b2, which expresses the weighting factor toward vertex 2 (specified in <paramref name="value2" />).</param>
        /// <param name="amount2">Barycentric coordinate b3, which expresses the weighting factor toward vertex 3 (specified in <paramref name="value3" />).</param>
        /// <param name="result">When the method completes, contains the 2D Cartesian coordinates of the specified point.</param>
        public static void Barycentric(ref Float2 value1, ref Float2 value2, ref Float2 value3, float amount1,
            float amount2, out Float2 result)
        {
            result = new Float2(value1.X + amount1 * (value2.X - value1.X) + amount2 * (value3.X - value1.X),
                value1.Y + amount1 * (value2.Y - value1.Y) + amount2 * (value3.Y - value1.Y));
        }

        /// <summary>
        /// Returns a <see cref="Float2" /> containing the 2D Cartesian coordinates of a point specified in Barycentric
        /// coordinates relative to a 2D triangle.
        /// </summary>
        /// <param name="value1">A <see cref="Float2" /> containing the 2D Cartesian coordinates of vertex 1 of the triangle.</param>
        /// <param name="value2">A <see cref="Float2" /> containing the 2D Cartesian coordinates of vertex 2 of the triangle.</param>
        /// <param name="value3">A <see cref="Float2" /> containing the 2D Cartesian coordinates of vertex 3 of the triangle.</param>
        /// <param name="amount1">Barycentric coordinate b2, which expresses the weighting factor toward vertex 2 (specified in <paramref name="value2" />).</param>
        /// <param name="amount2">Barycentric coordinate b3, which expresses the weighting factor toward vertex 3 (specified in <paramref name="value3" />).</param>
        /// <returns>A new <see cref="Float2" /> containing the 2D Cartesian coordinates of the specified point.</returns>
        public static Float2 Barycentric(Float2 value1, Float2 value2, Float2 value3, float amount1, float amount2)
        {
            Barycentric(ref value1, ref value2, ref value3, amount1, amount2, out Float2 result);
            return result;
        }

        /// <summary>
        /// Restricts a value to be within a specified range.
        /// </summary>
        /// <param name="value">The value to clamp.</param>
        /// <param name="min">The minimum value.</param>
        /// <param name="max">The maximum value.</param>
        /// <param name="result">When the method completes, contains the clamped value.</param>
        public static void Clamp(ref Float2 value, ref Float2 min, ref Float2 max, out Float2 result)
        {
            float x = value.X;
            x = x > max.X ? max.X : x;
            x = x < min.X ? min.X : x;
            float y = value.Y;
            y = y > max.Y ? max.Y : y;
            y = y < min.Y ? min.Y : y;
            result = new Float2(x, y);
        }

        /// <summary>
        /// Restricts a value to be within a specified range.
        /// </summary>
        /// <param name="value">The value to clamp.</param>
        /// <param name="min">The minimum value.</param>
        /// <param name="max">The maximum value.</param>
        /// <returns>The clamped value.</returns>
        public static Float2 Clamp(Float2 value, Float2 min, Float2 max)
        {
            Clamp(ref value, ref min, ref max, out Float2 result);
            return result;
        }

        /// <summary>
        /// Saturates this instance in the range [0,1].
        /// </summary>
        public void Saturate()
        {
            X = X < 0.0f ? 0.0f : X > 1.0f ? 1.0f : X;
            Y = Y < 0.0f ? 0.0f : Y > 1.0f ? 1.0f : Y;
        }

        /// <summary>
        /// Calculates the area of the triangle.
        /// </summary>
        /// <param name="v0">The first triangle vertex.</param>
        /// <param name="v1">The second triangle vertex.</param>
        /// <param name="v2">The third triangle vertex.</param>
        /// <returns>The triangle area.</returns>
        public static float TriangleArea(ref Float2 v0, ref Float2 v1, ref Float2 v2)
        {
            return Mathf.Abs((v0.X * (v1.Y - v2.Y) + v1.X * (v2.Y - v0.Y) + v2.X * (v0.Y - v1.Y)) / 2);
        }

        /// <summary>
        /// Calculates the distance between two vectors.
        /// </summary>
        /// <param name="value1">The first vector.</param>
        /// <param name="value2">The second vector.</param>
        /// <param name="result">When the method completes, contains the distance between the two vectors.</param>
        /// <remarks><see cref="Float2.DistanceSquared(ref Float2, ref Float2, out float)" /> may be preferred when only the relative distance is needed and speed is of the essence.</remarks>
        public static void Distance(ref Float2 value1, ref Float2 value2, out float result)
        {
            float x = value1.X - value2.X;
            float y = value1.Y - value2.Y;
            result = (float)Math.Sqrt(x * x + y * y);
        }

        /// <summary>
        /// Calculates the distance between two vectors.
        /// </summary>
        /// <param name="value1">The first vector.</param>
        /// <param name="value2">The second vector.</param>
        /// <returns>The distance between the two vectors.</returns>
        /// <remarks><see cref="Float2.DistanceSquared(Float2, Float2)" /> may be preferred when only the relative distance is needed and speed is of the essence.</remarks>
        public static float Distance(Float2 value1, Float2 value2)
        {
            float x = value1.X - value2.X;
            float y = value1.Y - value2.Y;
            return (float)Math.Sqrt(x * x + y * y);
        }

        /// <summary>
        /// Calculates the distance between two vectors.
        /// </summary>
        /// <param name="value1">The first vector.</param>
        /// <param name="value2">The second vector.</param>
        /// <returns>The distance between the two vectors.</returns>
        /// <remarks><see cref="Float2.DistanceSquared(ref Float2, ref Float2, out float)" /> may be preferred when only the relative distance is needed and speed is of the essence.</remarks>
        public static float Distance(ref Float2 value1, ref Float2 value2)
        {
            float x = value1.X - value2.X;
            float y = value1.Y - value2.Y;
            return (float)Math.Sqrt(x * x + y * y);
        }

        /// <summary>
        /// Calculates the squared distance between two vectors.
        /// </summary>
        /// <param name="value1">The first vector.</param>
        /// <param name="value2">The second vector</param>
        /// <param name="result">When the method completes, contains the squared distance between the two vectors.</param>
        public static void DistanceSquared(ref Float2 value1, ref Float2 value2, out float result)
        {
            float x = value1.X - value2.X;
            float y = value1.Y - value2.Y;
            result = x * x + y * y;
        }

        /// <summary>
        /// Calculates the squared distance between two vectors.
        /// </summary>
        /// <param name="value1">The first vector.</param>
        /// <param name="value2">The second vector</param>
        /// <returns>The squared distance between the two vectors.</returns>
        public static float DistanceSquared(ref Float2 value1, ref Float2 value2)
        {
            float x = value1.X - value2.X;
            float y = value1.Y - value2.Y;
            return x * x + y * y;
        }

        /// <summary>
        /// Calculates the squared distance between two vectors.
        /// </summary>
        /// <param name="value1">The first vector.</param>
        /// <param name="value2">The second vector.</param>
        /// <returns>The squared distance between the two vectors.</returns>
        public static float DistanceSquared(Float2 value1, Float2 value2)
        {
            float x = value1.X - value2.X;
            float y = value1.Y - value2.Y;
            return x * x + y * y;
        }

        /// <summary>
        /// Tests whether one vector is near another vector.
        /// </summary>
        /// <param name="left">The left vector.</param>
        /// <param name="right">The right vector.</param>
        /// <param name="epsilon">The epsilon.</param>
        /// <returns><c>true</c> if left and right are near, <c>false</c> otherwise</returns>
        public static bool NearEqual(Float2 left, Float2 right, float epsilon = Mathf.Epsilon)
        {
            return NearEqual(ref left, ref right, epsilon);
        }

        /// <summary>
        /// Tests whether one vector is near another vector.
        /// </summary>
        /// <param name="left">The left vector.</param>
        /// <param name="right">The right vector.</param>
        /// <param name="epsilon">The epsilon.</param>
        /// <returns><c>true</c> if left and right are near another, <c>false</c> otherwise</returns>
        public static bool NearEqual(ref Float2 left, ref Float2 right, float epsilon = Mathf.Epsilon)
        {
            return Mathf.WithinEpsilon(left.X, right.X, epsilon) && Mathf.WithinEpsilon(left.Y, right.Y, epsilon);
        }

        /// <summary>
        /// Calculates the dot product of two vectors.
        /// </summary>
        /// <param name="left">First source vector.</param>
        /// <param name="right">Second source vector.</param>
        /// <param name="result">When the method completes, contains the dot product of the two vectors.</param>
        public static void Dot(ref Float2 left, ref Float2 right, out float result)
        {
            result = left.X * right.X + left.Y * right.Y;
        }

        /// <summary>
        /// Calculates the dot product of two vectors.
        /// </summary>
        /// <param name="left">First source vector.</param>
        /// <param name="right">Second source vector.</param>
        /// <returns>The dot product of the two vectors.</returns>
        public static float Dot(ref Float2 left, ref Float2 right)
        {
            return left.X * right.X + left.Y * right.Y;
        }

        /// <summary>
        /// Calculates the dot product of two vectors.
        /// </summary>
        /// <param name="left">First source vector.</param>
        /// <param name="right">Second source vector.</param>
        /// <returns>The dot product of the two vectors.</returns>
        public static float Dot(Float2 left, Float2 right)
        {
            return left.X * right.X + left.Y * right.Y;
        }

        /// <summary>
        /// Calculates the cross product of two vectors.
        /// </summary>
        /// <param name="left">First source vector.</param>
        /// <param name="right">Second source vector.</param>
        /// <param name="result">When the method completes, contains the cross product of the two vectors.</param>
        public static void Cross(ref Float2 left, ref Float2 right, out float result)
        {
            result = left.X * right.Y - left.Y * right.X;
        }

        /// <summary>
        /// Calculates the cross product of two vectors.
        /// </summary>
        /// <param name="left">First source vector.</param>
        /// <param name="right">Second source vector.</param>
        /// <returns>The cross product of the two vectors.</returns>
        public static float Cross(ref Float2 left, ref Float2 right)
        {
            return left.X * right.Y - left.Y * right.X;
        }

        /// <summary>
        /// Calculates the cross product of two vectors.
        /// </summary>
        /// <param name="left">First source vector.</param>
        /// <param name="right">Second source vector.</param>
        /// <returns>The cross product of the two vectors.</returns>
        public static float Cross(Float2 left, Float2 right)
        {
            return left.X * right.Y - left.Y * right.X;
        }

        /// <summary>
        /// Converts the vector into a unit vector.
        /// </summary>
        /// <param name="value">The vector to normalize.</param>
        /// <param name="result">When the method completes, contains the normalized vector.</param>
        public static void Normalize(ref Float2 value, out Float2 result)
        {
            result = value;
            result.Normalize();
        }

        /// <summary>
        /// Converts the vector into a unit vector.
        /// </summary>
        /// <param name="value">The vector to normalize.</param>
        /// <returns>The normalized vector.</returns>
        public static Float2 Normalize(Float2 value)
        {
            value.Normalize();
            return value;
        }

        /// <summary>
        /// Makes sure that Length of the output vector is always below max and above 0.
        /// </summary>
        /// <param name="vector">Input vector.</param>
        /// <param name="max">Max Length</param>
        public static Float2 ClampLength(Float2 vector, float max)
        {
            return ClampLength(vector, 0, max);
        }

        /// <summary>
        /// Makes sure that Length of the output vector is always below max and above min.
        /// </summary>
        /// <param name="vector">Input vector.</param>
        /// <param name="min">Min Length</param>
        /// <param name="max">Max Length</param>
        public static Float2 ClampLength(Float2 vector, float min, float max)
        {
            ClampLength(vector, min, max, out Float2 result);
            return result;
        }

        /// <summary>
        /// Makes sure that Length of the output vector is always below max and above min.
        /// </summary>
        /// <param name="vector">Input vector.</param>
        /// <param name="min">Min Length</param>
        /// <param name="max">Max Length</param>
        /// <param name="result">The result value.</param>
        public static void ClampLength(Float2 vector, float min, float max, out Float2 result)
        {
            result = vector;
            float lenSq = result.LengthSquared;
            if (lenSq > max * max)
            {
                float scaleFactor = max / (float)Math.Sqrt(lenSq);
                result.X *= scaleFactor;
                result.Y *= scaleFactor;
            }

            if (lenSq < min * min)
            {
                float scaleFactor = min / (float)Math.Sqrt(lenSq);
                result.X *= scaleFactor;
                result.Y *= scaleFactor;
            }
        }

        /// <summary>
        /// Returns the vector with components rounded to the nearest integer.
        /// </summary>
        /// <param name="v">The value.</param>
        /// <returns>The result.</returns>
        public static Float2 Round(Float2 v)
        {
            return new Float2(Mathf.Round(v.X), Mathf.Round(v.Y));
        }

        /// <summary>
        /// Returns the vector with components containing the smallest integer greater to or equal to the original value.
        /// </summary>
        /// <param name="v">The value.</param>
        /// <returns>The result.</returns>
        public static Float2 Ceil(Float2 v)
        {
            return new Float2(Mathf.Ceil(v.X), Mathf.Ceil(v.Y));
        }

        /// <summary>
        /// Breaks the components of the vector into an integral and a fractional part. Returns vector made of fractional parts.
        /// </summary>
        /// <param name="v">The value.</param>
        /// <returns>The result.</returns>
        public static Float2 Mod(Float2 v)
        {
            return new Float2(v.X - (int)v.X, v.Y - (int)v.Y);
        }

        /// <summary>
        /// Performs a linear interpolation between two vectors.
        /// </summary>
        /// <param name="start">Start vector.</param>
        /// <param name="end">End vector.</param>
        /// <param name="amount">Value between 0 and 1 indicating the weight of <paramref name="end" />.</param>
        /// <param name="result">When the method completes, contains the linear interpolation of the two vectors.</param>
        /// <remarks>Passing <paramref name="amount" /> a value of 0 will cause <paramref name="start" /> to be returned; a value of 1 will cause <paramref name="end" /> to be returned.</remarks>
        public static void Lerp(ref Float2 start, ref Float2 end, float amount, out Float2 result)
        {
            result = default;
            result.X = Mathf.Lerp(start.X, end.X, amount);
            result.Y = Mathf.Lerp(start.Y, end.Y, amount);
        }

        /// <summary>
        /// Performs a linear interpolation between two vectors.
        /// </summary>
        /// <param name="start">Start vector.</param>
        /// <param name="end">End vector.</param>
        /// <param name="amount">Value between 0 and 1 indicating the weight of <paramref name="end" />.</param>
        /// <returns>The linear interpolation of the two vectors.</returns>
        /// <remarks>Passing <paramref name="amount" /> a value of 0 will cause <paramref name="start" /> to be returned; a value of 1 will cause <paramref name="end" /> to be returned.</remarks>
        public static Float2 Lerp(Float2 start, Float2 end, float amount)
        {
            Lerp(ref start, ref end, amount, out Float2 result);
            return result;
        }

        /// <summary>
        /// Performs a linear interpolation between two vectors.
        /// </summary>
        /// <param name="start">Start vector.</param>
        /// <param name="end">End vector.</param>
        /// <param name="amount">Value between 0 and 1 indicating the weight of <paramref name="end" />.</param>
        /// <param name="result">When the method completes, contains the linear interpolation of the two vectors.</param>
        /// <remarks>Passing <paramref name="amount" /> a value of 0 will cause <paramref name="start" /> to be returned; a value of 1 will cause <paramref name="end" /> to be returned.</remarks>
        public static void Lerp(ref Float2 start, ref Float2 end, ref Float2 amount, out Float2 result)
        {
            result = default;
            result.X = Mathf.Lerp(start.X, end.X, amount.X);
            result.Y = Mathf.Lerp(start.Y, end.Y, amount.Y);
        }

        /// <summary>
        /// Performs a linear interpolation between two vectors.
        /// </summary>
        /// <param name="start">Start vector.</param>
        /// <param name="end">End vector.</param>
        /// <param name="amount">Value between 0 and 1 indicating the weight of <paramref name="end" />.</param>
        /// <returns>The linear interpolation of the two vectors.</returns>
        /// <remarks>Passing <paramref name="amount" /> a value of 0 will cause <paramref name="start" /> to be returned; a value of 1 will cause <paramref name="end" /> to be returned.</remarks>
        public static Float2 Lerp(Float2 start, Float2 end, Float2 amount)
        {
            Lerp(ref start, ref end, ref amount, out Float2 result);
            return result;
        }

        /// <summary>
        /// Performs a cubic interpolation between two vectors.
        /// </summary>
        /// <param name="start">Start vector.</param>
        /// <param name="end">End vector.</param>
        /// <param name="amount">Value between 0 and 1 indicating the weight of <paramref name="end" />.</param>
        /// <param name="result">When the method completes, contains the cubic interpolation of the two vectors.</param>
        public static void SmoothStep(ref Float2 start, ref Float2 end, float amount, out Float2 result)
        {
            amount = Mathf.SmoothStep(amount);
            Lerp(ref start, ref end, amount, out result);
        }

        /// <summary>
        /// Performs a cubic interpolation between two vectors.
        /// </summary>
        /// <param name="start">Start vector.</param>
        /// <param name="end">End vector.</param>
        /// <param name="amount">Value between 0 and 1 indicating the weight of <paramref name="end" />.</param>
        /// <returns>The cubic interpolation of the two vectors.</returns>
        public static Float2 SmoothStep(Float2 start, Float2 end, float amount)
        {
            SmoothStep(ref start, ref end, amount, out Float2 result);
            return result;
        }

        /// <summary>
        /// Performs a Hermite spline interpolation.
        /// </summary>
        /// <param name="value1">First source position vector.</param>
        /// <param name="tangent1">First source tangent vector.</param>
        /// <param name="value2">Second source position vector.</param>
        /// <param name="tangent2">Second source tangent vector.</param>
        /// <param name="amount">Weighting factor.</param>
        /// <param name="result">When the method completes, contains the result of the Hermite spline interpolation.</param>
        public static void Hermite(ref Float2 value1, ref Float2 tangent1, ref Float2 value2, ref Float2 tangent2,
            float amount, out Float2 result)
        {
            result = default;
            float squared = amount * amount;
            float cubed = amount * squared;
            float part1 = 2.0f * cubed - 3.0f * squared + 1.0f;
            float part2 = -2.0f * cubed + 3.0f * squared;
            float part3 = cubed - 2.0f * squared + amount;
            float part4 = cubed - squared;
            result.X = value1.X * part1 + value2.X * part2 + tangent1.X * part3 + tangent2.X * part4;
            result.Y = value1.Y * part1 + value2.Y * part2 + tangent1.Y * part3 + tangent2.Y * part4;
        }

        /// <summary>
        /// Performs a Hermite spline interpolation.
        /// </summary>
        /// <param name="value1">First source position vector.</param>
        /// <param name="tangent1">First source tangent vector.</param>
        /// <param name="value2">Second source position vector.</param>
        /// <param name="tangent2">Second source tangent vector.</param>
        /// <param name="amount">Weighting factor.</param>
        /// <returns>The result of the Hermite spline interpolation.</returns>
        public static Float2 Hermite(Float2 value1, Float2 tangent1, Float2 value2, Float2 tangent2, float amount)
        {
            Hermite(ref value1, ref tangent1, ref value2, ref tangent2, amount, out Float2 result);
            return result;
        }

        /// <summary>
        /// Calculates the 2D vector perpendicular to the given 2D vector. The result is always rotated 90-degrees in a counter-clockwise direction for a 2D coordinate system where the positive Y axis goes up.
        /// </summary>
        /// <param name="inDirection">The input direction.</param>
        /// <returns>The result.</returns>
        public static Float2 Perpendicular(Float2 inDirection)
        {
            return new Float2(-inDirection.Y, inDirection.X);
        }

        /// <summary>
        /// Calculates the 2D vector perpendicular to the given 2D vector. The result is always rotated 90-degrees in a counter-clockwise direction for a 2D coordinate system where the positive Y axis goes up.
        /// </summary>
        /// <param name="inDirection">The in direction.</param>
        /// <param name="result">When the method completes, contains the result of the calculation.</param>
        public static void Perpendicular(ref Float2 inDirection, out Float2 result)
        {
            result = new Float2(-inDirection.Y, inDirection.X);
        }

        /// <summary>
        /// Performs a Catmull-Rom interpolation using the specified positions.
        /// </summary>
        /// <param name="value1">The first position in the interpolation.</param>
        /// <param name="value2">The second position in the interpolation.</param>
        /// <param name="value3">The third position in the interpolation.</param>
        /// <param name="value4">The fourth position in the interpolation.</param>
        /// <param name="amount">Weighting factor.</param>
        /// <param name="result">When the method completes, contains the result of the Catmull-Rom interpolation.</param>
        public static void CatmullRom(ref Float2 value1, ref Float2 value2, ref Float2 value3, ref Float2 value4,
            float amount, out Float2 result)
        {
            result = default;
            float squared = amount * amount;
            float cubed = amount * squared;
            result.X = 0.5f * (2.0f * value2.X + (-value1.X + value3.X) * amount +
                               (2.0f * value1.X - 5.0f * value2.X + 4.0f * value3.X - value4.X) * squared +
                               (-value1.X + 3.0f * value2.X - 3.0f * value3.X + value4.X) * cubed);
            result.Y = 0.5f * (2.0f * value2.Y + (-value1.Y + value3.Y) * amount +
                               (2.0f * value1.Y - 5.0f * value2.Y + 4.0f * value3.Y - value4.Y) * squared +
                               (-value1.Y + 3.0f * value2.Y - 3.0f * value3.Y + value4.Y) * cubed);
        }

        /// <summary>
        /// Performs a Catmull-Rom interpolation using the specified positions.
        /// </summary>
        /// <param name="value1">The first position in the interpolation.</param>
        /// <param name="value2">The second position in the interpolation.</param>
        /// <param name="value3">The third position in the interpolation.</param>
        /// <param name="value4">The fourth position in the interpolation.</param>
        /// <param name="amount">Weighting factor.</param>
        /// <returns>A vector that is the result of the Catmull-Rom interpolation.</returns>
        public static Float2 CatmullRom(Float2 value1, Float2 value2, Float2 value3, Float2 value4, float amount)
        {
            CatmullRom(ref value1, ref value2, ref value3, ref value4, amount, out Float2 result);
            return result;
        }

        /// <summary>
        /// Returns a vector containing the largest components of the specified vectors.
        /// </summary>
        /// <param name="left">The first source vector.</param>
        /// <param name="right">The second source vector.</param>
        /// <param name="result">When the method completes, contains an new vector composed of the largest components of the source vectors.</param>
        public static void Max(ref Float2 left, ref Float2 right, out Float2 result)
        {
            result = default;
            result.X = left.X > right.X ? left.X : right.X;
            result.Y = left.Y > right.Y ? left.Y : right.Y;
        }

        /// <summary>
        /// Returns a vector containing the largest components of the specified vectors.
        /// </summary>
        /// <param name="left">The first source vector.</param>
        /// <param name="right">The second source vector.</param>
        /// <returns>A vector containing the largest components of the source vectors.</returns>
        public static Float2 Max(Float2 left, Float2 right)
        {
            Max(ref left, ref right, out Float2 result);
            return result;
        }

        /// <summary>
        /// Returns a vector containing the smallest components of the specified vectors.
        /// </summary>
        /// <param name="left">The first source vector.</param>
        /// <param name="right">The second source vector.</param>
        /// <param name="result">When the method completes, contains an new vector composed of the smallest components of the source vectors.</param>
        public static void Min(ref Float2 left, ref Float2 right, out Float2 result)
        {
            result = default;
            result.X = left.X < right.X ? left.X : right.X;
            result.Y = left.Y < right.Y ? left.Y : right.Y;
        }

        /// <summary>
        /// Returns a vector containing the smallest components of the specified vectors.
        /// </summary>
        /// <param name="left">The first source vector.</param>
        /// <param name="right">The second source vector.</param>
        /// <returns>A vector containing the smallest components of the source vectors.</returns>
        public static Float2 Min(Float2 left, Float2 right)
        {
            Min(ref left, ref right, out Float2 result);
            return result;
        }

        /// <summary>
        /// Returns the absolute value of a vector.
        /// </summary>
        /// <param name="v">The value.</param>
        /// <returns> A vector which components are less or equal to 0.</returns>
        public static Float2 Abs(Float2 v)
        {
            return new Float2(Math.Abs(v.X), Math.Abs(v.Y));
        }

        /// <summary>
        /// Returns the reflection of a vector off a surface that has the specified normal.
        /// </summary>
        /// <param name="vector">The source vector.</param>
        /// <param name="normal">Normal of the surface.</param>
        /// <param name="result">When the method completes, contains the reflected vector.</param>
        /// <remarks>Reflect only gives the direction of a reflection off a surface, it does not determine whether the original vector was close enough to the surface to hit it.</remarks>
        public static void Reflect(ref Float2 vector, ref Float2 normal, out Float2 result)
        {
            result = default;
            float dot = vector.X * normal.X + vector.Y * normal.Y;
            result.X = vector.X - 2.0f * dot * normal.X;
            result.Y = vector.Y - 2.0f * dot * normal.Y;
        }

        /// <summary>
        /// Returns the reflection of a vector off a surface that has the specified normal.
        /// </summary>
        /// <param name="vector">The source vector.</param>
        /// <param name="normal">Normal of the surface.</param>
        /// <returns>The reflected vector.</returns>
        /// <remarks>Reflect only gives the direction of a reflection off a surface, it does not determine whether the original vector was close enough to the surface to hit it.</remarks>
        public static Float2 Reflect(Float2 vector, Float2 normal)
        {
            Reflect(ref vector, ref normal, out Float2 result);
            return result;
        }

        /// <summary>
        /// Transforms a 2D vector by the given <see cref="Quaternion" /> rotation.
        /// </summary>
        /// <param name="vector">The vector to rotate.</param>
        /// <param name="rotation">The <see cref="Quaternion" /> rotation to apply.</param>
        /// <param name="result">When the method completes, contains the transformed <see cref="Float2" />.</param>
        public static void Transform(ref Float2 vector, ref Quaternion rotation, out Float2 result)
        {
            float x = rotation.X + rotation.X;
            float y = rotation.Y + rotation.Y;
            float z = rotation.Z + rotation.Z;
            float wz = rotation.W * z;
            float xx = rotation.X * x;
            float xy = rotation.X * y;
            float yy = rotation.Y * y;
            float zz = rotation.Z * z;
            result = new Float2(vector.X * (1.0f - yy - zz) + vector.Y * (xy - wz),
                vector.X * (xy + wz) + vector.Y * (1.0f - xx - zz));
        }

        /// <summary>
        /// Transforms a 2D vector by the given <see cref="Quaternion" /> rotation.
        /// </summary>
        /// <param name="vector">The vector to rotate.</param>
        /// <param name="rotation">The <see cref="Quaternion" /> rotation to apply.</param>
        /// <returns>The transformed <see cref="Float2" />.</returns>
        public static Float2 Transform(Float2 vector, Quaternion rotation)
        {
            Transform(ref vector, ref rotation, out Float2 result);
            return result;
        }

        /// <summary>
        /// Transforms a 2D vector by the given <see cref="Matrix" />.
        /// </summary>
        /// <param name="vector">The source vector.</param>
        /// <param name="transform">The transformation <see cref="Matrix" />.</param>
        /// <param name="result">When the method completes, contains the transformed <see cref="Float4" />.</param>
        public static void Transform(ref Float2 vector, ref Matrix transform, out Float4 result)
        {
            result = new Float4(vector.X * transform.M11 + vector.Y * transform.M21 + transform.M41,
                vector.X * transform.M12 + vector.Y * transform.M22 + transform.M42,
                vector.X * transform.M13 + vector.Y * transform.M23 + transform.M43,
                vector.X * transform.M14 + vector.Y * transform.M24 + transform.M44);
        }

        /// <summary>
        /// Transforms a 2D vector by the given <see cref="Matrix" />.
        /// </summary>
        /// <param name="vector">The source vector.</param>
        /// <param name="transform">The transformation <see cref="Matrix" />.</param>
        /// <returns>The transformed <see cref="Float4" />.</returns>
        public static Float4 Transform(Float2 vector, Matrix transform)
        {
            Transform(ref vector, ref transform, out Float4 result);
            return result;
        }

        /// <summary>
        /// Performs a coordinate transformation using the given <see cref="Matrix" />.
        /// </summary>
        /// <param name="coordinate">The coordinate vector to transform.</param>
        /// <param name="transform">The transformation <see cref="Matrix" />.</param>
        /// <param name="result">When the method completes, contains the transformed coordinates.</param>
        /// <remarks>
        /// A coordinate transform performs the transformation with the assumption that the w component
        /// is one. The four dimensional vector obtained from the transformation operation has each
        /// component in the vector divided by the w component. This forces the w component to be one and
        /// therefore makes the vector homogeneous. The homogeneous vector is often preferred when working
        /// with coordinates as the w component can safely be ignored.
        /// </remarks>
        public static void TransformCoordinate(ref Float2 coordinate, ref Matrix transform, out Float2 result)
        {
            var vector = new Float4
            {
                X = coordinate.X * transform.M11 + coordinate.Y * transform.M21 + transform.M41,
                Y = coordinate.X * transform.M12 + coordinate.Y * transform.M22 + transform.M42,
                Z = coordinate.X * transform.M13 + coordinate.Y * transform.M23 + transform.M43,
                W = 1f / (coordinate.X * transform.M14 + coordinate.Y * transform.M24 + transform.M44)
            };
            result = new Float2(vector.X * vector.W, vector.Y * vector.W);
        }

        /// <summary>
        /// Performs a coordinate transformation using the given <see cref="Matrix" />.
        /// </summary>
        /// <param name="coordinate">The coordinate vector to transform.</param>
        /// <param name="transform">The transformation <see cref="Matrix" />.</param>
        /// <returns>The transformed coordinates.</returns>
        /// <remarks>
        /// A coordinate transform performs the transformation with the assumption that the w component
        /// is one. The four dimensional vector obtained from the transformation operation has each
        /// component in the vector divided by the w component. This forces the w component to be one and
        /// therefore makes the vector homogeneous. The homogeneous vector is often preferred when working
        /// with coordinates as the w component can safely be ignored.
        /// </remarks>
        public static Float2 TransformCoordinate(Float2 coordinate, Matrix transform)
        {
            TransformCoordinate(ref coordinate, ref transform, out Float2 result);
            return result;
        }

        /// <summary>
        /// Performs a normal transformation using the given <see cref="Matrix" />.
        /// </summary>
        /// <param name="normal">The normal vector to transform.</param>
        /// <param name="transform">The transformation <see cref="Matrix" />.</param>
        /// <param name="result">When the method completes, contains the transformed normal.</param>
        /// <remarks>
        /// A normal transform performs the transformation with the assumption that the w component
        /// is zero. This causes the fourth row and fourth column of the matrix to be unused. The
        /// end result is a vector that is not translated, but all other transformation properties
        /// apply. This is often preferred for normal vectors as normals purely represent direction
        /// rather than location because normal vectors should not be translated.
        /// </remarks>
        public static void TransformNormal(ref Float2 normal, ref Matrix transform, out Float2 result)
        {
            result = new Float2(normal.X * transform.M11 + normal.Y * transform.M21,
                normal.X * transform.M12 + normal.Y * transform.M22);
        }

        /// <summary>
        /// Performs a normal transformation using the given <see cref="Matrix" />.
        /// </summary>
        /// <param name="normal">The normal vector to transform.</param>
        /// <param name="transform">The transformation <see cref="Matrix" />.</param>
        /// <returns>The transformed normal.</returns>
        /// <remarks>
        /// A normal transform performs the transformation with the assumption that the w component
        /// is zero. This causes the fourth row and fourth column of the matrix to be unused. The
        /// end result is a vector that is not translated, but all other transformation properties
        /// apply. This is often preferred for normal vectors as normals purely represent direction
        /// rather than location because normal vectors should not be translated.
        /// </remarks>
        public static Float2 TransformNormal(Float2 normal, Matrix transform)
        {
            TransformNormal(ref normal, ref transform, out Float2 result);
            return result;
        }

        /// <summary>
        /// Snaps the input position into the grid.
        /// </summary>
        /// <param name="pos">The position to snap.</param>
        /// <param name="gridSize">The size of the grid.</param>
        /// <returns>The position snapped to the grid.</returns>
        public static Float2 SnapToGrid(Float2 pos, Float2 gridSize)
        {
            pos.X = Mathf.Ceil((pos.X - (gridSize.X * 0.5f)) / gridSize.X) * gridSize.X;
            pos.Y = Mathf.Ceil((pos.Y - (gridSize.Y * 0.5f)) / gridSize.Y) * gridSize.Y;
            return pos;
        }

        /// <summary>
        /// Adds two vectors.
        /// </summary>
        /// <param name="left">The first vector to add.</param>
        /// <param name="right">The second vector to add.</param>
        /// <returns>The sum of the two vectors.</returns>
        public static Float2 operator +(Float2 left, Float2 right)
        {
            return new Float2(left.X + right.X, left.Y + right.Y);
        }

        /// <summary>
        /// Multiplies a vector with another by performing component-wise multiplication equivalent to <see cref="Multiply(ref Float2,ref Float2,out Float2)" />.
        /// </summary>
        /// <param name="left">The first vector to multiply.</param>
        /// <param name="right">The second vector to multiply.</param>
        /// <returns>The multiplication of the two vectors.</returns>
        public static Float2 operator *(Float2 left, Float2 right)
        {
            return new Float2(left.X * right.X, left.Y * right.Y);
        }

        /// <summary>
        /// Assert a vector (return it unchanged).
        /// </summary>
        /// <param name="value">The vector to assert (unchanged).</param>
        /// <returns>The asserted (unchanged) vector.</returns>
        public static Float2 operator +(Float2 value)
        {
            return value;
        }

        /// <summary>
        /// Subtracts two vectors.
        /// </summary>
        /// <param name="left">The first vector to subtract.</param>
        /// <param name="right">The second vector to subtract.</param>
        /// <returns>The difference of the two vectors.</returns>
        public static Float2 operator -(Float2 left, Float2 right)
        {
            return new Float2(left.X - right.X, left.Y - right.Y);
        }

        /// <summary>
        /// Reverses the direction of a given vector.
        /// </summary>
        /// <param name="value">The vector to negate.</param>
        /// <returns>A vector facing in the opposite direction.</returns>
        public static Float2 operator -(Float2 value)
        {
            return new Float2(-value.X, -value.Y);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The scaled vector.</returns>
        public static Float2 operator *(float scale, Float2 value)
        {
            return new Float2(value.X * scale, value.Y * scale);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The scaled vector.</returns>
        public static Float2 operator *(Float2 value, float scale)
        {
            return new Float2(value.X * scale, value.Y * scale);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The scaled vector.</returns>
        public static Float2 operator /(Float2 value, float scale)
        {
            return new Float2(value.X / scale, value.Y / scale);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <param name="value">The vector to scale.</param>
        /// <returns>The scaled vector.</returns>
        public static Float2 operator /(float scale, Float2 value)
        {
            return new Float2(scale / value.X, scale / value.Y);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The scaled vector.</returns>
        public static Float2 operator *(double scale, Float2 value)
        {
            var s = (float)scale;
            return new Float2(value.X * s, value.Y * s);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The scaled vector.</returns>
        public static Float2 operator *(Float2 value, double scale)
        {
            var s = (float)scale;
            return new Float2(value.X * s, value.Y * s);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The scaled vector.</returns>
        public static Float2 operator /(Float2 value, double scale)
        {
            var s = (float)scale;
            return new Float2(value.X / s, value.Y / s);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <param name="value">The vector to scale.</param>
        /// <returns>The scaled vector.</returns>
        public static Float2 operator /(double scale, Float2 value)
        {
            var s = (float)scale;
            return new Float2(s / value.X, s / value.Y);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The scaled vector.</returns>
        public static Float2 operator /(Float2 value, Float2 scale)
        {
            return new Float2(value.X / scale.X, value.Y / scale.Y);
        }

        /// <summary>
        /// Remainder of value divided by scale.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The remained vector.</returns>
        public static Float2 operator %(Float2 value, float scale)
        {
            return new Float2(value.X % scale, value.Y % scale);
        }

        /// <summary>
        /// Remainder of value divided by scale.
        /// </summary>
        /// <param name="value">The amount by which to scale the vector.</param>
        /// <param name="scale">The vector to scale.</param>
        /// <returns>The remained vector.</returns>
        public static Float2 operator %(float value, Float2 scale)
        {
            return new Float2(value % scale.X, value % scale.Y);
        }

        /// <summary>
        /// Remainder of value divided by scale.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The remained vector.</returns>
        public static Float2 operator %(Float2 value, Float2 scale)
        {
            return new Float2(value.X % scale.X, value.Y % scale.Y);
        }

        /// <summary>
        /// Performs a component-wise addition.
        /// </summary>
        /// <param name="value">The input vector.</param>
        /// <param name="scalar">The scalar value to be added on elements</param>
        /// <returns>The vector with added scalar for each element.</returns>
        public static Float2 operator +(Float2 value, float scalar)
        {
            return new Float2(value.X + scalar, value.Y + scalar);
        }

        /// <summary>
        /// Performs a component-wise addition.
        /// </summary>
        /// <param name="value">The input vector.</param>
        /// <param name="scalar">The scalar value to be added on elements</param>
        /// <returns>The vector with added scalar for each element.</returns>
        public static Float2 operator +(float scalar, Float2 value)
        {
            return new Float2(scalar + value.X, scalar + value.Y);
        }

        /// <summary>
        /// Performs a component-wise subtraction.
        /// </summary>
        /// <param name="value">The input vector.</param>
        /// <param name="scalar">The scalar value to be subtracted from elements</param>
        /// <returns>The vector with subtracted scalar from each element.</returns>
        public static Float2 operator -(Float2 value, float scalar)
        {
            return new Float2(value.X - scalar, value.Y - scalar);
        }

        /// <summary>
        /// Performs a component-wise subtraction.
        /// </summary>
        /// <param name="value">The input vector.</param>
        /// <param name="scalar">The scalar value to be subtracted from elements</param>
        /// <returns>The vector with subtracted scalar from each element.</returns>
        public static Float2 operator -(float scalar, Float2 value)
        {
            return new Float2(scalar - value.X, scalar - value.Y);
        }

        /// <summary>
        /// Tests for equality between two objects.
        /// </summary>
        /// <param name="left">The first value to compare.</param>
        /// <param name="right">The second value to compare.</param>
        /// <returns><c>true</c> if <paramref name="left" /> has the same value as <paramref name="right" />; otherwise,<c>false</c>.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool operator ==(Float2 left, Float2 right)
        {
            return Mathf.NearEqual(left.X, right.X) && Mathf.NearEqual(left.Y, right.Y);
        }

        /// <summary>
        /// Tests for inequality between two objects.
        /// </summary>
        /// <param name="left">The first value to compare.</param>
        /// <param name="right">The second value to compare.</param>
        /// <returns><c>true</c> if <paramref name="left" /> has a different value than <paramref name="right" />; otherwise,<c>false</c>.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool operator !=(Float2 left, Float2 right)
        {
            return !Mathf.NearEqual(left.X, right.X) || !Mathf.NearEqual(left.Y, right.Y);
        }

        /// <summary>
        /// Performs an implicit conversion from <see cref="Float2" /> to <see cref="Vector2" />.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <returns>The result of the conversion.</returns>
        public static implicit operator Vector2(Float2 value)
        {
            return new Vector2(value.X, value.Y);
        }

        /// <summary>
        /// Performs an implicit conversion from <see cref="Float2" /> to <see cref="Double2" />.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <returns>The result of the conversion.</returns>
        public static implicit operator Double2(Float2 value)
        {
            return new Double2(value.X, value.Y);
        }

        /// <summary>
        /// Performs an explicit conversion from <see cref="Float2" /> to <see cref="Float3" />.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <returns>The result of the conversion.</returns>
        public static explicit operator Float3(Float2 value)
        {
            return new Float3(value, 0.0f);
        }

        /// <summary>
        /// Performs an explicit conversion from <see cref="Float2" /> to <see cref="Float4" />.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <returns>The result of the conversion.</returns>
        public static explicit operator Float4(Float2 value)
        {
            return new Float4(value, 0.0f, 0.0f);
        }

        /// <summary>
        /// Returns a <see cref="System.String" /> that represents this instance.
        /// </summary>
        /// <returns>A <see cref="System.String" /> that represents this instance.</returns>
        public override string ToString()
        {
            return string.Format(CultureInfo.CurrentCulture, "X:{0} Y:{1}", X, Y);
        }

        /// <summary>
        /// Returns a <see cref="System.String" /> that represents this instance.
        /// </summary>
        /// <param name="format">The format.</param>
        /// <returns>A <see cref="System.String" /> that represents this instance.</returns>
        public string ToString(string format)
        {
            if (format == null)
                return ToString();
            return string.Format(CultureInfo.CurrentCulture, _formatString,
                X.ToString(format, CultureInfo.CurrentCulture), Y.ToString(format, CultureInfo.CurrentCulture));
        }

        /// <summary>
        /// Returns a <see cref="System.String" /> that represents this instance.
        /// </summary>
        /// <param name="formatProvider">The format provider.</param>
        /// <returns>A <see cref="System.String" /> that represents this instance.</returns>
        public string ToString(IFormatProvider formatProvider)
        {
            return string.Format(formatProvider, _formatString, X, Y);
        }

        /// <summary>
        /// Returns a <see cref="System.String" /> that represents this instance.
        /// </summary>
        /// <param name="format">The format.</param>
        /// <param name="formatProvider">The format provider.</param>
        /// <returns>A <see cref="System.String" /> that represents this instance.</returns>
        public string ToString(string format, IFormatProvider formatProvider)
        {
            if (format == null)
                return ToString(formatProvider);
            return string.Format(formatProvider, _formatString, X.ToString(format, formatProvider),
                Y.ToString(format, formatProvider));
        }

        /// <summary>
        /// Returns a hash code for this instance.
        /// </summary>
        public override int GetHashCode()
        {
            unchecked
            {
                return (X.GetHashCode() * 397) ^ Y.GetHashCode();
            }
        }

        /// <summary>
        /// Determines whether the specified <see cref="Float2" /> is equal to this instance.
        /// </summary>
        /// <param name="other">The <see cref="Float2" /> to compare with this instance.</param>
        /// <returns><c>true</c> if the specified <see cref="Float2" /> is equal to this instance; otherwise, <c>false</c>.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public bool Equals(ref Float2 other)
        {
            return Mathf.NearEqual(other.X, X) && Mathf.NearEqual(other.Y, Y);
        }

        /// <summary>
        /// Determines whether the specified <see cref="Float2"/> are equal.
        /// </summary>
        public static bool Equals(ref Float2 a, ref Float2 b)
        {
            return Mathf.NearEqual(a.X, b.X) && Mathf.NearEqual(a.Y, b.Y);
        }

        /// <summary>
        /// Determines whether the specified <see cref="Float2" /> is equal to this instance.
        /// </summary>
        /// <param name="other">The <see cref="Float2" /> to compare with this instance.</param>
        /// <returns><c>true</c> if the specified <see cref="Float2" /> is equal to this instance; otherwise, <c>false</c>.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public bool Equals(Float2 other)
        {
            return Mathf.NearEqual(other.X, X) && Mathf.NearEqual(other.Y, Y);
        }

        /// <summary>
        /// Determines whether the specified <see cref="System.Object" /> is equal to this instance.
        /// </summary>
        /// <param name="value">The <see cref="System.Object" /> to compare with this instance.</param>
        /// <returns><c>true</c> if the specified <see cref="System.Object" /> is equal to this instance; otherwise, <c>false</c>.</returns>
        public override bool Equals(object value)
        {
            return value is Float2 other && Mathf.NearEqual(other.X, X) && Mathf.NearEqual(other.Y, Y);
        }
    }

    [Serializable]
#if SE_EDITOR
    [System.ComponentModel.TypeConverter(typeof(TypeConverters.Float3Converter))]
#endif
    public partial struct Float3 : IEquatable<Float3>, IFormattable
    {
        private static readonly string _formatString = "X:{0:F2} Y:{1:F2} Z:{2:F2}";

        /// <summary>
        /// The size of the <see cref="Float3" /> type, in bytes.
        /// </summary>
        public static readonly int SizeInBytes = Marshal.SizeOf(typeof(Float3));

        /// <summary>
        /// A <see cref="Float3" /> with all of its components set to zero.
        /// </summary>
        public static readonly Float3 Zero;

        /// <summary>
        /// The X unit <see cref="Float3" /> (1, 0, 0).
        /// </summary>
        public static readonly Float3 UnitX = new Float3(1.0f, 0.0f, 0.0f);

        /// <summary>
        /// The Y unit <see cref="Float3" /> (0, 1, 0).
        /// </summary>
        public static readonly Float3 UnitY = new Float3(0.0f, 1.0f, 0.0f);

        /// <summary>
        /// The Z unit <see cref="Float3" /> (0, 0, 1).
        /// </summary>
        public static readonly Float3 UnitZ = new Float3(0.0f, 0.0f, 1.0f);

        /// <summary>
        /// A <see cref="Float3" /> with all of its components set to one.
        /// </summary>
        public static readonly Float3 One = new Float3(1.0f, 1.0f, 1.0f);

        /// <summary>
        /// A <see cref="Float3" /> with all of its components set to half.
        /// </summary>
        public static readonly Float3 Half = new Float3(0.5f, 0.5f, 0.5f);

        /// <summary>
        /// A unit <see cref="Float3" /> designating up (0, 1, 0).
        /// </summary>
        public static readonly Float3 Up = new Float3(0.0f, 1.0f, 0.0f);

        /// <summary>
        /// A unit <see cref="Float3" /> designating down (0, -1, 0).
        /// </summary>
        public static readonly Float3 Down = new Float3(0.0f, -1.0f, 0.0f);

        /// <summary>
        /// A unit <see cref="Float3" /> designating left (-1, 0, 0).
        /// </summary>
        public static readonly Float3 Left = new Float3(-1.0f, 0.0f, 0.0f);

        /// <summary>
        /// A unit <see cref="Float3" /> designating right (1, 0, 0).
        /// </summary>
        public static readonly Float3 Right = new Float3(1.0f, 0.0f, 0.0f);

        /// <summary>
        /// A unit <see cref="Float3" /> designating forward in a left-handed coordinate system (0, 0, 1).
        /// </summary>
        public static readonly Float3 Forward = new Float3(0.0f, 0.0f, 1.0f);

        /// <summary>
        /// A unit <see cref="Float3" /> designating backward in a left-handed coordinate system (0, 0, -1).
        /// </summary>
        public static readonly Float3 Backward = new Float3(0.0f, 0.0f, -1.0f);

        /// <summary>
        /// A <see cref="Float3" /> with all components equal to <see cref="float.MinValue"/>.
        /// </summary>
        public static readonly Float3 Minimum = new Float3(float.MinValue);

        /// <summary>
        /// A <see cref="Float3" /> with all components equal to <see cref="float.MaxValue"/>.
        /// </summary>
        public static readonly Float3 Maximum = new Float3(float.MaxValue);

        /// <summary>
        /// Initializes a new instance of the <see cref="Float3" /> struct.
        /// </summary>
        /// <param name="value">The value that will be assigned to all components.</param>
        public Float3(float value)
        {
            X = value;
            Y = value;
            Z = value;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Float3" /> struct.
        /// </summary>
        /// <param name="x">Initial value for the X component of the vector.</param>
        /// <param name="y">Initial value for the Y component of the vector.</param>
        /// <param name="z">Initial value for the Z component of the vector.</param>
        public Float3(float x, float y, float z)
        {
            X = x;
            Y = y;
            Z = z;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Float3" /> struct.
        /// </summary>
        /// <param name="value">A vector containing the values with which to initialize the X and Y components.</param>
        /// <param name="z">Initial value for the Z component of the vector.</param>
        public Float3(Float2 value, float z)
        {
            X = value.X;
            Y = value.Y;
            Z = z;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Float3" /> struct.
        /// </summary>
        /// <param name="value">A vector containing the values with which to initialize the X, Y and Z components.</param>
        public Float3(Vector3 value)
        {
            X = (float)value.X;
            Y = (float)value.Y;
            Z = (float)value.Z;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Float3" /> struct.
        /// </summary>
        /// <param name="value">A vector containing the values with which to initialize the X, Y and Z components.</param>
        public Float3(Double3 value)
        {
            X = (float)value.X;
            Y = (float)value.Y;
            Z = (float)value.Z;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Float3" /> struct.
        /// </summary>
        /// <param name="value">A vector containing the values with which to initialize the X, Y and Z components.</param>
        public Float3(Float4 value)
        {
            X = value.X;
            Y = value.Y;
            Z = value.Z;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Float3" /> struct.
        /// </summary>
        /// <param name="values">The values to assign to the X, Y, and Z components of the vector. This must be an array with three elements.</param>
        /// <exception cref="ArgumentNullException">Thrown when <paramref name="values" /> is <c>null</c>.</exception>
        /// <exception cref="ArgumentOutOfRangeException"> Thrown when <paramref name="values" /> contains more or less than three elements.</exception>
        public Float3(float[] values)
        {
            if (values == null)
                throw new ArgumentNullException(nameof(values));
            if (values.Length != 3)
                throw new ArgumentOutOfRangeException(nameof(values),
                    "There must be three and only three input values for Float3.");
            X = values[0];
            Y = values[1];
            Z = values[2];
        }

        /// <summary>
        /// Gets a value indicting whether this instance is normalized.
        /// </summary>
        public bool IsNormalized => Mathf.IsOne(X * X + Y * Y + Z * Z);

        /// <summary>
        /// Gets the normalized vector. Returned vector has length equal 1.
        /// </summary>
        public Float3 Normalized
        {
            get
            {
                Float3 result = this;
                result.Normalize();
                return result;
            }
        }

        /// <summary>
        /// Gets a value indicting whether this vector is zero
        /// </summary>
        public bool IsZero => Mathf.IsZero(X) && Mathf.IsZero(Y) && Mathf.IsZero(Z);

        /// <summary>
        /// Gets a value indicting whether this vector is one
        /// </summary>
        public bool IsOne => Mathf.IsOne(X) && Mathf.IsOne(Y) && Mathf.IsOne(Z);

        /// <summary>
        /// Gets a minimum component value
        /// </summary>
        public float MinValue => Mathf.Min(X, Mathf.Min(Y, Z));

        /// <summary>
        /// Gets a maximum component value
        /// </summary>
        public float MaxValue => Mathf.Max(X, Mathf.Max(Y, Z));

        /// <summary>
        /// Gets an arithmetic average value of all vector components.
        /// </summary>
        public float AvgValue => (X + Y + Z) * (1.0f / 3.0f);

        /// <summary>
        /// Gets a sum of the component values.
        /// </summary>
        public float ValuesSum => X + Y + Z;

        /// <summary>
        /// Gets a vector with values being absolute values of that vector.
        /// </summary>
        public Float3 Absolute => new Float3(Mathf.Abs(X), Mathf.Abs(Y), Mathf.Abs(Z));

        /// <summary>
        /// Gets a vector with values being opposite to values of that vector.
        /// </summary>
        public Float3 Negative => new Float3(-X, -Y, -Z);

        /// <summary>
        /// Gets or sets the component at the specified index.
        /// </summary>
        /// <value>The value of the X, Y, or Z component, depending on the index.</value>
        /// <param name="index">The index of the component to access. Use 0 for the X component, 1 for the Y component, and 2 for the Z component.</param>
        /// <returns>The value of the component at the specified index.</returns>
        /// <exception cref="System.ArgumentOutOfRangeException">Thrown when the <paramref name="index" /> is out of the range [0, 2].</exception>
        public float this[int index]
        {
            get
            {
                switch (index)
                {
                    case 0: return X;
                    case 1: return Y;
                    case 2: return Z;
                }

                throw new ArgumentOutOfRangeException(nameof(index), "Indices for Float3 run from 0 to 2, inclusive.");
            }
            set
            {
                switch (index)
                {
                    case 0:
                        X = value;
                        break;
                    case 1:
                        Y = value;
                        break;
                    case 2:
                        Z = value;
                        break;
                    default:
                        throw new ArgumentOutOfRangeException(nameof(index),
                            "Indices for Float3 run from 0 to 2, inclusive.");
                }
            }
        }

        /// <summary>
        /// Calculates the length of the vector.
        /// </summary>
        /// <returns>The length of the vector.</returns>
        /// <remarks><see cref="Float3.LengthSquared" /> may be preferred when only the relative length is needed and speed is of the essence.</remarks>
        public float Length => (float)Math.Sqrt(X * X + Y * Y + Z * Z);

        /// <summary>
        /// Calculates the squared length of the vector.
        /// </summary>
        /// <returns>The squared length of the vector.</returns>
        /// <remarks>This method may be preferred to <see cref="Float3.Length" /> when only a relative length is needed and speed is of the essence.</remarks>
        public float LengthSquared => X * X + Y * Y + Z * Z;

        /// <summary>
        /// Converts the vector into a unit vector.
        /// </summary>
        public void Normalize()
        {
            float length = Length;
            if (length >= Mathf.Epsilon)
            {
                float inv = 1.0f / length;
                X *= inv;
                Y *= inv;
                Z *= inv;
            }
        }

        /// <summary>
        /// Creates an array containing the elements of the vector.
        /// </summary>
        /// <returns>A three-element array containing the components of the vector.</returns>
        public float[] ToArray()
        {
            return new[] { X, Y, Z };
        }

        /// <summary>
        /// Adds two vectors.
        /// </summary>
        /// <param name="left">The first vector to add.</param>
        /// <param name="right">The second vector to add.</param>
        /// <param name="result">When the method completes, contains the sum of the two vectors.</param>
        public static void Add(ref Float3 left, ref Float3 right, out Float3 result)
        {
            result = new Float3(left.X + right.X, left.Y + right.Y, left.Z + right.Z);
        }

        /// <summary>
        /// Adds two vectors.
        /// </summary>
        /// <param name="left">The first vector to add.</param>
        /// <param name="right">The second vector to add.</param>
        /// <returns>The sum of the two vectors.</returns>
        public static Float3 Add(Float3 left, Float3 right)
        {
            return new Float3(left.X + right.X, left.Y + right.Y, left.Z + right.Z);
        }

        /// <summary>
        /// Performs a component-wise addition.
        /// </summary>
        /// <param name="left">The input vector</param>
        /// <param name="right">The scalar value to be added to elements</param>
        /// <param name="result">The vector with added scalar for each element.</param>
        public static void Add(ref Float3 left, ref float right, out Float3 result)
        {
            result = new Float3(left.X + right, left.Y + right, left.Z + right);
        }

        /// <summary>
        /// Performs a component-wise addition.
        /// </summary>
        /// <param name="left">The input vector</param>
        /// <param name="right">The scalar value to be added to elements</param>
        /// <returns>The vector with added scalar for each element.</returns>
        public static Float3 Add(Float3 left, float right)
        {
            return new Float3(left.X + right, left.Y + right, left.Z + right);
        }

        /// <summary>
        /// Subtracts two vectors.
        /// </summary>
        /// <param name="left">The first vector to subtract.</param>
        /// <param name="right">The second vector to subtract.</param>
        /// <param name="result">When the method completes, contains the difference of the two vectors.</param>
        public static void Subtract(ref Float3 left, ref Float3 right, out Float3 result)
        {
            result = new Float3(left.X - right.X, left.Y - right.Y, left.Z - right.Z);
        }

        /// <summary>
        /// Subtracts two vectors.
        /// </summary>
        /// <param name="left">The first vector to subtract.</param>
        /// <param name="right">The second vector to subtract.</param>
        /// <returns>The difference of the two vectors.</returns>
        public static Float3 Subtract(Float3 left, Float3 right)
        {
            return new Float3(left.X - right.X, left.Y - right.Y, left.Z - right.Z);
        }

        /// <summary>
        /// Performs a component-wise subtraction.
        /// </summary>
        /// <param name="left">The input vector</param>
        /// <param name="right">The scalar value to be subtracted from elements</param>
        /// <param name="result">The vector with subtracted scalar for each element.</param>
        public static void Subtract(ref Float3 left, ref float right, out Float3 result)
        {
            result = new Float3(left.X - right, left.Y - right, left.Z - right);
        }

        /// <summary>
        /// Performs a component-wise subtraction.
        /// </summary>
        /// <param name="left">The input vector</param>
        /// <param name="right">The scalar value to be subtracted from elements</param>
        /// <returns>The vector with subtracted scalar for each element.</returns>
        public static Float3 Subtract(Float3 left, float right)
        {
            return new Float3(left.X - right, left.Y - right, left.Z - right);
        }

        /// <summary>
        /// Performs a component-wise subtraction.
        /// </summary>
        /// <param name="left">The scalar value to be subtracted from elements</param>
        /// <param name="right">The input vector.</param>
        /// <param name="result">The vector with subtracted scalar for each element.</param>
        public static void Subtract(ref float left, ref Float3 right, out Float3 result)
        {
            result = new Float3(left - right.X, left - right.Y, left - right.Z);
        }

        /// <summary>
        /// Performs a component-wise subtraction.
        /// </summary>
        /// <param name="left">The scalar value to be subtracted from elements</param>
        /// <param name="right">The input vector.</param>
        /// <returns>The vector with subtracted scalar for each element.</returns>
        public static Float3 Subtract(float left, Float3 right)
        {
            return new Float3(left - right.X, left - right.Y, left - right.Z);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <param name="result">When the method completes, contains the scaled vector.</param>
        public static void Multiply(ref Float3 value, float scale, out Float3 result)
        {
            result = new Float3(value.X * scale, value.Y * scale, value.Z * scale);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The scaled vector.</returns>
        public static Float3 Multiply(Float3 value, float scale)
        {
            return new Float3(value.X * scale, value.Y * scale, value.Z * scale);
        }

        /// <summary>
        /// Multiply a vector with another by performing component-wise multiplication.
        /// </summary>
        /// <param name="left">The first vector to multiply.</param>
        /// <param name="right">The second vector to multiply.</param>
        /// <param name="result">When the method completes, contains the multiplied vector.</param>
        public static void Multiply(ref Float3 left, ref Float3 right, out Float3 result)
        {
            result = new Float3(left.X * right.X, left.Y * right.Y, left.Z * right.Z);
        }

        /// <summary>
        /// Multiply a vector with another by performing component-wise multiplication.
        /// </summary>
        /// <param name="left">The first vector to Multiply.</param>
        /// <param name="right">The second vector to multiply.</param>
        /// <returns>The multiplied vector.</returns>
        public static Float3 Multiply(Float3 left, Float3 right)
        {
            return new Float3(left.X * right.X, left.Y * right.Y, left.Z * right.Z);
        }

        /// <summary>
        /// Divides a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector (per component).</param>
        /// <param name="result">When the method completes, contains the divided vector.</param>
        public static void Divide(ref Float3 value, ref Float3 scale, out Float3 result)
        {
            result = new Float3(value.X / scale.X, value.Y / scale.Y, value.Z / scale.Z);
        }

        /// <summary>
        /// Divides a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector (per component).</param>
        /// <returns>The divided vector.</returns>
        public static Float3 Divide(Float3 value, Float3 scale)
        {
            return new Float3(value.X / scale.X, value.Y / scale.Y, value.Z / scale.Z);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <param name="result">When the method completes, contains the scaled vector.</param>
        public static void Divide(ref Float3 value, float scale, out Float3 result)
        {
            result = new Float3(value.X / scale, value.Y / scale, value.Z / scale);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The scaled vector.</returns>
        public static Float3 Divide(Float3 value, float scale)
        {
            return new Float3(value.X / scale, value.Y / scale, value.Z / scale);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <param name="value">The vector to scale.</param>
        /// <param name="result">When the method completes, contains the scaled vector.</param>
        public static void Divide(float scale, ref Float3 value, out Float3 result)
        {
            result = new Float3(scale / value.X, scale / value.Y, scale / value.Z);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The scaled vector.</returns>
        public static Float3 Divide(float scale, Float3 value)
        {
            return new Float3(scale / value.X, scale / value.Y, scale / value.Z);
        }

        /// <summary>
        /// Reverses the direction of a given vector.
        /// </summary>
        /// <param name="value">The vector to negate.</param>
        /// <param name="result">When the method completes, contains a vector facing in the opposite direction.</param>
        public static void Negate(ref Float3 value, out Float3 result)
        {
            result = new Float3(-value.X, -value.Y, -value.Z);
        }

        /// <summary>
        /// Reverses the direction of a given vector.
        /// </summary>
        /// <param name="value">The vector to negate.</param>
        /// <returns>A vector facing in the opposite direction.</returns>
        public static Float3 Negate(Float3 value)
        {
            return new Float3(-value.X, -value.Y, -value.Z);
        }

        /// <summary>
        /// Returns a <see cref="Float3" /> containing the 3D Cartesian coordinates of a point specified in Barycentric coordinates relative to a 3D triangle.
        /// </summary>
        /// <param name="value1">A <see cref="Float3" /> containing the 3D Cartesian coordinates of vertex 1 of the triangle.</param>
        /// <param name="value2">A <see cref="Float3" /> containing the 3D Cartesian coordinates of vertex 2 of the triangle.</param>
        /// <param name="value3">A <see cref="Float3" /> containing the 3D Cartesian coordinates of vertex 3 of the triangle.</param>
        /// <param name="amount1">Barycentric coordinate b2, which expresses the weighting factor toward vertex 2 (specified in <paramref name="value2" />).</param>
        /// <param name="amount2">Barycentric coordinate b3, which expresses the weighting factor toward vertex 3 (specified in <paramref name="value3" />).</param>
        /// <param name="result">When the method completes, contains the 3D Cartesian coordinates of the specified point.</param>
        public static void Barycentric(ref Float3 value1, ref Float3 value2, ref Float3 value3, float amount1,
            float amount2, out Float3 result)
        {
            result = new Float3(value1.X + amount1 * (value2.X - value1.X) + amount2 * (value3.X - value1.X),
                value1.Y + amount1 * (value2.Y - value1.Y) + amount2 * (value3.Y - value1.Y),
                value1.Z + amount1 * (value2.Z - value1.Z) + amount2 * (value3.Z - value1.Z));
        }

        /// <summary>
        /// Returns a <see cref="Float3" /> containing the 3D Cartesian coordinates of a point specified in Barycentric coordinates relative to a 3D triangle.
        /// </summary>
        /// <param name="value1">A <see cref="Float3" /> containing the 3D Cartesian coordinates of vertex 1 of the triangle.</param>
        /// <param name="value2">A <see cref="Float3" /> containing the 3D Cartesian coordinates of vertex 2 of the triangle.</param>
        /// <param name="value3">A <see cref="Float3" /> containing the 3D Cartesian coordinates of vertex 3 of the triangle.</param>
        /// <param name="amount1">Barycentric coordinate b2, which expresses the weighting factor toward vertex 2 (specified in <paramref name="value2" />).</param>
        /// <param name="amount2">Barycentric coordinate b3, which expresses the weighting factor toward vertex 3 (specified in <paramref name="value3" />).</param>
        /// <returns>A new <see cref="Float3" /> containing the 3D Cartesian coordinates of the specified point.</returns>
        public static Float3 Barycentric(Float3 value1, Float3 value2, Float3 value3, float amount1, float amount2)
        {
            Barycentric(ref value1, ref value2, ref value3, amount1, amount2, out var result);
            return result;
        }

        /// <summary>
        /// Restricts a value to be within a specified range.
        /// </summary>
        /// <param name="value">The value to clamp.</param>
        /// <param name="min">The minimum value.</param>
        /// <param name="max">The maximum value.</param>
        /// <param name="result">When the method completes, contains the clamped value.</param>
        public static void Clamp(ref Float3 value, ref Float3 min, ref Float3 max, out Float3 result)
        {
            float x = value.X;
            x = x > max.X ? max.X : x;
            x = x < min.X ? min.X : x;
            float y = value.Y;
            y = y > max.Y ? max.Y : y;
            y = y < min.Y ? min.Y : y;
            float z = value.Z;
            z = z > max.Z ? max.Z : z;
            z = z < min.Z ? min.Z : z;
            result = new Float3(x, y, z);
        }

        /// <summary>
        /// Restricts a value to be within a specified range.
        /// </summary>
        /// <param name="value">The value to clamp.</param>
        /// <param name="min">The minimum value.</param>
        /// <param name="max">The maximum value.</param>
        /// <returns>The clamped value.</returns>
        public static Float3 Clamp(Float3 value, Float3 min, Float3 max)
        {
            Clamp(ref value, ref min, ref max, out var result);
            return result;
        }

        /// <summary>
        /// Calculates the cross product of two vectors.
        /// </summary>
        /// <param name="left">First source vector.</param>
        /// <param name="right">Second source vector.</param>
        /// <param name="result">When the method completes, contains he cross product of the two vectors.</param>
        public static void Cross(ref Float3 left, ref Float3 right, out Float3 result)
        {
            result = new Float3(left.Y * right.Z - left.Z * right.Y,
                left.Z * right.X - left.X * right.Z,
                left.X * right.Y - left.Y * right.X);
        }

        /// <summary>
        /// Calculates the cross product of two vectors.
        /// </summary>
        /// <param name="left">First source vector.</param>
        /// <param name="right">Second source vector.</param>
        /// <returns>The cross product of the two vectors.</returns>
        public static Float3 Cross(Float3 left, Float3 right)
        {
            Cross(ref left, ref right, out var result);
            return result;
        }

        /// <summary>
        /// Calculates the distance between two vectors.
        /// </summary>
        /// <param name="value1">The first vector.</param>
        /// <param name="value2">The second vector.</param>
        /// <param name="result">When the method completes, contains the distance between the two vectors.</param>
        /// <remarks><see cref="Float3.DistanceSquared(ref Float3, ref Float3, out float)" /> may be preferred when only the relative distance is needed and speed is of the essence.</remarks>
        public static void Distance(ref Float3 value1, ref Float3 value2, out float result)
        {
            float x = value1.X - value2.X;
            float y = value1.Y - value2.Y;
            float z = value1.Z - value2.Z;
            result = (float)Math.Sqrt(x * x + y * y + z * z);
        }

        /// <summary>
        /// Calculates the distance between two vectors.
        /// </summary>
        /// <param name="value1">The first vector.</param>
        /// <param name="value2">The second vector.</param>
        /// <returns>The distance between the two vectors.</returns>
        /// <remarks><see cref="Float3.DistanceSquared(ref Float3, ref Float3, out float)" /> may be preferred when only the relative distance is needed and speed is of the essence.</remarks>
        public static float Distance(ref Float3 value1, ref Float3 value2)
        {
            float x = value1.X - value2.X;
            float y = value1.Y - value2.Y;
            float z = value1.Z - value2.Z;
            return (float)Math.Sqrt(x * x + y * y + z * z);
        }

        /// <summary>
        /// Calculates the distance between two vectors.
        /// </summary>
        /// <param name="value1">The first vector.</param>
        /// <param name="value2">The second vector.</param>
        /// <returns>The distance between the two vectors.</returns>
        /// <remarks><see cref="Float3.DistanceSquared(Float3, Float3)" /> may be preferred when only the relative distance is needed and speed is of the essence.</remarks>
        public static float Distance(Float3 value1, Float3 value2)
        {
            float x = value1.X - value2.X;
            float y = value1.Y - value2.Y;
            float z = value1.Z - value2.Z;
            return (float)Math.Sqrt(x * x + y * y + z * z);
        }

        /// <summary>
        /// Calculates the squared distance between two vectors.
        /// </summary>
        /// <param name="value1">The first vector.</param>
        /// <param name="value2">The second vector.</param>
        /// <param name="result">When the method completes, contains the squared distance between the two vectors.</param>
        public static void DistanceSquared(ref Float3 value1, ref Float3 value2, out float result)
        {
            float x = value1.X - value2.X;
            float y = value1.Y - value2.Y;
            float z = value1.Z - value2.Z;
            result = x * x + y * y + z * z;
        }

        /// <summary>
        /// Calculates the squared distance between two vectors.
        /// </summary>
        /// <param name="value1">The first vector.</param>
        /// <param name="value2">The second vector.</param>
        /// <returns>The squared distance between the two vectors.</returns>
        public static float DistanceSquared(ref Float3 value1, ref Float3 value2)
        {
            float x = value1.X - value2.X;
            float y = value1.Y - value2.Y;
            float z = value1.Z - value2.Z;
            return x * x + y * y + z * z;
        }

        /// <summary>
        /// Calculates the squared distance between two vectors.
        /// </summary>
        /// <param name="value1">The first vector.</param>
        /// <param name="value2">The second vector.</param>
        /// <returns>The squared distance between the two vectors.</returns>
        public static float DistanceSquared(Float3 value1, Float3 value2)
        {
            float x = value1.X - value2.X;
            float y = value1.Y - value2.Y;
            float z = value1.Z - value2.Z;
            return x * x + y * y + z * z;
        }

        /// <summary>
        /// Calculates the distance between two vectors on the XY plane (ignoring Z).
        /// </summary>
        /// <param name="value1">The first vector.</param>
        /// <param name="value2">The second vector.</param>
        /// <param name="result">When the method completes, contains the distance between the two vectors in the XY plane.</param>
        public static void DistanceXY(ref Float3 value1, ref Float3 value2, out float result)
        {
            float x = value1.X - value2.X;
            float y = value1.Y - value2.Y;
            result = (float)Math.Sqrt(x * x + y * y);
        }

        /// <summary>
        /// Calculates the squared distance between two vectors on the XY plane (ignoring Z).
        /// </summary>
        /// <param name="value1">The first vector.</param>
        /// <param name="value2">The second vector</param>
        /// <param name="result">When the method completes, contains the squared distance between the two vectors in the XY plane.</param>
        public static void DistanceXYSquared(ref Float3 value1, ref Float3 value2, out float result)
        {
            float x = value1.X - value2.X;
            float y = value1.Y - value2.Y;
            result = x * x + y * y;
        }

        /// <summary>
        /// Calculates the distance between two vectors on the XZ plane (ignoring Y).
        /// </summary>
        /// <param name="value1">The first vector.</param>
        /// <param name="value2">The second vector.</param>
        /// <param name="result">When the method completes, contains the distance between the two vectors in the XY plane.</param>
        public static void DistanceXZ(ref Float3 value1, ref Float3 value2, out float result)
        {
            float x = value1.X - value2.X;
            float z = value1.Z - value2.Z;
            result = (float)Math.Sqrt(x * x + z * z);
        }

        /// <summary>
        /// Calculates the squared distance between two vectors on the XZ plane (ignoring Y).
        /// </summary>
        /// <param name="value1">The first vector.</param>
        /// <param name="value2">The second vector</param>
        /// <param name="result">When the method completes, contains the squared distance between the two vectors in the XY plane.</param>
        public static void DistanceXZSquared(ref Float3 value1, ref Float3 value2, out float result)
        {
            float x = value1.X - value2.X;
            float z = value1.Z - value2.Z;
            result = x * x + z * z;
        }

        /// <summary>
        /// Calculates the distance between two vectors on the YZ plane (ignoring X).
        /// </summary>
        /// <param name="value1">The first vector.</param>
        /// <param name="value2">The second vector.</param>
        /// <param name="result">When the method completes, contains the distance between the two vectors in the YZ plane.</param>
        public static void DistanceYZ(ref Float3 value1, ref Float3 value2, out float result)
        {
            float y = value1.Y - value2.Y;
            float z = value1.Z - value2.Z;
            result = (float)Math.Sqrt(y * y + z * z);
        }

        /// <summary>
        /// Calculates the squared distance between two vectors on the YZ plane (ignoring X).
        /// </summary>
        /// <param name="value1">The first vector.</param>
        /// <param name="value2">The second vector</param>
        /// <param name="result">When the method completes, contains the squared distance between the two vectors in the YZ plane.</param>
        public static void DistanceYZSquared(ref Float3 value1, ref Float3 value2, out float result)
        {
            float y = value1.Y - value2.Y;
            float z = value1.Z - value2.Z;
            result = y * y + z * z;
        }

        /// <summary>
        /// Tests whether one vector is near another vector.
        /// </summary>
        /// <param name="left">The left vector.</param>
        /// <param name="right">The right vector.</param>
        /// <param name="epsilon">The epsilon.</param>
        /// <returns><c>true</c> if left and right are near another, <c>false</c> otherwise</returns>
        public static bool NearEqual(Float3 left, Float3 right, float epsilon = Mathf.Epsilon)
        {
            return NearEqual(ref left, ref right, epsilon);
        }

        /// <summary>
        /// Tests whether one vector is near another vector.
        /// </summary>
        /// <param name="left">The left vector.</param>
        /// <param name="right">The right vector.</param>
        /// <param name="epsilon">The epsilon.</param>
        /// <returns><c>true</c> if left and right are near another, <c>false</c> otherwise</returns>
        public static bool NearEqual(ref Float3 left, ref Float3 right, float epsilon = Mathf.Epsilon)
        {
            return Mathf.WithinEpsilon(left.X, right.X, epsilon) && Mathf.WithinEpsilon(left.Y, right.Y, epsilon) &&
                   Mathf.WithinEpsilon(left.Z, right.Z, epsilon);
        }

        /// <summary>
        /// Calculates the dot product of two vectors.
        /// </summary>
        /// <param name="left">First source vector.</param>
        /// <param name="right">Second source vector.</param>
        /// <param name="result">When the method completes, contains the dot product of the two vectors.</param>
        public static void Dot(ref Float3 left, ref Float3 right, out float result)
        {
            result = left.X * right.X + left.Y * right.Y + left.Z * right.Z;
        }

        /// <summary>
        /// Calculates the dot product of two vectors.
        /// </summary>
        /// <param name="left">First source vector.</param>
        /// <param name="right">Second source vector.</param>
        /// <returns>The dot product of the two vectors.</returns>
        public static float Dot(ref Float3 left, ref Float3 right)
        {
            return left.X * right.X + left.Y * right.Y + left.Z * right.Z;
        }

        /// <summary>
        /// Calculates the dot product of two vectors.
        /// </summary>
        /// <param name="left">First source vector.</param>
        /// <param name="right">Second source vector.</param>
        /// <returns>The dot product of the two vectors.</returns>
        public static float Dot(Float3 left, Float3 right)
        {
            return left.X * right.X + left.Y * right.Y + left.Z * right.Z;
        }

        /// <summary>
        /// Converts the vector into a unit vector.
        /// </summary>
        /// <param name="value">The vector to normalize.</param>
        /// <param name="result">When the method completes, contains the normalized vector.</param>
        public static void Normalize(ref Float3 value, out Float3 result)
        {
            result = value;
            result.Normalize();
        }

        /// <summary>
        /// Converts the vector into a unit vector.
        /// </summary>
        /// <param name="value">The vector to normalize.</param>
        /// <returns>The normalized vector.</returns>
        public static Float3 Normalize(Float3 value)
        {
            value.Normalize();
            return value;
        }

        /// <summary>
        /// Makes sure that Length of the output vector is always below max and above 0.
        /// </summary>
        /// <param name="vector">Input vector.</param>
        /// <param name="max">Max Length</param>
        public static Float3 ClampLength(Float3 vector, float max)
        {
            return ClampLength(vector, 0, max);
        }

        /// <summary>
        /// Makes sure that Length of the output vector is always below max and above min.
        /// </summary>
        /// <param name="vector">Input vector.</param>
        /// <param name="min">Min Length</param>
        /// <param name="max">Max Length</param>
        public static Float3 ClampLength(Float3 vector, float min, float max)
        {
            ClampLength(vector, min, max, out Float3 result);
            return result;
        }

        /// <summary>
        /// Makes sure that Length of the output vector is always below max and above min.
        /// </summary>
        /// <param name="vector">Input vector.</param>
        /// <param name="min">Min Length</param>
        /// <param name="max">Max Length</param>
        /// <param name="result">The result vector.</param>
        public static void ClampLength(Float3 vector, float min, float max, out Float3 result)
        {
            result = default;
            result.X = vector.X;
            result.Y = vector.Y;
            result.Z = vector.Z;
            float lenSq = result.LengthSquared;
            if (lenSq > max * max)
            {
                float scaleFactor = max / (float)Math.Sqrt(lenSq);
                result.X *= scaleFactor;
                result.Y *= scaleFactor;
                result.Z *= scaleFactor;
            }

            if (lenSq < min * min)
            {
                float scaleFactor = min / (float)Math.Sqrt(lenSq);
                result.X *= scaleFactor;
                result.Y *= scaleFactor;
                result.Z *= scaleFactor;
            }
        }

        /// <summary>
        /// Performs a linear interpolation between two vectors.
        /// </summary>
        /// <param name="start">Start vector.</param>
        /// <param name="end">End vector.</param>
        /// <param name="amount">Value between 0 and 1 indicating the weight of <paramref name="end" />.</param>
        /// <param name="result">When the method completes, contains the linear interpolation of the two vectors.</param>
        /// <remarks>Passing <paramref name="amount" /> a value of 0 will cause <paramref name="start" /> to be returned; a value of 1 will cause <paramref name="end" /> to be returned.</remarks>
        public static void Lerp(ref Float3 start, ref Float3 end, float amount, out Float3 result)
        {
            result = default;
            result.X = Mathf.Lerp(start.X, end.X, amount);
            result.Y = Mathf.Lerp(start.Y, end.Y, amount);
            result.Z = Mathf.Lerp(start.Z, end.Z, amount);
        }

        /// <summary>
        /// Performs a linear interpolation between two vectors.
        /// </summary>
        /// <param name="start">Start vector.</param>
        /// <param name="end">End vector.</param>
        /// <param name="amount">Value between 0 and 1 indicating the weight of <paramref name="end" />.</param>
        /// <returns>The linear interpolation of the two vectors.</returns>
        /// <remarks>Passing <paramref name="amount" /> a value of 0 will cause <paramref name="start" /> to be returned; a value of 1 will cause <paramref name="end" /> to be returned.</remarks>
        public static Float3 Lerp(Float3 start, Float3 end, float amount)
        {
            Lerp(ref start, ref end, amount, out var result);
            return result;
        }

        /// <summary>
        /// Performs a cubic interpolation between two vectors.
        /// </summary>
        /// <param name="start">Start vector.</param>
        /// <param name="end">End vector.</param>
        /// <param name="amount">Value between 0 and 1 indicating the weight of <paramref name="end" />.</param>
        /// <param name="result">When the method completes, contains the cubic interpolation of the two vectors.</param>
        public static void SmoothStep(ref Float3 start, ref Float3 end, float amount, out Float3 result)
        {
            amount = Mathf.SmoothStep(amount);
            Lerp(ref start, ref end, amount, out result);
        }

        /// <summary>
        /// Performs a cubic interpolation between two vectors.
        /// </summary>
        /// <param name="start">Start vector.</param>
        /// <param name="end">End vector.</param>
        /// <param name="amount">Value between 0 and 1 indicating the weight of <paramref name="end" />.</param>
        /// <returns>The cubic interpolation of the two vectors.</returns>
        public static Float3 SmoothStep(Float3 start, Float3 end, float amount)
        {
            SmoothStep(ref start, ref end, amount, out var result);
            return result;
        }

        /// <summary>
        /// Moves a value current towards target.
        /// </summary>
        /// <param name="current">The position to move from.</param>
        /// <param name="target">The position to move towards.</param>
        /// <param name="maxDistanceDelta">The maximum distance that can be applied to the value.</param>
        /// <returns>The new position.</returns>
        public static Float3 MoveTowards(Float3 current, Float3 target, float maxDistanceDelta)
        {
            var to = target - current;
            var distanceSq = to.LengthSquared;
            if (distanceSq == 0 || (maxDistanceDelta >= 0 && distanceSq <= maxDistanceDelta * maxDistanceDelta))
                return target;
            var scale = maxDistanceDelta / Mathf.Sqrt(distanceSq);
            return new Float3(current.X + to.X * scale, current.Y + to.Y * scale, current.Z + to.Z * scale);
        }

        /// <summary>
        /// Performs a Hermite spline interpolation.
        /// </summary>
        /// <param name="value1">First source position vector.</param>
        /// <param name="tangent1">First source tangent vector.</param>
        /// <param name="value2">Second source position vector.</param>
        /// <param name="tangent2">Second source tangent vector.</param>
        /// <param name="amount">Weighting factor.</param>
        /// <param name="result">When the method completes, contains the result of the Hermite spline interpolation.</param>
        public static void Hermite(ref Float3 value1, ref Float3 tangent1, ref Float3 value2, ref Float3 tangent2,
            float amount, out Float3 result)
        {
            result = default;
            float squared = amount * amount;
            float cubed = amount * squared;
            float part1 = 2.0f * cubed - 3.0f * squared + 1.0f;
            float part2 = -2.0f * cubed + 3.0f * squared;
            float part3 = cubed - 2.0f * squared + amount;
            float part4 = cubed - squared;
            result.X = value1.X * part1 + value2.X * part2 + tangent1.X * part3 + tangent2.X * part4;
            result.Y = value1.Y * part1 + value2.Y * part2 + tangent1.Y * part3 + tangent2.Y * part4;
            result.Z = value1.Z * part1 + value2.Z * part2 + tangent1.Z * part3 + tangent2.Z * part4;
        }

        /// <summary>
        /// Performs a Hermite spline interpolation.
        /// </summary>
        /// <param name="value1">First source position vector.</param>
        /// <param name="tangent1">First source tangent vector.</param>
        /// <param name="value2">Second source position vector.</param>
        /// <param name="tangent2">Second source tangent vector.</param>
        /// <param name="amount">Weighting factor.</param>
        /// <returns>The result of the Hermite spline interpolation.</returns>
        public static Float3 Hermite(Float3 value1, Float3 tangent1, Float3 value2, Float3 tangent2, float amount)
        {
            Hermite(ref value1, ref tangent1, ref value2, ref tangent2, amount, out var result);
            return result;
        }

        /// <summary>
        /// Performs a Catmull-Rom interpolation using the specified positions.
        /// </summary>
        /// <param name="value1">The first position in the interpolation.</param>
        /// <param name="value2">The second position in the interpolation.</param>
        /// <param name="value3">The third position in the interpolation.</param>
        /// <param name="value4">The fourth position in the interpolation.</param>
        /// <param name="amount">Weighting factor.</param>
        /// <param name="result">When the method completes, contains the result of the Catmull-Rom interpolation.</param>
        public static void CatmullRom(ref Float3 value1, ref Float3 value2, ref Float3 value3, ref Float3 value4,
            float amount, out Float3 result)
        {
            result = default;
            float squared = amount * amount;
            float cubed = amount * squared;
            result.X = 0.5f * (2.0f * value2.X + (-value1.X + value3.X) * amount +
                               (2.0f * value1.X - 5.0f * value2.X + 4.0f * value3.X - value4.X) * squared +
                               (-value1.X + 3.0f * value2.X - 3.0f * value3.X + value4.X) * cubed);
            result.Y = 0.5f * (2.0f * value2.Y + (-value1.Y + value3.Y) * amount +
                               (2.0f * value1.Y - 5.0f * value2.Y + 4.0f * value3.Y - value4.Y) * squared +
                               (-value1.Y + 3.0f * value2.Y - 3.0f * value3.Y + value4.Y) * cubed);
            result.Z = 0.5f * (2.0f * value2.Z + (-value1.Z + value3.Z) * amount +
                               (2.0f * value1.Z - 5.0f * value2.Z + 4.0f * value3.Z - value4.Z) * squared +
                               (-value1.Z + 3.0f * value2.Z - 3.0f * value3.Z + value4.Z) * cubed);
        }

        /// <summary>
        /// Performs a Catmull-Rom interpolation using the specified positions.
        /// </summary>
        /// <param name="value1">The first position in the interpolation.</param>
        /// <param name="value2">The second position in the interpolation.</param>
        /// <param name="value3">The third position in the interpolation.</param>
        /// <param name="value4">The fourth position in the interpolation.</param>
        /// <param name="amount">Weighting factor.</param>
        /// <returns>A vector that is the result of the Catmull-Rom interpolation.</returns>
        public static Float3 CatmullRom(Float3 value1, Float3 value2, Float3 value3, Float3 value4, float amount)
        {
            CatmullRom(ref value1, ref value2, ref value3, ref value4, amount, out var result);
            return result;
        }

        /// <summary>
        /// Returns a vector containing the largest components of the specified vectors.
        /// </summary>
        /// <param name="left">The first source vector.</param>
        /// <param name="right">The second source vector.</param>
        /// <param name="result">When the method completes, contains an new vector composed of the largest components of the source vectors.</param>
        public static void Max(ref Float3 left, ref Float3 right, out Float3 result)
        {
            result = default;
            result.X = left.X > right.X ? left.X : right.X;
            result.Y = left.Y > right.Y ? left.Y : right.Y;
            result.Z = left.Z > right.Z ? left.Z : right.Z;
        }

        /// <summary>
        /// Returns a vector containing the largest components of the specified vectors.
        /// </summary>
        /// <param name="left">The first source vector.</param>
        /// <param name="right">The second source vector.</param>
        /// <returns>A vector containing the largest components of the source vectors.</returns>
        public static Float3 Max(Float3 left, Float3 right)
        {
            Max(ref left, ref right, out var result);
            return result;
        }

        /// <summary>
        /// Returns a vector containing the smallest components of the specified vectors.
        /// </summary>
        /// <param name="left">The first source vector.</param>
        /// <param name="right">The second source vector.</param>
        /// <param name="result">When the method completes, contains an new vector composed of the smallest components of the source vectors.</param>
        public static void Min(ref Float3 left, ref Float3 right, out Float3 result)
        {
            result = default;
            result.X = left.X < right.X ? left.X : right.X;
            result.Y = left.Y < right.Y ? left.Y : right.Y;
            result.Z = left.Z < right.Z ? left.Z : right.Z;
        }

        /// <summary>
        /// Returns a vector containing the smallest components of the specified vectors.
        /// </summary>
        /// <param name="left">The first source vector.</param>
        /// <param name="right">The second source vector.</param>
        /// <returns>A vector containing the smallest components of the source vectors.</returns>
        public static Float3 Min(Float3 left, Float3 right)
        {
            Min(ref left, ref right, out var result);
            return result;
        }

        /// <summary>
        /// Returns the absolute value of a vector.
        /// </summary>
        /// <param name="v">The value.</param>
        /// <returns> A vector which components are less or equal to 0.</returns>
        public static Float3 Abs(Float3 v)
        {
            return new Float3(Math.Abs(v.X), Math.Abs(v.Y), Math.Abs(v.Z));
        }

        /// <summary>
        /// Projects a vector onto another vector.
        /// </summary>
        /// <param name="vector">The vector to project.</param>
        /// <param name="onNormal">The projection normal vector.</param>
        /// <returns>The projected vector.</returns>
        public static Float3 Project(Float3 vector, Float3 onNormal)
        {
            float sqrMag = Dot(onNormal, onNormal);
            if (sqrMag < Mathf.Epsilon)
                return Zero;
            return onNormal * Dot(vector, onNormal) / sqrMag;
        }

        /// <summary>
        /// Projects a vector onto a plane defined by a normal orthogonal to the plane.
        /// </summary>
        /// <param name="vector">The vector to project.</param>
        /// <param name="planeNormal">The plane normal vector.</param>
        /// <returns>The projected vector.</returns>
        public static Float3 ProjectOnPlane(Float3 vector, Float3 planeNormal)
        {
            return vector - Project(vector, planeNormal);
        }

        /// <summary>
        /// Calculates the angle (in degrees) between <paramref name="from"/> and <paramref name="to"/>. This is always the smallest value.
        /// </summary>
        /// <param name="from">The first vector.</param>
        /// <param name="to">The second vector.</param>
        /// <returns>The angle (in degrees).</returns>
        public static float Angle(Float3 from, Float3 to)
        {
            float dot = Mathf.Clamp(Dot(from.Normalized, to.Normalized), -1.0f, 1.0f);
            if (Mathf.Abs(dot) > (1.0f - Mathf.Epsilon))
                return dot > 0.0f ? 0.0f : 180.0f;
            return (float)Math.Acos(dot) * Mathf.RadiansToDegrees;
        }

        /// <summary>
        /// Projects a 3D vector from object space into screen space.
        /// </summary>
        /// <param name="vector">The vector to project.</param>
        /// <param name="x">The X position of the viewport.</param>
        /// <param name="y">The Y position of the viewport.</param>
        /// <param name="width">The width of the viewport.</param>
        /// <param name="height">The height of the viewport.</param>
        /// <param name="minZ">The minimum depth of the viewport.</param>
        /// <param name="maxZ">The maximum depth of the viewport.</param>
        /// <param name="worldViewProjection">The combined world-view-projection matrix.</param>
        /// <param name="result">When the method completes, contains the vector in screen space.</param>
        public static void Project(ref Float3 vector, float x, float y, float width, float height, float minZ,
            float maxZ, ref Matrix worldViewProjection, out Float3 result)
        {
            TransformCoordinate(ref vector, ref worldViewProjection, out var v);
            result = new Float3((1.0f + v.X) * 0.5f * width + x, (1.0f - v.Y) * 0.5f * height + y,
                v.Z * (maxZ - minZ) + minZ);
        }

        /// <summary>
        /// Projects a 3D vector from object space into screen space.
        /// </summary>
        /// <param name="vector">The vector to project.</param>
        /// <param name="x">The X position of the viewport.</param>
        /// <param name="y">The Y position of the viewport.</param>
        /// <param name="width">The width of the viewport.</param>
        /// <param name="height">The height of the viewport.</param>
        /// <param name="minZ">The minimum depth of the viewport.</param>
        /// <param name="maxZ">The maximum depth of the viewport.</param>
        /// <param name="worldViewProjection">The combined world-view-projection matrix.</param>
        /// <returns>The vector in screen space.</returns>
        public static Float3 Project(Float3 vector, float x, float y, float width, float height, float minZ, float maxZ,
            Matrix worldViewProjection)
        {
            Project(ref vector, x, y, width, height, minZ, maxZ, ref worldViewProjection, out var result);
            return result;
        }

        /// <summary>
        /// Projects a 3D vector from screen space into object space.
        /// </summary>
        /// <param name="vector">The vector to project.</param>
        /// <param name="x">The X position of the viewport.</param>
        /// <param name="y">The Y position of the viewport.</param>
        /// <param name="width">The width of the viewport.</param>
        /// <param name="height">The height of the viewport.</param>
        /// <param name="minZ">The minimum depth of the viewport.</param>
        /// <param name="maxZ">The maximum depth of the viewport.</param>
        /// <param name="worldViewProjection">The combined world-view-projection matrix.</param>
        /// <param name="result">When the method completes, contains the vector in object space.</param>
        public static void Unproject(ref Float3 vector, float x, float y, float width, float height, float minZ,
            float maxZ, ref Matrix worldViewProjection, out Float3 result)
        {
            Matrix.Invert(ref worldViewProjection, out var matrix);
            var v = new Float3
            {
                X = (vector.X - x) / width * 2.0f - 1.0f,
                Y = -((vector.Y - y) / height * 2.0f - 1.0f),
                Z = (vector.Z - minZ) / (maxZ - minZ)
            };
            TransformCoordinate(ref v, ref matrix, out result);
        }

        /// <summary>
        /// Projects a 3D vector from screen space into object space.
        /// </summary>
        /// <param name="vector">The vector to project.</param>
        /// <param name="x">The X position of the viewport.</param>
        /// <param name="y">The Y position of the viewport.</param>
        /// <param name="width">The width of the viewport.</param>
        /// <param name="height">The height of the viewport.</param>
        /// <param name="minZ">The minimum depth of the viewport.</param>
        /// <param name="maxZ">The maximum depth of the viewport.</param>
        /// <param name="worldViewProjection">The combined world-view-projection matrix.</param>
        /// <returns>The vector in object space.</returns>
        public static Float3 Unproject(Float3 vector, float x, float y, float width, float height, float minZ,
            float maxZ, Matrix worldViewProjection)
        {
            Unproject(ref vector, x, y, width, height, minZ, maxZ, ref worldViewProjection, out var result);
            return result;
        }

        /// <summary>
        /// Returns the reflection of a vector off a surface that has the specified normal.
        /// </summary>
        /// <param name="vector">The source vector.</param>
        /// <param name="normal">Normal of the surface.</param>
        /// <param name="result">When the method completes, contains the reflected vector.</param>
        /// <remarks>Reflect only gives the direction of a reflection off a surface, it does not determine whether the original vector was close enough to the surface to hit it.</remarks>
        public static void Reflect(ref Float3 vector, ref Float3 normal, out Float3 result)
        {
            result = default;
            float dot = vector.X * normal.X + vector.Y * normal.Y + vector.Z * normal.Z;
            result.X = vector.X - 2.0f * dot * normal.X;
            result.Y = vector.Y - 2.0f * dot * normal.Y;
            result.Z = vector.Z - 2.0f * dot * normal.Z;
        }

        /// <summary>
        /// Returns the reflection of a vector off a surface that has the specified normal.
        /// </summary>
        /// <param name="vector">The source vector.</param>
        /// <param name="normal">Normal of the surface.</param>
        /// <returns>The reflected vector.</returns>
        /// <remarks>Reflect only gives the direction of a reflection off a surface, it does not determine whether the original vector was close enough to the surface to hit it.</remarks>
        public static Float3 Reflect(Float3 vector, Float3 normal)
        {
            Reflect(ref vector, ref normal, out var result);
            return result;
        }

        /// <summary>
        /// Transforms a 3D vector by the given <see cref="Quaternion" /> rotation.
        /// </summary>
        /// <param name="vector">The vector to rotate.</param>
        /// <param name="rotation">The <see cref="Quaternion" /> rotation to apply.</param>
        /// <param name="result">When the method completes, contains the transformed <see cref="Float3" />.</param>
        public static void Transform(ref Float3 vector, ref Quaternion rotation, out Float3 result)
        {
            float x = rotation.X + rotation.X;
            float y = rotation.Y + rotation.Y;
            float z = rotation.Z + rotation.Z;
            float wx = rotation.W * x;
            float wy = rotation.W * y;
            float wz = rotation.W * z;
            float xx = rotation.X * x;
            float xy = rotation.X * y;
            float xz = rotation.X * z;
            float yy = rotation.Y * y;
            float yz = rotation.Y * z;
            float zz = rotation.Z * z;
            result = new Float3(vector.X * (1.0f - yy - zz) + vector.Y * (xy - wz) + vector.Z * (xz + wy),
                vector.X * (xy + wz) + vector.Y * (1.0f - xx - zz) + vector.Z * (yz - wx),
                vector.X * (xz - wy) + vector.Y * (yz + wx) + vector.Z * (1.0f - xx - yy));
        }

        /// <summary>
        /// Transforms a 3D vector by the given <see cref="Quaternion" /> rotation.
        /// </summary>
        /// <param name="vector">The vector to rotate.</param>
        /// <param name="rotation">The <see cref="Quaternion" /> rotation to apply.</param>
        /// <returns>The transformed <see cref="Float3" />.</returns>
        public static Float3 Transform(Float3 vector, Quaternion rotation)
        {
            Transform(ref vector, ref rotation, out var result);
            return result;
        }

        /// <summary>
        /// Transforms a 3D vector by the given <see cref="Matrix3x3"/>.
        /// </summary>
        /// <param name="vector">The source vector.</param>
        /// <param name="transform">The transformation <see cref="Matrix3x3"/>.</param>
        /// <param name="result">When the method completes, contains the transformed <see cref="Float3"/>.</param>
        public static void Transform(ref Float3 vector, ref Matrix3x3 transform, out Float3 result)
        {
            result = new Float3((vector.X * transform.M11) + (vector.Y * transform.M21) + (vector.Z * transform.M31),
                (vector.X * transform.M12) + (vector.Y * transform.M22) + (vector.Z * transform.M32),
                (vector.X * transform.M13) + (vector.Y * transform.M23) + (vector.Z * transform.M33));
        }

        /// <summary>
        /// Transforms a 3D vector by the given <see cref="Matrix3x3"/>.
        /// </summary>
        /// <param name="vector">The source vector.</param>
        /// <param name="transform">The transformation <see cref="Matrix3x3"/>.</param>
        /// <returns>The transformed <see cref="Float3"/>.</returns>
        public static Float3 Transform(Float3 vector, Matrix3x3 transform)
        {
            Transform(ref vector, ref transform, out var result);
            return result;
        }

        /// <summary>
        /// Transforms a 3D vector by the given <see cref="Matrix" />.
        /// </summary>
        /// <param name="vector">The source vector.</param>
        /// <param name="transform">The transformation <see cref="Matrix" />.</param>
        /// <param name="result">When the method completes, contains the transformed <see cref="Float3" />.</param>
        public static void Transform(ref Float3 vector, ref Matrix transform, out Float3 result)
        {
            result = new Float3(
                vector.X * transform.M11 + vector.Y * transform.M21 + vector.Z * transform.M31 + transform.M41,
                vector.X * transform.M12 + vector.Y * transform.M22 + vector.Z * transform.M32 + transform.M42,
                vector.X * transform.M13 + vector.Y * transform.M23 + vector.Z * transform.M33 + transform.M43);
        }

        /// <summary>
        /// Transforms a 3D vector by the given <see cref="Matrix" />.
        /// </summary>
        /// <param name="vector">The source vector.</param>
        /// <param name="transform">The transformation <see cref="Matrix" />.</param>
        /// <param name="result">When the method completes, contains the transformed <see cref="Float4" />.</param>
        public static void Transform(ref Float3 vector, ref Matrix transform, out Float4 result)
        {
            result = new Float4(
                vector.X * transform.M11 + vector.Y * transform.M21 + vector.Z * transform.M31 + transform.M41,
                vector.X * transform.M12 + vector.Y * transform.M22 + vector.Z * transform.M32 + transform.M42,
                vector.X * transform.M13 + vector.Y * transform.M23 + vector.Z * transform.M33 + transform.M43,
                vector.X * transform.M14 + vector.Y * transform.M24 + vector.Z * transform.M34 + transform.M44);
        }

        /// <summary>
        /// Transforms a 3D vector by the given <see cref="Matrix" />.
        /// </summary>
        /// <param name="vector">The source vector.</param>
        /// <param name="transform">The transformation <see cref="Matrix" />.</param>
        /// <returns>The transformed <see cref="Float3" />.</returns>
        public static Float3 Transform(Float3 vector, Matrix transform)
        {
            Transform(ref vector, ref transform, out Float3 result);
            return result;
        }

        /// <summary>
        /// Performs a coordinate transformation using the given <see cref="Matrix" />.
        /// </summary>
        /// <param name="coordinate">The coordinate vector to transform.</param>
        /// <param name="transform">The transformation <see cref="Matrix" />.</param>
        /// <param name="result">When the method completes, contains the transformed coordinates.</param>
        /// <remarks>
        /// A coordinate transform performs the transformation with the assumption that the w component
        /// is one. The four dimensional vector obtained from the transformation operation has each
        /// component in the vector divided by the w component. This forces the w component to be one and
        /// therefore makes the vector homogeneous. The homogeneous vector is often preferred when working
        /// with coordinates as the w component can safely be ignored.
        /// </remarks>
        public static void TransformCoordinate(ref Float3 coordinate, ref Matrix transform, out Float3 result)
        {
            var vector = new Float4
            {
                X = coordinate.X * transform.M11 + coordinate.Y * transform.M21 + coordinate.Z * transform.M31 +
                    transform.M41,
                Y = coordinate.X * transform.M12 + coordinate.Y * transform.M22 + coordinate.Z * transform.M32 +
                    transform.M42,
                Z = coordinate.X * transform.M13 + coordinate.Y * transform.M23 + coordinate.Z * transform.M33 +
                    transform.M43,
                W = 1f / (coordinate.X * transform.M14 + coordinate.Y * transform.M24 + coordinate.Z * transform.M34 +
                          transform.M44)
            };
            result = new Float3(vector.X * vector.W, vector.Y * vector.W, vector.Z * vector.W);
        }

        /// <summary>
        /// Performs a coordinate transformation using the given <see cref="Matrix" />.
        /// </summary>
        /// <param name="coordinate">The coordinate vector to transform.</param>
        /// <param name="transform">The transformation <see cref="Matrix" />.</param>
        /// <returns>The transformed coordinates.</returns>
        /// <remarks>
        /// A coordinate transform performs the transformation with the assumption that the w component
        /// is one. The four dimensional vector obtained from the transformation operation has each
        /// component in the vector divided by the w component. This forces the w component to be one and
        /// therefore makes the vector homogeneous. The homogeneous vector is often preferred when working
        /// with coordinates as the w component can safely be ignored.
        /// </remarks>
        public static Float3 TransformCoordinate(Float3 coordinate, Matrix transform)
        {
            TransformCoordinate(ref coordinate, ref transform, out var result);
            return result;
        }

        /// <summary>
        /// Performs a normal transformation using the given <see cref="Matrix" />.
        /// </summary>
        /// <param name="normal">The normal vector to transform.</param>
        /// <param name="transform">The transformation <see cref="Matrix" />.</param>
        /// <param name="result">When the method completes, contains the transformed normal.</param>
        /// <remarks>
        /// A normal transform performs the transformation with the assumption that the w component
        /// is zero. This causes the fourth row and fourth column of the matrix to be unused. The
        /// end result is a vector that is not translated, but all other transformation properties
        /// apply. This is often preferred for normal vectors as normals purely represent direction
        /// rather than location because normal vectors should not be translated.
        /// </remarks>
        public static void TransformNormal(ref Float3 normal, ref Matrix transform, out Float3 result)
        {
            result = new Float3(normal.X * transform.M11 + normal.Y * transform.M21 + normal.Z * transform.M31,
                normal.X * transform.M12 + normal.Y * transform.M22 + normal.Z * transform.M32,
                normal.X * transform.M13 + normal.Y * transform.M23 + normal.Z * transform.M33);
        }

        /// <summary>
        /// Performs a normal transformation using the given <see cref="Matrix" />.
        /// </summary>
        /// <param name="normal">The normal vector to transform.</param>
        /// <param name="transform">The transformation <see cref="Matrix" />.</param>
        /// <returns>The transformed normal.</returns>
        /// <remarks>
        /// A normal transform performs the transformation with the assumption that the w component
        /// is zero. This causes the fourth row and fourth column of the matrix to be unused. The
        /// end result is a vector that is not translated, but all other transformation properties
        /// apply. This is often preferred for normal vectors as normals purely represent direction
        /// rather than location because normal vectors should not be translated.
        /// </remarks>
        public static Float3 TransformNormal(Float3 normal, Matrix transform)
        {
            TransformNormal(ref normal, ref transform, out var result);
            return result;
        }

        /// <summary>
        /// Snaps the input position into the grid.
        /// </summary>
        /// <param name="pos">The position to snap.</param>
        /// <param name="gridSize">The size of the grid.</param>
        /// <returns>The position snapped to the grid.</returns>
        public static Float3 SnapToGrid(Float3 pos, Float3 gridSize)
        {
            pos.X = Mathf.Ceil((pos.X - (gridSize.X * 0.5f)) / gridSize.X) * gridSize.X;
            pos.Y = Mathf.Ceil((pos.Y - (gridSize.Y * 0.5f)) / gridSize.Y) * gridSize.Y;
            pos.Z = Mathf.Ceil((pos.Z - (gridSize.Z * 0.5f)) / gridSize.Z) * gridSize.Z;
            return pos;
        }

        /// <summary>
        /// Adds two vectors.
        /// </summary>
        /// <param name="left">The first vector to add.</param>
        /// <param name="right">The second vector to add.</param>
        /// <returns>The sum of the two vectors.</returns>
        public static Float3 operator +(Float3 left, Float3 right)
        {
            return new Float3(left.X + right.X, left.Y + right.Y, left.Z + right.Z);
        }

        /// <summary>
        /// Multiplies a vector with another by performing component-wise multiplication equivalent to <see cref="Multiply(ref Float3,ref Float3,out Float3)" />.
        /// </summary>
        /// <param name="left">The first vector to multiply.</param>
        /// <param name="right">The second vector to multiply.</param>
        /// <returns>The multiplication of the two vectors.</returns>
        public static Float3 operator *(Float3 left, Float3 right)
        {
            return new Float3(left.X * right.X, left.Y * right.Y, left.Z * right.Z);
        }

        /// <summary>
        /// Assert a vector (return it unchanged).
        /// </summary>
        /// <param name="value">The vector to assert (unchanged).</param>
        /// <returns>The asserted (unchanged) vector.</returns>
        public static Float3 operator +(Float3 value)
        {
            return value;
        }

        /// <summary>
        /// Subtracts two vectors.
        /// </summary>
        /// <param name="left">The first vector to subtract.</param>
        /// <param name="right">The second vector to subtract.</param>
        /// <returns>The difference of the two vectors.</returns>
        public static Float3 operator -(Float3 left, Float3 right)
        {
            return new Float3(left.X - right.X, left.Y - right.Y, left.Z - right.Z);
        }

        /// <summary>
        /// Reverses the direction of a given vector.
        /// </summary>
        /// <param name="value">The vector to negate.</param>
        /// <returns>A vector facing in the opposite direction.</returns>
        public static Float3 operator -(Float3 value)
        {
            return new Float3(-value.X, -value.Y, -value.Z);
        }

        /// <summary>
        /// Transforms a vector by the given rotation.
        /// </summary>
        /// <param name="vector">The vector to transform.</param>
        /// <param name="rotation">The quaternion.</param>
        /// <returns>The scaled vector.</returns>
        public static Float3 operator *(Float3 vector, Quaternion rotation)
        {
            Transform(ref vector, ref rotation, out var result);
            return result;
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The scaled vector.</returns>
        public static Float3 operator *(float scale, Float3 value)
        {
            return new Float3(value.X * scale, value.Y * scale, value.Z * scale);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The scaled vector.</returns>
        public static Float3 operator *(Float3 value, float scale)
        {
            return new Float3(value.X * scale, value.Y * scale, value.Z * scale);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The scaled vector.</returns>
        public static Float3 operator /(Float3 value, float scale)
        {
            return new Float3(value.X / scale, value.Y / scale, value.Z / scale);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <param name="value">The vector to scale.</param>
        /// <returns>The scaled vector.</returns>
        public static Float3 operator /(float scale, Float3 value)
        {
            return new Float3(scale / value.X, scale / value.Y, scale / value.Z);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The scaled vector.</returns>
        public static Float3 operator *(double scale, Float3 value)
        {
            var s = (float)scale;
            return new Float3(value.X * s, value.Y * s, value.Z * s);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The scaled vector.</returns>
        public static Float3 operator *(Float3 value, double scale)
        {
            var s = (float)scale;
            return new Float3(value.X * s, value.Y * s, value.Z * s);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The scaled vector.</returns>
        public static Float3 operator /(Float3 value, double scale)
        {
            var s = (float)scale;
            return new Float3(value.X / s, value.Y / s, value.Z / s);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <param name="value">The vector to scale.</param>
        /// <returns>The scaled vector.</returns>
        public static Float3 operator /(double scale, Float3 value)
        {
            var s = (float)scale;
            return new Float3(s / value.X, s / value.Y, s / value.Z);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The scaled vector.</returns>
        public static Float3 operator /(Float3 value, Float3 scale)
        {
            return new Float3(value.X / scale.X, value.Y / scale.Y, value.Z / scale.Z);
        }

        /// <summary>
        /// Remainder of value divided by scale.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The remained vector.</returns>
        public static Float3 operator %(Float3 value, float scale)
        {
            return new Float3(value.X % scale, value.Y % scale, value.Z % scale);
        }

        /// <summary>
        /// Remainder of value divided by scale.
        /// </summary>
        /// <param name="value">The amount by which to scale the vector.</param>
        /// <param name="scale">The vector to scale.</param>
        /// <returns>The remained vector.</returns>
        public static Float3 operator %(float value, Float3 scale)
        {
            return new Float3(value % scale.X, value % scale.Y, value % scale.Z);
        }

        /// <summary>
        /// Remainder of value divided by scale.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The remained vector.</returns>
        public static Float3 operator %(Float3 value, Float3 scale)
        {
            return new Float3(value.X % scale.X, value.Y % scale.Y, value.Z % scale.Z);
        }

        /// <summary>
        /// Performs a component-wise addition.
        /// </summary>
        /// <param name="value">The input vector.</param>
        /// <param name="scalar">The scalar value to be added on elements</param>
        /// <returns>The vector with added scalar for each element.</returns>
        public static Float3 operator +(Float3 value, float scalar)
        {
            return new Float3(value.X + scalar, value.Y + scalar, value.Z + scalar);
        }

        /// <summary>
        /// Performs a component-wise addition.
        /// </summary>
        /// <param name="value">The input vector.</param>
        /// <param name="scalar">The scalar value to be added on elements</param>
        /// <returns>The vector with added scalar for each element.</returns>
        public static Float3 operator +(float scalar, Float3 value)
        {
            return new Float3(scalar + value.X, scalar + value.Y, scalar + value.Z);
        }

        /// <summary>
        /// Performs a component-wise subtraction.
        /// </summary>
        /// <param name="value">The input vector.</param>
        /// <param name="scalar">The scalar value to be subtracted from elements</param>
        /// <returns>The vector with added scalar from each element.</returns>
        public static Float3 operator -(Float3 value, float scalar)
        {
            return new Float3(value.X - scalar, value.Y - scalar, value.Z - scalar);
        }

        /// <summary>
        /// Performs a component-wise subtraction.
        /// </summary>
        /// <param name="value">The input vector.</param>
        /// <param name="scalar">The scalar value to be subtracted from elements</param>
        /// <returns>The vector with subtracted scalar from each element.</returns>
        public static Float3 operator -(float scalar, Float3 value)
        {
            return new Float3(scalar - value.X, scalar - value.Y, scalar - value.Z);
        }

        /// <summary>
        /// Tests for equality between two objects.
        /// </summary>
        /// <param name="left">The first value to compare.</param>
        /// <param name="right">The second value to compare.</param>
        /// <returns><c>true</c> if <paramref name="left" /> has the same value as <paramref name="right" />; otherwise, <c>false</c>.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool operator ==(Float3 left, Float3 right)
        {
            return Mathf.NearEqual(left.X, right.X) && Mathf.NearEqual(left.Y, right.Y) &&
                   Mathf.NearEqual(left.Z, right.Z);
        }

        /// <summary>
        /// Tests for inequality between two objects.
        /// </summary>
        /// <param name="left">The first value to compare.</param>
        /// <param name="right">The second value to compare.</param>
        /// <returns><c>true</c> if <paramref name="left" /> has a different value than <paramref name="right" />; otherwise, <c>false</c>.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool operator !=(Float3 left, Float3 right)
        {
            return !Mathf.NearEqual(left.X, right.X) || !Mathf.NearEqual(left.Y, right.Y) ||
                   !Mathf.NearEqual(left.Z, right.Z);
        }

        /// <summary>
        /// Performs an explicit conversion from <see cref="Float3" /> to <see cref="Vector3" />.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <returns>The result of the conversion.</returns>
        public static implicit operator Vector3(Float3 value)
        {
            return new Vector3(value.X, value.Y, value.Z);
        }

        /// <summary>
        /// Performs an implicit conversion from <see cref="Float3" /> to <see cref="Double3" />.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <returns>The result of the conversion.</returns>
        public static implicit operator Double3(Float3 value)
        {
            return new Double3(value.X, value.Y, value.Z);
        }

        /// <summary>
        /// Performs an explicit conversion from <see cref="Float3" /> to <see cref="Float2" />.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <returns>The result of the conversion.</returns>
        public static explicit operator Float2(Float3 value)
        {
            return new Float2(value.X, value.Y);
        }

        /// <summary>
        /// Performs an explicit conversion from <see cref="Float3" /> to <see cref="Float4" />.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <returns>The result of the conversion.</returns>
        public static explicit operator Float4(Float3 value)
        {
            return new Float4(value, 0.0f);
        }

        /// <summary>
        /// Returns a <see cref="System.String" /> that represents this instance.
        /// </summary>
        /// <returns>A <see cref="System.String" /> that represents this instance.</returns>
        public override string ToString()
        {
            return string.Format(CultureInfo.CurrentCulture, _formatString, X, Y, Z);
        }

        /// <summary>
        /// Returns a <see cref="System.String" /> that represents this instance.
        /// </summary>
        /// <param name="format">The format.</param>
        /// <returns>A <see cref="System.String" /> that represents this instance.</returns>
        public string ToString(string format)
        {
            if (format == null)
                return ToString();
            return string.Format(CultureInfo.CurrentCulture, _formatString,
                X.ToString(format, CultureInfo.CurrentCulture), Y.ToString(format, CultureInfo.CurrentCulture),
                Z.ToString(format, CultureInfo.CurrentCulture));
        }

        /// <summary>
        /// Returns a <see cref="System.String" /> that represents this instance.
        /// </summary>
        /// <param name="formatProvider">The format provider.</param>
        /// <returns>A <see cref="System.String" /> that represents this instance.</returns>
        public string ToString(IFormatProvider formatProvider)
        {
            return string.Format(formatProvider, _formatString, X, Y, Z);
        }

        /// <summary>
        /// Returns a <see cref="System.String" /> that represents this instance.
        /// </summary>
        /// <param name="format">The format.</param>
        /// <param name="formatProvider">The format provider.</param>
        /// <returns>A <see cref="System.String" /> that represents this instance.</returns>
        public string ToString(string format, IFormatProvider formatProvider)
        {
            if (format == null)
                return ToString(formatProvider);
            return string.Format(formatProvider, "X:{0} Y:{1} Z:{2}", X.ToString(format, formatProvider),
                Y.ToString(format, formatProvider), Z.ToString(format, formatProvider));
        }

        /// <summary>
        /// Returns a hash code for this instance.
        /// </summary>
        public override int GetHashCode()
        {
            unchecked
            {
                int hashCode = X.GetHashCode();
                hashCode = (hashCode * 397) ^ Y.GetHashCode();
                hashCode = (hashCode * 397) ^ Z.GetHashCode();
                return hashCode;
            }
        }

        /// <summary>
        /// Determines whether the specified <see cref="Float3" /> is equal to this instance.
        /// </summary>
        /// <param name="other">The <see cref="Float3" /> to compare with this instance.</param>
        /// <returns><c>true</c> if the specified <see cref="Float3" /> is equal to this instance; otherwise, <c>false</c>.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public bool Equals(ref Float3 other)
        {
            return Mathf.NearEqual(other.X, X) && Mathf.NearEqual(other.Y, Y) && Mathf.NearEqual(other.Z, Z);
        }

        /// <summary>
        /// Determines whether the specified <see cref="Float3" /> is equal to this instance.
        /// </summary>
        /// <param name="other">The <see cref="Float3" /> to compare with this instance.</param>
        /// <returns><c>true</c> if the specified <see cref="Float3" /> is equal to this instance; otherwise, <c>false</c>.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public bool Equals(Float3 other)
        {
            return Mathf.NearEqual(other.X, X) && Mathf.NearEqual(other.Y, Y) && Mathf.NearEqual(other.Z, Z);
        }

        /// <summary>
        /// Determines whether the specified <see cref="System.Object" /> is equal to this instance.
        /// </summary>
        /// <param name="value">The <see cref="System.Object" /> to compare with this instance.</param>
        /// <returns><c>true</c> if the specified <see cref="System.Object" /> is equal to this instance; otherwise, <c>false</c>.</returns>
        public override bool Equals(object value)
        {
            return value is Float3 other && Mathf.NearEqual(other.X, X) && Mathf.NearEqual(other.Y, Y) &&
                   Mathf.NearEqual(other.Z, Z);
        }
    }

    [Serializable]
#if SE_EDITOR
    [System.ComponentModel.TypeConverter(typeof(TypeConverters.Float4Converter))]
#endif
    public partial struct Float4 : IEquatable<Float4>, IFormattable
    {
        private static readonly string _formatString = "X:{0:F2} Y:{1:F2} Z:{2:F2} W:{3:F2}";

        /// <summary>
        /// The size of the <see cref="Float4" /> type, in bytes.
        /// </summary>
        public static readonly int SizeInBytes = Marshal.SizeOf(typeof(Float4));

        /// <summary>
        /// A <see cref="Float4" /> with all of its components set to zero.
        /// </summary>
        public static readonly Float4 Zero;

        /// <summary>
        /// The X unit <see cref="Float4" /> (1, 0, 0, 0).
        /// </summary>
        public static readonly Float4 UnitX = new Float4(1.0f, 0.0f, 0.0f, 0.0f);

        /// <summary>
        /// The Y unit <see cref="Float4" /> (0, 1, 0, 0).
        /// </summary>
        public static readonly Float4 UnitY = new Float4(0.0f, 1.0f, 0.0f, 0.0f);

        /// <summary>
        /// The Z unit <see cref="Float4" /> (0, 0, 1, 0).
        /// </summary>
        public static readonly Float4 UnitZ = new Float4(0.0f, 0.0f, 1.0f, 0.0f);

        /// <summary>
        /// The W unit <see cref="Float4" /> (0, 0, 0, 1).
        /// </summary>
        public static readonly Float4 UnitW = new Float4(0.0f, 0.0f, 0.0f, 1.0f);

        /// <summary>
        /// A <see cref="Float4" /> with all of its components set to half.
        /// </summary>
        public static readonly Float4 Half = new Float4(0.5f, 0.5f, 0.5f, 0.5f);

        /// <summary>
        /// A <see cref="Float4" /> with all of its components set to one.
        /// </summary>
        public static readonly Float4 One = new Float4(1.0f, 1.0f, 1.0f, 1.0f);

        /// <summary>
        /// A <see cref="Float4" /> with all components equal to <see cref="float.MinValue"/>.
        /// </summary>
        public static readonly Float4 Minimum = new Float4(float.MinValue);

        /// <summary>
        /// A <see cref="Float4" /> with all components equal to <see cref="float.MaxValue"/>.
        /// </summary>
        public static readonly Float4 Maximum = new Float4(float.MaxValue);

        /// <summary>
        /// Initializes a new instance of the <see cref="Float4" /> struct.
        /// </summary>
        /// <param name="value">The value that will be assigned to all components.</param>
        public Float4(float value)
        {
            X = value;
            Y = value;
            Z = value;
            W = value;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Float4" /> struct.
        /// </summary>
        /// <param name="x">Initial value for the X component of the vector.</param>
        /// <param name="y">Initial value for the Y component of the vector.</param>
        /// <param name="z">Initial value for the Z component of the vector.</param>
        /// <param name="w">Initial value for the W component of the vector.</param>
        public Float4(float x, float y, float z, float w)
        {
            X = x;
            Y = y;
            Z = z;
            W = w;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Float4" /> struct.
        /// </summary>
        /// <param name="value">A vector containing the values with which to initialize the X, Y, and Z components.</param>
        /// <param name="w">Initial value for the W component of the vector.</param>
        public Float4(Float3 value, float w)
        {
            X = value.X;
            Y = value.Y;
            Z = value.Z;
            W = w;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Float4" /> struct.
        /// </summary>
        /// <param name="xy">A vector containing the values with which to initialize the X and Y components.</param>
        /// <param name="zw">A vector containing the values with which to initialize the Z and W components.</param>
        public Float4(Float4 xy, Float4 zw)
        {
            X = xy.X;
            Y = xy.Y;
            Z = zw.X;
            W = zw.Y;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Float4" /> struct.
        /// </summary>
        /// <param name="value">A vector containing the values with which to initialize the X and Y components.</param>
        /// <param name="z">Initial value for the Z component of the vector.</param>
        /// <param name="w">Initial value for the W component of the vector.</param>
        public Float4(Float2 value, float z, float w)
        {
            X = value.X;
            Y = value.Y;
            Z = z;
            W = w;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Float4" /> struct.
        /// </summary>
        /// <param name="values">The values to assign to the X, Y, Z, and W components of the vector. This must be an array with four elements.</param>
        /// <exception cref="ArgumentNullException">Thrown when <paramref name="values" /> is <c>null</c>.</exception>
        /// <exception cref="ArgumentOutOfRangeException">Thrown when <paramref name="values" /> contains more or less than four elements.</exception>
        public Float4(float[] values)
        {
            if (values == null)
                throw new ArgumentNullException(nameof(values));
            if (values.Length != 4)
                throw new ArgumentOutOfRangeException(nameof(values),
                    "There must be four and only four input values for Float4.");
            X = values[0];
            Y = values[1];
            Z = values[2];
            W = values[3];
        }

        /// <summary>
        /// Gets a value indicting whether this instance is normalized.
        /// </summary>
        public bool IsNormalized => Mathf.IsOne(X * X + Y * Y + Z * Z + W * W);

        /// <summary>
        /// Gets a value indicting whether this vector is zero
        /// </summary>
        public bool IsZero => Mathf.IsZero(X) && Mathf.IsZero(Y) && Mathf.IsZero(Z) && Mathf.IsZero(W);

        /// <summary>
        /// Gets a value indicting whether this vector is one
        /// </summary>
        public bool IsOne => Mathf.IsOne(X) && Mathf.IsOne(Y) && Mathf.IsOne(Z) && Mathf.IsOne(W);

        /// <summary>
        /// Gets a minimum component value
        /// </summary>
        public float MinValue => Mathf.Min(X, Mathf.Min(Y, Mathf.Min(Z, W)));

        /// <summary>
        /// Gets a maximum component value
        /// </summary>
        public float MaxValue => Mathf.Max(X, Mathf.Max(Y, Mathf.Max(Z, W)));

        /// <summary>
        /// Gets an arithmetic average value of all vector components.
        /// </summary>
        public float AvgValue => (X + Y + Z + W) * (1.0f / 4.0f);

        /// <summary>
        /// Gets a sum of the component values.
        /// </summary>
        public float ValuesSum => X + Y + Z + W;

        /// <summary>
        /// Gets a vector with values being absolute values of that vector.
        /// </summary>
        public Float4 Absolute => new Float4(Mathf.Abs(X), Mathf.Abs(Y), Mathf.Abs(Z), Mathf.Abs(W));

        /// <summary>
        /// Gets a vector with values being opposite to values of that vector.
        /// </summary>
        public Float4 Negative => new Float4(-X, -Y, -Z, -W);

        /// <summary>
        /// Gets or sets the component at the specified index.
        /// </summary>
        /// <value>The value of the X, Y, Z, or W component, depending on the index.</value>
        /// <param name="index">The index of the component to access. Use 0 for the X component, 1 for the Y component, 2 for the Z component, and 3 for the W component.</param>
        /// <returns>The value of the component at the specified index.</returns>
        /// <exception cref="System.ArgumentOutOfRangeException">Thrown when the <paramref name="index" /> is out of the range [0,3].</exception>
        public float this[int index]
        {
            get
            {
                switch (index)
                {
                    case 0: return X;
                    case 1: return Y;
                    case 2: return Z;
                    case 3: return W;
                }

                throw new ArgumentOutOfRangeException(nameof(index), "Indices for Float4 run from 0 to 3, inclusive.");
            }
            set
            {
                switch (index)
                {
                    case 0:
                        X = value;
                        break;
                    case 1:
                        Y = value;
                        break;
                    case 2:
                        Z = value;
                        break;
                    case 3:
                        W = value;
                        break;
                    default:
                        throw new ArgumentOutOfRangeException(nameof(index),
                            "Indices for Float4 run from 0 to 3, inclusive.");
                }
            }
        }

        /// <summary>
        /// Calculates the length of the vector.
        /// </summary>
        /// <returns>The length of the vector.</returns>
        /// <remarks><see cref="Float4.LengthSquared" /> may be preferred when only the relative length is needed and speed is of the essence.</remarks>
        public float Length => (float)Math.Sqrt(X * X + Y * Y + Z * Z + W * W);

        /// <summary>
        /// Calculates the squared length of the vector.
        /// </summary>
        /// <returns>The squared length of the vector.</returns>
        /// <remarks>This method may be preferred to <see cref="Float4.Length" /> when only a relative length is needed and speed is of the essence.</remarks>
        public float LengthSquared => X * X + Y * Y + Z * Z + W * W;

        /// <summary>
        /// Converts the vector into a unit vector.
        /// </summary>
        public void Normalize()
        {
            float length = Length;
            if (length >= Mathf.Epsilon)
            {
                float inverse = 1.0f / length;
                X *= inverse;
                Y *= inverse;
                Z *= inverse;
                W *= inverse;
            }
        }

        /// <summary>
        /// Creates an array containing the elements of the vector.
        /// </summary>
        /// <returns>A four-element array containing the components of the vector.</returns>
        public float[] ToArray()
        {
            return new[] { X, Y, Z, W };
        }

        /// <summary>
        /// Adds two vectors.
        /// </summary>
        /// <param name="left">The first vector to add.</param>
        /// <param name="right">The second vector to add.</param>
        /// <param name="result">When the method completes, contains the sum of the two vectors.</param>
        public static void Add(ref Float4 left, ref Float4 right, out Float4 result)
        {
            result = new Float4(left.X + right.X, left.Y + right.Y, left.Z + right.Z, left.W + right.W);
        }

        /// <summary>
        /// Adds two vectors.
        /// </summary>
        /// <param name="left">The first vector to add.</param>
        /// <param name="right">The second vector to add.</param>
        /// <returns>The sum of the two vectors.</returns>
        public static Float4 Add(Float4 left, Float4 right)
        {
            return new Float4(left.X + right.X, left.Y + right.Y, left.Z + right.Z, left.W + right.W);
        }

        /// <summary>
        /// Performs a component-wise addition.
        /// </summary>
        /// <param name="left">The input vector</param>
        /// <param name="right">The scalar value to be added to elements</param>
        /// <param name="result">The vector with added scalar for each element.</param>
        public static void Add(ref Float4 left, ref float right, out Float4 result)
        {
            result = new Float4(left.X + right, left.Y + right, left.Z + right, left.W + right);
        }

        /// <summary>
        /// Performs a component-wise addition.
        /// </summary>
        /// <param name="left">The input vector</param>
        /// <param name="right">The scalar value to be added to elements</param>
        /// <returns>The vector with added scalar for each element.</returns>
        public static Float4 Add(Float4 left, float right)
        {
            return new Float4(left.X + right, left.Y + right, left.Z + right, left.W + right);
        }

        /// <summary>
        /// Subtracts two vectors.
        /// </summary>
        /// <param name="left">The first vector to subtract.</param>
        /// <param name="right">The second vector to subtract.</param>
        /// <param name="result">When the method completes, contains the difference of the two vectors.</param>
        public static void Subtract(ref Float4 left, ref Float4 right, out Float4 result)
        {
            result = new Float4(left.X - right.X, left.Y - right.Y, left.Z - right.Z, left.W - right.W);
        }

        /// <summary>
        /// Subtracts two vectors.
        /// </summary>
        /// <param name="left">The first vector to subtract.</param>
        /// <param name="right">The second vector to subtract.</param>
        /// <returns>The difference of the two vectors.</returns>
        public static Float4 Subtract(Float4 left, Float4 right)
        {
            return new Float4(left.X - right.X, left.Y - right.Y, left.Z - right.Z, left.W - right.W);
        }

        /// <summary>
        /// Performs a component-wise subtraction.
        /// </summary>
        /// <param name="left">The input vector</param>
        /// <param name="right">The scalar value to be subtracted from elements</param>
        /// <param name="result">The vector with subtracted scalar for each element.</param>
        public static void Subtract(ref Float4 left, ref float right, out Float4 result)
        {
            result = new Float4(left.X - right, left.Y - right, left.Z - right, left.W - right);
        }

        /// <summary>
        /// Performs a component-wise subtraction.
        /// </summary>
        /// <param name="left">The input vector</param>
        /// <param name="right">The scalar value to be subtracted from elements</param>
        /// <returns>The vector with subtracted scalar for each element.</returns>
        public static Float4 Subtract(Float4 left, float right)
        {
            return new Float4(left.X - right, left.Y - right, left.Z - right, left.W - right);
        }

        /// <summary>
        /// Performs a component-wise subtraction.
        /// </summary>
        /// <param name="left">The scalar value to be subtracted from elements</param>
        /// <param name="right">The input vector.</param>
        /// <param name="result">The vector with subtracted scalar for each element.</param>
        public static void Subtract(ref float left, ref Float4 right, out Float4 result)
        {
            result = new Float4(left - right.X, left - right.Y, left - right.Z, left - right.W);
        }

        /// <summary>
        /// Performs a component-wise subtraction.
        /// </summary>
        /// <param name="left">The scalar value to be subtracted from elements</param>
        /// <param name="right">The input vector.</param>
        /// <returns>The vector with subtracted scalar for each element.</returns>
        public static Float4 Subtract(float left, Float4 right)
        {
            return new Float4(left - right.X, left - right.Y, left - right.Z, left - right.W);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <param name="result">When the method completes, contains the scaled vector.</param>
        public static void Multiply(ref Float4 value, float scale, out Float4 result)
        {
            result = new Float4(value.X * scale, value.Y * scale, value.Z * scale, value.W * scale);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The scaled vector.</returns>
        public static Float4 Multiply(Float4 value, float scale)
        {
            return new Float4(value.X * scale, value.Y * scale, value.Z * scale, value.W * scale);
        }

        /// <summary>
        /// Multiplies a vector with another by performing component-wise multiplication.
        /// </summary>
        /// <param name="left">The first vector to multiply.</param>
        /// <param name="right">The second vector to multiply.</param>
        /// <param name="result">When the method completes, contains the multiplied vector.</param>
        public static void Multiply(ref Float4 left, ref Float4 right, out Float4 result)
        {
            result = new Float4(left.X * right.X, left.Y * right.Y, left.Z * right.Z, left.W * right.W);
        }

        /// <summary>
        /// Multiplies a vector with another by performing component-wise multiplication.
        /// </summary>
        /// <param name="left">The first vector to multiply.</param>
        /// <param name="right">The second vector to multiply.</param>
        /// <returns>The multiplied vector.</returns>
        public static Float4 Multiply(Float4 left, Float4 right)
        {
            return new Float4(left.X * right.X, left.Y * right.Y, left.Z * right.Z, left.W * right.W);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <param name="result">When the method completes, contains the scaled vector.</param>
        public static void Divide(ref Float4 value, float scale, out Float4 result)
        {
            result = new Float4(value.X / scale, value.Y / scale, value.Z / scale, value.W / scale);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The scaled vector.</returns>
        public static Float4 Divide(Float4 value, float scale)
        {
            return new Float4(value.X / scale, value.Y / scale, value.Z / scale, value.W / scale);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <param name="value">The vector to scale.</param>
        /// <param name="result">When the method completes, contains the scaled vector.</param>
        public static void Divide(float scale, ref Float4 value, out Float4 result)
        {
            result = new Float4(scale / value.X, scale / value.Y, scale / value.Z, scale / value.W);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The scaled vector.</returns>
        public static Float4 Divide(float scale, Float4 value)
        {
            return new Float4(scale / value.X, scale / value.Y, scale / value.Z, scale / value.W);
        }

        /// <summary>
        /// Reverses the direction of a given vector.
        /// </summary>
        /// <param name="value">The vector to negate.</param>
        /// <param name="result">When the method completes, contains a vector facing in the opposite direction.</param>
        public static void Negate(ref Float4 value, out Float4 result)
        {
            result = new Float4(-value.X, -value.Y, -value.Z, -value.W);
        }

        /// <summary>
        /// Reverses the direction of a given vector.
        /// </summary>
        /// <param name="value">The vector to negate.</param>
        /// <returns>A vector facing in the opposite direction.</returns>
        public static Float4 Negate(Float4 value)
        {
            return new Float4(-value.X, -value.Y, -value.Z, -value.W);
        }

        /// <summary>
        /// Returns a <see cref="Float4" /> containing the 4D Cartesian coordinates of a point specified in Barycentric coordinates relative to a 4D triangle.
        /// </summary>
        /// <param name="value1">A <see cref="Float4" /> containing the 4D Cartesian coordinates of vertex 1 of the triangle.</param>
        /// <param name="value2">A <see cref="Float4" /> containing the 4D Cartesian coordinates of vertex 2 of the triangle.</param>
        /// <param name="value3">A <see cref="Float4" /> containing the 4D Cartesian coordinates of vertex 3 of the triangle.</param>
        /// <param name="amount1">Barycentric coordinate b2, which expresses the weighting factor toward vertex 2 (specified in <paramref name="value2" />).</param>
        /// <param name="amount2">Barycentric coordinate b3, which expresses the weighting factor toward vertex 3 (specified in <paramref name="value3" />).</param>
        /// <param name="result">When the method completes, contains the 4D Cartesian coordinates of the specified point.</param>
        public static void Barycentric(ref Float4 value1, ref Float4 value2, ref Float4 value3, float amount1,
            float amount2, out Float4 result)
        {
            result = new Float4(value1.X + amount1 * (value2.X - value1.X) + amount2 * (value3.X - value1.X),
                value1.Y + amount1 * (value2.Y - value1.Y) + amount2 * (value3.Y - value1.Y),
                value1.Z + amount1 * (value2.Z - value1.Z) + amount2 * (value3.Z - value1.Z),
                value1.W + amount1 * (value2.W - value1.W) + amount2 * (value3.W - value1.W));
        }

        /// <summary>
        /// Returns a <see cref="Float4" /> containing the 4D Cartesian coordinates of a point specified in Barycentric coordinates relative to a 4D triangle.
        /// </summary>
        /// <param name="value1">A <see cref="Float4" /> containing the 4D Cartesian coordinates of vertex 1 of the triangle.</param>
        /// <param name="value2">A <see cref="Float4" /> containing the 4D Cartesian coordinates of vertex 2 of the triangle.</param>
        /// <param name="value3">A <see cref="Float4" /> containing the 4D Cartesian coordinates of vertex 3 of the triangle.</param>
        /// <param name="amount1">Barycentric coordinate b2, which expresses the weighting factor toward vertex 2 (specified in <paramref name="value2" />).</param>
        /// <param name="amount2">Barycentric coordinate b3, which expresses the weighting factor toward vertex 3 (specified in <paramref name="value3" />).</param>
        /// <returns>A new <see cref="Float4" /> containing the 4D Cartesian coordinates of the specified point.</returns>
        public static Float4 Barycentric(Float4 value1, Float4 value2, Float4 value3, float amount1, float amount2)
        {
            Barycentric(ref value1, ref value2, ref value3, amount1, amount2, out Float4 result);
            return result;
        }

        /// <summary>
        /// Restricts a value to be within a specified range.
        /// </summary>
        /// <param name="value">The value to clamp.</param>
        /// <param name="min">The minimum value.</param>
        /// <param name="max">The maximum value.</param>
        /// <param name="result">When the method completes, contains the clamped value.</param>
        public static void Clamp(ref Float4 value, ref Float4 min, ref Float4 max, out Float4 result)
        {
            float x = value.X;
            x = x > max.X ? max.X : x;
            x = x < min.X ? min.X : x;
            float y = value.Y;
            y = y > max.Y ? max.Y : y;
            y = y < min.Y ? min.Y : y;
            float z = value.Z;
            z = z > max.Z ? max.Z : z;
            z = z < min.Z ? min.Z : z;
            float w = value.W;
            w = w > max.W ? max.W : w;
            w = w < min.W ? min.W : w;
            result = new Float4(x, y, z, w);
        }

        /// <summary>
        /// Restricts a value to be within a specified range.
        /// </summary>
        /// <param name="value">The value to clamp.</param>
        /// <param name="min">The minimum value.</param>
        /// <param name="max">The maximum value.</param>
        /// <returns>The clamped value.</returns>
        public static Float4 Clamp(Float4 value, Float4 min, Float4 max)
        {
            Clamp(ref value, ref min, ref max, out Float4 result);
            return result;
        }

        /// <summary>
        /// Calculates the distance between two vectors.
        /// </summary>
        /// <param name="value1">The first vector.</param>
        /// <param name="value2">The second vector.</param>
        /// <param name="result">When the method completes, contains the distance between the two vectors.</param>
        /// <remarks><see cref="Float4.DistanceSquared(ref Float4, ref Float4, out float)" /> may be preferred when only the relative distance is needed and speed is of the essence.</remarks>
        public static void Distance(ref Float4 value1, ref Float4 value2, out float result)
        {
            float x = value1.X - value2.X;
            float y = value1.Y - value2.Y;
            float z = value1.Z - value2.Z;
            float w = value1.W - value2.W;
            result = (float)Math.Sqrt(x * x + y * y + z * z + w * w);
        }

        /// <summary>
        /// Calculates the distance between two vectors.
        /// </summary>
        /// <param name="value1">The first vector.</param>
        /// <param name="value2">The second vector.</param>
        /// <returns>The distance between the two vectors.</returns>
        /// <remarks><see cref="Float4.DistanceSquared(Float4, Float4)" /> may be preferred when only the relative distance is needed and speed is of the essence.</remarks>
        public static float Distance(Float4 value1, Float4 value2)
        {
            float x = value1.X - value2.X;
            float y = value1.Y - value2.Y;
            float z = value1.Z - value2.Z;
            float w = value1.W - value2.W;
            return (float)Math.Sqrt(x * x + y * y + z * z + w * w);
        }

        /// <summary>
        /// Calculates the squared distance between two vectors.
        /// </summary>
        /// <param name="value1">The first vector.</param>
        /// <param name="value2">The second vector.</param>
        /// <param name="result">When the method completes, contains the squared distance between the two vectors.</param>
        public static void DistanceSquared(ref Float4 value1, ref Float4 value2, out float result)
        {
            float x = value1.X - value2.X;
            float y = value1.Y - value2.Y;
            float z = value1.Z - value2.Z;
            float w = value1.W - value2.W;
            result = x * x + y * y + z * z + w * w;
        }

        /// <summary>
        /// Calculates the squared distance between two vectors.
        /// </summary>
        /// <param name="value1">The first vector.</param>
        /// <param name="value2">The second vector.</param>
        /// <returns>The squared distance between the two vectors.</returns>
        public static float DistanceSquared(Float4 value1, Float4 value2)
        {
            float x = value1.X - value2.X;
            float y = value1.Y - value2.Y;
            float z = value1.Z - value2.Z;
            float w = value1.W - value2.W;
            return x * x + y * y + z * z + w * w;
        }

        /// <summary>
        /// Tests whether one vector is near another vector.
        /// </summary>
        /// <param name="left">The left vector.</param>
        /// <param name="right">The right vector.</param>
        /// <param name="epsilon">The epsilon.</param>
        /// <returns><c>true</c> if left and right are near another, <c>false</c> otherwise</returns>
        public static bool NearEqual(Float4 left, Float4 right, float epsilon = Mathf.Epsilon)
        {
            return NearEqual(ref left, ref right, epsilon);
        }

        /// <summary>
        /// Tests whether one vector is near another vector.
        /// </summary>
        /// <param name="left">The left vector.</param>
        /// <param name="right">The right vector.</param>
        /// <param name="epsilon">The epsilon.</param>
        /// <returns><c>true</c> if left and right are near another, <c>false</c> otherwise</returns>
        public static bool NearEqual(ref Float4 left, ref Float4 right, float epsilon = Mathf.Epsilon)
        {
            return Mathf.WithinEpsilon(left.X, right.X, epsilon) && Mathf.WithinEpsilon(left.Y, right.Y, epsilon) &&
                   Mathf.WithinEpsilon(left.Z, right.Z, epsilon) && Mathf.WithinEpsilon(left.W, right.W, epsilon);
        }

        /// <summary>
        /// Calculates the dot product of two vectors.
        /// </summary>
        /// <param name="left">First source vector</param>
        /// <param name="right">Second source vector.</param>
        /// <param name="result">When the method completes, contains the dot product of the two vectors.</param>
        public static void Dot(ref Float4 left, ref Float4 right, out float result)
        {
            result = left.X * right.X + left.Y * right.Y + left.Z * right.Z + left.W * right.W;
        }

        /// <summary>
        /// Calculates the dot product of two vectors.
        /// </summary>
        /// <param name="left">First source vector.</param>
        /// <param name="right">Second source vector.</param>
        /// <returns>The dot product of the two vectors.</returns>
        public static float Dot(Float4 left, Float4 right)
        {
            return left.X * right.X + left.Y * right.Y + left.Z * right.Z + left.W * right.W;
        }

        /// <summary>
        /// Converts the vector into a unit vector.
        /// </summary>
        /// <param name="value">The vector to normalize.</param>
        /// <param name="result">When the method completes, contains the normalized vector.</param>
        public static void Normalize(ref Float4 value, out Float4 result)
        {
            result = value;
            result.Normalize();
        }

        /// <summary>
        /// Converts the vector into a unit vector.
        /// </summary>
        /// <param name="value">The vector to normalize.</param>
        /// <returns>The normalized vector.</returns>
        public static Float4 Normalize(Float4 value)
        {
            value.Normalize();
            return value;
        }

        /// <summary>
        /// Makes sure that Length of the output vector is always below max and above 0.
        /// </summary>
        /// <param name="vector">Input vector.</param>
        /// <param name="max">Max Length</param>
        public static Float4 ClampLength(Float4 vector, float max)
        {
            return ClampLength(vector, 0, max);
        }

        /// <summary>
        /// Makes sure that Length of the output vector is always below max and above min.
        /// </summary>
        /// <param name="vector">Input vector.</param>
        /// <param name="min">Min Length</param>
        /// <param name="max">Max Length</param>
        public static Float4 ClampLength(Float4 vector, float min, float max)
        {
            ClampLength(vector, min, max, out Float4 result);
            return result;
        }

        /// <summary>
        /// Makes sure that Length of the output vector is always below max and above min.
        /// </summary>
        /// <param name="vector">Input vector.</param>
        /// <param name="min">Min Length</param>
        /// <param name="max">Max Length</param>
        /// <param name="result">The result vector.</param>
        public static void ClampLength(Float4 vector, float min, float max, out Float4 result)
        {
            result = vector;
            float lenSq = result.LengthSquared;
            if (lenSq > max * max)
            {
                float scaleFactor = max / (float)Math.Sqrt(lenSq);
                result.X *= scaleFactor;
                result.Y *= scaleFactor;
                result.Z *= scaleFactor;
                result.W *= scaleFactor;
            }

            if (lenSq < min * min)
            {
                float scaleFactor = min / (float)Math.Sqrt(lenSq);
                result.X *= scaleFactor;
                result.Y *= scaleFactor;
                result.Z *= scaleFactor;
                result.W *= scaleFactor;
            }
        }

        /// <summary>
        /// Performs a linear interpolation between two vectors.
        /// </summary>
        /// <param name="start">Start vector.</param>
        /// <param name="end">End vector.</param>
        /// <param name="amount">Value between 0 and 1 indicating the weight of <paramref name="end" />.</param>
        /// <param name="result">When the method completes, contains the linear interpolation of the two vectors.</param>
        /// <remarks>Passing <paramref name="amount" /> a value of 0 will cause <paramref name="start" /> to be returned; a value of 1 will cause <paramref name="end" /> to be returned.</remarks>
        public static void Lerp(ref Float4 start, ref Float4 end, float amount, out Float4 result)
        {
            result = default;
            result.X = Mathf.Lerp(start.X, end.X, amount);
            result.Y = Mathf.Lerp(start.Y, end.Y, amount);
            result.Z = Mathf.Lerp(start.Z, end.Z, amount);
            result.W = Mathf.Lerp(start.W, end.W, amount);
        }

        /// <summary>
        /// Performs a linear interpolation between two vectors.
        /// </summary>
        /// <param name="start">Start vector.</param>
        /// <param name="end">End vector.</param>
        /// <param name="amount">Value between 0 and 1 indicating the weight of <paramref name="end" />.</param>
        /// <returns>The linear interpolation of the two vectors.</returns>
        /// <remarks>Passing <paramref name="amount" /> a value of 0 will cause <paramref name="start" /> to be returned; a value of 1 will cause <paramref name="end" /> to be returned.</remarks>
        public static Float4 Lerp(Float4 start, Float4 end, float amount)
        {
            Lerp(ref start, ref end, amount, out Float4 result);
            return result;
        }

        /// <summary>
        /// Performs a cubic interpolation between two vectors.
        /// </summary>
        /// <param name="start">Start vector.</param>
        /// <param name="end">End vector.</param>
        /// <param name="amount">Value between 0 and 1 indicating the weight of <paramref name="end" />.</param>
        /// <param name="result">When the method completes, contains the cubic interpolation of the two vectors.</param>
        public static void SmoothStep(ref Float4 start, ref Float4 end, float amount, out Float4 result)
        {
            amount = Mathf.SmoothStep(amount);
            Lerp(ref start, ref end, amount, out result);
        }

        /// <summary>
        /// Performs a cubic interpolation between two vectors.
        /// </summary>
        /// <param name="start">Start vector.</param>
        /// <param name="end">End vector.</param>
        /// <param name="amount">Value between 0 and 1 indicating the weight of <paramref name="end" />.</param>
        /// <returns>The cubic interpolation of the two vectors.</returns>
        public static Float4 SmoothStep(Float4 start, Float4 end, float amount)
        {
            SmoothStep(ref start, ref end, amount, out Float4 result);
            return result;
        }

        /// <summary>
        /// Performs a Hermite spline interpolation.
        /// </summary>
        /// <param name="value1">First source position vector.</param>
        /// <param name="tangent1">First source tangent vector.</param>
        /// <param name="value2">Second source position vector.</param>
        /// <param name="tangent2">Second source tangent vector.</param>
        /// <param name="amount">Weighting factor.</param>
        /// <param name="result">When the method completes, contains the result of the Hermite spline interpolation.</param>
        public static void Hermite(ref Float4 value1, ref Float4 tangent1, ref Float4 value2, ref Float4 tangent2,
            float amount, out Float4 result)
        {
            float squared = amount * amount;
            float cubed = amount * squared;
            float part1 = 2.0f * cubed - 3.0f * squared + 1.0f;
            float part2 = -2.0f * cubed + 3.0f * squared;
            float part3 = cubed - 2.0f * squared + amount;
            float part4 = cubed - squared;
            result = new Float4(value1.X * part1 + value2.X * part2 + tangent1.X * part3 + tangent2.X * part4,
                value1.Y * part1 + value2.Y * part2 + tangent1.Y * part3 + tangent2.Y * part4,
                value1.Z * part1 + value2.Z * part2 + tangent1.Z * part3 + tangent2.Z * part4,
                value1.W * part1 + value2.W * part2 + tangent1.W * part3 + tangent2.W * part4);
        }

        /// <summary>
        /// Performs a Hermite spline interpolation.
        /// </summary>
        /// <param name="value1">First source position vector.</param>
        /// <param name="tangent1">First source tangent vector.</param>
        /// <param name="value2">Second source position vector.</param>
        /// <param name="tangent2">Second source tangent vector.</param>
        /// <param name="amount">Weighting factor.</param>
        /// <returns>The result of the Hermite spline interpolation.</returns>
        public static Float4 Hermite(Float4 value1, Float4 tangent1, Float4 value2, Float4 tangent2, float amount)
        {
            Hermite(ref value1, ref tangent1, ref value2, ref tangent2, amount, out Float4 result);
            return result;
        }

        /// <summary>
        /// Performs a Catmull-Rom interpolation using the specified positions.
        /// </summary>
        /// <param name="value1">The first position in the interpolation.</param>
        /// <param name="value2">The second position in the interpolation.</param>
        /// <param name="value3">The third position in the interpolation.</param>
        /// <param name="value4">The fourth position in the interpolation.</param>
        /// <param name="amount">Weighting factor.</param>
        /// <param name="result">When the method completes, contains the result of the Catmull-Rom interpolation.</param>
        public static void CatmullRom(ref Float4 value1, ref Float4 value2, ref Float4 value3, ref Float4 value4,
            float amount, out Float4 result)
        {
            result = default;
            float squared = amount * amount;
            float cubed = amount * squared;
            result.X = 0.5f * (2.0f * value2.X + (-value1.X + value3.X) * amount +
                               (2.0f * value1.X - 5.0f * value2.X + 4.0f * value3.X - value4.X) * squared +
                               (-value1.X + 3.0f * value2.X - 3.0f * value3.X + value4.X) * cubed);
            result.Y = 0.5f * (2.0f * value2.Y + (-value1.Y + value3.Y) * amount +
                               (2.0f * value1.Y - 5.0f * value2.Y + 4.0f * value3.Y - value4.Y) * squared +
                               (-value1.Y + 3.0f * value2.Y - 3.0f * value3.Y + value4.Y) * cubed);
            result.Z = 0.5f * (2.0f * value2.Z + (-value1.Z + value3.Z) * amount +
                               (2.0f * value1.Z - 5.0f * value2.Z + 4.0f * value3.Z - value4.Z) * squared +
                               (-value1.Z + 3.0f * value2.Z - 3.0f * value3.Z + value4.Z) * cubed);
            result.W = 0.5f * (2.0f * value2.W + (-value1.W + value3.W) * amount +
                               (2.0f * value1.W - 5.0f * value2.W + 4.0f * value3.W - value4.W) * squared +
                               (-value1.W + 3.0f * value2.W - 3.0f * value3.W + value4.W) * cubed);
        }

        /// <summary>
        /// Performs a Catmull-Rom interpolation using the specified positions.
        /// </summary>
        /// <param name="value1">The first position in the interpolation.</param>
        /// <param name="value2">The second position in the interpolation.</param>
        /// <param name="value3">The third position in the interpolation.</param>
        /// <param name="value4">The fourth position in the interpolation.</param>
        /// <param name="amount">Weighting factor.</param>
        /// <returns>A vector that is the result of the Catmull-Rom interpolation.</returns>
        public static Float4 CatmullRom(Float4 value1, Float4 value2, Float4 value3, Float4 value4, float amount)
        {
            CatmullRom(ref value1, ref value2, ref value3, ref value4, amount, out Float4 result);
            return result;
        }

        /// <summary>
        /// Returns a vector containing the largest components of the specified vectors.
        /// </summary>
        /// <param name="left">The first source vector.</param>
        /// <param name="right">The second source vector.</param>
        /// <param name="result">When the method completes, contains an new vector composed of the largest components of the source vectors.</param>
        public static void Max(ref Float4 left, ref Float4 right, out Float4 result)
        {
            result = default;
            result.X = left.X > right.X ? left.X : right.X;
            result.Y = left.Y > right.Y ? left.Y : right.Y;
            result.Z = left.Z > right.Z ? left.Z : right.Z;
            result.W = left.W > right.W ? left.W : right.W;
        }

        /// <summary>
        /// Returns a vector containing the largest components of the specified vectors.
        /// </summary>
        /// <param name="left">The first source vector.</param>
        /// <param name="right">The second source vector.</param>
        /// <returns>A vector containing the largest components of the source vectors.</returns>
        public static Float4 Max(Float4 left, Float4 right)
        {
            Max(ref left, ref right, out Float4 result);
            return result;
        }

        /// <summary>
        /// Returns a vector containing the smallest components of the specified vectors.
        /// </summary>
        /// <param name="left">The first source vector.</param>
        /// <param name="right">The second source vector.</param>
        /// <param name="result">When the method completes, contains an new vector composed of the smallest components of the source vectors.</param>
        public static void Min(ref Float4 left, ref Float4 right, out Float4 result)
        {
            result = default;
            result.X = left.X < right.X ? left.X : right.X;
            result.Y = left.Y < right.Y ? left.Y : right.Y;
            result.Z = left.Z < right.Z ? left.Z : right.Z;
            result.W = left.W < right.W ? left.W : right.W;
        }

        /// <summary>
        /// Returns a vector containing the smallest components of the specified vectors.
        /// </summary>
        /// <param name="left">The first source vector.</param>
        /// <param name="right">The second source vector.</param>
        /// <returns>A vector containing the smallest components of the source vectors.</returns>
        public static Float4 Min(Float4 left, Float4 right)
        {
            Min(ref left, ref right, out Float4 result);
            return result;
        }

        /// <summary>
        /// Returns the absolute value of a vector.
        /// </summary>
        /// <param name="v">The value.</param>
        /// <returns> A vector which components are less or equal to 0.</returns>
        public static Float4 Abs(Float4 v)
        {
            return new Float4(Math.Abs(v.X), Math.Abs(v.Y), Math.Abs(v.Z), Math.Abs(v.W));
        }

        /// <summary>
        /// Transforms a 4D vector by the given <see cref="Quaternion" /> rotation.
        /// </summary>
        /// <param name="vector">The vector to rotate.</param>
        /// <param name="rotation">The <see cref="Quaternion" /> rotation to apply.</param>
        /// <param name="result">When the method completes, contains the transformed <see cref="Float4" />.</param>
        public static void Transform(ref Float4 vector, ref Quaternion rotation, out Float4 result)
        {
            float x = rotation.X + rotation.X;
            float y = rotation.Y + rotation.Y;
            float z = rotation.Z + rotation.Z;
            float wx = rotation.W * x;
            float wy = rotation.W * y;
            float wz = rotation.W * z;
            float xx = rotation.X * x;
            float xy = rotation.X * y;
            float xz = rotation.X * z;
            float yy = rotation.Y * y;
            float yz = rotation.Y * z;
            float zz = rotation.Z * z;
            result = new Float4(vector.X * (1.0f - yy - zz) + vector.Y * (xy - wz) + vector.Z * (xz + wy),
                vector.X * (xy + wz) + vector.Y * (1.0f - xx - zz) + vector.Z * (yz - wx),
                vector.X * (xz - wy) + vector.Y * (yz + wx) + vector.Z * (1.0f - xx - yy),
                vector.W);
        }

        /// <summary>
        /// Transforms a 4D vector by the given <see cref="Quaternion" /> rotation.
        /// </summary>
        /// <param name="vector">The vector to rotate.</param>
        /// <param name="rotation">The <see cref="Quaternion" /> rotation to apply.</param>
        /// <returns>The transformed <see cref="Float4" />.</returns>
        public static Float4 Transform(Float4 vector, Quaternion rotation)
        {
            Transform(ref vector, ref rotation, out Float4 result);
            return result;
        }

        /// <summary>
        /// Transforms a 4D vector by the given <see cref="Matrix" />.
        /// </summary>
        /// <param name="vector">The source vector.</param>
        /// <param name="transform">The transformation <see cref="Matrix" />.</param>
        /// <param name="result">When the method completes, contains the transformed <see cref="Float4" />.</param>
        public static void Transform(ref Float4 vector, ref Matrix transform, out Float4 result)
        {
            result = new Float4(
                vector.X * transform.M11 + vector.Y * transform.M21 + vector.Z * transform.M31 +
                vector.W * transform.M41,
                vector.X * transform.M12 + vector.Y * transform.M22 + vector.Z * transform.M32 +
                vector.W * transform.M42,
                vector.X * transform.M13 + vector.Y * transform.M23 + vector.Z * transform.M33 +
                vector.W * transform.M43,
                vector.X * transform.M14 + vector.Y * transform.M24 + vector.Z * transform.M34 +
                vector.W * transform.M44);
        }

        /// <summary>
        /// Transforms a 4D vector by the given <see cref="Matrix" />.
        /// </summary>
        /// <param name="vector">The source vector.</param>
        /// <param name="transform">The transformation <see cref="Matrix" />.</param>
        /// <returns>The transformed <see cref="Float4" />.</returns>
        public static Float4 Transform(Float4 vector, Matrix transform)
        {
            Transform(ref vector, ref transform, out Float4 result);
            return result;
        }

        /// <summary>
        /// Adds two vectors.
        /// </summary>
        /// <param name="left">The first vector to add.</param>
        /// <param name="right">The second vector to add.</param>
        /// <returns>The sum of the two vectors.</returns>
        public static Float4 operator +(Float4 left, Float4 right)
        {
            return new Float4(left.X + right.X, left.Y + right.Y, left.Z + right.Z, left.W + right.W);
        }

        /// <summary>
        /// Multiplies a vector with another by performing component-wise multiplication equivalent to <see cref="Multiply(ref Float4,ref Float4,out Float4)" />.
        /// </summary>
        /// <param name="left">The first vector to multiply.</param>
        /// <param name="right">The second vector to multiply.</param>
        /// <returns>The multiplication of the two vectors.</returns>
        public static Float4 operator *(Float4 left, Float4 right)
        {
            return new Float4(left.X * right.X, left.Y * right.Y, left.Z * right.Z, left.W * right.W);
        }

        /// <summary>
        /// Assert a vector (return it unchanged).
        /// </summary>
        /// <param name="value">The vector to assert (unchanged).</param>
        /// <returns>The asserted (unchanged) vector.</returns>
        public static Float4 operator +(Float4 value)
        {
            return value;
        }

        /// <summary>
        /// Subtracts two vectors.
        /// </summary>
        /// <param name="left">The first vector to subtract.</param>
        /// <param name="right">The second vector to subtract.</param>
        /// <returns>The difference of the two vectors.</returns>
        public static Float4 operator -(Float4 left, Float4 right)
        {
            return new Float4(left.X - right.X, left.Y - right.Y, left.Z - right.Z, left.W - right.W);
        }

        /// <summary>
        /// Reverses the direction of a given vector.
        /// </summary>
        /// <param name="value">The vector to negate.</param>
        /// <returns>A vector facing in the opposite direction.</returns>
        public static Float4 operator -(Float4 value)
        {
            return new Float4(-value.X, -value.Y, -value.Z, -value.W);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The scaled vector.</returns>
        public static Float4 operator *(float scale, Float4 value)
        {
            return new Float4(value.X * scale, value.Y * scale, value.Z * scale, value.W * scale);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The scaled vector.</returns>
        public static Float4 operator *(Float4 value, float scale)
        {
            return new Float4(value.X * scale, value.Y * scale, value.Z * scale, value.W * scale);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The scaled vector.</returns>
        public static Float4 operator /(Float4 value, float scale)
        {
            return new Float4(value.X / scale, value.Y / scale, value.Z / scale, value.W / scale);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <param name="value">The vector to scale.</param>
        /// <returns>The scaled vector.</returns>
        public static Float4 operator /(float scale, Float4 value)
        {
            return new Float4(scale / value.X, scale / value.Y, scale / value.Z, scale / value.W);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The scaled vector.</returns>
        public static Float4 operator *(double scale, Float4 value)
        {
            var s = (float)scale;
            return new Float4(value.X * s, value.Y * s, value.Z * s, value.W * s);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The scaled vector.</returns>
        public static Float4 operator *(Float4 value, double scale)
        {
            var s = (float)scale;
            return new Float4(value.X * s, value.Y * s, value.Z * s, value.W * s);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The scaled vector.</returns>
        public static Float4 operator /(Float4 value, double scale)
        {
            var s = (float)scale;
            return new Float4(value.X / s, value.Y / s, value.Z / s, value.W / s);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <param name="value">The vector to scale.</param>
        /// <returns>The scaled vector.</returns>
        public static Float4 operator /(double scale, Float4 value)
        {
            var s = (float)scale;
            return new Float4(s / value.X, s / value.Y, s / value.Z, s / value.W);
        }

        /// <summary>
        /// Scales a vector by the given value.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The scaled vector.</returns>
        public static Float4 operator /(Float4 value, Float4 scale)
        {
            return new Float4(value.X / scale.X, value.Y / scale.Y, value.Z / scale.Z, value.W / scale.W);
        }

        /// <summary>
        /// Remainder of value divided by scale.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The remained vector.</returns>
        public static Float4 operator %(Float4 value, float scale)
        {
            return new Float4(value.X % scale, value.Y % scale, value.Z % scale, value.W % scale);
        }

        /// <summary>
        /// Remainder of value divided by scale.
        /// </summary>
        /// <param name="value">The amount by which to scale the vector.</param>
        /// <param name="scale">The vector to scale.</param>
        /// <returns>The remained vector.</returns>
        public static Float4 operator %(float value, Float4 scale)
        {
            return new Float4(value % scale.X, value % scale.Y, value % scale.Z, value % scale.W);
        }

        /// <summary>
        /// Remainder of value divided by scale.
        /// </summary>
        /// <param name="value">The vector to scale.</param>
        /// <param name="scale">The amount by which to scale the vector.</param>
        /// <returns>The remained vector.</returns>
        public static Float4 operator %(Float4 value, Float4 scale)
        {
            return new Float4(value.X % scale.X, value.Y % scale.Y, value.Z % scale.Z, value.W % scale.W);
        }

        /// <summary>
        /// Performs a component-wise addition.
        /// </summary>
        /// <param name="value">The input vector.</param>
        /// <param name="scalar">The scalar value to be added on elements</param>
        /// <returns>The vector with added scalar for each element.</returns>
        public static Float4 operator +(Float4 value, float scalar)
        {
            return new Float4(value.X + scalar, value.Y + scalar, value.Z + scalar, value.W + scalar);
        }

        /// <summary>
        /// Performs a component-wise addition.
        /// </summary>
        /// <param name="value">The input vector.</param>
        /// <param name="scalar">The scalar value to be added on elements</param>
        /// <returns>The vector with added scalar for each element.</returns>
        public static Float4 operator +(float scalar, Float4 value)
        {
            return new Float4(scalar + value.X, scalar + value.Y, scalar + value.Z, scalar + value.W);
        }

        /// <summary>
        /// Performs a component-wise subtraction.
        /// </summary>
        /// <param name="value">The input vector.</param>
        /// <param name="scalar">The scalar value to be subtracted from elements</param>
        /// <returns>The vector with subtracted scalar from each element.</returns>
        public static Float4 operator -(Float4 value, float scalar)
        {
            return new Float4(value.X - scalar, value.Y - scalar, value.Z - scalar, value.W - scalar);
        }

        /// <summary>
        /// Performs a component-wise subtraction.
        /// </summary>
        /// <param name="value">The input vector.</param>
        /// <param name="scalar">The scalar value to be subtracted from elements</param>
        /// <returns>The vector with subtracted scalar from each element.</returns>
        public static Float4 operator -(float scalar, Float4 value)
        {
            return new Float4(scalar - value.X, scalar - value.Y, scalar - value.Z, scalar - value.W);
        }

        /// <summary>
        /// Tests for equality between two objects.
        /// </summary>
        /// <param name="left">The first value to compare.</param>
        /// <param name="right">The second value to compare.</param>
        /// <returns><c>true</c> if <paramref name="left" /> has the same value as <paramref name="right" />; otherwise, <c>false</c>.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool operator ==(Float4 left, Float4 right)
        {
            return Mathf.NearEqual(left.X, right.X) && Mathf.NearEqual(left.Y, right.Y) &&
                   Mathf.NearEqual(left.Z, right.Z) && Mathf.NearEqual(left.W, right.W);
        }

        /// <summary>
        /// Tests for inequality between two objects.
        /// </summary>
        /// <param name="left">The first value to compare.</param>
        /// <param name="right">The second value to compare.</param>
        /// <returns><c>true</c> if <paramref name="left" /> has a different value than <paramref name="right" />; otherwise, <c>false</c>.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool operator !=(Float4 left, Float4 right)
        {
            return !left.Equals(ref right);
        }

        /// <summary>
        /// Performs an explicit conversion from <see cref="Float4" /> to <see cref="Vector4" />.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <returns>The result of the conversion.</returns>
        public static implicit operator Vector4(Float4 value)
        {
            return new Vector4(value.X, value.Y, value.Z, value.W);
        }

        /// <summary>
        /// Performs an implicit conversion from <see cref="Float4" /> to <see cref="Double4" />.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <returns>The result of the conversion.</returns>
        public static implicit operator Double4(Float4 value)
        {
            return new Double4(value.X, value.Y, value.Z, value.W);
        }

        /// <summary>
        /// Performs an explicit conversion from <see cref="Float4" /> to <see cref="Float2" />.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <returns>The result of the conversion.</returns>
        public static explicit operator Float2(Float4 value)
        {
            return new Float2(value.X, value.Y);
        }

        /// <summary>
        /// Performs an explicit conversion from <see cref="Float4" /> to <see cref="Float3" />.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <returns>The result of the conversion.</returns>
        public static explicit operator Float3(Float4 value)
        {
            return new Float3(value.X, value.Y, value.Z);
        }

        /// <summary>
        /// Returns a <see cref="System.String" /> that represents this instance.
        /// </summary>
        /// <returns>A <see cref="System.String" /> that represents this instance.</returns>
        public override string ToString()
        {
            return string.Format(CultureInfo.CurrentCulture, _formatString, X, Y, Z, W);
        }

        /// <summary>
        /// Returns a <see cref="System.String" /> that represents this instance.
        /// </summary>
        /// <param name="format">The format.</param>
        /// <returns>A <see cref="System.String" /> that represents this instance.</returns>
        public string ToString(string format)
        {
            if (format == null)
                return ToString();
            return string.Format(CultureInfo.CurrentCulture, _formatString,
                X.ToString(format, CultureInfo.CurrentCulture), Y.ToString(format, CultureInfo.CurrentCulture),
                Z.ToString(format, CultureInfo.CurrentCulture), W.ToString(format, CultureInfo.CurrentCulture));
        }

        /// <summary>
        /// Returns a <see cref="System.String" /> that represents this instance.
        /// </summary>
        /// <param name="formatProvider">The format provider.</param>
        /// <returns>A <see cref="System.String" /> that represents this instance.</returns>
        public string ToString(IFormatProvider formatProvider)
        {
            return string.Format(formatProvider, _formatString, X, Y, Z, W);
        }

        /// <summary>
        /// Returns a <see cref="System.String" /> that represents this instance.
        /// </summary>
        /// <param name="format">The format.</param>
        /// <param name="formatProvider">The format provider.</param>
        /// <returns>A <see cref="System.String" /> that represents this instance.</returns>
        public string ToString(string format, IFormatProvider formatProvider)
        {
            if (format == null)
                return ToString(formatProvider);
            return string.Format(formatProvider, "X:{0} Y:{1} Z:{2} W:{3}", X.ToString(format, formatProvider),
                Y.ToString(format, formatProvider), Z.ToString(format, formatProvider),
                W.ToString(format, formatProvider));
        }

        /// <summary>
        /// Returns a hash code for this instance.
        /// </summary>
        public override int GetHashCode()
        {
            unchecked
            {
                int hashCode = X.GetHashCode();
                hashCode = (hashCode * 397) ^ Y.GetHashCode();
                hashCode = (hashCode * 397) ^ Z.GetHashCode();
                hashCode = (hashCode * 397) ^ W.GetHashCode();
                return hashCode;
            }
        }

        /// <summary>
        /// Determines whether the specified <see cref="Float4" /> is equal to this instance.
        /// </summary>
        /// <param name="other">The <see cref="Float4" /> to compare with this instance.</param>
        /// <returns><c>true</c> if the specified <see cref="Float4" /> is equal to this instance; otherwise, <c>false</c>.</returns>
        public bool Equals(ref Float4 other)
        {
            return Mathf.NearEqual(other.X, X) && Mathf.NearEqual(other.Y, Y) && Mathf.NearEqual(other.Z, Z) &&
                   Mathf.NearEqual(other.W, W);
        }

        /// <summary>
        /// Determines whether the specified <see cref="Float4" /> is equal to this instance.
        /// </summary>
        /// <param name="other">The <see cref="Float4" /> to compare with this instance.</param>
        /// <returns><c>true</c> if the specified <see cref="Float4" /> is equal to this instance; otherwise, <c>false</c>.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public bool Equals(Float4 other)
        {
            return Mathf.NearEqual(other.X, X) && Mathf.NearEqual(other.Y, Y) && Mathf.NearEqual(other.Z, Z) &&
                   Mathf.NearEqual(other.W, W);
        }

        /// <summary>
        /// Determines whether the specified <see cref="System.Object" /> is equal to this instance.
        /// </summary>
        /// <param name="value">The <see cref="System.Object" /> to compare with this instance.</param>
        /// <returns><c>true</c> if the specified <see cref="System.Object" /> is equal to this instance; otherwise, <c>false</c>.</returns>
        public override bool Equals(object value)
        {
            return value is Float4 other && Mathf.NearEqual(other.X, X) && Mathf.NearEqual(other.Y, Y) &&
                   Mathf.NearEqual(other.Z, Z) && Mathf.NearEqual(other.W, W);
        }
    }
}
