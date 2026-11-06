#pragma once

#include <Runtime/Core/Scripting/ScriptingTypeDefine.h>
#include <Runtime/API.h>

#include <Runtime/Core/Platform/CriticalSection.h>
#include <Runtime/Core/Platform/MemoryStats.h>
#include <Runtime/Core/Profiler/Profiler.h>
#include <Runtime/Core/Systems.h>
#include <Runtime/Core/Thread/JobSystem.h>
#include <Runtime/Core/Types/Collections/Dictionary.h>

namespace tracy
{
    class Worker;
}

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
        ReceiverEnded
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
        int32 Reserved = 0;
    };

    SE_CLASS(API(Static))
    class SE_API_RUNTIME Profiling
    {
        SCRIPTING_TYPE_MIN(Profiling)

    public:
        // Entry points require the registered system and execute its main-thread operations.

        /// Main-thread only. Returns Busy unless Idle, NotReady before initialization, or Unavailable
        /// when the editor profiler is disabled. Success starts connecting; retained history is never overwritten.
        SE_FUNCTION(API())
        static ProfilerCaptureResult StartCapture();
        /// Main-thread only. Cancels an unconfirmed connection, otherwise requests a stop and retains history.
        /// Success is a request, not completion; poll the state. NotReady means no stoppable capture exists.
        SE_FUNCTION(API())
        static ProfilerCaptureResult StopCapture();
        /// Main-thread only. Returns Busy while connecting/capturing/stopping/clearing.
        /// Disposes stopped or failed captures asynchronously; wait for Idle before starting again.
        SE_FUNCTION(API())
        static ProfilerCaptureResult ClearCapture();
        /// Main-thread only. Copies the latest known state into caller-owned storage; no release is needed.
        /// A busy Worker keeps its previous sample. Disabled profiling is represented by Unavailable state.
        /// WrongThread resets the output to an empty record; no Tracy pointers are returned.
        SE_FUNCTION(API())
        static ProfilerCaptureResult GetCaptureInfo(SE_PARAM(API(Out)) ProfilerCaptureInfo& info);
        /// Main-thread only. Reads a complete, session-local frame into caller-owned storage, without allocation.
        /// Returns Busy if Tracy is writing, InvalidSession after Clear/cancel, InvalidFrame outside the recorded
        /// complete intervals, or NotReady before capture acceptance. Every failure resets the output record.
        SE_FUNCTION(API())
        static ProfilerCaptureResult
        ReadCaptureFrame(uint64 sessionId, uint64 index, SE_PARAM(API(Out)) ProfilerCaptureFrame& frame);

        /// Main-thread only. Atomically replaces the one current frame-view snapshot and copies its values.
        /// firstFrame == uint64(-1) explicitly requests the latest page; selectionCount == 0 selects its last
        /// complete frame. Other selections must fit in the requested page. count must be 1..256.
        /// A page is clipped only at the recorded end; TotalFrames is the full frozen count.
        /// A request above 256 frames returns QueryTooLarge; malformed ranges return InvalidRange.
        /// Busy/invalid requests preserve the old snapshot. Failures zero info and clear the output list.
        /// Generated bindings own the temporary result conversion; releasing it leaves the snapshot intact.
        SE_FUNCTION(API())
        static ProfilerCaptureResult RefreshFrameView(uint64                                         sessionId,
                                                      uint64                                         firstFrame,
                                                      int32                                          count,
                                                      uint64                                         selectionFirst,
                                                      int32                                          selectionCount,
                                                      SE_PARAM(API(Out)) ProfilerFrameViewInfo&      info,
                                                      SE_PARAM(API(Out)) List<ProfilerCaptureFrame>& frames);
        /// Main-thread only. Copies an existing immutable snapshot without reading live Tracy data.
        /// Refresh, Clear, cancellation and exit invalidate the previous identity; it returns InvalidSnapshot.
        /// A foreign/cleared session returns InvalidSession. The bounded output list is independent of history;
        /// generated bindings manage its transfer lifetime. Failures zero info and clear the output list.
        SE_FUNCTION(API())
        static ProfilerCaptureResult ReadFrameView(uint64                                         sessionId,
                                                   uint64                                         snapshotId,
                                                   SE_PARAM(API(Out)) ProfilerFrameViewInfo&      info,
                                                   SE_PARAM(API(Out)) List<ProfilerCaptureFrame>& frames);
    };

    class SE_API_RUNTIME ProfilingSystem : public ISystem
    {
        ENGINE_SYSTEM(ProfilingSystem)
    public:
        using CaptureState                        = ProfilerCaptureState;
        using StopReason                          = ProfilerStopReason;
        using CaptureResult                       = ProfilerCaptureResult;
        using CaptureInfo                         = ProfilerCaptureInfo;
        using CaptureFrame                        = ProfilerCaptureFrame;
        using FrameViewInfo                       = ProfilerFrameViewInfo;
        static constexpr int32 MaxFrameViewFrames = 256;
        // Candidate soft threshold. Update sampling and receiver shutdown can overshoot it.
        static constexpr int64 CaptureMemoryLimitBytes = 512ll * 1024 * 1024;

        CaptureResult StartCapture();
        CaptureResult StopCapture();
        CaptureResult ClearCapture();
        CaptureResult GetCaptureInfo(CaptureInfo& info);
        CaptureResult ReadCaptureFrame(uint64 sessionId, uint64 index, CaptureFrame& frame);
        CaptureResult RefreshFrameView(uint64              sessionId,
                                       uint64              firstFrame,
                                       int32               count,
                                       uint64              selectionFirst,
                                       int32               selectionCount,
                                       FrameViewInfo&      info,
                                       List<CaptureFrame>& frames);
        CaptureResult
        ReadFrameView(uint64 sessionId, uint64 snapshotId, FrameViewInfo& info, List<CaptureFrame>& frames);

        /// <summary>
        /// The GPU memory stats.
        /// </summary>
        struct MemoryStatsGPU
        {
            /// <summary>
            /// The total amount of memory in bytes (as reported by the driver).
            /// </summary>
            uint64 Total;

            /// <summary>
            /// The used by the game amount of memory in bytes (estimated).
            /// </summary>
            uint64 Used;
        };

        /// <summary>
        /// Engine profiling data header. Contains main info and stats.
        /// </summary>
        struct MainStats
        {
            /// <summary>
            /// The process memory stats.
            /// </summary>
            ProcessMemoryStats ProcessMemory;

            /// <summary>
            /// The CPU memory stats.
            /// </summary>
            MemoryStats MemoryCPU;

            /// <summary>
            /// The GPU memory stats.
            /// </summary>
            MemoryStatsGPU MemoryGPU;

            /// <summary>
            /// The frames per second (fps counter).
            /// </summary>
            int32 FPS;

            /// <summary>
            /// The update time on CPU (in milliseconds).
            /// </summary>
            float UpdateTimeMs;

            /// <summary>
            /// The fixed update time on CPU (in milliseconds).
            /// </summary>
            float PhysicsTimeMs;

            /// <summary>
            /// The draw time on CPU (in milliseconds).
            /// </summary>
            float DrawCPUTimeMs;

            /// <summary>
            /// The draw time on GPU (in milliseconds).
            /// </summary>
            float DrawGPUTimeMs;

            /// <summary>
            /// The last rendered frame stats.
            /// </summary>
            RenderStatsData DrawStats;
        };

        /// <summary>
        /// The CPU thread stats.
        /// </summary>
        struct ThreadStats
        {
            /// <summary>
            /// The thread name.
            /// </summary>
            String Name;

            /// <summary>
            /// The events list.
            /// </summary>
            List<ProfilerCPU::Event> Events;
        };

        /// <summary>
        /// The network stat.
        /// </summary>
        struct NetworkEventStat
        {
            // Amount of occurrences.
            uint16 Count;
            // Transferred data size (in bytes).
            uint16 DataSize;
            // Transferred message (data+header) size (in bytes).
            uint16 MessageSize;
            // Amount of peers that will receive this message.
            uint16 Receivers;
            byte   Name[120];
        };

    public:
        /// <summary>
        /// Controls the engine profiler (CPU, GPU, etc.) usage.
        /// </summary>
        static bool GetEnabled();

        /// <summary>
        /// Controls the engine profiler (CPU, GPU, etc.) usage.
        /// </summary>
        static void SetEnabled(bool enabled);

        /// <summary>
        /// The current collected main stats by the profiler from the local session. Updated every frame.
        /// </summary>
        static MainStats Stats;

        /// <summary>
        /// The CPU threads profiler events.
        /// </summary>
        static List<ThreadStats, InlinedAllocation<64>> EventsCPU;

        /// <summary>
        /// The GPU rendering profiler events.
        /// </summary>
        static List<ProfilerGPU::Event> EventsGPU;

        /// <summary>
        /// The networking profiler events.
        /// </summary>
        static List<NetworkEventStat> EventsNetwork;

    public:
        ProfilingSystem();
        ~ProfilingSystem() override;

        void OnUpdate() override;

        // Explicit legacy local-stat collection; Worker capture does not read live CPU ring buffers.
        void Update();

        void OnDispose() override;

        bool OnInit() override;

    private:
        void            UpdateCaptureState();
        CaptureResult   BeginCaptureCleanup(CaptureState targetState);
        CaptureInfo     m_CaptureStateData;
        CriticalSection m_CaptureLocker;
        uint64          m_CaptureSequence    = 0;
        bool            m_CaptureInitialized = false;
        FrameViewInfo   m_CurrentFrameView;
        CaptureFrame    m_CurrentViewFrames[MaxFrameViewFrames];
        uint64          m_FrameViewSequence = 0;
#ifdef SE_TRACY_SERVER
        UniquePtr<tracy::Worker> m_CaptureWorker;
        Threading::JobHandle     m_CaptureCleanup         = 0;
        CaptureState             m_CleanupTarget          = CaptureState::Idle;
        double                   m_CaptureConnectionStart = 0;
#endif
    };

} // namespace SE
