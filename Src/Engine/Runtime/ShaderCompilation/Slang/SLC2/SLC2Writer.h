#pragma once

#include "Runtime/API.h"
#include "Runtime/Core/Types/Collections/List.h"
#include "Runtime/ShaderCompilation/Slang/SLC2/SLC2Artifact.h"

namespace SE
{
	class SE_API_RUNTIME SLC2Writer
	{
	public:
		static bool WriteDeterministic(const SLC2Artifact& artifact, List<byte>& output, String& error);
	};
}
