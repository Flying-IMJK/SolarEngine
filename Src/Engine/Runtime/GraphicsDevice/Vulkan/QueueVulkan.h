#pragma once

#include "Runtime/Core/Types/Variable.h"
#include "Runtime/Core/Thread/Threading.h"
#include "VulkanInclude.h"

namespace SE
{
	class GPUDeviceVulkan;
	class CmdBufferVulkan;

	/// <summary>
	/// Implementation of the command buffer queue for the Vulkan backend.
	/// </summary>
	class QueueVulkan
	{
	private:
		VkQueue m_Queue;
		uint32 m_FamilyIndex;
		uint32 m_QueueIndex;
		GPUDeviceVulkan* m_Device;
		CriticalSection m_Locker;
		CmdBufferVulkan* m_LastSubmittedCmdBuffer;
		uint64 m_LastSubmittedCmdBufferFenceCounter;

	public:
		QueueVulkan(GPUDeviceVulkan* device, uint32 familyIndex);

		inline uint32 GetFamilyIndex() const
		{
			return m_FamilyIndex;
		}

		void Submit(CmdBufferVulkan* cmdBuffer, uint32 signalSemaphoresCount = 0, const VkSemaphore* signalSemaphores = nullptr);

		inline void Submit(CmdBufferVulkan* cmdBuffer, VkSemaphore signalSemaphore)
		{
			Submit(cmdBuffer, 1, &signalSemaphore);
		}

		inline VkQueue GetHandle() const
		{
			return m_Queue;
		}

		void GetLastSubmittedInfo(CmdBufferVulkan*& cmdBuffer, uint64& fenceCounter) const;

	private:
		void UpdateLastSubmittedCommandBuffer(CmdBufferVulkan* cmdBuffer);
	};

}
