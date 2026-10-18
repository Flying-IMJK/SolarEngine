#include "ShaderProgramReflection.h"
#include "ShaderNameResolver.h"

namespace SE
{
	namespace
	{
		bool ResolveBindingKind(const ShaderIRResourceRange& range, const ShaderIRRangeBinding& binding, ShaderProgramBindingKind& output)
		{
			if (binding.DescriptorRanges.IsEmpty())
			{
				LOG_ERROR("Shader", "Reflected resource binding has no descriptor range.");
				return false;
			}

			bool hasKind = false;
			for (int32 index = 0; index < binding.DescriptorRanges.Count(); index++)
			{
				const String& descriptorType = binding.DescriptorRanges[index].DescriptorType;
				ShaderProgramBindingKind kind;
				if (descriptorType == SE_TEXT("Sampler"))
				{
					kind = ShaderProgramBindingKind::Sampler;
				}
				else if (descriptorType == SE_TEXT("Texture"))
				{
					kind = ShaderProgramBindingKind::Texture;
				}
				else if (descriptorType == SE_TEXT("TypedBuffer") || descriptorType == SE_TEXT("RawBuffer") || descriptorType == SE_TEXT("StorageBuffer"))
				{
					kind = ShaderProgramBindingKind::Buffer;
				}
				else
				{
					LOG_ERROR("Shader", "Unsupported reflected descriptor type: {0}", descriptorType);
					return false;
				}

				if (hasKind && output != kind)
				{
					LOG_ERROR("Shader", "One reflected resource maps to incompatible descriptor kinds.");
					return false;
				}
				output = kind;
				hasKind = true;
			}

			if ((range.ResourceKind == SE_TEXT("Sampler") && output != ShaderProgramBindingKind::Sampler) ||
				(range.ResourceKind == SE_TEXT("StorageBuffer") && output != ShaderProgramBindingKind::Buffer))
			{
				LOG_ERROR("Shader", "Logical resource kind does not match its descriptor kind.");
				return false;
			}
			return hasKind;
		}

		bool EnumerateTypeBindings(const ShaderProgramReflection& reflection, const uint32 typeId, const String& path, const bool isArrayElement, List<ShaderProgramBinding>& output)
		{
			const ShaderIRTypeRecord* type = reflection.GetType(typeId);
			if (type == nullptr)
			{
				LOG_ERROR("Shader", "Cannot enumerate an unknown reflection type.");
				return false;
			}

			if (type->Kind == SE_TEXT("Struct"))
			{
				// 结构体本身不是绑定叶子，规范路径由成员名逐层拼接。
				for (int32 index = 0; index < type->Members.Count(); index++)
				{
					const ShaderIRMember& member = type->Members[index];
					const String memberPath = path.IsEmpty() ? member.Name : String::Format(SE_TEXT("{0}.{1}"), path, member.Name);
					if (!EnumerateTypeBindings(reflection, member.TypeId, memberPath, isArrayElement, output))
					{
						return false;
					}
				}
				return true;
			}

			if (type->Kind == SE_TEXT("ParameterBlock") || type->Kind == SE_TEXT("ConstantBuffer"))
			{
				// Pruned IR 中子块的 ElementType 以 ParameterBlock 记录为准，不能依赖容器类型的旧 TypeId。
				ShaderVarLocation blockLocation;
				if (!ShaderNameResolver::Resolve(reflection, path, blockLocation))
				{
					return false;
				}
				const ShaderIRParameterBlock* block = reflection.GetBlock(blockLocation.BlockId);
				if (block == nullptr || blockLocation.TypeId != block->ElementTypeId)
				{
					LOG_ERROR("Shader", "Reflected parameter block path does not resolve to its element type.");
					return false;
				}
				return EnumerateTypeBindings(reflection, block->ElementTypeId, path, isArrayElement, output);
			}

			if (type->Kind == SE_TEXT("Array"))
			{
				for (uint32 index = 0; index < type->ElementCount; index++)
				{
					const String elementPath = String::Format(SE_TEXT("{0}[{1}]"), path, index);
					if (!EnumerateTypeBindings(reflection, type->ElementTypeId, elementPath, true, output))
					{
						return false;
					}
				}
				return true;
			}

			if (path.IsEmpty())
			{
				LOG_ERROR("Shader", "Reflected leaf binding has an empty path.");
				return false;
			}
			ShaderProgramBinding binding;
			binding.Path = path;
			binding.TypeName = type->Name;
			binding.UniformByteSize = type->UniformByteSize;
			binding.IsArrayElement = isArrayElement;
			if (!ShaderNameResolver::Resolve(reflection, path, binding.Location))
			{
				return false;
			}
			if (binding.Location.Kind == ShaderParameterAddressKind::UniformValue)
			{
				binding.Kind = ShaderProgramBindingKind::Uniform;
			}
			else if (binding.Location.Kind == ShaderParameterAddressKind::Resource && binding.Location.LogicalResourceSlotIndex >= 0)
			{
				if (!reflection.TryGetLogicalResourceSlotKind(static_cast<uint32>(binding.Location.LogicalResourceSlotIndex), binding.Kind))
				{
					LOG_ERROR("Shader", "Reflected resource slot index is outside the precomputed slot table.");
					return false;
				}
			}
			else
			{
				LOG_ERROR("Shader", "Reflected leaf path does not resolve to a bindable parameter: {0}", path);
				return false;
			}
			for (int32 index = 0; index < output.Count(); index++)
			{
				if (output[index].Path == binding.Path)
				{
					LOG_ERROR("Shader", "Duplicate reflected binding path: {0}", binding.Path);
					return false;
				}
			}
			output.Add(MoveTemp(binding));
			return true;
		}
	}

	bool ShaderProgramReflection::Initialize(const ShaderReflectionIR& layout)
	{
		Clear();
		if (layout.Schema != 3 || layout.RootBlockId >= static_cast<uint32>(layout.ParameterBlocks.Count()) || layout.Types.Count() == 0)
		{
			LOG_ERROR("Shader", "Shader reflection layout is invalid.");
			return false;
		}
		m_Layout = &layout;
		if (!BuildPrecomputedLayout() || !ResolveResourceSlotTypes())
		{
			Clear();
			return false;
		}
		return true;
	}

	bool ShaderProgramReflection::EnumerateBindings(List<ShaderProgramBinding>& output) const
	{
		output.Clear();
		if (m_Layout == nullptr)
		{
			LOG_ERROR("Shader", "Shader reflection is not initialized.");
			return false;
		}
		const ShaderIRParameterBlock* root = GetBlock(m_Layout->RootBlockId);
		if (root == nullptr)
		{
			LOG_ERROR("Shader", "Shader reflection root block is missing.");
			return false;
        }
        return EnumerateTypeBindings(*this, root->ElementTypeId, String::Empty, false, output);
    }

    bool ShaderProgramReflection::FindUniformBlockIndex(const uint32 blockId, uint32& output) const
    {
        if (blockId >= static_cast<uint32>(m_BlockToUniformBlockIndex.Count()) ||
            m_BlockToUniformBlockIndex[blockId] < 0)
        {
            return false;
        }
        output = static_cast<uint32>(m_BlockToUniformBlockIndex[blockId]);
        return true;
    }

    uint32 ShaderProgramReflection::GetUniformBlockCount() const
    {
        return static_cast<uint32>(m_UniformBlocks.Count());
    }

    const ShaderUniformBlockLayout* ShaderProgramReflection::GetUniformBlockLayout(const uint32 index) const
    {
        return index < static_cast<uint32>(m_UniformBlocks.Count()) ? &m_UniformBlocks[index] : nullptr;
    }

    bool ShaderProgramReflection::FindLogicalResourceSlotIndex(const ShaderResourceKey& key, uint32& output) const
    {
        return m_LogicalResourceSlotLookup.TryGet(key, output);
    }

    uint32 ShaderProgramReflection::GetLogicalResourceSlotCount() const
    {
        return static_cast<uint32>(m_LogicalResourceSlots.Count());
    }

    const ShaderLogicalResourceSlot* ShaderProgramReflection::GetLogicalResourceSlot(const uint32 index) const
    {
        return index < static_cast<uint32>(m_LogicalResourceSlots.Count()) ? &m_LogicalResourceSlots[index] : nullptr;
    }

    bool ShaderProgramReflection::TryGetLogicalResourceSlotKind(const uint32              slotIndex,
                                                                ShaderProgramBindingKind& output) const
    {
        if (slotIndex >= static_cast<uint32>(m_LogicalResourceSlots.Count()))
        {
            return false;
        }
        output = m_LogicalResourceSlots[slotIndex].Kind;
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

	void ShaderProgramReflection::Clear()
	{
		m_LogicalResourceSlotLookup.Clear();
		m_LogicalResourceSlots.Clear();
		m_BlockToUniformBlockIndex.Clear();
		m_UniformBlocks.Clear();
		m_Layout = nullptr;
	}

	bool ShaderProgramReflection::BuildPrecomputedLayout()
	{
		m_BlockToUniformBlockIndex.Resize(m_Layout->ParameterBlocks.Count());
		for (int32 index = 0; index < m_BlockToUniformBlockIndex.Count(); index++)
		{
			m_BlockToUniformBlockIndex[index] = -1;
		}
		List<byte> visitState;
		visitState.Resize(m_Layout->ParameterBlocks.Count());
		for (int32 index = 0; index < visitState.Count(); index++)
		{
			visitState[index] = 0;
		}
		if (!BuildBlockLayout(m_Layout->RootBlockId, visitState))
		{
			return false;
		}

		for (int32 index = 0; index < visitState.Count(); index++)
		{
			if (visitState[index] != 2)
			{
				LOG_ERROR("Shader", "Shader reflection contains a parameter block {0} that is not reachable from the root.", m_Layout->ParameterBlocks[index].Name);
				return false;
			}
		}
		return true;
	}

	bool ShaderProgramReflection::BuildBlockLayout(const uint32 blockId, List<byte>& visitState)
	{
		if (blockId >= static_cast<uint32>(visitState.Count()) || visitState[blockId] != 0)
		{
			LOG_ERROR("Shader", "Shader reflection parameter block graph is invalid or reaches a block more than once.");
			return false;
		}
		const ShaderIRParameterBlock* block = GetBlock(blockId);
		if (block == nullptr || block->Id != blockId)
		{
			LOG_ERROR("Shader", "Shader reflection parameter block identity is invalid.");
			return false;
		}
		const ShaderIRTypeRecord* type = GetType(block->ElementTypeId);
		if (type == nullptr || block->RangeBindings.Count() != type->ResourceRanges.Count())
		{
			LOG_ERROR("Shader", "Shader reflection parameter block does not map every logical resource range.");
			return false;
		}

		visitState[blockId] = 1;
		if (block->HasDefaultUniformBuffer)
		{
			if (block->UniformByteSize == 0)
			{
				LOG_ERROR("Shader", "Shader reflection uniform block has zero byte size.");
				return false;
			}
			ShaderUniformBlockLayout uniformBlock;
			uniformBlock.BlockId = blockId;
			uniformBlock.TypeId = block->ElementTypeId;
			uniformBlock.ByteSize = block->UniformByteSize;
			m_BlockToUniformBlockIndex[blockId] = m_UniformBlocks.Count();
			m_UniformBlocks.Add(uniformBlock);
		}
		else if (block->UniformByteSize != 0)
		{
			LOG_ERROR("Shader", "Shader reflection block has ordinary data without a default uniform buffer.");
			return false;
		}

		for (int32 rangeIndex = 0; rangeIndex < type->ResourceRanges.Count(); rangeIndex++)
		{
			const ShaderIRResourceRange& range = type->ResourceRanges[rangeIndex];
			const ShaderIRRangeBinding* binding = FindRangeBinding(blockId, static_cast<uint32>(rangeIndex));
			if (binding == nullptr)
			{
				LOG_ERROR("Shader", "Shader reflection range binding index is missing or unstable.");
				return false;
			}

			if (binding->SubBlockId >= 0)
			{
				if (!BuildBlockLayout(static_cast<uint32>(binding->SubBlockId), visitState))
				{
					return false;
				}
				continue;
			}

			if (binding->Flavor != SE_TEXT("simple"))
			{
				LOG_ERROR("Shader", "Shader reflection range is neither a simple resource nor a sub-block.");
				return false;
			}

			ShaderProgramBindingKind kind;
			if (!ResolveBindingKind(range, *binding, kind))
			{
				return false;
			}

			for (uint32 resourceIndex = 0; resourceIndex < range.Count; resourceIndex++)
			{
				ShaderResourceKey key;
				key.BlockId = blockId;
				key.RangeIndex = static_cast<uint32>(rangeIndex);
				key.ResourceIndex = resourceIndex;
				if (m_LogicalResourceSlotLookup.ContainsKey(key))
				{
					LOG_ERROR("Shader", "Shader reflection contains a duplicate logical resource key.");
					return false;
				}
				ShaderLogicalResourceSlot slot;
				slot.Key = key;
				slot.Kind = kind;
				const uint32 slotIndex = static_cast<uint32>(m_LogicalResourceSlots.Count());
				m_LogicalResourceSlots.Add(slot);
				m_LogicalResourceSlotLookup.Add(key, slotIndex);
			}
		}
		visitState[blockId] = 2;
		return true;
	}

	bool ShaderProgramReflection::ResolveResourceSlotTypes()
	{
		List<ShaderProgramBinding> bindings;
		if (!EnumerateBindings(bindings))
		{
			return false;
		}
		List<byte> resolvedSlots;
		resolvedSlots.Resize(m_LogicalResourceSlots.Count());
		for (int32 index = 0; index < resolvedSlots.Count(); index++)
		{
			resolvedSlots[index] = 0;
		}
		for (int32 index = 0; index < bindings.Count(); index++)
		{
			const ShaderVarLocation& location = bindings[index].Location;
			if (location.Kind != ShaderParameterAddressKind::Resource)
			{
				continue;
			}
			if (location.LogicalResourceSlotIndex < 0 || location.LogicalResourceSlotIndex >= m_LogicalResourceSlots.Count() || resolvedSlots[location.LogicalResourceSlotIndex] != 0)
			{
				LOG_ERROR("Shader", "Shader resource path does not map one-to-one to a logical resource slot.");
				return false;
			}
			ShaderLogicalResourceSlot& slot = m_LogicalResourceSlots[location.LogicalResourceSlotIndex];
			slot.TypeId = location.TypeId;
			resolvedSlots[location.LogicalResourceSlotIndex] = 1;
		}
		for (int32 index = 0; index < resolvedSlots.Count(); index++)
		{
			if (resolvedSlots[index] == 0)
			{
				LOG_ERROR("Shader", "Shader logical resource slot has no reachable NamePath.");
				return false;
			}
		}
		return true;
	}
}
