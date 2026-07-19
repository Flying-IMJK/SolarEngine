
#include "DescriptorSetVulkan.h"
#include "Runtime/Core/Types/Hash.h"
#include "Runtime/Engine.h"

namespace SE
{
	void DescriptorSetLayoutInfoVulkan::AddBindingsForStage(VkShaderStageFlagBits stageFlags, DescriptorSet::Stage descSet, const SpirvShaderDescriptorInfo* descriptorInfo)
	{
		const int32 descriptorSetIndex = descSet;
		if (descriptorSetIndex >= setLayouts.Count())
		{
			setLayouts.Resize(descriptorSetIndex + 1);
		}

		SetLayout& descSetLayout = setLayouts[descriptorSetIndex];

		VkDescriptorSetLayoutBinding binding;
		binding.stageFlags = stageFlags;
		binding.pImmutableSamplers = nullptr;
		for (uint32 descriptorIndex = 0; descriptorIndex < descriptorInfo->descriptorTypesCount; descriptorIndex++)
		{
			auto& descriptor = descriptorInfo->descriptorTypes[descriptorIndex];
			binding.binding = descriptorIndex;
			binding.descriptorType = descriptor.descriptorType;
			binding.descriptorCount = descriptor.count;

			layoutTypes[binding.descriptorType]++;
			descSetLayout.LayoutBindings.Add(binding);
			hash = HashCombine(hash, &binding, sizeof(binding));
		}
	}

	bool DescriptorSetLayoutInfoVulkan::operator==(const DescriptorSetLayoutInfoVulkan& other) const
	{
		if (other.setLayouts.Count() != setLayouts.Count())
		{
			return false;
		}
#if VULKAN_HASH_POOLS_WITH_LAYOUT_TYPES
		if (other.setLayoutsHash != setLayoutsHash)
		{
			return false;
		}
#endif
		for (int32 index = 0; index < other.setLayouts.Count(); index++)
		{
			const int32 bindingsCount = setLayouts[index].LayoutBindings.Count();
			if (other.setLayouts[index].LayoutBindings.Count() != bindingsCount)
			{
				return false;
			}
			if (bindingsCount != 0 &&
				Platform::MemoryCompare(other.setLayouts[index].LayoutBindings.Get(),
					setLayouts[index].LayoutBindings.Get(),
					bindingsCount * sizeof(VkDescriptorSetLayoutBinding)))
			{
				return false;
			}
		}
		return true;
	}

	DescriptorSetLayoutVulkan::DescriptorSetLayoutVulkan(GPUDeviceVulkan* device)
		: device(device)
	{
	}

	DescriptorSetLayoutVulkan::~DescriptorSetLayoutVulkan()
	{
		for (VkDescriptorSetLayout& handle : handles)
		{
			device->deferredDeletionQueue.EnqueueResource(DeferredDeletionQueueVulkan::Type::DescriptorSetLayout, handle);
		}
	}

	void DescriptorSetLayoutVulkan::Compile()
	{
//#if !BUILD_RELEASE
		ENGINE_ASSERT(handles.IsEmpty());
		const VkPhysicalDeviceLimits& limits = device->physicalDeviceLimits;
		ENGINE_ASSERT(layoutTypes[VK_DESCRIPTOR_TYPE_SAMPLER] + layoutTypes[VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER] < limits.maxDescriptorSetSamplers);
		ENGINE_ASSERT(layoutTypes[VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER]+ layoutTypes[VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC] < limits.maxDescriptorSetUniformBuffers);
		ENGINE_ASSERT(layoutTypes[VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC] < limits.maxDescriptorSetUniformBuffersDynamic);
		ENGINE_ASSERT(layoutTypes[VK_DESCRIPTOR_TYPE_STORAGE_BUFFER] + layoutTypes[VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC] < limits.maxDescriptorSetStorageBuffers);
		ENGINE_ASSERT(layoutTypes[VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC] < limits.maxDescriptorSetStorageBuffersDynamic);
		ENGINE_ASSERT(layoutTypes[VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER] + layoutTypes[VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE] + layoutTypes[VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER] < limits.maxDescriptorSetSampledImages);
		ENGINE_ASSERT(layoutTypes[VK_DESCRIPTOR_TYPE_STORAGE_IMAGE] + layoutTypes[VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER]< limits.maxDescriptorSetStorageImages);
//#endif

		handles.Resize(setLayouts.Count());
		for (int32 i = 0; i < setLayouts.Count(); i++)
		{
			auto& layout = setLayouts.Get()[i];
			VkDescriptorSetLayoutCreateInfo layoutInfo;
			VulkanTool::ZeroStruct(layoutInfo, VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO);
			layoutInfo.bindingCount = layout.LayoutBindings.Count();
			layoutInfo.pBindings = layout.LayoutBindings.Get();
			VALIDATE_VULKAN_RESULT(vkCreateDescriptorSetLayout(device->device, &layoutInfo, nullptr, &handles[i]));
		}

#if VULKAN_HASH_POOLS_WITH_LAYOUT_TYPES
		if (setLayoutsHash == 0)
		{
			setLayoutsHash = GetHash(layoutTypes, sizeof(layoutTypes));
		}
#endif

		VulkanTool::ZeroStruct(allocateInfo, VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO);
		allocateInfo.descriptorSetCount = handles.Count();
		allocateInfo.pSetLayouts = handles.Get();
	}

	DescriptorPoolVulkan::DescriptorPoolVulkan(GPUDeviceVulkan* device, const DescriptorSetLayoutVulkan& layout)
		: m_Device(device)
		, m_Handle(VK_NULL_HANDLE)
		, m_DescriptorSetsMax(0)
		, m_AllocatedDescriptorSetsCount(0)
		, m_AllocatedDescriptorSetsCountMax(0)
		, m_Layout(layout)
	{
		List<VkDescriptorPoolSize, FixedAllocation<VULKAN_DESCRIPTOR_TYPE_END + 1>> types;

		// The maximum amount of descriptor sets layout allocations to hold
		const uint32 MaxSetsAllocations = 256;
		m_DescriptorSetsMax = MaxSetsAllocations * (VULKAN_HASH_POOLS_WITH_LAYOUT_TYPES ? 1 : m_Layout.setLayouts.Count());
		for (uint32 typeIndex = VULKAN_DESCRIPTOR_TYPE_BEGIN; typeIndex <= VULKAN_DESCRIPTOR_TYPE_END; typeIndex++)
		{
			const VkDescriptorType descriptorType = (VkDescriptorType)typeIndex;
			const uint32 typesUsed = m_Layout.layoutTypes[descriptorType];
			if (typesUsed > 0)
			{
				VkDescriptorPoolSize& type = types.AddOne();
				Platform::MemoryClear(&type, sizeof(type));
				type.type = descriptorType;
				type.descriptorCount = typesUsed * MaxSetsAllocations;
			}
		}

		VkDescriptorPoolCreateInfo createInfo;
		VulkanTool::ZeroStruct(createInfo, VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO);
		createInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		createInfo.poolSizeCount = types.Count();
		createInfo.pPoolSizes = types.Get();
		createInfo.maxSets = m_DescriptorSetsMax;
		VALIDATE_VULKAN_RESULT(vkCreateDescriptorPool(m_Device->device, &createInfo, nullptr, &m_Handle));
	}

	DescriptorPoolVulkan::~DescriptorPoolVulkan()
	{
		if (m_Handle != VK_NULL_HANDLE)
		{
			vkDestroyDescriptorPool(m_Device->device, m_Handle, nullptr);
		}
	}

	void DescriptorPoolVulkan::Track(const DescriptorSetLayoutVulkan& layout)
	{
//#if !BUILD_RELEASE
		for (uint32 typeIndex = VULKAN_DESCRIPTOR_TYPE_BEGIN; typeIndex <= VULKAN_DESCRIPTOR_TYPE_END; typeIndex++)
		{
			ENGINE_ASSERT(m_Layout.layoutTypes[typeIndex] == layout.layoutTypes[typeIndex]);
		}
//#endif
		m_AllocatedDescriptorSetsCount += layout.setLayouts.Count();
		m_AllocatedDescriptorSetsCountMax = Math::Max(m_AllocatedDescriptorSetsCount, m_AllocatedDescriptorSetsCountMax);
	}

	void DescriptorPoolVulkan::TrackRemoveUsage(const DescriptorSetLayoutVulkan& layout)
	{
//#if !BUILD_RELEASE
		for (uint32 typeIndex = VULKAN_DESCRIPTOR_TYPE_BEGIN; typeIndex <= VULKAN_DESCRIPTOR_TYPE_END; typeIndex++)
		{
			ENGINE_ASSERT(m_Layout.layoutTypes[typeIndex] == layout.layoutTypes[typeIndex]);
		}
//#endif
		m_AllocatedDescriptorSetsCount -= layout.setLayouts.Count();
	}

	void DescriptorPoolVulkan::Reset()
	{
		if (m_Handle != VK_NULL_HANDLE)
		{
			VALIDATE_VULKAN_RESULT(vkResetDescriptorPool(m_Device->device, m_Handle, 0));
		}
		m_AllocatedDescriptorSetsCount = 0;
	}

	bool DescriptorPoolVulkan::AllocateDescriptorSets(const VkDescriptorSetAllocateInfo& descriptorSetAllocateInfo, VkDescriptorSet* result)
	{
		VkDescriptorSetAllocateInfo allocateInfo = descriptorSetAllocateInfo;
		allocateInfo.descriptorPool = m_Handle;
		return vkAllocateDescriptorSets(m_Device->device, &allocateInfo, result) == VK_SUCCESS;
	}

	TypedDescriptorPoolSetVulkan::~TypedDescriptorPoolSetVulkan()
	{
		for (auto pool = m_PoolListHead; pool;)
		{
			const auto next = pool->next;
			Delete(pool->element);
			Delete(pool);
			pool = next;
		}
	}

	bool TypedDescriptorPoolSetVulkan::AllocateDescriptorSets(const DescriptorSetLayoutVulkan& layout, VkDescriptorSet* outSets)
	{
		if (layout.handles.HasItems())
		{
			auto* pool = m_PoolListCurrent->element;
			while (!pool->AllocateDescriptorSets(layout.allocateInfo, outSets))
			{
				pool = GetFreePool(true);
			}
			return true;
		}
		return true;
	}

	DescriptorPoolVulkan* TypedDescriptorPoolSetVulkan::GetFreePool(bool forceNewPool)
	{
		if (!forceNewPool)
		{
			return m_PoolListCurrent->element;
		}
		if (m_PoolListCurrent->next)
		{
			m_PoolListCurrent = m_PoolListCurrent->next;
			return m_PoolListCurrent->element;
		}
		return PushNewPool();
	}

	DescriptorPoolVulkan* TypedDescriptorPoolSetVulkan::PushNewPool()
	{
		auto* newPool = New<DescriptorPoolVulkan>(m_Device, m_Layout);
		if (m_PoolListCurrent)
		{
			m_PoolListCurrent->next = New<PoolList>(newPool);
			m_PoolListCurrent = m_PoolListCurrent->next;
		}
		else
		{
			m_PoolListCurrent = m_PoolListHead = New<PoolList>(newPool);
		}
		return newPool;
	}

	void TypedDescriptorPoolSetVulkan::Reset()
	{
		for (PoolList* pool = m_PoolListHead; pool; pool = pool->next)
		{
			pool->element->Reset();
		}
		m_PoolListCurrent = m_PoolListHead;
	}

	DescriptorPoolSetContainerVulkan::DescriptorPoolSetContainerVulkan(GPUDeviceVulkan* device)
		: m_Device(device)
		, lastFrameUsed(Engine::FrameCount)
	{
	}

	DescriptorPoolSetContainerVulkan::~DescriptorPoolSetContainerVulkan()
	{
		m_TypedDescriptorPools.ClearDelete();
	}

	TypedDescriptorPoolSetVulkan* DescriptorPoolSetContainerVulkan::AcquireTypedPoolSet(const DescriptorSetLayoutVulkan& layout)
	{
		const uint32 hash = VULKAN_HASH_POOLS_WITH_LAYOUT_TYPES ? layout.setLayoutsHash : GetHash(layout);
		TypedDescriptorPoolSetVulkan* typedPool;
		if (!m_TypedDescriptorPools.TryGet(hash, typedPool))
		{
			typedPool = New<TypedDescriptorPoolSetVulkan>(m_Device, this, layout);
			m_TypedDescriptorPools.Add(hash, typedPool);
		}
		return typedPool;
	}

	void DescriptorPoolSetContainerVulkan::Reset()
	{
		for (auto typedPool : m_TypedDescriptorPools)
		{
			typedPool.Value->Reset();
		}
	}

	DescriptorPoolsManagerVulkan::DescriptorPoolsManagerVulkan(GPUDeviceVulkan* device)
		: m_Device(device)
	{
	}

	DescriptorPoolsManagerVulkan::~DescriptorPoolsManagerVulkan()
	{
		m_PoolSets.ClearDelete();
	}

	DescriptorPoolSetContainerVulkan* DescriptorPoolsManagerVulkan::AcquirePoolSetContainer()
	{
		Threading::ScopeLock lock(m_Locker);
		for (auto* poolSet : m_PoolSets)
		{
			if (poolSet->refs == 0)
			{
				poolSet->lastFrameUsed = Engine::FrameCount;
				poolSet->Reset();
				return poolSet;
			}
		}
		const auto poolSet = New<DescriptorPoolSetContainerVulkan>(m_Device);
		m_PoolSets.Add(poolSet);
		return poolSet;
	}

	void DescriptorPoolsManagerVulkan::GC()
	{
		Threading::ScopeLock lock(m_Locker);
		for (int32 i = m_PoolSets.Count() - 1; i >= 0; i--)
		{
			auto poolSet = m_PoolSets[i];
			if (poolSet->refs == 0 && Engine::FrameCount - poolSet->lastFrameUsed > VULKAN_RESOURCE_DELETE_SAFE_FRAMES_COUNT)
			{
				m_PoolSets.RemoveAt(i);
				Delete(poolSet);
				break;
			}
		}
	}

	PipelineLayoutVulkan::PipelineLayoutVulkan(GPUDeviceVulkan* device, const DescriptorSetLayoutInfoVulkan& layout)
		: device(device)
		, handle(VK_NULL_HANDLE)
		, descriptorSetLayout(device)
	{
		descriptorSetLayout.CopyFrom(layout);
		descriptorSetLayout.Compile();

		VkPipelineLayoutCreateInfo createInfo;
		VulkanTool::ZeroStruct(createInfo, VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO);
		createInfo.setLayoutCount = descriptorSetLayout.handles.Count();
		createInfo.pSetLayouts = descriptorSetLayout.handles.Get();
		VALIDATE_VULKAN_RESULT(vkCreatePipelineLayout(device->device, &createInfo, nullptr, &handle));
	}

	PipelineLayoutVulkan::~PipelineLayoutVulkan()
	{
		if (handle != VK_NULL_HANDLE)
			device->deferredDeletionQueue.EnqueueResource(DeferredDeletionQueueVulkan::Type::PipelineLayout, handle);
	}

	uint32 DescriptorSetWriterVulkan::SetupDescriptorWrites(const SpirvShaderDescriptorInfo& info, VkWriteDescriptorSet* writeDescriptors,
		VkDescriptorImageInfo* imageInfo, VkDescriptorBufferInfo* bufferInfo,
		VkBufferView* texelBufferView, uint8* bindingToDynamicOffset)
	{
		ENGINE_ASSERT(info.descriptorTypesCount <= SpirvShaderDescriptorInfo::MaxDescriptors);
		this->writeDescriptors = writeDescriptors;
		writesCount = info.descriptorTypesCount;
		this->bindingToDynamicOffset = bindingToDynamicOffset;
		uint32 dynamicOffsetIndex = 0;
		for (uint32 i = 0; i < info.descriptorTypesCount; i++)
		{
			const auto& descriptor = info.descriptorTypes[i];
			writeDescriptors->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptors->dstBinding = i;
			writeDescriptors->descriptorCount = descriptor.count;
			writeDescriptors->descriptorType = descriptor.descriptorType;

			switch (writeDescriptors->descriptorType)
			{
			case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
				bindingToDynamicOffset[i] = dynamicOffsetIndex;
				dynamicOffsetIndex++;
				writeDescriptors->pBufferInfo = bufferInfo++;
				break;
			case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
			case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
				writeDescriptors->pBufferInfo = bufferInfo;
				bufferInfo += descriptor.count;
				break;
			case VK_DESCRIPTOR_TYPE_SAMPLER:
			case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
			case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
			case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
				writeDescriptors->pImageInfo = imageInfo;
				imageInfo += descriptor.count;
				break;
			case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
			case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
				writeDescriptors->pTexelBufferView = texelBufferView;
				texelBufferView += descriptor.count;
				break;
			default:
				ENGINE_UNREACHABLE_CODE();
				break;
			}

			writeDescriptors++;
		}
		return dynamicOffsetIndex;
	}
}