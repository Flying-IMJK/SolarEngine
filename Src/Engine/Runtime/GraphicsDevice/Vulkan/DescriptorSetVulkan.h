#pragma once

#include "Runtime/Core/Types/Variable.h"
#include "Runtime/Core/Types/Collections/List.h"
#include "Runtime/Core/Types/Collections/Dictionary.h"
#include "Runtime/Core/Thread/Threading.h"

#include "VulkanTypes.h"
#include "GPUDeviceVulkan.h"

namespace SE
{
	#define VULKAN_DESCRIPTOR_TYPE_BEGIN VK_DESCRIPTOR_TYPE_SAMPLER
	#define VULKAN_DESCRIPTOR_TYPE_END VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT

	class DescriptorSetLayoutInfoVulkan
	{
	public:
		struct SetLayout
		{
			List<VkDescriptorSetLayoutBinding> LayoutBindings;
		};

		uint32 hash = 0;
		uint32 setLayoutsHash = 0;
		uint32 layoutTypes[VULKAN_DESCRIPTOR_TYPE_END];
		List<SetLayout> setLayouts;

	public:
		DescriptorSetLayoutInfoVulkan()
		{
			Platform::MemoryClear(layoutTypes, sizeof(layoutTypes));
		}

		void CopyFrom(const DescriptorSetLayoutInfoVulkan& info)
		{
			Platform::MemoryCopy(layoutTypes, info.layoutTypes, sizeof(layoutTypes));
			hash = info.hash;
			setLayoutsHash = info.setLayoutsHash;
			setLayouts = info.setLayouts;
		}

		bool operator==(const DescriptorSetLayoutInfoVulkan& other) const;

		friend inline uint32 GetHash(const DescriptorSetLayoutInfoVulkan& key)
		{
			return key.hash;
		}
	};

	class DescriptorSetLayoutVulkan : public DescriptorSetLayoutInfoVulkan
	{
	public:
		typedef List<VkDescriptorSetLayout, InlinedAllocation<5>> HandlesArray;

		GPUDeviceVulkan* device;
		HandlesArray handles;
		VkDescriptorSetAllocateInfo allocateInfo;

		DescriptorSetLayoutVulkan(GPUDeviceVulkan* device);
		~DescriptorSetLayoutVulkan();

		void Compile();

		friend inline uint32 GetHash(const DescriptorSetLayoutVulkan& key)
		{
			return key.hash;
		}
	};

	class DescriptorPoolVulkan
	{
	private:
		GPUDeviceVulkan* m_Device;
		VkDescriptorPool m_Handle;

		uint32 m_DescriptorSetsMax;
		uint32 m_AllocatedDescriptorSetsCount;
		uint32 m_AllocatedDescriptorSetsCountMax;

		const DescriptorSetLayoutVulkan& m_Layout;

	public:
		DescriptorPoolVulkan(GPUDeviceVulkan* device, const DescriptorSetLayoutVulkan& layout);
		~DescriptorPoolVulkan();

	public:
		inline VkDescriptorPool GetHandle() const
		{
			return m_Handle;
		}

		inline bool IsEmpty() const
		{
			return m_AllocatedDescriptorSetsCount == 0;
		}

		inline bool CanAllocate(const DescriptorSetLayoutVulkan& layout) const
		{
			return m_DescriptorSetsMax > m_AllocatedDescriptorSetsCount + layout.setLayouts.Count();
		}

		inline uint32 GetAllocatedDescriptorSetsCount() const
		{
			return m_AllocatedDescriptorSetsCount;
		}

	public:
		void Track(const DescriptorSetLayoutVulkan& layout);
		void TrackRemoveUsage(const DescriptorSetLayoutVulkan& layout);
		void Reset();
		bool AllocateDescriptorSets(const VkDescriptorSetAllocateInfo& descriptorSetAllocateInfo, VkDescriptorSet* result);
	};

	class DescriptorPoolSetContainerVulkan;

	class TypedDescriptorPoolSetVulkan
	{
		friend DescriptorPoolSetContainerVulkan;

	private:
		GPUDeviceVulkan* m_Device;
		const DescriptorPoolSetContainerVulkan* m_Owner;
		const DescriptorSetLayoutVulkan& m_Layout;

		class PoolList
		{
		public:
			DescriptorPoolVulkan* element;
			PoolList* next;

			PoolList(DescriptorPoolVulkan* element, PoolList* next = nullptr) : element(element), next(next)
			{
			}
		};

		PoolList* m_PoolListHead = nullptr;
		PoolList* m_PoolListCurrent = nullptr;

	public:
		TypedDescriptorPoolSetVulkan(GPUDeviceVulkan* device, const DescriptorPoolSetContainerVulkan* owner, const DescriptorSetLayoutVulkan& layout)
			: m_Device(device)
			, m_Owner(owner)
			, m_Layout(layout)
		{
			PushNewPool();
		};

		~TypedDescriptorPoolSetVulkan();

		bool AllocateDescriptorSets(const DescriptorSetLayoutVulkan& layout, VkDescriptorSet* outSets);

		const DescriptorPoolSetContainerVulkan* GetOwner() const
		{
			return m_Owner;
		}

	private:
		DescriptorPoolVulkan* GetFreePool(bool forceNewPool = false);
		DescriptorPoolVulkan* PushNewPool();
		void Reset();
	};

	class DescriptorPoolSetContainerVulkan
	{
	private:
		GPUDeviceVulkan* m_Device;
		Dictionary<uint32, TypedDescriptorPoolSetVulkan*> m_TypedDescriptorPools;

	public:
		DescriptorPoolSetContainerVulkan(GPUDeviceVulkan* device);
		~DescriptorPoolSetContainerVulkan();

	public:
		TypedDescriptorPoolSetVulkan* AcquireTypedPoolSet(const DescriptorSetLayoutVulkan& layout);
		void Reset();

		mutable uint64 refs = 0;
		mutable uint64 lastFrameUsed;
	};

	class DescriptorPoolsManagerVulkan
	{
	private:
		GPUDeviceVulkan* m_Device = nullptr;
		CriticalSection m_Locker;
		List<DescriptorPoolSetContainerVulkan*> m_PoolSets;

	public:
		DescriptorPoolsManagerVulkan(GPUDeviceVulkan* device);
		~DescriptorPoolsManagerVulkan();

		DescriptorPoolSetContainerVulkan* AcquirePoolSetContainer();
		void GC();
	};

	class PipelineLayoutVulkan
	{
	public:
		GPUDeviceVulkan* device;
		VkPipelineLayout handle;
		DescriptorSetLayoutVulkan descriptorSetLayout;

		PipelineLayoutVulkan(GPUDeviceVulkan* device, const DescriptorSetLayoutInfoVulkan& layout);
		~PipelineLayoutVulkan();
	};

	struct DescriptorSetWriteContainerVulkan
	{
		List<VkDescriptorImageInfo> descriptorImageInfo;
		List<VkDescriptorBufferInfo> descriptorBufferInfo;
		List<VkBufferView> descriptorTexelBufferView;
		List<VkWriteDescriptorSet> descriptorWrites;
		List<byte> bindingToDynamicOffset;

		void Release()
		{
			descriptorImageInfo.Resize(0);
			descriptorBufferInfo.Resize(0);
			descriptorTexelBufferView.Resize(0);
			descriptorWrites.Resize(0);
			bindingToDynamicOffset.Resize(0);
		}
	};

}
