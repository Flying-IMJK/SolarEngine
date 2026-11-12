#include <Runtime/Core/Profiler/ProfilingSystem.h>

#include "Runtime/Core/Scripting/ManagedCLR/CLRUtils.h"
#include "Runtime/Core/Thread/ThreadLocal.h"
#include "Runtime/Core/Types/Collections/ChunkedArray.h"
#include <Runtime/Core/Profiler/ProfilerLocalData.h>
#include <Runtime/Core/Thread/Threading.h>
#include <Runtime/Engine.h>
#include <Runtime/Graphics/GPUContext.h>
#include <Runtime/Graphics/GPUDevice.h>
#include <Runtime/Graphics/GPUTimerQuery.h>
#include <Runtime/Core/Scripting/Internal/InternalCalls.h>

namespace SE
{
    ProfilingSystem::MainStats                                ProfilingSystem::Stats;
    List<ProfilingSystem::ThreadStats, InlinedAllocation<64>> ProfilingSystem::EventsCPU;
    List<ProfilerGPU::Event>                                  ProfilingSystem::EventsGPU;
    List<ProfilingSystem::NetworkEventStat>                   ProfilingSystem::EventsNetwork;
    ENGINE_SYSTEM_REGISTER(ProfilingSystem)

    static int64 NowNs() { return static_cast<int64>(Platform::GetTimeSeconds() * 1000000000.0); }

    static StringAnsi LocalUtf8(const Char* pText)
    {
        if (!pText)
            return {};
        int32      length = 0;
        char*      pBytes = StringUtils::ConvertUTF162UTF8(pText, StringUtils::Length(pText), length);
        StringAnsi result(pBytes, length);
        PlatformAllocator::Free(pBytes);
        return result;
    }

    static int32 AddText(List<byte>& text, const char* pValue)
    {
        if (!pValue || !*pValue)
            return 0;
        const int32 offset = text.Count();
        text.Add(reinterpret_cast<const byte*>(pValue), static_cast<int32>(StringUtils::Length(pValue)) + 1);
        return offset;
    }


    
	namespace
    {
#if SE_PROFILER
        List<int32, InlinedAllocation<32>> ManagedEventsGPU;
#if TRACY_ENABLE && !PROFILE_CPU_USE_TRANSIENT_DATA
        CriticalSection ManagedSourceLocationsLocker;

        struct Location
        {
            String                    Name;
            StringAnsi                NameAnsi;
            tracy::SourceLocationData SrcLocation;
        };

        ChunkedArray<Location, 256>    ManagedSourceLocations;
        Threading::ThreadLocal<uint32> ManagedEventsCount;
#endif
#endif
    } // namespace

    DEFINE_INTERNAL_CALL(void) Native_BeginEvent(CLRString* nameObj)
    {
#if SE_PROFILER
        StringView name;
        CLRUtils::ToString(nameObj, name);
        ProfilerCPU::BeginEvent(*name);
#if TRACY_ENABLE
#if PROFILE_CPU_USE_TRANSIENT_DATA
        tracy::ScopedZone::Begin(
            __LINE__, __FILE__, strlen(__FILE__), __FUNCTION__, strlen(__FUNCTION__), name.Get(), name.Length());
#else
        Threading::ScopeLock       lock(ManagedSourceLocationsLocker);
        tracy::SourceLocationData* srcLoc = nullptr;
        for (auto e = ManagedSourceLocations.Begin(); e.IsNotEnd(); ++e)
        {
            if (name == e->Name)
            {
                srcLoc = &e->SrcLocation;
                break;
            }
        }
        if (!srcLoc)
        {
            auto& e          = ManagedSourceLocations.AddOne();
            e.Name           = name;
            e.NameAnsi       = name.Get();
            srcLoc           = &e.SrcLocation;
            srcLoc->name     = e.NameAnsi.Get();
            srcLoc->function = nullptr;
            srcLoc->file     = nullptr;
            srcLoc->line     = 0;
            srcLoc->color    = 0;
        }
        // static constexpr tracy::SourceLocationData tracySrcLoc{ nullptr, __FUNCTION__, __FILE__, (uint32_t)__LINE__,
        // 0 };
        if (tracy::ScopedZone::Begin(srcLoc))
            ManagedEventsCount.Get()++;
#endif
#endif
#endif
    }

    DEFINE_INTERNAL_CALL(void) Native_EndEvent()
    {
#if SE_PROFILER
#if TRACY_ENABLE
        uint32& tracyActive = ManagedEventsCount.Get();
        if (tracyActive > 0)
        {
            tracyActive--;
            tracy::ScopedZone::End();
        }
#endif
        ProfilerCPU::EndEvent();
#endif
    }

    DEFINE_INTERNAL_CALL(void) Native_BeginEventGPU(CLRString* nameObj)
    {
#if SE_PROFILER
        const StringView nameChars = CLRCore::String::GetChars(nameObj);
        const auto       index     = ProfilerGPU::BeginEvent(nameChars.Get());
        ManagedEventsGPU.Push(index);
#endif
    }

    DEFINE_INTERNAL_CALL(void) Native_EndEventGPU()
    {
#if SE_PROFILER
        const auto index = ManagedEventsGPU.Pop();
        ProfilerGPU::EndEvent(index);
#endif
    }


    ProfilingSystem::ProfilingSystem() : ISystem(SE_TEXT("Profiling"))
    {
        m_Local            = New<ProfilerLocalData>();
        m_Local->Published = MakeRef<ProfilerLocalData::History>();
        m_Local->View      = m_Local->Published;
        Platform::MemoryClear(&Stats, sizeof(Stats));
        ProfilerGPU::GetFrameCountCall = []() { return Engine::FrameCount; };
        ProfilerGPU::EventBeginCall    = [](const Char* pName) {
            if (GPUDevice::instance)
                GPUDevice::instance->GetMainContext()->EventBegin(pName);
        };
        ProfilerGPU::EventEndCall = []() {
            if (GPUDevice::instance)
                GPUDevice::instance->GetMainContext()->EventEnd();
        };
        ProfilerGPU::CreateTimerQueryProfileCall = []() -> IGPUTimerQueryProfile* {
            return GPUDevice::instance->CreateTimerQuery();
        };
        ProfilerGPU::DelateTimerQueryProfileCall = [](IGPUTimerQueryProfile* pTimer) {
            GPUTimerQuery* pQuery = static_cast<GPUTimerQuery*>(pTimer);
            pQuery->ReleaseGPU();
            Delete(pQuery);
        };
    }

    ProfilingSystem::~ProfilingSystem()
    {
        DrainCollector(true);
        Delete(m_Local);
    }

    bool ProfilingSystem::OnInit()
    {
        m_CaptureInitialized = true;
#if SE_PROFILER
        m_CaptureStateData.State = CaptureState::Idle;
#endif
        ProfilerGPU::s_FrameDataCall = [this](uint64 frameId, const ProfilerGPU::EventBuffer* pFrame) {
            if (pFrame && pFrame->HasData())
                OnGpuResolved(*pFrame);
            else
                OnGpuSubmitted(frameId, pFrame);
        };
        return true;
    }

    void ProfilingSystem::OnDispose()
    {
        m_Local->Recording = false;
        DrainCollector(true);
        ProfilerCPU::Enabled = false;
        ProfilerGPU::Enabled = false;
        ProfilerGPU::s_FrameDataCall.Unbind();
        ResetView();
        ResetMemoryHistory();
        ResetGpuHistory();
        m_Local->View.reset();
        m_Local->Published.reset();
        m_Local->Working = ProfilerLocalData::History();
        m_Local->Retired.clear();
        m_CaptureInitialized = false;
    }

    bool ProfilingSystem::GetEnabled() { return ProfilerCPU::Enabled && ProfilerGPU::Enabled; }
    void ProfilingSystem::SetEnabled(bool enabled)
    {
        ProfilerCPU::Enabled = enabled;
        ProfilerGPU::Enabled = enabled;
    }

    void ProfilingSystem::OnUpdate()
    {
        PROFILE_CPU();
        if (!m_Local->Recording)
            return;
        if (m_Local->LastEngineFrame == Engine::FrameCount)
        {
            return;
        }
        m_Local->LastEngineFrame    = Engine::FrameCount;
        auto request                = MakeRef<ProfilerLocalData::Request>();
        request->EngineFrame        = Engine::FrameCount;
        request->Frame.BeginNs      = m_Local->LastSampleNs;
        request->Frame.EndNs        = Math::Max(request->Frame.BeginNs + 1, NowNs());
        request->SubmittedNs        = request->Frame.EndNs;
        m_Local->LastSampleNs       = request->Frame.EndNs;
        request->Memory             = m_ManagedMemory;
        request->Memory.FrameId     = request->EngineFrame;
        request->Memory.TimeSeconds = Platform::GetTimeSeconds() - m_MemoryStart;
        // Resource traversal remains on the engine thread; the collector receives numeric values only.
        if (GPUDevice::instance)
        {
            request->Memory.GpuTotalBytes = GPUDevice::instance->GetTotalGraphicsMemory();
            request->Memory.GpuUsedBytes  = GPUDevice::instance->GetMemoryUsage();
            request->Memory.ValidMask |= 4;
        }
        m_Local->CpuCursors.Resize(ProfilerCPU::Threads.Count());
        request->CpuThreads.Resize(ProfilerCPU::Threads.Count());
        for (int32 i = 0; i < request->CpuThreads.Count(); ++i)
        {
            auto* source = ProfilerCPU::Threads[i];
            auto& copy   = request->CpuThreads[i];
            auto& cursor = m_Local->CpuCursors[i];
            copy.Id      = static_cast<uint64>(i) + 1;
            copy.Name    = source->GetName();
            source->Buffer.Extract(copy.Events, true);
            if (copy.Events.HasItems())
            {
                const auto& first    = copy.Events.First();
                const bool  repeated = cursor.HasLast && first.Start == cursor.Last.Start &&
                                      first.End == cursor.Last.End && first.Depth == cursor.Last.Depth;
                cursor.Last    = copy.Events.Last();
                cursor.HasLast = true;
                if (repeated)
                    copy.Events.RemoveAtKeepOrder(0);
            }
        }
        const double waiting = Platform::GetTimeSeconds();
        {
            Threading::ScopeLock lock(m_Local->InboxLocker);
            m_Local->SubmitWaitMs = (Platform::GetTimeSeconds() - waiting) * 1000.0;
            if (m_Local->RequestCount == MaxSamples)
            {
                ++m_Local->DroppedRequests;
                m_Local->SubmissionGap = true;
                return;
            }
            request->SessionId                                                               = m_Local->SessionId;
            request->Frame.Index                                                             = m_Local->NextSample++;
            request->Frame.DataLost                                                          = m_Local->SubmissionGap;
            m_Local->SubmissionGap                                                           = false;
            m_Local->Requests[(m_Local->RequestHead + m_Local->RequestCount++) % MaxSamples] = MoveTemp(request);
        }
        m_Local->Wake.NotifyOne();
    }

    void ProfilingSystem::UpdateCaptureState()
    {
        auto previous = MoveTemp(m_Local->View);
        {
            Threading::ScopeLock lock(m_Local->PublicationLocker);
            m_Local->View = m_Local->Published;
        }
        if (m_Local->View && m_Local->View->Info.SessionId == m_CaptureStateData.SessionId)
        {
            const auto state              = m_CaptureStateData.State;
            const auto reason             = m_CaptureStateData.StopReason;
            m_CaptureStateData            = m_Local->View->Info;
            m_CaptureStateData.State      = state;
            m_CaptureStateData.StopReason = reason;
        }
        Threading::ScopeLock lock(m_Local->InboxLocker);
        m_CaptureStateData.PendingRequests = m_Local->RequestCount + m_Local->Busy;
        m_CaptureStateData.DroppedRequests = m_Local->DroppedRequests;
        m_CaptureStateData.SubmitWaitMs    = m_Local->SubmitWaitMs;
        m_CaptureStateData.TailMayBeIncomplete |= m_Local->DroppedRequests != 0;
    }

    void ProfilingSystem::DrainCollector(bool exit)
    {
        if (!m_Local->Collector)
            return;
        {
            Threading::ScopeLock lock(m_Local->InboxLocker);
            if (exit)
                m_Local->Exit = true;
            m_Local->Wake.NotifyAll();
            if (!exit)
                while (m_Local->RequestCount || m_Local->Busy)
                    m_Local->Wake.Wait(m_Local->InboxLocker);
        }
        if (exit)
        {
            m_Local->Collector->Join();
            Delete(m_Local->Collector);
            m_Local->Collector = nullptr;
        }
    }

    void ProfilingSystem::PublishHistory()
    {
        auto& history = m_Local->Working;
        auto& info    = history.Info;
        if (!history.Samples.empty())
        {
            info.CompleteFrames  = history.Samples.back()->Frame.Index + 1;
            info.OldestFrame     = history.Samples.front()->Frame.Index;
            info.CompleteBeginNs = history.Samples.front()->Frame.BeginNs;
            info.CompleteEndNs = info.LastTimeNs = history.Samples.back()->Frame.EndNs;
        }
        info.GpuFrames  = static_cast<int32>(history.GpuSamples.size());
        info.GpuPending = 0;
        for (const auto& sample : history.GpuSamples)
            info.GpuPending += sample->Info.State == ProfilerGpuFrameState::Pending;
        auto                                  snapshot = MakeRef<ProfilerLocalData::History>(history);
        SharedPtr<ProfilerLocalData::History> previous;
        {
            Threading::ScopeLock lock(m_Local->PublicationLocker);
            previous           = MoveTemp(m_Local->Published);
            m_Local->Published = MoveTemp(snapshot);
        }
        if (previous)
            m_Local->Retired.push_back(MoveTemp(previous));
        // Retain the collector's final reference until all main-thread readers release the old history.
        // Consequently event/name batches are destroyed here, outside the publication lock.
        for (auto it = m_Local->Retired.begin(); it != m_Local->Retired.end();)
        {
            if (it->use_count() == 1)
                it = m_Local->Retired.erase(it);
            else
                ++it;
        }
    }

    int32 ProfilingSystem::Collect()
    {
        auto& history                  = m_Local->Working;
        history.Info.CollectorThreadId = Platform::GetCurrentThreadID();
        for (;;)
        {
            SharedPtr<ProfilerLocalData::Request> request;
            {
                Threading::ScopeLock lock(m_Local->InboxLocker);
                m_Local->Busy = false;
                m_Local->Wake.NotifyAll();
                while (!m_Local->RequestCount && !m_Local->Exit)
                    m_Local->Wake.Wait(m_Local->InboxLocker);
                if (!m_Local->RequestCount && m_Local->Exit)
                    break;
                request              = MoveTemp(m_Local->Requests[m_Local->RequestHead]);
                m_Local->RequestHead = (m_Local->RequestHead + 1) % MaxSamples;
                --m_Local->RequestCount;
                m_Local->Busy = true;
            }
            if (request->SessionId != history.Info.SessionId)
                continue;
            const double started = Platform::GetTimeSeconds();
            if (request->Type == ProfilerLocalData::RequestType::Sample)
            {
                auto sample         = MakeRef<ProfilerLocalData::Sample>();
                sample->Frame       = request->Frame;
                sample->EngineFrame = request->EngineFrame;
                sample->Memory      = request->Memory;
                for (const auto& sourceThread : request->CpuThreads)
                {
                    ProfilerLocalData::CpuThread thread;
                    thread.Id   = sourceThread.Id;
                    thread.Name = LocalUtf8(sourceThread.Name.Get());
                    thread.Events.EnsureCapacity(sourceThread.Events.Count());
                    List<int32> parents;
                    bool        includeRoot = false;
                    for (const auto& source : sourceThread.Events)
                    {
                        if (source.Depth == 0)
                            includeRoot = source.Start >= m_Local->SessionStartMs;
                        if (!includeRoot)
                            continue;
                        ProfilerLocalData::CpuEvent event;
                        event.Name           = LocalUtf8(source.Name);
                        event.Value.BeginNs  = static_cast<int64>(source.Start * 1000000.0);
                        event.Value.EndNs    = source.End > 0 ? static_cast<int64>(source.End * 1000000.0) : -1;
                        event.Value.Depth    = source.Depth;
                        event.Value.EndValid = source.End > 0;
                        while (parents.Count() > source.Depth && parents.HasItems())
                            parents.RemoveLast();
                        if (source.Depth >= 0 && parents.Count() == source.Depth)
                        {
                            event.Value.ParentIndex = parents.IsEmpty() ? -1 : parents.Last();
                            parents.Add(thread.Events.Count());
                        }
                        else
                            event.Value.EndValid = 0;
                        thread.Events.Add(MoveTemp(event));
                    }
                    sample->Threads.Add(MoveTemp(thread));
                }
                ProcessMemoryStats process;
                if (Platform::TryGetProcessMemoryStats(process))
                {
                    sample->Memory.ProcessPhysicalBytes = process.UsedPhysicalMemory;
                    sample->Memory.ProcessPrivateBytes  = process.UsedVirtualMemory;
                    sample->Memory.ValidMask |= 1;
                }
                MemoryStats system;
                if (Platform::TryGetMemoryStats(system))
                {
                    sample->Memory.SystemTotalBytes = system.TotalPhysicalMemory;
                    sample->Memory.SystemUsedBytes  = system.UsedPhysicalMemory;
                    sample->Memory.ValidMask |= 2;
                }
                sample->Memory.NativeReadSeconds = Platform::GetTimeSeconds() - m_MemoryStart;
                history.Samples.push_back(MoveTemp(sample));
                while (history.Samples.size() > MaxSamples)
                    history.Samples.pop_front();
                while (!history.GpuSamples.empty() &&
                       history.GpuSamples.front()->Info.FrameId < history.Samples.front()->EngineFrame)
                    history.GpuSamples.pop_front();
            }
            else if (request->Type == ProfilerLocalData::RequestType::GpuSubmitted)
            {
                auto sample  = MakeRef<ProfilerLocalData::GpuSample>();
                sample->Info = request->Gpu;
                if (history.Samples.empty() || sample->Info.FrameId >= history.Samples.front()->EngineFrame)
                    history.GpuSamples.push_back(MoveTemp(sample));
                while (history.GpuSamples.size() > MaxSamples)
                    history.GpuSamples.pop_front();
            }
            else
            {
                for (auto& retained : history.GpuSamples)
                {
                    if (retained->Info.FrameId != request->Gpu.FrameId ||
                        (retained->Info.State != ProfilerGpuFrameState::Pending && !retained->ResolutionMayBeLost))
                        continue;
                    auto record                  = MakeRef<ProfilerLocalData::GpuSample>(*retained);
                    record->ResolutionMayBeLost  = false;
                    record->Info.ResolvedAtFrame = request->Gpu.ResolvedAtFrame;
                    record->Text.Add(0);
                    List<int32> parents;
                    int32       validTimings = 0;
                    for (const auto& source : request->GpuEvents)
                    {
                        ProfilerGpuEvent event;
                        event.NameOffset = record->Text.Count();
                        const auto name  = LocalUtf8(source.Name.Get());
                        record->Text.Add(reinterpret_cast<const byte*>(name.Get()), name.Length());
                        record->Text.Add(0);
                        event.Depth       = source.Depth;
                        event.Complete    = source.State == ProfilerGPU::EventState::Completed;
                        event.TimingValid = event.Complete && source.Time >= 0 && source.Time < Math::POS_INFINITY;
                        event.StatsValid  = event.Complete;
                        while (parents.Count() > source.Depth && parents.HasItems())
                            parents.RemoveLast();
                        if (source.Depth >= 0 && parents.Count() == source.Depth)
                        {
                            event.ParentIndex = parents.IsEmpty() ? -1 : parents.Last();
                            parents.Add(record->Events.Count());
                        }
                        else
                            event.Complete = event.TimingValid = event.StatsValid = 0;
                        if (event.TimingValid)
                        {
                            event.DurationMs = source.Time;
                            ++validTimings;
                            if (event.ParentIndex >= 0)
                            {
                                const auto& parent = record->Events[event.ParentIndex];
                                if (parent.TimingValid && parent.DurationMs > 0)
                                    event.ParentPercent = source.Time / parent.DurationMs * 100.0f;
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
                        record->Info.IncompleteEvents += !event.Complete;
                        record->Events.Add(event);
                    }
                    record->Info.EventCount = record->Events.Count();
                    record->Info.State      = record->Info.IncompleteEvents ? ProfilerGpuFrameState::Incomplete :
                                              validTimings                  ? ProfilerGpuFrameState::Resolved :
                                                                              ProfilerGpuFrameState::TimingUnavailable;
                    retained                = MoveTemp(record);
                    break;
                }
            }
            uint64 lostGpuThrough;
            {
                Threading::ScopeLock lock(m_Local->InboxLocker);
                lostGpuThrough = m_Local->GpuLossThrough;
            }
            for (auto& sample : history.GpuSamples)
            {
                if (sample->Info.State == ProfilerGpuFrameState::Pending && sample->Info.FrameId <= lostGpuThrough)
                {
                    auto incomplete        = MakeRef<ProfilerLocalData::GpuSample>(*sample);
                    incomplete->Info.State = ProfilerGpuFrameState::Incomplete;
                    // A lower frame may still have a valid resolution already admitted to the inbox.
                    incomplete->ResolutionMayBeLost = true;
                    sample                          = MoveTemp(incomplete);
                }
            }
            if (request->Type == ProfilerLocalData::RequestType::Sample)
            {
                history.Info.CollectMs      = (Platform::GetTimeSeconds() - started) * 1000.0;
                history.Info.PublishDelayMs = (NowNs() - request->SubmittedNs) / 1000000.0;
            }
            PublishHistory();
        }
        return 0;
    }

    CaptureResult ProfilingSystem::StartCapture()
    {
        if (!Threading::IsMainThread())
        {
            return CaptureResult::WrongThread;
        }
        Threading::ScopeLock lock(m_CaptureLocker);
        UpdateCaptureState();
#if !SE_PROFILER
        return CaptureResult::Unavailable;
#endif
        if (!m_CaptureInitialized)
            return CaptureResult::NotReady;
        if (m_CaptureStateData.State != CaptureState::Idle)
            return CaptureResult::Busy;
        ResetView();
        ResetGpuHistory();
        ResetMemoryHistory();
        m_Local->LastEngineFrame     = ~uint64(0);
        m_Local->LastSampleNs        = NowNs();
        m_CaptureStateData           = CaptureInfo();
        m_CaptureStateData.SessionId = ++m_CaptureSequence;
        m_CaptureStateData.ProcessId = Platform::GetCurrentProcessId();
        m_CaptureStateData.State     = CaptureState::Capturing;
        m_MemoryStart                = Platform::GetTimeSeconds();
        m_Local->SessionId           = m_CaptureStateData.SessionId;
        m_Local->NextSample          = 0;
        m_Local->DroppedRequests     = 0;
        m_Local->GpuLossThrough      = 0;
        // Exclude the frame already in flight at Start, including buffers from an immediately cleared session.
        m_Local->GpuFirstFrame = Engine::FrameCount + 1;
        m_Local->SubmissionGap = false;
        m_Local->Exit          = false;
        m_Local->Accepting     = true;
        m_Local->Working       = ProfilerLocalData::History();
        m_Local->Working.Info  = m_CaptureStateData;
        m_Local->Published     = MakeRef<ProfilerLocalData::History>(m_Local->Working);
        m_Local->View          = m_Local->Published;
        m_Local->Collector     = Threading::StartThread([this]() { return Collect(); }, SE_TEXT("Profiler Collector"));
        if (!m_Local->Collector)
        {
            m_Local->Accepting       = false;
            m_CaptureStateData.State = CaptureState::Failed;
            m_CaptureStateData.Error = CaptureResult::InternalError;
            return CaptureResult::InternalError;
        }
        m_Local->CpuCursors.Clear();
        m_Local->SessionStartMs = Platform::GetTimeSeconds() * 1000.0;
        ProfilerCPU::Enabled    = true;
        ProfilerGPU::Enabled    = true;
        m_Local->Recording      = true;
        return CaptureResult::Ok;
    }

    CaptureResult ProfilingSystem::StopCapture()
    {
        if (!Threading::IsMainThread())
            return CaptureResult::WrongThread;
        Threading::ScopeLock lock(m_CaptureLocker);
        UpdateCaptureState();
        if (m_CaptureStateData.State != CaptureState::Capturing)
            return CaptureResult::NotReady;
        m_Local->Recording = false;
        {
            Threading::ScopeLock inboxLock(m_Local->InboxLocker);
            m_Local->Accepting = false;
        }
        ProfilerCPU::Enabled = false;
        ProfilerGPU::Enabled = false;
        DrainCollector(false);
        m_CaptureStateData.State      = CaptureState::Stopped;
        m_CaptureStateData.StopReason = StopReason::User;
        UpdateCaptureState();
        return CaptureResult::Ok;
    }

    CaptureResult ProfilingSystem::ClearCapture()
    {
        if (!Threading::IsMainThread())
            return CaptureResult::WrongThread;
        Threading::ScopeLock lock(m_CaptureLocker);
        UpdateCaptureState();
        if (m_Local->Recording)
        {
            return CaptureResult::Busy;
        }
        DrainCollector(true);
        ResetView();
        ResetMemoryHistory();
        ResetGpuHistory();
        m_Local->View.reset();
        m_Local->Published.reset();
        m_Local->Working = ProfilerLocalData::History();
        m_Local->Retired.clear();
        m_Local->DroppedRequests = 0;
        m_Local->GpuLossThrough  = 0;
        m_Local->SessionId       = 0;
        m_Local->LastSampleNs    = 0;
        m_CaptureStateData       = CaptureInfo();
        m_CaptureStateData.State = CaptureState::Idle;
        m_Local->Published       = MakeRef<ProfilerLocalData::History>();
        m_Local->View            = m_Local->Published;
        return CaptureResult::Ok;
    }

    CaptureResult ProfilingSystem::GetCaptureInfo(CaptureInfo& info)
    {
        info = CaptureInfo();
        if (!Threading::IsMainThread())
            return CaptureResult::WrongThread;
        Threading::ScopeLock lock(m_CaptureLocker);
        UpdateCaptureState();
        info = m_CaptureStateData;
        return CaptureResult::Ok;
    }

    CaptureResult ProfilingSystem::ReadCaptureFrame(uint64 sessionId, uint64 index, CaptureFrame& frame)
    {
        frame = CaptureFrame();
        if (!Threading::IsMainThread())
            return CaptureResult::WrongThread;
        Threading::ScopeLock lock(m_CaptureLocker);
        UpdateCaptureState();
        if (!sessionId || sessionId != m_CaptureStateData.SessionId)
            return CaptureResult::InvalidSession;
        if (m_Local->View->Samples.empty() || index < m_Local->View->Samples.front()->Frame.Index ||
            index > m_Local->View->Samples.back()->Frame.Index)
            return CaptureResult::InvalidFrame;
        frame = m_Local->View->Samples[index - m_Local->View->Samples.front()->Frame.Index]->Frame;
        return CaptureResult::Ok;
    }

    void ProfilingSystem::ResetGpuHistory() { m_Local->Working.GpuSamples.clear(); }

    void ProfilingSystem::OnGpuSubmitted(uint64 frameId, const ProfilerGPU::EventBuffer* pFrame)
    {
        uint64 sessionId;
        {
            Threading::ScopeLock lock(m_Local->InboxLocker);
            if (!m_Local->Accepting || m_Local->Exit || frameId < m_Local->GpuFirstFrame)
                return;
            sessionId = m_Local->SessionId;
        }
        auto request         = MakeRef<ProfilerLocalData::Request>();
        request->Type        = ProfilerLocalData::RequestType::GpuSubmitted;
        request->SubmittedNs = NowNs();
        request->Gpu.FrameId = frameId;
        request->Gpu.State   = !pFrame              ? ProfilerGpuFrameState::BufferUnavailable :
                               pFrame->Count() == 0 ? ProfilerGpuFrameState::NoEvents :
                                                      ProfilerGpuFrameState::Pending;
        if (pFrame)
        {
            request->Gpu.PresentMs        = pFrame->PresentTime;
            request->Gpu.PresentAvailable = pFrame->PresentMeasured;
        }
        {
            Threading::ScopeLock lock(m_Local->InboxLocker);
            if (!m_Local->Accepting || m_Local->Exit || sessionId != m_Local->SessionId)
                return;
            if (m_Local->RequestCount == MaxSamples)
            {
                ++m_Local->DroppedRequests;
                return;
            }
            request->SessionId                                                               = sessionId;
            m_Local->Requests[(m_Local->RequestHead + m_Local->RequestCount++) % MaxSamples] = MoveTemp(request);
        }
        m_Local->Wake.NotifyOne();
    }

    void ProfilingSystem::OnGpuResolved(const ProfilerGPU::EventBuffer& frame)
    {
        uint64 sessionId;
        {
            Threading::ScopeLock lock(m_Local->InboxLocker);
            if (!m_Local->Collector || m_Local->Exit)
                return;
            sessionId = m_Local->SessionId;
        }
        auto request                 = MakeRef<ProfilerLocalData::Request>();
        request->Type                = ProfilerLocalData::RequestType::GpuResolved;
        request->SubmittedNs         = NowNs();
        request->Gpu.FrameId         = frame.FrameIndex;
        request->Gpu.ResolvedAtFrame = Engine::FrameCount;
        request->GpuEvents.EnsureCapacity(frame.Count());
        for (int32 i = 0; i < frame.Count(); ++i)
        {
            const auto&                 source = *frame.Get(i);
            ProfilerLocalData::GpuEvent copy;
            copy.Name  = source.Name;
            copy.Stats = source.Stats;
            copy.Time  = source.Time;
            copy.Depth = source.Depth;
            copy.State = source.State;
            request->GpuEvents.Add(MoveTemp(copy));
        }
        {
            Threading::ScopeLock lock(m_Local->InboxLocker);
            // Stopped sessions still accept resolutions of their admitted GPU frames until Clear.
            if (!m_Local->Collector || m_Local->Exit || sessionId != m_Local->SessionId)
                return;
            if (m_Local->RequestCount == MaxSamples)
            {
                ++m_Local->DroppedRequests;
                m_Local->GpuLossThrough = Math::Max(m_Local->GpuLossThrough, frame.FrameIndex);
                return;
            }
            request->SessionId                                                               = sessionId;
            m_Local->Requests[(m_Local->RequestHead + m_Local->RequestCount++) % MaxSamples] = MoveTemp(request);
        }
        m_Local->Wake.NotifyOne();
    }

    CaptureResult ProfilingSystem::RefreshGpuView(uint64                  sessionId,
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
        if (!sessionId || sessionId != capture.SessionId)
            return CaptureResult::InvalidSession;
        int selected = -1;
        for (const auto& sample : m_Local->View->GpuSamples)
        {
            if (sample->Info.FrameId == frameId || frameId == ~uint64(0))
                selected = frames.Count();
            frames.Add(sample->Info);
        }
        if (frameId != ~uint64(0) && selected < 0)
            return CaptureResult::InvalidFrame;
        if (selected >= 0)
        {
            const auto& sample = m_Local->View->GpuSamples[selected];
            for (const auto& event : sample->Events)
                events.Add(event);
            for (byte value : sample->Text)
                text.Add(value);
            info.SelectedFrameId = sample->Info.FrameId;
        }
        info.SessionId     = sessionId;
        info.SnapshotId    = ++m_FrameViewSequence;
        info.SelectedIndex = selected;
        ResetView();
        m_CurrentGpuView   = info;
        m_CurrentGpuFrames = frames;
        m_CurrentGpuEvents = events;
        m_CurrentGpuText   = text;
        return CaptureResult::Ok;
    }

    void ProfilingSystem::ResetMemoryHistory()
    {
        m_ManagedMemory = ProfilerMemorySample();
        m_Local->CpuCursors.Clear();
        m_Local->CpuCursors.SetCapacity(0);
    }

    CaptureResult ProfilingSystem::RefreshMemoryOverview(uint64                      sessionId,
                                                                          int64                       first,
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
        if (!sessionId || sessionId != capture.SessionId)
            return CaptureResult::InvalidSession;
        const int64 total  = static_cast<int64>(m_Local->View->Samples.size());
        const int64 oldest = static_cast<int64>(capture.OldestFrame);
        const int64 next   = oldest + total;
        if (first == -1)
            first = Math::Max(oldest, next - MaxFrameViewFrames);
        if (first < oldest || first > next)
            return CaptureResult::InvalidRange;
        info.SessionId    = sessionId;
        info.SnapshotId   = ++m_FrameViewSequence;
        info.TotalSamples = static_cast<int32>(total);
        info.First        = first;
        info.OldestSample = oldest;
        info.Count        = static_cast<int32>(Math::Min(int64(MaxFrameViewFrames), next - first));
        for (int32 i = 0; i < info.Count; ++i)
            samples.Add(m_Local->View->Samples[first - oldest + i]->Memory);
        ResetView();
        m_CurrentMemoryOverview = info;
        m_CurrentMemorySamples  = samples;
        return CaptureResult::Ok;
    }
    CaptureResult ProfilingSystem::SubmitManagedMemory(uint64 sessionId,
                                                                        int64  heapBytes,
                                                                        int64  allocatedBytes,
                                                                        int32  gc0,
                                                                        int32  gc1,
                                                                        int32  gc2,
                                                                        double readSeconds)
    {
        if (!Threading::IsMainThread())
        {
            return CaptureResult::WrongThread;
        }
        Threading::ScopeLock lock(m_CaptureLocker);
        UpdateCaptureState();
        if (sessionId == 0 || sessionId != m_CaptureStateData.SessionId)
        {
            return CaptureResult::InvalidSession;
        }
        if (m_CaptureStateData.State != CaptureState::Capturing)
        {
            return CaptureResult::NotReady;
        }
        if (heapBytes < 0 || allocatedBytes < 0 || gc0 < 0 || gc1 < 0 || gc2 < 0 ||
            !(readSeconds >= 0 && readSeconds < Math::POS_INFINITY))
        {
            return CaptureResult::InvalidRange;
        }
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

    void ProfilingSystem::ResetMemoryView()
    {
        m_CurrentMemoryOverview = ProfilerMemoryOverviewInfo();
        m_CurrentMemorySamples.Clear();
        m_CurrentMemorySamples.SetCapacity(0);
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
        m_CurrentFrameView = FrameViewInfo();
        m_CurrentCpuThreads.Clear();
        m_CurrentCpuThreads.SetCapacity(0);
        m_CurrentCpuEvents.Clear();
        m_CurrentCpuEvents.SetCapacity(0);
        m_CurrentCpuText.Clear();
        m_CurrentCpuText.SetCapacity(0);
        m_CurrentCpuStatistics.Clear();
        m_CurrentCpuStatistics.SetCapacity(0);
        m_PendingCpuThreads.Clear();
        m_PendingCpuThreads.SetCapacity(0);
        m_PendingCpuEvents.Clear();
        m_PendingCpuEvents.SetCapacity(0);
        m_PendingCpuText.Clear();
        m_PendingCpuText.SetCapacity(0);
        m_PendingCpuStatistics.Clear();
        m_PendingCpuStatistics.SetCapacity(0);
    }

    int32 ProfilingSystem::CopyCpuText(const char* pText) { return AddText(m_PendingCpuText, pText); }

    CaptureResult ProfilingSystem::CopyCpuView(uint64 firstSample, int32 sampleCount, uint64 threadId, int32& incomplete)
    {
        m_PendingCpuThreads.Clear();
        m_PendingCpuEvents.Clear();
        m_PendingCpuText.Clear();
        m_PendingCpuText.Add(0);
        Dictionary<uint64, List<const ProfilerLocalData::CpuThread*>> threads;
        bool                                                          knownThread = threadId == 0;
        for (const auto& sample : m_Local->View->Samples)
        {
            for (const auto& thread : sample->Threads)
            {
                if (threadId && thread.Id != threadId)
                    continue;
                knownThread = true;
                if (sample->Frame.Index >= firstSample && sample->Frame.Index - firstSample < uint64(sampleCount))
                    threads[thread.Id].Add(&thread);
            }
        }
        if (!knownThread)
            return CaptureResult::InvalidRange;
        if (threads.Count() > MaxCpuThreads)
            return CaptureResult::QueryTooLarge;
        Dictionary<StringAnsi, int32> sources;
        incomplete = 0;
        for (const auto& entry : threads)
        {
            ProfilerCpuThread thread;
            thread.Id         = entry.Key;
            thread.NameOffset = CopyCpuText(entry.Value.First()->Name.Get());
            thread.FirstEvent = m_PendingCpuEvents.Count();
            for (const auto* eventSet : entry.Value)
            {
                const int32 firstEvent = m_PendingCpuEvents.Count();
                for (const auto& source : eventSet->Events)
                {
                    if (m_PendingCpuEvents.Count() >= MaxCpuEvents || source.Value.Depth >= MaxCpuDepth ||
                        m_PendingCpuText.Count() + source.Name.Length() + 1 > MaxCpuTextBytes)
                        return CaptureResult::QueryTooLarge;
                    auto event       = source.Value;
                    event.NameOffset = CopyCpuText(source.Name.Get());
                    int32& sourceId  = sources[source.Name];
                    if (!sourceId)
                        sourceId = sources.Count();
                    event.SourceLocationId = sourceId;
                    if (event.ParentIndex >= 0)
                        event.ParentIndex += firstEvent;
                    m_PendingCpuEvents.Add(event);
                    thread.MaxDepth = Math::Max(thread.MaxDepth, event.Depth + 1);
                    incomplete += !event.EndValid;
                }
            }
            thread.EventCount = m_PendingCpuEvents.Count() - thread.FirstEvent;
            m_PendingCpuThreads.Add(thread);
        }
        return CaptureResult::Ok;
    }

    void ProfilingSystem::BuildCpuStatistics(int64 beginNs, int64 endNs)
    {
        // Direct-child coverage uses a union so overlapping siblings are subtracted once.
        const int32            count = m_PendingCpuEvents.Count();
        List<CpuChildCoverage> m_CpuChildCoverage;
        m_CpuChildCoverage.Resize(count);
        m_PendingCpuStatistics.Clear();
        Dictionary<int32, int32> statisticIndices;
        for (const auto& event : m_PendingCpuEvents)
        {
            if (statisticIndices.ContainsKey(event.SourceLocationId))
                continue;
            statisticIndices.Add(event.SourceLocationId, m_PendingCpuStatistics.Count());
            ProfilerCpuStatistic row;
            row.SourceLocationId = event.SourceLocationId;
            row.NameOffset       = event.NameOffset;
            m_PendingCpuStatistics.Add(row);
        }
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
            ProfilerCpuStatistic&   row   = m_PendingCpuStatistics[statisticIndices[event.SourceLocationId]];
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

    CaptureResult ProfilingSystem::RefreshFrameView(uint64                      sessionId,
                                                                     uint64                      firstFrame,
                                                                     int32                       count,
                                                                     uint64                      selectionFirst,
                                                                     int32                       selectionCount,
                                                                     uint64                      threadId,
                                                                     CaptureInfo&                capture,
                                                                     FrameViewInfo&              info,
                                                                     List<CaptureFrame>&         frames,
                                                                     List<ProfilerCpuThread>&    threads,
                                                                     List<ProfilerCpuEvent>&     events,
                                                                     List<byte>&                 text,
                                                                     List<ProfilerCpuStatistic>& statistics)
    {
        capture = CaptureInfo();
        info    = FrameViewInfo();
        frames.Clear();
        threads.Clear();
        events.Clear();
        text.Clear();
        statistics.Clear();
        if (!Threading::IsMainThread())
            return CaptureResult::WrongThread;
        Threading::ScopeLock lock(m_CaptureLocker);
        UpdateCaptureState();
        capture = m_CaptureStateData;
        if (!sessionId || sessionId != capture.SessionId)
            return CaptureResult::InvalidSession;
        if (count <= 0 || selectionCount < 0 || selectionCount > count)
            return CaptureResult::InvalidRange;
        if (count > MaxFrameViewFrames)
            return CaptureResult::QueryTooLarge;
        const uint64 oldest =
            m_Local->View->Samples.empty() ? capture.CompleteFrames : m_Local->View->Samples.front()->Frame.Index;
        if (firstFrame == ~uint64(0))
            firstFrame = Math::Max(oldest, capture.CompleteFrames > uint64(count) ? capture.CompleteFrames - count : 0);
        if (firstFrame < oldest || firstFrame > capture.CompleteFrames)
            return CaptureResult::InvalidRange;
        FrameViewInfo next;
        next.SessionId      = sessionId;
        next.SnapshotId     = ++m_FrameViewSequence;
        next.TotalFrames    = capture.CompleteFrames;
        next.FirstFrame     = firstFrame;
        next.FrameCount     = static_cast<int32>(Math::Min(uint64(count), capture.CompleteFrames - firstFrame));
        next.FilterThreadId = threadId;
        next.SelectionFirst =
            selectionCount == 0 && next.FrameCount ? firstFrame + next.FrameCount - 1 : selectionFirst;
        next.SelectionCount = selectionCount == 0 ? (next.FrameCount ? 1 : 0) : selectionCount;
        if (next.SelectionCount &&
            (next.SelectionFirst < firstFrame || next.SelectionFirst >= firstFrame + next.FrameCount ||
             next.SelectionCount > firstFrame + next.FrameCount - next.SelectionFirst))
            return CaptureResult::InvalidRange;
        for (int32 i = 0; i < next.FrameCount; ++i)
        {
            frames.Add(m_Local->View->Samples[firstFrame - oldest + i]->Frame);
            next.TailMayBeIncomplete |= frames.Last().DataLost;
        }
        next.TailMayBeIncomplete |= capture.TailMayBeIncomplete;
        if (next.FrameCount)
        {
            next.BeginNs          = frames.First().BeginNs;
            next.EndNs            = frames.Last().EndNs;
            next.SelectionBeginNs = frames[static_cast<int32>(next.SelectionFirst - firstFrame)].BeginNs;
            next.SelectionEndNs =
                frames[static_cast<int32>(next.SelectionFirst - firstFrame + next.SelectionCount - 1)].EndNs;
        }
        auto result = CopyCpuView(
            next.SelectionFirst, static_cast<int32>(next.SelectionCount), threadId, next.IncompleteCpuEvents);
        if (result != CaptureResult::Ok)
        {
            frames.Clear();
            return result;
        }
        // A completed root can arrive several updates after its measured start.
        // Fit the selected batches to their real events instead of clipping them to submission times.
        for (const auto& event : m_PendingCpuEvents)
        {
            next.SelectionBeginNs = Math::Min(next.SelectionBeginNs, event.BeginNs);
            if (event.EndValid)
                next.SelectionEndNs = Math::Max(next.SelectionEndNs, event.EndNs);
        }
        next.BeginNs = Math::Min(next.BeginNs, next.SelectionBeginNs);
        next.EndNs   = Math::Max(next.EndNs, next.SelectionEndNs);
        BuildCpuStatistics(next.SelectionBeginNs, next.SelectionEndNs);
        ResetMemoryView();
        m_CurrentGpuView = ProfilerGpuViewInfo();
        m_CurrentGpuFrames.Clear();
        m_CurrentGpuEvents.Clear();
        m_CurrentGpuText.Clear();
        m_CurrentFrameView = next;
        for (int32 i = 0; i < frames.Count(); ++i)
            m_CurrentViewFrames[i] = frames[i];
        m_CurrentCpuThreads    = m_PendingCpuThreads;
        m_CurrentCpuEvents     = m_PendingCpuEvents;
        m_CurrentCpuText       = m_PendingCpuText;
        m_CurrentCpuStatistics = m_PendingCpuStatistics;
        threads                = m_CurrentCpuThreads;
        events                 = m_CurrentCpuEvents;
        text                   = m_CurrentCpuText;
        statistics             = m_CurrentCpuStatistics;
        info                   = next;
        return CaptureResult::Ok;
    }

    CaptureResult ProfilingSystem::ReadFrameView(uint64 sessionId, uint64 snapshotId, FrameViewInfo& info, List<CaptureFrame>& frames)
    {
        info = FrameViewInfo();
        frames.Clear();
        if (!Threading::IsMainThread())
            return CaptureResult::WrongThread;
        Threading::ScopeLock lock(m_CaptureLocker);
        UpdateCaptureState();
        if (!sessionId || sessionId != m_CaptureStateData.SessionId)
            return CaptureResult::InvalidSession;
        if (!snapshotId || snapshotId != m_CurrentFrameView.SnapshotId)
            return CaptureResult::InvalidSnapshot;
        info = m_CurrentFrameView;
        for (int32 i = 0; i < info.FrameCount; ++i)
            frames.Add(m_CurrentViewFrames[i]);
        return CaptureResult::Ok;
    }

    CaptureResult ProfilingSystem::ReadCaptureThreads(uint64                   sessionId,
                                                                       int32                    first,
                                                                       int32&                   total,
                                                                       List<ProfilerCpuThread>& threads,
                                                                       List<byte>&              text)
    {
        total = 0;
        threads.Clear();
        text.Clear();
        text.Add(0);
        if (!Threading::IsMainThread())
        {
            return CaptureResult::WrongThread;
        }
        Threading::ScopeLock lock(m_CaptureLocker);
        UpdateCaptureState();
        if (!sessionId || sessionId != m_CaptureStateData.SessionId)
        {
            return CaptureResult::InvalidSession;
        }
        Dictionary<uint64, const ProfilerLocalData::CpuThread*> directory;
        for (const auto& sample : m_Local->View->Samples)
        {
            for (const auto& thread : sample->Threads)
            {
                directory[thread.Id] = &thread;
            }
        }
        total = static_cast<int32>(directory.Count());
        if (first < 0 || first > total)
        {
            return CaptureResult::InvalidRange;
        }
        int32 position = 0;
        for (const auto& entry : directory)
        {
            if (position++ < first || threads.Count() == MaxCpuThreads)
            {
                continue;
            }
            ProfilerCpuThread thread;
            thread.Id         = entry.Key;
            thread.NameOffset = AddText(text, entry.Value->Name.Get());
            threads.Add(thread);
        }
        return CaptureResult::Ok;
    }

    int32 Profiling::GetHistoryCapacity() { return ProfilingSystem::MaxSamples; }

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
                                                           int64                       first,
                                                           ProfilerCaptureInfo&        capture,
                                                           ProfilerMemoryOverviewInfo& info,
                                                           List<ProfilerMemorySample>& samples)
    {
        return Systems::GetSystem<ProfilingSystem>()->RefreshMemoryOverview(sessionId, first, capture, info, samples);
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
