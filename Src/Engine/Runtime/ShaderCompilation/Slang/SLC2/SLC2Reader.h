#pragma once

#include "Runtime/API.h"
#include "Runtime/Core/Types/Collections/List.h"
#include "Runtime/Core/Types/Strings/String.h"
#include "Runtime/ShaderCompilation/Slang/SLC2/SLC2Artifact.h"

namespace SE
{
	class SE_API_RUNTIME SLC2Reader
	{
	public:
		static bool Read(const byte* data, int32 length, SLC2Artifact& artifact, String& error);
		static bool Read(const List<byte>& data, SLC2Artifact& artifact, String& error);
		static bool ReadAndValidate(const byte* data, int32 length, String& error);
		static bool ReadAndValidate(const List<byte>& data, String& error);
	};
}
