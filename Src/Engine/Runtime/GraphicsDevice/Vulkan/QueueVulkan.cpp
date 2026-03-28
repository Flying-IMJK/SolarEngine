#pragma once
#include "QueueVulkan.h"
#include "VulkanTool.h"
#include "CmdBufferVulkan.h"
#include "GPUDeviceVulkan.h"
#include "VulkanNative.h"

namespace SE
{

	QueueVulkan::QueueVulkan(GPUDeviceVulkan* device, uint32 familyIndex)
		: m_Queue(VK_NULL_HANDLE)
		, m_FamilyIndex(familyIndex)
		, m_QueueIndex(0)
		, m_Device(device)
		, m_LastSubmittedCmdBuffer(nullptr)
		, m_LastSubmittedCmdBufferFenceCounter(0)
	{
		vkGetDeviceQueue(device->device, familyIndex, 0, &m_Queue);
	}

	void QueueVulkan::Submit(CmdBufferVulkan* cmdBuffer, uint32 signalSemaphoresCount, const VkSemaphore* signalSemaphores)
	{
		ENGINE_ASSERT(cmdBuffer->HasEnded());
		auto fence = cmdBuffer->GetFence();
		ENGINE_ASSERT(!fence->IsSignaled);

		const VkCommandBuffer cmdBuffers[] = { cmdBuffer->GetHandle() };

		VkSubmitInfo submitInfo;
		VulkanTool::ZeroStruct(submitInfo, VK_STRUCTURE_TYPE_SUBMIT_INFO);
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = cmdBuffers;
		submitInfo.signalSemaphoreCount = signalSemaphoresCount;
		submitInfo.pSignalSemaphores = signalSemaphores;

		List<VkSemaphore, InlinedAllocation<8>> waitSemaphores;
		if (cmdBuffer->m_WaitSemaphores.HasItems())
		{
			waitSemaphores.EnsureCapacity(cmdBuffer->m_WaitSemaphores.Count());
			for (auto semaphore : cmdBuffer->m_WaitSemaphores)
			{
				waitSemaphores.Add(semaphore->GetHandle());
			}
			submitInfo.waitSemaphoreCount = cmdBuffer->m_WaitSemaphores.Count();
			submitInfo.pWaitSemaphores = waitSemaphores.Get();
			submitInfo.pWaitDstStageMask = cmdBuffer->m_WaitFlags.Get();
		}

		VALIDATE_VULKAN_RESULT(vkQueueSubmit(m_Queue, 1, &submitInfo, fence->handle));

		// Mark semaphores as submitted
		cmdBuffer->m_State = CmdBufferVulkan::State::Submitted;
		cmdBuffer->m_WaitFlags.Clear();
		cmdBuffer->m_SubmittedWaitSemaphores = cmdBuffer->m_WaitSemaphores;
		cmdBuffer->m_WaitSemaphores.Clear();
		cmdBuffer->m_SubmittedFenceCounter = cmdBuffer->m_FenceSignaledCounter;

#if 0
		// Wait for the GPU to be idle on every submit (useful for tracking GPU hangs)
		const bool WaitForIdleOnSubmit = false;
		if (WaitForIdleOnSubmit)
		{
			// Use 200ms timeout
			bool success = m_Device->FenceManager.WaitForFence(fence, 200 * 1000 * 1000);
			ENGINE_ASSERT(success);
			ENGINE_ASSERT(m_Device->FenceManager.IsFenceSignaled(fence));
			cmdBuffer->GetOwner()->RefreshFenceStatus();
		}
#endif

		UpdateLastSubmittedCommandBuffer(cmdBuffer);

		cmdBuffer->GetOwner()->RefreshFenceStatus(cmdBuffer);
	}

	void QueueVulkan::GetLastSubmittedInfo(CmdBufferVulkan*& cmdBuffer, uint64& fenceCounter) const
	{
		Threading::ScopeLock locker(m_Locker);
		cmdBuffer = m_LastSubmittedCmdBuffer;
		fenceCounter = m_LastSubmittedCmdBufferFenceCounter;
	}

	void QueueVulkan::UpdateLastSubmittedCommandBuffer(CmdBufferVulkan* cmdBuffer)
	{
		Threading::ScopeLock locker(m_Locker);
		m_LastSubmittedCmdBuffer = cmdBuffer;
		m_LastSubmittedCmdBufferFenceCounter = cmdBuffer->GetFenceSignaledCounter();
	}
} // SE