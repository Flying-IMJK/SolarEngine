#pragma once


#include "TextProcessing.h"

namespace SE::ShaderParser
{
	typedef TextProcessing Reader;
	typedef Reader::Token Token;
	typedef Reader::SeparatorData Separator;
	// Don't count ending '\0' character
	#define MACRO_LENGTH(macro) (ARRAY_SIZE(macro) - 1)
}

