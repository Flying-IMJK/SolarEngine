#pragma once

#include "Runtime/API.h"
#include "Runtime/Core/Types/Strings/String.h"
#include "Runtime/ShaderCompilation/ShaderCompileTypes.h"
#include "Runtime/ShaderCompilation/Slang/SLC2/SLC2Artifact.h"

#include <slang.h>

namespace SE
{
	class SE_API_RUNTIME SlangShaderCompiler
	{
	public:
		ShaderCompileResult Compile(const ShaderCompileRequest& request);

	private:

		bool BuildReflection(slang::IComponentType*   linked,
                             const List<ShaderStage>& stages,
                             SlangInt                 targetIndex,
                             slang::ProgramLayout*    layout,
                             ShaderReflectionIR&      output,
                             String&                  error);

		void AddDiagnostic(const String& text);
		void AddSlangDiagnostics(void* diagnosticsBlob);

		String m_Diagnostics;
	};
}
