#pragma once

#include <Runtime/Core/Scripting/ScriptingTypeDefine.h>
#include <Runtime/API.h>

#include <Runtime/Core/Platform/CriticalSection.h>
#include <Runtime/Core/Platform/MemoryStats.h>
#include <Runtime/Core/Profiler/Profiler.h>
#include <Runtime/Core/Systems.h>
#include <Runtime/Core/Thread/JobSystem.h>
#include <Runtime/Core/Types/Collections/Dictionary.h>
#include <Runtime/Core/Profiler/ProfilerStats.h>

namespace tracy
{
    class Worker;
}

namespace SE
{

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
        static constexpr int32 MaxCpuEvents       = 65536;
        static constexpr int32 MaxCpuThreads      = 256;
        static constexpr int32 MaxCpuDepth        = 128;
        static constexpr int32 MaxCpuTextBytes    = 1024 * 1024;
        // Candidate soft threshold. Update sampling and receiver shutdown can overshoot it.
        static constexpr int64 CaptureMemoryLimitBytes = 512ll * 1024 * 1024;

        CaptureResult StartCapture();
        CaptureResult SubmitManagedMemory(uint64 sessionId,
                                          int64  heapBytes,
                                          int64  allocatedBytes,
                                          int32  gc0,
                                          int32  gc1,
                                          int32  gc2,
                                          double readSeconds);

        CaptureResult RefreshMemoryOverview(uint64                      sessionId,
                                            int32                       first,
                                            ProfilerCaptureInfo&        capture,
                                            ProfilerMemoryOverviewInfo& info,
                                            List<ProfilerMemorySample>& samples);

        CaptureResult RefreshAllocationView(uint64                      sessionId,
                                            int64                       beginNs,
                                            int64                       endNs,
                                            int64                       inspectNs,
                                            int32                       filter,
                                            uint64                      address,
                                            int32                       searchAddress,
                                            ProfilerCaptureInfo&        capture,
                                            ProfilerAllocationViewInfo& info,
                                            List<ProfilerMemoryBucket>& buckets,
                                            List<ProfilerAllocation>&   page);

        CaptureResult ReadAllocationPage(uint64                      sessionId,
                                         uint64                      snapshotId,
                                         int32                       first,
                                         ProfilerAllocationPageInfo& info,
                                         List<ProfilerAllocation>&   page);

        CaptureResult ReadAllocationDetails(uint64                              sessionId,
                                            uint64                              snapshotId,
                                            uint64                              poolId,
                                            uint64                              recordId,
                                            ProfilerAllocation&                 allocation,
                                            List<ProfilerAllocationStackFrame>& frames,
                                            List<byte>&                         text);
        CaptureResult StopCapture();
        CaptureResult ClearCapture();
        CaptureResult GetCaptureInfo(CaptureInfo& info);
        CaptureResult RefreshGpuView(uint64 sessionId, uint64 frameId, CaptureInfo& capture, ProfilerGpuViewInfo& info, List<ProfilerGpuFrame>& frames, List<ProfilerGpuEvent>& events, List<byte>& text);
        CaptureResult ReadCaptureFrame(uint64 sessionId, uint64 index, CaptureFrame& frame);

        CaptureResult RefreshFrameView(uint64                      sessionId,
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
                                       List<ProfilerCpuStatistic>& statistics);

        CaptureResult ReadFrameView(uint64 sessionId, uint64 snapshotId, FrameViewInfo& info, List<CaptureFrame>& frames);
        CaptureResult ReadCaptureThreads(uint64 sessionId, int32 first, int32& total, List<ProfilerCpuThread>& threads, List<byte>& text);

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
        static constexpr int32 MaxMemorySamples     = 36000;
        static constexpr int32 MaxAllocationScan    = 1048576;
        static constexpr int32 MaxAllocationRecords = 262144;
        static constexpr int32 MaxMemoryBuckets     = 2048;
        static constexpr int32 MaxAllocationPage    = 256;
        void                   SampleMemory();
        void                   ResetMemoryHistory();
        void                   ResetMemoryView();
        struct MemoryChange
        {
            int64  TimeNs = 0;
            uint64 Size   = 0;
            bool   Free   = false;
            bool   operator<(const MemoryChange& other) const { return TimeNs < other.TimeNs; }
        };
        struct AllocationOrder
        {
            int64  TimeNs   = 0;
            uint64 PoolId   = 0;
            uint64 RecordId = 0;
            int32  Index    = 0;
            bool   operator<(const AllocationOrder& other) const
            {
                return TimeNs != other.TimeNs ? TimeNs < other.TimeNs :
                       PoolId != other.PoolId ? PoolId < other.PoolId :
                                                RecordId < other.RecordId;
            }
        };
        double                     m_MemoryStart      = 0;
        double                     m_LastMemorySample = -1;
        ProfilerMemorySample       m_ManagedMemory;
        List<ProfilerMemorySample> m_MemoryHistory;
        ProfilerMemoryOverviewInfo m_CurrentMemoryOverview;
        List<ProfilerMemorySample> m_CurrentMemorySamples;
        ProfilerAllocationViewInfo m_CurrentAllocationView;
        List<ProfilerAllocation>   m_CurrentAllocations;
        List<int32>                m_MatchingAllocations;
        List<ProfilerMemoryBucket> m_CurrentMemoryBuckets;
        struct GpuHistoryFrame
        {
            ProfilerGpuFrame Info;
            int32            FirstEvent = 0;
            int32            FirstText  = 0;
            int32            TextLength = 0;
        };
        static constexpr int32    MaxGpuFrames    = 2048;
        static constexpr int32    MaxGpuEvents    = 65536;
        static constexpr int32    MaxGpuTextBytes = 4 * 1024 * 1024;
        void                      ResetGpuHistory();
        void                      OnGpuSubmitted(uint64 frameId, const ProfilerGPU::EventBuffer* pFrame);
        void                      OnGpuResolved(const ProfilerGPU::EventBuffer& frame);
        void                      StopForGpuCapacity();
        uint64                    m_FirstGpuFrame = 0;
        List<GpuHistoryFrame>     m_GpuHistoryFrames;
        Dictionary<uint64, int32> m_GpuFrameIndices;
        List<ProfilerGpuEvent>    m_GpuHistoryEvents;
        List<byte>                m_GpuHistoryText;
        ProfilerGpuViewInfo       m_CurrentGpuView;
        List<ProfilerGpuFrame>    m_CurrentGpuFrames;
        List<ProfilerGpuEvent>    m_CurrentGpuEvents;
        List<byte>                m_CurrentGpuText;
        struct CpuChildCoverage
        {
            int64  EndNs      = 0;
            uint64 CoveredNs  = 0;
            bool   Incomplete = false;
        };
        void                       BuildCpuStatistics(int64 beginNs, int64 endNs);
        List<ProfilerCpuStatistic> m_CurrentCpuStatistics;
        List<ProfilerCpuStatistic> m_PendingCpuStatistics;
        List<int32>                m_SourceStatisticIndices;
        List<CpuChildCoverage>     m_CpuChildCoverage;
        void                       ResetView();
        // Called only with the engine lock and Tracy data lock held; buffers are reserved beforehand.
        CaptureResult           CopyCpuView(int64 beginNs, int64 endNs, uint64 threadId, int32& incomplete);
        int32                   CopyCpuText(const char* pText);
        List<ProfilerCpuThread> m_CurrentCpuThreads;
        List<ProfilerCpuEvent>  m_CurrentCpuEvents;
        List<byte>              m_CurrentCpuText;
        List<ProfilerCpuThread> m_PendingCpuThreads;
        List<ProfilerCpuEvent>  m_PendingCpuEvents;
        List<byte>              m_PendingCpuText;
        void                    UpdateCaptureState();
        CaptureResult           BeginCaptureCleanup(CaptureState targetState);
        CaptureInfo             m_CaptureStateData;
        CriticalSection         m_CaptureLocker;
        uint64                  m_CaptureSequence    = 0;
        bool                    m_CaptureInitialized = false;
        FrameViewInfo           m_CurrentFrameView;
        CaptureFrame            m_CurrentViewFrames[MaxFrameViewFrames];
        uint64                  m_FrameViewSequence = 0;
#ifdef SE_TRACY_SERVER
        UniquePtr<tracy::Worker> m_CaptureWorker;
        Threading::JobHandle     m_CaptureCleanup         = 0;
        CaptureState             m_CleanupTarget          = CaptureState::Idle;
        double                   m_CaptureConnectionStart = 0;
#endif
    };

    SE_CLASS(API(Static))
    class SE_API_RUNTIME Profiling
    {
        SCRIPTING_TYPE_MIN(Profiling)

    public:
        // Entry points require the registered system and execute its main-thread operations.
        /// Main-thread only, Capturing session only. Records independently read CLR process counters, never requests
        /// GC. readSeconds is the managed read duration; receipt elapsed time is recorded separately from native
        /// samples. Invalid values return InvalidRange; this operation does not replace an analysis snapshot.
        SE_FUNCTION(API())
        static ProfilerCaptureResult SubmitManagedMemory(uint64 sessionId,
                                                         int64  heapBytes,
                                                         int64  allocatedBytes,
                                                         int32  gc0,
                                                         int32  gc1,
                                                         int32  gc2,
                                                         double readSeconds);

        /// Main-thread only. first == -1 requests the latest 256 samples; otherwise pages the retained engine history.
        /// Returns owned values with per-source validity (1 process, 2 system, 4 GPU, 8 managed).
        /// Engine elapsed and managed receipt times are not Tracy times or atomic simultaneous measurements.
        /// Success replaces the current CPU/GPU/memory snapshot. Invalid requests preserve the prior snapshot.
        SE_FUNCTION(API())
        static ProfilerCaptureResult RefreshMemoryOverview(uint64                                         sessionId,
                                                           int32                                          first,
                                                           SE_PARAM(API(Out)) ProfilerCaptureInfo&        capture,
                                                           SE_PARAM(API(Out)) ProfilerMemoryOverviewInfo& info,
                                                           SE_PARAM(API(Out)) List<ProfilerMemorySample>& samples);

        /// Main-thread only. Copies every recorded pool under the Worker lock, then computes an immutable result.
        /// Times are Tracy nanoseconds: range [beginNs,endNs), inspection inclusive of allocation/exclusive of free.
        /// -1 requests latest end, latest ten-second begin, or inspection at end. filter: 0 overlapping, 1 active, 2
        /// allocated in range. searchAddress == 1 filters exact uint64 addresses. Statistics/curve are before
        /// list/address filtering. Scan/record limits return QueryTooLarge with no truncation. Busy/failure clears
        /// outputs and preserves old snapshot. Success replaces the current analysis identity; owned bucket envelope
        /// and first 256 rows share that snapshot.
        SE_FUNCTION(API())
        static ProfilerCaptureResult RefreshAllocationView(uint64                                         sessionId,
                                                           int64                                          beginNs,
                                                           int64                                          endNs,
                                                           int64                                          inspectNs,
                                                           int32                                          filter,
                                                           uint64                                         address,
                                                           int32                                          searchAddress,
                                                           SE_PARAM(API(Out)) ProfilerCaptureInfo&        capture,
                                                           SE_PARAM(API(Out)) ProfilerAllocationViewInfo& info,
                                                           SE_PARAM(API(Out)) List<ProfilerMemoryBucket>& buckets,
                                                           SE_PARAM(API(Out)) List<ProfilerAllocation>&   page);
        /// Main-thread only. Copies up to 256 rows of the current frozen matching set, without reading live Worker
        /// records. first is a matching-row offset. Session/snapshot replacement or Clear returns
        /// InvalidSession/InvalidSnapshot. Failures clear outputs; releasing the generated transfer arrays leaves the
        /// native snapshot intact.
        SE_FUNCTION(API())
        static ProfilerCaptureResult ReadAllocationPage(uint64                                         sessionId,
                                                        uint64                                         snapshotId,
                                                        int32                                          first,
                                                        SE_PARAM(API(Out)) ProfilerAllocationPageInfo& info,
                                                        SE_PARAM(API(Out)) List<ProfilerAllocation>&   page);

        /// Main-thread only. Uses frozen pool/record identity and lifecycle, then copies currently available stack
        /// symbols. Empty CallstackId means none recorded in this snapshot; unresolved frames retain native addresses.
        /// No wait for symbols; explicit rereads can update symbols without replacing the query or pagination.
        /// Result is bounded to 256 expanded frames and 256 KiB UTF-8; QueryTooLarge never returns a partial stack.
        /// Failures clear outputs and preserve the current snapshot. Generated bindings own transfer copies.
        SE_FUNCTION(API())
        static ProfilerCaptureResult ReadAllocationDetails(uint64                                 sessionId,
                                                           uint64                                 snapshotId,
                                                           uint64                                 poolId,
                                                           uint64                                 recordId,
                                                           SE_PARAM(API(Out)) ProfilerAllocation& allocation,
                                                           SE_PARAM(API(Out))
                                                               List<ProfilerAllocationStackFrame>& frames,
                                                           SE_PARAM(API(Out)) List<byte>&          text);

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

        /// Main-thread only. Copies a frozen GPU catalogue and selected frame in one call without polling queries.
        /// frameId == uint64(-1) follows the latest resolved sample, otherwise chooses that exact engine frame.
        /// Missing frames return InvalidFrame and retain the prior snapshot. Successful page changes invalidate
        /// prior CPU/GPU snapshot identities. Pending results contain metadata, never fabricated durations.
        /// GPU engine frame IDs have no inferred mapping to Tracy local frame IDs. Text is owned UTF-8.
        SE_FUNCTION(API())
        static ProfilerCaptureResult RefreshGpuView(uint64                                     sessionId,
                                                    uint64                                     frameId,
                                                    SE_PARAM(API(Out)) ProfilerCaptureInfo&    capture,
                                                    SE_PARAM(API(Out)) ProfilerGpuViewInfo&    info,
                                                    SE_PARAM(API(Out)) List<ProfilerGpuFrame>& frames,
                                                    SE_PARAM(API(Out)) List<ProfilerGpuEvent>& events,
                                                    SE_PARAM(API(Out)) List<byte>&             text);

        /// Main-thread only. Reads a complete, session-local frame into caller-owned storage, without allocation.
        /// Returns Busy if Tracy is writing, InvalidSession after Clear/cancel, InvalidFrame outside the recorded
        /// complete intervals, or NotReady before capture acceptance. Every failure resets the output record.
        SE_FUNCTION(API())
        static ProfilerCaptureResult ReadCaptureFrame(uint64 sessionId, uint64 index, SE_PARAM(API(Out)) ProfilerCaptureFrame& frame);

        /// Main-thread only. Atomically replaces the frame/CPU snapshot and returns a batched capture sample.
        /// firstFrame == uint64(-1) explicitly requests the latest page; selectionCount == 0 selects its last
        /// complete frame. Other selections must fit in the requested page. count must be 1..256.
        /// A page is clipped only at the recorded end; TotalFrames is the full frozen count.
        /// A request above 256 frames returns QueryTooLarge; malformed ranges return InvalidRange.
        /// CPU data covers the selected intervals; threadId == 0 includes all threads. Event identity is
        /// (SessionId, SnapshotId, array index). ParentIndex references the same event array, or -1.
        /// UTF-8 text offsets reference the owned, null-terminated text result; zero denotes empty text.
        /// EndValid == 0 means EndNs is unknown, never an inferred duration.
        /// Busy/invalid/CPU-capacity failures preserve the old snapshot and clear view outputs.
        /// capture receives the latest state even on query failure, except WrongThread/Unavailable.
        /// Generated bindings own the temporary result conversion; releasing it leaves the snapshot intact.
        SE_FUNCTION(API())
        static ProfilerCaptureResult RefreshFrameView(uint64                                         sessionId,
                                                      uint64                                         firstFrame,
                                                      int32                                          count,
                                                      uint64                                         selectionFirst,
                                                      int32                                          selectionCount,
                                                      uint64                                         threadId,
                                                      SE_PARAM(API(Out)) ProfilerCaptureInfo&        capture,
                                                      SE_PARAM(API(Out)) ProfilerFrameViewInfo&      info,
                                                      SE_PARAM(API(Out)) List<ProfilerCaptureFrame>& frames,
                                                      SE_PARAM(API(Out)) List<ProfilerCpuThread>&    threads,
                                                      SE_PARAM(API(Out)) List<ProfilerCpuEvent>&     events,
                                                      SE_PARAM(API(Out)) List<byte>&                 text,
                                                      SE_PARAM(API(Out)) List<ProfilerCpuStatistic>& statistics);

        /// Main-thread only. Copies an existing immutable snapshot without reading live Tracy data.
        /// Refresh, Clear, cancellation and exit invalidate the previous identity; it returns InvalidSnapshot.
        /// A foreign/cleared session returns InvalidSession. The bounded output list is independent of history;
        /// generated bindings manage its transfer lifetime. Failures zero info and clear the output list.
        SE_FUNCTION(API())
        static ProfilerCaptureResult ReadFrameView(uint64                                         sessionId,
                                                   uint64                                         snapshotId,
                                                   SE_PARAM(API(Out)) ProfilerFrameViewInfo&      info,
                                                   SE_PARAM(API(Out)) List<ProfilerCaptureFrame>& frames);

        /// Main-thread only. Copies a directory page of up to 256 thread IDs/names for explicit filtering,
        /// including before the first successful CPU query. This is not an analysis snapshot.
        /// first is a directory index; total is the full directory size. Busy/invalid failures clear outputs.
        /// UTF-8 text is caller-owned and uses the same offset convention as the CPU result.
        SE_FUNCTION(API())
        static ProfilerCaptureResult ReadCaptureThreads(uint64                                      sessionId,
                                                        int32                                       first,
                                                        SE_PARAM(API(Out)) int32&                   total,
                                                        SE_PARAM(API(Out)) List<ProfilerCpuThread>& threads,
                                                        SE_PARAM(API(Out)) List<byte>&              text);
    };

} // namespace SE
