#pragma once

#include <Runtime/Core/Platform/ConditionVariable.h>
#include <Runtime/Core/Platform/Thread.h>
#include <Runtime/Core/Profiler/ProfilerStats.h>
#include <Runtime/Core/Profiler/ProfilingSystem.h>
#include <Runtime/Core/Types/Collections/Dictionary.h>
#include <Runtime/Core/Types/Collections/Queue.h>
#include <Runtime/Core/Types/Strings/String.h>

namespace SE
{
    struct ProfilerLocalData
    {
        struct CpuEvent
        {
            ProfilerCpuEvent Value;
            StringAnsi       Name;
        };

        struct CpuThread
        {
            uint64         Id = 0;
            StringAnsi     Name;
            List<CpuEvent> Events;
        };

        struct Sample
        {
            ProfilerCaptureFrame Frame;
            uint64               EngineFrame = 0;
            ProfilerMemorySample Memory;
            List<CpuThread>      Threads;
        };

        struct GpuSample
        {
            ProfilerGpuFrame       Info;
            List<ProfilerGpuEvent> Events;
            List<byte>             Text;
            bool                   ResolutionMayBeLost = false;
        };

        // Immutable once published. Only the collector edits its Working history.
        struct History
        {
            Queue<SharedPtr<Sample>>    Samples;
            Queue<SharedPtr<GpuSample>> GpuSamples;
            ProfilerCaptureInfo         Info;
        };

        enum class RequestType
        {
            Sample,
            GpuSubmitted,
            GpuResolved
        };
        struct GpuEvent
        {
            String                  Name;
            RenderStatsData         Stats;
            float                   Time  = 0;
            int32                   Depth = 0;
            ProfilerGPU::EventState State = ProfilerGPU::EventState::Active;
        };
        struct RawCpuThread
        {
            uint64                   Id = 0;
            String                   Name;
            List<ProfilerCPU::Event> Events;
        };
        // Extract retains the last item. Remember it here to avoid exporting that item twice.
        struct CpuCursor
        {
            ProfilerCPU::Event Last;
            bool               HasLast = false;
        };
        struct Request
        {
            RequestType          Type        = RequestType::Sample;
            uint64               SessionId   = 0;
            uint64               EngineFrame = 0;
            int64                SubmittedNs = 0;
            ProfilerCaptureFrame Frame;
            ProfilerMemorySample Memory;
            ProfilerGpuFrame     Gpu;
            List<GpuEvent>       GpuEvents;
            List<RawCpuThread>   CpuThreads;
        };

        List<CpuCursor> CpuCursors;
        double          SessionStartMs = 0;
        bool            Recording      = false;

        // Lock order: lifecycle/main query -> inbox or publication. The collector never takes the query lock.
        CriticalSection    InboxLocker;
        ConditionVariable  Wake;
        SharedPtr<Request> Requests[ProfilingSystem::MaxSamples];
        int32              RequestHead     = 0;
        int32              RequestCount    = 0;
        bool               Busy            = false;
        bool               Exit            = false;
        bool               Accepting       = false;
        uint64             SessionId       = 0;
        uint64             NextSample      = 0;
        uint64             DroppedRequests = 0;
        uint64             GpuLossThrough  = 0;
        uint64             GpuFirstFrame   = 0;
        double             SubmitWaitMs    = 0;
        bool               SubmissionGap   = false;
        Thread*            Collector       = nullptr;

        CriticalSection           PublicationLocker;
        SharedPtr<History>        Published;
        SharedPtr<History>        View;
        History                   Working;
        Queue<SharedPtr<History>> Retired;
        uint64                    LastEngineFrame = ~uint64(0);
        int64                     LastSampleNs    = 0;
    };
} // namespace SE
