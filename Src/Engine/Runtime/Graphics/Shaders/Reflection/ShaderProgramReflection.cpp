#include "ShaderProgramReflection.h"

namespace SE
{
	bool ShaderProgramReflection::Initialize(const ShaderReflectionIR& layout)
	{
		if (layout.Schema != 2 || layout.RootBlockId >= static_cast<uint32>(layout.ParameterBlocks.Count()) || layout.Types.Count() == 0)
		{
			LOG_ERROR("Shader", "Shader reflection layout is invalid.");
			return false;
		}
		m_Layout = &layout;
		return true;
	}

	const ShaderReflectionIR& ShaderProgramReflection::GetLayout() const
	{
		ENGINE_ASSERT(m_Layout != nullptr);
		return *m_Layout;
	}

	const ShaderIRTypeRecord* ShaderProgramReflection::GetType(const uint32 typeId) const
	{
		return m_Layout != nullptr && typeId < static_cast<uint32>(m_Layout->Types.Count()) ? &m_Layout->Types[typeId] : nullptr;
	}

	const ShaderIRParameterBlock* ShaderProgramReflection::GetBlock(const uint32 blockId) const
	{
		return m_Layout != nullptr && blockId < static_cast<uint32>(m_Layout->ParameterBlocks.Count()) ? &m_Layout->ParameterBlocks[blockId] : nullptr;
	}

	const ShaderIRMember* ShaderProgramReflection::FindMember(const uint32 typeId, const String& name) const
	{
		const ShaderIRTypeRecord* type = GetType(typeId);
		if (type == nullptr || type->Kind != SE_TEXT("Struct"))
		{
			return nullptr;
		}
		for (int32 index = 0; index < type->Members.Count(); index++)
		{
			if (type->Members[index].Name == name)
			{
				return &type->Members[index];
			}
		}
		return nullptr;
	}

	const ShaderIRRangeBinding* ShaderProgramReflection::FindRangeBinding(const uint32 blockId, const uint32 rangeIndex) const
	{
		const ShaderIRParameterBlock* block = GetBlock(blockId);
		if (block == nullptr)
		{
			return nullptr;
		}
		for (int32 index = 0; index < block->RangeBindings.Count(); index++)
		{
			if (block->RangeBindings[index].RangeIndex == rangeIndex)
			{
				return &block->RangeBindings[index];
			}
		}
		return nullptr;
	}
}
