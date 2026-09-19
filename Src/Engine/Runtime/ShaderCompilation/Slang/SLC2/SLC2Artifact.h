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
	};

	struct SE_API_RUNTIME SLC2VertexInputSignatureElement
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
		List<ShaderVariantGroup> VariantGroups;
		List<SLC2TargetRecord> Targets;
	};

	struct SE_API_RUNTIME SLC2Artifact
	{
		String Format = SE_TEXT("SLC2");
		uint32 Version = 4;
		String CompilerBuildTag;
		List<SLC2ProgramRecord> Programs;
	};


	struct SE_API_RUNTIME SLC2ProgramStageDeclaration
    {
        ShaderStage Stage = ShaderStage::Max;
        String      EntryPoint;
    };

    struct SE_API_RUNTIME SLC2ProgramDeclaration
    {
        String                             ProgramId;
        List<SLC2ProgramStageDeclaration> Stages;
        List<ShaderVariantGroup>           VariantGroups;
    };
}
