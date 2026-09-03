#pragma once

#include "Runtime/API.h"
#include "Runtime/Core/Types/Collections/List.h"
#include "Runtime/Core/Types/Strings/String.h"
#include "Runtime/ShaderCompilation/Slang/ShaderReflectionIR.h"
#include "Runtime/ShaderCompilation/ShaderCompileTypes.h"
#include "Runtime/ShaderCompilation/Slang/ShaderVariantPlanner.h"

namespace SE
{
	struct SE_API_RUNTIME SLC2StageRecord
	{
        ShaderStage Stage = ShaderStage::Max;
		String EntryPoint;
		List<byte> Code;
	};

	struct SE_API_RUNTIME SLC2VariantRecord
	{
		String Variant;
		ShaderReflectionIR Layout;
		List<SLC2StageRecord> Stages;
	};

	struct SE_API_RUNTIME SLC2TargetRecord
	{
		ShaderCompileTarget Target;
		String TargetKey;
		List<SLC2VariantRecord> Variants;
	};

	struct SE_API_RUNTIME SLC2ProgramRecord
	{
		String ProgramId;
		List<ShaderVariantGroup> VariantGroups;
		List<SLC2TargetRecord> Targets;
	};

	struct SE_API_RUNTIME SLC2Artifact
	{
		String Format = SE_TEXT("SLC2");
		uint32 Version = 2;
		String CompilerBuildTag;
		List<SLC2ProgramRecord> Programs;
	};
}
