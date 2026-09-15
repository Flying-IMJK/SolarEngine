#pragma once

#include "Runtime/API.h"
#include "Runtime/Core/Types/Collections/List.h"
#include "Runtime/Core/Types/Strings/String.h"
#include "Runtime/ShaderCompilation/ShaderCompileTypes.h"
#include "Runtime/ShaderCompilation/Slang/SLC2/SLC2Artifact.h"
#include "Runtime/ShaderCompilation/Slang/ShaderVariantPlanner.h"

namespace SE
{
	class SE_API_RUNTIME SlangShaderCompiler
	{
	public:
		ShaderCompileResult Compile(const ShaderCompileRequest& request);

	private:
		String m_Diagnostics;

	private:
		void AddDiagnostic(const String& text);
		void AddSlangDiagnostics(void* diagnosticsBlob);
	};
}
