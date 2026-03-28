#pragma once

// Include Free Type library
// Source: https://www.freetype.org
#include <ft2build.h>
#include FT_FREETYPE_H

#include "Core/Math/Math.h"

namespace SE
{
	// Logs the free type error
	#define LOG_FT_ERROR(error) LOG_ERROR("FreeType", "FreeType error '{0:#x}' at {1}:{2}", error, SE_TEXT(__FILE__), __LINE__)


	// Convert the given value from 26.6 space into rounded pixel space
	template<typename ReturnType, typename InputType>
	inline ReturnType Convert26Dot6ToRoundedPixel(InputType value)
	{
		return static_cast<ReturnType>(Math::RoundToInt(value / 64.0f));
	}

	// Convert the given value from pixel space into 26.6 space
	template<typename ReturnType, typename InputType>
	inline ReturnType ConvertPixelTo26Dot6(InputType value)
	{
		return static_cast<ReturnType>(value * 64);
	}

	// Convert the given value from pixel space into 16.16 space
	template<typename ReturnType, typename InputType>
	inline ReturnType ConvertPixelTo16Dot16(InputType value)
	{
		return static_cast<ReturnType>(value * 65536);
	}
}
