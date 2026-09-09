#pragma once

#include "Runtime/API.h"
#include "Runtime/Core/Types/Collections/List.h"
#include "Runtime/Core/Types/Strings/String.h"
#include "Runtime/ShaderCompilation/Slang/ShaderReflectionIR.h"
#include "Runtime/ShaderCompilation/ShaderCompileTypes.h"
#include "Runtime/ShaderCompilation/Slang/ShaderVariantPlanner.h"

namespace SE
{
	// 顶点输入频率是跨后端缓存语义，不能直接保存 VkVertexInputRate 等平台枚举。
	enum class SLC2VertexInputRate : byte
	{
		PerVertex,
	};

	// Shader 数值类型使用受控枚举保存，避免运行时依赖任意字符串解释类型。
	enum class SLC2VertexInputType : byte
	{
		Float3,
	};

	struct SE_API_RUNTIME SLC2VertexBufferBinding
	{
		uint32 Slot = 0;
		uint32 Stride = 0;
		SLC2VertexInputRate InputRate = SLC2VertexInputRate::PerVertex;
		uint32 InstanceStepRate = 0;
	};

	struct SE_API_RUNTIME SLC2VertexInputElement
	{
		String Semantic;
		uint32 SemanticIndex = 0;
		PixelFormat Format = PixelFormat::Undefined;
		uint32 Slot = 0;
		uint32 Offset = 0;
	};

	struct SE_API_RUNTIME SLC2VertexBufferLayout
	{
		// 物理布局属于 Program，并在全部 Target/Variant 间保持稳定。
		List<SLC2VertexBufferBinding> Bindings;
		List<SLC2VertexInputElement> Elements;
	};

	struct SE_API_RUNTIME SLC2VertexInputSignatureElement
	{
		String Semantic;
		uint32 SemanticIndex = 0;
		uint32 Location = 0;
		SLC2VertexInputType ShaderType = SLC2VertexInputType::Float3;
	};

	struct SE_API_RUNTIME SLC2StageRecord
	{
        ShaderStage Stage = ShaderStage::Max;
		String EntryPoint;
		// 逻辑签名只对 Vertex Stage 有效；合法的 system-value-only 入口使用空数组。
		List<SLC2VertexInputSignatureElement> VertexInputSignature;
		List<byte> Code;
		int32 OutputControlPoints = 0;
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
		SLC2VertexBufferLayout VertexBufferLayout;
		List<ShaderVariantGroup> VariantGroups;
		List<SLC2TargetRecord> Targets;
	};

	struct SE_API_RUNTIME SLC2Artifact
	{
		String Format = SE_TEXT("SLC2");
		uint32 Version = 3;
		String CompilerBuildTag;
		List<SLC2ProgramRecord> Programs;
	};
}
