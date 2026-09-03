#pragma once

#include "Runtime/API.h"
#include "Runtime/Core/Types/Collections/List.h"
#include "Runtime/Core/Types/Strings/String.h"
#include "Runtime/Graphics/Base/GPUEnums.h"

namespace SE
{
	enum class ShaderCompileStatus
	{
		Success,
		Failed,
	};

	enum class ShaderTargetPlatform
	{
		Unknown,
		Windows,
		Linux,
		MacOS,
		PS4,
		PS5,
	};


	struct SE_API_RUNTIME ShaderCompileTarget
	{
		ShaderTargetPlatform Platform = ShaderTargetPlatform::Unknown;
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
		String SourceCode;
		List<ShaderCompileTarget> Targets;
		List<ShaderProgramVariantSelection> VariantSelections;
	};

	struct SE_API_RUNTIME ShaderCompileMessage
	{
		String Text;
	};

	struct SE_API_RUNTIME ShaderCompileResult
	{
		ShaderCompileStatus Status = ShaderCompileStatus::Failed;
		ShaderCompileMessage CompileMessage;
		List<byte> SLC2Data;
	};

	SE_API_RUNTIME String ToString(const ShaderCompileStatus value);
	SE_API_RUNTIME String ToString(const ShaderTargetPlatform value);

	SE_API_RUNTIME String BuildTargetKey(const ShaderCompileTarget& target);
}
