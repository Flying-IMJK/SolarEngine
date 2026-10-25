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
		static bool Resolve(const ShaderProgramReflection& reflection, const String& path, ShaderVarLocation& location, String& msg);
		// ConstantBuffer 目标允许空 NamePath 表示根 Global ConstantBuffer；普通变量解析仍拒绝空路径。
		static bool ResolveUniformBlock(const ShaderProgramReflection& reflection, const String& path, ShaderVarLocation& location, String& msg);
		static bool ResolveMember(const ShaderProgramReflection& reflection, const ShaderVarLocation& source, const String& name, ShaderVarLocation& location, String& msg);
		static bool ResolveIndex(const ShaderProgramReflection& reflection, const ShaderVarLocation& source, uint32 index, ShaderVarLocation& location, String& msg);
	};
}
