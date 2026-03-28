#include "VectorSIMD.h"

//-------------------------------------------------------------------------

namespace SE
{
	VectorSIMD const VectorSIMD::UnitX = { 1, 0, 0, 0 };
	VectorSIMD const VectorSIMD::UnitY = { 0, 1, 0, 0 };
	VectorSIMD const VectorSIMD::UnitZ = { 0, 0, 1, 0 };
	VectorSIMD const VectorSIMD::UnitW = { 0, 0, 0, 1 };

	VectorSIMD const VectorSIMD::Origin = { 0, 0, 0, 1 };
	VectorSIMD const VectorSIMD::WorldForward = { 0, -1, 0, 0 };
	VectorSIMD const VectorSIMD::WorldBackward = { 0, 1, 0, 0 };
	VectorSIMD const VectorSIMD::WorldUp = { 0, 0, 1, 0 };
	VectorSIMD const VectorSIMD::WorldDown = { 0, 0, -1, 0 };
	VectorSIMD const VectorSIMD::WorldLeft = { 1, 0, 0, 0 };
	VectorSIMD const VectorSIMD::WorldRight = { -1, 0, 0, 0 };

	VectorSIMD const VectorSIMD::Infinity = { 0x7F800000, 0x7F800000, 0x7F800000, 0x7F800000 };
	VectorSIMD const VectorSIMD::QNaN = { 0x7FC00000, 0x7FC00000, 0x7FC00000, 0x7FC00000 };

	VectorSIMD const VectorSIMD::NegativeOne(-1.0f);
	VectorSIMD const VectorSIMD::Zero(0.0f);
	VectorSIMD const VectorSIMD::Half(0.5f);
	VectorSIMD const VectorSIMD::One(1.0f);

	VectorSIMD const VectorSIMD::Epsilon(Math::EPSILON);
	VectorSIMD const VectorSIMD::LargeEpsilon(Math::Double_EPSILON);
	VectorSIMD const VectorSIMD::OneMinusEpsilon(1.0f - Math::EPSILON);
	VectorSIMD const VectorSIMD::EpsilonMinusOne(Math::EPSILON - 1.0f);
	VectorSIMD const VectorSIMD::NormalizeCheckThreshold(0.01f); // Squared Error

	VectorSIMD const VectorSIMD::Pi(Math::PI);
	VectorSIMD const VectorSIMD::PiDivTwo(Math::PiDivTwo);
	VectorSIMD const VectorSIMD::TwoPi(Math::TWO_PI);
	VectorSIMD const VectorSIMD::OneDivTwoPi(Math::ONE_OVER_PI);

	VectorSIMD const VectorSIMD::Select0000(0, 0, 0, 0);
	VectorSIMD const VectorSIMD::Select0001(0, 0, 0, 1);
	VectorSIMD const VectorSIMD::Select0010(0, 0, 1, 0);
	VectorSIMD const VectorSIMD::Select0011(0, 0, 1, 1);
	VectorSIMD const VectorSIMD::Select0100(0, 1, 0, 0);
	VectorSIMD const VectorSIMD::Select0101(0, 1, 0, 1);
	VectorSIMD const VectorSIMD::Select0110(0, 1, 1, 0);
	VectorSIMD const VectorSIMD::Select0111(0, 1, 1, 1);
	VectorSIMD const VectorSIMD::Select1000(1, 0, 0, 0);
	VectorSIMD const VectorSIMD::Select1001(1, 0, 0, 1);
	VectorSIMD const VectorSIMD::Select1010(1, 0, 1, 0);
	VectorSIMD const VectorSIMD::Select1011(1, 0, 1, 1);
	VectorSIMD const VectorSIMD::Select1100(1, 1, 0, 0);
	VectorSIMD const VectorSIMD::Select1101(1, 1, 0, 1);
	VectorSIMD const VectorSIMD::Select1110(1, 1, 1, 0);
	VectorSIMD const VectorSIMD::Select1111(1, 1, 1, 1);

	VectorSIMD const VectorSIMD::BoxCorners[8] =
		{
			{ -1.0f, -1.0f, 1.0f, 0.0f },
			{ 1.0f, -1.0f, 1.0f, 0.0f },
			{ 1.0f, 1.0f, 1.0f, 0.0f },
			{ -1.0f, 1.0f, 1.0f, 0.0f },
			{ -1.0f, -1.0f, -1.0f, 0.0f },
			{ 1.0f, -1.0f, -1.0f, 0.0f },
			{ 1.0f, 1.0f, -1.0f, 0.0f },
			{ -1.0f, 1.0f, -1.0f, 0.0f },
		};

	//-------------------------------------------------------------------------

	VectorSIMD VectorSIMD::MultiplyAdd(VectorSIMD const& v, VectorSIMD const& multiplier, VectorSIMD const& addend)
	{
		// result = addend + ( vec * multiplier )
		VectorSIMD result;
		result = _mm_mul_ps(v, multiplier);
		result = _mm_add_ps(result, addend);
		return result;
	}

	VectorSIMD VectorSIMD::MultiplySubtract(VectorSIMD const& vec,
		VectorSIMD const& multiplier,
		VectorSIMD const& subtrahend)
	{
		// result = ( vec * multiplier ) - subtrahend
		auto r = _mm_mul_ps(vec, multiplier);
		return _mm_sub_ps(r, subtrahend);
	}

	VectorSIMD VectorSIMD::NegativeMultiplySubtract(VectorSIMD const& vec,
		VectorSIMD const& multiplier,
		VectorSIMD const& minuend)
	{
		// result = minuend - ( vec * multiplier )
		auto r = _mm_mul_ps(vec, multiplier);
		return _mm_sub_ps(minuend, r);
	}

	VectorSIMD VectorSIMD::Lerp(VectorSIMD const& from, VectorSIMD const& to, float t)
	{
		ENGINE_ASSERT(t >= 0.0f && t <= 1.0f);

		VectorSIMD L = _mm_sub_ps(to, from);
		VectorSIMD S = _mm_set_ps1(t);

		VectorSIMD result;
		result = _mm_mul_ps(L, S);
		result = _mm_add_ps(result, from);
		return result;
	}

	VectorSIMD VectorSIMD::NLerp(VectorSIMD const& from, VectorSIMD const& to, float t)
	{
		ENGINE_ASSERT(t >= 0.0f && t <= 1.0f);

		// Calculate the final length
		auto const fromLength = from.Length3();
		auto const toLength = to.Length3();
		auto const finalLength = VectorSIMD::Lerp(fromLength, toLength, t);

		// Normalize vectors
		VectorSIMD const normalizedFrom = from / fromLength;
		VectorSIMD const normalizedTo = to / toLength;

		// LERP
		auto const finalDirection = Lerp(normalizedFrom, normalizedTo, t);
		auto result = finalDirection.GetNormalized3() * finalLength;
		return result;
	}

	VectorSIMD VectorSIMD::SLerp(VectorSIMD const& from, VectorSIMD const& to, float t)
	{
		ENGINE_ASSERT(t >= 0.0f && t <= 1.0f);
		if (from.LengthSquared3().IsLessThan4(Epsilon) || from.LengthSquared3().IsLessThan4(Epsilon))
		{
			return Lerp(from, to, t);
		}

		// Calculate the final length
		VectorSIMD const fromLength = from.Length3();
		VectorSIMD const toLength = to.Length3();
		VectorSIMD const finalLength = Lerp(fromLength, toLength, t);

		// Normalize vectors
		VectorSIMD const normalizedFrom = from / fromLength;
		VectorSIMD const normalizedTo = to / toLength;

		// Handle parallel VectorSIMD
		VectorSIMD result;
		if (normalizedFrom.IsParallelTo(normalizedTo))
		{
			result = normalizedFrom;
		}
		else // Interpolate the rotation between the vectors
		{
			VectorSIMD const dot = Dot3(normalizedFrom, normalizedTo);
			VectorSIMD const angle = ACos(dot);
			VectorSIMD const axis = Cross3(normalizedFrom, normalizedTo).Normalize3();
			VectorSIMD const interpolatedAngle = Lerp(Zero, angle, t);

			// Quaternion const rotation(Radians(interpolatedAngle.ToFloat()), axis);
			// VectorSIMD const finalDirection = rotation.RotateVector(normalizedFrom);
			// result = finalDirection.GetNormalized3() * finalLength;
		}

		return result;
	}

	VectorSIMD VectorSIMD::Select(VectorSIMD const& v0, VectorSIMD const& v1, VectorSIMD const& control)
	{
		auto const ctrl = _mm_cmpneq_ps(control, VectorSIMD::Zero);

		VectorSIMD result;
		auto vTemp1 = _mm_andnot_ps(ctrl, v0);
		auto vTemp2 = _mm_and_ps(v1, ctrl);
		result = _mm_or_ps(vTemp1, vTemp2);
		return result;
	}

	//-------------------------------------------------------------------------
	// Trigonometry

	VectorSIMD VectorSIMD::Sin(VectorSIMD const& vec)
	{
		// Force the value within the bounds of pi
		auto m_x = VectorSIMD::AngleMod2Pi(vec);

		// Map in [-pi/2,pi/2] with sin(m_y) = sin(m_x).
		__m128 sign = _mm_and_ps(m_x, SIMD::g_signMask);
		__m128 c = _mm_or_ps(VectorSIMD::Pi, sign); // pi when m_x >= 0, -pi when m_x < 0
		__m128 absx = _mm_andnot_ps(sign, m_x);     // |m_x|
		__m128 rflx = _mm_sub_ps(c, m_x);
		__m128 comp = _mm_cmple_ps(absx, VectorSIMD::PiDivTwo);
		__m128 select0 = _mm_and_ps(comp, m_x);
		__m128 select1 = _mm_andnot_ps(comp, rflx);
		m_x = _mm_or_ps(select0, select1);

		__m128 x2 = _mm_mul_ps(m_x, m_x);

		// Compute polynomial approximation
		const auto SC1 = SIMD::g_sinCoefficients1;
		auto vConstants = _mm_shuffle_ps(SC1, SC1, _MM_SHUFFLE(0, 0, 0, 0));
		__m128 Result = _mm_mul_ps(vConstants, x2);

		const auto SC0 = SIMD::g_sinCoefficients0;
		vConstants = _mm_shuffle_ps(SC0, SC0, _MM_SHUFFLE(3, 3, 3, 3));
		Result = _mm_add_ps(Result, vConstants);
		Result = _mm_mul_ps(Result, x2);

		vConstants = _mm_shuffle_ps(SC0, SC0, _MM_SHUFFLE(2, 2, 2, 2));
		Result = _mm_add_ps(Result, vConstants);
		Result = _mm_mul_ps(Result, x2);

		vConstants = _mm_shuffle_ps(SC0, SC0, _MM_SHUFFLE(1, 1, 1, 1));
		Result = _mm_add_ps(Result, vConstants);
		Result = _mm_mul_ps(Result, x2);

		vConstants = _mm_shuffle_ps(SC0, SC0, _MM_SHUFFLE(0, 0, 0, 0));
		Result = _mm_add_ps(Result, vConstants);
		Result = _mm_mul_ps(Result, x2);
		Result = _mm_add_ps(Result, VectorSIMD::One);
		Result = _mm_mul_ps(Result, m_x);
		return Result;
	}

	VectorSIMD VectorSIMD::Cos(VectorSIMD const& vec)
	{
		// Map V to m_x in [-pi,pi].
		auto m_x = VectorSIMD::AngleMod2Pi(vec);

		// Map in [-pi/2,pi/2] with cos(m_y) = sign*cos(m_x).
		auto sign = _mm_and_ps(m_x, SIMD::g_signMask);
		__m128 c = _mm_or_ps(VectorSIMD::Pi, sign); // pi when m_x >= 0, -pi when m_x < 0
		__m128 absx = _mm_andnot_ps(sign, m_x);     // |m_x|
		__m128 rflx = _mm_sub_ps(c, m_x);
		__m128 comp = _mm_cmple_ps(absx, VectorSIMD::PiDivTwo);
		__m128 select0 = _mm_and_ps(comp, m_x);
		__m128 select1 = _mm_andnot_ps(comp, rflx);
		m_x = _mm_or_ps(select0, select1);
		select0 = _mm_and_ps(comp, VectorSIMD::One);
		select1 = _mm_andnot_ps(comp, VectorSIMD::NegativeOne);
		sign = _mm_or_ps(select0, select1);

		__m128 x2 = _mm_mul_ps(m_x, m_x);

		// Compute polynomial approximation
		const auto CC1 = SIMD::g_cosCoefficients1;
		auto vConstants = _mm_shuffle_ps(CC1, CC1, _MM_SHUFFLE(0, 0, 0, 0));
		__m128 Result = _mm_mul_ps(vConstants, x2);

		const auto CC0 = SIMD::g_cosCoefficients0;
		vConstants = _mm_shuffle_ps(CC0, CC0, _MM_SHUFFLE(3, 3, 3, 3));
		Result = _mm_add_ps(Result, vConstants);
		Result = _mm_mul_ps(Result, x2);

		vConstants = _mm_shuffle_ps(CC0, CC0, _MM_SHUFFLE(2, 2, 2, 2));
		Result = _mm_add_ps(Result, vConstants);
		Result = _mm_mul_ps(Result, x2);

		vConstants = _mm_shuffle_ps(CC0, CC0, _MM_SHUFFLE(1, 1, 1, 1));
		Result = _mm_add_ps(Result, vConstants);
		Result = _mm_mul_ps(Result, x2);

		vConstants = _mm_shuffle_ps(CC0, CC0, _MM_SHUFFLE(0, 0, 0, 0));
		Result = _mm_add_ps(Result, vConstants);
		Result = _mm_mul_ps(Result, x2);
		Result = _mm_add_ps(Result, VectorSIMD::One);
		Result = _mm_mul_ps(Result, sign);
		return Result;
	}


	void VectorSIMD::SinCos(VectorSIMD& sin, VectorSIMD& cos, VectorSIMD const& angle)
	{
		// Force the value within the bounds of pi
		auto m_x = VectorSIMD::AngleMod2Pi(angle);
	
		// Map in [-pi/2,pi/2] with sin(m_y) = sin(m_x), cos(m_y) = sign*cos(m_x).
		auto sign = _mm_and_ps(m_x, SIMD::g_signMask);
		__m128 c = _mm_or_ps(VectorSIMD::Pi, sign); // pi when m_x >= 0, -pi when m_x < 0
		__m128 absx = _mm_andnot_ps(sign, m_x);     // |m_x|
		__m128 rflx = _mm_sub_ps(c, m_x);
		__m128 comp = _mm_cmple_ps(absx, VectorSIMD::PiDivTwo);
		__m128 select0 = _mm_and_ps(comp, m_x);
		__m128 select1 = _mm_andnot_ps(comp, rflx);
		m_x = _mm_or_ps(select0, select1);
		select0 = _mm_and_ps(comp, VectorSIMD::One);
		select1 = _mm_andnot_ps(comp, VectorSIMD::NegativeOne);
		sign = _mm_or_ps(select0, select1);
	
		__m128 x2 = _mm_mul_ps(m_x, m_x);
	
		// Compute polynomial approximation of sine
		const auto SC1 = SIMD::g_sinCoefficients1;
		auto vConstants = _mm_shuffle_ps(SC1, SC1, _MM_SHUFFLE(0, 0, 0, 0));
		__m128 Result = _mm_mul_ps(vConstants, x2);
	
		const auto SC0 = SIMD::g_sinCoefficients0;
		vConstants = _mm_shuffle_ps(SC0, SC0, _MM_SHUFFLE(3, 3, 3, 3));
		Result = _mm_add_ps(Result, vConstants);
		Result = _mm_mul_ps(Result, x2);
	
		vConstants = _mm_shuffle_ps(SC0, SC0, _MM_SHUFFLE(2, 2, 2, 2));
		Result = _mm_add_ps(Result, vConstants);
		Result = _mm_mul_ps(Result, x2);
	
		vConstants = _mm_shuffle_ps(SC0, SC0, _MM_SHUFFLE(1, 1, 1, 1));
		Result = _mm_add_ps(Result, vConstants);
		Result = _mm_mul_ps(Result, x2);
	
		vConstants = _mm_shuffle_ps(SC0, SC0, _MM_SHUFFLE(0, 0, 0, 0));
		Result = _mm_add_ps(Result, vConstants);
		Result = _mm_mul_ps(Result, x2);
		Result = _mm_add_ps(Result, VectorSIMD::One);
		Result = _mm_mul_ps(Result, m_x);
		sin = Result;
	
		// Compute polynomial approximation of cosine
		const auto CC1 = SIMD::g_cosCoefficients1;
		vConstants = _mm_shuffle_ps(CC1, CC1, _MM_SHUFFLE(0, 0, 0, 0));
		Result = _mm_mul_ps(vConstants, x2);
	
		const auto CC0 = SIMD::g_cosCoefficients0;
		vConstants = _mm_shuffle_ps(CC0, CC0, _MM_SHUFFLE(3, 3, 3, 3));
		Result = _mm_add_ps(Result, vConstants);
		Result = _mm_mul_ps(Result, x2);
	
		vConstants = _mm_shuffle_ps(CC0, CC0, _MM_SHUFFLE(2, 2, 2, 2));
		Result = _mm_add_ps(Result, vConstants);
		Result = _mm_mul_ps(Result, x2);
	
		vConstants = _mm_shuffle_ps(CC0, CC0, _MM_SHUFFLE(1, 1, 1, 1));
		Result = _mm_add_ps(Result, vConstants);
		Result = _mm_mul_ps(Result, x2);
	
		vConstants = _mm_shuffle_ps(CC0, CC0, _MM_SHUFFLE(0, 0, 0, 0));
		Result = _mm_add_ps(Result, vConstants);
		Result = _mm_mul_ps(Result, x2);
		Result = _mm_add_ps(Result, VectorSIMD::One);
		Result = _mm_mul_ps(Result, sign);
		cos = Result;
	}

	VectorSIMD VectorSIMD::Tan(VectorSIMD const& vec)
	{
		static const VectorSIMD tanCoefficients0 = { 1.0f, -4.667168334e-1f, 2.566383229e-2f, -3.118153191e-4f };
		static const VectorSIMD tanCoefficients1 = { 4.981943399e-7f, -1.333835001e-1f, 3.424887824e-3f, -1.786170734e-5f };
		static const VectorSIMD
			tanConstants = { 1.570796371f, 6.077100628e-11f, 0.000244140625f, 0.63661977228f /*2 / Pi*/};
		static const SIMD::UIntMask mask = { 0x1, 0x1, 0x1, 0x1 };
	
		VectorSIMD TwoDivPi = tanConstants.GetSplatW();
		VectorSIMD C0 = tanConstants.GetSplatX();
		VectorSIMD C1 = tanConstants.GetSplatY();
		VectorSIMD vEpsilon = tanConstants.GetSplatZ();
	
		VectorSIMD VA = (vec * TwoDivPi).Round();
		VectorSIMD VC = VectorSIMD::NegativeMultiplySubtract(VA, C0, vec);
		VectorSIMD VB = VA.GetAbs();
		VC = VectorSIMD::NegativeMultiplySubtract(VA, C1, VC);
		reinterpret_cast<__m128i*>(&VB)[0] = _mm_cvttps_epi32(VB);
	
		VectorSIMD VC2 = VC * VC;
		VectorSIMD T7 = tanCoefficients1.GetSplatW();
		VectorSIMD T6 = tanCoefficients1.GetSplatZ();
		VectorSIMD T4 = tanCoefficients1.GetSplatX();
		VectorSIMD T3 = tanCoefficients0.GetSplatW();
		VectorSIMD T5 = tanCoefficients1.GetSplatY();
		VectorSIMD T2 = tanCoefficients0.GetSplatZ();
		VectorSIMD T1 = tanCoefficients0.GetSplatY();
		VectorSIMD T0 = tanCoefficients0.GetSplatX();
	
		VectorSIMD VBIsEven = _mm_and_ps(VB, mask);
		VBIsEven = _mm_castsi128_ps(_mm_cmpeq_epi32(_mm_castps_si128(VBIsEven), _mm_castps_si128(VectorSIMD::Zero)));
	
		VectorSIMD N = VectorSIMD::MultiplyAdd(VC2, T7, T6);
		VectorSIMD D = VectorSIMD::MultiplyAdd(VC2, T4, T3);
		N = VectorSIMD::MultiplyAdd(VC2, N, T5);
		D = VectorSIMD::MultiplyAdd(VC2, D, T2);
		N = VC2 * N;
		D = VectorSIMD::MultiplyAdd(VC2, D, T1);
		N = VectorSIMD::MultiplyAdd(VC, N, VC);
		VectorSIMD VCNearZero = VC.InBounds(vEpsilon);
		D = VectorSIMD::MultiplyAdd(VC2, D, T0);
	
		N = VectorSIMD::Select(N, VC, VCNearZero);
		D = VectorSIMD::Select(D, VectorSIMD::One, VCNearZero);
	
		VectorSIMD R0 = N.GetNegated();
		VectorSIMD R1 = N / D;
		R0 = D / R0;
	
		VectorSIMD VIsZero = vec.EqualsZero();
		VectorSIMD Result = VectorSIMD::Select(R0, R1, VBIsEven);
		Result = VectorSIMD::Select(Result, Zero, VIsZero);
	
		return Result;
	}
	
	VectorSIMD VectorSIMD::ASin(VectorSIMD const& vec)
	{
		__m128 nonnegative = _mm_cmpge_ps(vec, VectorSIMD::Zero);
		__m128 mvalue = _mm_sub_ps(VectorSIMD::Zero, vec);
		__m128 m_x = _mm_max_ps(vec, mvalue); // |vec|
	
		// Compute (1-|vec|), clamp to zero to avoid sqrt of negative number.
		__m128 oneMValue = _mm_sub_ps(VectorSIMD::One, m_x);
		__m128 clampOneMValue = _mm_max_ps(VectorSIMD::Zero, oneMValue);
		__m128 root = _mm_sqrt_ps(clampOneMValue); // sqrt(1-|vec|)
	
		// Compute polynomial approximation
		const auto AC1 = SIMD::g_arcCoefficients1;
		auto vConstants = _mm_shuffle_ps(AC1, AC1, _MM_SHUFFLE(3, 3, 3, 3));
		__m128 t0 = _mm_mul_ps(vConstants, m_x);
	
		vConstants = _mm_shuffle_ps(AC1, AC1, _MM_SHUFFLE(2, 2, 2, 2));
		t0 = _mm_add_ps(t0, vConstants);
		t0 = _mm_mul_ps(t0, m_x);
	
		vConstants = _mm_shuffle_ps(AC1, AC1, _MM_SHUFFLE(1, 1, 1, 1));
		t0 = _mm_add_ps(t0, vConstants);
		t0 = _mm_mul_ps(t0, m_x);
	
		vConstants = _mm_shuffle_ps(AC1, AC1, _MM_SHUFFLE(0, 0, 0, 0));
		t0 = _mm_add_ps(t0, vConstants);
		t0 = _mm_mul_ps(t0, m_x);
	
		const auto AC0 = SIMD::g_arcCoefficients0;
		vConstants = _mm_shuffle_ps(AC0, AC0, _MM_SHUFFLE(3, 3, 3, 3));
		t0 = _mm_add_ps(t0, vConstants);
		t0 = _mm_mul_ps(t0, m_x);
	
		vConstants = _mm_shuffle_ps(AC0, AC0, _MM_SHUFFLE(2, 2, 2, 2));
		t0 = _mm_add_ps(t0, vConstants);
		t0 = _mm_mul_ps(t0, m_x);
	
		vConstants = _mm_shuffle_ps(AC0, AC0, _MM_SHUFFLE(1, 1, 1, 1));
		t0 = _mm_add_ps(t0, vConstants);
		t0 = _mm_mul_ps(t0, m_x);
	
		vConstants = _mm_shuffle_ps(AC0, AC0, _MM_SHUFFLE(0, 0, 0, 0));
		t0 = _mm_add_ps(t0, vConstants);
		t0 = _mm_mul_ps(t0, root);
	
		__m128 t1 = _mm_sub_ps(VectorSIMD::Pi, t0);
		t0 = _mm_and_ps(nonnegative, t0);
		t1 = _mm_andnot_ps(nonnegative, t1);
		t0 = _mm_or_ps(t0, t1);
		t0 = _mm_sub_ps(VectorSIMD::PiDivTwo, t0);
		return t0;
	}
	
	VectorSIMD VectorSIMD::ACos(VectorSIMD const& vec)
	{
		__m128 nonnegative = _mm_cmpge_ps(vec, VectorSIMD::Zero);
		__m128 mvalue = _mm_sub_ps(VectorSIMD::Zero, vec);
		__m128 m_x = _mm_max_ps(vec, mvalue); // |vec|
	
		// Compute (1-|vec|), clamp to zero to avoid sqrt of negative number.
		__m128 oneMValue = _mm_sub_ps(VectorSIMD::One, m_x);
		__m128 clampOneMValue = _mm_max_ps(VectorSIMD::Zero, oneMValue);
		__m128 root = _mm_sqrt_ps(clampOneMValue); // sqrt(1-|vec|)
	
		// Compute polynomial approximation
		const auto AC1 = SIMD::g_arcCoefficients1;
		auto vConstants = _mm_shuffle_ps(AC1, AC1, _MM_SHUFFLE(3, 3, 3, 3));
		__m128 t0 = _mm_mul_ps(vConstants, m_x);
	
		vConstants = _mm_shuffle_ps(AC1, AC1, _MM_SHUFFLE(2, 2, 2, 2));
		t0 = _mm_add_ps(t0, vConstants);
		t0 = _mm_mul_ps(t0, m_x);
	
		vConstants = _mm_shuffle_ps(AC1, AC1, _MM_SHUFFLE(1, 1, 1, 1));
		t0 = _mm_add_ps(t0, vConstants);
		t0 = _mm_mul_ps(t0, m_x);
	
		vConstants = _mm_shuffle_ps(AC1, AC1, _MM_SHUFFLE(0, 0, 0, 0));
		t0 = _mm_add_ps(t0, vConstants);
		t0 = _mm_mul_ps(t0, m_x);
	
		const auto AC0 = SIMD::g_arcCoefficients0;
		vConstants = _mm_shuffle_ps(AC0, AC0, _MM_SHUFFLE(3, 3, 3, 3));
		t0 = _mm_add_ps(t0, vConstants);
		t0 = _mm_mul_ps(t0, m_x);
	
		vConstants = _mm_shuffle_ps(AC0, AC0, _MM_SHUFFLE(2, 2, 2, 2));
		t0 = _mm_add_ps(t0, vConstants);
		t0 = _mm_mul_ps(t0, m_x);
	
		vConstants = _mm_shuffle_ps(AC0, AC0, _MM_SHUFFLE(1, 1, 1, 1));
		t0 = _mm_add_ps(t0, vConstants);
		t0 = _mm_mul_ps(t0, m_x);
	
		vConstants = _mm_shuffle_ps(AC0, AC0, _MM_SHUFFLE(0, 0, 0, 0));
		t0 = _mm_add_ps(t0, vConstants);
		t0 = _mm_mul_ps(t0, root);
	
		__m128 t1 = _mm_sub_ps(VectorSIMD::Pi, t0);
		t0 = _mm_and_ps(nonnegative, t0);
		t1 = _mm_andnot_ps(nonnegative, t1);
		t0 = _mm_or_ps(t0, t1);
		return t0;
	}
	
	VectorSIMD VectorSIMD::ATan(VectorSIMD const& vec)
	{
		__m128 absV = vec.GetAbs();
		__m128 invV = _mm_div_ps(VectorSIMD::One, vec);
		__m128 comp = _mm_cmpgt_ps(vec, VectorSIMD::One);
		__m128 select0 = _mm_and_ps(comp, VectorSIMD::One);
		__m128 select1 = _mm_andnot_ps(comp, VectorSIMD::NegativeOne);
		__m128 sign = _mm_or_ps(select0, select1);
		comp = _mm_cmple_ps(absV, VectorSIMD::One);
		select0 = _mm_and_ps(comp, VectorSIMD::Zero);
		select1 = _mm_andnot_ps(comp, sign);
		sign = _mm_or_ps(select0, select1);
		select0 = _mm_and_ps(comp, vec);
		select1 = _mm_andnot_ps(comp, invV);
		__m128 m_x = _mm_or_ps(select0, select1);
	
		__m128 x2 = _mm_mul_ps(m_x, m_x);
	
		// Compute polynomial approximation
		VectorSIMD const TC1 = SIMD::g_aTanCoefficients1;
		VectorSIMD vConstants = _mm_shuffle_ps(TC1, TC1, _MM_SHUFFLE(3, 3, 3, 3));
		__m128 Result = _mm_mul_ps(vConstants, x2);
	
		vConstants = _mm_shuffle_ps(TC1, TC1, _MM_SHUFFLE(2, 2, 2, 2));
		Result = _mm_add_ps(Result, vConstants);
		Result = _mm_mul_ps(Result, x2);
	
		vConstants = _mm_shuffle_ps(TC1, TC1, _MM_SHUFFLE(1, 1, 1, 1));
		Result = _mm_add_ps(Result, vConstants);
		Result = _mm_mul_ps(Result, x2);
	
		vConstants = _mm_shuffle_ps(TC1, TC1, _MM_SHUFFLE(0, 0, 0, 0));
		Result = _mm_add_ps(Result, vConstants);
		Result = _mm_mul_ps(Result, x2);
	
		VectorSIMD const TC0 = SIMD::g_aTanCoefficients0;
		vConstants = _mm_shuffle_ps(TC0, TC0, _MM_SHUFFLE(3, 3, 3, 3));
		Result = _mm_add_ps(Result, vConstants);
		Result = _mm_mul_ps(Result, x2);
	
		vConstants = _mm_shuffle_ps(TC0, TC0, _MM_SHUFFLE(2, 2, 2, 2));
		Result = _mm_add_ps(Result, vConstants);
		Result = _mm_mul_ps(Result, x2);
	
		vConstants = _mm_shuffle_ps(TC0, TC0, _MM_SHUFFLE(1, 1, 1, 1));
		Result = _mm_add_ps(Result, vConstants);
		Result = _mm_mul_ps(Result, x2);
	
		vConstants = _mm_shuffle_ps(TC0, TC0, _MM_SHUFFLE(0, 0, 0, 0));
		Result = _mm_add_ps(Result, vConstants);
		Result = _mm_mul_ps(Result, x2);
		Result = _mm_add_ps(Result, VectorSIMD::One);
		Result = _mm_mul_ps(Result, m_x);
		__m128 result1 = _mm_mul_ps(sign, VectorSIMD::PiDivTwo);
		result1 = _mm_sub_ps(result1, Result);
	
		comp = _mm_cmpeq_ps(sign, VectorSIMD::Zero);
		select0 = _mm_and_ps(comp, Result);
		select1 = _mm_andnot_ps(comp, result1);
		Result = _mm_or_ps(select0, select1);
		return Result;
	}
	
	VectorSIMD VectorSIMD::ATan2(VectorSIMD const& Y, VectorSIMD const& X)
	{
		VectorSIMD ATanResultValid = VectorSIMD(SIMD::g_trueMask);
	
		VectorSIMD vPi = VectorSIMD(SIMD::g_aTan2Constants).GetSplatX();
		VectorSIMD vPiOverTwo = VectorSIMD(SIMD::g_aTan2Constants).GetSplatY();
		VectorSIMD vPiOverFour = VectorSIMD(SIMD::g_aTan2Constants).GetSplatZ();
		VectorSIMD vThreePiOverFour = VectorSIMD(SIMD::g_aTan2Constants).GetSplatW();
	
		VectorSIMD YEqualsZero = Y.EqualsZero();
		VectorSIMD XEqualsZero = X.EqualsZero();
		VectorSIMD XIsPositive = _mm_and_ps(X, SIMD::g_signMask);
		XIsPositive = _mm_castsi128_ps(_mm_cmpeq_epi32(_mm_castps_si128(XIsPositive), _mm_castps_si128(VectorSIMD::Zero)));
		VectorSIMD YEqualsInfinity = Y.EqualsInfinity();
		VectorSIMD XEqualsInfinity = X.EqualsInfinity();
	
		VectorSIMD YSign = _mm_and_ps(Y, SIMD::g_signMask);
		vPi = _mm_castsi128_ps(_mm_or_si128(_mm_castps_si128(vPi), _mm_castps_si128(YSign)));
		vPiOverTwo = _mm_castsi128_ps(_mm_or_si128(_mm_castps_si128(vPiOverTwo), _mm_castps_si128(YSign)));
		vPiOverFour = _mm_castsi128_ps(_mm_or_si128(_mm_castps_si128(vPiOverFour), _mm_castps_si128(YSign)));
		vThreePiOverFour = _mm_castsi128_ps(_mm_or_si128(_mm_castps_si128(vThreePiOverFour), _mm_castps_si128(YSign)));
	
		VectorSIMD R1 = VectorSIMD::Select(vPi, YSign, XIsPositive);
		VectorSIMD R2 = VectorSIMD::Select(ATanResultValid, vPiOverTwo, XEqualsZero);
		VectorSIMD R3 = VectorSIMD::Select(R2, R1, YEqualsZero);
		VectorSIMD R4 = VectorSIMD::Select(vThreePiOverFour, vPiOverFour, XIsPositive);
		VectorSIMD R5 = VectorSIMD::Select(vPiOverTwo, R4, XEqualsInfinity);
		VectorSIMD Result = VectorSIMD::Select(R3, R5, YEqualsInfinity);
		ATanResultValid = _mm_castsi128_ps(_mm_cmpeq_epi32(_mm_castps_si128(Result), _mm_castps_si128(ATanResultValid)));
	
		VectorSIMD V = Y / X;
		VectorSIMD R0 = VectorSIMD::ATan(V);
		R1 = VectorSIMD::Select(vPi, VectorSIMD(SIMD::g_signMask), XIsPositive);
		R2 = R0 + R1;
	
		return VectorSIMD::Select(Result, R2, ATanResultValid);
	}
	
	VectorSIMD VectorSIMD::SinEst(VectorSIMD const& vec)
	{
		// Force the value within the bounds of pi
		auto m_x = VectorSIMD::AngleMod2Pi(vec);
	
		// Map in [-pi/2,pi/2] with sin(m_y) = sin(m_x).
		__m128 sign = _mm_and_ps(m_x, SIMD::g_signMask);
		__m128 c = _mm_or_ps(VectorSIMD::Pi, sign); // pi when m_x >= 0, -pi when m_x < 0
		__m128 absx = _mm_andnot_ps(sign, m_x);     // |m_x|
		__m128 rflx = _mm_sub_ps(c, m_x);
		__m128 comp = _mm_cmple_ps(absx, VectorSIMD::PiDivTwo);
		__m128 select0 = _mm_and_ps(comp, m_x);
		__m128 select1 = _mm_andnot_ps(comp, rflx);
		m_x = _mm_or_ps(select0, select1);
	
		__m128 x2 = _mm_mul_ps(m_x, m_x);
	
		// Compute polynomial approximation
		const auto SEC = SIMD::g_sinCoefficients1;
		auto vConstants = _mm_shuffle_ps(SEC, SEC, _MM_SHUFFLE(3, 3, 3, 3));
		__m128 Result = _mm_mul_ps(vConstants, x2);
	
		vConstants = _mm_shuffle_ps(SEC, SEC, _MM_SHUFFLE(2, 2, 2, 2));
		Result = _mm_add_ps(Result, vConstants);
		Result = _mm_mul_ps(Result, x2);
	
		vConstants = _mm_shuffle_ps(SEC, SEC, _MM_SHUFFLE(1, 1, 1, 1));
		Result = _mm_add_ps(Result, vConstants);
		Result = _mm_mul_ps(Result, x2);
	
		Result = _mm_add_ps(Result, VectorSIMD::One);
		Result = _mm_mul_ps(Result, m_x);
		return Result;
	}
	
	VectorSIMD VectorSIMD::CosEst(VectorSIMD const& vec)
	{
		// Map V to m_x in [-pi,pi].
		auto m_x = VectorSIMD::AngleMod2Pi(vec);
	
		// Map in [-pi/2,pi/2] with cos(m_y) = sign*cos(m_x).
		auto sign = _mm_and_ps(m_x, SIMD::g_signMask);
		__m128 c = _mm_or_ps(VectorSIMD::Pi, sign); // pi when m_x >= 0, -pi when m_x < 0
		__m128 absx = _mm_andnot_ps(sign, m_x);     // |m_x|
		__m128 rflx = _mm_sub_ps(c, m_x);
		__m128 comp = _mm_cmple_ps(absx, VectorSIMD::PiDivTwo);
		__m128 select0 = _mm_and_ps(comp, m_x);
		__m128 select1 = _mm_andnot_ps(comp, rflx);
		m_x = _mm_or_ps(select0, select1);
		select0 = _mm_and_ps(comp, VectorSIMD::One);
		select1 = _mm_andnot_ps(comp, VectorSIMD::NegativeOne);
		sign = _mm_or_ps(select0, select1);
	
		__m128 x2 = _mm_mul_ps(m_x, m_x);
	
		// Compute polynomial approximation
		const auto CEC = SIMD::g_cosCoefficients1;
		auto vConstants = _mm_shuffle_ps(CEC, CEC, _MM_SHUFFLE(3, 3, 3, 3));
		__m128 Result = _mm_mul_ps(vConstants, x2);
	
		vConstants = _mm_shuffle_ps(CEC, CEC, _MM_SHUFFLE(2, 2, 2, 2));
		Result = _mm_add_ps(Result, vConstants);
		Result = _mm_mul_ps(Result, x2);
	
		vConstants = _mm_shuffle_ps(CEC, CEC, _MM_SHUFFLE(1, 1, 1, 1));
		Result = _mm_add_ps(Result, vConstants);
		Result = _mm_mul_ps(Result, x2);
	
		Result = _mm_add_ps(Result, VectorSIMD::One);
		Result = _mm_mul_ps(Result, sign);
		return Result;
	}
	
	VectorSIMD VectorSIMD::TanEst(VectorSIMD const& vec)
	{
		VectorSIMD W = VectorSIMD(SIMD::g_tanEstCoefficients).GetSplatW();
		VectorSIMD V1 = (vec * W).Round();
		V1 = VectorSIMD::NegativeMultiplySubtract(VectorSIMD::Pi, V1, vec);
	
		VectorSIMD const T0 = VectorSIMD(SIMD::g_tanEstCoefficients).GetSplatX();
		VectorSIMD const T1 = VectorSIMD(SIMD::g_tanEstCoefficients).GetSplatY();
		VectorSIMD const T2 = VectorSIMD(SIMD::g_tanEstCoefficients).GetSplatZ();
	
		auto V2T2 = VectorSIMD::NegativeMultiplySubtract(V1, V1, T2);
		auto V2 = V1 * V1;
		auto V1T0 = V1 * T0;
		auto V1T1 = V1 * T1;
	
		auto N = VectorSIMD::MultiplyAdd(V2, V1T1, V1T0);
		auto D = V2T2.GetInverseEst();
		return N * D;
	}
	
	VectorSIMD VectorSIMD::ASinEst(VectorSIMD const& vec)
	{
		__m128 nonnegative = _mm_cmpge_ps(vec, VectorSIMD::Zero);
		__m128 mvalue = _mm_sub_ps(VectorSIMD::Zero, vec);
		__m128 m_x = _mm_max_ps(vec, mvalue); // |vec|
	
		// Compute (1-|vec|), clamp to zero to avoid sqrt of negative number.
		__m128 oneMValue = _mm_sub_ps(VectorSIMD::One, m_x);
		__m128 clampOneMValue = _mm_max_ps(VectorSIMD::Zero, oneMValue);
		__m128 root = _mm_sqrt_ps(clampOneMValue); // sqrt(1-|vec|)
	
		// Compute polynomial approximation
		const auto AEC = SIMD::g_arcEstCoefficients;
		auto vConstants = _mm_shuffle_ps(AEC, AEC, _MM_SHUFFLE(3, 3, 3, 3));
		__m128 t0 = _mm_mul_ps(vConstants, m_x);
	
		vConstants = _mm_shuffle_ps(AEC, AEC, _MM_SHUFFLE(2, 2, 2, 2));
		t0 = _mm_add_ps(t0, vConstants);
		t0 = _mm_mul_ps(t0, m_x);
	
		vConstants = _mm_shuffle_ps(AEC, AEC, _MM_SHUFFLE(1, 1, 1, 1));
		t0 = _mm_add_ps(t0, vConstants);
		t0 = _mm_mul_ps(t0, m_x);
	
		vConstants = _mm_shuffle_ps(AEC, AEC, _MM_SHUFFLE(0, 0, 0, 0));
		t0 = _mm_add_ps(t0, vConstants);
		t0 = _mm_mul_ps(t0, root);
	
		__m128 t1 = _mm_sub_ps(VectorSIMD::Pi, t0);
		t0 = _mm_and_ps(nonnegative, t0);
		t1 = _mm_andnot_ps(nonnegative, t1);
		t0 = _mm_or_ps(t0, t1);
		t0 = _mm_sub_ps(VectorSIMD::PiDivTwo, t0);
		return t0;
	}
	
	VectorSIMD VectorSIMD::ACosEst(VectorSIMD const& vec)
	{
		__m128 nonnegative = _mm_cmpge_ps(vec, VectorSIMD::Zero);
		__m128 mvalue = _mm_sub_ps(VectorSIMD::Zero, vec);
		__m128 m_x = _mm_max_ps(vec, mvalue); // |vec|
	
		// Compute (1-|vec|), clamp to zero to avoid sqrt of negative number.
		__m128 oneMValue = _mm_sub_ps(VectorSIMD::One, m_x);
		__m128 clampOneMValue = _mm_max_ps(VectorSIMD::Zero, oneMValue);
		__m128 root = _mm_sqrt_ps(clampOneMValue); // sqrt(1-|vec|)
	
		// Compute polynomial approximation
		auto vConstants = _mm_shuffle_ps(SIMD::g_arcEstCoefficients, SIMD::g_arcEstCoefficients, _MM_SHUFFLE(3, 3, 3, 3));
		__m128 t0 = _mm_mul_ps(vConstants, m_x);
	
		vConstants = _mm_shuffle_ps(SIMD::g_arcEstCoefficients, SIMD::g_arcEstCoefficients, _MM_SHUFFLE(2, 2, 2, 2));
		t0 = _mm_add_ps(t0, vConstants);
		t0 = _mm_mul_ps(t0, m_x);
	
		vConstants = _mm_shuffle_ps(SIMD::g_arcEstCoefficients, SIMD::g_arcEstCoefficients, _MM_SHUFFLE(1, 1, 1, 1));
		t0 = _mm_add_ps(t0, vConstants);
		t0 = _mm_mul_ps(t0, m_x);
	
		vConstants = _mm_shuffle_ps(SIMD::g_arcEstCoefficients, SIMD::g_arcEstCoefficients, _MM_SHUFFLE(0, 0, 0, 0));
		t0 = _mm_add_ps(t0, vConstants);
		t0 = _mm_mul_ps(t0, root);
	
		__m128 t1 = _mm_sub_ps(VectorSIMD::Pi, t0);
		t0 = _mm_and_ps(nonnegative, t0);
		t1 = _mm_andnot_ps(nonnegative, t1);
		t0 = _mm_or_ps(t0, t1);
		return t0;
	}
	
	VectorSIMD VectorSIMD::ATanEst(VectorSIMD const& vec)
	{
		__m128 absV = vec.GetAbs();
		__m128 invV = _mm_div_ps(VectorSIMD::One, vec);
		__m128 comp = _mm_cmpgt_ps(vec, VectorSIMD::One);
		__m128 select0 = _mm_and_ps(comp, VectorSIMD::One);
		__m128 select1 = _mm_andnot_ps(comp, VectorSIMD::NegativeOne);
		__m128 sign = _mm_or_ps(select0, select1);
		comp = _mm_cmple_ps(absV, VectorSIMD::One);
		select0 = _mm_and_ps(comp, VectorSIMD::Zero);
		select1 = _mm_andnot_ps(comp, sign);
		sign = _mm_or_ps(select0, select1);
		select0 = _mm_and_ps(comp, vec);
		select1 = _mm_andnot_ps(comp, invV);
		__m128 m_x = _mm_or_ps(select0, select1);
	
		__m128 x2 = _mm_mul_ps(m_x, m_x);
	
		// Compute polynomial approximation
		VectorSIMD const AEC = SIMD::g_aTanEstCoefficients1;
		VectorSIMD vConstants = _mm_shuffle_ps(AEC, AEC, _MM_SHUFFLE(3, 3, 3, 3));
		__m128 Result = _mm_mul_ps(vConstants, x2);
	
		vConstants = _mm_shuffle_ps(AEC, AEC, _MM_SHUFFLE(2, 2, 2, 2));
		Result = _mm_add_ps(Result, vConstants);
		Result = _mm_mul_ps(Result, x2);
	
		vConstants = _mm_shuffle_ps(AEC, AEC, _MM_SHUFFLE(1, 1, 1, 1));
		Result = _mm_add_ps(Result, vConstants);
		Result = _mm_mul_ps(Result, x2);
	
		vConstants = _mm_shuffle_ps(AEC, AEC, _MM_SHUFFLE(0, 0, 0, 0));
		Result = _mm_add_ps(Result, vConstants);
		Result = _mm_mul_ps(Result, x2);
	
		// ATanEstCoefficients0 is already splatted
		Result = _mm_add_ps(Result, SIMD::g_aTanEstCoefficients0);
		Result = _mm_mul_ps(Result, m_x);
		__m128 result1 = _mm_mul_ps(sign, VectorSIMD::PiDivTwo);
		result1 = _mm_sub_ps(result1, Result);
	
		comp = _mm_cmpeq_ps(sign, VectorSIMD::Zero);
		select0 = _mm_and_ps(comp, Result);
		select1 = _mm_andnot_ps(comp, result1);
		Result = _mm_or_ps(select0, select1);
		return Result;
	}
	
	VectorSIMD VectorSIMD::ATan2Est(VectorSIMD const& X, VectorSIMD const& Y)
	{
		VectorSIMD ATanResultValid = VectorSIMD(SIMD::g_trueMask);
	
		VectorSIMD vPi = VectorSIMD(SIMD::g_aTan2Constants).GetSplatX();
		VectorSIMD vPiOverTwo = VectorSIMD(SIMD::g_aTan2Constants).GetSplatY();
		VectorSIMD vPiOverFour = VectorSIMD(SIMD::g_aTan2Constants).GetSplatZ();
		VectorSIMD vThreePiOverFour = VectorSIMD(SIMD::g_aTan2Constants).GetSplatW();
	
		VectorSIMD YEqualsZero = Y.EqualsZero();
		VectorSIMD XEqualsZero = X.EqualsZero();
		VectorSIMD XIsPositive = _mm_and_ps(X, SIMD::g_signMask);
		XIsPositive = _mm_castsi128_ps(_mm_cmpeq_epi32(_mm_castps_si128(XIsPositive), _mm_castps_si128(VectorSIMD::Zero)));
		VectorSIMD YEqualsInfinity = Y.EqualsInfinity();
		VectorSIMD XEqualsInfinity = X.EqualsInfinity();
	
		VectorSIMD YSign = _mm_and_ps(Y, SIMD::g_signMask);
		vPi = _mm_castsi128_ps(_mm_or_si128(_mm_castps_si128(vPi), _mm_castps_si128(YSign)));
		vPiOverTwo = _mm_castsi128_ps(_mm_or_si128(_mm_castps_si128(vPiOverTwo), _mm_castps_si128(YSign)));
		vPiOverFour = _mm_castsi128_ps(_mm_or_si128(_mm_castps_si128(vPiOverFour), _mm_castps_si128(YSign)));
		vThreePiOverFour = _mm_castsi128_ps(_mm_or_si128(_mm_castps_si128(vThreePiOverFour), _mm_castps_si128(YSign)));
	
		VectorSIMD R1 = VectorSIMD::Select(vPi, YSign, XIsPositive);
		VectorSIMD R2 = VectorSIMD::Select(ATanResultValid, vPiOverTwo, XEqualsZero);
		VectorSIMD R3 = VectorSIMD::Select(R2, R1, YEqualsZero);
		VectorSIMD R4 = VectorSIMD::Select(vThreePiOverFour, vPiOverFour, XIsPositive);
		VectorSIMD R5 = VectorSIMD::Select(vPiOverTwo, R4, XEqualsInfinity);
		VectorSIMD Result = VectorSIMD::Select(R3, R5, YEqualsInfinity);
		ATanResultValid = _mm_castsi128_ps(_mm_cmpeq_epi32(_mm_castps_si128(Result), _mm_castps_si128(ATanResultValid)));
	
		VectorSIMD Reciprocal = X.GetInverseEst();
		VectorSIMD V = Y * Reciprocal;
		VectorSIMD R0 = VectorSIMD::ATanEst(V);
	
		R1 = VectorSIMD::Select(vPi, VectorSIMD(SIMD::g_signMask), XIsPositive);
		R2 = R0 + R1;
		Result = VectorSIMD::Select(Result, R2, ATanResultValid);
	
		return Result;
	}
	
	VectorSIMD VectorSIMD::AngleMod2Pi(VectorSIMD const& angles)
	{
		// Modulo the range of the given angles such that -Pi <= Angles < Pi
		VectorSIMD result = _mm_mul_ps(angles, VectorSIMD::OneDivTwoPi);
		result.Round();
		result = _mm_mul_ps(result, VectorSIMD::TwoPi);
		result = _mm_sub_ps(angles, result);
		return result;
	}
	
	//-------------------------------------------------------------------------

	VectorSIMD &VectorSIMD::Normalize2()
	{
		// Perform the dot product on m_x and m_y only
		auto vLengthSq = _mm_mul_ps(data, data);
		auto vTemp = _mm_shuffle_ps(vLengthSq, vLengthSq, _MM_SHUFFLE(1, 1, 1, 1));
		vLengthSq = _mm_add_ss(vLengthSq, vTemp);
		vLengthSq = _mm_shuffle_ps(vLengthSq, vLengthSq, _MM_SHUFFLE(0, 0, 0, 0));
		// Prepare for the division
		auto vResult = _mm_sqrt_ps(vLengthSq);
		// Create zero with a single instruction
		auto vZeroMask = _mm_setzero_ps();
		// Test for a divide by zero (Must be FP to detect -0.0)
		vZeroMask = _mm_cmpneq_ps(vZeroMask, vResult);
		// Failsafe on zero (Or epsilon) length planes
		// If the length is infinity, set the elements to zero
		vLengthSq = _mm_cmpneq_ps(vLengthSq, VectorSIMD::Infinity);
		// Divide to perform the normalization
		vResult = _mm_div_ps(data, vResult);
		// Any that are infinity, set to zero
		vResult = _mm_and_ps(vResult, vZeroMask);
		// Select qnan or result based on infinite length
		auto vTemp1 = _mm_andnot_ps(vLengthSq, VectorSIMD::QNaN);
		auto vTemp2 = _mm_and_ps(vResult, vLengthSq);
		data = _mm_or_ps(vTemp1, vTemp2);
	
		*this = Select(*this, VectorSIMD::Zero, Select0011);
	
		return *this;
	}
	
	VectorSIMD VectorSIMD::GetNormalized2() const
	{
		VectorSIMD v = *this;
		v.Normalize2();
		return v;
	}
	
	VectorSIMD &VectorSIMD::Normalize3()
	{
		// Perform the dot product on m_x,m_y and m_z only
		auto vLengthSq = _mm_mul_ps(data, data);
		auto vTemp = _mm_shuffle_ps(vLengthSq, vLengthSq, _MM_SHUFFLE(2, 1, 2, 1));
		vLengthSq = _mm_add_ss(vLengthSq, vTemp);
		vTemp = _mm_shuffle_ps(vTemp, vTemp, _MM_SHUFFLE(1, 1, 1, 1));
		vLengthSq = _mm_add_ss(vLengthSq, vTemp);
		vLengthSq = _mm_shuffle_ps(vLengthSq, vLengthSq, _MM_SHUFFLE(0, 0, 0, 0));
		// Prepare for the division
		auto vResult = _mm_sqrt_ps(vLengthSq);
		// Create zero with a single instruction
		auto vZeroMask = _mm_setzero_ps();
		// Test for a divide by zero (Must be FP to detect -0.0)
		vZeroMask = _mm_cmpneq_ps(vZeroMask, vResult);
		// Failsafe on zero (Or epsilon) length planes
		// If the length is infinity, set the elements to zero
		vLengthSq = _mm_cmpneq_ps(vLengthSq, VectorSIMD::Infinity);
		// Divide to perform the normalization
		vResult = _mm_div_ps(data, vResult);
		// Any that are infinity, set to zero
		vResult = _mm_and_ps(vResult, vZeroMask);
		// Select qnan or result based on infinite length
		auto vTemp1 = _mm_andnot_ps(vLengthSq, VectorSIMD::QNaN);
		auto vTemp2 = _mm_and_ps(vResult, vLengthSq);
		data = _mm_or_ps(vTemp1, vTemp2);
	
		*this = Select(*this, VectorSIMD::Zero, Select0001);
	
		return *this;
	}
	
	VectorSIMD VectorSIMD::GetNormalized3() const
	{
		VectorSIMD v = *this;
		v.Normalize3();
		return v;
	}
	
	VectorSIMD &VectorSIMD::Normalize4()
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
		// Prepare for the division
		auto vResult = _mm_sqrt_ps(vLengthSq);
		// Create zero with a single instruction
		auto vZeroMask = _mm_setzero_ps();
		// Test for a divide by zero (Must be FP to detect -0.0)
		vZeroMask = _mm_cmpneq_ps(vZeroMask, vResult);
		// Failsafe on zero (Or epsilon) length planes
		// If the length is infinity, set the elements to zero
		vLengthSq = _mm_cmpneq_ps(vLengthSq, VectorSIMD::Infinity);
		// Divide to perform the normalization
		vResult = _mm_div_ps(data, vResult);
		// Any that are infinity, set to zero
		vResult = _mm_and_ps(vResult, vZeroMask);
		// Select qnan or result based on infinite length
		auto vTemp1 = _mm_andnot_ps(vLengthSq, VectorSIMD::QNaN);
		auto vTemp2 = _mm_and_ps(vResult, vLengthSq);
		data = _mm_or_ps(vTemp1, vTemp2);
	
		return *this;
	}
	
	VectorSIMD VectorSIMD::GetNormalized4() const
	{
		VectorSIMD v = *this;
		v.Normalize4();
		return v;
	}
	
	VectorSIMD &VectorSIMD::Floor()
	{
		VectorSIMD result;
	
		// To handle NAN, INF and numbers greater than 8388608, use masking
		__m128i vTest = _mm_and_si128(_mm_castps_si128(data), SIMD::g_absMask);
		vTest = _mm_cmplt_epi32(vTest, SIMD::g_noFraction);
		// Truncate
		__m128i vInt = _mm_cvttps_epi32(data);
		result = _mm_cvtepi32_ps(vInt);
		__m128 vLarger = _mm_cmpgt_ps(result, data);
		// 0 -> 0, 0xffffffff -> -1.0f
		vLarger = _mm_cvtepi32_ps(_mm_castps_si128(vLarger));
		result = _mm_add_ps(result, vLarger);
		// All numbers less than 8388608 will use the round to int
		result = _mm_and_ps(result, _mm_castsi128_ps(vTest));
		// All others, use the ORIGINAL value
		vTest = _mm_andnot_si128(vTest, _mm_castps_si128(data));
		result = _mm_or_ps(result, _mm_castsi128_ps(vTest));
	
		data = result;
		return *this;
	}
	
	VectorSIMD VectorSIMD::GetFloor() const
	{
		VectorSIMD v = *this;
		v.Floor();
		return v;
	}
	
	VectorSIMD &VectorSIMD::Ceil()
	{
		VectorSIMD result;
	
		// To handle NAN, INF and numbers greater than 8388608, use masking
		__m128i vTest = _mm_and_si128(_mm_castps_si128(data), SIMD::g_absMask);
		vTest = _mm_cmplt_epi32(vTest, SIMD::g_noFraction);
		// Truncate
		__m128i vInt = _mm_cvttps_epi32(data);
		result = _mm_cvtepi32_ps(vInt);
		__m128 vSmaller = _mm_cmplt_ps(result, data);
		// 0 -> 0, 0xffffffff -> -1.0f
		vSmaller = _mm_cvtepi32_ps(_mm_castps_si128(vSmaller));
		result = _mm_sub_ps(result, vSmaller);
		// All numbers less than 8388608 will use the round to int
		result = _mm_and_ps(result, _mm_castsi128_ps(vTest));
		// All others, use the ORIGINAL value
		vTest = _mm_andnot_si128(vTest, _mm_castps_si128(data));
		result = _mm_or_ps(result, _mm_castsi128_ps(vTest));
	
		data = result;
		return *this;
	}
	
	VectorSIMD VectorSIMD::GetCeil() const
	{
		VectorSIMD v = *this;
		v.Ceil();
		return v;
	}
	
	VectorSIMD &VectorSIMD::Round()
	{
		__m128 sign = _mm_and_ps(data, SIMD::g_signMask);
		__m128 sMagic = _mm_or_ps(SIMD::g_noFraction, sign);
		__m128 R1 = _mm_add_ps(data, sMagic);
		R1 = _mm_sub_ps(R1, sMagic);
		__m128 R2 = _mm_and_ps(data, SIMD::g_absMask);
		__m128 mask = _mm_cmple_ps(R2, SIMD::g_noFraction);
		R2 = _mm_andnot_ps(mask, data);
		R1 = _mm_and_ps(R1, mask);
		data = _mm_xor_ps(R1, R2);
		return *this;
	}
	
	VectorSIMD VectorSIMD::GetRound() const
	{
		VectorSIMD v = *this;
		v.Round();
		return v;
	}
	
	VectorSIMD VectorSIMD::GetSign() const
	{
		VectorSIMD const selectMask = GreaterThanEqual(VectorSIMD::Zero);
		return VectorSIMD::Select(VectorSIMD::NegativeOne, VectorSIMD::One, selectMask);
	}
	
	//-------------------------------------------------------------------------
	
	// Copied from DirectX Math
	
	bool VectorSIMD::SolveCubic(float e, float f, float g, float& t, float& u, float& v)
	{
		float p, q, h, rc, d, theta, costh3, sinth3;
	
		p = f - e * e / 3.0f;
		q = g - e * f / 3.0f + e * e * e * 2.0f / 27.0f;
		h = q * q / 4.0f + p * p * p / 27.0f;
	
		if (h > 0)
		{
			t = u = v = 0.f;
			return false; // only one real root
		}
	
		if ((h == 0) && (q == 0)) // all the same root
		{
			t = -e / 3;
			u = -e / 3;
			v = -e / 3;
	
			return true;
		}
	
		d = sqrtf(q * q / 4.0f - h);
		if (d < 0)
		{
			rc = -powf(-d, 1.0f / 3.0f);
		}
		else
		{
			rc = powf(d, 1.0f / 3.0f);
		}
	
		theta = Math::ACos(-q / (2.0f * d));
		costh3 = Math::Cos(theta / 3.0f);
		sinth3 = sqrtf(3.0f) * Math::Sin(theta / 3.0f);
	
		t = 2.0f * rc * costh3 - e / 3.0f;
		u = -rc * (costh3 + sinth3) - e / 3.0f;
		v = -rc * (costh3 - sinth3) - e / 3.0f;
	
		return true;
	}
	
	VectorSIMD VectorSIMD::CalculateEigenVector(float m11, float m12, float m13, float m22, float m23, float m33, float e)
	{
		Float3 fTmp;
		fTmp.Raw[0] = m12 * m23 - m13 * (m22 - e);
		fTmp.Raw[1] = m13 * m12 - m23 * (m11 - e);
		fTmp.Raw[2] = (m11 - e) * (m22 - e) - m12 * m12;
	
		if (fTmp.IsZero()) // planar or linear
		{
			float f1, f2, f3;
	
			// we only have one equation - find a valid one
			if ((m11 - e != 0) || (m12 != 0) || (m13 != 0))
			{
				f1 = m11 - e;
				f2 = m12;
				f3 = m13;
			}
			else if ((m12 != 0) || (m22 - e != 0) || (m23 != 0))
			{
				f1 = m12;
				f2 = m22 - e;
				f3 = m23;
			}
			else if ((m13 != 0) || (m23 != 0) || (m33 - e != 0))
			{
				f1 = m13;
				f2 = m23;
				f3 = m33 - e;
			}
			else
			{
				// error, we'll just make something up - we have NO context
				f1 = 1.0f;
				f2 = 0.0f;
				f3 = 0.0f;
			}
	
			if (f1 == 0)
			{
				fTmp.x = 0.0f;
			}
			else
			{
				fTmp.x = 1.0f;
			}
	
			if (f2 == 0)
			{
				fTmp.y = 0.0f;
			}
			else
			{
				fTmp.y = 1.0f;
			}
	
			if (f3 == 0)
			{
				fTmp.z = 0.0f;
	
				// recalculate y to make equation work
				if (m12 != 0)
				{
					fTmp.y = -f1 / f2;
				}
			}
			else
			{
				fTmp.z = (f2 - f1) / f3;
			}
		}
	
		//-------------------------------------------------------------------------
	
		VectorSIMD vTmp(fTmp);
	
		// Multiply by a value large enough to make the vector non-zero.
		if (vTmp.LengthSquared3().GetX() <= 1e-5f)
		{
			vTmp = vTmp * 1e5f;
		}
	
		return vTmp.GetNormalized3();
	}
	
	bool VectorSIMD::CalculateEigenVectors(float m11,
		float m12,
		float m13,
		float m22,
		float m23,
		float m33,
		float e1,
		float e2,
		float e3,
		VectorSIMD& V1,
		VectorSIMD& V2,
		VectorSIMD& V3) noexcept
	{
		V1 = CalculateEigenVector(m11, m12, m13, m22, m23, m33, e1);
		V2 = CalculateEigenVector(m11, m12, m13, m22, m23, m33, e2);
		V3 = CalculateEigenVector(m11, m12, m13, m22, m23, m33, e3);
	
		bool v1z = false;
		bool v2z = false;
		bool v3z = false;
	
		VectorSIMD Zero = VectorSIMD::Zero;
	
		if (V1.IsZero3())
		{
			v1z = true;
		}
	
		if (V2.IsZero3())
		{
			v2z = true;
		}
	
		if (V3.IsZero3())
		{
			v3z = true;
		}
	
		bool e12 = (Math::Abs(VectorSIMD::Dot3(V1, V2).GetX()) > 0.1f); // check for non-orthogonal vectors
		bool e13 = (Math::Abs(VectorSIMD::Dot3(V1, V3).GetX()) > 0.1f);
		bool e23 = (Math::Abs(VectorSIMD::Dot3(V2, V3).GetX()) > 0.1f);
	
		if ((v1z && v2z && v3z) || (e12 && e13 && e23) ||
			(e12 && v3z) || (e13 && v2z) || (e23 && v1z)) // all eigenvectors are 0- any basis set
		{
			V1 = VectorSIMD::UnitX;
			V2 = VectorSIMD::UnitY;
			V3 = VectorSIMD::UnitZ;
			return true;
		}
	
		if (v1z && v2z)
		{
			VectorSIMD vTmp = VectorSIMD::Cross3(VectorSIMD::UnitY, V3);
			if (vTmp.LengthSquared3().GetX() < 1e-5f)
			{
				vTmp = VectorSIMD::Cross3(VectorSIMD::UnitX, V3);
			}
			V1 = vTmp.GetNormalized3();
			V2 = VectorSIMD::Cross3(V3, V1);
			return true;
		}
	
		if (v3z && v1z)
		{
			VectorSIMD vTmp = VectorSIMD::Cross3(VectorSIMD::UnitY, V2);
			if (vTmp.LengthSquared3().GetX() < 1e-5f)
			{
				vTmp = VectorSIMD::Cross3(VectorSIMD::UnitX, V2);
			}
			V3 = vTmp.GetNormalized3();
			V1 = VectorSIMD::Cross3(V2, V3);
			return true;
		}
	
		if (v2z && v3z)
		{
			VectorSIMD vTmp = VectorSIMD::Cross3(VectorSIMD::UnitY, V1);
			if (vTmp.LengthSquared3().GetX() < 1e-5f)
			{
				vTmp = VectorSIMD::Cross3(VectorSIMD::UnitX, V1);
			}
			V2 = vTmp.GetNormalized3();
			V3 = VectorSIMD::Cross3(V1, V2);
			return true;
		}
	
		if ((v1z) || e12)
		{
			V1 = VectorSIMD::Cross3(V2, V3);
			return true;
		}
	
		if ((v2z) || e23)
		{
			V2 = VectorSIMD::Cross3(V3, V1);
			return true;
		}
	
		if ((v3z) || e13)
		{
			V3 = VectorSIMD::Cross3(V1, V2);
			return true;
		}
	
		return true;
	}
	
	bool VectorSIMD::CalculateEigenVectorsFromCovarianceMatrix(float Cxx,
		float Cyy,
		float Czz,
		float Cxy,
		float Cxz,
		float Cyz,
		VectorSIMD& V1,
		VectorSIMD& V2,
		VectorSIMD& V3)
	{
		// Calculate the eigenvalues by solving a cubic equation.
		float e = -(Cxx + Cyy + Czz);
		float f = Cxx * Cyy + Cyy * Czz + Czz * Cxx - Cxy * Cxy - Cxz * Cxz - Cyz * Cyz;
		float g = Cxy * Cxy * Czz + Cxz * Cxz * Cyy + Cyz * Cyz * Cxx - Cxy * Cyz * Cxz * 2.0f - Cxx * Cyy * Czz;
	
		float ev1, ev2, ev3;
		if (!SolveCubic(e, f, g, ev1, ev2, ev3))
		{
			// set them to arbitrary orthonormal basis set
			V1 = VectorSIMD::UnitX;
			V2 = VectorSIMD::UnitY;
			V3 = VectorSIMD::UnitZ;
			return false;
		}
	
		return CalculateEigenVectors(Cxx, Cxy, Cxz, Cyy, Cyz, Czz, ev1, ev2, ev3, V1, V2, V3);
	}

}