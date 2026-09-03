#pragma once

#include "Runtime/API.h"
#include "Runtime/Core/Types/Collections/List.h"
#include "Reflection/ShaderVar.h"
#include "ShaderBindingSnapshot.h"

namespace SE
{
	class ShaderProgramReflection;

	struct SE_API_RUNTIME ShaderRuntimeResourceSlot
	{
		uint32 RangeIndex = 0;
		uint32 ResourceIndex = 0;
		ShaderRuntimeResourceType Type = ShaderRuntimeResourceType::Texture;
		void* Value = nullptr;
	};

	// 一个具体 blockId 的可变状态；子块由 rangeBinding.SubBlockId 唯一确定。
	// 这里同时保存普通 uniform 字节数据和显式资源槽位，最终统一导出到 ShaderBindingSnapshot。
	class SE_API_RUNTIME ShaderParameterBlock
	{
	public:
		~ShaderParameterBlock();

		bool Initialize(const ShaderProgramReflection& reflection, uint32 blockId);
		bool SetResource(const ShaderVarLocation& location, ShaderRuntimeResourceType type, void* value);
		bool SetUniform(const ShaderVarLocation& location, const void* data, uint32 size);
		bool AppendResources(ShaderBindingSnapshot& snapshot) const;
		void AppendUniformData(ShaderBindingSnapshot& snapshot) const;

		uint32 GetBlockId() const;
		ShaderParameterBlock* FindSubBlock(uint32 blockId);
		const ShaderParameterBlock* FindSubBlock(uint32 blockId) const;
		ShaderParameterBlock* FindBlock(uint32 blockId);
		const ShaderParameterBlock* FindBlock(uint32 blockId) const;

	private:
		void Clear();
		ShaderRuntimeResourceSlot* FindResourceSlot(uint32 rangeIndex, uint32 resourceIndex);

		const ShaderProgramReflection* m_Reflection = nullptr;
		uint32 m_BlockId = 0;
		List<byte> m_UniformData;
		List<ShaderRuntimeResourceSlot> m_Resources;
		List<ShaderParameterBlock*> m_SubBlocks;
	};
}
