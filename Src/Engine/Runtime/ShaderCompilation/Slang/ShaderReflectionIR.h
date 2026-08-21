#pragma once

#include "Runtime/API.h"
#include "Runtime/Core/Types/Collections/List.h"
#include "Runtime/Core/Types/Strings/String.h"
#include "Runtime/ShaderCompilation/ShaderCompileTypes.h"

namespace SE
{
	struct SE_API_RUNTIME ShaderIRMember
	{
		String Name;
		uint32 TypeId = 0;
		// 成员偏移只相对直接父类型，运行时按 BindingPath 逐层累加到最终位置。
		uint32 UniformOffset = 0;
		uint32 ResourceRangeOffset = 0;
		uint32 ResourceIndexOffset = 0;
	};

	// 描述类型内部声明的逻辑资源范围，不等价于 Vulkan/DX 的物理 descriptor 绑定点。
	struct SE_API_RUNTIME ShaderIRResourceRange
	{
		String ResourceKind;
		uint32 Count = 0;
		uint32 BaseIndex = 0;
		String InternalRole;
		uint32 OwnerRangeOffset = 0;
	};

	// 后端无关的类型记录，用于运行时按名称路径重建 Shader 参数树。
	struct SE_API_RUNTIME ShaderIRTypeRecord
	{
		uint32 Id = 0;
		String Kind;
		String Name;
		uint32 UniformByteSize = 0;
		uint32 ElementTypeId = 0;
		uint32 ElementCount = 0;
		uint32 ElementByteStride = 0;
		String ResourceKind;
		String Shape;
		String Access;
		List<ShaderIRMember> Members;
		List<ShaderIRResourceRange> ResourceRanges;
	};

	// 当前 Target 下已经展开的物理绑定范围，字段必须保持稳定文本，避免序列化 Slang 原生枚举。
	struct SE_API_RUNTIME ShaderIRDescriptorRange
	{
		uint32 Set = 0;
		uint32 Binding = 0;
		uint32 DescriptorCount = 0;
		String DescriptorType;
		String StageMask;
	};

	// ParameterBlock 记录一次具体出现位置；即使 ElementTypeId 相同，也不能在运行时合并。
	struct SE_API_RUNTIME ShaderIRParameterBlock
	{
		uint32 Id = 0;
		String Name;
		uint32 ElementTypeId = 0;
		uint32 UniformByteSize = 0;
		List<ShaderIRDescriptorRange> DescriptorRanges;
	};

	// SLC2 中保存的运行时反射输入；运行时只读取该 IR，不再调用 Slang 或 SPIR-V 反射。
	struct SE_API_RUNTIME ShaderReflectionIR
	{
		uint32 RootBlockId = 0;
		String PipelineLayoutFingerprint;
		List<ShaderIRTypeRecord> Types;
		List<ShaderIRParameterBlock> ParameterBlocks;
	};
}
