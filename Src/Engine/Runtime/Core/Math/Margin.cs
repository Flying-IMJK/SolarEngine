using System;
using System.Globalization;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace SE
{
    /// <summary>
    /// Describes margins around a rectangle.
    /// </summary>
    [Serializable]
    [StructLayout(LayoutKind.Sequential, Pack = 4)]
    public struct Margin : IEquatable<Margin>, IFormattable
    {
        private static readonly string _formatString = "Left:{0:F2} Right:{1:F2} Top:{2:F2} Bottom:{3:F2}";

        public static readonly int SizeInBytes = Marshal.SizeOf(typeof(Margin));
        public static readonly Margin Zero;

        public float Left;
        public float Right;
        public float Top;
        public float Bottom;

        public Float2 Location => new Float2(Left, Top);
        public Float2 Size => new Float2(Left + Right, Top + Bottom);
        public float Width => Left + Right;
        public float Height => Top + Bottom;
        public bool IsZero => Mathf.IsZero(Left) && Mathf.IsZero(Right) && Mathf.IsZero(Top) && Mathf.IsZero(Bottom);

        public Margin(float value)
        {
            Left = value;
            Right = value;
            Top = value;
            Bottom = value;
        }

        public Margin(float left, float right, float top, float bottom)
        {
            Left = left;
            Right = right;
            Top = top;
            Bottom = bottom;
        }

        public void ShrinkRectangle(ref Rectangle rect)
        {
            rect.Location.X += Left;
            rect.Location.Y += Top;
            rect.Size.X -= Left + Right;
            rect.Size.Y -= Top + Bottom;
        }

        public void ExpandRectangle(ref Rectangle rect)
        {
            rect.Location.X -= Left;
            rect.Location.Y -= Top;
            rect.Size.X += Left + Right;
            rect.Size.Y += Top + Bottom;
        }

        public static Margin operator +(Margin left, Margin right)
        {
            return new Margin(left.Left + right.Left, left.Right + right.Right, left.Top + right.Top, left.Bottom + right.Bottom);
        }

        public static Margin operator -(Margin left, Margin right)
        {
            return new Margin(left.Left - right.Left, left.Right - right.Right, left.Top - right.Top, left.Bottom - right.Bottom);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool operator ==(Margin left, Margin right)
        {
            return left.Equals(ref right);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool operator !=(Margin left, Margin right)
        {
            return !left.Equals(ref right);
        }

        public override string ToString()
        {
            return string.Format(CultureInfo.CurrentCulture, "Left:{0} Right:{1} Top:{2} Bottom:{3}", Left, Right, Top, Bottom);
        }

        public string ToString(string format)
        {
            if (format == null)
                return ToString();
            return string.Format(CultureInfo.CurrentCulture, _formatString,
                Left.ToString(format, CultureInfo.CurrentCulture),
                Right.ToString(format, CultureInfo.CurrentCulture),
                Top.ToString(format, CultureInfo.CurrentCulture),
                Bottom.ToString(format, CultureInfo.CurrentCulture));
        }

        public string ToString(IFormatProvider formatProvider)
        {
            return string.Format(formatProvider, _formatString, Left, Right, Top, Bottom);
        }

        public string ToString(string format, IFormatProvider formatProvider)
        {
            if (format == null)
                return ToString(formatProvider);
            return string.Format(formatProvider, _formatString,
                Left.ToString(format, formatProvider),
                Right.ToString(format, formatProvider),
                Top.ToString(format, formatProvider),
                Bottom.ToString(format, formatProvider));
        }

        public override int GetHashCode()
        {
            unchecked
            {
                int hashCode = Left.GetHashCode();
                hashCode = (hashCode * 397) ^ Right.GetHashCode();
                hashCode = (hashCode * 397) ^ Top.GetHashCode();
                hashCode = (hashCode * 397) ^ Bottom.GetHashCode();
                return hashCode;
            }
        }

        public bool Equals(ref Margin other)
        {
            return Mathf.NearEqual(other.Left, Left) &&
                   Mathf.NearEqual(other.Right, Right) &&
                   Mathf.NearEqual(other.Top, Top) &&
                   Mathf.NearEqual(other.Bottom, Bottom);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool Equals(ref Margin a, ref Margin b)
        {
            return a.Equals(ref b);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public bool Equals(Margin other)
        {
            return Equals(ref other);
        }

        public override bool Equals(object value)
        {
            return value is Margin margin && Equals(ref margin);
        }
    }
}
