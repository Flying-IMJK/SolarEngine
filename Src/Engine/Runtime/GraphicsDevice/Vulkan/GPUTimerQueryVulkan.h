#pragma once

#include "Runtime/Graphics/GPUTimerQuery.h"
#include "GPUDeviceVulkan.h"

namespace SE
{
	/// <summary>
	/// GPU timer query object for Vulkan backend.
	/// </summary>
	class GPUTimerQueryVulkan : public GPUResourceVulkan<GPUTimerQuery>
	{
	private:
		struct Query
		{
			BufferedQueryPoolVulkan* Pool;
			uint32 Index;
			uint64 Result;
		};

		struct QueryPair
		{
			Query Begin;
			Query End;
		};

		bool m_HasResult = false;
		bool m_EndCalled = false;
		bool m_Interrupted = false;
		float m_TimeDelta = 0.0f;
		int32 m_QueryIndex;
		List<QueryPair, InlinedAllocation<8>> m_Queries;

	public:
		/// <summary>
		/// Initializes a new instance of the <see cref="GPUTimerQueryVulkan"/> class.
		/// </summary>
		/// <param name="device">The graphics device.</param>
		GPUTimerQueryVulkan(GPUDeviceVulkan* device);

	public:
		/// <summary>
		/// Interrupts an in-progress query, allowing the command buffer to submitted. Interrupted queries must be resumed using Resume().
		/// </summary>
		/// <param name="cmdBuffer">The GPU commands buffer.</param>
		void Interrupt(CmdBufferVulkan* cmdBuffer);

		/// <summary>
		/// Resumes an interrupted query, restoring it back to its original in-progress state.
		/// </summary>
		/// <param name="cmdBuffer">The GPU commands buffer.</param>
		void Resume(CmdBufferVulkan* cmdBuffer);

	private:
		bool GetResult(Query& query);
		void WriteTimestamp(CmdBufferVulkan* cmdBuffer, Query& query, VkPipelineStageFlagBits stage) const;
		bool TryGetResult();
		bool UseQueries();

	public:
		// [GPUTimerQuery]
		void Begin() override;
		void End() override;
		bool HasResult() override;
		float GetResult() override;

	protected:
		// [GPUResourceVulkan]
		void OnReleaseGPU() override;
	};
}
