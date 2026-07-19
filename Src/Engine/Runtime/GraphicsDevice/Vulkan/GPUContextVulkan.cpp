
#include "GPUContextVulkan.h"
#include "Runtime/Graphics/Base/GPUUtils.h"

#include "CmdBufferVulkan.h"
#include "VulkanNative.h"
#include "DescriptorSetVulkan.h"
#include "GPUTextureVulkan.h"
#include "GPUBufferVulkan.h"
#include "GPUPipelineStateVulkan.h"
#include "GPUShaderProgramVulkan.h"
#include "GPUShaderVulkan.h"
#include "GPUSamplerVulkan.h"
#include "Runtime/Core/Math/Rectangle.h"

namespace SE
{
	void PipelineBarrierVulkan::Execute(const CmdBufferVulkan* cmdBuffer)
	{
		ENGINE_ASSERT(cmdBuffer->IsOutsideRenderPass());
		vkCmdPipelineBarrier(cmdBuffer->GetHandle(), SourceStage, DestStage,
			0, 0, nullptr, BufferBarriers.Count(),
			BufferBarriers.Get(), ImageBarriers.Count(), ImageBarriers.Get());

		// Reset
		SourceStage = 0;
		DestStage = 0;
		ImageBarriers.Clear();
		BufferBarriers.Clear();
#if VK_ENABLE_BARRIERS_DEBUG
		ImageBarriersDebug.Clear();
#endif
	}


	GPUContextVulkan::GPUContextVulkan(GPUDeviceVulkan* device, QueueVulkan* queue)
		: GPUContext((GPUDevice*)device)
		, _device(device)
		, _queue(queue)
		, _cmdBufferManager(New<CmdBufferManagerVulkan>(device, this))
	{
		Platform::MemoryClear(_rtHandles, sizeof(GPUTextureViewVulkan*) * GPU_MAX_RT_BINDED );
		Platform::MemoryClear(_cbHandles, sizeof(DescriptorResourceVulkan*) * GPU_MAX_CB_BINDED );
		Platform::MemoryClear(_srHandles, sizeof(DescriptorResourceVulkan*) * GPU_MAX_SR_BINDED );
		Platform::MemoryClear(_uaHandles, sizeof(DescriptorResourceVulkan*) * GPU_MAX_UA_BINDED );
		Platform::MemoryClear(_samplerHandles, sizeof(VkSampler) * GPU_MAX_SAMPLER_BINDED);


		// Setup descriptor handles tables lookup cache
		_handles[(int32)SpirvShaderResourceBindingType::INVALID] = nullptr;
		_handles[(int32)SpirvShaderResourceBindingType::CB] = _cbHandles;
		_handles[(int32)SpirvShaderResourceBindingType::SAMPLER] = nullptr;
		_handles[(int32)SpirvShaderResourceBindingType::SRV] = _srHandles;
		_handles[(int32)SpirvShaderResourceBindingType::UAV] = _uaHandles;
#ifdef SE_DEVELOPMENT
		_handlesSizes[(int32)SpirvShaderResourceBindingType::INVALID] = 0;
		_handlesSizes[(int32)SpirvShaderResourceBindingType::CB] = GPU_MAX_CB_BINDED;
		_handlesSizes[(int32)SpirvShaderResourceBindingType::SAMPLER] = GPU_MAX_SAMPLER_BINDED;
		_handlesSizes[(int32)SpirvShaderResourceBindingType::SRV] = GPU_MAX_SR_BINDED;
		_handlesSizes[(int32)SpirvShaderResourceBindingType::UAV] = GPU_MAX_UA_BINDED;
#endif
	}

	GPUContextVulkan::~GPUContextVulkan()
	{
		for (int32 i = 0; i < _descriptorPools.Count(); i++)
		{
			_descriptorPools[i].ClearDelete();
		}
		_descriptorPools.Clear();
		Delete(_cmdBufferManager);
	}



	void GPUContextVulkan::UpdateDescriptorSets(const SpirvShaderDescriptorInfo& descriptorInfo, DescriptorSetWriterVulkan& dsWriter, bool& needsWrite)
	{
		for (uint32 i = 0; i < descriptorInfo.descriptorTypesCount; i++)
		{
			const auto& descriptor = descriptorInfo.descriptorTypes[i];
			const int32 descriptorIndex = descriptor.binding;
			DescriptorResourceVulkan** handles = _handles[(int32)descriptor.bindingType];
			for (uint32 index = 0; index < descriptor.count; index++)
			{
				const uint32 slot = descriptor.slot + index;
				ENGINE_ASSERT(slot < _handlesSizes[(int32)descriptor.bindingType]);
				switch (descriptor.descriptorType)
				{
				// Sampler
				case VK_DESCRIPTOR_TYPE_SAMPLER:
				{
					const VkSampler handle = _samplerHandles[slot];
					ENGINE_ASSERT(handle);
					needsWrite |= dsWriter.WriteSampler(descriptorIndex, handle, index);
					break;
				}
				// Sampled Image
				case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
				{
					auto handle = (GPUTextureViewVulkan*)handles[slot];
					if (!handle)
					{
						const auto dummy = _device->dummyResources.GetDummyTexture(descriptor.resourceType);
						switch (descriptor.resourceType)
						{
						case SpirvShaderResourceType::Texture1D:
						case SpirvShaderResourceType::Texture2D:
							handle = static_cast<GPUTextureViewVulkan*>(dummy->View(0));
							break;
						case SpirvShaderResourceType::Texture3D:
							handle = static_cast<GPUTextureViewVulkan*>(dummy->ViewVolume());
							break;
						case SpirvShaderResourceType::TextureCube:
						case SpirvShaderResourceType::Texture1DArray:
						case SpirvShaderResourceType::Texture2DArray:
							handle = static_cast<GPUTextureViewVulkan*>(dummy->ViewArray());
							break;
						}
					}
					VkImageView imageView;
					VkImageLayout layout;
					handle->AsImage(this, imageView, layout);
					ENGINE_ASSERT(imageView != VK_NULL_HANDLE);
					needsWrite |= dsWriter.WriteImage(descriptorIndex, imageView, layout, index);
					break;
				}
				// Uniform Texel Buffer
				case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
				{
					auto handle = handles[slot];
					if (!handle)
					{
						const auto dummy = _device->dummyResources.GetDummyBuffer();
						handle = (DescriptorResourceVulkan*)dummy->View()->GetNativePtr();
					}
					VkBufferView bufferView;
					handle->AsUniformTexelBuffer(this, bufferView);
					ENGINE_ASSERT(bufferView != VK_NULL_HANDLE);
					needsWrite |= dsWriter.WriteUniformTexelBuffer(descriptorIndex, bufferView, index);
					break;
				}
				// Storage Image
				case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
				{
					auto handle = handles[slot];
					ENGINE_ASSERT(handle);
					VkImageView imageView;
					VkImageLayout layout;
					handle->AsStorageImage(this, imageView, layout);
					needsWrite |= dsWriter.WriteStorageImage(descriptorIndex, imageView, layout, index);
					break;
				}
				// Storage Image
				case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
				{
					auto handle = handles[slot];
					if (!handle)
					{
						const auto dummy = _device->dummyResources.GetDummyBuffer();
						handle = (DescriptorResourceVulkan*)dummy->View()->GetNativePtr();
					}
					VkBuffer buffer;
					VkDeviceSize offset, range;
					handle->AsStorageBuffer(this, buffer, offset, range);
					needsWrite |= dsWriter.WriteStorageBuffer(descriptorIndex, buffer, offset, range, index);
					break;
				}
				// Storage Texel Buffer
				case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
				{
					auto handle = handles[slot];
					if (!handle)
					{
						const auto dummy = _device->dummyResources.GetDummyBuffer();
						handle = (DescriptorResourceVulkan*)dummy->View()->GetNativePtr();
					}
					VkBufferView bufferView;
					handle->AsStorageTexelBuffer(this, bufferView);
					ENGINE_ASSERT(bufferView != VK_NULL_HANDLE);
					needsWrite |= dsWriter.WriteStorageTexelBuffer(descriptorIndex, bufferView, index);
					break;
				}
				// Uniform Buffer Dynamic
				case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
				{
					auto handle = handles[slot];
					VkBuffer buffer = VK_NULL_HANDLE;
					VkDeviceSize offset = 0, range = 0;
					uint32 dynamicOffset = 0;
					if (handle)
						handle->AsDynamicUniformBuffer(this, buffer, offset, range, dynamicOffset);
					else
					{
						const auto dummy = _device->dummyResources.GetDummyBuffer();
						buffer = dummy->GetHandle();
						range = dummy->GetSize();
					}
					needsWrite |= dsWriter.WriteDynamicUniformBuffer(descriptorIndex, buffer, offset, range, dynamicOffset, index);
					break;
				}
				default:
					// Unknown or invalid descriptor type
					ENGINE_UNREACHABLE_CODE();
					break;
				}
			}
		}
	}

	void GPUContextVulkan::UpdateDescriptorSets(ComputePipelineStateVulkan* pipelineState)
	{
		const auto cmdBuffer = _cmdBufferManager->GetCmdBuffer();
		const auto pipelineLayout = pipelineState->GetLayout();
		ENGINE_ASSERT(pipelineLayout);

		bool needsWrite = false;

		// No current descriptor pools set - acquire one and reset
		const bool newDescriptorPool = pipelineState->AcquirePoolSet(cmdBuffer);
		needsWrite |= newDescriptorPool;

		// Update descriptors
		UpdateDescriptorSets(*pipelineState->descriptorInfo, pipelineState->dsWriter, needsWrite);

		// Allocate sets if need to
		//if (needsWrite) // TODO: write on change only?f
		{
			if (!pipelineState->AllocateDescriptorSets())
			{
				return;
			}
			const VkDescriptorSet descriptorSet = pipelineState->descriptorSetHandles[DescriptorSet::Compute];
			pipelineState->dsWriter.SetDescriptorSet(descriptorSet);

			vkUpdateDescriptorSets(_device->device, pipelineState->dsWriteContainer.descriptorWrites.Count(), pipelineState->dsWriteContainer.descriptorWrites.Get(), 0, nullptr);
		}
	}

	void GPUContextVulkan::OnDrawCall()
	{
		GPUPipelineStateVulkan* pipelineState = _currentState;
		ENGINE_ASSERT(pipelineState && pipelineState->IsValid());
		const auto cmdBuffer = _cmdBufferManager->GetCmdBuffer();

		// End previous render pass if render targets layout was modified
		if (_rtDirtyFlag && cmdBuffer->IsInsideRenderPass())
			EndRenderPass();

		if (pipelineState->hasDescriptorsPerStageMask)
		{
			// Get descriptor pools set
			bool needsWrite = pipelineState->AcquirePoolSet(cmdBuffer);

			// Update descriptors for every used shader stage
			uint32 remainingHasDescriptorsPerStageMask = pipelineState->hasDescriptorsPerStageMask;
			for (int32 stage = 0; stage < DescriptorSet::GraphicsStagesCount && remainingHasDescriptorsPerStageMask; stage++)
			{
				if (remainingHasDescriptorsPerStageMask & 1)
				{
					UpdateDescriptorSets(*pipelineState->descriptorInfoPerStage[stage], pipelineState->dsWriter[stage], needsWrite);
				}
				remainingHasDescriptorsPerStageMask >>= 1;
			}

			// Allocate sets if need to
			//if (needsWrite) // TODO: write on change only?
			{
				if (!pipelineState->currentTypedDescriptorPoolSet->AllocateDescriptorSets(*pipelineState->descriptorSetsLayout, pipelineState->descriptorSetHandles.Get()))
					return;
				uint32 remainingStagesMask = pipelineState->hasDescriptorsPerStageMask;
				uint32 stage = 0;
				while (remainingStagesMask)
				{
					if (remainingStagesMask & 1)
						pipelineState->dsWriter[stage].SetDescriptorSet(pipelineState->descriptorSetHandles[stage]);
					remainingStagesMask >>= 1;
					stage++;
				}


				int descriptorWriteCount = pipelineState->dsWriteContainer.descriptorWrites.Count();
				VkWriteDescriptorSet* descriptorWrites = pipelineState->dsWriteContainer.descriptorWrites.Get();
				vkUpdateDescriptorSets(_device->device, descriptorWriteCount, descriptorWrites, 0, nullptr);
			}
		}

		// Bind any missing vertex buffers to null if required by the current state
		const auto vertexInputState = pipelineState->GetVertexInputState();
		const int32 missingVBs = vertexInputState->vertexBindingDescriptionCount - _vbCount;
		if (missingVBs > 0)
		{
			VkBuffer buffers[GPU_MAX_VB_BINDED];
			VkDeviceSize offsets[GPU_MAX_VB_BINDED] = {};
			for (int32 i = 0; i < missingVBs; i++)
				buffers[i] = _device->dummyResources.GetDummyVertexBuffer()->GetHandle(); 
			vkCmdBindVertexBuffers(cmdBuffer->GetHandle(), _vbCount, missingVBs, buffers, offsets);
		}

		// Start render pass if not during one
		if (cmdBuffer->IsOutsideRenderPass())
			BeginRenderPass();
		else if (_barriers.HasBarrier())
		{
			// TODO: implement better image/buffer barriers - and remove this renderPass split to flush barriers
			EndRenderPass();
			BeginRenderPass();
		}

		// Bind pipeline
		if (_psDirtyFlag && pipelineState && (_rtDepth || _rtCount))
		{
			_psDirtyFlag = false;
			const auto cmdBuffer = _cmdBufferManager->GetCmdBuffer();
			const auto pipeline = pipelineState->GetState(_renderPass);
			vkCmdBindPipeline(cmdBuffer->GetHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
//			RENDER_STAT_PS_STATE_CHANGE();
		}

		// Bind descriptors sets to the graphics pipeline
		if (pipelineState->hasDescriptorsPerStageMask)
		{
			vkCmdBindDescriptorSets(
				cmdBuffer->GetHandle(),
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				pipelineState->GetLayout()->handle,
				0,
				pipelineState->descriptorSetHandles.Count(),
				pipelineState->descriptorSetHandles.Get(),
				pipelineState->dynamicOffsets.Count(),
				pipelineState->dynamicOffsets.Get());
		}

		_rtDirtyFlag = false;
	}

	void GPUContextVulkan::AddImageBarrier(VkImage image, VkImageLayout srcLayout, VkImageLayout dstLayout, const VkImageSubresourceRange& subresourceRange, GPUTextureViewVulkan* handle)
	{
#if VK_ENABLE_BARRIERS_BATCHING
		// Auto-flush on overflow
		if (_barriers.IsFull())
		{
			const auto cmdBuffer = _cmdBufferManager->GetCmdBuffer();
			if (cmdBuffer->IsInsideRenderPass())
				EndRenderPass();
			_barriers.Execute(cmdBuffer);
		}
#endif

		// Insert barrier
#if VK_ENABLE_BARRIERS_DEBUG
		_barriers.ImageBarriersDebug.Add(handle);
#endif
		VkImageMemoryBarrier& imageBarrier = _barriers.ImageBarriers.AddOne();
		VulkanTool::ZeroStruct(imageBarrier, VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER);
		imageBarrier.image = image;
		imageBarrier.subresourceRange = subresourceRange;
		imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageBarrier.oldLayout = srcLayout;
		imageBarrier.newLayout = dstLayout;
		_barriers.SourceStage |= VulkanTool::GetImageBarrierFlags(srcLayout, imageBarrier.srcAccessMask);
		_barriers.DestStage |= VulkanTool::GetImageBarrierFlags(dstLayout, imageBarrier.dstAccessMask);
#if VK_ENABLE_BARRIERS_DEBUG
		LOG(Warning, "Image Barrier: 0x{0:x}, {1} -> {2} for baseMipLevel: {3}, baseArrayLayer: {4}, levelCount: {5}, layerCount: {6} ({7})",
        (uintptr)image,
        ::ToString(srcLayout),
        ::ToString(dstLayout),
        subresourceRange.baseMipLevel,
        subresourceRange.baseArrayLayer,
        subresourceRange.levelCount,
        subresourceRange.layerCount,
        handle && handle->Owner->AsGPUResource() ? handle->Owner->AsGPUResource()->ToString() : String::Empty
    );
#endif

#if !VK_ENABLE_BARRIERS_BATCHING
		// Auto-flush without batching
		const auto cmdBuffer = _cmdBufferManager->GetCmdBuffer();
		if (cmdBuffer->IsInsideRenderPass())
			EndRenderPass();
		_barriers.Execute(cmdBuffer);
#endif
	}

	void GPUContextVulkan::AddImageBarrier(GPUTextureViewVulkan* handle, VkImageLayout dstLayout)
	{
		auto& state = handle->owner->State;
		const auto subresourceIndex = handle->subresourceIndex;
		if (subresourceIndex == -1)
		{
			const int32 mipLevels = state.GetSubresourcesCount() / handle->owner->ArraySlices;
			if (state.AreAllSubresourcesSame())
			{
				const VkImageLayout srcLayout = state.GetSubresourceState(-1);
				if (srcLayout != dstLayout)
				{
					// Transition entire resource at once
					VkImageSubresourceRange range;
					range.aspectMask = handle->info.subresourceRange.aspectMask;
					range.baseMipLevel = 0;
					range.levelCount = mipLevels;
					range.baseArrayLayer = 0;
					range.layerCount = handle->owner->ArraySlices;
					AddImageBarrier(handle->image, srcLayout, dstLayout, range, handle);
					state.SetResourceState(dstLayout);
				}
			}
			else
			{
				// Slow path to transition each subresource
				for (int32 i = 0; i < state.GetSubresourcesCount(); i++)
				{
					const VkImageLayout srcLayout = state.GetSubresourceState(i);
					if (srcLayout != dstLayout)
					{
						VkImageSubresourceRange range;
						range.aspectMask = handle->info.subresourceRange.aspectMask;
						range.baseMipLevel = i % mipLevels;
						range.levelCount = 1;
						range.baseArrayLayer = i / mipLevels;
						range.layerCount = 1;
						AddImageBarrier(handle->image, srcLayout, dstLayout, range, handle);
						state.SetSubresourceState(i, dstLayout);
					}
				}
			}
			ENGINE_ASSERT(state.CheckResourceState(dstLayout));
			state.SetResourceState(dstLayout);
		}
		else
		{
			const VkImageLayout srcLayout = state.GetSubresourceState(subresourceIndex);
			if (srcLayout != dstLayout)
			{
				// Transition a single subresource
				AddImageBarrier(handle->image, srcLayout, dstLayout, handle->info.subresourceRange, handle);
				state.SetSubresourceState(subresourceIndex, dstLayout);
			}
		}
	}

	void GPUContextVulkan::AddImageBarrier(GPUTextureVulkan* texture, int32 mipSlice, int32 arraySlice, VkImageLayout dstLayout)
	{
		// Skip if no need to perform image layout transition
		const auto subresourceIndex = GPUUtils::CalcSubresourceIndex(mipSlice, arraySlice, texture->MipLevels());
		auto& state = texture->State;
		const auto srcLayout = state.GetSubresourceState(subresourceIndex);
		if (srcLayout == dstLayout)
			return;

		VkImageSubresourceRange range;
		range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		range.baseMipLevel = mipSlice;
		range.levelCount = 1;
		range.baseArrayLayer = arraySlice;
		range.layerCount = 1;
		AddImageBarrier(texture->GetHandle(), srcLayout, dstLayout, range, nullptr);
		state.SetSubresourceState(subresourceIndex, dstLayout);
	}

	void GPUContextVulkan::AddImageBarrier(GPUTextureVulkan* texture, VkImageLayout dstLayout)
	{
		// Check for fast path to transition the entire resource at once
		auto& state = texture->State;
		if (state.AreAllSubresourcesSame())
		{
			// Skip if no need to perform image layout transition
			const auto srcLayout = state.GetSubresourceState(0);
			if (srcLayout == dstLayout)
				return;

			VkImageSubresourceRange range;
			range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			range.baseMipLevel = 0;
			range.levelCount = texture->MipLevels();
			range.baseArrayLayer = 0;
			range.layerCount = texture->ArraySize();
			AddImageBarrier(texture->GetHandle(), srcLayout, dstLayout, range, nullptr);
			state.SetResourceState(dstLayout);
		}
		else
		{
			// Slow path to transition each subresource
			for (int32 arraySlice = 0; arraySlice < texture->ArraySize(); arraySlice++)
			{
				for (int32 mipSlice = 0; mipSlice < texture->MipLevels(); mipSlice++)
				{
					AddImageBarrier(texture, mipSlice, arraySlice, dstLayout);
				}
			}
		}
	}

	void GPUContextVulkan::AddBufferBarrier(GPUBufferVulkan* buffer, VkAccessFlags dstAccess)
	{
		// Skip if no need to perform buffer memory transition
		if ((buffer->Access & dstAccess) == dstAccess)
			return;

#if VK_ENABLE_BARRIERS_BATCHING
		// Auto-flush on overflow
    if (_barriers.IsFull())
    {
        const auto cmdBuffer = _cmdBufferManager->GetCmdBuffer();
        if (cmdBuffer->IsInsideRenderPass())
            EndRenderPass();
        _barriers.Execute(cmdBuffer);
    }
#endif

		// Insert barrier
		VkBufferMemoryBarrier& bufferBarrier = _barriers.BufferBarriers.AddOne();
		VulkanTool::ZeroStruct(bufferBarrier, VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER);
		bufferBarrier.buffer = buffer->GetHandle();
		bufferBarrier.offset = 0;
		bufferBarrier.size = buffer->GetSize();
		bufferBarrier.srcAccessMask = buffer->Access;
		bufferBarrier.dstAccessMask = dstAccess;
		bufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		bufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		_barriers.SourceStage |= VulkanTool::GetBufferBarrierFlags(buffer->Access);
		_barriers.DestStage |= VulkanTool::GetBufferBarrierFlags(dstAccess);
		buffer->Access = dstAccess;

#if !VK_ENABLE_BARRIERS_BATCHING
		// Auto-flush without batching
		const auto cmdBuffer = _cmdBufferManager->GetCmdBuffer();
		if (cmdBuffer->IsInsideRenderPass())
			EndRenderPass();
		_barriers.Execute(cmdBuffer);
#endif
	}

	void GPUContextVulkan::FlushBarriers()
	{
#if VK_ENABLE_BARRIERS_BATCHING
		if (_barriers.HasBarrier())
		{
			const auto cmdBuffer = _cmdBufferManager->GetCmdBuffer();
			if (cmdBuffer->IsInsideRenderPass())
				EndRenderPass();
			_barriers.Execute(cmdBuffer);
		}
#endif
	}

	DescriptorPoolVulkan* GPUContextVulkan::AllocateDescriptorSets(const VkDescriptorSetAllocateInfo& descriptorSetAllocateInfo, const DescriptorSetLayoutVulkan& layout, VkDescriptorSet* outSets)
	{
		VkResult result = VK_ERROR_OUT_OF_DEVICE_MEMORY;
		VkDescriptorSetAllocateInfo allocateInfo = descriptorSetAllocateInfo;
		DescriptorPoolVulkan* pool = nullptr;

		const uint32 hash = VULKAN_HASH_POOLS_WITH_LAYOUT_TYPES ? layout.setLayoutsHash : GetHash(layout);
		DescriptorPoolArray* typedDescriptorPools = _descriptorPools.TryGet(hash);

		if (typedDescriptorPools != nullptr)
		{
			pool = typedDescriptorPools->HasItems() ? typedDescriptorPools->Last() : nullptr;
			if (pool && pool->CanAllocate(layout))
			{
				allocateInfo.descriptorPool = pool->GetHandle();
				result = vkAllocateDescriptorSets(_device->device, &allocateInfo, outSets);
			}
		}
		else
		{
			typedDescriptorPools = &_descriptorPools.Add(hash, DescriptorPoolArray())->Value;
		}

		if (result < VK_SUCCESS)
		{
			if (pool && pool->IsEmpty())
			{
				LOG_VULKAN_RESULT(result);
			}
			else
			{
				pool = New<DescriptorPoolVulkan>(_device, layout);
				typedDescriptorPools->Add(pool);
				allocateInfo.descriptorPool = pool->GetHandle();
				VALIDATE_VULKAN_RESULT(vkAllocateDescriptorSets(_device->device, &allocateInfo, outSets));
			}
		}

		return pool;
	}

	void GPUContextVulkan::BeginRenderPass()
	{
		auto cmdBuffer = _cmdBufferManager->GetCmdBuffer();

		// Build render targets layout descriptor and framebuffer key
		FramebufferVulkan::Desc framebufferKey;
		framebufferKey.attachmentCount = _rtCount;
		RenderTargetLayoutVulkan layout;
		layout.RTsCount = _rtCount;
		layout.BlendEnable = _currentState && _currentState->blendEnable;
		layout.DepthFormat = _rtDepth ? _rtDepth->GetFormat() : PixelFormat::Undefined;
		for (int32 i = 0; i < GPU_MAX_RT_BINDED; i++)
		{
			auto handle = _rtHandles[i];
			if (handle)
			{
				layout.RTVsFormats[i] = handle->GetFormat();
				framebufferKey.attachments[i] = handle->GetFramebufferView();
				AddImageBarrier(handle, handle->layoutRTV);
			}
			else
			{
				layout.RTVsFormats[i] = PixelFormat::Undefined;
				framebufferKey.attachments[i] = VK_NULL_HANDLE;
			}
		}
		GPUTextureViewVulkan* handle;
		if (_rtDepth)
		{
			handle = _rtDepth;
			layout.ReadDepth = true;
			layout.ReadStencil = PixelFormatIsStencilSupport(handle->GetFormat());
			layout.WriteDepth = handle->layoutRTV == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL || handle->layoutRTV == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL || handle->layoutRTV == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
			layout.WriteStencil = handle->layoutRTV == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL || handle->layoutRTV == VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL || handle->layoutRTV == VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL;
			if (_currentState && 0)
			{
				// TODO: use this but only if state doesn't change during whole render pass (eg. 1st draw call might not draw depth but 2nd might)
				layout.ReadDepth &= _currentState->depthReadEnable;
				layout.ReadStencil &= _currentState->stencilReadEnable;
				layout.WriteDepth &= _currentState->depthWriteEnable;
				layout.WriteStencil &= _currentState->stencilWriteEnable;
			}
			framebufferKey.attachmentCount++;
			framebufferKey.attachments[_rtCount] = handle->GetFramebufferView();
			AddImageBarrier(handle, handle->layoutRTV);
		}
		else
		{
			handle = _rtHandles[0];
			layout.ReadDepth = false;
			layout.WriteDepth = false;
		}
		layout.MSAA = handle->GetMSAA();
		layout.Extent.width = handle->extent.width;
		layout.Extent.height = handle->extent.height;
		layout.Layers = handle->layers;

		// Get or create objects
		auto renderPass = _device->GetOrCreateRenderPass(layout);
		framebufferKey.renderPass = renderPass;
		auto framebuffer = _device->GetOrCreateFramebuffer(framebufferKey, layout.Extent, layout.Layers);
		_renderPass = renderPass;

		FlushBarriers();

		// TODO: use clear values for render pass begin to improve performance
		cmdBuffer->BeginRenderPass(renderPass, framebuffer, 0, nullptr);
	}

	void GPUContextVulkan::EndRenderPass()
	{
		auto cmdBuffer = _cmdBufferManager->GetCmdBuffer();
		cmdBuffer->EndRenderPass();
		_renderPass = nullptr;

		// Place a barrier between RenderPasses, so that color / depth outputs can be read in subsequent passes
		// TODO: remove it in future and use proper barriers without whole pipeline stalls
		vkCmdPipelineBarrier(cmdBuffer->GetHandle(), VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 0, nullptr);
	}



	void GPUContextVulkan::FrameBegin()
	{
		GPUContext::FrameBegin();

		// Setup
		_psDirtyFlag = 0;
		_rtDirtyFlag = 0;
		_cbDirtyFlag = 0;
		_rtCount = 0;
		_vbCount = 0;
		_stencilRef = 0;
		_renderPass = nullptr;
		_currentState = nullptr;
		_rtDepth = nullptr;
		Platform::MemoryClear(_rtHandles, sizeof(_rtHandles));
		Platform::MemoryClear(_cbHandles, sizeof(_cbHandles));
		Platform::MemoryClear(_srHandles, sizeof(_srHandles));
		Platform::MemoryClear(_uaHandles, sizeof(_uaHandles));
		Platform::MemoryCopy(_samplerHandles, _device->dummyResources.GetStaticSamplers(), sizeof(VkSampler) * GPU_STATIC_SAMPLERS_COUNT);
		Platform::MemoryClear(_samplerHandles + GPU_STATIC_SAMPLERS_COUNT, sizeof(_samplerHandles) - sizeof(VkSampler) * GPU_STATIC_SAMPLERS_COUNT);

#if VULKAN_RESET_QUERY_POOLS
		// Reset pending queries
		if (_device->QueriesToReset.HasItems())
		{
			const auto cmdBuffer = _cmdBufferManager->GetCmdBuffer();
			for (auto query : _device->QueriesToReset)
				query->Reset(cmdBuffer);
			_device->QueriesToReset.Clear();
		}
#endif
	}

	void GPUContextVulkan::FrameEnd()
	{
		const auto cmdBuffer = GetCmdBufferManager()->GetActiveCmdBuffer();
		if (cmdBuffer && cmdBuffer->IsInsideRenderPass())
		{
			EndRenderPass();
		}

		// Execute any queued layout transitions that weren't already handled by the render pass
		FlushBarriers();

		// Base
		GPUContext::FrameEnd();
	}

	void GPUContextVulkan::EventBegin(const Char* name)
	{
		const auto cmdBuffer = _cmdBufferManager->GetCmdBuffer();
		cmdBuffer->BeginEvent(name);
	}

	void GPUContextVulkan::EventEnd()
	{
		const auto cmdBuffer = _cmdBufferManager->GetCmdBuffer();
		cmdBuffer->EndEvent();
	}

	void* GPUContextVulkan::GetNativePtr() const
	{
		const auto cmdBuffer = _cmdBufferManager->GetCmdBuffer();
		return (void*)cmdBuffer->GetHandle();
	}

	bool GPUContextVulkan::IsDepthBufferBinded()
	{
		return _rtDepth != nullptr;
	}

	void GPUContextVulkan::ResetRenderTarget()
	{
		if (_rtDepth || _rtCount != 0)
		{
			_rtDirtyFlag = true;
			_psDirtyFlag = true;
			_rtCount = 0;
			_rtDepth = nullptr;
			Platform::MemoryClear(_rtHandles, sizeof(_rtHandles));

			const auto cmdBuffer = _cmdBufferManager->GetActiveCmdBuffer();
			if (cmdBuffer && cmdBuffer->IsInsideRenderPass())
			{
				EndRenderPass();
			}
		}
	}

	void GPUContextVulkan::SetRenderTarget(GPUTextureView* rt)
	{
		const auto rtVulkan = static_cast<GPUTextureViewVulkan*>(rt);

		if (_rtDepth != nullptr || _rtCount != 1 || _rtHandles[0] != rtVulkan)
		{
			_rtDirtyFlag = true;
			_psDirtyFlag = true;
			_rtCount = 1;
			_rtDepth = nullptr;
			_rtHandles[0] = rtVulkan;
		}
	}

	void GPUContextVulkan::SetRenderTarget(GPUTextureView* rt, GPUTextureView* depthBuffer)
	{
		const auto rtVulkan = static_cast<GPUTextureViewVulkan*>(rt);
		const auto depthBufferVulkan = static_cast<GPUTextureViewVulkan*>(depthBuffer);
		const auto rtCount = rtVulkan ? 1 : 0;

		if (_rtDepth != depthBufferVulkan || _rtCount != rtCount || _rtHandles[0] != rtVulkan)
		{
			_rtDirtyFlag = true;
			_psDirtyFlag = true;
			_rtCount = rtCount;
			_rtDepth = depthBufferVulkan;
			_rtHandles[0] = rtVulkan;
		}
	}

	void GPUContextVulkan::SetRenderTarget(const Span<GPUTextureView*>& rts, GPUTextureView* depthBuffer)
	{
		ENGINE_ASSERT(Math::RangeInclusive(rts.Length(), 1, GPU_MAX_RT_BINDED));

		const auto depthBufferVulkan = static_cast<GPUTextureViewVulkan*>(depthBuffer);

		GPUTextureViewVulkan* rtvs[GPU_MAX_RT_BINDED];
		for (int32 i = 0; i < rts.Length(); i++)
		{
			rtvs[i] = static_cast<GPUTextureViewVulkan*>(rts[i]);
		}
		const int32 rtvsSize = sizeof(GPUTextureViewVulkan*) * rts.Length();

		if (_rtDepth != depthBufferVulkan || _rtCount != rts.Length() || Platform::MemoryCompare(_rtHandles, rtvs, rtvsSize) != 0)
		{
			_rtDirtyFlag = true;
			_psDirtyFlag = true;
			_rtCount = rts.Length();
			_rtDepth = depthBufferVulkan;
			Platform::MemoryCopy(_rtHandles, rtvs, rtvsSize);
		}
	}

	void GPUContextVulkan::SetBlendFactor(const Float4& value)
	{
		const auto cmdBuffer = _cmdBufferManager->GetCmdBuffer();
		vkCmdSetBlendConstants(cmdBuffer->GetHandle(), value.Raw);
	}

	void GPUContextVulkan::SetStencilRef(uint32 value)
	{
		if (_stencilRef != value)
		{
			_stencilRef = value;
			const auto cmdBuffer = _cmdBufferManager->GetCmdBuffer();
			vkCmdSetStencilReference(cmdBuffer->GetHandle(), VK_STENCIL_FRONT_AND_BACK, _stencilRef);
		}
	}

	void GPUContextVulkan::SetViewport(const Viewport& viewport)
	{
		vkCmdSetViewport(_cmdBufferManager->GetCmdBuffer()->GetHandle(), 0, 1, (VkViewport*)&viewport);
	}

	void GPUContextVulkan::SetScissor(const Rectangle& scissorRect)
	{
		VkRect2D rect;
		rect.offset.x = (int32)scissorRect.Location.x;
		rect.offset.y = (int32)scissorRect.Location.y;
		rect.extent.width = (uint32)scissorRect.Size.x;
		rect.extent.height = (uint32)scissorRect.Size.y;
		vkCmdSetScissor(_cmdBufferManager->GetCmdBuffer()->GetHandle(), 0, 1, &rect);
	}

	void GPUContextVulkan::Dispatch(GPUShaderProgramCS* shader, uint32 threadGroupCountX, uint32 threadGroupCountY, uint32 threadGroupCountZ)
	{
		ENGINE_ASSERT(shader);
		const auto cmdBuffer = _cmdBufferManager->GetCmdBuffer();
		auto shaderVulkan = (GPUShaderProgramCSVulkan*)shader;

		// End any render pass
		if (cmdBuffer->IsInsideRenderPass())
			EndRenderPass();

		auto pipelineState = shaderVulkan->GetOrCreateState();
		UpdateDescriptorSets(pipelineState);
		FlushBarriers();

		// Bind pipeline
		vkCmdBindPipeline(cmdBuffer->GetHandle(), VK_PIPELINE_BIND_POINT_COMPUTE, pipelineState->GetHandle());
//		RENDER_STAT_PS_STATE_CHANGE();

		// Bind descriptors sets to the compute pipeline
		pipelineState->Bind(cmdBuffer);

		// Dispatch
		vkCmdDispatch(cmdBuffer->GetHandle(), threadGroupCountX, threadGroupCountY, threadGroupCountZ);
//		RENDER_STAT_DISPATCH_CALL();

		// Place a barrier between dispatches, so that UAVs can be read+write in subsequent passes
		// TODO: optimize it by moving inputs/outputs into higher-layer so eg. Global SDF can manually optimize it
		vkCmdPipelineBarrier(cmdBuffer->GetHandle(), VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 0, nullptr);
	}

	void GPUContextVulkan::DispatchIndirect(GPUShaderProgramCS* shader, GPUBuffer* bufferForArgs, uint32 offsetForArgs)
	{
		ENGINE_ASSERT(shader && bufferForArgs);
		const auto cmdBuffer = _cmdBufferManager->GetCmdBuffer();
		auto shaderVulkan = (GPUShaderProgramCSVulkan*)shader;
		const auto bufferForArgsVulkan = (GPUBufferVulkan*)bufferForArgs;

		// End any render pass
		if (cmdBuffer->IsInsideRenderPass())
			EndRenderPass();

		auto pipelineState = shaderVulkan->GetOrCreateState();
		UpdateDescriptorSets(pipelineState);
		AddBufferBarrier(bufferForArgsVulkan, VK_ACCESS_INDIRECT_COMMAND_READ_BIT);
		FlushBarriers();

		// Bind pipeline
		vkCmdBindPipeline(cmdBuffer->GetHandle(), VK_PIPELINE_BIND_POINT_COMPUTE, pipelineState->GetHandle());
//		RENDER_STAT_PS_STATE_CHANGE();

		// Bind descriptors sets to the compute pipeline
		pipelineState->Bind(cmdBuffer);

		// Dispatch
		vkCmdDispatchIndirect(cmdBuffer->GetHandle(), bufferForArgsVulkan->GetHandle(), offsetForArgs);
//		RENDER_STAT_DISPATCH_CALL();

		// Place a barrier between dispatches, so that UAVs can be read+write in subsequent passes
		// TODO: optimize it by moving inputs/outputs into higher-layer so eg. Global SDF can manually optimize it
		vkCmdPipelineBarrier(cmdBuffer->GetHandle(), VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, 0, nullptr,
			0, nullptr, 0, nullptr);
	}

	void GPUContextVulkan::ResolveMultisample(GPUTexture* sourceMultisampleTexture,
		GPUTexture* destTexture,
		int32 sourceSubResource,
		int32 destSubResource,
		PixelFormat format)
	{
		ENGINE_ASSERT(sourceMultisampleTexture && sourceMultisampleTexture->IsMultiSample());
		ENGINE_ASSERT(destTexture && !destTexture->IsMultiSample());

		// TODO: use render pass to resolve attachments

		const auto cmdBuffer = _cmdBufferManager->GetCmdBuffer();
		if (cmdBuffer->IsInsideRenderPass())
			EndRenderPass();

		const auto dstResourceVulkan = dynamic_cast<GPUTextureVulkan*>(destTexture);
		const auto srcResourceVulkan = dynamic_cast<GPUTextureVulkan*>(sourceMultisampleTexture);

		const int32 srcMipIndex = sourceSubResource % dstResourceVulkan->MipLevels();
		const int32 srcArrayIndex = sourceSubResource / dstResourceVulkan->MipLevels();
		const int32 dstMipIndex = destSubResource % dstResourceVulkan->MipLevels();
		const int32 dstArrayIndex = destSubResource / dstResourceVulkan->MipLevels();

		int32 width, height, depth;
		sourceMultisampleTexture->GetMipSize(srcMipIndex, width, height, depth);

		AddImageBarrier(dstResourceVulkan, dstMipIndex, dstArrayIndex, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		AddImageBarrier(srcResourceVulkan, srcMipIndex, srcArrayIndex, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
		FlushBarriers();

		VkImageResolve region;
		region.srcSubresource.aspectMask = srcResourceVulkan->DefaultAspectMask;
		region.srcSubresource.mipLevel = srcMipIndex;
		region.srcSubresource.baseArrayLayer = srcArrayIndex;
		region.srcSubresource.layerCount = 1;
		region.srcOffset = { 0, 0, 0 };
		region.dstSubresource.aspectMask = dstResourceVulkan->DefaultAspectMask;
		region.dstSubresource.mipLevel = dstMipIndex;
		region.dstSubresource.baseArrayLayer = dstArrayIndex;
		region.dstSubresource.layerCount = 1;
		region.dstOffset = { 0, 0, 0 };
		region.extent = { (uint32_t)width, (uint32_t)height, (uint32_t)depth };

		vkCmdResolveImage(cmdBuffer->GetHandle(), srcResourceVulkan->GetHandle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dstResourceVulkan->GetHandle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
	}

	void GPUContextVulkan::DrawInstanced(uint32 verticesCount,
		uint32 instanceCount,
		int32 startInstance,
		int32 startVertex)
	{
		OnDrawCall();
		const auto cmdBuffer = _cmdBufferManager->GetCmdBuffer();
		vkCmdDraw(cmdBuffer->GetHandle(), verticesCount, instanceCount, startVertex, startInstance);
//		RENDER_STAT_DRAW_CALL(verticesCount * instanceCount, verticesCount * instanceCount / 3);
	}

	void GPUContextVulkan::DrawIndexedInstanced(uint32 indicesCount,
		uint32 instanceCount,
		int32 startInstance,
		int32 startVertex,
		int32 startIndex)
	{
		OnDrawCall();
		const auto cmdBuffer = _cmdBufferManager->GetCmdBuffer();
		vkCmdDrawIndexed(cmdBuffer->GetHandle(), indicesCount, instanceCount, startIndex, startVertex, startInstance);
//		RENDER_STAT_DRAW_CALL(0, indicesCount / 3 * instanceCount);

	}
	void GPUContextVulkan::DrawInstancedIndirect(GPUBuffer* bufferForArgs, uint32 offsetForArgs)
	{
		ENGINE_ASSERT(bufferForArgs && bufferForArgs->GetFlags().IsFlag(GPUBufferFlags::Argument));
		OnDrawCall();
		auto bufferForArgsVK = (GPUBufferVulkan*)bufferForArgs;
		const auto cmdBuffer = _cmdBufferManager->GetCmdBuffer();
		vkCmdDrawIndirect(cmdBuffer->GetHandle(), bufferForArgsVK->GetHandle(), (VkDeviceSize)offsetForArgs, 1, sizeof(VkDrawIndirectCommand));
//		RENDER_STAT_DRAW_CALL(0, 0);
	}

	void GPUContextVulkan::DrawIndexedInstancedIndirect(GPUBuffer* bufferForArgs, uint32 offsetForArgs)
	{
		ENGINE_ASSERT(bufferForArgs && bufferForArgs->GetFlags().IsFlag(GPUBufferFlags::Argument));
		OnDrawCall();
		auto bufferForArgsVK = (GPUBufferVulkan*)bufferForArgs;
		const auto cmdBuffer = _cmdBufferManager->GetCmdBuffer();
		vkCmdDrawIndexedIndirect(cmdBuffer->GetHandle(), bufferForArgsVK->GetHandle(), (VkDeviceSize)offsetForArgs, 1, sizeof(VkDrawIndexedIndirectCommand));
//		RENDER_STAT_DRAW_CALL(0, 0);
	}

	void GPUContextVulkan::ResetSR()
	{
		Platform::MemoryClear(_srHandles, sizeof(_srHandles));
	}

	void GPUContextVulkan::ResetUA()
	{
		Platform::MemoryClear(_uaHandles, sizeof(_uaHandles));
	}

	void GPUContextVulkan::ResetCB()
	{
		_cbDirtyFlag = false;
		Platform::MemoryClear(_cbHandles, sizeof(_cbHandles));
	}
	void GPUContextVulkan::BindSR(int32 slot, GPUResourceView* view)
	{
		ENGINE_ASSERT(slot >= 0 && slot < GPU_MAX_SR_BINDED);
		const auto handle = view ? (DescriptorResourceVulkan*)view->GetNativePtr() : nullptr;
		if (_srHandles[slot] != handle)
		{
			_srHandles[slot] = handle;
			if (view)
				*view->LastRenderTime = _lastRenderTime;
		}
	}
	void GPUContextVulkan::BindUA(int32 slot, GPUResourceView* view)
	{
		ENGINE_ASSERT(slot >= 0 && slot < GPU_MAX_UA_BINDED);
		const auto handle = view ? (DescriptorResourceVulkan*)view->GetNativePtr() : nullptr;
		if (_uaHandles[slot] != handle)
		{
			_uaHandles[slot] = handle;
			if (view)
				*view->LastRenderTime = _lastRenderTime;
		}

	}
	void GPUContextVulkan::BindCB(int32 slot, GPUConstantBuffer* cb)
	{
		ENGINE_ASSERT(slot >= 0 && slot < GPU_MAX_CB_BINDED);

		const auto cbVulkan = static_cast<GPUConstantBufferVulkan*>(cb);

		if (_cbHandles[slot] != cbVulkan)
		{
			_cbDirtyFlag = true;
			_cbHandles[slot] = cbVulkan;
		}
	}

	void GPUContextVulkan::BindVB(const Span<GPUBuffer*>& vertexBuffers, const uint32* vertexBuffersOffsets)
	{
		_vbCount = vertexBuffers.Length();
		if (vertexBuffers.Length() == 0)
			return;

		ENGINE_ASSERT(_vbCount <= GPU_MAX_VB_BINDED);

		const auto cmdBuffer = _cmdBufferManager->GetCmdBuffer();
		VkBuffer buffers[GPU_MAX_VB_BINDED];
		VkDeviceSize offsets[GPU_MAX_VB_BINDED];

		Platform::MemoryClear(buffers, sizeof(VkBuffer) * GPU_MAX_VB_BINDED);
		Platform::MemoryClear(offsets, sizeof(VkDeviceSize) * GPU_MAX_VB_BINDED);

		for (int32 i = 0; i < _vbCount; i++)
		{
			auto vbVulkan = static_cast<GPUBufferVulkan*>(vertexBuffers[i]);
			if (!vbVulkan)
			{
				vbVulkan = _device->dummyResources.GetDummyVertexBuffer();
			}
			buffers[i] = vbVulkan->GetHandle();
			offsets[i] = vertexBuffersOffsets ? vertexBuffersOffsets[i] : 0;
		}
		vkCmdBindVertexBuffers(cmdBuffer->GetHandle(), 0, _vbCount, buffers, offsets);
	}

	void GPUContextVulkan::BindVB(GPUBuffer* vertexBuffers, const uint32 vertexBuffersOffsets)
	{
		_vbCount = 1;

		const auto cmdBuffer = _cmdBufferManager->GetCmdBuffer();
		VkBuffer buffers[1];
		VkDeviceSize offsets[1];

		auto vbVulkan = static_cast<GPUBufferVulkan*>(vertexBuffers);
		if (!vbVulkan)
		{
			vbVulkan = _device->dummyResources.GetDummyVertexBuffer();
		}
		buffers[0] = vbVulkan->GetHandle();
		offsets[0] = vertexBuffersOffsets;

		vkCmdBindVertexBuffers(cmdBuffer->GetHandle(), 0, 1, buffers, offsets);
	}

	void GPUContextVulkan::BindIB(GPUBuffer* indexBuffer)
	{
		const auto cmdBuffer = _cmdBufferManager->GetCmdBuffer();
		const auto ibVulkan = static_cast<GPUBufferVulkan*>(indexBuffer)->GetHandle();
		vkCmdBindIndexBuffer(cmdBuffer->GetHandle(), ibVulkan, 0, indexBuffer->GetFormat() == PixelFormat::R32_UInt ? VK_INDEX_TYPE_UINT32 : VK_INDEX_TYPE_UINT16);
	}

	void GPUContextVulkan::BindSampler(int32 slot, GPUSampler* sampler)
	{
		ENGINE_ASSERT(slot >= 0/*GPU_STATIC_SAMPLERS_COUNT*/ && slot < GPU_MAX_SAMPLER_BINDED);
		const auto handle = sampler ? ((GPUSamplerVulkan*)sampler)->Sampler : VK_NULL_HANDLE;
		_samplerHandles[slot] = handle;
	}

	void GPUContextVulkan::Clear(GPUTextureView* rt, const Color& color)
	{
		auto rtVulkan = static_cast<GPUTextureViewVulkan*>(rt);

		if (rtVulkan)
		{
			// TODO: detect if inside render pass and use ClearAttachments
			// TODO: delay clear for attachments before render pass to use render pass clear values for faster clearing

			const auto cmdBuffer = _cmdBufferManager->GetCmdBuffer();
			if (cmdBuffer->IsInsideRenderPass())
				EndRenderPass();

			AddImageBarrier(rtVulkan, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
			FlushBarriers();

			vkCmdClearColorImage(cmdBuffer->GetHandle(), rtVulkan->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				(const VkClearColorValue*)color.raw, 1, &rtVulkan->info.subresourceRange);
		}
	}

	void GPUContextVulkan::ClearDepth(GPUTextureView* depthBuffer, float depthValue)
	{
		const auto rtVulkan = static_cast<GPUTextureViewVulkan*>(depthBuffer);

		if (rtVulkan)
		{
			// TODO: detect if inside render pass and use ClearAttachments
			// TODO: delay clear for attachments before render pass to use render pass clear values for faster clearing

			const auto cmdBuffer = _cmdBufferManager->GetCmdBuffer();
			if (cmdBuffer->IsInsideRenderPass())
				EndRenderPass();

			AddImageBarrier(rtVulkan, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
			FlushBarriers();

			VkClearDepthStencilValue clear;
			clear.depth = depthValue;
			clear.stencil = 0;
			vkCmdClearDepthStencilImage(cmdBuffer->GetHandle(), rtVulkan->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear, 1, &rtVulkan->info.subresourceRange);
		}
	}

	void GPUContextVulkan::ClearUA(GPUBuffer* buf, const Float4& value)
	{
		const auto bufVulkan = static_cast<GPUBufferVulkan*>(buf);
		if (bufVulkan)
		{
			const auto cmdBuffer = _cmdBufferManager->GetCmdBuffer();
			if (cmdBuffer->IsInsideRenderPass())
				EndRenderPass();

			// TODO: add support for other components if buffer has them
			uint32_t* data = (uint32_t*)&value;
			vkCmdFillBuffer(cmdBuffer->GetHandle(), bufVulkan->GetHandle(), 0, bufVulkan->GetSize(), *data);
		}
	}

	void GPUContextVulkan::ClearUA(GPUBuffer* buf, const uint32 value[4])
	{
		const auto bufVulkan = static_cast<GPUBufferVulkan*>(buf);
		if (bufVulkan)
		{
			const auto cmdBuffer = _cmdBufferManager->GetCmdBuffer();
			if (cmdBuffer->IsInsideRenderPass())
				EndRenderPass();

			// TODO: add support for other components if buffer has them
			vkCmdFillBuffer(cmdBuffer->GetHandle(), bufVulkan->GetHandle(), 0, bufVulkan->GetSize(), value[0]);
		}
	}

	void GPUContextVulkan::ClearUA(GPUTexture* texture, const uint32 value[4])
	{
		const auto texVulkan = static_cast<GPUTextureVulkan*>(texture);
		if (texVulkan)
		{
			auto rtVulkan = static_cast<GPUTextureViewVulkan*>(texVulkan->View(0));
			const auto cmdBuffer = _cmdBufferManager->GetCmdBuffer();
			if (cmdBuffer->IsInsideRenderPass())
				EndRenderPass();

			AddImageBarrier(rtVulkan, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
			FlushBarriers();

			vkCmdClearColorImage(cmdBuffer->GetHandle(), rtVulkan->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, (const VkClearColorValue*)value, 1, &rtVulkan->info.subresourceRange);
		}
	}

	void GPUContextVulkan::ClearUA(GPUTexture* texture, const Float4& value)
	{
		const auto texVulkan = static_cast<GPUTextureVulkan*>(texture);
		if (texVulkan)
		{
			auto rtVulkan = ((GPUTextureViewVulkan*)(texVulkan->IsVolume() ? texVulkan->ViewVolume() : texVulkan->View(0)));
			const auto cmdBuffer = _cmdBufferManager->GetCmdBuffer();
			if (cmdBuffer->IsInsideRenderPass())
				EndRenderPass();

			AddImageBarrier(rtVulkan, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
			FlushBarriers();

			vkCmdClearColorImage(cmdBuffer->GetHandle(), rtVulkan->image,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, (const VkClearColorValue*)value.Raw, 1, &rtVulkan->info.subresourceRange);
		}
	}

	void GPUContextVulkan::SetState(GPUPipelineState* state)
	{
		if (_currentState != state)
		{
			_currentState = static_cast<GPUPipelineStateVulkan*>(state);
			_psDirtyFlag = true;
		}
	}

	GPUPipelineState* GPUContextVulkan::GetState() const
	{
		return _currentState;
	}

	void GPUContextVulkan::ClearState()
	{
		ResetRenderTarget();
		ResetSR();
		ResetUA();
		ResetCB();
		SetState(nullptr);

		FlushState();
	}

	void GPUContextVulkan::FlushState()
	{
		const auto cmdBuffer = _cmdBufferManager->GetCmdBuffer();
		if (cmdBuffer->IsInsideRenderPass())
		{
			EndRenderPass();
		}

		FlushBarriers();
	}

	void GPUContextVulkan::Flush()
	{
		FlushState();
		_currentState = nullptr;

		// Execute commands
		_cmdBufferManager->SubmitActiveCmdBuffer();
		_cmdBufferManager->PrepareForNewActiveCommandBuffer();
		ENGINE_ASSERT(_cmdBufferManager->HasPendingActiveCmdBuffer() && _cmdBufferManager->GetActiveCmdBuffer()->GetState() == CmdBufferVulkan::State::IsInsideBegin);
	}


	void GPUContextVulkan::UpdateBuffer(GPUBuffer* buffer, const void* data, uint32 size, uint32 offset)
	{
		ENGINE_ASSERT(data);
		ENGINE_ASSERT(buffer && buffer->GetSize() >= size);

		const auto cmdBuffer = _cmdBufferManager->GetCmdBuffer();
		if (cmdBuffer->IsInsideRenderPass())
			EndRenderPass();

		const auto bufferVulkan = static_cast<GPUBufferVulkan*>(buffer);

		// Memory transfer barrier
		// TODO: batch pipeline barriers
		const VkMemoryBarrier barrierBefore = { VK_STRUCTURE_TYPE_MEMORY_BARRIER, nullptr, VK_ACCESS_MEMORY_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT };
		vkCmdPipelineBarrier(cmdBuffer->GetHandle(), VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 1, &barrierBefore, 0, nullptr, 0, nullptr);

		// Use direct update for small buffers
		const uint32 alignedSize = Math::AlignUp<uint32>(size, 4);
		if (alignedSize > buffer->GetSize())
		{
			int a= 1;
		}
		if (size <= 16 * 1024 && alignedSize <= buffer->GetSize())
		{
/*			AddBufferBarrier(bufferVulkan, VK_ACCESS_TRANSFER_WRITE_BIT);
			FlushBarriers();*/

			vkCmdUpdateBuffer(cmdBuffer->GetHandle(), bufferVulkan->GetHandle(), offset, alignedSize, data);
		}
		else
		{
			auto staging = _device->stagingManager.AcquireBuffer(size, GPUResourceUsage::StagingUpload);
			staging->SetData(data, size);

			VkBufferCopy region;
			region.size = size;
			region.srcOffset = 0;
			region.dstOffset = offset;
			vkCmdCopyBuffer(cmdBuffer->GetHandle(), ((GPUBufferVulkan*)staging)->GetHandle(), ((GPUBufferVulkan*)buffer)->GetHandle(), 1, &region);

			_device->stagingManager.ReleaseBuffer(cmdBuffer, staging);
		}

		// Memory transfer barrier
		// TODO: batch pipeline barriers
		const VkMemoryBarrier barrierAfter = { VK_STRUCTURE_TYPE_MEMORY_BARRIER, nullptr,
											   VK_ACCESS_TRANSFER_WRITE_BIT,
											   VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT };
		vkCmdPipelineBarrier(cmdBuffer->GetHandle(),
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			0,
			1,
			&barrierAfter,
			0,
			nullptr,
			0,
			nullptr);
	}

	void GPUContextVulkan::CopyBuffer(GPUBuffer* dstBuffer, GPUBuffer* srcBuffer, uint32 size, uint32 dstOffset, uint32 srcOffset)
	{
		ENGINE_ASSERT(dstBuffer && srcBuffer);
		const auto cmdBuffer = _cmdBufferManager->GetCmdBuffer();

		// Ensure to end active render pass
		if (cmdBuffer->IsInsideRenderPass())
			EndRenderPass();

		auto dstBufferVulkan = static_cast<GPUBufferVulkan*>(dstBuffer);
		auto srcBufferVulkan = static_cast<GPUBufferVulkan*>(srcBuffer);

		// Transition resources
		AddBufferBarrier(dstBufferVulkan, VK_ACCESS_TRANSFER_WRITE_BIT);
		AddBufferBarrier(srcBufferVulkan, VK_ACCESS_TRANSFER_READ_BIT);
		FlushBarriers();

		VkBufferCopy bufferCopy;
		bufferCopy.srcOffset = srcOffset;
		bufferCopy.dstOffset = dstOffset;
		bufferCopy.size = size;
		vkCmdCopyBuffer(cmdBuffer->GetHandle(), srcBufferVulkan->GetHandle(), dstBufferVulkan->GetHandle(), 1, &bufferCopy);
	}

	void GPUContextVulkan::UpdateTexture(GPUTexture* texture, int32 arrayIndex, int32 mipIndex, const void* data, uint32 rowPitch, uint32 slicePitch)
	{
		ENGINE_ASSERT(texture && texture->IsAllocated() && data);

		const auto cmdBuffer = _cmdBufferManager->GetCmdBuffer();
		if (cmdBuffer->IsInsideRenderPass())
			EndRenderPass();

		const auto textureVulkan = DynamicCast<GPUTextureVulkan>(texture);

		AddImageBarrier(textureVulkan, mipIndex, arrayIndex, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		FlushBarriers();

		auto buffer = _device->stagingManager.AcquireBuffer(slicePitch, GPUResourceUsage::StagingUpload);
		buffer->SetData(data, slicePitch);

		// Setup buffer copy region
		int32 mipWidth, mipHeight, mipDepth;
		texture->GetMipSize(mipIndex, mipWidth, mipHeight, mipDepth);
		VkBufferImageCopy bufferCopyRegion;
		Platform::MemoryClear(&bufferCopyRegion, sizeof(bufferCopyRegion));
		bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		bufferCopyRegion.imageSubresource.mipLevel = mipIndex;
		bufferCopyRegion.imageSubresource.baseArrayLayer = arrayIndex;
		bufferCopyRegion.imageSubresource.layerCount = 1;
		bufferCopyRegion.imageExtent.width = static_cast<uint32_t>(mipWidth);
		bufferCopyRegion.imageExtent.height = static_cast<uint32_t>(mipHeight);
		bufferCopyRegion.imageExtent.depth = static_cast<uint32_t>(mipDepth);

		// Copy mip level from staging buffer
		vkCmdCopyBufferToImage(cmdBuffer->GetHandle(), ((GPUBufferVulkan*)buffer)->GetHandle(), textureVulkan->GetHandle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &bufferCopyRegion);

		_device->stagingManager.ReleaseBuffer(cmdBuffer, buffer);
	}

	void GPUContextVulkan::CopyTexture(GPUTexture* dstResource, uint32 dstSubresource, uint32 dstX, uint32 dstY, uint32 dstZ, GPUTexture* srcResource, uint32 srcSubresource)
	{
		ENGINE_ASSERT(dstResource && srcResource);
		const auto cmdBuffer = _cmdBufferManager->GetCmdBuffer();

		// Ensure to end active render pass
		if (cmdBuffer->IsInsideRenderPass())
			EndRenderPass();

		const auto dstTextureVulkan = static_cast<GPUTextureVulkan*>(dstResource);
		const auto srcTextureVulkan = static_cast<GPUTextureVulkan*>(srcResource);

		// Transition resources
		AddImageBarrier(dstTextureVulkan, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		AddImageBarrier(srcTextureVulkan, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
		FlushBarriers();

		// Prepare
		const int32 dstMipIndex = dstSubresource % dstTextureVulkan->MipLevels();
		const int32 dstArrayIndex = dstSubresource / dstTextureVulkan->MipLevels();
		const int32 srcMipIndex = srcSubresource % srcTextureVulkan->MipLevels();
		const int32 srcArrayIndex = srcSubresource / srcTextureVulkan->MipLevels();
		int32 mipWidth, mipHeight, mipDepth;
		srcTextureVulkan->GetMipSize(srcMipIndex, mipWidth, mipHeight, mipDepth);

		// Copy
		VkImageCopy region;
		Platform::MemoryClear(&region, sizeof(VkBufferImageCopy));
		region.extent.width = mipWidth;
		region.extent.height = mipHeight;
		region.extent.depth = mipDepth;
		region.dstOffset.x = dstX;
		region.dstOffset.y = dstY;
		region.dstOffset.z = dstZ;
		region.srcSubresource.baseArrayLayer = srcArrayIndex;
		region.srcSubresource.layerCount = 1;
		region.srcSubresource.mipLevel = srcMipIndex;
		region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.dstSubresource.baseArrayLayer = dstArrayIndex;
		region.dstSubresource.layerCount = 1;
		region.dstSubresource.mipLevel = dstMipIndex;
		region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		vkCmdCopyImage(cmdBuffer->GetHandle(), srcTextureVulkan->GetHandle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dstTextureVulkan->GetHandle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
	}

	void GPUContextVulkan::ResetCounter(GPUBuffer* buffer)
	{
		ENGINE_ASSERT(buffer);
		const auto cmdBuffer = _cmdBufferManager->GetCmdBuffer();

		const auto bufferVulkan = static_cast<GPUBufferVulkan*>(buffer);
		const auto counter = bufferVulkan->Counter;
		ENGINE_ASSERT(counter != nullptr);

		AddBufferBarrier(counter, VK_ACCESS_TRANSFER_WRITE_BIT);
		FlushBarriers();

		uint32 value = 0;
		vkCmdUpdateBuffer(cmdBuffer->GetHandle(), counter->GetHandle(), 0, 4, &value);
	}

	void GPUContextVulkan::CopyCounter(GPUBuffer* dstBuffer, uint32 dstOffset, GPUBuffer* srcBuffer)
	{
		ENGINE_ASSERT(dstBuffer && srcBuffer);
		const auto cmdBuffer = _cmdBufferManager->GetCmdBuffer();

		const auto dstBufferVulkan = static_cast<GPUBufferVulkan*>(dstBuffer);
		const auto srcBufferVulkan = static_cast<GPUBufferVulkan*>(srcBuffer);
		const auto counter = srcBufferVulkan->Counter;
		ENGINE_ASSERT(counter != nullptr);

		AddBufferBarrier(dstBufferVulkan, VK_ACCESS_TRANSFER_WRITE_BIT);
		AddBufferBarrier(counter, VK_ACCESS_TRANSFER_READ_BIT);
		FlushBarriers();

		VkBufferCopy bufferCopy;
		bufferCopy.srcOffset = 0;
		bufferCopy.dstOffset = dstOffset;
		bufferCopy.size = 4;
		vkCmdCopyBuffer(cmdBuffer->GetHandle(), srcBufferVulkan->GetHandle(), dstBufferVulkan->GetHandle(), 1, &bufferCopy);
	}

	void GPUContextVulkan::CopyResource(GPUResource* dstResource, GPUResource* srcResource)
	{
		ENGINE_ASSERT(dstResource && srcResource);
		const auto cmdBuffer = _cmdBufferManager->GetCmdBuffer();

		// Ensure to end active render pass
		if (cmdBuffer->IsInsideRenderPass())
			EndRenderPass();

		auto dstTextureVulkan = dynamic_cast<GPUTextureVulkan*>(dstResource);
		auto srcTextureVulkan = dynamic_cast<GPUTextureVulkan*>(srcResource);
		auto dstBufferVulkan = dynamic_cast<GPUBufferVulkan*>(dstResource);
		auto srcBufferVulkan = dynamic_cast<GPUBufferVulkan*>(srcResource);

		// Buffer -> Buffer
		if (srcBufferVulkan && dstBufferVulkan)
		{
			// Transition resources
			AddBufferBarrier(dstBufferVulkan, VK_ACCESS_TRANSFER_WRITE_BIT);
			AddBufferBarrier(srcBufferVulkan, VK_ACCESS_TRANSFER_READ_BIT);
			FlushBarriers();

			// Copy
			VkBufferCopy bufferCopy;
			bufferCopy.srcOffset = 0;
			bufferCopy.dstOffset = 0;
			bufferCopy.size = srcBufferVulkan->GetSize();
			ENGINE_ASSERT(bufferCopy.size == dstBufferVulkan->GetSize());
			vkCmdCopyBuffer(cmdBuffer->GetHandle(), srcBufferVulkan->GetHandle(), dstBufferVulkan->GetHandle(), 1, &bufferCopy);
		}
		// Texture -> Texture
		else if (srcTextureVulkan && dstTextureVulkan)
		{
			if (dstTextureVulkan->IsStaging())
			{
				// Staging Texture -> Staging Texture
				if (srcTextureVulkan->IsStaging())
				{
					ENGINE_ASSERT(dstTextureVulkan->StagingBuffer && srcTextureVulkan->StagingBuffer);
					CopyResource(dstTextureVulkan->StagingBuffer, srcTextureVulkan->StagingBuffer);
				}
					// Texture -> Staging Texture
				else
				{
					// Transition resources
					ENGINE_ASSERT(dstTextureVulkan->StagingBuffer);
					AddBufferBarrier(dstTextureVulkan->StagingBuffer, VK_ACCESS_TRANSFER_WRITE_BIT);
					AddImageBarrier(srcTextureVulkan, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
					FlushBarriers();

					// Copy
					int32 copyOffset = 0;
					const int32 arraySize = srcTextureVulkan->ArraySize();
					const int32 mipMaps = srcTextureVulkan->MipLevels();
					for (int32 arraySlice = 0; arraySlice < arraySize; arraySlice++)
					{
						VkBufferImageCopy regions[GPU_MAX_TEXTURE_MIP_LEVELS];
						uint32_t mipWidth = srcTextureVulkan->Width();
						uint32_t mipHeight = srcTextureVulkan->Height();
						uint32_t mipDepth = srcTextureVulkan->Depth();

						for (int32 mipLevel = 0; mipLevel < mipMaps; mipLevel++)
						{
							VkBufferImageCopy& region = regions[mipLevel];
							region.bufferOffset = copyOffset;
							region.bufferRowLength = mipWidth;
							region.bufferImageHeight = mipHeight;
							region.imageOffset = { 0, 0, 0 };
							region.imageExtent = { mipWidth, mipHeight, mipDepth };
							region.imageSubresource.baseArrayLayer = arraySlice;
							region.imageSubresource.layerCount = 1;
							region.imageSubresource.mipLevel = mipLevel;
							region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

							// TODO: pitch/slice alignment on Vulkan?
							copyOffset += dstTextureVulkan->ComputeSubresourceSize(mipLevel, 1, 1);

							if (mipWidth != 1)
								mipWidth >>= 1;
							if (mipHeight != 1)
								mipHeight >>= 1;
							if (mipDepth != 1)
								mipDepth >>= 1;
						}

						vkCmdCopyImageToBuffer(cmdBuffer->GetHandle(),
							srcTextureVulkan->GetHandle(),
							VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
							dstTextureVulkan->StagingBuffer->GetHandle(),
							mipMaps,
							regions);
					}
				}
			}
			else
			{
				// Transition resources
				AddImageBarrier(dstTextureVulkan, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
				AddImageBarrier(srcTextureVulkan, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
				FlushBarriers();

				// Copy
				const int32 arraySize = srcTextureVulkan->ArraySize();
				const int32 mipMaps = srcTextureVulkan->MipLevels();
				ENGINE_ASSERT(dstTextureVulkan->MipLevels() == mipMaps);
				VkImageCopy regions[GPU_MAX_TEXTURE_MIP_LEVELS];
				uint32_t mipWidth = srcTextureVulkan->Width();
				uint32_t mipHeight = srcTextureVulkan->Height();
				uint32_t mipDepth = srcTextureVulkan->Depth();
				for (int32 mipLevel = 0; mipLevel < mipMaps; mipLevel++)
				{
					VkImageCopy& region = regions[mipLevel];
					region.extent = { mipWidth, mipHeight, mipDepth };
					region.srcOffset = { 0, 0, 0 };
					region.srcSubresource.baseArrayLayer = 0;
					region.srcSubresource.layerCount = arraySize;
					region.srcSubresource.mipLevel = mipLevel;
					region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					region.dstOffset = { 0, 0, 0 };
					region.dstSubresource.baseArrayLayer = 0;
					region.dstSubresource.layerCount = arraySize;
					region.dstSubresource.mipLevel = mipLevel;
					region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

					if (mipWidth != 1)
						mipWidth >>= 1;
					if (mipHeight != 1)
						mipHeight >>= 1;
					if (mipDepth != 1)
						mipDepth >>= 1;
				}
				vkCmdCopyImage(cmdBuffer->GetHandle(),
					srcTextureVulkan->GetHandle(),
					VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					dstTextureVulkan->GetHandle(),
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					mipMaps,
					regions);
			}
		}
		else
		{
			LOG_WARNING("Graphic", "GPUContext Cannot copy data between buffer and texture.");
		}
	}

	void GPUContextVulkan::CopySubresource(GPUResource* dstResource, uint32 dstSubresource, GPUResource* srcResource, uint32 srcSubresource)
	{
		ENGINE_ASSERT(dstResource && srcResource);
		const auto cmdBuffer = _cmdBufferManager->GetCmdBuffer();

		// Ensure to end active render pass
		if (cmdBuffer->IsInsideRenderPass())
			EndRenderPass();

		auto dstTextureVulkan = dynamic_cast<GPUTextureVulkan*>(dstResource);
		auto srcTextureVulkan = dynamic_cast<GPUTextureVulkan*>(srcResource);
		auto dstBufferVulkan = dynamic_cast<GPUBufferVulkan*>(dstResource);
		auto srcBufferVulkan = dynamic_cast<GPUBufferVulkan*>(srcResource);

		// Buffer -> Buffer
		if (srcBufferVulkan && dstBufferVulkan)
		{
			ENGINE_ASSERT(dstSubresource == 0 && srcSubresource == 0);

			// Transition resources
			AddBufferBarrier(dstBufferVulkan, VK_ACCESS_TRANSFER_WRITE_BIT);
			AddBufferBarrier(srcBufferVulkan, VK_ACCESS_TRANSFER_READ_BIT);
			FlushBarriers();

			// Copy
			VkBufferCopy bufferCopy;
			bufferCopy.srcOffset = 0;
			bufferCopy.dstOffset = 0;
			bufferCopy.size = srcBufferVulkan->GetSize();
			ENGINE_ASSERT(bufferCopy.size == dstBufferVulkan->GetSize());
			vkCmdCopyBuffer(cmdBuffer->GetHandle(), srcBufferVulkan->GetHandle(), dstBufferVulkan->GetHandle(), 1, &bufferCopy);
		}
			// Texture -> Texture
		else if (srcTextureVulkan && dstTextureVulkan)
		{
			const int32 dstMipMaps = dstTextureVulkan->MipLevels();
			const int32 dstMipIndex = dstSubresource % dstMipMaps;
			const int32 dstArrayIndex = dstSubresource / dstMipMaps;
			const int32 srcMipMaps = srcTextureVulkan->MipLevels();
			const int32 srcMipIndex = srcSubresource % srcMipMaps;
			const int32 srcArrayIndex = srcSubresource / srcMipMaps;

			if (dstTextureVulkan->IsStaging())
			{
				// Staging Texture -> Staging Texture
				if (srcTextureVulkan->IsStaging())
				{
					ENGINE_ASSERT(dstTextureVulkan->StagingBuffer && srcTextureVulkan->StagingBuffer);
					CopyResource(dstTextureVulkan->StagingBuffer, srcTextureVulkan->StagingBuffer);
				}
					// Texture -> Staging Texture
				else
				{
					// Transition resources
					ENGINE_ASSERT(dstTextureVulkan->StagingBuffer);
					AddBufferBarrier(dstTextureVulkan->StagingBuffer, VK_ACCESS_TRANSFER_WRITE_BIT);
					AddImageBarrier(srcTextureVulkan, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
					FlushBarriers();

					// Copy
					int32 copyOffset = 0;
					uint32 subResourceCount = 0;
					for (int32 arraySlice = 0; arraySlice < dstTextureVulkan->ArraySize() && subResourceCount < dstSubresource; arraySlice++)
					{
						for (int32 mipLevel = 0; mipLevel < dstMipMaps && subResourceCount < dstSubresource; mipLevel++)
						{
							// TODO: pitch/slice alignment on Vulkan?
							copyOffset += dstTextureVulkan->ComputeSubresourceSize(mipLevel, 1, 1);
							subResourceCount++;
						}
					}
					VkBufferImageCopy region;
					region.bufferOffset = copyOffset;
					region.bufferRowLength = Math::Max<uint32_t>(dstTextureVulkan->Width() >> dstMipIndex, 1);
					region.bufferImageHeight = Math::Max<uint32_t>(dstTextureVulkan->Height() >> dstMipIndex, 1);
					region.imageOffset = { 0, 0, 0 };
					region.imageExtent = { Math::Max<uint32_t>(srcTextureVulkan->Width() >> srcMipIndex, 1), Math::Max<uint32_t>(srcTextureVulkan->Height() >> srcMipIndex, 1), Math::Max<uint32_t>(srcTextureVulkan->Depth() >> srcMipIndex, 1) };
					region.imageSubresource.baseArrayLayer = srcArrayIndex;
					region.imageSubresource.layerCount = 1;
					region.imageSubresource.mipLevel = srcMipIndex;
					region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					vkCmdCopyImageToBuffer(cmdBuffer->GetHandle(), srcTextureVulkan->GetHandle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dstTextureVulkan->StagingBuffer->GetHandle(), 1, &region);
				}
			}
			else
			{
				// Transition resources
				AddImageBarrier(dstTextureVulkan, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
				AddImageBarrier(srcTextureVulkan, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
				FlushBarriers();

				// Copy
				int32 mipWidth, mipHeight, mipDepth;
				srcTextureVulkan->GetMipSize(srcMipIndex, mipWidth, mipHeight, mipDepth);
				VkImageCopy region;;
				region.extent = { Math::Max<uint32_t>(mipWidth, 1), Math::Max<uint32_t>(mipWidth, 1), Math::Max<uint32_t>(mipDepth, 1) };
				region.srcOffset = { 0, 0, 0 };
				region.srcSubresource.baseArrayLayer = srcArrayIndex;
				region.srcSubresource.layerCount = 1;
				region.srcSubresource.mipLevel = srcMipIndex;
				region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				region.dstOffset = { 0, 0, 0 };
				region.dstSubresource.baseArrayLayer = dstArrayIndex;
				region.dstSubresource.layerCount = 1;
				region.dstSubresource.mipLevel = dstMipIndex;
				region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				vkCmdCopyImage(cmdBuffer->GetHandle(), srcTextureVulkan->GetHandle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dstTextureVulkan->GetHandle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
			}
		}
		else
		{
			LOG_WARNING("Graphic", "GPUContext Cannot copy data between buffer and texture.");
		}
	}

	void GPUContextVulkan::UpdateCB(GPUConstantBuffer* cb, const void* data)
	{
		ENGINE_ASSERT(data && cb);
		auto cbVulkan = static_cast<GPUConstantBufferVulkan*>(cb);
		const uint32 size = cbVulkan->GetSize();
		if (size == 0)
			return;
		const auto cmdBuffer = _cmdBufferManager->GetCmdBuffer();

		// Allocate bytes for the buffer
		const auto allocation = _device->uniformBufferUploader->Allocate(size, 0, this);

		// Copy data
		Platform::MemoryCopy(allocation.CPUAddress, data, allocation.Size);

		// Cache the allocation to update the descriptor
		cbVulkan->Allocation = allocation;

		// Mark CB slot as dirty if this CB is binded to the pipeline
		for (int32 i = 0; i < ARRAY_SIZE(_cbHandles); i++)
		{
			if (_cbHandles[i] == cbVulkan)
			{
				_cbDirtyFlag = true;
				break;
			}
		}
	}

} // SE