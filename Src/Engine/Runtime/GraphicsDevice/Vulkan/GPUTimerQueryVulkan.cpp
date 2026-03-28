
#include "GPUTimerQueryVulkan.h"
#include "GPUContextVulkan.h"
#include "CmdBufferVulkan.h"

namespace SE
{
	GPUTimerQueryVulkan::GPUTimerQueryVulkan(GPUDeviceVulkan* device)
		: GPUResourceVulkan<GPUTimerQuery>(device, String::Empty)
	{
	}

	void GPUTimerQueryVulkan::Interrupt(CmdBufferVulkan* cmdBuffer)
	{
		if (!m_Interrupted)
		{
			m_Interrupted = true;
			WriteTimestamp(cmdBuffer, m_Queries[m_QueryIndex].End, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
		}
	}

	void GPUTimerQueryVulkan::Resume(CmdBufferVulkan* cmdBuffer)
	{
		ASSERT(m_Interrupted);

		QueryPair e;
		e.End.Pool = nullptr;

		m_Interrupted = false;
		WriteTimestamp(cmdBuffer, e.Begin, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);

		m_Queries.Add(e);
		m_QueryIndex++;
	}

	bool GPUTimerQueryVulkan::GetResult(Query& query)
	{
		if (query.Pool)
		{
			const auto context = (GPUContextVulkan*)m_Device->GetMainContext();
			if (query.Pool->GetResults(context, query.Index, query.Result))
			{
				// Release query
				query.Pool->ReleaseQuery(query.Index);
				query.Pool = nullptr;
			}
			else
			{
				// No results
				return true;
			}
		}

		// Has result
		return false;
	}

	void GPUTimerQueryVulkan::WriteTimestamp(CmdBufferVulkan* cmdBuffer, Query& query, VkPipelineStageFlagBits stage) const
	{
		auto pool = m_Device->FindAvailableTimestampQueryPool();
		uint32 index;
		pool->AcquireQuery(index);

		vkCmdWriteTimestamp(cmdBuffer->GetHandle(), stage, pool->GetHandle(), index);
		pool->MarkQueryAsStarted(index);

		query.Pool = pool;
		query.Index = index;
	}

	bool GPUTimerQueryVulkan::TryGetResult()
	{
		// Try get queries value (if not already)
		for (int32 i = 0; i < m_Queries.Count(); i++)
		{
			auto& e = m_Queries[i];

			if (GetResult(e.Begin))
				return false;
			if (GetResult(e.End))
				return false;
		}

		// Calculate delta time (accumulated)
		uint64 delta = 0;
		for (int32 i = 0; i < m_Queries.Count(); i++)
		{
			auto& e = m_Queries[i];

			if (e.End.Result > e.Begin.Result)
			{
				delta += e.End.Result - e.Begin.Result;
			}
		}

		// Calculate event duration in milliseconds
		const double frequency = double(m_Device->physicalDeviceLimits.timestampPeriod) / 1e6;
		m_TimeDelta = static_cast<float>((delta * frequency));

		// Clear data for next usage
		m_HasResult = true;
		for (int32 i = 0; i < m_Queries.Count(); i++)
		{
			auto& e = m_Queries[i];

			if (e.Begin.Pool)
			{
				e.Begin.Pool->ReleaseQuery(e.Begin.Index);
			}

			if (e.End.Pool)
			{
				e.End.Pool->ReleaseQuery(e.End.Index);
			}
		}
		m_Queries.Clear();
		return true;
	}

	bool GPUTimerQueryVulkan::UseQueries()
	{
		return m_Device->physicalDeviceLimits.timestampComputeAndGraphics == VK_TRUE;
	}

	void GPUTimerQueryVulkan::OnReleaseGPU()
	{
		m_HasResult = false;
		m_EndCalled = false;
		m_TimeDelta = 0.0f;

		for (int32 i = 0; i < m_Queries.Count(); i++)
		{
			auto& e = m_Queries[i];

			if (e.Begin.Pool)
			{
				e.Begin.Pool->ReleaseQuery(e.Begin.Index);
			}

			if (e.End.Pool)
			{
				e.End.Pool->ReleaseQuery(e.End.Index);
			}
		}
		m_Queries.Clear();
	}

	void GPUTimerQueryVulkan::Begin()
	{
		if (UseQueries())
		{
			const auto context = (GPUContextVulkan*)m_Device->GetMainContext();
			const auto cmdBuffer = context->GetCmdBufferManager()->GetCmdBuffer();

			QueryPair e;
			e.End.Pool = nullptr;

			m_QueryIndex = 0;
			m_Interrupted = false;
			WriteTimestamp(cmdBuffer, e.Begin, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
			context->GetCmdBufferManager()->OnQueryBegin(this);

			ASSERT(_queries.IsEmpty());
			m_Queries.Add(e);
		}
	}

	void GPUTimerQueryVulkan::End()
	{
		if (m_EndCalled)
			return;

		if (UseQueries())
		{
			const auto context = (GPUContextVulkan*)m_Device->GetMainContext();
			const auto cmdBuffer = context->GetCmdBufferManager()->GetCmdBuffer();

			if (!m_Interrupted)
			{
				WriteTimestamp(cmdBuffer, m_Queries[m_QueryIndex].End, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
			}
			context->GetCmdBufferManager()->OnQueryEnd(this);
		}

		m_EndCalled = true;
	}

	bool GPUTimerQueryVulkan::HasResult()
	{
		if (!m_EndCalled)
			return false;
		if (m_HasResult)
			return true;

		return TryGetResult();
	}

	float GPUTimerQueryVulkan::GetResult()
	{
		if (m_HasResult)
			return m_TimeDelta;

		TryGetResult();
		return m_TimeDelta;
	}
}
