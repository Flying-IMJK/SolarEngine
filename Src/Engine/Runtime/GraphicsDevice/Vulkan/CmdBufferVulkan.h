#pragma once

#include "Runtime/Core/Types/Collections/List.h"

#include "VulkanInclude.h"

namespace SE
{
	class QueueVulkan;
	class GPUContextVulkan;
	class GPUTimerQueryVulkan;
	class SemaphoreVulkan;
	class GPUDeviceVulkan;
	class CmdBufferPoolVulkan;
	class DescriptorPoolSetContainerVulkan;
	class RenderPassVulkan;
	class FenceVulkan;
	class FramebufferVulkan;

	/// <summary>
	/// Vulkan后端 命令缓冲区实现
	/// </summary>
	class CmdBufferVulkan
	{
		friend QueueVulkan;

	public:
		enum class State
		{
			ReadyForBegin,
			IsInsideBegin,
			IsInsideRenderPass,
			HasEnded,
			Submitted,
		};
		VkCommandBuffer m_CommandBuffer;
	private:
		GPUDeviceVulkan* m_Device;

		State m_State;

		List<VkPipelineStageFlags> m_WaitFlags;
		List<SemaphoreVulkan*> m_WaitSemaphores;
		List<SemaphoreVulkan*> m_SubmittedWaitSemaphores;

		FenceVulkan* m_Fence;
		int32 m_EventsBegin = 0;

		// The latest value when command buffer was submitted.
		volatile uint64 m_SubmittedFenceCounter;

		// The latest value passed after the fence was signaled.
		volatile uint64 m_FenceSignaledCounter;

		CmdBufferPoolVulkan* m_CommandBufferPool;

		DescriptorPoolSetContainerVulkan* m_DescriptorPoolSetContainer = nullptr;

	public:
		CmdBufferVulkan(GPUDeviceVulkan* device, CmdBufferPoolVulkan* pool);
		~CmdBufferVulkan();

	public:
		CmdBufferPoolVulkan* GetOwner() const
		{
			return m_CommandBufferPool;
		}

		State GetState() const
		{
			return m_State;
		}

		FenceVulkan* GetFence() const
		{
			return m_Fence;
		}

		inline bool IsInsideRenderPass() const
		{
			return m_State == State::IsInsideRenderPass;
		}

		inline bool IsOutsideRenderPass() const
		{
			return m_State == State::IsInsideBegin;
		}

		inline bool HasBegun() const
		{
			return m_State == State::IsInsideBegin || m_State == State::IsInsideRenderPass;
		}

		inline bool HasEnded() const
		{
			return m_State == State::HasEnded;
		}

		inline bool IsSubmitted() const
		{
			return m_State == State::Submitted;
		}

		inline VkCommandBuffer GetHandle() const
		{
			return m_CommandBuffer;
		}

		inline volatile uint64 GetFenceSignaledCounter() const
		{
			return m_FenceSignaledCounter;
		}

		inline volatile uint64 GetSubmittedFenceCounter() const
		{
			return m_SubmittedFenceCounter;
		}

	public:
		void AddWaitSemaphore(VkPipelineStageFlags waitFlags, SemaphoreVulkan* waitSemaphore);

		void Begin();
		void End();

		void BeginRenderPass(RenderPassVulkan* renderPass, FramebufferVulkan* framebuffer, uint32 clearValueCount, VkClearValue* clearValues);
		void EndRenderPass();

		inline DescriptorPoolSetContainerVulkan* GetDescriptorPoolSet() const
		{
			return m_DescriptorPoolSetContainer;
		}


		void BeginEvent(const Char* name);
    	void EndEvent();

		void RefreshFenceStatus();
	};

	class CmdBufferPoolVulkan
	{
		friend class CmdBufferManagerVulkan;
	private:
		GPUDeviceVulkan* _device;
		VkCommandPool _handle;
		List<CmdBufferVulkan*> _cmdBuffers;

		CmdBufferVulkan* Create();

		void Create(uint32 queueFamilyIndex);

	public:
		CmdBufferPoolVulkan(GPUDeviceVulkan* device);
		~CmdBufferPoolVulkan();

	public:
		inline VkCommandPool GetHandle() const
		{
			return _handle;
		}

		void RefreshFenceStatus(const CmdBufferVulkan* skipCmdBuffer = nullptr);
	};

	class CmdBufferManagerVulkan
	{
	private:
		GPUDeviceVulkan* _device;
		CmdBufferPoolVulkan _pool;
		QueueVulkan* _queue;
		CmdBufferVulkan* _activeCmdBuffer;
		List<GPUTimerQueryVulkan*> _queriesInProgress;

	public:
		CmdBufferManagerVulkan(GPUDeviceVulkan* device, GPUContextVulkan* context);

	public:
		inline VkCommandPool GetHandle() const
		{
			return _pool.GetHandle();
		}

		inline CmdBufferVulkan* GetActiveCmdBuffer() const
		{
			return _activeCmdBuffer;
		}

		inline bool HasPendingActiveCmdBuffer() const
		{
			return _activeCmdBuffer != nullptr;
		}

		inline bool HasQueriesInProgress() const
		{
			return _queriesInProgress.Count() != 0;
		}

		inline CmdBufferVulkan* GetCmdBuffer()
		{
			if (!_activeCmdBuffer)
				PrepareForNewActiveCommandBuffer();
			return _activeCmdBuffer;
		}

	public:
		void SubmitActiveCmdBuffer(SemaphoreVulkan* signalSemaphore = nullptr);
		void WaitForCmdBuffer(CmdBufferVulkan* cmdBuffer, float timeInSecondsToWait = 1.0f);
		void RefreshFenceStatus(CmdBufferVulkan* skipCmdBuffer = nullptr)
		{
			_pool.RefreshFenceStatus(skipCmdBuffer);
		}
		void PrepareForNewActiveCommandBuffer();

		void OnQueryBegin(GPUTimerQueryVulkan* query);
		void OnQueryEnd(GPUTimerQueryVulkan* query);
	};

} // SE
