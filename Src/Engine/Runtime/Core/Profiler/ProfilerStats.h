#pragma once

#include <Runtime/API.h>
#include <Runtime/Core/Scripting/ScriptingType.h>


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
        Connecting,
        Capturing,
        Stopping,
        Stopped,
        Clearing,
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
        WrongProcess,
        ConnectionFailed,
        ProtocolMismatch,
        ClientUnavailable,
        TimedOut,
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
        CapacityLimit,
        ReceiverEnded,
        GpuHistoryCapacity,
        MemoryHistoryCapacity
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
        // Global Tracy Server counted allocations, not process memory or an isolated Worker total.
        SE_FIELD(API())
        int64 ServerMemoryBytes = 0;
        SE_FIELD(API())
        int64 MemoryLimitBytes = 0;
        // Sample latched when the engine requested a capacity stop; zero for other stop reasons.
        SE_FIELD(API())
        int64 CapacityStopBytes = 0;
        SE_FIELD(API())
        int64 CompleteBeginNs = 0;
        SE_FIELD(API())
        int64 CompleteEndNs = 0;
        SE_FIELD(API())
        int32 GpuFrames = 0;
        SE_FIELD(API())
        int32 GpuPending = 0;
        SE_FIELD(API())
        int32 GpuHistoryLimit = 0;
        SE_FIELD(API())
        int32 ReservedGpu = 0;
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
    // RepresentativeEvent is the largest clipped complete instance in the same event array, or -1.
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
        CapacityExceeded,
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
        int32 First = 0;
        SE_FIELD(API())
        int32 Count = 0;
        SE_FIELD(API())
        int32 CapacityReached = 0;
    };

    SE_STRUCT(API())
    struct SE_API_RUNTIME ProfilerAllocation
    {
        SCRIPTING_TYPE_MIN(ProfilerAllocation)
        SE_FIELD(API())
        uint64 PoolId = 0;
        SE_FIELD(API())
        uint64 RecordId = 0;
        SE_FIELD(API())
        uint64 Address = 0;
        SE_FIELD(API())
        uint64 Size = 0;
        SE_FIELD(API())
        int64 AllocNs = 0;
        SE_FIELD(API())
        int64 FreeNs = -1;
        SE_FIELD(API())
        uint64 AllocThread = 0;
        SE_FIELD(API())
        uint64 FreeThread = 0;
        SE_FIELD(API())
        uint32 CallstackId = 0;
        SE_FIELD(API())
        int32 ActiveAtInspection = 0;
    };

    SE_STRUCT(API())
    struct SE_API_RUNTIME ProfilerMemoryBucket
    {
        SCRIPTING_TYPE_MIN(ProfilerMemoryBucket)
        SE_FIELD(API())
        int64 BeginNs = 0;
        SE_FIELD(API())
        int64 EndNs = 0;
        SE_FIELD(API())
        uint64 MinBytes = 0;
        SE_FIELD(API())
        uint64 MaxBytes = 0;
        SE_FIELD(API())
        uint64 EndBytes = 0;
        SE_FIELD(API())
        uint64 Allocations = 0;
        SE_FIELD(API())
        uint64 Frees = 0;
    };

    SE_STRUCT(API())
    struct SE_API_RUNTIME ProfilerAllocationViewInfo
    {
        SCRIPTING_TYPE_MIN(ProfilerAllocationViewInfo)
        SE_FIELD(API())
        uint64 SessionId = 0;
        SE_FIELD(API())
        uint64 SnapshotId = 0;
        SE_FIELD(API())
        int64 BeginNs = 0;
        SE_FIELD(API())
        int64 EndNs = 0;
        SE_FIELD(API())
        int64 InspectNs = 0;
        SE_FIELD(API())
        int64 RecordedUntilNs = 0;
        SE_FIELD(API())
        uint64 RecordedAllocations = 0;
        SE_FIELD(API())
        uint64 RangeAllocations = 0;
        SE_FIELD(API())
        uint64 RangeAllocatedBytes = 0;
        SE_FIELD(API())
        uint64 RangeFrees = 0;
        SE_FIELD(API())
        uint64 RangeFreedBytes = 0;
        SE_FIELD(API())
        uint64 ActiveAllocations = 0;
        SE_FIELD(API())
        uint64 ActiveBytes = 0;
        SE_FIELD(API())
        int32 MatchingCount = 0;
        SE_FIELD(API())
        int32 FrozenCount = 0;
        SE_FIELD(API())
        int32 Filter = 0;
        SE_FIELD(API())
        int32 Reserved = 0;
    };

    SE_STRUCT(API())
    struct SE_API_RUNTIME ProfilerAllocationPageInfo
    {
        SCRIPTING_TYPE_MIN(ProfilerAllocationPageInfo)
        SE_FIELD(API())
        uint64 SessionId = 0;
        SE_FIELD(API())
        uint64 SnapshotId = 0;
        SE_FIELD(API())
        int32 First = 0;
        SE_FIELD(API())
        int32 Total = 0;
        SE_FIELD(API())
        int32 Count = 0;
        SE_FIELD(API())
        int32 HasMore = 0;
    };

    SE_STRUCT(API())
    struct SE_API_RUNTIME ProfilerAllocationStackFrame
    {
        SCRIPTING_TYPE_MIN(ProfilerAllocationStackFrame)
        SE_FIELD(API())
        uint64 Address = 0;
        SE_FIELD(API())
        int32 NameOffset = 0;
        SE_FIELD(API())
        int32 FileOffset = 0;
        SE_FIELD(API())
        int32 ImageOffset = 0;
        SE_FIELD(API())
        uint32 Line = 0;
        SE_FIELD(API())
        int32 Resolved = 0;
        SE_FIELD(API())
        int32 InlineDepth = 0;
    };

}