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
	enum class VertexInputRate : byte
	{
		PerVertex,
	};

	// Shader 数值类型使用受控枚举保存，避免运行时依赖任意字符串解释类型。
	enum class VertexInputType : byte
	{
		Float,
		Float2,
		Float3,
		Float4,
		Int,
		Int2,
		Int3,
		Int4,
		UInt,
		UInt2,
		UInt3,
		UInt4,
		Half,
		Half2,
		Half3,
		Half4,
	};

	struct SE_API_RUNTIME VertexInputSignatureElement
	{
		/// <summary>
        /// 输入语义名称 例如 POSITION, NORMAL, TEXCOORD 等。语义名称大写。
		/// </summary>
		String Semantic;
		/// <summary>
		/// 输入语义名称索引 例如 TEXCOORD0, TEXCOORD1, TEXCOORD2
		/// </summary>
		uint32 SemanticIndex = 0;
		/// <summary>
        /// 表示在Shader 中定义的输入位置索引，
		/// struct VertexInput
        /// {
        ///     float3 Position : POSITION0;
        ///     float2 UV : TEXCOORD0;
        /// };
		/// Position  -> Location 0
		///		  UV  -> Location 1
		/// </summary>
		uint32 Location = 0;
		VertexInputType ShaderType = VertexInputType::Float3;
	};

	struct SE_API_RUNTIME ShaderStageRecord
	{
        ShaderStage Stage = ShaderStage::Max;
        StringAnsi  EntryPoint;
		// 逻辑签名只对 Vertex Stage 有效；合法的 system-value-only 入口使用空数组。
		List<VertexInputSignatureElement> VertexInputSignature;
		List<byte> Code;
		int32 OutputControlPoints = 0;
	};

	struct SE_API_RUNTIME ShaderVariantRecord
	{
		StringAnsi Variant;
		ShaderReflectionIR Layout;
		List<ShaderStageRecord> Stages;
	};

	struct SE_API_RUNTIME ShaderTargetRecord
	{
		ShaderProfile Target;
		List<ShaderVariantRecord> Variants;
	};

	struct SE_API_RUNTIME ShaderProgramRecord
	{
		String ProgramId;
		List<ShaderVariantGroup> VariantGroups;
		List<ShaderTargetRecord> Targets;
	};

	struct SE_API_RUNTIME ShaderArtifact
	{
		String Format = SE_TEXT("SLC2");
		uint32 Version = 4;
		String CompilerBuildTag;
		List<ShaderProgramRecord> Programs;
	};


	struct SE_API_RUNTIME ShaderProgramStageDeclaration
    {
        ShaderStage Stage = ShaderStage::Max;
        StringAnsi  EntryPoint;
    };

    struct SE_API_RUNTIME ShaderProgramDeclaration
    {
        String                            ProgramId;
        List<ShaderProgramStageDeclaration> Stages;
        List<ShaderVariantGroup>          VariantGroups;
    };
}
