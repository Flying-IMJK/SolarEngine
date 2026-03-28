#pragma once

//
#include "SIMD.h"
#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"

//-------------------------------------------------------------------------
// This math library is heavily based on the DirectX math library
//-------------------------------------------------------------------------
// https://github.com/Microsoft/DirectXMath
//-------------------------------------------------------------------------

namespace SE
{
    /*class SE_API_CORE alignas(16) VectorSIMD
    {
    public:
        static VectorSIMD const UnitX;
        static VectorSIMD const UnitY;
        static VectorSIMD const UnitZ;
        static VectorSIMD const UnitW;

        static VectorSIMD const Origin;
        static VectorSIMD const WorldForward;
        static VectorSIMD const WorldBackward;
        static VectorSIMD const WorldUp;
        static VectorSIMD const WorldDown;
        static VectorSIMD const WorldLeft;
        static VectorSIMD const WorldRight;

        static VectorSIMD const NegativeOne;
        static VectorSIMD const Zero;
        static VectorSIMD const Half;
        static VectorSIMD const One;
        static VectorSIMD const Epsilon;
        static VectorSIMD const LargeEpsilon;
        static VectorSIMD const OneMinusEpsilon;
        static VectorSIMD const EpsilonMinusOne;
        static VectorSIMD const NormalizeCheckThreshold;
        static VectorSIMD const Pi;
        static VectorSIMD const PiDivTwo;
        static VectorSIMD const TwoPi;
        static VectorSIMD const OneDivTwoPi;

        static VectorSIMD const Select0000;
        static VectorSIMD const Select0001;
        static VectorSIMD const Select0010;
        static VectorSIMD const Select0011;
        static VectorSIMD const Select0100;
        static VectorSIMD const Select0101;
        static VectorSIMD const Select0110;
        static VectorSIMD const Select0111;
        static VectorSIMD const Select1000;
        static VectorSIMD const Select1001;
        static VectorSIMD const Select1010;
        static VectorSIMD const Select1011;
        static VectorSIMD const Select1100;
        static VectorSIMD const Select1101;
        static VectorSIMD const Select1110;
        static VectorSIMD const Select1111;

        static VectorSIMD const Infinity;
        static VectorSIMD const QNaN;

        static VectorSIMD const BoxCorners[8];

        // Utils
        //-------------------------------------------------------------------------

        inline static VectorSIMD Cross2(VectorSIMD const &v0, VectorSIMD const &v1) { return v0.Cross2(v1); }
        inline static VectorSIMD Cross3(VectorSIMD const &v0, VectorSIMD const &v1) { return v0.Cross3(v1); }
        inline static VectorSIMD Dot2(VectorSIMD const &v0, VectorSIMD const &v1) { return v0.Dot2(v1); }
        inline static VectorSIMD Dot3(VectorSIMD const &v0, VectorSIMD const &v1) { return v0.Dot3(v1); }
        inline static VectorSIMD Dot4(VectorSIMD const &v0, VectorSIMD const &v1) { return v0.Dot4(v1); }
        inline static VectorSIMD Average2(VectorSIMD const &v0, VectorSIMD const &v1);
        inline static VectorSIMD Average3(VectorSIMD const &v0, VectorSIMD const &v1);
        inline static VectorSIMD Average4(VectorSIMD const &v0, VectorSIMD const &v1);
        inline static VectorSIMD Min(VectorSIMD const &v0, VectorSIMD const &v1);
        inline static VectorSIMD Max(VectorSIMD const &v0, VectorSIMD const &v1);
        inline static VectorSIMD Clamp(VectorSIMD const &v, VectorSIMD const &min, VectorSIMD const &max);
        inline static VectorSIMD Xor(VectorSIMD const &vec0, VectorSIMD const &vec1);

        //-------------------------------------------------------------------------
        // Copied from DirectX Math
        static bool SolveCubic(float e, float f, float g, float &t, float &u, float &v);

        static VectorSIMD CalculateEigenVector(float m11, float m12, float m13, float m22, float m23, float m33, float e);

        static bool CalculateEigenVectors(float m11, float m12, float m13, float m22, float m23, float m33, float e1, float e2, float e3, VectorSIMD &V1, VectorSIMD &V2, VectorSIMD &V3) noexcept;

        static bool CalculateEigenVectorsFromCovarianceMatrix(float Cxx, float Cyy, float Czz, float Cxy, float Cxz, float Cyz, VectorSIMD &V1, VectorSIMD &V2, VectorSIMD &V3);

        //-------------------------------------------------------------------------
        // Add the multiplied results to a VectorSIMD: ( vec * mul ) + addend
        static VectorSIMD MultiplyAdd(VectorSIMD const &vec, VectorSIMD const &multiplier, VectorSIMD const &addend);

        // Subtract a VectorSIMD from the multiplied result: (vec * mul ) - subtrahend
		static VectorSIMD MultiplySubtract(VectorSIMD const &vec, VectorSIMD const &multiplier, VectorSIMD const &subtrahend);

        // Subtract the multiplied result from a VectorSIMD: minuend - (vec * mul )
		static VectorSIMD NegativeMultiplySubtract(VectorSIMD const &vec, VectorSIMD const &multiplier, VectorSIMD const &minuend);

        // Sum up scaled versions of two vectors
		inline static VectorSIMD LinearCombination(VectorSIMD const &v0, VectorSIMD const &v1, float scale0, float scale1) { return (v0 * scale0) + (v1 * scale1); }

        // Linear interpolation of one VectorSIMD to another
		static VectorSIMD Lerp(VectorSIMD const &from, VectorSIMD const &to, float t);

        // Normalized linear interpolation of one VectorSIMD to another
		static VectorSIMD NLerp(VectorSIMD const &from, VectorSIMD const &to, float t);

        // Spherical interpolation of one VectorSIMD to another
        static VectorSIMD SLerp(VectorSIMD const &from, VectorSIMD const &to, float t);

        // Combine the two vectors based on the control: 0 means select from v0, 1 means select from v1. E.G. To select XY from v0 and ZW from v1, control = VectorSIMD( 0, 0, 1, 1 )
		static VectorSIMD Select(VectorSIMD const &v0, VectorSIMD const &v1, VectorSIMD const &control);

        // Get a permutation of two vectors, each template argument represents the element index to select ( v0: 0-3, v1: 4-7 );
        template <uint32 PermuteX, uint32 PermuteY, uint32 PermuteZ, uint32 PermuteW>
        inline static VectorSIMD Permute(VectorSIMD const &v0, VectorSIMD const &v1);

        // Trigonometry
        //-------------------------------------------------------------------------

		static VectorSIMD Sin(VectorSIMD const &vec);
		static VectorSIMD Cos(VectorSIMD const &vec);
		static VectorSIMD Tan(VectorSIMD const &vec);
        static VectorSIMD ASin(VectorSIMD const &vec);
        static VectorSIMD ACos(VectorSIMD const &vec);
        static VectorSIMD ATan(VectorSIMD const &vec);
        static VectorSIMD ATan2(VectorSIMD const &vec0, VectorSIMD const &vec1);

        static VectorSIMD SinEst(VectorSIMD const &vec);
        static VectorSIMD CosEst(VectorSIMD const &vec);
        static VectorSIMD TanEst(VectorSIMD const &vec);
        static VectorSIMD ASinEst(VectorSIMD const &vec);
        static VectorSIMD ACosEst(VectorSIMD const &vec);
        static VectorSIMD ATanEst(VectorSIMD const &vec);
        static VectorSIMD ATan2Est(VectorSIMD const &vec0, VectorSIMD const &vec1);

        inline static void SinCos(VectorSIMD &sin, VectorSIMD &cos, float angle) { return SinCos(sin, cos, VectorSIMD(angle)); }
		static void SinCos(VectorSIMD &sin, VectorSIMD &cos, VectorSIMD const &angle);

		static VectorSIMD AngleMod2Pi(VectorSIMD const &angles);

    public:
        inline operator __m128 &() { return data; }
        inline operator __m128 const &() const { return data; }

        inline VectorSIMD() {}
        inline explicit VectorSIMD(Axis axis);
        inline explicit VectorSIMD(ZeroInit_t) { memset(this, 0, sizeof(VectorSIMD)); }
        inline explicit VectorSIMD(float v) { data = _mm_set1_ps(v); }
        inline VectorSIMD(__m128 v) : data(v) {}
        inline VectorSIMD(float ix, float iy, float iz, float iw = 1.0f) { data = _mm_set_ps(iw, iz, iy, ix); }

        inline VectorSIMD(Float2 const &v, float iz = 0.0f, float iw = 0.0f) { data = _mm_set_ps(iw, iz, v.y, v.x); }
        inline VectorSIMD(Float3 const &v, float iw = 1.0f) { data = _mm_set_ps(iw, v.z, v.y, v.x); } // Default behavior: create points (m_w=1)
        inline VectorSIMD(Float4 const &v) { data = _mm_loadu_ps(&v.x); }
        inline VectorSIMD(float const *pValues) { data = _mm_loadu_ps(pValues); }

        //-------------------------------------------------------------------------

        inline bool IsValid() const { return !IsNaN4() && !IsInfinite4(); }

        //-------------------------------------------------------------------------

        inline void Store(float *pValues) const {  _mm_storeu_ps(pValues, data); }
        inline void StoreFloat(float &value) const { _mm_store_ss(&value, data); }
        inline void StoreFloat2(Float2 &value) const
		{
			auto yVec = _mm_shuffle_ps(data, data, _MM_SHUFFLE(1, 1, 1, 1));
			_mm_store_ss(&value.x, data);
			_mm_store_ss(&value.y, yVec);
		}
        inline void StoreFloat3(Float3 &value) const
		{
			auto yVec = _mm_shuffle_ps(data, data, _MM_SHUFFLE(1, 1, 1, 1));
			auto zVec = _mm_shuffle_ps(data, data, _MM_SHUFFLE(2, 2, 2, 2));
			_mm_store_ss(&value.x, data);
			_mm_store_ss(&value.y, yVec);
			_mm_store_ss(&value.z, zVec);
		}
        inline void StoreFloat4(Float4 &value) const
		{
			_mm_storeu_ps(&value.x, data);
		}

        inline float ToFloat() const
		{
			float v;
			StoreFloat(v);
			return v;
		}
        inline Float2 ToFloat2() const
		{
			Float2 v;
			StoreFloat2(v);
			return v;
		}
        inline Float3 ToFloat3() const
		{
			Float3 v;
			StoreFloat3(v);
			return v;
		}
        inline Float4 ToFloat4() const
		{
			Float4 v;
			StoreFloat4(v);
			return v;
		}

        inline operator Float2() const { return ToFloat2(); }
        inline operator Float3() const { return ToFloat3(); }
        inline operator Float4() const { return ToFloat4(); }

        // Element accessors
        //-------------------------------------------------------------------------

        inline float GetX() const { return _mm_cvtss_f32(data); }
        inline float GetY() const
        {
            auto vTemp = GetSplatY();
            return _mm_cvtss_f32(vTemp);
        }
        inline float GetZ() const
        {
            auto vTemp = GetSplatZ();
            return _mm_cvtss_f32(vTemp);
        }
        inline float GetW() const
        {
            auto vTemp = GetSplatW();
            return _mm_cvtss_f32(vTemp);
        }

        inline void SetX(float x) { data = _mm_move_ss(data, _mm_set_ss(x)); }
        inline void SetY(float y) { data = _mm_insert_ps(data, _mm_set_ss(y), 0x10); }
        inline void SetZ(float z) { data = _mm_insert_ps(data, _mm_set_ss(z), 0x20); }
        inline void SetW(float w) { data = _mm_insert_ps(data, _mm_set_ss(w), 0x30); }

        inline float operator[](uint32_t i) const;

        // TODO: fix this since it is UB
        // inline float& operator[]( uint32_t i ) { ENGINE_ASSERT( i < 4 ); return data.m128_f32[i]; }

        // W component operations - needed primarily for homogeneous coordinate operations
        //-------------------------------------------------------------------------

        inline bool IsW1() const { return GetSplatW().IsEqual4(VectorSIMD::One); }
        inline bool IsW0() const { return GetSplatW().IsZero4(); }
        inline VectorSIMD &SetW0()
        {
            SetW(0.0f);
            return *this;
        }
        inline VectorSIMD &SetW1()
        {
            SetW(1.0f);
            return *this;
        }
        inline VectorSIMD GetWithW0() const
        {
            VectorSIMD v = *this;
            v.SetW0();
            return v;
        }
        inline VectorSIMD GetWithW1() const
        {
            VectorSIMD v = *this;
            v.SetW1();
            return v;
        }

        // Dimensional Getters
        //-------------------------------------------------------------------------

        // Returns only the first two components, z=w=0
        inline VectorSIMD Get2D() const { return VectorSIMD::Select(*this, VectorSIMD::Zero, VectorSIMD::Select0011); }

        // Returns only the first three components, w = 0
        inline VectorSIMD Get3D() const { return VectorSIMD::Select(*this, VectorSIMD::Zero, VectorSIMD::Select0001); }

        // Algebraic operators
        //-------------------------------------------------------------------------

        inline VectorSIMD operator+(VectorSIMD const &v) const { return _mm_add_ps(data, v); }
        inline VectorSIMD &operator+=(VectorSIMD const &v)
        {
            data = _mm_add_ps(data, v);
            return *this;
        }
        inline VectorSIMD operator-(VectorSIMD const &v) const { return _mm_sub_ps(data, v); }
        inline VectorSIMD &operator-=(VectorSIMD const &v)
        {
            data = _mm_sub_ps(data, v);
            return *this;
        }
        inline VectorSIMD operator*(VectorSIMD const &v) const { return _mm_mul_ps(data, v); }
        inline VectorSIMD &operator*=(VectorSIMD const &v)
        {
            data = _mm_mul_ps(data, v);
            return *this;
        }
        inline VectorSIMD operator/(VectorSIMD const &v) const { return _mm_div_ps(data, v); }
        inline VectorSIMD &operator/=(VectorSIMD const &v)
        {
            data = _mm_div_ps(data, v);
            return *this;
        }

        inline VectorSIMD operator*(float const f) const { return operator*(VectorSIMD(f)); }
        inline VectorSIMD &operator*=(float const f) { return operator*=(VectorSIMD(f)); }
        inline VectorSIMD operator/(float const f) const { return operator/(VectorSIMD(f)); }
        inline VectorSIMD &operator/=(float const f) { return operator/=(VectorSIMD(f)); }

        inline VectorSIMD operator-() const { return GetNegated(); }

        inline VectorSIMD Orthogonal2D() const;
        inline VectorSIMD Cross2(VectorSIMD const &other) const;
        inline VectorSIMD Cross3(VectorSIMD const &other) const;
        inline VectorSIMD Dot2(VectorSIMD const &other) const;
        inline VectorSIMD Dot3(VectorSIMD const &other) const;
        inline VectorSIMD Dot4(VectorSIMD const &other) const;
        inline float GetDot2(VectorSIMD const &other) const { return Dot2(other).ToFloat(); }
        inline float GetDot3(VectorSIMD const &other) const { return Dot3(other).ToFloat(); }
        inline float GetDot4(VectorSIMD const &other) const { return Dot4(other).ToFloat(); }

        inline VectorSIMD ScalarProjection(VectorSIMD const &other) const;
        inline float GetScalarProjection(VectorSIMD const &other) const { return ScalarProjection(other).ToFloat(); }
        inline VectorSIMD VectorProjection(VectorSIMD const &other) const;

        // Transformations
        //-------------------------------------------------------------------------

        inline VectorSIMD &Invert()
        {
            data = _mm_div_ps(VectorSIMD::One, data);
            return *this;
        }
        inline VectorSIMD GetInverse() const { return _mm_div_ps(VectorSIMD::One, data); }
        inline VectorSIMD GetReciprocal() const { return GetInverse(); }

        inline VectorSIMD &InvertEst()
        {
            data = _mm_rcp_ps(data);
            return *this;
        }
        inline VectorSIMD GetInverseEst() const { return _mm_rcp_ps(data); }

        inline VectorSIMD &Negate()
        {
            data = _mm_sub_ps(VectorSIMD::Zero, data);
            return *this;
        }
        inline VectorSIMD GetNegated() const { return _mm_sub_ps(VectorSIMD::Zero, data); }

        inline VectorSIMD &Abs()
        {
            data = _mm_max_ps(_mm_sub_ps(VectorSIMD::Zero, data), data);
            return *this;
        }
        inline VectorSIMD GetAbs() const { return _mm_max_ps(_mm_sub_ps(VectorSIMD::Zero, data), data); }

        inline VectorSIMD &Sqrt()
        {
            data = _mm_sqrt_ps(data);
            return *this;
        }
        inline VectorSIMD GetSqrt() { return _mm_sqrt_ps(data); }

        inline VectorSIMD &ReciprocalSqrt()
        {
            data = _mm_div_ps(VectorSIMD::One, _mm_sqrt_ps(data));
            return *this;
        }
        inline VectorSIMD GetReciprocalSqrt() { return _mm_div_ps(VectorSIMD::One, _mm_sqrt_ps(data)); }

        inline VectorSIMD &EstimatedReciprocalSqrt()
        {
            data = _mm_rsqrt_ps(data);
            return *this;
        }
        inline VectorSIMD GetEstimatedReciprocalSqrt() { return _mm_rsqrt_ps(data); }

        VectorSIMD &Normalize2();
        VectorSIMD &Normalize3();
        VectorSIMD &Normalize4();

        VectorSIMD GetNormalized2() const;
        VectorSIMD GetNormalized3() const;
        VectorSIMD GetNormalized4() const;

        VectorSIMD &Floor();
        VectorSIMD GetFloor() const;
        VectorSIMD &Ceil();
        VectorSIMD GetCeil() const;
        VectorSIMD &Round();
        VectorSIMD GetRound() const;

        VectorSIMD GetSign() const;

        // Permutations
        //-------------------------------------------------------------------------

        inline VectorSIMD GetSplatX() const { return _mm_shuffle_ps(data, data, _MM_SHUFFLE(0, 0, 0, 0)); }
        inline VectorSIMD GetSplatY() const { return _mm_shuffle_ps(data, data, _MM_SHUFFLE(1, 1, 1, 1)); }
        inline VectorSIMD GetSplatZ() const { return _mm_shuffle_ps(data, data, _MM_SHUFFLE(2, 2, 2, 2)); }
        inline VectorSIMD GetSplatW() const { return _mm_shuffle_ps(data, data, _MM_SHUFFLE(3, 3, 3, 3)); }

        // Get a shuffled version of this VectorSIMD, each argument represents the element index in the original VectorSIMD
        template <uint32_t xIdx, uint32_t yIdx, uint32_t zIdx, uint32_t wIdx>
        inline VectorSIMD Swizzle() const
        {
            static_assert(xIdx < 4, "Element index parameter out of range");
            static_assert(yIdx < 4, "Element index parameter out of range");
            static_assert(zIdx < 4, "Element index parameter out of range");
            static_assert(wIdx < 4, "Element index parameter out of range");
            return _mm_shuffle_ps(data, data, _MM_SHUFFLE(wIdx, zIdx, yIdx, xIdx));
        }

        // Get a shuffled version of this VectorSIMD, each argument represents the element index in the original VectorSIMD
        inline VectorSIMD Swizzle(uint32_t xIdx, uint32_t yIdx, uint32_t zIdx, uint32_t wIdx) const
        {
            ENGINE_ASSERT(xIdx < 4 && yIdx < 4 && zIdx < 4 && wIdx < 4);
            uint32_t const elem[4] = {xIdx, yIdx, zIdx, wIdx};
            __m128i vControl = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&elem[0]));
            return _mm_permutevar_ps(data, vControl);
        }

        // Get a shuffled version of this VectorSIMD, each argument represents the element index in the original VectorSIMD
        inline VectorSIMD Shuffle(uint32_t xIdx, uint32_t yIdx, uint32_t zIdx, uint32_t wIdx) const { return Swizzle(xIdx, yIdx, zIdx, wIdx); }

        // Get a shuffled version of this VectorSIMD, each argument represents the element index in the original VectorSIMD
        template <uint32_t xIdx, uint32_t yIdx, uint32_t zIdx, uint32_t wIdx>
        inline VectorSIMD Shuffle() const { return Swizzle<xIdx, yIdx, zIdx, wIdx>(); }

        // Queries
        //-------------------------------------------------------------------------

        inline VectorSIMD Length2() const;
        inline VectorSIMD Length3() const;
        inline VectorSIMD Length4() const;

        inline float GetLength2() const { return Length2().GetX(); }
        inline float GetLength3() const { return Length3().GetX(); }
        inline float GetLength4() const { return Length4().GetX(); }

        inline VectorSIMD InverseLength2() const;
        inline VectorSIMD InverseLength3() const;
        inline VectorSIMD InverseLength4() const;

        inline float GetInverseLength2() const { return InverseLength2().GetX(); }
        inline float GetInverseLength3() const { return InverseLength3().GetX(); }
        inline float GetInverseLength4() const { return InverseLength4().GetX(); }

        inline VectorSIMD LengthSquared2() const { return VectorSIMD::Dot2(data, data); }
        inline VectorSIMD LengthSquared3() const { return VectorSIMD::Dot3(data, data); }
        inline VectorSIMD LengthSquared4() const { return VectorSIMD::Dot4(data, data); }

        inline float GetLengthSquared2() const { return LengthSquared2().GetX(); }
        inline float GetLengthSquared3() const { return LengthSquared3().GetX(); }
        inline float GetLengthSquared4() const { return LengthSquared4().GetX(); }

        inline VectorSIMD Distance2(VectorSIMD const &to) const { return (to - *this).Length2(); }
        inline VectorSIMD Distance3(VectorSIMD const &to) const { return (to - *this).Length3(); }
        inline VectorSIMD Distance4(VectorSIMD const &to) const { return (to - *this).Length4(); }

        inline float GetDistance2(VectorSIMD const &to) const { return (to - *this).Length2().GetX(); }
        inline float GetDistance3(VectorSIMD const &to) const { return (to - *this).Length3().GetX(); }
        inline float GetDistance4(VectorSIMD const &to) const { return (to - *this).Length4().GetX(); }

        inline VectorSIMD DistanceSquared2(VectorSIMD const &to) const { return (to - *this).LengthSquared2(); }
        inline VectorSIMD DistanceSquared3(VectorSIMD const &to) const { return (to - *this).LengthSquared3(); }
        inline VectorSIMD DistanceSquared4(VectorSIMD const &to) const { return (to - *this).LengthSquared4(); }

        inline float GetDistanceSquared2(VectorSIMD const &to) const { return (to - *this).GetLengthSquared2(); }
        inline float GetDistanceSquared3(VectorSIMD const &to) const { return (to - *this).GetLengthSquared3(); }
        inline float GetDistanceSquared4(VectorSIMD const &to) const { return (to - *this).GetLengthSquared4(); }

        inline bool IsNormalized2() const { return (LengthSquared2() - VectorSIMD::One).Abs().IsLessThanEqual4(VectorSIMD::NormalizeCheckThreshold); }
        inline bool IsNormalized3() const { return (LengthSquared3() - VectorSIMD::One).Abs().IsLessThanEqual4(VectorSIMD::NormalizeCheckThreshold); }
        inline bool IsNormalized4() const { return (LengthSquared4() - VectorSIMD::One).Abs().IsLessThanEqual4(VectorSIMD::NormalizeCheckThreshold); }

        inline VectorSIMD InBounds(VectorSIMD const &bounds) const; // Is this VectorSIMD within the range [-bounds, bounds]

        inline bool IsInBounds2(VectorSIMD const &bounds) const { return (_mm_movemask_ps(InBounds(bounds)) & 0x3) == 0x3 != 0; }
        inline bool IsInBounds3(VectorSIMD const &bounds) const { return (_mm_movemask_ps(InBounds(bounds)) & 0x7) == 0x7 != 0; }
        inline bool IsInBounds4(VectorSIMD const &bounds) const { return (_mm_movemask_ps(InBounds(bounds)) == 0x0f) != 0; }

        inline VectorSIMD Equal(VectorSIMD const &v) const { return _mm_cmpeq_ps(*this, v); }

        inline bool IsEqual2(VectorSIMD const &v) const { return (((_mm_movemask_ps(Equal(v)) & 3) == 3) != 0); }
        inline bool IsEqual3(VectorSIMD const &v) const { return (((_mm_movemask_ps(Equal(v)) & 7) == 7) != 0); }
        inline bool IsEqual4(VectorSIMD const &v) const { return ((_mm_movemask_ps(Equal(v)) == 0x0f) != 0); }

        inline VectorSIMD NearEqual(VectorSIMD const &v, VectorSIMD const &epsilon) const;

        inline bool IsNearEqual2(VectorSIMD const &v, float epsilon) const { return IsNearEqual2(v, VectorSIMD(epsilon)); }
        inline bool IsNearEqual3(VectorSIMD const &v, float epsilon) const { return IsNearEqual3(v, VectorSIMD(epsilon)); }
        inline bool IsNearEqual4(VectorSIMD const &v, float epsilon) const { return IsNearEqual4(v, VectorSIMD(epsilon)); }

        inline bool IsNearEqual2(VectorSIMD const &v, VectorSIMD const &epsilon = VectorSIMD::Epsilon) const { return (((_mm_movemask_ps(NearEqual(v, epsilon)) & 3) == 0x3) != 0); }
        inline bool IsNearEqual3(VectorSIMD const &v, VectorSIMD const &epsilon = VectorSIMD::Epsilon) const { return (((_mm_movemask_ps(NearEqual(v, epsilon)) & 7) == 0x7) != 0); }
        inline bool IsNearEqual4(VectorSIMD const &v, VectorSIMD const &epsilon = VectorSIMD::Epsilon) const { return ((_mm_movemask_ps(NearEqual(v, epsilon)) == 0xf) != 0); }

        inline VectorSIMD GreaterThan(VectorSIMD const &v) const { return _mm_cmpgt_ps(data, v); }
        inline bool IsAnyGreaterThan(VectorSIMD const &v) const { return !GreaterThan(v).IsZero4(); }

        inline bool IsGreaterThan2(VectorSIMD const &v) const { return (((_mm_movemask_ps(GreaterThan(v)) & 3) == 3) != 0); }
        inline bool IsGreaterThan3(VectorSIMD const &v) const { return (((_mm_movemask_ps(GreaterThan(v)) & 7) == 7) != 0); }
        inline bool IsGreaterThan4(VectorSIMD const &v) const { return ((_mm_movemask_ps(GreaterThan(v)) == 0x0f) != 0); }

        inline VectorSIMD GreaterThanEqual(VectorSIMD const &v) const { return _mm_cmpge_ps(data, v); }
        inline bool IsAnyGreaterThanEqual(VectorSIMD const &v) const { return !GreaterThanEqual(v).IsZero4(); }

        inline bool IsGreaterThanEqual2(VectorSIMD const &v) const { return ((_mm_movemask_ps(GreaterThanEqual(v)) & 3) == 3) != 0; }
        inline bool IsGreaterThanEqual3(VectorSIMD const &v) const { return ((_mm_movemask_ps(GreaterThanEqual(v)) & 7) == 7) != 0; }
        inline bool IsGreaterThanEqual4(VectorSIMD const &v) const { return (_mm_movemask_ps(GreaterThanEqual(v)) == 0x0f) != 0; }

        inline VectorSIMD LessThan(VectorSIMD const &v) const { return _mm_cmplt_ps(data, v); }
        inline bool IsAnyLessThan(VectorSIMD const &v) const { return !LessThan(v).IsZero4(); }

        inline bool IsLessThan2(VectorSIMD const &v) const { return (((_mm_movemask_ps(LessThan(v)) & 3) == 3) != 0); }
        inline bool IsLessThan3(VectorSIMD const &v) const { return (((_mm_movemask_ps(LessThan(v)) & 7) == 7) != 0); }
        inline bool IsLessThan4(VectorSIMD const &v) const { return ((_mm_movemask_ps(LessThan(v)) == 0x0f) != 0); }

        inline VectorSIMD LessThanEqual(VectorSIMD const &v) const { return _mm_cmple_ps(data, v); }
        inline bool IsAnyLessThanEqual(VectorSIMD const &v) const { return !LessThanEqual(v).IsZero4(); }

        inline bool IsLessThanEqual2(VectorSIMD const &v) const { return (((_mm_movemask_ps(LessThanEqual(v)) & 3) == 3) != 0); }
        inline bool IsLessThanEqual3(VectorSIMD const &v) const { return (((_mm_movemask_ps(LessThanEqual(v)) & 7) == 7) != 0); }
        inline bool IsLessThanEqual4(VectorSIMD const &v) const { return ((_mm_movemask_ps(LessThanEqual(v)) == 0x0f) != 0); }

        inline VectorSIMD EqualsZero() const { return Equal(VectorSIMD::Zero); }
        inline bool IsAnyEqualToZero2() const { return !EqualsZero().IsZero2(); }
        inline bool IsAnyEqualToZero3() const { return !EqualsZero().IsZero3(); }
        inline bool IsAnyEqualToZero4() const { return !EqualsZero().IsZero4(); }

        inline bool IsZero2() const { return IsEqual2(VectorSIMD::Zero); }
        inline bool IsZero3() const { return IsEqual3(VectorSIMD::Zero); }
        inline bool IsZero4() const { return IsEqual4(VectorSIMD::Zero); }

        inline VectorSIMD NearEqualsZero(float epsilon = Math::EPSILON) const { return NearEqual(VectorSIMD::Zero, VectorSIMD(epsilon)); }

        inline bool IsNearZero2(float epsilon = Math::EPSILON) const { return IsNearEqual2(VectorSIMD::Zero, VectorSIMD(epsilon)); }
        inline bool IsNearZero3(float epsilon = Math::EPSILON) const { return IsNearEqual3(VectorSIMD::Zero, VectorSIMD(epsilon)); }
        inline bool IsNearZero4(float epsilon = Math::EPSILON) const { return IsNearEqual4(VectorSIMD::Zero, VectorSIMD(epsilon)); }

        inline VectorSIMD EqualsInfinity() const
        {
            __m128 vTemp = _mm_and_ps(data, SIMD::g_absMask);
            return _mm_cmpeq_ps(vTemp, VectorSIMD::Infinity);
        }

        inline bool IsInfinite2() const { return (_mm_movemask_ps(EqualsInfinity()) & 3) != 0; }
        inline bool IsInfinite3() const { return (_mm_movemask_ps(EqualsInfinity()) & 7) != 0; }
        inline bool IsInfinite4() const { return (_mm_movemask_ps(EqualsInfinity()) != 0); }

        inline VectorSIMD EqualsNaN() const { return _mm_cmpneq_ps(data, data); }

        inline bool IsNaN2() const { return (_mm_movemask_ps(EqualsNaN()) & 3) != 0; }
        inline bool IsNaN3() const { return (_mm_movemask_ps(EqualsNaN()) & 7) != 0; }
        inline bool IsNaN4() const { return (_mm_movemask_ps(EqualsNaN()) != 0); }

        inline bool IsParallelTo(VectorSIMD const &v) const;

        inline void ToDirectionAndLength2(VectorSIMD &direction, float &length) const;
        inline void ToDirectionAndLength3(VectorSIMD &direction, float &length) const;

        bool operator==(VectorSIMD const &rhs) const { return IsEqual4(rhs); }
        bool operator!=(VectorSIMD const &rhs) const { return !IsEqual4(rhs); }

    public:
        __m128 data;
    };

    //-------------------------------------------------------------------------

    static_assert(sizeof(VectorSIMD) == 16, "VectorSIMD size must be 16 bytes!");

    //-------------------------------------------------------------------------

    //-------------------------------------------------------------------------

    inline float VectorSIMD::operator[](uint32 i) const
    {
        ENGINE_ASSERT(i < 4);

        switch (i)
        {
        case 0:
            return GetX();
            break;
        case 1:
            return GetY();
            break;
        case 2:
            return GetZ();
            break;
        case 3:
            return GetW();
            break;
        }

        ENGINE_UNREACHABLE_CODE();
        return 0.0f;
    }

    //-------------------------------------------------------------------------

    inline VectorSIMD VectorSIMD::Length2() const
    {
        VectorSIMD result;

        result = _mm_mul_ps(data, data);
        auto vTemp = _mm_shuffle_ps(result, result, _MM_SHUFFLE(1, 1, 1, 1));
        // m_x+m_y
        result = _mm_add_ss(result, vTemp);
        result = _mm_shuffle_ps(result, result, _MM_SHUFFLE(0, 0, 0, 0));
        result = _mm_sqrt_ps(result);
        return result;
    }

    inline VectorSIMD VectorSIMD::Length3() const
    {
        VectorSIMD result;

        // Perform the dot product on m_x,m_y and m_z
        result = _mm_mul_ps(data, data);
        // vTemp has m_z and m_y
        auto vTemp = _mm_shuffle_ps(result, result, _MM_SHUFFLE(1, 2, 1, 2));
        // m_x+m_z, m_y
        result = _mm_add_ss(result, vTemp);
        // m_y,m_y,m_y,m_y
        vTemp = _mm_shuffle_ps(vTemp, vTemp, _MM_SHUFFLE(1, 1, 1, 1));
        // m_x+m_z+m_y,??,??,??
        result = _mm_add_ss(result, vTemp);
        // Splat the length squared
        result = _mm_shuffle_ps(result, result, _MM_SHUFFLE(0, 0, 0, 0));
        // Get the length
        result = _mm_sqrt_ps(result);

        return result;
    }

    inline VectorSIMD VectorSIMD::Length4() const
    {
        VectorSIMD result;

        // Perform the dot product on m_x,m_y,m_z and m_w
        result = _mm_mul_ps(data, data);
        // vTemp has m_z and m_w
        auto vTemp = _mm_shuffle_ps(result, result, _MM_SHUFFLE(3, 2, 3, 2));
        // m_x+m_z, m_y+m_w
        result = _mm_add_ps(result, vTemp);
        // m_x+m_z,m_x+m_z,m_x+m_z,m_y+m_w
        result = _mm_shuffle_ps(result, result, _MM_SHUFFLE(1, 0, 0, 0));
        // ??,??,m_y+m_w,m_y+m_w
        vTemp = _mm_shuffle_ps(vTemp, result, _MM_SHUFFLE(3, 3, 0, 0));
        // ??,??,m_x+m_z+m_y+m_w,??
        result = _mm_add_ps(result, vTemp);
        // Splat the length
        result = _mm_shuffle_ps(result, result, _MM_SHUFFLE(2, 2, 2, 2));
        // Get the length
        result = _mm_sqrt_ps(result);

        return result;
    }

    inline VectorSIMD VectorSIMD::InverseLength2() const
    {
        // Perform the dot product on m_x and m_y
        auto vLengthSq = _mm_mul_ps(data, data);
        // vTemp has m_y splatted
        auto vTemp = _mm_shuffle_ps(vLengthSq, vLengthSq, _MM_SHUFFLE(1, 1, 1, 1));
        // m_x+m_y
        vLengthSq = _mm_add_ss(vLengthSq, vTemp);
        vLengthSq = _mm_sqrt_ss(vLengthSq);
        vLengthSq = _mm_div_ss(VectorSIMD::One, vLengthSq);
        vLengthSq = _mm_shuffle_ps(vLengthSq, vLengthSq, _MM_SHUFFLE(0, 0, 0, 0));
        return vLengthSq;
    }

    inline VectorSIMD VectorSIMD::InverseLength3() const
    {
        // Perform the dot product
        auto vDot = _mm_mul_ps(data, data);
        // m_x=Dot.m_y, m_y=Dot.m_z
        auto vTemp = _mm_shuffle_ps(vDot, vDot, _MM_SHUFFLE(2, 1, 2, 1));
        // Result.m_x = m_x+m_y
        vDot = _mm_add_ss(vDot, vTemp);
        // m_x=Dot.m_z
        vTemp = _mm_shuffle_ps(vTemp, vTemp, _MM_SHUFFLE(1, 1, 1, 1));
        // Result.m_x = (m_x+m_y)+m_z
        vDot = _mm_add_ss(vDot, vTemp);
        // Splat m_x
        vDot = _mm_shuffle_ps(vDot, vDot, _MM_SHUFFLE(0, 0, 0, 0));
        // Get the reciprocal
        vDot = _mm_sqrt_ps(vDot);
        // Get the reciprocal
        vDot = _mm_div_ps(VectorSIMD::One, vDot);
        return vDot;
    }

    inline VectorSIMD VectorSIMD::InverseLength4() const
    {
        // Perform the dot product on m_x,m_y,m_z and m_w
        auto vLengthSq = _mm_mul_ps(data, data);
        // vTemp has m_z and m_w
        auto vTemp = _mm_shuffle_ps(vLengthSq, vLengthSq, _MM_SHUFFLE(3, 2, 3, 2));
        // m_x+m_z, m_y+m_w
        vLengthSq = _mm_add_ps(vLengthSq, vTemp);
        // m_x+m_z,m_x+m_z,m_x+m_z,m_y+m_w
        vLengthSq = _mm_shuffle_ps(vLengthSq, vLengthSq, _MM_SHUFFLE(1, 0, 0, 0));
        // ??,??,m_y+m_w,m_y+m_w
        vTemp = _mm_shuffle_ps(vTemp, vLengthSq, _MM_SHUFFLE(3, 3, 0, 0));
        // ??,??,m_x+m_z+m_y+m_w,??
        vLengthSq = _mm_add_ps(vLengthSq, vTemp);
        // Splat the length
        vLengthSq = _mm_shuffle_ps(vLengthSq, vLengthSq, _MM_SHUFFLE(2, 2, 2, 2));
        // Get the reciprocal
        vLengthSq = _mm_sqrt_ps(vLengthSq);
        // Accurate!
        vLengthSq = _mm_div_ps(VectorSIMD::One, vLengthSq);
        return vLengthSq;
    }

    //-------------------------------------------------------------------------

    inline VectorSIMD VectorSIMD::NearEqual(VectorSIMD const &v, VectorSIMD const &epsilon) const
    {
        // Get the difference
        auto vDelta = _mm_sub_ps(data, v);
        // Get the absolute value of the difference
        auto vTemp = _mm_setzero_ps();
        vTemp = _mm_sub_ps(vTemp, vDelta);
        vTemp = _mm_max_ps(vTemp, vDelta);
        vTemp = _mm_cmple_ps(vTemp, epsilon);
        return vTemp;
    }

    inline VectorSIMD VectorSIMD::InBounds(VectorSIMD const &bounds) const
    {
        // Test if less than or equal
        auto vTemp1 = _mm_cmple_ps(data, bounds);
        // Negate the bounds
        auto vTemp2 = _mm_mul_ps(bounds, VectorSIMD::NegativeOne);
        // Test if greater or equal (Reversed)
        vTemp2 = _mm_cmple_ps(vTemp2, data);
        // Blend answers
        vTemp1 = _mm_and_ps(vTemp1, vTemp2);
        return vTemp1;
    }

    inline bool VectorSIMD::IsParallelTo(VectorSIMD const &v) const
    {
        VectorSIMD const vAbsDot = VectorSIMD::Dot3(*this, v).GetAbs();
        VectorSIMD const vAbsDelta = VectorSIMD::One - vAbsDot;
        return vAbsDelta.IsLessThanEqual4(VectorSIMD::Epsilon);
    }

    //-------------------------------------------------------------------------

    // inline VectorSIMD::VectorSIMD(Axis axis)
    // {
    //     switch (axis)
    //     {
    //     case Axis::X:
    //         *this = VectorSIMD::UnitX;
    //         break;
    //     case Axis::Y:
    //         *this = VectorSIMD::UnitY;
    //         break;
    //     case Axis::Z:
    //         *this = VectorSIMD::UnitZ;
    //         break;
    //     default:
    //         ENGINE_HALT();
    //         break;
    //     }
    // }

    inline VectorSIMD VectorSIMD::Orthogonal2D() const
    {
        static VectorSIMD const negX(-1.0f, 1.0f, 1.0f, 1.0f);

        VectorSIMD result;
        result = _mm_shuffle_ps(*this, *this, _MM_SHUFFLE(3, 2, 0, 1));
        result = _mm_mul_ps(result, negX);
        return result;
    }

    inline VectorSIMD VectorSIMD::Cross2(VectorSIMD const &other) const
    {
        VectorSIMD vResult = _mm_shuffle_ps(other.data, other.data, _MM_SHUFFLE(0, 1, 0, 1));
        vResult = _mm_mul_ps(vResult, data);
        VectorSIMD vTemp = vResult.GetSplatY();
        vResult = _mm_sub_ss(vResult, vTemp);
        vResult = vResult.GetSplatX();
        return vResult;
    }

    inline VectorSIMD VectorSIMD::Cross3(VectorSIMD const &other) const
    {
        auto vTemp1 = _mm_shuffle_ps(data, data, _MM_SHUFFLE(3, 0, 2, 1));
        auto vTemp2 = _mm_shuffle_ps(other, other, _MM_SHUFFLE(3, 1, 0, 2));
        VectorSIMD result = _mm_mul_ps(vTemp1, vTemp2);
        vTemp1 = _mm_shuffle_ps(vTemp1, vTemp1, _MM_SHUFFLE(3, 0, 2, 1));
        vTemp2 = _mm_shuffle_ps(vTemp2, vTemp2, _MM_SHUFFLE(3, 1, 0, 2));
        vTemp1 = _mm_mul_ps(vTemp1, vTemp2);
        result = _mm_sub_ps(result, vTemp1);
        result = _mm_and_ps(result, SIMD::g_maskXYZ0);
        return result;
    }

    inline VectorSIMD VectorSIMD::Dot2(VectorSIMD const &other) const
    {
        // Perform the dot product on m_x and m_y
        VectorSIMD result = _mm_mul_ps(data, other);
        // vTemp has m_y splatted
        auto vTemp = _mm_shuffle_ps(result, result, _MM_SHUFFLE(1, 1, 1, 1));
        // m_x+m_y
        result = _mm_add_ss(result, vTemp);
        result = _mm_shuffle_ps(result, result, _MM_SHUFFLE(0, 0, 0, 0));
        return result;
    }

    inline VectorSIMD VectorSIMD::Dot3(VectorSIMD const &vOther) const
    {
        // Perform the dot product
        auto vDot = _mm_mul_ps(data, vOther);
        // m_x=Dot.vector4_f32[1], m_y=Dot.vector4_f32[2]
        auto vTemp = _mm_shuffle_ps(vDot, vDot, _MM_SHUFFLE(2, 1, 2, 1));
        // Result.vector4_f32[0] = m_x+m_y
        vDot = _mm_add_ss(vDot, vTemp);
        // m_x=Dot.vector4_f32[2]
        vTemp = _mm_shuffle_ps(vTemp, vTemp, _MM_SHUFFLE(1, 1, 1, 1));
        // Result.vector4_f32[0] = (m_x+m_y)+m_z
        vDot = _mm_add_ss(vDot, vTemp);
        // Splat m_x
        VectorSIMD result = _mm_shuffle_ps(vDot, vDot, _MM_SHUFFLE(0, 0, 0, 0));
        return result;
    }

    inline VectorSIMD VectorSIMD::Dot4(VectorSIMD const &other) const
    {
        auto vTemp2 = other;
        auto vTemp = _mm_mul_ps(data, vTemp2);
        vTemp2 = _mm_shuffle_ps(vTemp2, vTemp, _MM_SHUFFLE(1, 0, 0, 0)); // Copy X to the Z position and Y to the W position
        vTemp2 = _mm_add_ps(vTemp2, vTemp);                              // Add Z = X+Z; W = Y+W;
        vTemp = _mm_shuffle_ps(vTemp, vTemp2, _MM_SHUFFLE(0, 3, 0, 0));  // Copy W to the Z position
        vTemp = _mm_add_ps(vTemp, vTemp2);                               // Add Z and W together
        return _mm_shuffle_ps(vTemp, vTemp, _MM_SHUFFLE(2, 2, 2, 2));    // Splat Z and return
    }

    inline VectorSIMD VectorSIMD::ScalarProjection(VectorSIMD const &other) const
    {
        VectorSIMD const normalizedThis = GetNormalized3();
        VectorSIMD const projection = other.Dot3(normalizedThis);
        return projection;
    }

    inline VectorSIMD VectorSIMD::VectorProjection(VectorSIMD const &other) const
    {
        VectorSIMD const normalizedThis = GetNormalized3();
        VectorSIMD const dotOther = other.Dot3(normalizedThis);
        VectorSIMD const projection = normalizedThis * dotOther;
        return projection;
    }

    inline VectorSIMD VectorSIMD::Average2(VectorSIMD const &v0, VectorSIMD const &v1)
    {
        auto avg4 = Average4(v0, v1);
        return VectorSIMD::Select(avg4, VectorSIMD::Zero, VectorSIMD(0, 0, 1, 1));
    }

    inline VectorSIMD VectorSIMD::Average3(VectorSIMD const &v0, VectorSIMD const &v1)
    {
        auto avg4 = Average4(v0, v1);
        return VectorSIMD::Select(avg4, VectorSIMD::Zero, VectorSIMD(0, 0, 0, 1));
    }

    inline VectorSIMD VectorSIMD::Average4(VectorSIMD const &v0, VectorSIMD const &v1)
    {
        return (v0 + v1) * VectorSIMD::Half;
    }

    inline VectorSIMD VectorSIMD::Min(VectorSIMD const &v0, VectorSIMD const &v1)
    {
        VectorSIMD result;
        result = _mm_min_ps(v0, v1);
        return result;
    }

    inline VectorSIMD VectorSIMD::Max(VectorSIMD const &v0, VectorSIMD const &v1)
    {
        VectorSIMD result;
        result = _mm_max_ps(v0, v1);
        return result;
    }

    inline VectorSIMD VectorSIMD::Clamp(VectorSIMD const &v, VectorSIMD const &min, VectorSIMD const &max)
    {
        VectorSIMD result;
        result = _mm_max_ps(min, v);
        result = _mm_min_ps(result, max);
        return result;
    }

    inline VectorSIMD VectorSIMD::Xor(VectorSIMD const &v0, VectorSIMD const &v1)
    {
        __m128i V = _mm_xor_si128(_mm_castps_si128(v0), _mm_castps_si128(v1));

        VectorSIMD result;
        result = _mm_castsi128_ps(V);
        return result;
    }

    template <uint32 PermuteX, uint32 PermuteY, uint32 PermuteZ, uint32 PermuteW>
    inline VectorSIMD VectorSIMD::Permute(VectorSIMD const &v0, VectorSIMD const &v1)
    {
        static_assert(PermuteX <= 7, "Element index parameter out of range");
        static_assert(PermuteY <= 7, "Element index parameter out of range");
        static_assert(PermuteZ <= 7, "Element index parameter out of range");
        static_assert(PermuteW <= 7, "Element index parameter out of range");

        uint32_t const shuffle = _MM_SHUFFLE(PermuteW & 3, PermuteZ & 3, PermuteY & 3, PermuteX & 3);
        bool const whichX = PermuteX > 3;
        bool const whichY = PermuteY > 3;
        bool const whichZ = PermuteZ > 3;
        bool const whichW = PermuteW > 3;

        static SIMD::UIntMask const selectMask = {whichX ? 0xFFFFFFFF : 0, whichY ? 0xFFFFFFFF : 0, whichZ ? 0xFFFFFFFF : 0, whichW ? 0xFFFFFFFF : 0};
        __m128 shuffled1 = _mm_shuffle_ps(v0, v0, shuffle);
        __m128 shuffled2 = _mm_shuffle_ps(v1, v1, shuffle);
        __m128 masked1 = _mm_andnot_ps(selectMask, shuffled1);
        __m128 masked2 = _mm_and_ps(selectMask, shuffled2);
        return _mm_or_ps(masked1, masked2);
    }


    //-------------------------------------------------------------------------

    inline void VectorSIMD::ToDirectionAndLength2(VectorSIMD &direction, float &length) const
    {
        VectorSIMD const vLength = Length2();
        direction = VectorSIMD::Select(*this, VectorSIMD::Zero, Select0011);
        direction /= vLength;
        length = vLength.ToFloat();
    }

    inline void VectorSIMD::ToDirectionAndLength3(VectorSIMD &direction, float &length) const
    {
        VectorSIMD const vLength = Length3();
        direction = VectorSIMD::Select(*this, VectorSIMD::Zero, Select0001);
        direction /= vLength;
        length = vLength.ToFloat();
    }*/
}