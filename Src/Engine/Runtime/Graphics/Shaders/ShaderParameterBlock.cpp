#include "ShaderParameterBlock.h"

#include "Reflection/ShaderProgramReflection.h"
#include "Runtime/Core/Memory/Memory.h"

namespace SE
{
	namespace
	{
		bool TryGetRuntimeResourceType(const String& descriptorType, ShaderRuntimeResourceType& output)
		{
			if (descriptorType == SE_TEXT("Sampler"))
			{
				output = ShaderRuntimeResourceType::Sampler;
				return true;
			}
			if (descriptorType == SE_TEXT("Texture"))
			{
				output = ShaderRuntimeResourceType::Texture;
				return true;
			}
			if (descriptorType == SE_TEXT("TypedBuffer") || descriptorType == SE_TEXT("RawBuffer") || descriptorType == SE_TEXT("StorageBuffer"))
			{
				output = ShaderRuntimeResourceType::Buffer;
				return true;
			}
			return false;
		}

		bool ResolveRuntimeResourceType(const ShaderIRResourceRange& range, const ShaderIRRangeBinding& binding, ShaderRuntimeResourceType& output)
		{
			if (binding.DescriptorRanges.Count() == 0)
			{
				LOG_ERROR("Graphics", "Shader resource range has no descriptor ranges.");
				return false;
			}

			bool hasType = false;
			for (int32 descriptorIndex = 0; descriptorIndex < binding.DescriptorRanges.Count(); descriptorIndex++)
			{
				ShaderRuntimeResourceType descriptorType;
				if (!TryGetRuntimeResourceType(binding.DescriptorRanges[descriptorIndex].DescriptorType, descriptorType))
				{
					LOG_ERROR("Graphics", "Shader resource descriptor type is not supported by runtime binding: {0}", binding.DescriptorRanges[descriptorIndex].DescriptorType);
					return false;
				}
				if (!hasType)
				{
					output = descriptorType;
					hasType = true;
					continue;
				}
				if (output != descriptorType)
				{
					LOG_ERROR("Graphics", "Shader resource range maps to incompatible runtime resource types.");
					return false;
				}
			}

			if (range.ResourceKind == SE_TEXT("Sampler") && output != ShaderRuntimeResourceType::Sampler)
			{
				LOG_ERROR("Graphics", "Shader sampler resource does not map to a sampler descriptor.");
				return false;
			}
			if (range.ResourceKind == SE_TEXT("StorageBuffer") && output != ShaderRuntimeResourceType::Buffer)
			{
				LOG_ERROR("Graphics", "Shader storage buffer resource does not map to a buffer descriptor.");
				return false;
			}
			return hasType;
		}
	}

	ShaderParameterBlock::~ShaderParameterBlock()
	{
		Clear();
	}

	bool ShaderParameterBlock::Initialize(const ShaderProgramReflection& reflection, const uint32 blockId)
	{
		Clear();
		const ShaderIRParameterBlock* block = reflection.GetBlock(blockId);
		if (block == nullptr)
		{
			LOG_ERROR("Graphics", "Shader parameter block was not found.");
			return false;
		}
		const ShaderIRTypeRecord* type = reflection.GetType(block->ElementTypeId);
		if (type == nullptr)
		{
			LOG_ERROR("Graphics", "Shader parameter block element type was not found.");
			return false;
		}

		m_Reflection = &reflection;
		m_BlockId = blockId;
		m_UniformData.Resize(static_cast<int32>(block->UniformByteSize));

		// 按 SLC2 反射的资源 range 建立运行时槽位；sub-block 递归建立自己的 ParameterBlock 状态。
		for (int32 rangeIndex = 0; rangeIndex < type->ResourceRanges.Count(); rangeIndex++)
		{
			const ShaderIRRangeBinding* binding = reflection.FindRangeBinding(blockId, static_cast<uint32>(rangeIndex));
			if (binding == nullptr)
			{
				LOG_ERROR("Graphics", "Shader parameter block resource range has no binding mapping.");
				Clear();
				return false;
			}
			if (binding->SubBlockId >= 0)
			{
				ShaderParameterBlock* subBlock = New<ShaderParameterBlock>();
				if (!subBlock->Initialize(reflection, static_cast<uint32>(binding->SubBlockId)))
				{
					Delete(subBlock);
					Clear();
					return false;
				}
				m_SubBlocks.Add(subBlock);
				continue;
			}
			if (binding->Flavor != SE_TEXT("simple"))
			{
				LOG_ERROR("Graphics", "Shader parameter block range mapping is neither simple nor a sub-block.");
				Clear();
				return false;
			}

			const ShaderIRResourceRange& range = type->ResourceRanges[rangeIndex];
			if (!range.InternalRole.IsEmpty())
			{
				// defaultUniformBuffer 等内部 range 由普通 uniform 数据路径处理，不暴露为用户可绑定资源。
				continue;
			}
			ShaderRuntimeResourceType runtimeType;
			if (!ResolveRuntimeResourceType(range, *binding, runtimeType))
			{
				Clear();
				return false;
			}
			for (uint32 resourceIndex = 0; resourceIndex < range.Count; resourceIndex++)
			{
				ShaderRuntimeResourceSlot slot;
				slot.RangeIndex = static_cast<uint32>(rangeIndex);
				slot.ResourceIndex = resourceIndex;
				slot.Type = runtimeType;
				m_Resources.Add(slot);
			}
		}
		return true;
	}

	bool ShaderParameterBlock::SetResource(const ShaderVarLocation& location, const ShaderRuntimeResourceType type, void* value)
	{
		if (location.BlockId != m_BlockId || location.ResourceRangeIndex < 0 || location.ResourceIndex < 0)
		{
			LOG_ERROR("Graphics", "Shader variable does not identify a resource slot in this parameter block.");
			return false;
		}
		ShaderRuntimeResourceSlot* slot = FindResourceSlot(static_cast<uint32>(location.ResourceRangeIndex), static_cast<uint32>(location.ResourceIndex));
		if (slot == nullptr || slot->Type != type)
		{
			LOG_ERROR("Graphics", "Shader resource type does not match the reflected slot.");
			return false;
		}
		slot->Value = value;
		return true;
	}

	bool ShaderParameterBlock::SetUniform(const ShaderVarLocation& location, const void* data, const uint32 size)
	{
		if (location.BlockId != m_BlockId || location.UniformByteOffset < 0 || data == nullptr || size == 0 || static_cast<uint64>(location.UniformByteOffset) + size > static_cast<uint64>(m_UniformData.Count()))
		{
			LOG_ERROR("Graphics", "Shader uniform write is outside the parameter block.");
			return false;
		}
		Platform::MemoryCopy(m_UniformData.Get() + location.UniformByteOffset, data, size);
		return true;
	}

	bool ShaderParameterBlock::AppendResources(ShaderBindingSnapshot& snapshot) const
	{
		for (int32 index = 0; index < m_Resources.Count(); index++)
		{
			const ShaderRuntimeResourceSlot& slot = m_Resources[index];
			if (slot.Value == nullptr)
			{
				LOG_ERROR("Graphics", "Shader resource is not bound (block={0}, range={1}, index={2}).", m_BlockId, slot.RangeIndex, slot.ResourceIndex);
				return false;
			}
			ShaderBindingResource resource;
			resource.BlockId = m_BlockId;
			resource.RangeIndex = slot.RangeIndex;
			resource.ResourceIndex = slot.ResourceIndex;
			resource.Type = slot.Type;
			resource.Value = slot.Value;
			snapshot.AddResource(resource);
		}
		for (int32 index = 0; index < m_SubBlocks.Count(); index++)
		{
			if (!m_SubBlocks[index]->AppendResources(snapshot))
			{
				return false;
			}
		}
		return true;
	}

	void ShaderParameterBlock::AppendUniformData(ShaderBindingSnapshot& snapshot) const
	{
		const ShaderIRParameterBlock* block = m_Reflection->GetBlock(m_BlockId);
		if (block->HasDefaultUniformBuffer)
		{
			// 只有反射中实际存在 defaultUniformBuffer 的 block 才提交普通 uniform 字节数据。
			snapshot.AddUniformData(m_BlockId, m_UniformData);
		}
		for (int32 index = 0; index < m_SubBlocks.Count(); index++)
		{
			m_SubBlocks[index]->AppendUniformData(snapshot);
		}
	}

	uint32 ShaderParameterBlock::GetBlockId() const
	{
		return m_BlockId;
	}

	ShaderParameterBlock* ShaderParameterBlock::FindSubBlock(const uint32 blockId)
	{
		for (int32 index = 0; index < m_SubBlocks.Count(); index++)
		{
			if (m_SubBlocks[index]->m_BlockId == blockId)
			{
				return m_SubBlocks[index];
			}
		}
		return nullptr;
	}

	const ShaderParameterBlock* ShaderParameterBlock::FindSubBlock(const uint32 blockId) const
	{
		for (int32 index = 0; index < m_SubBlocks.Count(); index++)
		{
			if (m_SubBlocks[index]->m_BlockId == blockId)
			{
				return m_SubBlocks[index];
			}
		}
		return nullptr;
	}

	ShaderParameterBlock* ShaderParameterBlock::FindBlock(const uint32 blockId)
	{
		if (m_BlockId == blockId)
		{
			return this;
		}
		for (int32 index = 0; index < m_SubBlocks.Count(); index++)
		{
			ShaderParameterBlock* block = m_SubBlocks[index]->FindBlock(blockId);
			if (block != nullptr)
			{
				return block;
			}
		}
		return nullptr;
	}

	const ShaderParameterBlock* ShaderParameterBlock::FindBlock(const uint32 blockId) const
	{
		if (m_BlockId == blockId)
		{
			return this;
		}
		for (int32 index = 0; index < m_SubBlocks.Count(); index++)
		{
			const ShaderParameterBlock* block = m_SubBlocks[index]->FindBlock(blockId);
			if (block != nullptr)
			{
				return block;
			}
		}
		return nullptr;
	}

	void ShaderParameterBlock::Clear()
	{
		m_SubBlocks.ClearDelete();
		m_Resources.Clear();
		m_UniformData.Clear();
		m_Reflection = nullptr;
		m_BlockId = 0;
	}

	ShaderRuntimeResourceSlot* ShaderParameterBlock::FindResourceSlot(const uint32 rangeIndex, const uint32 resourceIndex)
	{
		for (int32 index = 0; index < m_Resources.Count(); index++)
		{
			if (m_Resources[index].RangeIndex == rangeIndex && m_Resources[index].ResourceIndex == resourceIndex)
			{
				return &m_Resources[index];
			}
		}
		return nullptr;
	}
}
