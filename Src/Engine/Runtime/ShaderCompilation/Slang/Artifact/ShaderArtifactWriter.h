#pragma once

#include "Runtime/API.h"
#include "Runtime/Core/Types/Collections/List.h"
#include "Runtime/ShaderCompilation/Slang/Artifact/ShaderArtifact.h"

namespace SE
{
	class SE_API_RUNTIME ShaderArtifactWriter
	{
	public:
		static bool WriteDeterministic(const ShaderArtifact& artifact, List<byte>& output, String& error);
	};
}
