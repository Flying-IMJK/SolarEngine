#pragma once

#include "Runtime/API.h"
#include "Runtime/Core/Types/Collections/List.h"
#include "Runtime/Core/Types/Strings/String.h"

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
	};

	enum class ShaderGraphicsBackend
	{
		Unknown,
		Vulkan,
		DirectX,
	};

	enum class ShaderCompileShaderModel
	{
		Unknown,
		SM_5_0,
		SM_6_0,
		SM_6_6,
	};

	enum class SlangShaderStage
	{
		Unknown,
		Vertex,
		Hull,
		Domain,
		Geometry,
		Pixel,
		Compute,
	};

	struct SE_API_RUNTIME ShaderCompileTarget
	{
		ShaderTargetPlatform Platform = ShaderTargetPlatform::Unknown;
		ShaderGraphicsBackend Backend = ShaderGraphicsBackend::Unknown;
		ShaderCompileShaderModel ShaderModel = ShaderCompileShaderModel::Unknown;
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
	SE_API_RUNTIME String ToString(const ShaderGraphicsBackend value);
	SE_API_RUNTIME String ToString(const ShaderCompileShaderModel value);
	SE_API_RUNTIME String ToString(const SlangShaderStage value);

	SE_API_RUNTIME bool ParseSlangShaderStage(const char* text, SlangShaderStage& stage);
	SE_API_RUNTIME String BuildTargetKey(const ShaderCompileTarget& target);
}
