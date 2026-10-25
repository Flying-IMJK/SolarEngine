#pragma once

#include "Runtime/API.h"
#include "Runtime/Core/Types/Collections/List.h"
#include "Runtime/Core/Types/Strings/String.h"
#include "Runtime/Graphics/Base/GPUEnums.h"

namespace SE
{
	struct SE_API_RUNTIME ShaderCompileTarget
	{
		ShaderProfile Profile = ShaderProfile::Unknown;
		FeatureLevel Feature = FeatureLevel::ES2;
	};

	struct SE_API_RUNTIME ShaderVariantRequest
	{
		List<String> Defines;
	};

	struct SE_API_RUNTIME ShaderProgramVariantSelection
	{
		String ProgramId;
		List<ShaderVariantRequest> Variants;
	};

	struct SE_API_RUNTIME ShaderCompileRequest
	{
		String ShaderName;
		String SourcePath;
		char* SourceCode;
        int SourceCodeLength;
		List<ShaderCompileTarget> Targets;
		List<ShaderProgramVariantSelection> VariantSelections;
	};



	struct SE_API_RUNTIME ShaderCompileResult
	{
        bool       Success;
        String     CompileMessage;
		List<byte> SLC2Data;
		List<String> SourceDependencies;
	};

	SE_API_RUNTIME String BuildTargetKey(const ShaderCompileTarget& target);
}
