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
		String Role;
		uint32 Set = 0;
		uint32 Binding = 0;
		uint32 ArrayElementBase = 0;
		uint32 LogicalElementStride = 1;
		uint32 DescriptorCount = 0;
		String DescriptorType;
		String StageMask;
		List<String> Flags;
	};

	// 一个逻辑 resource range 在当前 Target 下的唯一落点。simple 直接写 descriptor，
	// constantBuffer/parameterBlock 进入子块，两个分支不能同时出现。
	struct SE_API_RUNTIME ShaderIRRangeBinding
	{
		uint32 RangeIndex = 0;
		String Flavor;
		List<ShaderIRDescriptorRange> DescriptorRanges;
		int32 SubBlockId = -1;
	};

	// ParameterBlock 记录一次具体出现位置；即使 ElementTypeId 相同，也不能在运行时合并。
	struct SE_API_RUNTIME ShaderIRParameterBlock
	{
		uint32 Id = 0;
		String Name;
		uint32 ElementTypeId = 0;
		uint32 UniformByteSize = 0;
		bool HasDefaultUniformBuffer = false;
		ShaderIRDescriptorRange DefaultUniformBuffer;
		List<ShaderIRRangeBinding> RangeBindings;
	};

	// SLC2 中保存的运行时反射输入；运行时只读取该 IR。
	struct SE_API_RUNTIME ShaderReflectionIR
	{
		uint32 Schema = 2;
		uint32 RootBlockId = 0;
		String PipelineLayoutFingerprint;
		List<ShaderIRTypeRecord> Types;
		List<ShaderIRParameterBlock> ParameterBlocks;
	};

	// 不依赖 Slang/Vulkan 的规范物理布局身份。Builder、Reader 与运行时均使用这一入口。
	SE_API_RUNTIME String BuildPipelineLayoutFingerprint(const ShaderReflectionIR& layout);
	SE_API_RUNTIME String ComputeSHA256Hex(const byte* data, int32 length);
}
