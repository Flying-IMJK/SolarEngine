

#include "ProfilerGPU.h"
#include "Core/Logging/Logging.h"

namespace SE
{
	RenderStatsData RenderStatsData::Counter;

	Function<IGPUTimerQueryProfile*()> ProfilerGPU::CreateTimerQueryProfileCall;
	Function<void(const Char*)> ProfilerGPU::EventBeginCall;
	Function<void()> ProfilerGPU::EventEndCall;
	Function<uint64()> ProfilerGPU::GetFrameCountCall;

	int32 ProfilerGPU::m_Depth = 0;
	List<IGPUTimerQueryProfile*> ProfilerGPU::m_TimerQueriesPool;
	List<IGPUTimerQueryProfile*> ProfilerGPU::m_TimerQueriesFree;
	bool ProfilerGPU::Enabled = false;
	int32 ProfilerGPU::CurrentBuffer = 0;
	ProfilerGPU::EventBuffer ProfilerGPU::Buffers[PROFILER_GPU_EVENTS_FRAMES];

	bool ProfilerGPU::EventBuffer::HasData() const
	{
		return m_IsResolved && m_Data.HasItems();
	}

	void ProfilerGPU::EventBuffer::EndAll()
	{
		for (int32 i = 0; i < m_Data.Count(); i++)
		{
			m_Data[i].Timer->End();
		}
	}

	void ProfilerGPU::EventBuffer::TryResolve()
	{
		if (m_IsResolved || m_Data.IsEmpty())
			return;

		// Check all the queries from the back to the front (in some cases inner queries are not finished)
		for (int32 i = m_Data.Count() - 1; i >= 0; i--)
		{
			const Event& e = m_Data[i];
			ENGINE_ASSERT(e.Timer != nullptr);
			if (!e.Timer->HasResult())
			{
				return;
			}
		}

		// Collect queries results and free them
		for (int32 i = 0; i < m_Data.Count(); i++)
		{
			auto& e = m_Data[i];
			e.Time = e.Timer->GetResult();
			m_TimerQueriesFree.Add(e.Timer);
			e.Timer = nullptr;
		}

		m_IsResolved = true;
	}

	int32 ProfilerGPU::EventBuffer::Add(const Event& e)
	{
		const int32 index = m_Data.Count();
		m_Data.Add(e);
		return index;
	}

	void ProfilerGPU::EventBuffer::Extract(List<Event>& data) const
	{
		// Don't use unresolved data
		ASSERT(m_IsResolved);
		data = m_Data;
	}

	void ProfilerGPU::EventBuffer::Clear()
	{
		m_Data.Clear();
		m_IsResolved = false;
		FrameIndex = 0;
		PresentTime = 0.0f;
	}

	IGPUTimerQueryProfile* ProfilerGPU::GetTimerQuery()
	{
		IGPUTimerQueryProfile* result;
		if (m_TimerQueriesFree.HasItems())
		{
			result = m_TimerQueriesFree.Last();
			m_TimerQueriesFree.RemoveLast();
		}
		else
		{
			result = CreateTimerQueryProfileCall();
			m_TimerQueriesPool.Add(result);
		}
		return result;
	}

	int32 ProfilerGPU::BeginEvent(const Char* name)
	{
		if (!Enabled)
			return -1;

		EventBeginCall(name);

		Event e;
		e.Name = name;
		e.Stats = RenderStatsData::Counter;
		e.Timer = GetTimerQuery();
		e.Timer->Begin();
		e.Depth = m_Depth++;

		auto& buffer = Buffers[CurrentBuffer];
		const auto index = buffer.Add(e);
		return index;
	}

	void ProfilerGPU::EndEvent(int32 index)
	{
		if (index == -1)
			return;
		m_Depth--;

		auto& buffer = Buffers[CurrentBuffer];
		auto e = buffer.Get(index);
		e->Stats.Mix(RenderStatsData::Counter);
		e->Timer->End();

		EventEndCall();
	}

	void ProfilerGPU::BeginFrame()
	{
		// Clear stats
		RenderStatsData::Counter = RenderStatsData();
		m_Depth = 0;
		auto& buffer = Buffers[CurrentBuffer];
		buffer.FrameIndex = GetFrameCountCall();
		buffer.PresentTime = 0.0f;

		// Try to resolve previous frames
		for (int32 i = 0; i < PROFILER_GPU_EVENTS_FRAMES; i++)
		{
			Buffers[i].TryResolve();
		}
	}

	void ProfilerGPU::OnPresent()
	{
		// End all current frame queries to prevent invalid event duration values
		auto& buffer = Buffers[CurrentBuffer];
		buffer.EndAll();
	}

	void ProfilerGPU::OnPresentTime(float time)
	{
		auto& buffer = Buffers[CurrentBuffer];
		buffer.PresentTime += time;
	}

	void ProfilerGPU::EndFrame()
	{
		if (m_Depth)
		{
			LOG_WARNING("Profiler", "GPU Profiler events start/end mismatch");
		}

		// Move frame
		CurrentBuffer = (CurrentBuffer + 1) % PROFILER_GPU_EVENTS_FRAMES;

		// Prepare current frame buffer
		auto& buffer = Buffers[CurrentBuffer];
		buffer.Clear();
	}

	bool ProfilerGPU::GetLastFrameData(float& drawTimeMs, float& presentTimeMs, RenderStatsData& statsData)
	{
		uint64 maxFrame = 0;
		int32 maxFrameIndex = -1;
		auto& frames = Buffers;
		for (uint32 i = 0; i < ARRAY_SIZE(frames); i++)
		{
			if (frames[i].HasData() && frames[i].FrameIndex > maxFrame)
			{
				maxFrame = frames[i].FrameIndex;
				maxFrameIndex = i;
			}
		}
		if (maxFrameIndex != -1)
		{
			auto& frame = frames[maxFrameIndex];
			const auto root = frame.Get(0);
			drawTimeMs = root->Time;
			presentTimeMs = frame.PresentTime;
			statsData = root->Stats;
			return true;
		}

		// No data
		drawTimeMs = 0.0f;
		presentTimeMs = 0.0f;
		Platform::MemoryClear(&statsData, sizeof(statsData));
		return false;
	}

	void ProfilerGPU::Dispose()
	{
		m_TimerQueriesPool.ClearDelete();
		m_TimerQueriesFree.Clear();
	}
}
