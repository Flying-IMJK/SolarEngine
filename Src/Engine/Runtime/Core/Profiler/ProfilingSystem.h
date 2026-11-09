#pragma once

#include <Runtime/Core/Scripting/ScriptingTypeDefine.h>
#include <Runtime/API.h>

#include <Runtime/Core/Platform/CriticalSection.h>
#include <Runtime/Core/Platform/MemoryStats.h>
#include <Runtime/Core/Profiler/Profiler.h>
#include <Runtime/Core/Profiler/ProfilerStats.h>
#include <Runtime/Core/Systems.h>

namespace SE
{
    struct ProfilerLocalData;

    class SE_API_RUNTIME ProfilingSystem : public ISystem
    {
        ENGINE_SYSTEM(ProfilingSystem)
    public:
        using CaptureState  = ProfilerCaptureState;
        using StopReason    = ProfilerStopReason;
        using CaptureResult = ProfilerCaptureResult;
        using CaptureInfo   = ProfilerCaptureInfo;
        using CaptureFrame  = ProfilerCaptureFrame;
        using FrameViewInfo = ProfilerFrameViewInfo;

        // The single capacity setting for CPU, GPU and memory sample histories.
        static constexpr int32 MaxSamples         = 600;
        static constexpr int32 MaxFrameViewFrames = 256;
        static constexpr int32 MaxCpuEvents       = 65536;
        static constexpr int32 MaxCpuThreads      = 256;
        static constexpr int32 MaxCpuDepth        = 128;
        static constexpr int32 MaxCpuTextBytes    = 1024 * 1024;

        CaptureResult StartCapture();
        CaptureResult SubmitManagedMemory(uint64 sessionId,
                                          int64  heapBytes,
                                          int64  allocatedBytes,
                                          int32  gc0,
                                          int32  gc1,
                                          int32  gc2,
                                          double readSeconds);

        CaptureResult RefreshMemoryOverview(uint64                      sessionId,
                                            int64                       first,
                                            ProfilerCaptureInfo&        capture,
                                            ProfilerMemoryOverviewInfo& info,
                                            List<ProfilerMemorySample>& samples);

        CaptureResult StopCapture();
        CaptureResult ClearCapture();
        CaptureResult GetCaptureInfo(CaptureInfo& info);
        CaptureResult RefreshGpuView(uint64                  sessionId,
                                     uint64                  frameId,
                                     CaptureInfo&            capture,
                                     ProfilerGpuViewInfo&    info,
                                     List<ProfilerGpuFrame>& frames,
                                     List<ProfilerGpuEvent>& events,
                                     List<byte>&             text);
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

        CaptureResult
        ReadFrameView(uint64 sessionId, uint64 snapshotId, FrameViewInfo& info, List<CaptureFrame>& frames);
        CaptureResult ReadCaptureThreads(uint64                   sessionId,
                                         int32                    first,
                                         int32&                   total,
                                         List<ProfilerCpuThread>& threads,
                                         List<byte>&              text);

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

        void OnDispose() override;

        bool OnInit() override;

    private:
        struct CpuChildCoverage
        {
            int64  EndNs      = 0;
            uint64 CoveredNs  = 0;
            bool   Incomplete = false;
        };
        void                 OnGpuSubmitted(uint64 frameId, const ProfilerGPU::EventBuffer* pFrame);
        void                 OnGpuResolved(const ProfilerGPU::EventBuffer& frame);
        int32                Collect();
        void                 DrainCollector(bool exit);
        void                 PublishHistory();
        void                 ResetMemoryHistory();
        void                 ResetGpuHistory();
        void                 ResetMemoryView();
        void                 ResetView();
        void                 UpdateCaptureState();
        void                 BuildCpuStatistics(int64 beginNs, int64 endNs);
        CaptureResult        CopyCpuView(uint64 firstSample, int32 sampleCount, uint64 threadId, int32& incomplete);
        int32                CopyCpuText(const char* pText);
        ProfilerLocalData*   m_Local = nullptr;
        CaptureInfo          m_CaptureStateData;
        CriticalSection      m_CaptureLocker;
        uint64               m_CaptureSequence    = 0;
        bool                 m_CaptureInitialized = false;
        uint64               m_FrameViewSequence  = 0;
        double               m_MemoryStart        = 0;
        ProfilerMemorySample m_ManagedMemory;
        ProfilerMemoryOverviewInfo m_CurrentMemoryOverview;
        List<ProfilerMemorySample> m_CurrentMemorySamples;
        ProfilerGpuViewInfo        m_CurrentGpuView;
        List<ProfilerGpuFrame>     m_CurrentGpuFrames;
        List<ProfilerGpuEvent>     m_CurrentGpuEvents;
        List<byte>                 m_CurrentGpuText;
        FrameViewInfo              m_CurrentFrameView;
        CaptureFrame               m_CurrentViewFrames[MaxFrameViewFrames];
        List<ProfilerCpuThread>    m_CurrentCpuThreads;
        List<ProfilerCpuEvent>     m_CurrentCpuEvents;
        List<byte>                 m_CurrentCpuText;
        List<ProfilerCpuThread>    m_PendingCpuThreads;
        List<ProfilerCpuEvent>     m_PendingCpuEvents;
        List<byte>                 m_PendingCpuText;
        List<ProfilerCpuStatistic> m_CurrentCpuStatistics;
        List<ProfilerCpuStatistic> m_PendingCpuStatistics;
    };

    SE_CLASS(API(Static))
    class SE_API_RUNTIME Profiling
    {
        SCRIPTING_TYPE_MIN(Profiling)

    public:
        SE_FUNCTION(API())
        static int32 GetHistoryCapacity();

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
        /// Engine elapsed and managed receipt times are not frame boundaries or atomic simultaneous measurements.
        /// Success replaces the current CPU/GPU/memory snapshot. Invalid requests preserve the prior snapshot.
        SE_FUNCTION(API())
        static ProfilerCaptureResult RefreshMemoryOverview(uint64                                         sessionId,
                                                           int64                                          first,
                                                           SE_PARAM(API(Out)) ProfilerCaptureInfo&        capture,
                                                           SE_PARAM(API(Out)) ProfilerMemoryOverviewInfo& info,
                                                           SE_PARAM(API(Out)) List<ProfilerMemorySample>& samples);

        /// Main-thread only. Returns Busy unless Idle, NotReady before initialization, or Unavailable
        /// when the editor profiler is disabled. Success starts local sampling immediately.
        SE_FUNCTION(API())
        static ProfilerCaptureResult StartCapture();

        /// Main-thread only. Stops local sampling immediately and retains history.
        /// Already admitted GPU queries may resolve later. NotReady means no active capture exists.
        SE_FUNCTION(API())
        static ProfilerCaptureResult StopCapture();

        /// Main-thread only. Returns Busy while capturing. Releases retained data and returns to Idle.
        SE_FUNCTION(API())
        static ProfilerCaptureResult ClearCapture();

        /// Main-thread only. Copies the latest known state into caller-owned storage; no release is needed.
        /// Disabled profiling is represented by Unavailable state. WrongThread resets the output record.
        SE_FUNCTION(API())
        static ProfilerCaptureResult GetCaptureInfo(SE_PARAM(API(Out)) ProfilerCaptureInfo& info);

        /// Main-thread only. Copies a frozen GPU catalogue and selected frame in one call without polling queries.
        /// frameId == uint64(-1) follows the latest admitted sample, otherwise chooses that exact engine frame.
        /// Missing frames return InvalidFrame and retain the prior snapshot. Successful page changes invalidate
        /// prior CPU/GPU snapshot identities. Pending results contain metadata, never fabricated durations.
        /// GPU engine frame IDs are independent of CPU sample sequence indices. Text is owned UTF-8.
        SE_FUNCTION(API())
        static ProfilerCaptureResult RefreshGpuView(uint64                                     sessionId,
                                                    uint64                                     frameId,
                                                    SE_PARAM(API(Out)) ProfilerCaptureInfo&    capture,
                                                    SE_PARAM(API(Out)) ProfilerGpuViewInfo&    info,
                                                    SE_PARAM(API(Out)) List<ProfilerGpuFrame>& frames,
                                                    SE_PARAM(API(Out)) List<ProfilerGpuEvent>& events,
                                                    SE_PARAM(API(Out)) List<byte>&             text);

        /// Main-thread only. Reads a complete, session-local frame into caller-owned storage, without allocation.
        /// Returns InvalidSession after Clear, or InvalidFrame outside the retained sample window.
        /// Every failure resets the output record.
        SE_FUNCTION(API())
        static ProfilerCaptureResult
        ReadCaptureFrame(uint64 sessionId, uint64 index, SE_PARAM(API(Out)) ProfilerCaptureFrame& frame);

        /// Main-thread only. Atomically replaces the frame/CPU snapshot and returns a batched capture sample.
        /// firstFrame == uint64(-1) explicitly requests the latest page; selectionCount == 0 selects its last
        /// complete frame. Other selections must fit in the requested page. count must be 1..256.
        /// A page is clipped at the retained end; TotalFrames is the absolute next sample sequence.
        /// A request above 256 frames returns QueryTooLarge; malformed ranges return InvalidRange.
        /// CPU data contains completed roots from the selected extraction batches; threadId == 0 includes all threads.
        /// Event identity is (SessionId, SnapshotId, array index). ParentIndex references the same event array, or -1.
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

        /// Main-thread only. Copies an existing immutable snapshot independently of rolling history.
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
