// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;

namespace SE
{
    /// <summary>
    /// A representation of a sphere of values via Spherical Harmonics (SH).
    /// </summary>
    /// <typeparam name="TDataType">The type of data contained by the sphere</typeparam>
    public abstract class SphericalHarmonics<TDataType>
    {
        /// <summary>
        /// The maximum order supported.
        /// </summary>
        public const int MaximumOrder = 5;

        private int m_Order;

        /// <summary>
        /// The order of calculation of the spherical harmonic.
        /// </summary>
        public int Order
        {
            get => m_Order;
            internal set
            {
                if (m_Order > 5)
                    throw new NotSupportedException("Only orders inferior or equal to 5 are supported");

                m_Order = Math.Max(1, value);
            }
        }

        /// <summary>
        /// Get the coefficients defining the spherical harmonics (the spherical coordinates x{l,m} multiplying the spherical base Y{l,m}).
        /// </summary>
        public TDataType[] Coefficients { get; internal set; }

        /// <summary>
        /// Creates a null spherical harmonics (for serialization).
        /// </summary>
        internal SphericalHarmonics()
        {
        }

        /// <summary>
        /// The desired order to
        /// </summary>
        /// <param name="order"></param>
        protected SphericalHarmonics(int order)
        {
            m_Order = order;
            Coefficients = new TDataType[order * order];
        }

        /// <summary>
        /// Evaluate the value of the spherical harmonics in the provided direction.
        /// </summary>
        /// <param name="direction">The direction</param>
        /// <returns>The value of the spherical harmonics in the direction</returns>
        public abstract TDataType Evaluate(Float3 direction);

        /// <summary>
        /// Returns the coefficient x{l,m} of the spherical harmonics (the {l,m} spherical coordinate corresponding to the spherical base Y{l,m}).
        /// </summary>
        /// <param name="l">the l index of the coefficient</param>
        /// <param name="m">the m index of the coefficient</param>
        /// <returns>the value of the coefficient</returns>
        public TDataType this[int l, int m]
        {
            get
            {
                CheckIndicesValidity(l, m, m_Order);
                return Coefficients[LmToCoefficientIndex(l, m)];
            }
            set
            {
                CheckIndicesValidity(l, m, m_Order);
                Coefficients[LmToCoefficientIndex(l, m)] = value;
            }
        }

        // ReSharper disable UnusedParameter.Local
        private static void CheckIndicesValidity(int l, int m, int maxOrder)
        // ReSharper restore UnusedParameter.Local
        {
            if (l > maxOrder - 1)
                throw new IndexOutOfRangeException(string.Format("'l' parameter should be between '0' and '{0}' (order-1).", maxOrder - 1));

            if (Math.Abs(m) > l)
                throw new IndexOutOfRangeException("'m' parameter should be between '-l' and '+l'.");
        }

        private static int LmToCoefficientIndex(int l, int m)
        {
            return l * l + l + m;
        }
    }

    /// <summary>
    /// A spherical harmonics representation of a cubemap.
    /// </summary>
    public class SphericalHarmonics : SphericalHarmonics<Color>
    {
        private readonly float[] m_BaseValues;

        private const float Pi4 = 4 * Mathf.Pi;
        private const float Pi16 = 16 * Mathf.Pi;
        private const float Pi64 = 64 * Mathf.Pi;
        private static readonly float SqrtPi = (float)Math.Sqrt(Mathf.Pi);

        /// <summary>
        /// Base coefficients for SH.
        /// </summary>
        public static readonly float[] BaseCoefficients =
        {
            (float)(1.0 / (2.0 * SqrtPi)),

            (float)(-Math.Sqrt(3.0 / Pi4)),
            (float)(Math.Sqrt(3.0 / Pi4)),
            (float)(-Math.Sqrt(3.0 / Pi4)),

            (float)(Math.Sqrt(15.0 / Pi4)),
            (float)(-Math.Sqrt(15.0 / Pi4)),
            (float)(Math.Sqrt(5.0 / Pi16)),
            (float)(-Math.Sqrt(15.0 / Pi4)),
            (float)(Math.Sqrt(15.0 / Pi16)),

            -(float)Math.Sqrt(70 / Pi64),
            (float)Math.Sqrt(105 / Pi4),
            -(float)Math.Sqrt(42 / Pi64),
            (float)Math.Sqrt(7 / Pi16),
            -(float)Math.Sqrt(42 / Pi64),
            (float)Math.Sqrt(105 / Pi16),
            -(float)Math.Sqrt(70 / Pi64),

            3 * (float)Math.Sqrt(35 / Pi16),
            -3 * (float)Math.Sqrt(70 / Pi64),
            3 * (float)Math.Sqrt(5 / Pi16),
            -3 * (float)Math.Sqrt(10 / Pi64),
            (float)(1.0 / (16.0 * SqrtPi)),
            -3 * (float)Math.Sqrt(10 / Pi64),
            3 * (float)Math.Sqrt(5 / Pi64),
            -3 * (float)Math.Sqrt(70 / Pi64),
            3 * (float)Math.Sqrt(35 / (4 * Pi64)),
        };

        internal SphericalHarmonics()
        {
        }

        /// <summary>
        /// Create a new instance of Spherical Harmonics of provided order.
        /// </summary>
        /// <param name="order">The order of the harmonics</param>
        public SphericalHarmonics(int order)
        : base(order)
        {
            m_BaseValues = new float[order * order];
        }

        /// <summary>
        /// Evaluates the color for the specified direction.
        /// </summary>
        /// <param name="direction">The direction to evaluate.</param>
        /// <returns>The color computed for this direction.</returns>
        public override Color Evaluate(Float3 direction)
        {
            var x = direction.X;
            var y = direction.Y;
            var z = direction.Z;

            var x2 = x * x;
            var y2 = y * y;
            var z2 = z * z;

            var z3 = (float)Math.Pow(z, 3.0);

            var x4 = (float)Math.Pow(x, 4.0);
            var y4 = (float)Math.Pow(y, 4.0);
            var z4 = (float)Math.Pow(z, 4.0);

            //Equations based on data from: http://ppsloan.org/publications/StupidSH36.pdf
            m_BaseValues[0] = 1 / (2 * SqrtPi);

            if (Order > 1)
            {
                m_BaseValues[1] = -(float)Math.Sqrt(3 / Pi4) * y;
                m_BaseValues[2] = (float)Math.Sqrt(3 / Pi4) * z;
                m_BaseValues[3] = -(float)Math.Sqrt(3 / Pi4) * x;

                if (Order > 2)
                {
                    m_BaseValues[4] = (float)Math.Sqrt(15 / Pi4) * y * x;
                    m_BaseValues[5] = -(float)Math.Sqrt(15 / Pi4) * y * z;
                    m_BaseValues[6] = (float)Math.Sqrt(5 / Pi16) * (3 * z2 - 1);
                    m_BaseValues[7] = -(float)Math.Sqrt(15 / Pi4) * x * z;
                    m_BaseValues[8] = (float)Math.Sqrt(15 / Pi16) * (x2 - y2);

                    if (Order > 3)
                    {
                        m_BaseValues[9] = -(float)Math.Sqrt(70 / Pi64) * y * (3 * x2 - y2);
                        m_BaseValues[10] = (float)Math.Sqrt(105 / Pi4) * y * x * z;
                        m_BaseValues[11] = -(float)Math.Sqrt(42 / Pi64) * y * (-1 + 5 * z2);
                        m_BaseValues[12] = (float)Math.Sqrt(7 / Pi16) * (5 * z3 - 3 * z);
                        m_BaseValues[13] = -(float)Math.Sqrt(42 / Pi64) * x * (-1 + 5 * z2);
                        m_BaseValues[14] = (float)Math.Sqrt(105 / Pi16) * (x2 - y2) * z;
                        m_BaseValues[15] = -(float)Math.Sqrt(70 / Pi64) * x * (x2 - 3 * y2);

                        if (Order > 4)
                        {
                            m_BaseValues[16] = 3 * (float)Math.Sqrt(35 / Pi16) * x * y * (x2 - y2);
                            m_BaseValues[17] = -3 * (float)Math.Sqrt(70 / Pi64) * y * z * (3 * x2 - y2);
                            m_BaseValues[18] = 3 * (float)Math.Sqrt(5 / Pi16) * y * x * (-1 + 7 * z2);
                            m_BaseValues[19] = -3 * (float)Math.Sqrt(10 / Pi64) * y * z * (-3 + 7 * z2);
                            m_BaseValues[20] = (105 * z4 - 90 * z2 + 9) / (16 * SqrtPi);
                            m_BaseValues[21] = -3 * (float)Math.Sqrt(10 / Pi64) * x * z * (-3 + 7 * z2);
                            m_BaseValues[22] = 3 * (float)Math.Sqrt(5 / Pi64) * (x2 - y2) * (-1 + 7 * z2);
                            m_BaseValues[23] = -3 * (float)Math.Sqrt(70 / Pi64) * x * z * (x2 - 3 * y2);
                            m_BaseValues[24] = 3 * (float)Math.Sqrt(35 / (4 * Pi64)) * (x4 - 6 * y2 * x2 + y4);
                        }
                    }
                }
            }

            var data = new Color();

            for (int i = 0; i < m_BaseValues.Length; i++)
                data += Coefficients[i] * m_BaseValues[i];

            return data;
        }
    }
}
