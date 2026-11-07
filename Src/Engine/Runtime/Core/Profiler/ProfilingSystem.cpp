
#include <Runtime/Core/Profiler/ProfilingSystem.h>
#include <Runtime/Core/Thread/JobSystem.h>
#include <Runtime/Core/Thread/Threading.h>
#include <Runtime/Core/Types/Collections/Sorting.h>
#include <Runtime/Engine.h>
#include <Runtime/Graphics/GPUContext.h>
#include <Runtime/Graphics/GPUDevice.h>
#include <Runtime/Graphics/GPUTimerQuery.h>

#ifdef SE_TRACY_SERVER
#include <TracyMemory.hpp>
#include <TracyWorker.hpp>
#endif

namespace SE
{
    ProfilingSystem::MainStats                                ProfilingSystem::Stats;
    List<ProfilingSystem::ThreadStats, InlinedAllocation<64>> ProfilingSystem::EventsCPU;
    List<ProfilerGPU::Event>                                  ProfilingSystem::EventsGPU;
    List<ProfilingSystem::NetworkEventStat>                   ProfilingSystem::EventsNetwork;

    ENGINE_SYSTEM_REGISTER(ProfilingSystem)

    ProfilingSystem::ProfilingSystem() : ISystem(SE_TEXT("Profiling"))
    {
        Platform::MemoryClear(&ProfilingSystem::Stats, sizeof(ProfilingSystem::MainStats));

        ProfilerGPU::GetFrameCountCall = []() { return Engine::FrameCount; };

        ProfilerGPU::EventBeginCall = [](const Char* event) {
            GPUDevice::instance->GetMainContext()->EventBegin(event);
        };

        ProfilerGPU::EventEndCall = []() { GPUDevice::instance->GetMainContext()->EventEnd(); };

        ProfilerGPU::CreateTimerQueryProfileCall = []() {
            IGPUTimerQueryProfile* timerQuery = GPUDevice::instance->CreateTimerQuery();
            return timerQuery;
        };

        ProfilerGPU::DelateTimerQueryProfileCall = [](IGPUTimerQueryProfile* timerQuery) {
            GPUTimerQuery* gpuTimerQuery = static_cast<GPUTimerQuery*>(timerQuery);

            gpuTimerQuery->ReleaseGPU();
            Delete(gpuTimerQuery);
        };

#if SE_EDITOR
        ProfilerGPU::Enabled = true;
#endif
    }

    ProfilingSystem::~ProfilingSystem() = default;

    bool ProfilingSystem::OnInit()
    {
        Threading::ScopeLock lock(m_CaptureLocker);
        m_CaptureInitialized              = true;
        ProfilerGPU::s_FrameSubmittedCall = [this](uint64 frameId, const ProfilerGPU::EventBuffer* pFrame) {
            OnGpuSubmitted(frameId, pFrame);
        };
        ProfilerGPU::s_FrameResolvedCall = [this](const ProfilerGPU::EventBuffer& frame) { OnGpuResolved(frame); };
#ifdef SE_TRACY_SERVER
        m_CaptureStateData.State = CaptureState::Idle;
#endif
        return true;
    }

    void ProfilingSystem::OnUpdate()
    {
        Threading::ScopeLock lock(m_CaptureLocker);
        UpdateCaptureState();
        SampleMemory();
    }

    void ProfilingSystem::Update()
    {
        if (GPUDevice::instance == nullptr)
            return;
        ZoneScoped;

        // Capture stats
        {
            auto& stats = ProfilingSystem::Stats;

            stats.ProcessMemory   = Platform::GetProcessMemoryStats();
            stats.MemoryCPU       = Platform::GetMemoryStats();
            stats.MemoryGPU.Total = GPUDevice::instance->GetTotalGraphicsMemory();
            stats.MemoryGPU.Used  = GPUDevice::instance->GetMemoryUsage();
            //			stats.FPS = Engine::GetFramesPerSecond();

            //			stats.UpdateTimeMs = static_cast<float>(Time::Update.LastLength * 1000.0);
            //			stats.PhysicsTimeMs = static_cast<float>(Time::Physics.LastLength * 1000.0);
            //			stats.DrawCPUTimeMs = static_cast<float>(Time::Draw.LastLength * 1000.0);

            float presentTime;
            ProfilerGPU::GetLastFrameData(stats.DrawGPUTimeMs, presentTime, stats.DrawStats);
            stats.DrawCPUTimeMs = Math::Max(stats.DrawCPUTimeMs - presentTime,
                                            0.0f); // Remove swapchain present wait time to exclude from drawing on CPU
        }

        // Extract CPU profiler events
        Platform::MemoryBarrier();
        const auto& threads = ProfilerCPU::Threads;
        Platform::MemoryBarrier();
        for (auto& pt : ProfilingSystem::EventsCPU)
            pt.Events.Clear();
        ProfilingSystem::EventsCPU.EnsureCapacity(threads.Count());
        for (int32 i = 0; i < threads.Count(); i++)
        {
            ProfilerCPU::Thread* thread = threads[i];
            if (thread == nullptr)
                continue;
            ProfilingSystem::ThreadStats* pt = nullptr;
            for (auto& e : ProfilingSystem::EventsCPU)
            {
                if (e.Name == thread->GetName())
                {
                    pt = &e;
                    break;
                }
            }
            if (!pt)
            {
                pt       = &ProfilingSystem::EventsCPU.AddOne();
                pt->Name = thread->GetName();
            }

            thread->Buffer.Extract(pt->Events, true);
        }

#if 0
        // Print CPU threads events to the log
        for (auto& pt : ProfilingTools::EventsCPU)
        {
            auto& events = pt.Events;
            if (events.HasItems())
            {
                LOG_FLOOR();
                LOG(Info, "Thread: {0}", pt.Name);
                for (int j = 0; j < events.Count(); j++)
                {
                    auto e = events[j];
                    String prev;
                    for (int d = 0; d < e.Depth; d++)
                        prev += TEXT("\t");
                    LOG(Warning, "{2}{0}, Time: {1} ms", e.Name, ((int)((e.End - e.Start) * 1000.0f) / 1000.0f), prev);
                }
                LOG(Info, "");
                LOG_FLOOR();
            }
        }
#endif

        // Get the last resolved GPU frame events
        ProfilingSystem::EventsGPU.Clear();
        uint64 maxFrame      = 0;
        int32  maxFrameIndex = -1;
        auto&  frames        = ProfilerGPU::Buffers;
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
            auto& frame = frames[maxFrameIndex];
            frame.Extract(ProfilingSystem::EventsGPU);
        }

        // Get the last events from networking runtime
        {
            /*			auto& networkEvents = ProfilingTools::EventsNetwork;
                        networkEvents.Resize(NetworkInternal::ProfilerEvents.Count());
                        int32 i = 0;
                        for (const auto& e : NetworkInternal::ProfilerEvents)
                        {
                            const auto& src = e.Value;
                            auto& dst = networkEvents[i++];
                            dst.Count = src.Count;
                            dst.DataSize = src.DataSize;
                            dst.MessageSize = src.MessageSize;
                            dst.Receivers = src.Receivers;
                            const StringAnsiView& typeName = e.Key.First.GetType().Fullname;
                            uint64 len = Math::Min<uint64>(typeName.Length(), ARRAY_SIZE(dst.Name) - 10);
                            Platform::MemoryCopy(dst.Name, typeName.Get(), len);
                            const StringAnsiView& name = e.Key.Second;
                            if (name.HasChars())
                            {
                                uint64 pos = len;
                                dst.Name[pos++] = ':';
                                dst.Name[pos++] = ':';
                                len = Math::Min<uint64>(name.Length(), ARRAY_SIZE(dst.Name) - pos - 1);
                                Platform::MemoryCopy(dst.Name + pos, name.Get(), len);
                                dst.Name[pos + len] = 0;
                            }
                            else
                            {
                                dst.Name[len] = 0;
                            }
                        }
                        NetworkInternal::ProfilerEvents.Clear();*/
        }

#if 0
        // Print CPU events to the log
        {
            if (ProfilingTools::EventsCPU.HasItems())
            {
                LOG_FLOOR();
                LOG(Info, "CPU");
                for (auto& pt : ProfilingTools::EventsCPU)
                {
                    LOG(Info, "");
                    LOG(Warning, "Thread {0}", pt.Name);
                    for (auto& e : pt.Events)
                    {
                        String prev;
                        for (int32 d = 0; d < e.Depth; d++)
                            prev += TEXT("\t");
                        const double time = e.End - e.Start;
                        LOG(Warning, "\t{2}{0}, Time: {1} ms", e.Name, ((int32)(time * 1000.0f) / 1000.0f), prev);
                    }
                }
                LOG(Info, "");
                LOG_FLOOR();
            }
        }
#endif
#if 0
        // Print GPU events to the log
        {
            auto& events = ProfilingTools::EventsGPU;
            if (events.HasItems())
            {
                LOG_FLOOR();
                LOG(Info, "GPU");
                for (int j = 0; j < events.Count(); j++)
                {
                    auto e = events[j];
                    String prev;
                    for (int d = 0; d < e.Depth; d++)
                        prev += TEXT("\t");
                    LOG(Warning, "{2}{0}, Time: {1} ms", e.Name, ((int)(e.Time * 1000.0f) / 1000.0f), prev);
                }
                LOG(Info, "");
                LOG_FLOOR();
            }
        }
#endif
    }

    void ProfilingSystem::OnDispose()
    {
        ProfilerGPU::s_FrameSubmittedCall.Unbind();
        ProfilerGPU::s_FrameResolvedCall.Unbind();
#ifdef SE_TRACY_SERVER
        if (m_CaptureWorker)
            m_CaptureWorker->Shutdown();
        if (m_CaptureCleanup != 0)
        {
            Threading::JobSystem::Wait(m_CaptureCleanup);
            m_CaptureCleanup = 0;
        }
        // The existing destructor joins Tracy's threads. No data lock is held here.
        m_CaptureWorker.reset();
#endif
        Threading::ScopeLock lock(m_CaptureLocker);
        m_CaptureInitialized = false;
        ResetView();
        ResetGpuHistory();
        m_CaptureStateData = CaptureInfo();
        ProfilingSystem::EventsCPU.Clear();
        ProfilingSystem::EventsCPU.SetCapacity(0);
        ProfilingSystem::EventsGPU.SetCapacity(0);
        ProfilingSystem::EventsNetwork.SetCapacity(0);
    }

    bool ProfilingSystem::GetEnabled() { return ProfilerCPU::Enabled && ProfilerGPU::Enabled; }

    void ProfilingSystem::SetEnabled(bool enabled)
    {
        ProfilerCPU::Enabled = enabled;
        ProfilerGPU::Enabled = enabled;
        //		NetworkInternal::EnableProfiling = enabled;
    }

    void ProfilingSystem::ResetGpuHistory()
    {
        ResetMemoryHistory();
        m_FirstGpuFrame  = 0;
        m_CurrentGpuView = ProfilerGpuViewInfo();
        m_GpuHistoryFrames.Clear();
        m_GpuHistoryFrames.SetCapacity(0);
        m_GpuFrameIndices.Clear();
        m_GpuFrameIndices.SetCapacity(0);
        m_GpuHistoryEvents.Clear();
        m_GpuHistoryEvents.SetCapacity(0);
        m_GpuHistoryText.Clear();
        m_GpuHistoryText.SetCapacity(0);
        m_CaptureStateData.GpuFrames       = 0;
        m_CaptureStateData.GpuPending      = 0;
        m_CaptureStateData.GpuHistoryLimit = 0;
    }

    void ProfilingSystem::StopForGpuCapacity()
    {
        m_CaptureStateData.GpuHistoryLimit = 1;
#ifdef SE_TRACY_SERVER
        if (m_CaptureStateData.State == CaptureState::Capturing && m_CaptureWorker && m_CaptureWorker->IsConnected())
        {
            m_CaptureStateData.StopReason          = StopReason::GpuHistoryCapacity;
            m_CaptureStateData.State               = CaptureState::Stopping;
            m_CaptureStateData.TailMayBeIncomplete = 1;
            m_CaptureWorker->Shutdown();
        }
#endif
    }

    void ProfilingSystem::OnGpuSubmitted(uint64 frameId, const ProfilerGPU::EventBuffer* pFrame)
    {
        Threading::ScopeLock lock(m_CaptureLocker);
        if (m_CaptureStateData.State != CaptureState::Capturing || m_FirstGpuFrame == 0 || frameId < m_FirstGpuFrame)
            return;
#ifdef SE_TRACY_SERVER
        if (!m_CaptureWorker || !m_CaptureWorker->IsConnected())
            return;
#endif
        if (m_GpuHistoryFrames.Count() == MaxGpuFrames)
        {
            StopForGpuCapacity();
            return;
        }
        const int32      index  = m_GpuHistoryFrames.Count();
        GpuHistoryFrame& record = m_GpuHistoryFrames.AddOne();
        record.Info.FrameId     = frameId;
        record.Info.State       = !pFrame ? (ProfilerGPU::Enabled ? ProfilerGpuFrameState::BufferUnavailable :
                                                                    ProfilerGpuFrameState::TimingUnavailable) :
                                  pFrame->Count() == 0 ? ProfilerGpuFrameState::NoEvents :
                                                         ProfilerGpuFrameState::Pending;
        if (pFrame)
        {
            record.Info.EventCount       = pFrame->Count();
            record.Info.PresentMs        = pFrame->PresentTime;
            record.Info.PresentAvailable = pFrame->PresentMeasured ? 1 : 0;
        }
        m_GpuFrameIndices.Add(frameId, index);
        m_CaptureStateData.GpuFrames = m_GpuHistoryFrames.Count();
        m_CaptureStateData.GpuPending += record.Info.State == ProfilerGpuFrameState::Pending ? 1 : 0;
    }

    void ProfilingSystem::OnGpuResolved(const ProfilerGPU::EventBuffer& frame)
    {
        Threading::ScopeLock lock(m_CaptureLocker);
        int32                index;
        if (!m_GpuFrameIndices.TryGet(frame.FrameIndex, index))
            return;
        GpuHistoryFrame& record = m_GpuHistoryFrames[index];
        if (record.Info.State != ProfilerGpuFrameState::Pending)
            return;
        m_CaptureStateData.GpuPending--;
        record.Info.ResolvedAtFrame = Engine::FrameCount;
        record.FirstEvent           = m_GpuHistoryEvents.Count();
        record.FirstText            = m_GpuHistoryText.Count();
        if (frame.Count() > MaxGpuEvents - record.FirstEvent)
        {
            record.Info.State = ProfilerGpuFrameState::CapacityExceeded;
            StopForGpuCapacity();
            return;
        }
        List<int32> parents;
        int32       validTimings = 0;
        for (int32 i = 0; i < frame.Count(); i++)
        {
            const ProfilerGPU::Event& source = *frame.Get(i);
            if (source.Name.Length() >= MaxGpuTextBytes - m_GpuHistoryText.Count())
            {
                m_GpuHistoryEvents.Resize(record.FirstEvent);
                m_GpuHistoryText.Resize(record.FirstText);
                record.Info.State = ProfilerGpuFrameState::CapacityExceeded;
                StopForGpuCapacity();
                return;
            }
            int32 length = 0;
            char* name   = StringUtils::ConvertUTF162UTF8(source.Name.GetText(), source.Name.Length(), length);
            if (length >= MaxGpuTextBytes - m_GpuHistoryText.Count())
            {
                PlatformAllocator::Free(name);
                m_GpuHistoryEvents.Resize(record.FirstEvent);
                m_GpuHistoryText.Resize(record.FirstText);
                record.Info.State = ProfilerGpuFrameState::CapacityExceeded;
                StopForGpuCapacity();
                return;
            }
            ProfilerGpuEvent& event = m_GpuHistoryEvents.AddOne();
            event.NameOffset        = m_GpuHistoryText.Count();
            if (length > 0)
                m_GpuHistoryText.Add(reinterpret_cast<const byte*>(name), length);
            m_GpuHistoryText.Add(0);
            PlatformAllocator::Free(name);
            event.Depth    = source.Depth;
            event.Complete = source.Complete && !source.ForcedEnd ? 1 : 0;
            event.TimingValid =
                event.Complete && source.TimingValid && source.Time >= 0 && source.Time < Math::POS_INFINITY ? 1 : 0;
            event.StatsValid = event.Complete;
            while (parents.Count() > source.Depth && parents.HasItems())
                parents.RemoveLast();
            if (source.Depth >= 0 && source.Depth == parents.Count())
            {
                event.ParentIndex = source.Depth > 0 ? parents.Last() : -1;
                parents.Add(i);
            }
            else
            {
                event.Complete    = 0;
                event.TimingValid = 0;
                event.StatsValid  = 0;
            }
            if (event.TimingValid)
            {
                event.DurationMs = source.Time;
                validTimings++;
                if (event.ParentIndex >= 0)
                {
                    const ProfilerGpuEvent& parent = m_GpuHistoryEvents[record.FirstEvent + event.ParentIndex];
                    if (parent.TimingValid && parent.DurationMs > 0)
                    {
                        const float percent = source.Time / parent.DurationMs * 100.0f;
                        if (percent < Math::POS_INFINITY)
                            event.ParentPercent = percent;
                    }
                }
            }
            if (event.StatsValid)
            {
                event.DrawCalls            = source.Stats.DrawCalls;
                event.DispatchCalls        = source.Stats.DispatchCalls;
                event.Vertices             = source.Stats.Vertices;
                event.Triangles            = source.Stats.Triangles;
                event.PipelineStateChanges = source.Stats.PipelineStateChanges;
            }
            record.Info.IncompleteEvents += event.Complete == 0 ? 1 : 0;
        }
        record.TextLength = m_GpuHistoryText.Count() - record.FirstText;
        record.Info.State = record.Info.IncompleteEvents > 0 ? ProfilerGpuFrameState::Incomplete :
                            validTimings == 0                ? ProfilerGpuFrameState::TimingUnavailable :
                                                               ProfilerGpuFrameState::Resolved;
    }

    ProfilingSystem::CaptureResult ProfilingSystem::RefreshGpuView(uint64                  sessionId,
                                                                   uint64                  frameId,
                                                                   CaptureInfo&            capture,
                                                                   ProfilerGpuViewInfo&    info,
                                                                   List<ProfilerGpuFrame>& frames,
                                                                   List<ProfilerGpuEvent>& events,
                                                                   List<byte>&             text)
    {
        capture = CaptureInfo();
        info    = ProfilerGpuViewInfo();
        frames.Clear();
        events.Clear();
        text.Clear();
        if (!Threading::IsMainThread())
            return CaptureResult::WrongThread;
        Threading::ScopeLock lock(m_CaptureLocker);
        UpdateCaptureState();
        capture = m_CaptureStateData;
        if (capture.State == CaptureState::Unavailable)
            return CaptureResult::Unavailable;
        if (sessionId == 0 || sessionId != capture.SessionId)
            return CaptureResult::InvalidSession;
        if (capture.State != CaptureState::Capturing && capture.State != CaptureState::Stopping &&
            capture.State != CaptureState::Stopped)
            return CaptureResult::NotReady;
        int32 selected = -1;
        if (frameId != uint64(-1))
        {
            if (!m_GpuFrameIndices.TryGet(frameId, selected))
                return CaptureResult::InvalidFrame;
        }
        else
        {
            // Live prefers the newest usable result, not the newest perpetually pending frame.
            for (int32 i = m_GpuHistoryFrames.Count() - 1; i >= 0; i--)
            {
                const ProfilerGpuFrameState state = m_GpuHistoryFrames[i].Info.State;
                if (state == ProfilerGpuFrameState::Resolved || state == ProfilerGpuFrameState::Incomplete)
                {
                    selected = i;
                    break;
                }
            }
            if (selected < 0)
            {
                for (int32 i = m_GpuHistoryFrames.Count() - 1; i >= 0; i--)
                {
                    if (m_GpuHistoryFrames[i].Info.State != ProfilerGpuFrameState::Pending)
                    {
                        selected = i;
                        break;
                    }
                }
            }
            if (selected < 0 && m_GpuHistoryFrames.HasItems())
                selected = m_GpuHistoryFrames.Count() - 1;
        }
        for (const GpuHistoryFrame& record : m_GpuHistoryFrames)
            frames.Add(record.Info);
        if (selected >= 0)
        {
            const GpuHistoryFrame& record     = m_GpuHistoryFrames[selected];
            info.SelectedFrameId              = record.Info.FrameId;
            const ProfilerGpuFrameState state = record.Info.State;
            if (state == ProfilerGpuFrameState::Resolved || state == ProfilerGpuFrameState::Incomplete ||
                state == ProfilerGpuFrameState::TimingUnavailable)
            {
                if (record.TextLength > 0)
                {
                    text.Add(m_GpuHistoryText.Get() + record.FirstText, record.TextLength);
                    for (int32 i = 0; i < record.Info.EventCount; i++)
                    {
                        ProfilerGpuEvent event = m_GpuHistoryEvents[record.FirstEvent + i];
                        event.NameOffset -= record.FirstText;
                        events.Add(event);
                    }
                }
            }
        }
        info.SessionId     = sessionId;
        info.SelectedIndex = selected;
        info.SnapshotId    = ++m_FrameViewSequence;
        ResetView();
        m_CurrentGpuView   = info;
        m_CurrentGpuFrames = frames;
        m_CurrentGpuEvents = events;
        m_CurrentGpuText   = text;
        return CaptureResult::Ok;
    }

    ProfilerCaptureResult Profiling::RefreshGpuView(uint64                  sessionId,
                                                    uint64                  frameId,
                                                    ProfilerCaptureInfo&    capture,
                                                    ProfilerGpuViewInfo&    info,
                                                    List<ProfilerGpuFrame>& frames,
                                                    List<ProfilerGpuEvent>& events,
                                                    List<byte>&             text)
    {
        return Systems::GetSystem<ProfilingSystem>()->RefreshGpuView(
            sessionId, frameId, capture, info, frames, events, text);
    }

    void ProfilingSystem::ResetMemoryHistory()
    {
        m_MemoryStart      = 0;
        m_LastMemorySample = -1;
        m_ManagedMemory    = ProfilerMemorySample();
        m_MemoryHistory.Clear();
        m_MemoryHistory.SetCapacity(0);
    }

    void ProfilingSystem::SampleMemory()
    {
        if (m_CaptureStateData.State != CaptureState::Capturing)
            return;
        const double now = Platform::GetTimeSeconds() - m_MemoryStart;
        if (m_LastMemorySample >= 0 && now - m_LastMemorySample < 0.1)
            return;
        m_LastMemorySample = now;
        if (m_MemoryHistory.Count() == MaxMemorySamples)
        {
#ifdef SE_TRACY_SERVER
            if (m_CaptureWorker && m_CaptureWorker->IsConnected())
            {
                m_CaptureStateData.StopReason          = StopReason::MemoryHistoryCapacity;
                m_CaptureStateData.State               = CaptureState::Stopping;
                m_CaptureStateData.TailMayBeIncomplete = 1;
                m_CaptureWorker->Shutdown();
            }
#endif
            return;
        }
        ProfilerMemorySample sample = m_ManagedMemory;
        sample.TimeSeconds          = now;
        sample.FrameId              = Engine::FrameCount;
        // Each source has its own validity. PrivateUsage is private committed bytes, not virtual address-space usage.
        ProcessMemoryStats process = {};
        MemoryStats        system  = {};
        if (Platform::TryGetProcessMemoryStats(process))
        {
            sample.ValidMask |= 1;
            sample.ProcessPhysicalBytes = process.UsedPhysicalMemory;
            sample.ProcessPrivateBytes  = process.UsedVirtualMemory;
        }
        if (Platform::TryGetMemoryStats(system))
        {
            sample.ValidMask |= 2;
            sample.SystemTotalBytes = system.TotalPhysicalMemory;
            sample.SystemUsedBytes  = system.UsedPhysicalMemory;
        }
        if (GPUDevice::instance)
        {
            sample.ValidMask |= 4;
            sample.GpuTotalBytes = GPUDevice::instance->GetTotalGraphicsMemory();
            sample.GpuUsedBytes  = GPUDevice::instance->GetMemoryUsage();
        }
        m_MemoryHistory.Add(sample);
    }

    ProfilingSystem::CaptureResult ProfilingSystem::SubmitManagedMemory(uint64 sessionId,
                                                                        int64  heapBytes,
                                                                        int64  allocatedBytes,
                                                                        int32  gc0,
                                                                        int32  gc1,
                                                                        int32  gc2,
                                                                        double readSeconds)
    {
        if (!Threading::IsMainThread())
            return CaptureResult::WrongThread;
        Threading::ScopeLock lock(m_CaptureLocker);
        if (sessionId == 0 || sessionId != m_CaptureStateData.SessionId)
            return CaptureResult::InvalidSession;
        if (m_CaptureStateData.State != CaptureState::Capturing)
            return CaptureResult::NotReady;
        if (heapBytes < 0 || allocatedBytes < 0 || gc0 < 0 || gc1 < 0 || gc2 < 0 ||
            !(readSeconds >= 0 && readSeconds < Math::POS_INFINITY))
            return CaptureResult::InvalidRange;
        m_ManagedMemory                       = ProfilerMemorySample();
        m_ManagedMemory.ValidMask             = 8;
        m_ManagedMemory.ManagedHeapBytes      = heapBytes;
        m_ManagedMemory.ManagedAllocatedBytes = allocatedBytes;
        m_ManagedMemory.Gc0                   = gc0;
        m_ManagedMemory.Gc1                   = gc1;
        m_ManagedMemory.Gc2                   = gc2;
        m_ManagedMemory.ManagedReceiptSeconds = Platform::GetTimeSeconds() - m_MemoryStart;
        m_ManagedMemory.ManagedReadSeconds    = readSeconds;
        return CaptureResult::Ok;
    }

    ProfilingSystem::CaptureResult ProfilingSystem::RefreshMemoryOverview(uint64                      sessionId,
                                                                          int32                       first,
                                                                          ProfilerCaptureInfo&        capture,
                                                                          ProfilerMemoryOverviewInfo& info,
                                                                          List<ProfilerMemorySample>& samples)
    {
        capture = CaptureInfo();
        info    = ProfilerMemoryOverviewInfo();
        samples.Clear();
        if (!Threading::IsMainThread())
            return CaptureResult::WrongThread;
        Threading::ScopeLock lock(m_CaptureLocker);
        UpdateCaptureState();
        capture = m_CaptureStateData;
        if (sessionId == 0 || sessionId != capture.SessionId)
            return CaptureResult::InvalidSession;
        if (capture.State != CaptureState::Capturing && capture.State != CaptureState::Stopping &&
            capture.State != CaptureState::Stopped)
            return CaptureResult::NotReady;
        const int32 total = m_MemoryHistory.Count();
        if (first == -1)
            first = Math::Max(0, total - MaxAllocationPage);
        if (first < 0 || first > total)
            return CaptureResult::InvalidRange;
        info.SessionId       = sessionId;
        info.SnapshotId      = ++m_FrameViewSequence;
        info.TotalSamples    = total;
        info.First           = first;
        info.Count           = Math::Min(MaxAllocationPage, total - first);
        info.CapacityReached = total == MaxMemorySamples ? 1 : 0;
        for (int32 i = 0; i < info.Count; i++)
            samples.Add(m_MemoryHistory[first + i]);
        ResetView();
        m_CurrentMemoryOverview = info;
        m_CurrentMemorySamples  = samples;
        return CaptureResult::Ok;
    }

    ProfilingSystem::CaptureResult ProfilingSystem::RefreshAllocationView(uint64                      sessionId,
                                                                          int64                       beginNs,
                                                                          int64                       endNs,
                                                                          int64                       inspectNs,
                                                                          int32                       filter,
                                                                          uint64                      address,
                                                                          int32                       searchAddress,
                                                                          ProfilerCaptureInfo&        capture,
                                                                          ProfilerAllocationViewInfo& info,
                                                                          List<ProfilerMemoryBucket>& buckets,
                                                                          List<ProfilerAllocation>&   page)
    {
        capture = CaptureInfo();
        info    = ProfilerAllocationViewInfo();
        buckets.Clear();
        page.Clear();
        if (!Threading::IsMainThread())
            return CaptureResult::WrongThread;
        Threading::ScopeLock lock(m_CaptureLocker);
        UpdateCaptureState();
        capture = m_CaptureStateData;
#ifdef SE_TRACY_SERVER
        if (!m_CaptureWorker || sessionId == 0 || sessionId != capture.SessionId)
            return CaptureResult::InvalidSession;
        if (capture.State != CaptureState::Capturing && capture.State != CaptureState::Stopping &&
            capture.State != CaptureState::Stopped)
            return CaptureResult::NotReady;
        if (filter < 0 || filter > 2 || (searchAddress != 0 && searchAddress != 1))
            return CaptureResult::InvalidRange;
        List<ProfilerAllocation> records;
        records.EnsureCapacity(MaxAllocationRecords);
        if (!m_CaptureWorker->GetDataLock().try_lock())
            return CaptureResult::Busy;
        ProfilerAllocationViewInfo next;
        next.SessionId       = sessionId;
        next.RecordedUntilNs = m_CaptureWorker->GetLastTime();
        // -1 requests a bounded latest ten-second interval in Tracy's own clock.
        if (endNs == -1)
            endNs = next.RecordedUntilNs;
        if (beginNs == -1)
            beginNs = Math::Max(int64(0), endNs - 10000000000ll);
        if (inspectNs == -1)
            inspectNs = endNs;
        CaptureResult result = CaptureResult::Ok;
        if (beginNs < 0 || endNs <= beginNs || endNs > next.RecordedUntilNs || inspectNs < beginNs || inspectNs > endNs)
            result = CaptureResult::InvalidRange;
        int32 scanned = 0;
        // Reserve outside Tracy's lock. Copy immutable values only, with a hard scan and retained-record bound.
        for (const auto& pool : m_CaptureWorker->GetMemNameMap())
        {
            if (result != CaptureResult::Ok)
                break;
            const auto& data = pool.second->data;
            next.RecordedAllocations += data.size();
            for (size_t i = 0; i < data.size(); i++)
            {
                const auto& source = data[i];
                if (source.TimeAlloc() > endNs)
                    break;
                if (++scanned > MaxAllocationScan)
                {
                    result = CaptureResult::QueryTooLarge;
                    break;
                }
                const int64 freed = source.TimeFree();
                // Records freed before this interval cannot affect either the curve or inspection.
                if (freed >= 0 && freed < beginNs)
                    continue;
                if (records.Count() == MaxAllocationRecords)
                {
                    result = CaptureResult::QueryTooLarge;
                    break;
                }
                ProfilerAllocation& row = records.AddOne();
                row.PoolId              = pool.first;
                row.RecordId            = i;
                row.Address             = source.Ptr();
                row.Size                = source.Size();
                row.AllocNs             = source.TimeAlloc();
                row.FreeNs              = freed;
                row.AllocThread         = m_CaptureWorker->DecompressThread(source.ThreadAlloc());
                row.FreeThread          = freed >= 0 ? m_CaptureWorker->DecompressThread(source.ThreadFree()) : 0;
                row.CallstackId         = source.CsAlloc();
                row.ActiveAtInspection  = row.AllocNs <= inspectNs && (freed < 0 || freed > inspectNs) ? 1 : 0;
            }
        }
        m_CaptureWorker->GetDataLock().unlock();
        if (result != CaptureResult::Ok)
            return result;
        next.BeginNs     = beginNs;
        next.EndNs       = endNs;
        next.InspectNs   = inspectNs;
        next.Filter      = filter;
        next.FrozenCount = records.Count();
        List<AllocationOrder> order;
        List<MemoryChange>    changes;
        List<int32>           matching;
        order.EnsureCapacity(records.Count());
        changes.EnsureCapacity(records.Count() * 2);
        uint64 alive = 0;
        for (int32 i = 0; i < records.Count(); i++)
        {
            const auto&     row = records[i];
            AllocationOrder key;
            key.TimeNs   = row.AllocNs;
            key.PoolId   = row.PoolId;
            key.RecordId = row.RecordId;
            key.Index    = i;
            order.Add(key);
            const bool inRange = row.AllocNs >= beginNs && row.AllocNs < endNs;
            if (inRange)
            {
                next.RangeAllocations++;
                next.RangeAllocatedBytes += row.Size;
                MemoryChange change;
                change.TimeNs = row.AllocNs;
                change.Size   = row.Size;
                changes.Add(change);
            }
            else if (row.AllocNs < beginNs)
                alive += row.Size;
            if (row.FreeNs >= beginNs && row.FreeNs < endNs)
            {
                next.RangeFrees++;
                next.RangeFreedBytes += row.Size;
                MemoryChange change;
                change.TimeNs = row.FreeNs;
                change.Size   = row.Size;
                change.Free   = true;
                changes.Add(change);
            }
            if (row.ActiveAtInspection)
            {
                next.ActiveAllocations++;
                next.ActiveBytes += row.Size;
            }
        }
        // Engine MergeSort avoids the existing QuickSort's quadratic behaviour on already time-sorted histories.
        Sorting::MergeSort(order);
        for (const AllocationOrder& key : order)
        {
            const auto& row              = records[key.Index];
            const bool  allocatedInRange = row.AllocNs >= beginNs && row.AllocNs < endNs;
            const bool  overlaps =
                allocatedInRange || (row.AllocNs < beginNs && (row.FreeNs < 0 || row.FreeNs > beginNs));
            const bool match = filter == 0 ? overlaps : filter == 1 ? row.ActiveAtInspection != 0 : allocatedInRange;
            if (match && (!searchAddress || row.Address == address))
                matching.Add(key.Index);
        }
        Sorting::MergeSort(changes);
        const int32 bucketCount = static_cast<int32>(Math::Min(int64(MaxMemoryBuckets), endNs - beginNs));
        List<ProfilerMemoryBucket> curve;
        curve.Resize(bucketCount);
        int32 changeIndex = 0;
        for (int32 b = 0; b < bucketCount; b++)
        {
            auto& bucket = curve[b];
            // Quotient/remainder partition avoids multiplying a long capture duration by the bucket index.
            const int64 length = endNs - beginNs;
            bucket.BeginNs     = beginNs + (length / bucketCount) * b + (length % bucketCount) * b / bucketCount;
            bucket.EndNs = beginNs + (length / bucketCount) * (b + 1) + (length % bucketCount) * (b + 1) / bucketCount;
            bucket.MinBytes = bucket.MaxBytes = alive;
            while (changeIndex < changes.Count() && changes[changeIndex].TimeNs < bucket.EndNs)
            {
                const int64 time    = changes[changeIndex].TimeNs;
                uint64      added   = 0;
                uint64      removed = 0;
                do
                {
                    const auto& change = changes[changeIndex++];
                    if (change.Free)
                    {
                        removed += change.Size;
                        bucket.Frees++;
                    }
                    else
                    {
                        added += change.Size;
                        bucket.Allocations++;
                    }
                } while (changeIndex < changes.Count() && changes[changeIndex].TimeNs == time);
                alive += added;
                ENGINE_ASSERT(alive >= removed);
                alive -= removed;
                // The left-limit value at a bucket boundary lies outside this half-open bucket.
                if (time == bucket.BeginNs)
                    bucket.MinBytes = bucket.MaxBytes = alive;
                bucket.MinBytes = Math::Min(bucket.MinBytes, alive);
                bucket.MaxBytes = Math::Max(bucket.MaxBytes, alive);
            }
            bucket.EndBytes = alive;
        }
        next.MatchingCount = matching.Count();
        if (next.RecordedAllocations == 0)
            curve.Clear();
        next.SnapshotId = ++m_FrameViewSequence;
        for (int32 i = 0; i < Math::Min(MaxAllocationPage, matching.Count()); i++)
            page.Add(records[matching[i]]);
        buckets = curve;
        ResetView();
        m_CurrentAllocationView = next;
        m_CurrentAllocations.Swap(records);
        m_MatchingAllocations.Swap(matching);
        m_CurrentMemoryBuckets.Swap(curve);
        info = next;
        return CaptureResult::Ok;
#else
        return CaptureResult::Unavailable;
#endif
    }

    ProfilingSystem::CaptureResult ProfilingSystem::ReadAllocationPage(uint64                      sessionId,
                                                                       uint64                      snapshotId,
                                                                       int32                       first,
                                                                       ProfilerAllocationPageInfo& info,
                                                                       List<ProfilerAllocation>&   page)
    {
        info = ProfilerAllocationPageInfo();
        page.Clear();
        if (!Threading::IsMainThread())
            return CaptureResult::WrongThread;
        Threading::ScopeLock lock(m_CaptureLocker);
        if (sessionId == 0 || sessionId != m_CaptureStateData.SessionId)
            return CaptureResult::InvalidSession;
        if (snapshotId == 0 || snapshotId != m_CurrentAllocationView.SnapshotId)
            return CaptureResult::InvalidSnapshot;
        if (first < 0 || first > m_MatchingAllocations.Count())
            return CaptureResult::InvalidRange;
        info.SessionId  = sessionId;
        info.SnapshotId = snapshotId;
        info.First      = first;
        info.Total      = m_MatchingAllocations.Count();
        info.Count      = Math::Min(MaxAllocationPage, info.Total - first);
        info.HasMore    = first + info.Count < info.Total ? 1 : 0;
        for (int32 i = 0; i < info.Count; i++)
            page.Add(m_CurrentAllocations[m_MatchingAllocations[first + i]]);
        return CaptureResult::Ok;
    }

    ProfilingSystem::CaptureResult ProfilingSystem::ReadAllocationDetails(uint64              sessionId,
                                                                          uint64              snapshotId,
                                                                          uint64              poolId,
                                                                          uint64              recordId,
                                                                          ProfilerAllocation& allocation,
                                                                          List<ProfilerAllocationStackFrame>& frames,
                                                                          List<byte>&                         text)
    {
        allocation = ProfilerAllocation();
        frames.Clear();
        text.Clear();
        if (!Threading::IsMainThread())
            return CaptureResult::WrongThread;
        Threading::ScopeLock lock(m_CaptureLocker);
        if (sessionId == 0 || sessionId != m_CaptureStateData.SessionId)
            return CaptureResult::InvalidSession;
        if (snapshotId == 0 || snapshotId != m_CurrentAllocationView.SnapshotId)
            return CaptureResult::InvalidSnapshot;
#ifdef SE_TRACY_SERVER
        const ProfilerAllocation* selected = nullptr;
        for (const auto& row : m_CurrentAllocations)
        {
            if (row.PoolId == poolId && row.RecordId == recordId)
            {
                selected = &row;
                break;
            }
        }
        if (!selected)
            return CaptureResult::InvalidRange;
        ProfilerAllocation copy = *selected;
        if (copy.CallstackId == 0)
        {
            allocation = copy;
            return CaptureResult::Ok;
        }
        // Bounded output storage is allocated before the foreign lock, with no symbol queries that wait.
        frames.EnsureCapacity(256);
        text.EnsureCapacity(256 * 1024);
        text.Add(0);
        if (!m_CaptureWorker->GetDataLock().try_lock())
        {
            text.Clear();
            return CaptureResult::Busy;
        }
        CaptureResult result = CaptureResult::Ok;
        const auto&   stack  = m_CaptureWorker->GetCallstack(copy.CallstackId);
        for (const auto& id : stack)
        {
            if (frames.Count() == 256)
            {
                result = CaptureResult::QueryTooLarge;
                break;
            }
            const auto* symbols = id.custom == 0 ? m_CaptureWorker->GetCallstackFrame(id) : nullptr;
            const int32 count   = symbols ? Math::Max(1, int32(symbols->size)) : 1;
            if (count > 256 - frames.Count())
            {
                result = CaptureResult::QueryTooLarge;
                break;
            }
            for (int32 i = 0; i < count; i++)
            {
                ProfilerAllocationStackFrame row;
                row.Address     = id.sel == 0 ? m_CaptureWorker->GetCanonicalPointer(id) : 0;
                row.InlineDepth = i;
                if (symbols && symbols->size > 0)
                {
                    row.Resolved           = 1;
                    const auto& symbol     = symbols->data[i];
                    row.Line               = symbol.line;
                    const char* values[3]  = {m_CaptureWorker->GetString(symbol.name),
                                              m_CaptureWorker->GetString(symbol.file),
                                              m_CaptureWorker->GetString(symbols->imageName)};
                    int32*      offsets[3] = {&row.NameOffset, &row.FileOffset, &row.ImageOffset};
                    for (int32 n = 0; n < 3; n++)
                    {
                        const char* value = values[n];
                        if (!value || !*value)
                            continue;
                        int32 length = 0;
                        while (length < 256 * 1024 && value[length])
                            length++;
                        if (length >= 256 * 1024 - text.Count())
                        {
                            result = CaptureResult::QueryTooLarge;
                            break;
                        }
                        *offsets[n] = text.Count();
                        text.Add(reinterpret_cast<const byte*>(value), length + 1);
                    }
                    if (result != CaptureResult::Ok)
                        break;
                }
                frames.Add(row);
            }
            if (result != CaptureResult::Ok)
                break;
        }
        m_CaptureWorker->GetDataLock().unlock();
        if (result != CaptureResult::Ok)
        {
            frames.Clear();
            text.Clear();
            return result;
        }
        allocation = copy;
        return CaptureResult::Ok;
#else
        return CaptureResult::Unavailable;
#endif
    }

    ProfilerCaptureResult Profiling::SubmitManagedMemory(uint64 sessionId,
                                                         int64  heapBytes,
                                                         int64  allocatedBytes,
                                                         int32  gc0,
                                                         int32  gc1,
                                                         int32  gc2,
                                                         double readSeconds)
    {
        return Systems::GetSystem<ProfilingSystem>()->SubmitManagedMemory(
            sessionId, heapBytes, allocatedBytes, gc0, gc1, gc2, readSeconds);
    }

    ProfilerCaptureResult Profiling::RefreshMemoryOverview(uint64                      sessionId,
                                                           int32                       first,
                                                           ProfilerCaptureInfo&        capture,
                                                           ProfilerMemoryOverviewInfo& info,
                                                           List<ProfilerMemorySample>& samples)
    {
        return Systems::GetSystem<ProfilingSystem>()->RefreshMemoryOverview(sessionId, first, capture, info, samples);
    }

    ProfilerCaptureResult Profiling::RefreshAllocationView(uint64                      sessionId,
                                                           int64                       beginNs,
                                                           int64                       endNs,
                                                           int64                       inspectNs,
                                                           int32                       filter,
                                                           uint64                      address,
                                                           int32                       searchAddress,
                                                           ProfilerCaptureInfo&        capture,
                                                           ProfilerAllocationViewInfo& info,
                                                           List<ProfilerMemoryBucket>& buckets,
                                                           List<ProfilerAllocation>&   page)
    {
        return Systems::GetSystem<ProfilingSystem>()->RefreshAllocationView(
            sessionId, beginNs, endNs, inspectNs, filter, address, searchAddress, capture, info, buckets, page);
    }

    ProfilerCaptureResult Profiling::ReadAllocationPage(uint64                      sessionId,
                                                        uint64                      snapshotId,
                                                        int32                       first,
                                                        ProfilerAllocationPageInfo& info,
                                                        List<ProfilerAllocation>&   page)
    {
        return Systems::GetSystem<ProfilingSystem>()->ReadAllocationPage(sessionId, snapshotId, first, info, page);
    }

    ProfilerCaptureResult Profiling::ReadAllocationDetails(uint64                              sessionId,
                                                           uint64                              snapshotId,
                                                           uint64                              poolId,
                                                           uint64                              recordId,
                                                           ProfilerAllocation&                 allocation,
                                                           List<ProfilerAllocationStackFrame>& frames,
                                                           List<byte>&                         text)
    {
        return Systems::GetSystem<ProfilingSystem>()->ReadAllocationDetails(
            sessionId, snapshotId, poolId, recordId, allocation, frames, text);
    }

    void ProfilingSystem::ResetMemoryView()
    {
        m_CurrentMemoryOverview = ProfilerMemoryOverviewInfo();
        m_CurrentMemorySamples.Clear();
        m_CurrentMemorySamples.SetCapacity(0);
        m_CurrentAllocationView = ProfilerAllocationViewInfo();
        m_CurrentAllocations.Clear();
        m_CurrentAllocations.SetCapacity(0);
        m_MatchingAllocations.Clear();
        m_MatchingAllocations.SetCapacity(0);
        m_CurrentMemoryBuckets.Clear();
        m_CurrentMemoryBuckets.SetCapacity(0);
    }

    void ProfilingSystem::ResetView()
    {
        ResetMemoryView();
        m_CurrentGpuView = ProfilerGpuViewInfo();
        m_CurrentGpuFrames.Clear();
        m_CurrentGpuFrames.SetCapacity(0);
        m_CurrentGpuEvents.Clear();
        m_CurrentGpuEvents.SetCapacity(0);
        m_CurrentGpuText.Clear();
        m_CurrentGpuText.SetCapacity(0);
        m_CurrentCpuStatistics.Clear();
        m_CurrentCpuStatistics.SetCapacity(0);
        m_PendingCpuStatistics.Clear();
        m_PendingCpuStatistics.SetCapacity(0);
        m_SourceStatisticIndices.Clear();
        m_SourceStatisticIndices.SetCapacity(0);
        m_CpuChildCoverage.Clear();
        m_CpuChildCoverage.SetCapacity(0);
        m_CurrentFrameView = FrameViewInfo();
        m_CurrentCpuThreads.Clear();
        m_CurrentCpuThreads.SetCapacity(0);
        m_CurrentCpuEvents.Clear();
        m_CurrentCpuEvents.SetCapacity(0);
        m_CurrentCpuText.Clear();
        m_CurrentCpuText.SetCapacity(0);
        m_PendingCpuThreads.Clear();
        m_PendingCpuThreads.SetCapacity(0);
        m_PendingCpuEvents.Clear();
        m_PendingCpuEvents.SetCapacity(0);
        m_PendingCpuText.Clear();
        m_PendingCpuText.SetCapacity(0);
    }

    ProfilingSystem::CaptureResult ProfilingSystem::BeginCaptureCleanup(CaptureState targetState)
    {
#ifdef SE_TRACY_SERVER
        m_CleanupTarget = targetState;
        ResetView();
        ResetGpuHistory();
        m_CaptureStateData.State = CaptureState::Clearing;
        if (m_CaptureWorker)
        {
            // Shutdown is atomic; Disconnect also writes a non-atomic diagnostic flag.
            m_CaptureWorker->Shutdown();
            try
            {
                tracy::Worker*        worker = m_CaptureWorker.get();
                Function<void(int32)> cleanup([worker](int32) { Delete(worker); });
                m_CaptureCleanup = Threading::JobSystem::Dispatch(cleanup);
                // Dispatch owns destruction, which may complete before this holder is released.
                m_CaptureWorker.release();
            }
            catch (...)
            {
                m_CaptureStateData.State = CaptureState::Failed;
                m_CaptureStateData.Error = CaptureResult::InternalError;
                return CaptureResult::InternalError;
            }
        }
        return CaptureResult::Ok;
#else
        return CaptureResult::Unavailable;
#endif
    }

    void ProfilingSystem::UpdateCaptureState()
    {
#ifdef SE_TRACY_SERVER
        if (m_CaptureCleanup != 0)
        {
            if (Threading::JobSystem::IsCompleted(m_CaptureCleanup))
            {
                m_CaptureCleanup          = 0;
                const CaptureResult error = m_CaptureStateData.Error;
                m_CaptureStateData        = CaptureInfo();
                m_CaptureStateData.State  = m_CleanupTarget;
                m_CaptureStateData.Error  = m_CleanupTarget == CaptureState::Failed ? error : CaptureResult::Ok;
            }
            return;
        }
        if (!m_CaptureWorker)
        {
            return;
        }

        m_CaptureStateData.ServerMemoryBytes = tracy::memUsage.load();

        if (m_CaptureStateData.State == CaptureState::Connecting)
        {
            const uint8 handshake = m_CaptureWorker->GetHandshakeStatus();
            if (handshake == tracy::HandshakeProtocolMismatch || handshake == tracy::HandshakeNotAvailable ||
                handshake == tracy::HandshakeDropped)
            {
                m_CaptureStateData.Error =
                    handshake == tracy::HandshakeProtocolMismatch ? CaptureResult::ProtocolMismatch :
                    handshake == tracy::HandshakeNotAvailable     ? CaptureResult::ClientUnavailable :
                                                                    CaptureResult::ConnectionFailed;
                BeginCaptureCleanup(CaptureState::Failed);
                return;
            }
            if (m_CaptureWorker->HasData() && m_CaptureWorker->IsConnected())
            {
                // HasData publishes the welcome data. History is not exposed before identity validation.
                if (m_CaptureWorker->GetPid() != Platform::GetCurrentProcessId())
                {
                    m_CaptureStateData.Error = CaptureResult::WrongProcess;
                    BeginCaptureCleanup(CaptureState::Failed);
                    return;
                }
                if (!m_CaptureWorker->IsOnDemand())
                {
                    m_CaptureStateData.Error = CaptureResult::ProtocolMismatch;
                    BeginCaptureCleanup(CaptureState::Failed);
                    return;
                }
                m_CaptureStateData.State = CaptureState::Capturing;
                m_FirstGpuFrame          = Engine::FrameCount + 1;
                m_MemoryStart            = Platform::GetTimeSeconds();
            }
            else if (Platform::GetTimeSeconds() - m_CaptureConnectionStart > 10.0)
            {
                m_CaptureStateData.Error = CaptureResult::TimedOut;
                BeginCaptureCleanup(CaptureState::Failed);
                return;
            }
        }

        if (m_CaptureStateData.State != CaptureState::Capturing && m_CaptureStateData.State != CaptureState::Stopping &&
            m_CaptureStateData.State != CaptureState::Stopped)
        {
            return;
        }

        // Do not depend on a successful data-lock sample to enforce the soft threshold.
        // A disconnected receiver has no confirmed capacity cause. Latch only our own stop request.
        if (m_CaptureStateData.State == CaptureState::Capturing && m_CaptureWorker->IsConnected() &&
            m_CaptureStateData.ServerMemoryBytes > m_CaptureStateData.MemoryLimitBytes)
        {
            m_CaptureStateData.StopReason          = StopReason::CapacityLimit;
            m_CaptureStateData.CapacityStopBytes   = m_CaptureStateData.ServerMemoryBytes;
            m_CaptureStateData.State               = CaptureState::Stopping;
            m_CaptureStateData.TailMayBeIncomplete = 1;
            m_CaptureWorker->Shutdown();
        }

        // The engine lock protects our state. Tracy writers require their own synchronization.
        // This foreign-lock region is allocation-free and has a single exit.
        if (!m_CaptureWorker->GetDataLock().try_lock())
        {
            return;
        }

        // For this live Worker, IsConnected becomes false only after its processing path closes.
        // We reached Capturing by observing true, so the initial false cannot look like a completed stop.
        if (!m_CaptureWorker->IsConnected() && m_CaptureStateData.State != CaptureState::Stopped)
        {
            if (m_CaptureStateData.StopReason == StopReason::None)
                m_CaptureStateData.StopReason = StopReason::ReceiverEnded;
            m_CaptureStateData.State               = CaptureState::Stopped;
            m_CaptureStateData.TailMayBeIncomplete = 1;
        }
        const tracy::FrameData* frames = m_CaptureWorker->GetFramesBase();
        if (frames)
        {
            // Two startup markers and an on-demand boundary precede the first real FrameMark.
            // Skip those intervals and the last, still-open frame even after stopping.
            const size_t count                 = m_CaptureWorker->GetFrameCount(*frames);
            m_CaptureStateData.CompleteFrames  = count > 4 ? count - 4 : 0;
            m_CaptureStateData.CompleteBeginNs = count > 4 ? m_CaptureWorker->GetFrameBegin(*frames, 3) : 0;
            m_CaptureStateData.CompleteEndNs   = count > 4 ? m_CaptureWorker->GetFrameBegin(*frames, count - 1) : 0;
        }
        m_CaptureStateData.LastTimeNs = m_CaptureWorker->GetLastTime();
        if (m_CaptureWorker->GetFailureType() != tracy::Worker::Failure::None)
            m_CaptureStateData.Error = CaptureResult::InternalError;
        m_CaptureWorker->GetDataLock().unlock();
#endif
    }

    ProfilingSystem::CaptureResult ProfilingSystem::StartCapture()
    {
#ifdef SE_TRACY_SERVER
        if (!Threading::IsMainThread())
        {
            return CaptureResult::WrongThread;
        }
        Threading::ScopeLock lock(m_CaptureLocker);
        if (!m_CaptureInitialized)
        {
            return CaptureResult::NotReady;
        }
        UpdateCaptureState();
        if (m_CaptureStateData.State != CaptureState::Idle)
        {
            return CaptureResult::Busy;
        }
        try
        {
            m_CaptureStateData = CaptureInfo();
            ResetView();
            ResetGpuHistory();
            m_CaptureStateData.SessionId        = ++m_CaptureSequence;
            m_CaptureStateData.ProcessId        = Platform::GetCurrentProcessId();
            m_CaptureConnectionStart            = Platform::GetTimeSeconds();
            m_CaptureStateData.MemoryLimitBytes = CaptureMemoryLimitBytes;
            // The engine latches its stop reason before Shutdown; disable Tracy's duplicate cutoff.
            m_CaptureWorker          = CreateScope<tracy::Worker>("127.0.0.1", 8086, -1);
            m_CaptureStateData.State = CaptureState::Connecting;
            ProfilerCPU::Enabled     = true;
            return CaptureResult::Ok;
        }
        catch (...)
        {
            m_CaptureStateData.State = CaptureState::Failed;
            m_CaptureStateData.Error = CaptureResult::InternalError;
            return CaptureResult::InternalError;
        }
#else
        return CaptureResult::Unavailable;
#endif
    }

    ProfilingSystem::CaptureResult ProfilingSystem::StopCapture()
    {
#ifdef SE_TRACY_SERVER
        if (!Threading::IsMainThread())
        {
            return CaptureResult::WrongThread;
        }
        Threading::ScopeLock lock(m_CaptureLocker);

        // A not-yet-published connection is cancelled, never mistaken for retained history.
        if (m_CaptureStateData.State == CaptureState::Connecting)
        {
            return BeginCaptureCleanup(CaptureState::Idle);
        }

        UpdateCaptureState();
        if (m_CaptureStateData.State == CaptureState::Stopping || m_CaptureStateData.State == CaptureState::Stopped)
        {
            return CaptureResult::Ok;
        }

        if (m_CaptureStateData.State != CaptureState::Capturing || !m_CaptureWorker)
        {
            return CaptureResult::NotReady;
        }

        m_CaptureStateData.State = CaptureState::Stopping;
        // A busy data lock may have deferred observing an already-ended receiver above.
        m_CaptureStateData.StopReason = m_CaptureWorker->IsConnected() ? StopReason::User : StopReason::ReceiverEnded;
        m_CaptureStateData.TailMayBeIncomplete = 1;
        m_CaptureWorker->Shutdown();
        ProfilerCPU::Enabled = false;
        return CaptureResult::Ok;
#else
        return CaptureResult::Unavailable;
#endif
    }

    ProfilingSystem::CaptureResult ProfilingSystem::ClearCapture()
    {
#ifdef SE_TRACY_SERVER
        if (!Threading::IsMainThread())
        {
            return CaptureResult::WrongThread;
        }
        Threading::ScopeLock lock(m_CaptureLocker);
        UpdateCaptureState();

        if (m_CaptureStateData.State == CaptureState::Idle)
        {
            return CaptureResult::Ok;
        }
        if (m_CaptureStateData.State != CaptureState::Stopped && m_CaptureStateData.State != CaptureState::Failed)
        {
            return CaptureResult::Busy;
        }

        if (m_CaptureWorker)
        {
            return BeginCaptureCleanup(CaptureState::Idle);
        }
        else
        {
            ResetView();
            ResetGpuHistory();
            m_CaptureStateData       = CaptureInfo();
            m_CaptureStateData.State = CaptureState::Idle;
        }
        return CaptureResult::Ok;
#else
        return CaptureResult::Unavailable;
#endif
    }

    ProfilingSystem::CaptureResult ProfilingSystem::GetCaptureInfo(CaptureInfo& info)
    {
        if (!Threading::IsMainThread())
        {
            info = CaptureInfo();
            return CaptureResult::WrongThread;
        }
        Threading::ScopeLock lock(m_CaptureLocker);
        UpdateCaptureState();
        info = m_CaptureStateData;
        return CaptureResult::Ok;
    }

    ProfilingSystem::CaptureResult
    ProfilingSystem::ReadCaptureFrame(uint64 sessionId, uint64 index, CaptureFrame& frame)
    {
        frame = CaptureFrame();
#ifdef SE_TRACY_SERVER
        if (!Threading::IsMainThread())
            return CaptureResult::WrongThread;
        Threading::ScopeLock lock(m_CaptureLocker);
        if (!m_CaptureWorker || sessionId == 0 || sessionId != m_CaptureStateData.SessionId)
            return CaptureResult::InvalidSession;
        if (m_CaptureStateData.State != CaptureState::Capturing && m_CaptureStateData.State != CaptureState::Stopping &&
            m_CaptureStateData.State != CaptureState::Stopped)
            return CaptureResult::NotReady;
        if (!m_CaptureWorker->GetDataLock().try_lock())
            return CaptureResult::Busy;
        const tracy::FrameData* frames = m_CaptureWorker->GetFramesBase();
        const size_t            count  = frames ? m_CaptureWorker->GetFrameCount(*frames) : 0;
        CaptureResult           result = CaptureResult::InvalidFrame;
        if (count > 4 && index < count - 4)
        {
            const size_t nativeIndex = static_cast<size_t>(index) + 3;
            frame.Index              = index;
            frame.BeginNs            = m_CaptureWorker->GetFrameBegin(*frames, nativeIndex);
            frame.EndNs              = m_CaptureWorker->GetFrameBegin(*frames, nativeIndex + 1);
            result                   = CaptureResult::Ok;
        }
        m_CaptureWorker->GetDataLock().unlock();
        return result;
#else
        return CaptureResult::Unavailable;
#endif
    }

    int32 ProfilingSystem::CopyCpuText(const char* pText)
    {
        if (!pText || !*pText)
            return 0;
        const int32 offset    = m_PendingCpuText.Count();
        const int32 available = MaxCpuTextBytes - offset;
        int32       length    = 0;
        while (length < available && pText[length])
            length++;
        if (length == available)
            return -1;
        m_PendingCpuText.Add(reinterpret_cast<const byte*>(pText), length + 1);
        return offset;
    }

    ProfilingSystem::CaptureResult
    ProfilingSystem::CopyCpuView(int64 beginNs, int64 endNs, uint64 threadId, int32& incomplete)
    {
#ifdef SE_TRACY_SERVER
        // Live vectors hold short_ptr values. Tracy also supports direct compacted vectors;
        // interpret those only here, following its own timeline traversal, never across the API.
        auto copyLevel = [&](auto&& self, const auto& zones, int32 depth, int32 parent) -> CaptureResult {
            auto zoneAt = [&zones](size_t index) -> const tracy::ZoneEvent& {
                return zones.is_magic() ? reinterpret_cast<const tracy::ZoneEvent*>(zones.data())[index] :
                                          *zones[index];
            };
            size_t low  = 0;
            size_t high = zones.size();
            // Siblings are ordered; an open final sibling must not be skipped using an inferred end.
            while (low < high)
            {
                const size_t            middle = low + (high - low) / 2;
                const tracy::ZoneEvent& zone   = zoneAt(middle);
                if (zone.IsEndValid() && zone.End() < beginNs)
                    low = middle + 1;
                else
                    high = middle;
            }
            for (size_t i = low; i < zones.size(); i++)
            {
                const tracy::ZoneEvent& zone = zoneAt(i);
                if (zone.Start() >= endNs)
                    break;
                if (zone.IsEndValid() && zone.End() <= beginNs && zone.Start() != zone.End())
                    continue;
                if (depth >= MaxCpuDepth || m_PendingCpuEvents.Count() >= MaxCpuEvents)
                    return CaptureResult::QueryTooLarge;
                if (zone.IsEndValid() && zone.End() < zone.Start())
                    return CaptureResult::InternalError;
                const int32       index             = m_PendingCpuEvents.Count();
                ProfilerCpuEvent& event             = m_PendingCpuEvents.AddOne();
                event.BeginNs                       = zone.Start();
                event.EndValid                      = zone.IsEndValid() ? 1 : 0;
                event.EndNs                         = event.EndValid ? zone.End() : -1;
                event.ParentIndex                   = parent;
                event.Depth                         = depth;
                event.SourceLocationId              = zone.SrcLoc();
                const tracy::SourceLocation& source = m_CaptureWorker->GetSourceLocation(zone.SrcLoc());
                event.Line                          = source.line;
                event.NameOffset                    = CopyCpuText(m_CaptureWorker->GetZoneName(zone, source));
                event.FunctionOffset                = CopyCpuText(m_CaptureWorker->GetString(source.function));
                event.FileOffset                    = CopyCpuText(m_CaptureWorker->GetString(source.file));
                if (event.NameOffset < 0 || event.FunctionOffset < 0 || event.FileOffset < 0)
                    return CaptureResult::QueryTooLarge;
                int32& statisticIndex = m_SourceStatisticIndices[static_cast<uint16>(zone.SrcLoc())];
                if (statisticIndex < 0)
                {
                    statisticIndex                  = m_PendingCpuStatistics.Count();
                    ProfilerCpuStatistic& statistic = m_PendingCpuStatistics.AddOne();
                    statistic.SourceLocationId      = event.SourceLocationId;
                    statistic.NameOffset            = CopyCpuText(m_CaptureWorker->GetZoneName(source));
                    statistic.FunctionOffset        = event.FunctionOffset;
                    statistic.FileOffset            = event.FileOffset;
                    statistic.Line                  = event.Line;
                    if (statistic.NameOffset < 0)
                        return CaptureResult::QueryTooLarge;
                }
                incomplete += event.EndValid == 0 ? 1 : 0;
                ProfilerCpuThread& thread = m_PendingCpuThreads.Last();
                thread.MaxDepth           = Math::Max(thread.MaxDepth, depth + 1);
                if (zone.HasChildren())
                {
                    const CaptureResult result =
                        self(self, m_CaptureWorker->GetZoneChildren(zone.Child()), depth + 1, index);
                    if (result != CaptureResult::Ok)
                        return result;
                }
            }
            return CaptureResult::Ok;
        };

        const auto& threads = m_CaptureWorker->GetThreadData();
        if (threadId == 0 && threads.size() > MaxCpuThreads)
            return CaptureResult::QueryTooLarge;
        bool found = threadId == 0;
        for (const tracy::ThreadData* source : threads)
        {
            if (threadId != 0 && threadId != source->id)
                continue;
            ProfilerCpuThread& thread = m_PendingCpuThreads.AddOne();
            thread.Id                 = source->id;
            thread.NameOffset         = CopyCpuText(m_CaptureWorker->GetThreadName(source->id));
            if (thread.NameOffset < 0)
                return CaptureResult::QueryTooLarge;
            thread.FirstEvent          = m_PendingCpuEvents.Count();
            found                      = true;
            const CaptureResult result = copyLevel(copyLevel, source->timeline, 0, -1);
            if (result != CaptureResult::Ok)
                return result;
            thread.EventCount = m_PendingCpuEvents.Count() - thread.FirstEvent;
        }
        return found ? CaptureResult::Ok : CaptureResult::InvalidRange;
#else
        return CaptureResult::Unavailable;
#endif
    }

    void ProfilingSystem::BuildCpuStatistics(int64 beginNs, int64 endNs)
    {
        // Work only on owned frozen values, after releasing Tracy's writer lock.
        const int32 count = m_PendingCpuEvents.Count();
        m_CpuChildCoverage.Resize(count);
        for (int32 i = 0; i < count; i++)
        {
            m_CpuChildCoverage[i]       = CpuChildCoverage();
            m_CpuChildCoverage[i].EndNs = Math::Max(beginNs, m_PendingCpuEvents[i].BeginNs);
        }
        for (int32 i = 0; i < count; i++)
        {
            const ProfilerCpuEvent& event = m_PendingCpuEvents[i];
            if (event.ParentIndex < 0)
                continue;
            const ProfilerCpuEvent& parent    = m_PendingCpuEvents[event.ParentIndex];
            CpuChildCoverage&       coverage  = m_CpuChildCoverage[event.ParentIndex];
            const int64             begin     = Math::Max(beginNs, Math::Max(parent.BeginNs, event.BeginNs));
            const int64             parentEnd = parent.EndValid ? Math::Min(endNs, parent.EndNs) : endNs;
            if (!event.EndValid)
            {
                // A missing child end cannot be treated as zero when reporting the parent's self time.
                if (begin < parentEnd)
                    coverage.Incomplete = true;
                continue;
            }
            const int64 end = Math::Min(parentEnd, event.EndNs);
            // Direct siblings are visited in start order. Extend their union, never subtract descendants twice.
            const int64 uncoveredBegin = Math::Max(begin, coverage.EndNs);
            if (end > uncoveredBegin)
                coverage.CoveredNs += static_cast<uint64>(end - uncoveredBegin);
            coverage.EndNs = Math::Max(coverage.EndNs, end);
        }
        for (int32 i = 0; i < count; i++)
        {
            const ProfilerCpuEvent& event = m_PendingCpuEvents[i];
            ProfilerCpuStatistic&   row =
                m_PendingCpuStatistics[m_SourceStatisticIndices[static_cast<uint16>(event.SourceLocationId)]];
            if (!event.EndValid)
            {
                row.IncompleteCalls++;
                continue;
            }
            const int64  begin    = Math::Max(beginNs, event.BeginNs);
            const int64  end      = Math::Min(endNs, event.EndNs);
            const uint64 duration = end > begin ? static_cast<uint64>(end - begin) : 0;
            row.Calls++;
            row.TotalNs += duration;
            if (row.RepresentativeEvent < 0 || duration > row.MaxNs)
            {
                row.MaxNs               = duration;
                row.RepresentativeEvent = i;
            }
            if (m_CpuChildCoverage[i].Incomplete)
                row.SelfIncompleteCalls++;
            else
                row.SelfNs += duration - m_CpuChildCoverage[i].CoveredNs;
        }
        for (ProfilerCpuStatistic& row : m_PendingCpuStatistics)
            row.AverageNs = row.Calls != 0 ? static_cast<double>(row.TotalNs) / row.Calls : 0;
    }

    ProfilingSystem::CaptureResult ProfilingSystem::RefreshFrameView(uint64                      sessionId,
                                                                     uint64                      firstFrame,
                                                                     int32                       count,
                                                                     uint64                      selectionFirst,
                                                                     int32                       selectionCount,
                                                                     uint64                      threadId,
                                                                     CaptureInfo&                capture,
                                                                     FrameViewInfo&              info,
                                                                     List<CaptureFrame>&         viewFrames,
                                                                     List<ProfilerCpuThread>&    cpuThreads,
                                                                     List<ProfilerCpuEvent>&     cpuEvents,
                                                                     List<byte>&                 cpuText,
                                                                     List<ProfilerCpuStatistic>& statistics)
    {
        capture = CaptureInfo();
        info    = FrameViewInfo();
        viewFrames.Clear();
        cpuThreads.Clear();
        cpuEvents.Clear();
        cpuText.Clear();
        statistics.Clear();
#ifdef SE_TRACY_SERVER
        if (!Threading::IsMainThread())
        {
            return CaptureResult::WrongThread;
        }
        Threading::ScopeLock lock(m_CaptureLocker);
        UpdateCaptureState();
        capture = m_CaptureStateData;
        if (count < 1 || selectionCount < 0 || selectionCount > count)
        {
            return CaptureResult::InvalidRange;
        }
        if (count > MaxFrameViewFrames)
        {
            return CaptureResult::QueryTooLarge;
        }
        if (!m_CaptureWorker || sessionId == 0 || sessionId != m_CaptureStateData.SessionId)
        {
            return CaptureResult::InvalidSession;
        }
        if (m_CaptureStateData.State != CaptureState::Capturing && m_CaptureStateData.State != CaptureState::Stopping &&
            m_CaptureStateData.State != CaptureState::Stopped)
        {
            return CaptureResult::NotReady;
        }
        // Reserve bounded staging storage outside the foreign lock. Failed queries leave published values intact.
        m_PendingCpuThreads.Clear();
        m_PendingCpuEvents.Clear();
        m_PendingCpuText.Clear();
        m_PendingCpuThreads.EnsureCapacity(MaxCpuThreads);
        m_PendingCpuEvents.EnsureCapacity(MaxCpuEvents);
        m_PendingCpuText.EnsureCapacity(MaxCpuTextBytes);
        m_PendingCpuText.Add(0);
        m_PendingCpuStatistics.Clear();
        m_PendingCpuStatistics.EnsureCapacity(MaxCpuEvents);
        // Source IDs are signed 16-bit values owned by this Worker; the unsigned bits index all identities.
        m_SourceStatisticIndices.Resize(65536);
        for (int32& index : m_SourceStatisticIndices)
            index = -1;
        CaptureFrame pendingFrames[MaxFrameViewFrames];
        if (!m_CaptureWorker->GetDataLock().try_lock())
        {
            return CaptureResult::Busy;
        }

        // Bounded, allocation-free copy while holding Tracy's own writer lock. Publish only after validation.
        const tracy::FrameData* frames   = m_CaptureWorker->GetFramesBase();
        const size_t            rawCount = frames ? m_CaptureWorker->GetFrameCount(*frames) : 0;
        FrameViewInfo           next;
        next.SessionId      = sessionId;
        next.FilterThreadId = threadId;
        next.TotalFrames    = rawCount > 4 ? rawCount - 4 : 0;
        next.FirstFrame =
            firstFrame == uint64(-1) ? (next.TotalFrames > uint64(count) ? next.TotalFrames - count : 0) : firstFrame;
        next.HasPartialFrame     = rawCount > 3 ? 1 : 0;
        next.TailMayBeIncomplete = m_CaptureStateData.TailMayBeIncomplete;
        CaptureResult result     = CaptureResult::Ok;
        if ((next.TotalFrames == 0 && (next.FirstFrame != 0 || selectionCount != 0)) ||
            (next.TotalFrames != 0 && next.FirstFrame >= next.TotalFrames))
        {
            result = CaptureResult::InvalidRange;
        }
        else
        {
            const uint64 remaining = next.TotalFrames - next.FirstFrame;
            next.FrameCount        = remaining < uint64(count) ? static_cast<int32>(remaining) : count;
            next.SelectionFirst =
                selectionCount == 0 && next.FrameCount > 0 ? next.FirstFrame + next.FrameCount - 1 : selectionFirst;
            next.SelectionCount = selectionCount == 0 ? (next.FrameCount > 0 ? 1 : 0) : selectionCount;
            if (next.FrameCount == 0)
            {
                next.SelectionFirst = 0;
            }
            if (next.SelectionCount != 0 &&
                (next.SelectionFirst < next.FirstFrame ||
                 next.SelectionFirst - next.FirstFrame >= uint64(next.FrameCount) ||
                 next.SelectionCount > uint64(next.FrameCount) - (next.SelectionFirst - next.FirstFrame)))
            {
                result = CaptureResult::InvalidRange;
            }
        }
        if (result == CaptureResult::Ok)
        {
            for (int32 i = 0; i < next.FrameCount; i++)
            {
                CaptureFrame& frame      = pendingFrames[i];
                frame.Index              = next.FirstFrame + i;
                const size_t nativeIndex = static_cast<size_t>(frame.Index) + 3;
                frame.BeginNs            = m_CaptureWorker->GetFrameBegin(*frames, nativeIndex);
                frame.EndNs              = m_CaptureWorker->GetFrameBegin(*frames, nativeIndex + 1);
            }
        }
        if (result == CaptureResult::Ok && next.FrameCount > 0)
        {
            next.BeginNs          = pendingFrames[0].BeginNs;
            next.EndNs            = pendingFrames[next.FrameCount - 1].EndNs;
            const int32 selected  = static_cast<int32>(next.SelectionFirst - next.FirstFrame);
            next.SelectionBeginNs = pendingFrames[selected].BeginNs;
            next.SelectionEndNs   = pendingFrames[selected + static_cast<int32>(next.SelectionCount) - 1].EndNs;
            result = CopyCpuView(next.SelectionBeginNs, next.SelectionEndNs, threadId, next.IncompleteCpuEvents);
        }
        m_CaptureWorker->GetDataLock().unlock();
        if (result != CaptureResult::Ok)
        {
            return result;
        }

        BuildCpuStatistics(next.SelectionBeginNs, next.SelectionEndNs);
        ResetMemoryView();
        m_CurrentGpuView = ProfilerGpuViewInfo();
        m_CurrentGpuFrames.Clear();
        m_CurrentGpuEvents.Clear();
        m_CurrentGpuText.Clear();
        next.SnapshotId    = ++m_FrameViewSequence;
        m_CurrentFrameView = next;
        m_CurrentCpuThreads.Swap(m_PendingCpuThreads);
        m_CurrentCpuEvents.Swap(m_PendingCpuEvents);
        m_CurrentCpuText.Swap(m_PendingCpuText);
        m_CurrentCpuStatistics.Swap(m_PendingCpuStatistics);
        viewFrames.Resize(next.FrameCount);
        for (int32 i = 0; i < next.FrameCount; i++)
        {
            m_CurrentViewFrames[i] = pendingFrames[i];
            viewFrames[i]          = pendingFrames[i];
        }
        cpuThreads = m_CurrentCpuThreads;
        cpuEvents  = m_CurrentCpuEvents;
        cpuText    = m_CurrentCpuText;
        statistics = m_CurrentCpuStatistics;
        info       = next;
        return CaptureResult::Ok;
#else
        return CaptureResult::Unavailable;
#endif
    }

    ProfilingSystem::CaptureResult ProfilingSystem::ReadFrameView(uint64              sessionId,
                                                                  uint64              snapshotId,
                                                                  FrameViewInfo&      info,
                                                                  List<CaptureFrame>& viewFrames)
    {
        info = FrameViewInfo();
        viewFrames.Clear();
#ifdef SE_TRACY_SERVER
        if (!Threading::IsMainThread())
        {
            return CaptureResult::WrongThread;
        }
        Threading::ScopeLock lock(m_CaptureLocker);
        if (sessionId == 0 || sessionId != m_CaptureStateData.SessionId)
        {
            return CaptureResult::InvalidSession;
        }
        if (snapshotId == 0 || snapshotId != m_CurrentFrameView.SnapshotId)
        {
            return CaptureResult::InvalidSnapshot;
        }
        viewFrames.Resize(m_CurrentFrameView.FrameCount);
        for (int32 i = 0; i < m_CurrentFrameView.FrameCount; i++)
        {
            viewFrames[i] = m_CurrentViewFrames[i];
        }
        info = m_CurrentFrameView;
        return CaptureResult::Ok;
#else
        return CaptureResult::Unavailable;
#endif
    }

    ProfilingSystem::CaptureResult ProfilingSystem::ReadCaptureThreads(uint64                   sessionId,
                                                                       int32                    first,
                                                                       int32&                   total,
                                                                       List<ProfilerCpuThread>& threads,
                                                                       List<byte>&              text)
    {
        total = 0;
        threads.Clear();
        text.Clear();
#ifdef SE_TRACY_SERVER
        if (!Threading::IsMainThread())
            return CaptureResult::WrongThread;
        Threading::ScopeLock lock(m_CaptureLocker);
        if (!m_CaptureWorker || sessionId == 0 || sessionId != m_CaptureStateData.SessionId)
            return CaptureResult::InvalidSession;
        if (m_CaptureStateData.State != CaptureState::Capturing && m_CaptureStateData.State != CaptureState::Stopping &&
            m_CaptureStateData.State != CaptureState::Stopped)
            return CaptureResult::NotReady;
        if (first < 0)
            return CaptureResult::InvalidRange;
        m_PendingCpuThreads.Clear();
        m_PendingCpuText.Clear();
        m_PendingCpuThreads.EnsureCapacity(MaxCpuThreads);
        m_PendingCpuText.EnsureCapacity(MaxCpuTextBytes);
        m_PendingCpuText.Add(0);
        if (!m_CaptureWorker->GetDataLock().try_lock())
            return CaptureResult::Busy;
        const auto&   source = m_CaptureWorker->GetThreadData();
        CaptureResult result = CaptureResult::Ok;
        const int32   count  = static_cast<int32>(source.size());
        if ((count == 0 && first != 0) || (count != 0 && first >= count))
            result = CaptureResult::InvalidRange;
        else
        {
            const int32 end = first + Math::Min(MaxCpuThreads, count - first);
            for (int32 i = first; i < end; i++)
            {
                ProfilerCpuThread& thread = m_PendingCpuThreads.AddOne();
                thread.Id                 = source[i]->id;
                thread.NameOffset         = CopyCpuText(m_CaptureWorker->GetThreadName(thread.Id));
                if (thread.NameOffset < 0)
                {
                    result = CaptureResult::QueryTooLarge;
                    break;
                }
            }
        }
        m_CaptureWorker->GetDataLock().unlock();
        if (result == CaptureResult::Ok)
        {
            total   = count;
            threads = m_PendingCpuThreads;
            text    = m_PendingCpuText;
        }
        return result;
#else
        return CaptureResult::Unavailable;
#endif
    }

    ProfilerCaptureResult Profiling::ReadCaptureThreads(uint64                   sessionId,
                                                        int32                    first,
                                                        int32&                   total,
                                                        List<ProfilerCpuThread>& threads,
                                                        List<byte>&              text)
    {
        return Systems::GetSystem<ProfilingSystem>()->ReadCaptureThreads(sessionId, first, total, threads, text);
    }

    ProfilerCaptureResult Profiling::StartCapture() { return Systems::GetSystem<ProfilingSystem>()->StartCapture(); }

    ProfilerCaptureResult Profiling::StopCapture() { return Systems::GetSystem<ProfilingSystem>()->StopCapture(); }

    ProfilerCaptureResult Profiling::ClearCapture() { return Systems::GetSystem<ProfilingSystem>()->ClearCapture(); }

    ProfilerCaptureResult Profiling::GetCaptureInfo(ProfilerCaptureInfo& info)
    {
        return Systems::GetSystem<ProfilingSystem>()->GetCaptureInfo(info);
    }

    ProfilerCaptureResult Profiling::ReadCaptureFrame(uint64 sessionId, uint64 index, ProfilerCaptureFrame& frame)
    {
        return Systems::GetSystem<ProfilingSystem>()->ReadCaptureFrame(sessionId, index, frame);
    }

    ProfilerCaptureResult Profiling::RefreshFrameView(uint64                      sessionId,
                                                      uint64                      firstFrame,
                                                      int32                       count,
                                                      uint64                      selectionFirst,
                                                      int32                       selectionCount,
                                                      uint64                      threadId,
                                                      ProfilerCaptureInfo&        capture,
                                                      ProfilerFrameViewInfo&      info,
                                                      List<ProfilerCaptureFrame>& frames,
                                                      List<ProfilerCpuThread>&    threads,
                                                      List<ProfilerCpuEvent>&     events,
                                                      List<byte>&                 text,
                                                      List<ProfilerCpuStatistic>& statistics)
    {
        return Systems::GetSystem<ProfilingSystem>()->RefreshFrameView(sessionId,
                                                                       firstFrame,
                                                                       count,
                                                                       selectionFirst,
                                                                       selectionCount,
                                                                       threadId,
                                                                       capture,
                                                                       info,
                                                                       frames,
                                                                       threads,
                                                                       events,
                                                                       text,
                                                                       statistics);
    }

    ProfilerCaptureResult Profiling::ReadFrameView(uint64                      sessionId,
                                                   uint64                      snapshotId,
                                                   ProfilerFrameViewInfo&      info,
                                                   List<ProfilerCaptureFrame>& frames)
    {
        return Systems::GetSystem<ProfilingSystem>()->ReadFrameView(sessionId, snapshotId, info, frames);
    }
} // namespace SE
