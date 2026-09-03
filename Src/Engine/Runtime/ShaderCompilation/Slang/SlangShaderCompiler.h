#pragma once

#include "Runtime/API.h"
#include "Runtime/Core/Types/Collections/List.h"
#include "Runtime/Core/Types/Strings/String.h"
#include "Runtime/ShaderCompilation/ShaderCompileTypes.h"
#include "Runtime/ShaderCompilation/Slang/ShaderVariantPlanner.h"

namespace SE
{
	struct SE_API_RUNTIME SlangProgramStageDeclaration
	{
        ShaderStage Stage = ShaderStage::Max;
		String EntryPoint;
	};

	struct SE_API_RUNTIME SlangProgramDeclaration
	{
		String ProgramId;
		List<SlangProgramStageDeclaration> Stages;
		List<ShaderVariantGroup> VariantGroups;
	};

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
