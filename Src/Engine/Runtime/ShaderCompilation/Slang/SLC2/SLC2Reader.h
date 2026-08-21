#pragma once

#include "Runtime/API.h"
#include "Runtime/Core/Types/Collections/List.h"
#include "Runtime/Core/Types/Strings/String.h"

namespace SE
{
	class SE_API_RUNTIME SLC2Reader
	{
	public:
		static bool ReadAndValidate(const byte* data, int32 length, String& error);
		static bool ReadAndValidate(const List<byte>& data, String& error);
	};
}
