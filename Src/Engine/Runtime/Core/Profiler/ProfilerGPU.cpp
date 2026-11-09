

#include <Runtime/Core/Profiler/ProfilerGPU.h>
#include <Runtime/Core/Logging/Logging.h>

namespace SE
{
    RenderStatsData RenderStatsData::Counter;

    Function<IGPUTimerQueryProfile*()>     ProfilerGPU::CreateTimerQueryProfileCall;
    Function<void(IGPUTimerQueryProfile*)> ProfilerGPU::DelateTimerQueryProfileCall;
    Function<void(const Char*)>            ProfilerGPU::EventBeginCall;
    Function<void()>                       ProfilerGPU::EventEndCall;
    Function<uint64()>                     ProfilerGPU::GetFrameCountCall;
    Function<void(uint64, const ProfilerGPU::EventBuffer*)> ProfilerGPU::s_FrameDataCall;

    int32                        ProfilerGPU::m_Depth = 0;
    List<IGPUTimerQueryProfile*> ProfilerGPU::m_TimerQueriesPool;
    List<IGPUTimerQueryProfile*> ProfilerGPU::m_TimerQueriesFree;
    bool                         ProfilerGPU::Enabled       = false;
    int32                        ProfilerGPU::CurrentBuffer = -1;
    ProfilerGPU::EventBuffer     ProfilerGPU::Buffers[PROFILER_GPU_EVENTS_FRAMES];

    bool ProfilerGPU::EventBuffer::HasData() const { return m_IsResolved && m_Data.HasItems(); }

    void ProfilerGPU::EventBuffer::EndAll()
    {
        for (int32 i = 0; i < m_Data.Count(); i++)
        {
            auto& event = m_Data[i];
            if (event.State == EventState::Active)
            {
                event.Timer->End();
                event.State = EventState::Incomplete;
            }
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
            e.Time  = e.Timer->IsSupported() ? e.Timer->GetResult() : -1.0f;
            m_TimerQueriesFree.Add(e.Timer);
            e.Timer = nullptr;
        }

        m_IsResolved = true;
        if (s_FrameDataCall.IsBinded())
            s_FrameDataCall(FrameIndex, this);
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
        m_IsResolved    = false;
        FrameIndex      = 0;
        PresentTime     = 0.0f;
        PresentMeasured = false;
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
        if (!Enabled || CurrentBuffer < 0)
            return -1;

        EventBeginCall(name);

        Event e;
        e.Name  = name;
        e.Stats = RenderStatsData::Counter;
        e.Timer = GetTimerQuery();
        e.Timer->Begin();
        e.Depth = m_Depth++;

        auto&      buffer = Buffers[CurrentBuffer];
        const auto index  = buffer.Add(e);
        return index;
    }

    void ProfilerGPU::EndEvent(int32 index)
    {
        if (index == -1)
            return;
        if (CurrentBuffer < 0)
        {
            LOG_WARNING("Profiler", "GPU event ended outside its recording frame");
            return;
        }
        m_Depth--;

        auto& buffer = Buffers[CurrentBuffer];
        auto  e      = buffer.Get(index);
        if (e->State == EventState::Active)
        {
            e->Stats.Mix(RenderStatsData::Counter);
            e->State = EventState::Completed;
            e->Timer->End();
        }

        EventEndCall();
    }

    void ProfilerGPU::BeginFrame()
    {
        // Clear stats
        RenderStatsData::Counter = RenderStatsData();
        m_Depth                  = 0;
        // Try to resolve previous frames
        for (int32 i = 0; i < PROFILER_GPU_EVENTS_FRAMES; i++)
        {
            Buffers[i].TryResolve();
        }
        // Keep every pending query alive. If all slots are busy, skip this frame without waiting or overwriting.
        CurrentBuffer = -1;
        if (Enabled)
        {
            uint64 oldest = uint64(-1);
            for (int32 i = 0; i < PROFILER_GPU_EVENTS_FRAMES; i++)
            {
                if (Buffers[i].IsReusable() && Buffers[i].FrameIndex < oldest)
                {
                    oldest        = Buffers[i].FrameIndex;
                    CurrentBuffer = i;
                }
            }
        }
        if (CurrentBuffer >= 0)
        {
            Buffers[CurrentBuffer].Clear();
            Buffers[CurrentBuffer].FrameIndex = GetFrameCountCall();
        }
    }

    void ProfilerGPU::OnPresent()
    {
        if (CurrentBuffer < 0)
            return;
        // End all current frame queries to prevent invalid event duration values
        auto& buffer = Buffers[CurrentBuffer];
        buffer.EndAll();
    }

    void ProfilerGPU::OnPresentTime(float time)
    {
        if (CurrentBuffer < 0)
            return;
        auto& buffer = Buffers[CurrentBuffer];
        buffer.PresentTime += time;
        buffer.PresentMeasured = true;
    }

    void ProfilerGPU::EndFrame()
    {
        if (m_Depth)
        {
            LOG_WARNING("Profiler", "GPU Profiler events start/end mismatch");
        }

        if (s_FrameDataCall.IsBinded())
            s_FrameDataCall(GetFrameCountCall(), CurrentBuffer >= 0 ? &Buffers[CurrentBuffer] : nullptr);
        CurrentBuffer = -1;
    }

    bool ProfilerGPU::GetLastFrameData(float& drawTimeMs, float& presentTimeMs, RenderStatsData& statsData)
    {
        uint64 maxFrame      = 0;
        int32  maxFrameIndex = -1;
        auto&  frames        = Buffers;
        for (uint32 i = 0; i < ARRAY_SIZE(frames); i++)
        {
            if (frames[i].HasData() && frames[i].FrameIndex > maxFrame)
            {
                maxFrame      = frames[i].FrameIndex;
                maxFrameIndex = i;
            }
        }
        if (maxFrameIndex != -1)
        {
            auto&      frame = frames[maxFrameIndex];
            const auto root  = frame.Get(0);
            drawTimeMs       = root->Time;
            presentTimeMs    = frame.PresentTime;
            statsData        = root->Stats;
            return true;
        }

        // No data
        drawTimeMs    = 0.0f;
        presentTimeMs = 0.0f;
        Platform::MemoryClear(&statsData, sizeof(statsData));
        return false;
    }

    void ProfilerGPU::Dispose()
    {
        for (size_t i = 0; i < m_TimerQueriesPool.Count(); i++)
        {
            IGPUTimerQueryProfile* profile = m_TimerQueriesPool[i];
            DelateTimerQueryProfileCall(profile);
        }
        m_TimerQueriesFree.Clear();
        m_TimerQueriesPool.Clear();
        for (EventBuffer& buffer : Buffers)
            buffer.Clear();
    }
} // namespace SE
