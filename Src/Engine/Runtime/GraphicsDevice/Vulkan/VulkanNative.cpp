#include "VulkanNative.h"
#include "VulkanTool.h"
#include "GPUDeviceVulkan.h"
#include "CmdBufferVulkan.h"
#include "GPUTextureVulkan.h"
#include "GPUBufferVulkan.h"
#include "QueueVulkan.h"

#include "Runtime/Engine.h"
//#include "Runtime/RHI/VulkanRHI/VulkanRHI.h"
//#include "Runtime/RHI/VulkanRHI/VulkanConvert.h"
#include "Runtime/Core/Types/Collections/ListExtensions.h"


namespace SE
{

	FenceManagerVulkan::~FenceManagerVulkan()
	{
		ENGINE_ASSERT(m_UsedFences.IsEmpty());
	}

	void FenceManagerVulkan::Dispose()
	{
		Threading::ScopeLock lock(m_Device->m_FenceLock);
		ENGINE_ASSERT(m_UsedFences.IsEmpty());
		for (FenceVulkan* fence : m_FreeFences)
		{
			DestroyFence(fence);
		}
		m_UsedFences.Clear();
	}

	FenceVulkan* FenceManagerVulkan::AllocateFence(bool createSignaled)
	{
		Threading::ScopeLock lock(m_Device->m_FenceLock);
		FenceVulkan* fence;
		if (m_FreeFences.HasItems())
		{
			fence = m_FreeFences.Last();
			m_FreeFences.RemoveLast();
			m_UsedFences.Add(fence);
			if (createSignaled)
			{
				fence->IsSignaled = true;
			}
		}
		else
		{
			fence = New<FenceVulkan>();
			fence->IsSignaled = createSignaled;
			VkFenceCreateInfo info;
			VulkanTool::ZeroStruct(info, VK_STRUCTURE_TYPE_FENCE_CREATE_INFO);
			info.flags = createSignaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;
			VALIDATE_VULKAN_RESULT(vkCreateFence(m_Device->device, &info, nullptr, &fence->handle));
			m_UsedFences.Add(fence);
		}
		return fence;
	}

	bool FenceManagerVulkan::WaitForFence(FenceVulkan* fence, uint64 timeInNanoseconds) const
	{
		ENGINE_ASSERT(m_UsedFences.Contains(fence));
		ENGINE_ASSERT(!fence->IsSignaled);
		const VkResult result = vkWaitForFences(m_Device->device, 1, &fence->handle, true, timeInNanoseconds);
		LOG_VULKAN_RESULT(result);
		if (result == VK_SUCCESS)
		{
			fence->IsSignaled = true;
			return false;
		}
		return true;
	}

	void FenceManagerVulkan::ResetFence(FenceVulkan* fence) const
	{
		if (fence->IsSignaled)
		{
			VALIDATE_VULKAN_RESULT(vkResetFences(m_Device->device, 1, &fence->handle));
			fence->IsSignaled = false;
		}
	}

	void FenceManagerVulkan::ReleaseFence(FenceVulkan*& fence)
	{
		Threading::ScopeLock lock(m_Device->m_FenceLock);
		ResetFence(fence);
		m_UsedFences.Remove(fence);
		m_FreeFences.Add(fence);
		fence = nullptr;
	}

	void FenceManagerVulkan::WaitAndReleaseFence(FenceVulkan*& fence, uint64 timeInNanoseconds)
	{
		Threading::ScopeLock lock(m_Device->m_FenceLock);
		if (!fence->IsSignaled)
		{
			WaitForFence(fence, timeInNanoseconds);
		}
		ResetFence(fence);
		m_UsedFences.Remove(fence);
		m_FreeFences.Add(fence);
		fence = nullptr;
	}

	bool FenceManagerVulkan::CheckFenceState(FenceVulkan* fence) const
	{
		ENGINE_ASSERT(m_UsedFences.Contains(fence));
		ENGINE_ASSERT(!fence->IsSignaled);
		const VkResult result = vkGetFenceStatus(m_Device->device, fence->handle);
		if (result == VK_SUCCESS)
		{
			fence->IsSignaled = true;
			return true;
		}
		return false;
	}

	void FenceManagerVulkan::DestroyFence(FenceVulkan* fence) const
	{
		vkDestroyFence(m_Device->device, fence->handle, nullptr);
		fence->handle = VK_NULL_HANDLE;
		Delete(fence);
	}



	SemaphoreVulkan::SemaphoreVulkan(GPUDeviceVulkan* device)
		: m_Device(device)
	{
		VkSemaphoreCreateInfo info;
		VulkanTool::ZeroStruct(info, VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO);
		VALIDATE_VULKAN_RESULT(vkCreateSemaphore(device->device, &info, nullptr, &m_SemaphoreHandle));
	}

	SemaphoreVulkan::~SemaphoreVulkan()
	{
		ENGINE_ASSERT(m_SemaphoreHandle != VK_NULL_HANDLE);
		m_Device->deferredDeletionQueue.EnqueueResource(DeferredDeletionQueueVulkan::Semaphore, m_SemaphoreHandle);
		m_SemaphoreHandle = VK_NULL_HANDLE;
	}

	FramebufferVulkan::FramebufferVulkan(GPUDeviceVulkan* device, const Desc& key, const VkExtent2D& extent, uint32 layers)
		: m_Device(device)
		, handle(VK_NULL_HANDLE)
		, extent(extent)
		, layers(layers)
	{
		Platform::MemoryCopy(attachments, key.attachments, sizeof(attachments));

		VkFramebufferCreateInfo createInfo;
		VulkanTool::ZeroStruct(createInfo, VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO);
		createInfo.renderPass = key.renderPass->handle;
		createInfo.attachmentCount = key.attachmentCount;
		createInfo.pAttachments = key.attachments;
		createInfo.width = extent.width;
		createInfo.height = extent.height;
		createInfo.layers = layers;
		VALIDATE_VULKAN_RESULT(vkCreateFramebuffer(device->device, &createInfo, nullptr, &handle));
	}

	FramebufferVulkan::~FramebufferVulkan()
	{
		m_Device->deferredDeletionQueue.EnqueueResource(DeferredDeletionQueueVulkan::Type::Framebuffer, handle);
	}

	bool FramebufferVulkan::HasReference(VkImageView imageView) const
	{
		for (int32 i = 0; i < ARRAY_SIZE(attachments); i++)
		{
			if (attachments[i] == imageView)
				return true;
		}
		return false;
	}

	RenderPassVulkan::RenderPassVulkan(GPUDeviceVulkan* device, const RenderTargetLayoutVulkan& layout)
		: m_Device(device)
		, handle(VK_NULL_HANDLE)
		, layout(layout)
	{
		const int32 colorAttachmentsCount = layout.RTsCount;
		const bool hasDepthStencilAttachment = layout.DepthFormat != PixelFormat::Undefined;
		const int32 attachmentsCount = colorAttachmentsCount + (hasDepthStencilAttachment ? 1 : 0);

		VkAttachmentReference colorReferences[GPU_MAX_RT_BINDED];
		VkAttachmentReference depthStencilReference;
		VkAttachmentDescription attachments[GPU_MAX_RT_BINDED + 1];

		VkSubpassDescription subpassDesc;
		Platform::MemoryClear(&subpassDesc, sizeof(subpassDesc));
		subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpassDesc.colorAttachmentCount = colorAttachmentsCount;
		subpassDesc.pColorAttachments = colorReferences;
		subpassDesc.pResolveAttachments = nullptr;
		for (int32 i = 0; i < colorAttachmentsCount; i++)
		{
			VkAttachmentDescription& attachment = attachments[i];
			attachment.flags = 0;
			attachment.format = VulkanTool::ToVulkanFormat(layout.RTVsFormats[i]);
			attachment.samples = (VkSampleCountFlagBits)layout.MSAA;
#if PLATFORM_ANDROID
			attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD; // TODO: Adreno 640 has glitches when blend is disabled and rt data not loaded
#elif PLATFORM_MAC || PLATFORM_IOS
			attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD; // MoltenVK seams to have glitches (tiled arch of gpu)
#else
			// TODO: we need render passes into high-level rendering api to perform more optimal rendering (esp. for tiled gpus)
			attachment.loadOp = layout.BlendEnable ? VK_ATTACHMENT_LOAD_OP_LOAD : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
#endif
			attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			VkAttachmentReference& reference = colorReferences[i];
			reference.attachment = i;
			reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		}
		if (hasDepthStencilAttachment)
		{
			VkImageLayout depthStencilLayout;
#if 0
			// TODO: enable extension and use separateDepthStencilLayouts from Vulkan 1.2
        if (layout.ReadStencil || layout.WriteStencil)
        {
            if (layout.WriteDepth && layout.WriteStencil)
                depthStencilLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            else if (layout.WriteDepth && !layout.WriteStencil)
                depthStencilLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
            else if (layout.WriteStencil && !layout.WriteDepth)
                depthStencilLayout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL;
            else if (layout.ReadDepth)
                depthStencilLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
            else
                depthStencilLayout = VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL;
        }
        else
        {
            // Depth-only
            if (layout.ReadDepth && !layout.WriteDepth)
                depthStencilLayout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL;
            else
                depthStencilLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
        }
#else
			if ((layout.ReadDepth || layout.ReadStencil) && !(layout.WriteDepth || layout.WriteStencil))
				depthStencilLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
			else
				depthStencilLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
#endif

			// Use last slot for depth stencil attachment
			VkAttachmentDescription& attachment = attachments[colorAttachmentsCount];
			attachment.flags = 0;
			attachment.format = VulkanTool::ToVulkanFormat(layout.DepthFormat);
			attachment.samples = (VkSampleCountFlagBits)layout.MSAA;
			attachment.loadOp = layout.ReadDepth || layout.ReadStencil ? VK_ATTACHMENT_LOAD_OP_LOAD : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			//attachment.storeOp = layout.WriteDepth || layout.WriteStencil ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // For some reason, read-only depth results in artifacts
			attachment.stencilLoadOp = layout.ReadStencil ? VK_ATTACHMENT_LOAD_OP_LOAD : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachment.stencilStoreOp = layout.WriteStencil ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachment.initialLayout = depthStencilLayout;
			attachment.finalLayout = depthStencilLayout;
			depthStencilReference.attachment = colorAttachmentsCount;
			depthStencilReference.layout = depthStencilLayout;
			subpassDesc.pDepthStencilAttachment = &depthStencilReference;
		}

		VkRenderPassCreateInfo createInfo;
		VulkanTool::ZeroStruct(createInfo, VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO);
		createInfo.attachmentCount = attachmentsCount;
		createInfo.pAttachments = attachments;
		createInfo.subpassCount = 1;
		createInfo.pSubpasses = &subpassDesc;
		VALIDATE_VULKAN_RESULT(vkCreateRenderPass(device->device, &createInfo, nullptr, &handle));
#if VULKAN_USE_DEBUG_DATA
		debugCreateInfo = createInfo;
#endif
	}

	RenderPassVulkan::~RenderPassVulkan()
	{
		m_Device->deferredDeletionQueue.EnqueueResource(DeferredDeletionQueueVulkan::Type::RenderPass, handle);
	}

	DeferredDeletionQueueVulkan::DeferredDeletionQueueVulkan(GPUDeviceVulkan* device)
		: _device(device)
	{
	}

	DeferredDeletionQueueVulkan::~DeferredDeletionQueueVulkan()
	{
		ENGINE_ASSERT(_entries.IsEmpty());
	}

	void DeferredDeletionQueueVulkan::ReleaseResources(bool immediately)
	{
		const uint64 checkFrame = Engine::FrameCount - VULKAN_RESOURCE_DELETE_SAFE_FRAMES_COUNT;
		Threading::ScopeLock lock(_locker);
		for (int32 i = 0; i < _entries.Count(); i++)
		{
			Entry* e = &_entries.Get()[i];
			if (immediately || (checkFrame > e->FrameNumber && (e->CmdBuffer == nullptr || e->FenceCounter < e->CmdBuffer->GetFenceSignaledCounter())))
			{
				if (e->AllocationHandle == VK_NULL_HANDLE)
				{
					switch (e->StructureType)
					{
					#define SWITCH_CASE(type) case Type::type: vkDestroy##type(_device->device, (Vk##type)e->Handle, nullptr); break
					SWITCH_CASE(RenderPass);
					SWITCH_CASE(Buffer);
					SWITCH_CASE(BufferView);
					SWITCH_CASE(Image);
					SWITCH_CASE(ImageView);
					SWITCH_CASE(Pipeline);
					SWITCH_CASE(PipelineLayout);
					SWITCH_CASE(Framebuffer);
					SWITCH_CASE(DescriptorSetLayout);
					SWITCH_CASE(Sampler);
					SWITCH_CASE(Semaphore);
					SWITCH_CASE(ShaderModule);
					SWITCH_CASE(Event);
					SWITCH_CASE(QueryPool);
					#undef SWITCH_CASE
					default:
						ENGINE_UNREACHABLE_CODE();
						break;
					}
				}
				else
				{
					if (e->StructureType == Image)
					{
						vmaDestroyImage(_device->allocator, (VkImage)e->Handle, e->AllocationHandle);
					}
					else if (e->StructureType == Buffer)
					{
						vmaDestroyBuffer(_device->allocator, (VkBuffer)e->Handle, e->AllocationHandle);
					}
					else
					{
						ENGINE_UNREACHABLE_CODE();
					}
				}

				_entries.RemoveAt(i--);
				if (_entries.IsEmpty())
				{
					break;
				}
			}
		}
	}

	void DeferredDeletionQueueVulkan::EnqueueGenericResource(Type type, uint64 handle, VmaAllocation allocation)
	{
		ENGINE_ASSERT(handle != 0);

		Entry entry;
		_device->graphicsQueue->GetLastSubmittedInfo(entry.CmdBuffer, entry.FenceCounter);
		entry.Handle = handle;
		entry.AllocationHandle = allocation;
		entry.StructureType = type;
		entry.FrameNumber = Engine::FrameCount;

		Threading::ScopeLock lock(_locker);
//#if BUILD_DEBUG && 0
		Function<bool(const Entry&)> Containshandle = [handle](const Entry& e)
		{
			return e.Handle == handle;
		};
		ENGINE_ASSERT(!ListExtensions::Any(_entries, Containshandle));
//#endif
		_entries.Add(entry);
	}


	StagingManagerVulkan::StagingManagerVulkan(GPUDeviceVulkan* device)
		: m_Device(device)
	{
	}

	GPUBuffer* StagingManagerVulkan::AcquireBuffer(uint32 size, GPUResourceUsage usage)
	{
		// Try reuse free buffer
		{
			Threading::ScopeLock lock(m_Locker);

			for (int32 i = 0; i < m_FreeBuffers.Count(); i++)
			{
				auto& freeBuffer = m_FreeBuffers[i];
				if (freeBuffer.Buffer->GetSize() == size && freeBuffer.Buffer->GetDescription().Usage == usage)
				{
					const auto buffer = freeBuffer.Buffer;
					m_FreeBuffers.RemoveAt(i);
					return buffer;
				}
			}
		}

		// Allocate new buffer
		auto buffer = m_Device->CreateBuffer(TEXT("Pooled Staging"));
		if (!buffer->Init(GPUBufferDescription::Buffer(size, GPUBufferFlags::None, PixelFormat::Undefined, nullptr, 0, usage)))
		{
			LOG_WARNING("Graphic", "StagingManagerVulkan Failed to create pooled staging buffer.");
			return nullptr;
		}

		// Cache buffer
		{
			Threading::ScopeLock lock(m_Locker);

			m_AllBuffers.Add(buffer);

			m_AllBuffersAllocSize += size;
			m_AllBuffersTotalSize += size;
			m_AllBuffersPeekSize = Math::Max(m_AllBuffersTotalSize, m_AllBuffersPeekSize);
		}

		return buffer;
	}

	void StagingManagerVulkan::ReleaseBuffer(CmdBufferVulkan* cmdBuffer, GPUBuffer*& buffer)
	{
		Threading::ScopeLock lock(m_Locker);

		if (cmdBuffer)
		{
			// Return to pending pool (need to wait until command buffer will be executed and buffer will be reusable)
			auto& item = m_PendingBuffers.AddOne();
			item.Buffer = buffer;
			item.CmdBuffer = cmdBuffer;
			item.FenceCounter = cmdBuffer->GetFenceSignaledCounter();
		}
		else
		{
			// Return to pool
			FreeEntry& freeEntry = m_FreeBuffers.AddOne();
			freeEntry.Buffer = buffer;
			freeEntry.FrameNumber = Engine::FrameCount;
		}

		// Clear reference
		buffer = nullptr;
	}

	void StagingManagerVulkan::ProcessPendingFree()
	{
		Threading::ScopeLock lock(m_Locker);

		// Find staging buffers that has been processed by the GPU and can be reused
		for (int32 i = m_PendingBuffers.Count() - 1; i >= 0; i--)
		{
			auto& pending = m_PendingBuffers[i];
			if (pending.FenceCounter < pending.CmdBuffer->GetFenceSignaledCounter())
			{
				// Return to pool
				FreeEntry& freeEntry = m_FreeBuffers.AddOne();
				freeEntry.Buffer = pending.Buffer;
				freeEntry.FrameNumber = Engine::FrameCount;

				m_PendingBuffers.RemoveAt(i);
			}
		}

		// Free staging buffers that has not been used for a few frames
		for (int32 i = m_FreeBuffers.Count() - 1; i >= 0; i--)
		{
			auto& freeEntry = m_FreeBuffers.Get()[i];
			if (freeEntry.FrameNumber + VULKAN_RESOURCE_DELETE_SAFE_FRAMES_COUNT < Engine::FrameCount)
			{
				auto buffer = freeEntry.Buffer;

				// Remove buffer from lists
				m_AllBuffers.Remove(buffer);
				m_FreeBuffers.RemoveAt(i);

				// Update stats
				m_AllBuffersFreeSize += buffer->GetSize();
				m_AllBuffersTotalSize -= buffer->GetSize();

				// Release memory
				DeleteObjectSafe(buffer);
			}
		}
	}

	void StagingManagerVulkan::Dispose()
	{
		Threading::ScopeLock lock(m_Locker);

		LOG_INFO("Graphic", "StagingManagerVulkan Vulkan staging buffers peek memory usage: {0}, allocs: {1}, frees: {2}",
			StringUtils::ToString(m_AllBuffersPeekSize),
			StringUtils::ToString(m_AllBuffersAllocSize),
			StringUtils::ToString(m_AllBuffersFreeSize));

		// Release buffers and clear memory
		for (auto& buffer : m_AllBuffers)
		{
			DeleteObjectSafe(buffer);
		}
		m_AllBuffers.Resize(0);
		m_PendingBuffers.Resize(0);
	}


	QueryPoolVulkan::QueryPoolVulkan(GPUDeviceVulkan* device, int32 capacity, VkQueryType type)
		: m_Device(device)
		, m_Handle(VK_NULL_HANDLE)
		, m_Count(0)
		, m_Capacity(capacity)
		, m_Type(type)
	{
		VkQueryPoolCreateInfo createInfo;
		VulkanTool::ZeroStruct(createInfo, VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO);
		createInfo.queryType = type;
		createInfo.queryCount = capacity;
		VALIDATE_VULKAN_RESULT(vkCreateQueryPool(device->device, &createInfo, nullptr, &m_Handle));
#if VULKAN_RESET_QUERY_POOLS
		m_ResetRanges.Add(Range{ 0, static_cast<uint32>(capacity) });
    	device->QueriesToReset.Add(this);
#endif
	}

	QueryPoolVulkan::~QueryPoolVulkan()
	{
#if VULKAN_RESET_QUERY_POOLS
		_device->QueriesToReset.Remove(this);
#endif
		m_Device->deferredDeletionQueue.EnqueueResource(DeferredDeletionQueueVulkan::Type::QueryPool, m_Handle);
	}


	BufferedQueryPoolVulkan::BufferedQueryPoolVulkan(GPUDeviceVulkan* device, int32 capacity, VkQueryType type)
		: QueryPoolVulkan(device, capacity, type)
		, m_LastBeginIndex(0)
	{
		m_QueryOutput.Resize(capacity);
		m_UsedQueryBits.AddZeroed((capacity + 63) / 64);
		m_StartedQueryBits.AddZeroed((capacity + 63) / 64);
		m_ReadResultsBits.AddZeroed((capacity + 63) / 64);
	}

	bool BufferedQueryPoolVulkan::AcquireQuery(uint32& resultIndex)
	{
		const uint64 allUsedMask = (uint64)-1;
		for (int32 wordIndex = m_LastBeginIndex / 64; wordIndex < m_UsedQueryBits.Count(); wordIndex++)
		{
			uint64 beginQueryWord = m_UsedQueryBits[wordIndex];
			if (beginQueryWord != allUsedMask)
			{
				resultIndex = 0;
				while ((beginQueryWord & 1) == 1)
				{
					resultIndex++;
					beginQueryWord >>= 1;
				}
				resultIndex += wordIndex * 64;
				const uint64 bit = (uint64)1 << (uint64)(resultIndex % 64);
				m_UsedQueryBits[wordIndex] = m_UsedQueryBits[wordIndex] | bit;
				m_ReadResultsBits[wordIndex] &= ~bit;
				m_LastBeginIndex = resultIndex + 1;
				return true;
			}
		}

		return false;
	}

	void BufferedQueryPoolVulkan::ReleaseQuery(uint32 queryIndex)
	{
		const uint32 word = queryIndex / 64;
		const uint64 bit = (uint64)1 << (queryIndex % 64);
		m_UsedQueryBits[word] = m_UsedQueryBits[word] & ~bit;
		m_ReadResultsBits[word] = m_ReadResultsBits[word] & ~bit;
		if (queryIndex < (uint32)m_LastBeginIndex)
		{
			// Use the lowest word available
			const uint64 allUsedMask = (uint64)-1;
			const int32 lastQueryWord = m_LastBeginIndex / 64;
			if (lastQueryWord < m_UsedQueryBits.Count() && m_UsedQueryBits[lastQueryWord] == allUsedMask)
			{
				m_LastBeginIndex = (uint32)queryIndex;
			}
		}
	}

	void BufferedQueryPoolVulkan::MarkQueryAsStarted(uint32 queryIndex)
	{
		const uint32 word = queryIndex / 64;
		const uint64 bit = (uint64)1 << (queryIndex % 64);
		m_StartedQueryBits[word] = m_StartedQueryBits[word] | bit;
	}

	bool BufferedQueryPoolVulkan::GetResults(GPUContextVulkan* context, uint32 index, uint64& result)
	{
		const uint64 bit = (uint64)(index % 64);
		const uint64 bitMask = (uint64)1 << bit;
		const uint32 word = index / 64;

		if ((m_StartedQueryBits[word] & bitMask) == 0)
		{
			// Query never started/ended
			result = 0;
			return true;
		}

		if ((m_ReadResultsBits[word] & bitMask) == 0)
		{
			const VkResult vkResult = vkGetQueryPoolResults(m_Device->device, m_Handle, index, 1, sizeof(uint64), &m_QueryOutput[index], sizeof(uint64), VK_QUERY_RESULT_64_BIT);
			if (vkResult == VK_SUCCESS)
			{
				m_ReadResultsBits[word] = m_ReadResultsBits[word] | bitMask;

#if VULKAN_RESET_QUERY_POOLS
				// Add to reset
            if (!_device->QueriesToReset.Contains(this))
                m_Device->QueriesToReset.Add(this);
            m_ResetRanges.Add(Range{ index, 1 });
#endif
			}
			else if (vkResult == VK_NOT_READY)
			{
				// No results yet
				result = 0;
				return false;
			}
			else
			{
				LOG_VULKAN_RESULT(vkResult);
			}
		}

		result = m_QueryOutput[index];
		return true;
	}

	bool BufferedQueryPoolVulkan::HasRoom() const
	{
		const uint64 allUsedMask = (uint64)-1;
		if (m_LastBeginIndex < m_UsedQueryBits.Count() * 64)
		{
			ENGINE_ASSERT((m_UsedQueryBits[m_LastBeginIndex / 64] & allUsedMask) != allUsedMask);
			return true;
		}
		return false;
	}


	DummyResourcesVulkan::DummyResourcesVulkan(GPUDeviceVulkan* device)
		: _device(device)
		, _dummyBuffer(nullptr)
		, _dummyVB(nullptr)
	{
		Platform::MemoryClear(_dummyTextures, sizeof(_dummyTextures));
		Platform::MemoryClear(_staticSamplers, sizeof(_staticSamplers));
	}

	void InitSampler(VkSamplerCreateInfo& createInfo, bool supportsMirrorClampToEdge, GPUSamplerFilter filter, GPUSamplerAddressMode addressU, GPUSamplerAddressMode addressV, GPUSamplerAddressMode addressW, GPUSamplerCompareFunction compareFunction)
	{
		createInfo.magFilter = VulkanTool::ToVulkanMagFilterMode(filter);
		createInfo.minFilter = VulkanTool::ToVulkanMinFilterMode(filter);
		createInfo.mipmapMode = VulkanTool::ToVulkanMipFilterMode(filter);
		createInfo.addressModeU = VulkanTool::ToVulkanWrapMode(addressU, supportsMirrorClampToEdge);
		createInfo.addressModeV = VulkanTool::ToVulkanWrapMode(addressV, supportsMirrorClampToEdge);
		createInfo.addressModeW = VulkanTool::ToVulkanWrapMode(addressW, supportsMirrorClampToEdge);
		createInfo.compareEnable = compareFunction != GPUSamplerCompareFunction::Never ? VK_TRUE : VK_FALSE;
		createInfo.compareOp = VulkanTool::ToVulkanSamplerCompareFunction(compareFunction);
	}

	VkSampler* DummyResourcesVulkan::GetStaticSamplers()
	{
		static_assert(GPU_STATIC_SAMPLERS_COUNT == 6, "Update static samplers setup.");
		if (!_staticSamplers[0])
		{
			const bool supportsMirrorClampToEdge = GPUDeviceVulkan::optionalDeviceExtensions.HasMirrorClampToEdge;

			VkSamplerCreateInfo createInfo;
			VulkanTool::ZeroStruct(createInfo, VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO);
			createInfo.mipLodBias = 0.0f;
			createInfo.minLod = 0.0f;
			createInfo.maxLod = Max_float;
			createInfo.maxAnisotropy = 1.0f;
			createInfo.anisotropyEnable = VK_FALSE;
			createInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;

			// Linear Clamp
			InitSampler(createInfo, supportsMirrorClampToEdge, GPUSamplerFilter::Trilinear, GPUSamplerAddressMode::Clamp, GPUSamplerAddressMode::Clamp, GPUSamplerAddressMode::Clamp, GPUSamplerCompareFunction::Never);
			VALIDATE_VULKAN_RESULT(vkCreateSampler(_device->device, &createInfo, nullptr, &_staticSamplers[0]));

			// Point Clamp
			InitSampler(createInfo, supportsMirrorClampToEdge, GPUSamplerFilter::Point, GPUSamplerAddressMode::Clamp, GPUSamplerAddressMode::Clamp, GPUSamplerAddressMode::Clamp, GPUSamplerCompareFunction::Never);
			VALIDATE_VULKAN_RESULT(vkCreateSampler(_device->device, &createInfo, nullptr, &_staticSamplers[1]));

			// Linear Wrap
			InitSampler(createInfo, supportsMirrorClampToEdge, GPUSamplerFilter::Trilinear, GPUSamplerAddressMode::Wrap, GPUSamplerAddressMode::Wrap, GPUSamplerAddressMode::Wrap, GPUSamplerCompareFunction::Never);
			VALIDATE_VULKAN_RESULT(vkCreateSampler(_device->device, &createInfo, nullptr, &_staticSamplers[2]));

			// Point Wrap
			InitSampler(createInfo, supportsMirrorClampToEdge, GPUSamplerFilter::Point, GPUSamplerAddressMode::Wrap, GPUSamplerAddressMode::Wrap, GPUSamplerAddressMode::Wrap, GPUSamplerCompareFunction::Never);
			VALIDATE_VULKAN_RESULT(vkCreateSampler(_device->device, &createInfo, nullptr, &_staticSamplers[3]));

			// Shadow
			InitSampler(createInfo, supportsMirrorClampToEdge, GPUSamplerFilter::Point, GPUSamplerAddressMode::Clamp, GPUSamplerAddressMode::Clamp, GPUSamplerAddressMode::Clamp, GPUSamplerCompareFunction::Less);
			VALIDATE_VULKAN_RESULT(vkCreateSampler(_device->device, &createInfo, nullptr, &_staticSamplers[4]));

			// Shadow PCF
			InitSampler(createInfo, supportsMirrorClampToEdge, GPUSamplerFilter::Trilinear, GPUSamplerAddressMode::Clamp, GPUSamplerAddressMode::Clamp, GPUSamplerAddressMode::Clamp, GPUSamplerCompareFunction::Less);
			VALIDATE_VULKAN_RESULT(vkCreateSampler(_device->device, &createInfo, nullptr, &_staticSamplers[5]));
		}
		return _staticSamplers;
	}

	GPUTextureVulkan* DummyResourcesVulkan::GetDummyTexture(SpirvShaderResourceType type)
	{
		int32 index;
		switch (type)
		{
		case SpirvShaderResourceType::Texture1D:
			index = 0;
			break;
		case SpirvShaderResourceType::Texture2D:
			index = 1;
			break;
		case SpirvShaderResourceType::Texture3D:
			index = 2;
			break;
		case SpirvShaderResourceType::TextureCube:
			index = 3;
			break;
		case SpirvShaderResourceType::Texture1DArray:
			index = 4;
			break;
		case SpirvShaderResourceType::Texture2DArray:
			index = 5;
			break;
		default:
			ENGINE_UNREACHABLE_CODE();
			return nullptr;
		}

		auto texture = _dummyTextures[index];
		if (!texture)
		{
			texture = (GPUTextureVulkan*)_device->CreateTexture(SE_TEXT("DummyTexture"));
			GPUTextureDescription desc;
			const PixelFormat format = PixelFormat::R8G8B8A8_UNorm;
			const GPUTextureBitFlags flags = GPUTextureBitFlags(GPUTextureFlags::ShaderResource, GPUTextureFlags::UnorderedAccess);
			switch (type)
			{
			case SpirvShaderResourceType::Texture1D:
				desc = GPUTextureDescription::New1D(1, 1, format, flags, 1);
				break;
			case SpirvShaderResourceType::Texture2D:
				desc = GPUTextureDescription::New2D(1, 1, format, flags);
				break;
			case SpirvShaderResourceType::Texture3D:
				desc = GPUTextureDescription::New3D(1, 1, 1, format, flags);
				break;
			case SpirvShaderResourceType::TextureCube:
				desc = GPUTextureDescription::NewCube(1, format, flags);
				break;
			case SpirvShaderResourceType::Texture1DArray:
				desc = GPUTextureDescription::New1D(1, 1, format, flags, 4);
				break;
			case SpirvShaderResourceType::Texture2DArray:
				desc = GPUTextureDescription::New2D(1, 1, format, flags, 4);
				break;
			default: ;
			}
			texture->Init(desc);
			ENGINE_ASSERT(texture->View(0));
			_dummyTextures[index] = texture;
		}

		return texture;
	}

	GPUBufferVulkan* DummyResourcesVulkan::GetDummyBuffer()
	{
		if (!_dummyBuffer)
		{
			_dummyBuffer = (GPUBufferVulkan*)_device->CreateBuffer(SE_TEXT("DummyBuffer"));
			_dummyBuffer->Init(GPUBufferDescription::Buffer(sizeof(int32) * 256, EnumFlags<GPUBufferFlags>(GPUBufferFlags::ShaderResource, GPUBufferFlags::UnorderedAccess), PixelFormat::R32_SInt));
		}

		return _dummyBuffer;
	}

	GPUBufferVulkan* DummyResourcesVulkan::GetDummyVertexBuffer()
	{
		if (!_dummyVB)
		{
			_dummyVB = (GPUBufferVulkan*)_device->CreateBuffer(TEXT("DummyVertexBuffer"));
			_dummyVB->Init(GPUBufferDescription::Vertex(sizeof(Color32), 1, &Colors::Transparent));
		}

		return _dummyVB;
	}

	void DummyResourcesVulkan::Dispose()
	{
		for (GPUTextureVulkan*& textureVulkan : _dummyTextures)
		{
			DeleteObjectSafe(textureVulkan);
		}

		if (_dummyBuffer)
		{
			DeleteObjectSafe(_dummyBuffer);
		};

		if (_dummyVB)
		{
			DeleteObjectSafe(_dummyVB);
		};

		for (int32 i = 0; i < ARRAY_SIZE(_staticSamplers); i++)
		{
			auto& sampler = _staticSamplers[i];
			if (sampler != VK_NULL_HANDLE)
			{
				_device->deferredDeletionQueue.EnqueueResource(DeferredDeletionQueueVulkan::Type::Sampler, sampler);
				sampler = VK_NULL_HANDLE;
			}
		}
	}

	uint32 GetHash(const RenderTargetLayoutVulkan& key)
	{
		uint32 hash = (int32)key.MSAA * 11;
		hash = HashCombine(hash, key.Flags);
		hash = HashCombine(hash, (uint32)key.DepthFormat * 93473262);
		hash = HashCombine(hash, key.Extent.width);
		hash = HashCombine(hash, key.Extent.height);
		for (int32 i = 0; i < ARRAY_SIZE(key.RTVsFormats); i++)
		{
			hash = HashCombine(hash, (uint32)key.RTVsFormats[i]);
		}
		return hash;
	}

	uint32 GetHash(const FramebufferVulkan::Desc& key)
	{
		uint32 hash = (int32)(intptr)key.renderPass;
		hash = HashCombine(hash, (uint32)key.attachmentCount * 136);
		for (int32 i = 0; i < ARRAY_SIZE(key.attachments); i++)
		{
			hash = HashCombine(hash, (int32)(intptr)&key.attachments[i]);
		}
		return hash;
	}

	uint32 GetHash(RenderPassVulkan* key)
	{
		uint32 hash = Hash::XXHash::GetHash32(&key->handle, sizeof(VkRenderPass));

		return HashCombine(hash, GetHash(key->layout));
	}
}

