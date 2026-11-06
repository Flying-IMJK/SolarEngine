
#include <Runtime/System/ProfilingSystem.h>
#include <Runtime/Core/Platform/CriticalSection.h>
#include <Runtime/Core/Thread/JobSystem.h>
#include <Runtime/Core/Thread/Threading.h>
#include <Runtime/Engine.h>
// #include "Engine/Engine/Time.h"
#include <Runtime/Graphics/GPUContext.h>
#include <Runtime/Graphics/GPUDevice.h>
#include <Runtime/Graphics/GPUTimerQuery.h>
// #include "Engine/Networking/NetworkInternal.h"

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
        m_CaptureInitialized = true;
#ifdef SE_TRACY_SERVER
        m_CaptureStateData.State = CaptureState::Idle;
#endif
        return true;
    }

    void ProfilingSystem::OnUpdate()
    {
        Threading::ScopeLock lock(m_CaptureLocker);
        UpdateCaptureState();
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
        m_CurrentFrameView   = FrameViewInfo();
        m_CaptureStateData   = CaptureInfo();
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

    ProfilingSystem::CaptureResult ProfilingSystem::BeginCaptureCleanup(CaptureState targetState)
    {
#ifdef SE_TRACY_SERVER
        m_CleanupTarget          = targetState;
        m_CurrentFrameView       = FrameViewInfo();
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
            return CaptureResult::WrongThread;
        Threading::ScopeLock lock(m_CaptureLocker);
        if (!m_CaptureInitialized)
            return CaptureResult::NotReady;
        UpdateCaptureState();
        if (m_CaptureStateData.State != CaptureState::Idle)
            return CaptureResult::Busy;
        try
        {
            m_CaptureStateData                  = CaptureInfo();
            m_CurrentFrameView                  = FrameViewInfo();
            m_CaptureStateData.SessionId        = ++m_CaptureSequence;
            m_CaptureStateData.ProcessId        = Platform::GetCurrentProcessId();
            m_CaptureConnectionStart            = Platform::GetTimeSeconds();
            m_CaptureStateData.MemoryLimitBytes = CaptureMemoryLimitBytes;
            // The engine latches its stop reason before Shutdown; disable Tracy's duplicate cutoff.
            m_CaptureWorker          = CreateScope<tracy::Worker>("127.0.0.1", 8086, -1);
            m_CaptureStateData.State = CaptureState::Connecting;
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
            return CaptureResult::WrongThread;
        Threading::ScopeLock lock(m_CaptureLocker);
        // A not-yet-published connection is cancelled, never mistaken for retained history.
        if (m_CaptureStateData.State == CaptureState::Connecting)
        {
            return BeginCaptureCleanup(CaptureState::Idle);
        }
        UpdateCaptureState();
        if (m_CaptureStateData.State == CaptureState::Stopping || m_CaptureStateData.State == CaptureState::Stopped)
            return CaptureResult::Ok;
        if (m_CaptureStateData.State != CaptureState::Capturing || !m_CaptureWorker)
            return CaptureResult::NotReady;
        m_CaptureStateData.State = CaptureState::Stopping;
        // A busy data lock may have deferred observing an already-ended receiver above.
        m_CaptureStateData.StopReason = m_CaptureWorker->IsConnected() ? StopReason::User : StopReason::ReceiverEnded;
        m_CaptureStateData.TailMayBeIncomplete = 1;
        m_CaptureWorker->Shutdown();
        return CaptureResult::Ok;
#else
        return CaptureResult::Unavailable;
#endif
    }

    ProfilingSystem::CaptureResult ProfilingSystem::ClearCapture()
    {
#ifdef SE_TRACY_SERVER
        if (!Threading::IsMainThread())
            return CaptureResult::WrongThread;
        Threading::ScopeLock lock(m_CaptureLocker);
        UpdateCaptureState();
        if (m_CaptureStateData.State == CaptureState::Idle)
            return CaptureResult::Ok;
        if (m_CaptureStateData.State != CaptureState::Stopped && m_CaptureStateData.State != CaptureState::Failed)
            return CaptureResult::Busy;
        if (m_CaptureWorker)
            return BeginCaptureCleanup(CaptureState::Idle);
        else
        {
            m_CurrentFrameView       = FrameViewInfo();
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

    ProfilingSystem::CaptureResult ProfilingSystem::RefreshFrameView(uint64              sessionId,
                                                                     uint64              firstFrame,
                                                                     int32               count,
                                                                     uint64              selectionFirst,
                                                                     int32               selectionCount,
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
        if (count < 1 || selectionCount < 0 || selectionCount > count)
        {
            return CaptureResult::InvalidRange;
        }
        if (count > MaxFrameViewFrames)
        {
            return CaptureResult::QueryTooLarge;
        }
        Threading::ScopeLock lock(m_CaptureLocker);
        if (!m_CaptureWorker || sessionId == 0 || sessionId != m_CaptureStateData.SessionId)
        {
            return CaptureResult::InvalidSession;
        }
        if (m_CaptureStateData.State != CaptureState::Capturing && m_CaptureStateData.State != CaptureState::Stopping &&
            m_CaptureStateData.State != CaptureState::Stopped)
        {
            return CaptureResult::NotReady;
        }
        if (!m_CaptureWorker->GetDataLock().try_lock())
        {
            return CaptureResult::Busy;
        }

        // Bounded, allocation-free copy while holding Tracy's own writer lock. Publish only after validation.
        const tracy::FrameData* frames   = m_CaptureWorker->GetFramesBase();
        const size_t            rawCount = frames ? m_CaptureWorker->GetFrameCount(*frames) : 0;
        FrameViewInfo           next;
        next.SessionId   = sessionId;
        next.TotalFrames = rawCount > 4 ? rawCount - 4 : 0;
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
                CaptureFrame& frame      = m_CurrentViewFrames[i];
                frame.Index              = next.FirstFrame + i;
                const size_t nativeIndex = static_cast<size_t>(frame.Index) + 3;
                frame.BeginNs            = m_CaptureWorker->GetFrameBegin(*frames, nativeIndex);
                frame.EndNs              = m_CaptureWorker->GetFrameBegin(*frames, nativeIndex + 1);
            }
        }
        m_CaptureWorker->GetDataLock().unlock();
        if (result != CaptureResult::Ok)
        {
            return result;
        }

        if (next.FrameCount > 0)
        {
            next.BeginNs          = m_CurrentViewFrames[0].BeginNs;
            next.EndNs            = m_CurrentViewFrames[next.FrameCount - 1].EndNs;
            const int32 selected  = static_cast<int32>(next.SelectionFirst - next.FirstFrame);
            next.SelectionBeginNs = m_CurrentViewFrames[selected].BeginNs;
            next.SelectionEndNs   = m_CurrentViewFrames[selected + static_cast<int32>(next.SelectionCount) - 1].EndNs;
        }
        next.SnapshotId    = ++m_FrameViewSequence;
        m_CurrentFrameView = next;
        viewFrames.Resize(next.FrameCount);
        for (int32 i = 0; i < next.FrameCount; i++)
        {
            viewFrames[i] = m_CurrentViewFrames[i];
        }
        info = next;
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
                                                      ProfilerFrameViewInfo&      info,
                                                      List<ProfilerCaptureFrame>& frames)
    {
        return Systems::GetSystem<ProfilingSystem>()->RefreshFrameView(
            sessionId, firstFrame, count, selectionFirst, selectionCount, info, frames);
    }

    ProfilerCaptureResult Profiling::ReadFrameView(uint64                      sessionId,
                                                   uint64                      snapshotId,
                                                   ProfilerFrameViewInfo&      info,
                                                   List<ProfilerCaptureFrame>& frames)
    {
        return Systems::GetSystem<ProfilingSystem>()->ReadFrameView(sessionId, snapshotId, info, frames);
    }
} // namespace SE
