#pragma once

#include <Runtime/Core/Scripting/ScriptingType.h>
#include <Runtime/API.h>

namespace SE
{
    /// <summary>
    /// Profiler tools for development. Allows to gather profiling data and events from the engine.
    /// </summary>
    SE_ENUM(API())
    enum class ProfilerCaptureState : int32
    {
        Unavailable,
        Idle,
        Capturing,
        Stopped,
        Failed
    };

    SE_ENUM(API())
    enum class ProfilerCaptureResult : int32
    {
        Ok,
        Unavailable,
        NotReady,
        Busy,
        InvalidSession,
        InvalidFrame,
        WrongThread,
        InternalError,
        InvalidSnapshot,
        InvalidRange,
        QueryTooLarge
    };

    SE_ENUM(API())
    enum class ProfilerStopReason : int32
    {
        None,
        User,
    };

    SE_STRUCT(API())
    struct SE_API_RUNTIME ProfilerCaptureInfo
    {
        SCRIPTING_TYPE_MIN(ProfilerCaptureInfo)
        SE_FIELD(API())
        uint64 SessionId = 0;
        SE_FIELD(API())
        uint64 ProcessId = 0;
        SE_FIELD(API())
        uint64 CompleteFrames = 0;
        // Absolute first retained sample identity. CompleteFrames is the next sample sequence.
        SE_FIELD(API())
        uint64 OldestFrame = 0;
        SE_FIELD(API())
        int64 LastTimeNs = 0;
        SE_FIELD(API())
        ProfilerCaptureState State = ProfilerCaptureState::Unavailable;
        SE_FIELD(API())
        ProfilerCaptureResult Error = ProfilerCaptureResult::Ok;
        SE_FIELD(API())
        int32 TailMayBeIncomplete = 0;
        SE_FIELD(API())
        ProfilerStopReason StopReason = ProfilerStopReason::None;
        SE_FIELD(API())
        int64 CompleteBeginNs = 0;
        SE_FIELD(API())
        int64 CompleteEndNs = 0;
        SE_FIELD(API())
        int32 GpuFrames = 0;
        SE_FIELD(API())
        int32 GpuPending = 0;
        SE_FIELD(API())
        int32 PendingRequests = 0;
        SE_FIELD(API())
        uint64 DroppedRequests = 0;
        SE_FIELD(API())
        uint64 CollectorThreadId = 0;
        // Last CPU/memory request, including OS reads; publication and GPU conversion are separate work.
        SE_FIELD(API())
        double CollectMs = 0;
        SE_FIELD(API())
        double SubmitWaitMs = 0;
        SE_FIELD(API())
        double PublishDelayMs = 0;
    };

    SE_STRUCT(API())
    struct SE_API_RUNTIME ProfilerCaptureFrame
    {
        SCRIPTING_TYPE_MIN(ProfilerCaptureFrame)
        SE_FIELD(API())
        uint64 Index = 0;
        SE_FIELD(API())
        int64 BeginNs = 0;
        SE_FIELD(API())
        int64 EndNs = 0;
        // A known submission gap intersects this batch. Raw ring loss is not reported by ProfilerCPU.
        SE_FIELD(API())
        int32 DataLost = 0;
    };

    SE_STRUCT(API())
    struct SE_API_RUNTIME ProfilerFrameViewInfo
    {
        SCRIPTING_TYPE_MIN(ProfilerFrameViewInfo)
        SE_FIELD(API())
        uint64 SessionId = 0;
        SE_FIELD(API())
        uint64 SnapshotId = 0;
        SE_FIELD(API())
        uint64 TotalFrames = 0;
        SE_FIELD(API())
        uint64 FirstFrame = 0;
        SE_FIELD(API())
        int64 BeginNs = 0;
        SE_FIELD(API())
        int64 EndNs = 0;
        SE_FIELD(API())
        uint64 SelectionFirst = 0;
        SE_FIELD(API())
        uint64 SelectionCount = 0;
        SE_FIELD(API())
        int64 SelectionBeginNs = 0;
        SE_FIELD(API())
        int64 SelectionEndNs = 0;
        SE_FIELD(API())
        int32 FrameCount = 0;
        SE_FIELD(API())
        int32 HasPartialFrame = 0;
        SE_FIELD(API())
        int32 TailMayBeIncomplete = 0;
        SE_FIELD(API())
        int32 IncompleteCpuEvents = 0;
        SE_FIELD(API())
        uint64 FilterThreadId = 0;
    };

    SE_STRUCT(API())
    struct SE_API_RUNTIME ProfilerCpuThread
    {
        SCRIPTING_TYPE_MIN(ProfilerCpuThread)
        // Session-local display identity assigned by the sampling layer; not an OS thread ID.
        SE_FIELD(API())
        uint64 Id = 0;
        SE_FIELD(API())
        int32 NameOffset = 0;
        SE_FIELD(API())
        int32 FirstEvent = 0;
        SE_FIELD(API())
        int32 EventCount = 0;
        SE_FIELD(API())
        int32 MaxDepth = 0;
    };

    SE_STRUCT(API())
    struct SE_API_RUNTIME ProfilerCpuEvent
    {
        SCRIPTING_TYPE_MIN(ProfilerCpuEvent)
        SE_FIELD(API())
        int64 BeginNs = 0;
        SE_FIELD(API())
        int64 EndNs = -1;
        SE_FIELD(API())
        int32 ParentIndex = -1;
        SE_FIELD(API())
        int32 Depth = 0;
        SE_FIELD(API())
        int32 SourceLocationId = 0;
        SE_FIELD(API())
        uint32 Line = 0;
        SE_FIELD(API())
        int32 NameOffset = 0;
        SE_FIELD(API())
        int32 FunctionOffset = 0;
        SE_FIELD(API())
        int32 FileOffset = 0;
        SE_FIELD(API())
        int32 EndValid = 0;
    };

    // Source-location totals for the frozen selected range and explicit thread filter.
    // Calls/Total/Average/Max include complete events only. SelfNs is a known subtotal when
    // SelfIncompleteCalls != 0; it must not be displayed as a complete self-time total.
    // RepresentativeEvent is the largest complete instance in the same event array, or -1.
    SE_STRUCT(API())
    struct SE_API_RUNTIME ProfilerCpuStatistic
    {
        SCRIPTING_TYPE_MIN(ProfilerCpuStatistic)
        SE_FIELD(API())
        int32 SourceLocationId = 0;
        SE_FIELD(API())
        int32 NameOffset = 0;
        SE_FIELD(API())
        int32 FunctionOffset = 0;
        SE_FIELD(API())
        int32 FileOffset = 0;
        SE_FIELD(API())
        uint32 Line = 0;
        SE_FIELD(API())
        int32 RepresentativeEvent = -1;
        SE_FIELD(API())
        uint64 Calls = 0;
        SE_FIELD(API())
        uint64 IncompleteCalls = 0;
        SE_FIELD(API())
        uint64 TotalNs = 0;
        SE_FIELD(API())
        uint64 SelfNs = 0;
        SE_FIELD(API())
        uint64 MaxNs = 0;
        SE_FIELD(API())
        double AverageNs = 0;
        SE_FIELD(API())
        uint64 SelfIncompleteCalls = 0;
    };

    SE_ENUM(API())
    enum class ProfilerGpuFrameState : int32
    {
        Pending,
        Resolved,
        NoEvents,
        BufferUnavailable,
        TimingUnavailable,
        Incomplete
    };

    SE_STRUCT(API())
    struct SE_API_RUNTIME ProfilerGpuFrame
    {
        SCRIPTING_TYPE_MIN(ProfilerGpuFrame)
        SE_FIELD(API())
        uint64 FrameId = 0;
        SE_FIELD(API())
        uint64 ResolvedAtFrame = 0;
        SE_FIELD(API())
        ProfilerGpuFrameState State = ProfilerGpuFrameState::Pending;
        SE_FIELD(API())
        int32 EventCount = 0;
        SE_FIELD(API())
        float PresentMs = 0;
        SE_FIELD(API())
        int32 PresentAvailable = 0;
        SE_FIELD(API())
        int32 IncompleteEvents = 0;
    };

    SE_STRUCT(API())
    struct SE_API_RUNTIME ProfilerGpuEvent
    {
        SCRIPTING_TYPE_MIN(ProfilerGpuEvent)
        SE_FIELD(API())
        int32 NameOffset = 0;
        SE_FIELD(API())
        int32 Depth = 0;
        SE_FIELD(API())
        int32 ParentIndex = -1;
        SE_FIELD(API())
        int32 Complete = 0;
        SE_FIELD(API())
        int32 TimingValid = 0;
        SE_FIELD(API())
        int32 StatsValid = 0;
        SE_FIELD(API())
        float DurationMs = -1;
        SE_FIELD(API())
        float ParentPercent = -1;
        SE_FIELD(API())
        int64 DrawCalls = 0;
        SE_FIELD(API())
        int64 DispatchCalls = 0;
        SE_FIELD(API())
        int64 Vertices = 0;
        SE_FIELD(API())
        int64 Triangles = 0;
        SE_FIELD(API())
        int64 PipelineStateChanges = 0;
    };

    SE_STRUCT(API())
    struct SE_API_RUNTIME ProfilerGpuViewInfo
    {
        SCRIPTING_TYPE_MIN(ProfilerGpuViewInfo)
        SE_FIELD(API())
        uint64 SessionId = 0;
        SE_FIELD(API())
        uint64 SnapshotId = 0;
        SE_FIELD(API())
        uint64 SelectedFrameId = 0;
        SE_FIELD(API())
        int32 SelectedIndex = -1;
    };

    SE_STRUCT(API())
    struct SE_API_RUNTIME ProfilerMemorySample
    {
        SCRIPTING_TYPE_MIN(ProfilerMemorySample)
        SE_FIELD(API())
        double TimeSeconds = 0;
        // OS counters are read by the collector later than the submitted frame boundary.
        SE_FIELD(API())
        double NativeReadSeconds = 0;
        SE_FIELD(API())
        uint64 FrameId = 0;
        SE_FIELD(API())
        uint64 ProcessPhysicalBytes = 0;
        SE_FIELD(API())
        uint64 ProcessPrivateBytes = 0;
        SE_FIELD(API())
        uint64 SystemTotalBytes = 0;
        SE_FIELD(API())
        uint64 SystemUsedBytes = 0;
        SE_FIELD(API())
        uint64 GpuTotalBytes = 0;
        SE_FIELD(API())
        uint64 GpuUsedBytes = 0;
        SE_FIELD(API())
        int32 ValidMask = 0;
        SE_FIELD(API())
        int32 Gc0 = 0;
        SE_FIELD(API())
        int32 Gc1 = 0;
        SE_FIELD(API())
        int32 Gc2 = 0;
        SE_FIELD(API())
        int64 ManagedHeapBytes = 0;
        SE_FIELD(API())
        int64 ManagedAllocatedBytes = 0;
        SE_FIELD(API())
        double ManagedReceiptSeconds = -1;
        SE_FIELD(API())
        double ManagedReadSeconds = 0;
    };

    SE_STRUCT(API())
    struct SE_API_RUNTIME ProfilerMemoryOverviewInfo
    {
        SCRIPTING_TYPE_MIN(ProfilerMemoryOverviewInfo)
        SE_FIELD(API())
        uint64 SessionId = 0;
        SE_FIELD(API())
        uint64 SnapshotId = 0;
        SE_FIELD(API())
        int32 TotalSamples = 0;
        SE_FIELD(API())
        uint64 First = 0;
        SE_FIELD(API())
        uint64 OldestSample = 0;
        SE_FIELD(API())
        int32 Count = 0;
    };

} // namespace SE
