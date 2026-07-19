
#include "CmdBufferVulkan.h"
#include "VulkanTool.h"
#include "QueueVulkan.h"
#include "GPUDeviceVulkan.h"
#include "GPUContextVulkan.h"
#include "Runtime/Engine.h"

#include "Runtime/Core/Profiler/Profiler.h"

namespace SE
{
	void CmdBufferVulkan::AddWaitSemaphore(VkPipelineStageFlags waitFlags, SemaphoreVulkan* waitSemaphore)
	{
		m_WaitFlags.Add(waitFlags);
		ENGINE_ASSERT(!m_WaitSemaphores.Contains(waitSemaphore));
		m_WaitSemaphores.Add(waitSemaphore);
	}

	void CmdBufferVulkan::Begin()
	{
		PROFILE_CPU();
		ENGINE_ASSERT(m_State == State::ReadyForBegin);

		VkCommandBufferBeginInfo beginInfo;
		VulkanTool::ZeroStruct(beginInfo, VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO);
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		VALIDATE_VULKAN_RESULT(vkBeginCommandBuffer(m_CommandBuffer, &beginInfo));

		// Acquire a descriptor pool set on
		if (m_DescriptorPoolSetContainer == nullptr)
		{
			m_DescriptorPoolSetContainer = m_Device->descriptorPoolsManager->AcquirePoolSetContainer();
		}

		m_State = State::IsInsideBegin;

#if GPU_ALLOW_PROFILE_EVENTS
		// Reset events counter
		m_EventsBegin = 0;
#endif
	}

	void CmdBufferVulkan::End()
	{
		PROFILE_CPU();
		ENGINE_ASSERT(IsOutsideRenderPass());

#if GPU_ALLOW_PROFILE_EVENTS && VK_EXT_debug_utils
		// End remaining events
		while (m_EventsBegin--)
			vkCmdEndDebugUtilsLabelEXT(GetHandle());
#endif

		VALIDATE_VULKAN_RESULT(vkEndCommandBuffer(GetHandle()));
		m_State = State::HasEnded;
	}

	void CmdBufferVulkan::BeginRenderPass(RenderPassVulkan* renderPass, FramebufferVulkan* framebuffer, uint32 clearValueCount, VkClearValue* clearValues)
	{
		ENGINE_ASSERT(IsOutsideRenderPass());
		VkRenderPassBeginInfo info;
		VulkanTool::ZeroStruct(info, VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO);
		info.renderPass = renderPass->handle;
		info.framebuffer = framebuffer->handle;
		info.renderArea.offset.x = 0;
		info.renderArea.offset.y = 0;
		info.renderArea.extent.width = framebuffer->extent.width;
		info.renderArea.extent.height = framebuffer->extent.height;
		info.clearValueCount = clearValueCount;
		info.pClearValues = clearValues;
		vkCmdBeginRenderPass(m_CommandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);
		m_State = State::IsInsideRenderPass;
	}

	void CmdBufferVulkan::EndRenderPass()
	{
		ENGINE_ASSERT(IsInsideRenderPass());
		vkCmdEndRenderPass(m_CommandBuffer);
		m_State = State::IsInsideBegin;
	}

	void CmdBufferVulkan::BeginEvent(const Char * name)
	{
#if VK_EXT_debug_utils
		if (!vkCmdBeginDebugUtilsLabelEXT)
			return;

		m_EventsBegin++;

		// Convert to ANSI
		char buffer[101];
		int32 i = 0;
		while (i < 100 && name[i])
		{
			buffer[i] = (char)name[i];
			i++;
		}
		buffer[i] = 0;

		VkDebugUtilsLabelEXT label;
		VulkanTool::ZeroStruct(label, VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT);
		label.pLabelName = buffer;
		vkCmdBeginDebugUtilsLabelEXT(GetHandle(), &label);
#endif
	}

	void CmdBufferVulkan::EndEvent()
	{
#if VK_EXT_debug_utils
		if (m_EventsBegin == 0 || !vkCmdEndDebugUtilsLabelEXT)
			return;
		m_EventsBegin--;

		vkCmdEndDebugUtilsLabelEXT(GetHandle());
#endif
	}


	CmdBufferVulkan::CmdBufferVulkan(GPUDeviceVulkan* device, CmdBufferPoolVulkan* pool)
		: m_Device(device)
		, m_CommandBuffer(VK_NULL_HANDLE)
		, m_State(State::ReadyForBegin)
		, m_Fence(nullptr)
		, m_SubmittedFenceCounter(0)
		, m_FenceSignaledCounter(0)
		, m_CommandBufferPool(pool)
	{
		VkCommandBufferAllocateInfo createCmdBufInfo;
		VulkanTool::ZeroStruct(createCmdBufInfo, VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO);
		createCmdBufInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		createCmdBufInfo.commandBufferCount = 1;
		createCmdBufInfo.commandPool = m_CommandBufferPool->GetHandle();
		VALIDATE_VULKAN_RESULT(vkAllocateCommandBuffers(m_Device->device, &createCmdBufInfo, &m_CommandBuffer));
		m_Fence = m_Device->fenceManager.AllocateFence();
	}

	CmdBufferVulkan::~CmdBufferVulkan()
	{
		FenceManagerVulkan& fenceManager = m_Device->fenceManager;
		if (m_State == State::Submitted)
		{
			// Wait 60ms
			const uint64 waitForCmdBufferInNanoSeconds = 60 * 1000 * 1000LL;
			fenceManager.WaitAndReleaseFence(m_Fence, waitForCmdBufferInNanoSeconds);
		}
		else
		{
			// Just free the fence, command buffer was not submitted
			fenceManager.ReleaseFence(m_Fence);
		}

		vkFreeCommandBuffers(m_Device->device, m_CommandBufferPool->GetHandle(), 1, &m_CommandBuffer);
	}

	void CmdBufferVulkan::RefreshFenceStatus()
	{
		if (m_State == State::Submitted)
		{
			PROFILE_CPU();
			if (m_Device->fenceManager.IsFenceSignaled(m_Fence))
			{
				m_State = State::ReadyForBegin;
				m_SubmittedWaitSemaphores.Clear();

				vkResetCommandBuffer(m_CommandBuffer, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
				m_Device->fenceManager.ResetFence(m_Fence);
				m_FenceSignaledCounter++;

				if (m_DescriptorPoolSetContainer)
				{
					m_DescriptorPoolSetContainer->lastFrameUsed = Engine::FrameCount;
					m_DescriptorPoolSetContainer = nullptr;
				}
			}
		}
		else
		{
			ENGINE_ASSERT(!m_Fence->IsSignaled);
		}
	}

	CmdBufferVulkan* CmdBufferPoolVulkan::Create()
	{
		const auto cmdBuffer = New<CmdBufferVulkan>(_device, this);
		_cmdBuffers.Add(cmdBuffer);
		return cmdBuffer;
	}

	void CmdBufferPoolVulkan::RefreshFenceStatus(const CmdBufferVulkan* skipCmdBuffer)
	{
		for (int32 i = 0; i < _cmdBuffers.Count(); i++)
		{
			auto cmdBuffer = _cmdBuffers[i];
			if (cmdBuffer != skipCmdBuffer)
			{
				cmdBuffer->RefreshFenceStatus();
			}
		}
	}

	void CmdBufferPoolVulkan::Create(uint32 queueFamilyIndex)
	{
		VkCommandPoolCreateInfo poolInfo;
		VulkanTool::ZeroStruct(poolInfo, VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO);
		poolInfo.queueFamilyIndex = queueFamilyIndex;
		// TODO: use VK_COMMAND_POOL_CREATE_TRANSIENT_BIT?
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		VALIDATE_VULKAN_RESULT(vkCreateCommandPool(_device->device, &poolInfo, nullptr, &_handle));
	}

	CmdBufferPoolVulkan::CmdBufferPoolVulkan(GPUDeviceVulkan* device)
		: _device(device)
		, _handle(VK_NULL_HANDLE)
	{
	}

	CmdBufferPoolVulkan::~CmdBufferPoolVulkan()
	{
		for (int32 i = 0; i < _cmdBuffers.Count(); i++)
			Delete(_cmdBuffers[i]);
		vkDestroyCommandPool(_device->device, _handle, nullptr);
	}




	CmdBufferManagerVulkan::CmdBufferManagerVulkan(GPUDeviceVulkan* device, GPUContextVulkan* context)
		: _device(device)
		, _pool(device)
		, _queue(context->GetQueue())
		, _activeCmdBuffer(nullptr)
	{
		_pool.Create(_queue->GetFamilyIndex());
	}

	void CmdBufferManagerVulkan::SubmitActiveCmdBuffer(SemaphoreVulkan* signalSemaphore)
	{
		PROFILE_CPU();
		ENGINE_ASSERT(_activeCmdBuffer);
		if (!_activeCmdBuffer->IsSubmitted() && _activeCmdBuffer->HasBegun())
		{
			if (_activeCmdBuffer->IsInsideRenderPass())
				_activeCmdBuffer->EndRenderPass();

#if VULKAN_USE_QUERIES
			// Pause all active queries
        for (int32 i = 0; i < _queriesInProgress.Count(); i++)
        {
            _queriesInProgress.Get()[i]->Interrupt(_activeCmdBuffer);
        }
#endif

			_activeCmdBuffer->End();

			if (signalSemaphore)
			{
				_queue->Submit(_activeCmdBuffer, signalSemaphore->GetHandle());
			}
			else
			{
				_queue->Submit(_activeCmdBuffer);
			}
		}
		_activeCmdBuffer = nullptr;
	}

	void CmdBufferManagerVulkan::WaitForCmdBuffer(CmdBufferVulkan* cmdBuffer, float timeInSecondsToWait)
	{
		PROFILE_CPU();
		ASSERT(cmdBuffer->IsSubmitted());
		const bool failed = _device->fenceManager.WaitForFence(cmdBuffer->GetFence(), (uint64)(timeInSecondsToWait * 1e9));
		ASSERT(!failed);
		cmdBuffer->RefreshFenceStatus();
	}

	void CmdBufferManagerVulkan::PrepareForNewActiveCommandBuffer()
	{
		PROFILE_CPU();
		ENGINE_ASSERT(_activeCmdBuffer == nullptr);
		for (int32 i = 0; i < _pool._cmdBuffers.Count(); i++)
		{
			auto cmdBuffer = _pool._cmdBuffers.Get()[i];
			cmdBuffer->RefreshFenceStatus();
			if (cmdBuffer->GetState() == CmdBufferVulkan::State::ReadyForBegin)
			{
				_activeCmdBuffer = cmdBuffer;
				break;
			}
			else
			{
				ENGINE_ASSERT(cmdBuffer->GetState() == CmdBufferVulkan::State::Submitted);
			}
		}

		if (_activeCmdBuffer == nullptr)
		{
			// Always begin fresh command buffer for rendering
			_activeCmdBuffer = _pool.Create();
		}

		_activeCmdBuffer->Begin();

#if VULKAN_USE_QUERIES
		// Resume any paused queries with the new command buffer
		for (int32 i = 0; i < _queriesInProgress.Count(); i++)
		{
			_queriesInProgress.Get()[i]->Resume(_activeCmdBuffer);
		}
#endif
	}

	void CmdBufferManagerVulkan::OnQueryBegin(GPUTimerQueryVulkan* query)
	{
#if VULKAN_USE_QUERIES
		_queriesInProgress.Add(query);
#endif
	}

	void CmdBufferManagerVulkan::OnQueryEnd(GPUTimerQueryVulkan* query)
	{
#if VULKAN_USE_QUERIES
		_queriesInProgress.Remove(query);
#endif
	}

} // SE