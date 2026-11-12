#pragma once

#include "Runtime/API.h"
#include "Runtime/Core/Types/Collections/List.h"
#include "Runtime/Core/Types/Strings/String.h"
#include "Runtime/ShaderCompilation/Slang/Artifact/ShaderArtifact.h"

namespace SE
{
	class SE_API_RUNTIME ShaderArtifactReader
	{
	public:
		static bool Read(const byte* data, int32 length, ShaderArtifact& artifact, String& error);
		static bool Read(const List<byte>& data, ShaderArtifact& artifact, String& error);
		static bool ReadAndValidate(const byte* data, int32 length, String& error);
		static bool ReadAndValidate(const List<byte>& data, String& error);
	};
}
