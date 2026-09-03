#pragma once

#include "Runtime/API.h"
#include "Runtime/Core/Types/Strings/String.h"
#include "ShaderVar.h"

namespace SE
{
	class ShaderProgramReflection;

	class SE_API_RUNTIME ShaderNameResolver
	{
	public:
		static bool Resolve(const ShaderProgramReflection& reflection, const String& path, ShaderVarLocation& location);
		static bool ResolveMember(const ShaderProgramReflection& reflection, const ShaderVarLocation& source, const String& name, ShaderVarLocation& location);
		static bool ResolveIndex(const ShaderProgramReflection& reflection, const ShaderVarLocation& source, uint32 index, ShaderVarLocation& location);
	};
}
