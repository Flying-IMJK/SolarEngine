// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

// -----------------------------------------------------------------------------
// Original code from SharpDX project. https://github.com/sharpdx/SharpDX/
// Greetings to Alexandre Mutel. Original code published with the following license:
// -----------------------------------------------------------------------------
//
// Copyright (c) 2010-2011 SharpDX - Alexandre Mutel
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
using System.Globalization;
using System.Runtime.InteropServices;

namespace SE
{
    /// <summary>
    /// A half precision (16 bit) floating point value.
    /// </summary>
    [Serializable]
    [StructLayout(LayoutKind.Sequential, Pack = 2)]
    public struct Half
    {
        private ushort value;

        /// <summary>
        /// Number of decimal digits of precision.
        /// </summary>
        public const int PrecisionDigits = 3;

        /// <summary>
        /// Number of bits in the mantissa.
        /// </summary>
        public const int MantissaBits = 11;

        /// <summary>
        /// Maximum decimal exponent.
        /// </summary>
        public const int MaximumDecimalExponent = 4;

        /// <summary>
        /// Maximum binary exponent.
        /// </summary>
        public const int MaximumBinaryExponent = 15;

        /// <summary>
        /// Minimum decimal exponent.
        /// </summary>
        public const int MinimumDecimalExponent = -4;

        /// <summary>
        /// Minimum binary exponent.
        /// </summary>
        public const int MinimumBinaryExponent = -14;

        /// <summary>
        /// Exponent radix.
        /// </summary>
        public const int ExponentRadix = 2;

        /// <summary>
        /// Additional rounding.
        /// </summary>
        public const int AdditionRounding = 1;

        /// <summary>
        /// Smallest such that 1.0 + epsilon != 1.0
        /// </summary>
        public static readonly float Epsilon;

        /// <summary>
        /// Maximum value of the number.
        /// </summary>
        public static readonly float MaxValue;

        /// <summary>
        /// Minimum value of the number.
        /// </summary>
        public static readonly float MinValue;

        /// <summary>
        /// A <see cref="Half"/> whose value is 0.0f.
        /// </summary>
        public static readonly Half Zero;

        /// <summary>
        /// A <see cref="Half"/> whose value is 1.0f.
        /// </summary>
        public static readonly Half One;

        /// <summary>
        /// Initializes a new instance of the <see cref = "T:SE.Half" /> structure.
        /// </summary>
        /// <param name = "value">The floating point value that should be stored in 16 bit format.</param>
        public Half(float value)
        {
            this.value = HalfUtils.Pack(value);
        }

        /// <summary>
        /// Gets or sets the raw 16 bit value used to back this half-float.
        /// </summary>
        public ushort RawValue
        {
            get => value;
            set => this.value = value;
        }

        /// <summary>
        /// Converts an array of half precision values into full precision values.
        /// </summary>
        /// <param name = "values">The values to be converted.</param>
        /// <returns>An array of converted values.</returns>
        public static float[] ConvertToFloat(Half[] values)
        {
            float[] results = new float[values.Length];
            for (int i = 0; i < results.Length; i++)
                results[i] = HalfUtils.Unpack(values[i].RawValue);
            return results;
        }

        /// <summary>
        /// Converts an array of full precision values into half precision values.
        /// </summary>
        /// <param name = "values">The values to be converted.</param>
        /// <returns>An array of converted values.</returns>
        public static Half[] ConvertToHalf(float[] values)
        {
            Half[] results = new Half[values.Length];
            for (int i = 0; i < results.Length; i++)
                results[i] = new Half(values[i]);
            return results;
        }

        /// <summary>
        /// Performs an explicit conversion from <see cref = "T:System.Single" /> to <see cref = "T:SE.Half" />.
        /// </summary>
        /// <param name = "value">The value to be converted.</param>
        /// <returns>The converted value.</returns>
        public static explicit operator Half(float value)
        {
            return new Half(value);
        }

        /// <summary>
        /// Performs an implicit conversion from <see cref = "T:SE.Half" /> to <see cref = "T:System.Single" />.
        /// </summary>
        /// <param name = "value">The value to be converted.</param>
        /// <returns>The converted value.</returns>
        public static implicit operator float(Half value)
        {
            return HalfUtils.Unpack(value.value);
        }

        /// <summary>
        /// Tests for equality between two objects.
        /// </summary>
        /// <param name = "left">The first value to compare.</param>
        /// <param name = "right">The second value to compare.</param>
        /// <returns><c>true</c> if <paramref name = "left" /> has the same value as <paramref name = "right" />; otherwise, <c>false</c>.</returns>
        public static bool operator ==(Half left, Half right)
        {
            return left.value == right.value;
        }

        /// <summary>
        /// Tests for inequality between two objects.
        /// </summary>
        /// <param name = "left">The first value to compare.</param>
        /// <param name = "right">The second value to compare.</param>
        /// <returns><c>true</c> if <paramref name = "left" /> has a different value than <paramref name = "right" />; otherwise, <c>false</c>.</returns>
        public static bool operator !=(Half left, Half right)
        {
            return left.value != right.value;
        }

        /// <summary>
        /// Converts the value of the object to its equivalent string representation.
        /// </summary>
        /// <returns>The string representation of the value of this instance.</returns>
        public override string ToString()
        {
            float num = this;
            return num.ToString(CultureInfo.CurrentCulture);
        }

        /// <summary>
        /// Returns the hash code for this instance.
        /// </summary>
        /// <returns>A 32-bit signed integer hash code.</returns>
        public override int GetHashCode()
        {
            ushort num = value;
            return (((num * 3) / 2) ^ num);
        }

        /// <summary>
        /// Determines whether the specified object instances are considered equal.
        /// </summary>
        /// <param name = "value1" />
        /// <param name = "value2" />
        /// <returns><c>true</c> if <paramref name = "value1" /> is the same instance as <paramref name = "value2" /> or  if both are <c>null</c> references or if <c>value1.Equals(value2)</c> returns <c>true</c>; otherwise, <c>false</c>.</returns>
        public static bool Equals(ref Half value1, ref Half value2)
        {
            return value1.value == value2.value;
        }

        /// <summary>
        /// Returns a value that indicates whether the current instance is equal to the specified object.
        /// </summary>
        /// <param name = "other">Object to make the comparison with.</param>
        /// <returns><c>true</c> if the current instance is equal to the specified object; <c>false</c> otherwise.</returns>
        public bool Equals(Half other)
        {
            return other.value == value;
        }

        /// <summary>
        /// Returns a value that indicates whether the current instance is equal to a specified object.
        /// </summary>
        /// <param name = "obj">Object to make the comparison with.</param>
        /// <returns><c>true</c> if the current instance is equal to the specified object; <c>false</c> otherwise.</returns>
        public override bool Equals(object obj)
        {
            if (obj == null)
            {
                return false;
            }

            if (obj.GetType() != base.GetType())
            {
                return false;
            }

            Half half = (Half)obj;
            return half.value == value;
        }

        static Half()
        {
            Epsilon = 0.0004887581f;
            MaxValue = 65504f;
            MinValue = 6.103516E-05f;
            Zero = (Half)0.0f;
            One = (Half)1.0f;
        }
    }

    /// <summary>
    /// Defines a two component vector, using half precision floating point coordinates.
    /// </summary>
    [Serializable]
    [StructLayout(LayoutKind.Sequential, Pack = 2)]
    public struct Half2 : IEquatable<Half2>
    {
        /// <summary>
        /// Gets or sets the X component of the vector.
        /// </summary>
        /// <value>The X component of the vector.</value>
        public Half X;

        /// <summary>
        /// Gets or sets the Y component of the vector.
        /// </summary>
        /// <value>The Y component of the vector.</value>
        public Half Y;

        /// <summary>
        /// Initializes a new instance of the <see cref="T:SE.Half2" /> structure.
        /// </summary>
        /// <param name="x">The X component.</param>
        /// <param name="y">The Y component.</param>
        public Half2(Half x, Half y)
        {
            X = x;
            Y = y;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="T:SE.Half2" /> structure.
        /// </summary>
        /// <param name="value">The value to set for both the X and Y components.</param>
        public Half2(Half value)
        {
            X = value;
            Y = value;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="T:SE.Half2" /> structure.
        /// </summary>
        /// <param name="x">The X component.</param>
        /// <param name="y">The Y component.</param>
        public Half2(float x, float y)
        {
            X = (Half)x;
            Y = (Half)y;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="T:SE.Half2" /> structure.
        /// </summary>
        /// <param name="value">The value to set for both the X and Y components.</param>
        public Half2(float value)
        {
            X = (Half)value;
            Y = (Half)value;
        }

        /// <summary>
        /// Tests for equality between two objects.
        /// </summary>
        /// <param name="left">The first value to compare.</param>
        /// <param name="right">The second value to compare.</param>
        /// <returns>
        /// <c>true</c> if <paramref name="left" /> has the same value as <paramref name="right" />; otherwise, <c>false</c>.</returns>
        public static bool operator ==(Half2 left, Half2 right)
        {
            return Equals(ref left, ref right);
        }

        /// <summary>
        /// Tests for inequality between two objects.
        /// </summary>
        /// <param name="left">The first value to compare.</param>
        /// <param name="right">The second value to compare.</param>
        /// <returns>
        /// <c>true</c> if <paramref name="left" /> has a different value than <paramref name="right" />; otherwise, <c>false</c>.</returns>
        [return: MarshalAs(UnmanagedType.U1)]
        public static bool operator !=(Half2 left, Half2 right)
        {
            return !Equals(ref left, ref right);
        }

        /// <summary>
        /// Returns the hash code for this instance.
        /// </summary>
        /// <returns>A 32-bit signed integer hash code.</returns>
        public override int GetHashCode()
        {
            return (Y.GetHashCode() + X.GetHashCode());
        }

        /// <summary>
        /// Determines whether the specified object instances are considered equal. 
        /// </summary>
        /// <param name="value1" />
        /// <param name="value2" />
        /// <returns>
        /// <c>true</c> if <paramref name="value1" /> is the same instance as <paramref name="value2" /> or 
        /// if both are <c>null</c> references or if <c>value1.Equals(value2)</c> returns <c>true</c>; otherwise, <c>false</c>.</returns>
        public static bool Equals(ref Half2 value1, ref Half2 value2)
        {
            return ((value1.X == value2.X) && (value1.Y == value2.Y));
        }

        /// <summary>
        /// Returns a value that indicates whether the current instance is equal to the specified object. 
        /// </summary>
        /// <param name="other">Object to make the comparison with.</param>
        /// <returns>
        /// <c>true</c> if the current instance is equal to the specified object; <c>false</c> otherwise.</returns>
        public bool Equals(Half2 other)
        {
            return ((X == other.X) && (Y == other.Y));
        }

        /// <summary>
        /// Returns a value that indicates whether the current instance is equal to a specified object. 
        /// </summary>
        /// <param name="obj">Object to make the comparison with.</param>
        /// <returns>
        /// <c>true</c> if the current instance is equal to the specified object; <c>false</c> otherwise.</returns>
        public override bool Equals(object obj)
        {
            if (obj == null)
            {
                return false;
            }

            if (obj.GetType() != GetType())
            {
                return false;
            }

            return Equals((Half2)obj);
        }

        /// <summary>
        /// Returns a <see cref="System.String" /> that represents this instance.
        /// </summary>
        /// <returns>A <see cref="System.String" /> that represents this instance.</returns>
        public override string ToString()
        {
            return ((Float2)this).ToString();
        }

        /// <summary>
        /// Performs an explicit conversion from <see cref="Float2"/> to <see cref="Half3"/>.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <returns>The result of the conversion.</returns>
        public static explicit operator Half2(Float2 value)
        {
            return new Half2((Half)value.X, (Half)value.Y);
        }

        /// <summary>
        /// Performs an explicit conversion from <see cref="Float2"/> to <see cref="Half3"/>.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <returns>The result of the conversion.</returns>
        public static explicit operator Float2(Half2 value)
        {
            return new Float2(value.X, value.Y);
        }
    }

    /// <summary>
    /// Defines a three component vector, using half precision floating point coordinates.
    /// </summary>
    [Serializable]
    [StructLayout(LayoutKind.Sequential, Pack = 2)]
    public struct Half3 : IEquatable<Half3>
    {
        /// <summary>
        /// Gets or sets the X component of the vector.
        /// </summary>
        /// <value>The X component of the vector.</value>
        public Half X;

        /// <summary>
        /// Gets or sets the Y component of the vector.
        /// </summary>
        /// <value>The Y component of the vector.</value>
        public Half Y;

        /// <summary>
        /// Gets or sets the Z component of the vector.
        /// </summary>
        /// <value>The Z component of the vector.</value>
        public Half Z;

        /// <summary>
        /// Initializes a new instance of the <see cref="T:SE.Half3" /> structure.
        /// </summary>
        /// <param name="x">The X component.</param>
        /// <param name="y">The Y component.</param>
        /// <param name="z">The Z component.</param>
        public Half3(Half x, Half y, Half z)
        {
            X = x;
            Y = y;
            Z = z;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="T:SE.Half3" /> structure.
        /// </summary>
        /// <param name="value">The value to set for the X, Y, and Z components.</param>
        public Half3(Half value)
        {
            X = value;
            Y = value;
            Z = value;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="T:SE.Half3" /> structure.
        /// </summary>
        /// <param name="x">The X component.</param>
        /// <param name="y">The Y component.</param>
        /// <param name="z">The Z component.</param>
        public Half3(float x, float y, float z)
        {
            X = (Half)x;
            Y = (Half)y;
            Z = (Half)z;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="T:SE.Half3" /> structure.
        /// </summary>
        /// <param name="value">The value to set for the X, Y, and Z components.</param>
        public Half3(float value)
        {
            X = (Half)value;
            Y = (Half)value;
            Z = (Half)value;
        }

        /// <summary>
        /// Tests for equality between two objects.
        /// </summary>
        /// <param name="left">The first value to compare.</param>
        /// <param name="right">The second value to compare.</param>
        /// <returns>
        /// <c>true</c> if <paramref name="left" /> has the same value as <paramref name="right" />; otherwise, <c>false</c>.</returns>
        public static bool operator ==(Half3 left, Half3 right)
        {
            return Equals(ref left, ref right);
        }

        /// <summary>
        /// Tests for inequality between two objects.
        /// </summary>
        /// <param name="left">The first value to compare.</param>
        /// <param name="right">The second value to compare.</param>
        /// <returns>
        /// <c>true</c> if <paramref name="left" /> has a different value than <paramref name="right" />; otherwise, <c>false</c>.</returns>
        [return: MarshalAs(UnmanagedType.U1)]
        public static bool operator !=(Half3 left, Half3 right)
        {
            return !Equals(ref left, ref right);
        }

        /// <summary>
        /// Returns the hash code for this instance.
        /// </summary>
        /// <returns>A 32-bit signed integer hash code.</returns>
        public override int GetHashCode()
        {
            int num = Z.GetHashCode() + Y.GetHashCode();
            return (X.GetHashCode() + num);
        }

        /// <summary>
        /// Determines whether the specified object instances are considered equal. 
        /// </summary>
        /// <param name="value1" />
        /// <param name="value2" />
        /// <returns>
        /// <c>true</c> if <paramref name="value1" /> is the same instance as <paramref name="value2" /> or 
        /// if both are <c>null</c> references or if <c>value1.Equals(value2)</c> returns <c>true</c>; otherwise, <c>false</c>.</returns>
        public static bool Equals(ref Half3 value1, ref Half3 value2)
        {
            return (((value1.X == value2.X) && (value1.Y == value2.Y)) && (value1.Z == value2.Z));
        }

        /// <summary>
        /// Returns a value that indicates whether the current instance is equal to the specified object. 
        /// </summary>
        /// <param name="other">Object to make the comparison with.</param>
        /// <returns>
        /// <c>true</c> if the current instance is equal to the specified object; <c>false</c> otherwise.</returns>
        public bool Equals(Half3 other)
        {
            return (((X == other.X) && (Y == other.Y)) && (Z == other.Z));
        }

        /// <summary>
        /// Performs an explicit conversion from <see cref="Float3"/> to <see cref="Half3"/>.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <returns>The result of the conversion.</returns>
        public static explicit operator Half3(Float3 value)
        {
            return new Half3((Half)value.X, (Half)value.Y, (Half)value.Z);
        }

        /// <summary>
        /// Performs an explicit conversion from <see cref="Float3"/> to <see cref="Half3"/>.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <returns>The result of the conversion.</returns>
        public static explicit operator Float3(Half3 value)
        {
            return new Float3(value.X, value.Y, value.Z);
        }

        /// <summary>
        /// Returns a value that indicates whether the current instance is equal to a specified object. 
        /// </summary>
        /// <param name="obj">Object to make the comparison with.</param>
        /// <returns>
        /// <c>true</c> if the current instance is equal to the specified object; <c>false</c> otherwise.</returns>
        public override bool Equals(object obj)
        {
            if (obj == null)
            {
                return false;
            }

            if (obj.GetType() != GetType())
            {
                return false;
            }

            return Equals((Half3)obj);
        }

        /// <summary>
        /// Returns a <see cref="System.String" /> that represents this instance.
        /// </summary>
        /// <returns>A <see cref="System.String" /> that represents this instance.</returns>
        public override string ToString()
        {
            return ((Float3)this).ToString();
        }
    }

    /// <summary>
    /// Defines a four component vector, using half precision floating point coordinates.
    /// </summary>
    [Serializable]
    [StructLayout(LayoutKind.Sequential, Pack = 2)]
    public struct Half4 : IEquatable<Half4>
    {
        /// <summary>
        /// Gets or sets the X component of the vector.
        /// </summary>
        /// <value>The X component of the vector.</value>
        public Half X;

        /// <summary>
        /// Gets or sets the Y component of the vector.
        /// </summary>
        /// <value>The Y component of the vector.</value>
        public Half Y;

        /// <summary>
        /// Gets or sets the Z component of the vector.
        /// </summary>
        /// <value>The Z component of the vector.</value>
        public Half Z;

        /// <summary>
        /// Gets or sets the W component of the vector.
        /// </summary>
        /// <value>The W component of the vector.</value>
        public Half W;

        /// <summary>
        /// Initializes a new instance of the <see cref="T:SE.Half4" /> structure.
        /// </summary>
        /// <param name="x">The X component.</param>
        /// <param name="y">The Y component.</param>
        /// <param name="z">The Z component.</param>
        /// <param name="w">The W component.</param>
        public Half4(Half x, Half y, Half z, Half w)
        {
            X = x;
            Y = y;
            Z = z;
            W = w;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="T:SE.Half4" /> structure.
        /// </summary>
        /// <param name="value">The value to set for the X, Y, Z, and W components.</param>
        public Half4(Half value)
        {
            X = value;
            Y = value;
            Z = value;
            W = value;
        }

        /// <summary>
        /// Tests for equality between two objects.
        /// </summary>
        /// <param name="left">The first value to compare.</param>
        /// <param name="right">The second value to compare.</param>
        /// <returns>
        /// <c>true</c> if <paramref name="left" /> has the same value as <paramref name="right" />; otherwise, <c>false</c>.</returns>
        public static bool operator ==(Half4 left, Half4 right)
        {
            return Equals(ref left, ref right);
        }

        /// <summary>
        /// Tests for inequality between two objects.
        /// </summary>
        /// <param name="left">The first value to compare.</param>
        /// <param name="right">The second value to compare.</param>
        /// <returns>
        /// <c>true</c> if <paramref name="left" /> has a different value than <paramref name="right" />; otherwise, <c>false</c>.</returns>
        public static bool operator !=(Half4 left, Half4 right)
        {
            return !Equals(ref left, ref right);
        }

        /// <summary>
        /// Returns the hash code for this instance.
        /// </summary>
        /// <returns>A 32-bit signed integer hash code.</returns>
        public override int GetHashCode()
        {
            int num2 = W.GetHashCode() + Z.GetHashCode();
            int num = Y.GetHashCode() + num2;
            return (X.GetHashCode() + num);
        }

        /// <summary>
        /// Determines whether the specified object instances are considered equal. 
        /// </summary>
        /// <param name="value1" />
        /// <param name="value2" />
        /// <returns>
        /// <c>true</c> if <paramref name="value1" /> is the same instance as <paramref name="value2" /> or 
        /// if both are <c>null</c> references or if <c>value1.Equals(value2)</c> returns <c>true</c>; otherwise, <c>false</c>.</returns>
        public static bool Equals(ref Half4 value1, ref Half4 value2)
        {
            return (((value1.X == value2.X) && (value1.Y == value2.Y)) &&
                    ((value1.Z == value2.Z) && (value1.W == value2.W)));
        }

        /// <summary>
        /// Returns a value that indicates whether the current instance is equal to the specified object. 
        /// </summary>
        /// <param name="other">Object to make the comparison with.</param>
        /// <returns>
        /// <c>true</c> if the current instance is equal to the specified object; <c>false</c> otherwise.</returns>
        public bool Equals(Half4 other)
        {
            return (((X == other.X) && (Y == other.Y)) && ((Z == other.Z) && (W == other.W)));
        }

        /// <summary>
        /// Performs an explicit conversion from <see cref="Float4"/> to <see cref="Half4"/>.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <returns>The result of the conversion.</returns>
        public static explicit operator Half4(Float4 value)
        {
            return new Half4((Half)value.X, (Half)value.Y, (Half)value.Z, (Half)value.W);
        }

        /// <summary>
        /// Performs an explicit conversion from <see cref="Float4"/> to <see cref="Half4"/>.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <returns>The result of the conversion.</returns>
        public static explicit operator Float4(Half4 value)
        {
            return new Float4(value.X, value.Y, value.Z, value.W);
        }

        /// <summary>
        /// Returns a value that indicates whether the current instance is equal to a specified object. 
        /// </summary>
        /// <param name="obj">Object to make the comparison with.</param>
        /// <returns>
        /// <c>true</c> if the current instance is equal to the specified object; <c>false</c> otherwise.</returns>
        public override bool Equals(object obj)
        {
            if (obj == null)
            {
                return false;
            }

            if (obj.GetType() != GetType())
            {
                return false;
            }

            return Equals((Half4)obj);
        }

        /// <summary>
        /// Returns a <see cref="System.String" /> that represents this instance.
        /// </summary>
        /// <returns>A <see cref="System.String" /> that represents this instance.</returns>
        public override string ToString()
        {
            return ((Float4)this).ToString();
        }
    }
}
