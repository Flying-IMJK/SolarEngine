#pragma once

#include "Runtime/API.h"
#include "Runtime/Core/Types/Strings/String.h"
#include "Runtime/ShaderCompilation/Slang/ShaderReflectionIR.h"

#include <slang.h>

namespace SE
{
	class SE_API_RUNTIME SlangReflectionBuilder
	{
	public:
		bool Build(
			const String& programId,
			const String& targetKey,
			const String& variant,
			slang::ProgramLayout* layout,
			ShaderReflectionIR& output,
			String& error);
	};
}
