#pragma once

namespace SE
{
	enum class TypeIDCore
	{
		Invalid = -1,

		Bool = 0,
		Uint8,
		Int8,
		Uint16,
		Int16,
		Uint32,
		Int32,
		Uint64,
		Int64,
		Float,
		Double,
		UUID,
		StringID,
		TypeID,
		String,
		Color,
		Float2,
		Float3,
		Float4,
		Int2,
		Int3,
		Int4,
		Double2,
		Double3,
		Double4,
		Quaternion,
		Transform,
		Matrix,
		Microseconds,
		Milliseconds,
		Seconds,
		FloatRange,
		FloatCurve,

		TBitFlags,
		TBitMovedFlags,

		List,
		NumTypes,
	};

}
