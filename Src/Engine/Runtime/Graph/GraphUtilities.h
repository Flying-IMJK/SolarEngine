#pragma once
#include "Core/Math/Curves.h"
#include "Core/Serialization/TextWriter.h"
#include "Runtime/Render/Assets/Material/MaterialParams.h"
#include "Runtime/Utilities/Variant.h"

namespace SE::GraphUtilities {

	typedef float (*MathOp1)(float);
	typedef float (*MathOp2)(float, float);
	typedef float (*MathOp3)(float, float, float);

	void ApplySomeMathHere(Variant& v, Variant& a, MathOp1 op);
	void ApplySomeMathHere(Variant& v, Variant& a, Variant& b, MathOp2 op);
	void ApplySomeMathHere(Variant& v, Variant& a, Variant& b, Variant& c, MathOp3 op);

	void ApplySomeMathHere(uint16 typeId, Variant& v, Variant& a);
	void ApplySomeMathHere(uint16 typeId, Variant& v, Variant& a, Variant& b);

	int32 CountComponents(VariantTypes type);


	void GenerateShaderConstantBuffer(TextWriterUnicode& writer, List<SerializedMaterialParam>& parameters);
	const Char* GenerateShaderResources(TextWriterUnicode& writer, List<SerializedMaterialParam>& parameters, int32 startRegister);
	const Char* GenerateSamplers(TextWriterUnicode& writer, List<SerializedMaterialParam>& parameters, int32 startRegister);
	template<typename T>
	void SampleCurve(TextWriterUnicode& writer, const Math::BezierCurve<T>& curve, const String& time, const String& value);

} // SE

