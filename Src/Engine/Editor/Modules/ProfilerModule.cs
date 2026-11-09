using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Text;
using CaptureInfo = SE.ProfilerCaptureInfo;
using CaptureFrame = SE.ProfilerCaptureFrame;
using CaptureState = SE.ProfilerCaptureState;
using CaptureResult = SE.ProfilerCaptureResult;
using StopReason = SE.ProfilerStopReason;
using FrameViewInfo = SE.ProfilerFrameViewInfo;
using CpuThread = SE.ProfilerCpuThread;
using CpuEvent = SE.ProfilerCpuEvent;
using CpuStatistic = SE.ProfilerCpuStatistic;
using GpuFrame = SE.ProfilerGpuFrame;
using GpuEvent = SE.ProfilerGpuEvent;
using GpuViewInfo = SE.ProfilerGpuViewInfo;

namespace SE.Editor
{
    /// <summary>
    /// Owns the editor's local capture workflow. All methods run on the editor main thread.
    /// Transport records are explicit copies, independent of rolling native history.
    /// </summary>
    public sealed class ProfilerModule : EditorModule
    {
        public const int MaxViewFrames = 256;
        public int HistoryCapacity => m_NativeAvailable ? Profiling.GetHistoryCapacity() : 0;
        public ulong OldestFrame => Info.OldestFrame;

        public bool IsMemoryVisible { get; private set; }
        public ProfilerMemoryOverviewInfo MemoryOverview { get; private set; }
        public ProfilerMemorySample[] MemorySamples { get; private set; } = Array.Empty<ProfilerMemorySample>();
        public string MemoryDetails { get; private set; } = "Select a memory sample.";
        private long m_MemoryFirst = -1;

        public void ShowMemoryPage()
        {
            IsMemoryVisible = true;
            IsGpuVisible = false;
            m_ViewDirty = true;
            Refresh();
        }

        public void BrowseMemory(bool newer, bool end)
        {
            Pause();
            long oldest = (long)MemoryOverview.OldestSample;
            long last = Math.Max(oldest, oldest + MemoryOverview.TotalSamples - MaxViewFrames);
            m_MemoryFirst = end ? newer ? last : oldest :
                Math.Clamp((long)MemoryOverview.First + (newer ? MaxViewFrames : -MaxViewFrames), oldest, last);
            m_ViewDirty = true;
            Refresh();
        }

        public void SelectMemorySample(ulong snapshot, int index)
        {
            if (snapshot != MemoryOverview.SnapshotId || index < 0 || index >= MemorySamples.Length)
                return;
            Pause();
            ProfilerMemorySample sample = MemorySamples[index];
            string Value(ulong bytes, int mask) => (sample.ValidMask & mask) != 0 ? $"{bytes:N0} bytes ({bytes / 1048576.0:0.##} MiB)" : "Unavailable";
            MemoryDetails = $"Sample {MemoryOverview.First + (ulong)index} submitted at {sample.TimeSeconds:0.##} s, frame {sample.FrameId}; OS counters read at {sample.NativeReadSeconds:0.##} s\n" +
                $"Process working set: {Value(sample.ProcessPhysicalBytes, 1)}\nProcess private committed: {Value(sample.ProcessPrivateBytes, 1)}\n" +
                $"System physical used: {Value(sample.SystemUsedBytes, 2)}\nSystem physical total: {Value(sample.SystemTotalBytes, 2)}\n" +
                $"Driver-reported GPU heap total: {Value(sample.GpuTotalBytes, 4)}\nEngine GPU resource estimate: {Value(sample.GpuUsedBytes, 4)}\n";
            if ((sample.ValidMask & 8) != 0)
                MemoryDetails += $"Managed heap estimate: {sample.ManagedHeapBytes:N0} bytes\nManaged allocated, process cumulative: {sample.ManagedAllocatedBytes:N0} bytes\n" +
                    $"GC collections (process cumulative): {sample.Gc0}/{sample.Gc1}/{sample.Gc2}\n" +
                    $"Managed receipt elapsed: {sample.ManagedReceiptSeconds:0.##} s; counter read took {sample.ManagedReadSeconds:0.##} s.\n";
            else
                MemoryDetails += "Managed sample unavailable.\n";
            MemoryDetails += "Sources are sampled separately. Receipt time is not atomic native/managed measurement. Values overlap and must not be summed.";
            Changed?.Invoke();
        }

        private void ResetMemoryView()
        {
            MemoryOverview = default;
            MemorySamples = Array.Empty<ProfilerMemorySample>();
            MemoryDetails = "Select a memory sample.";
        }

        public enum StatisticSortColumn
        {
            Name, Calls, Incomplete, Total, Self, Average, Maximum
        }

        private CpuStatistic[] m_CpuStatistics = Array.Empty<CpuStatistic>();
        private readonly Dictionary<int, int> m_StatisticBySource = new Dictionary<int, int>();
        private readonly List<int> m_VisibleStatistics = new List<int>();
        public ReadOnlySpan<CpuStatistic> CpuStatistics => m_CpuStatistics;
        public int VisibleStatisticCount => m_VisibleStatistics.Count;
        public int SelectedCpuStatistic { get; private set; } = -1;
        public string StatisticFilter { get; private set; } = string.Empty;
        public StatisticSortColumn StatisticSort { get; private set; } = StatisticSortColumn.Total;
        public bool StatisticSortDescending { get; private set; } = true;
        public int StatisticPresentationVersion { get; private set; }
        public bool IsCpuStatisticsVisible { get; private set; }
        public int CpuFocusVersion { get; private set; }
        private GpuFrame[] m_GpuFrames = Array.Empty<GpuFrame>();
        private GpuEvent[] m_GpuEvents = Array.Empty<GpuEvent>();
        private Dictionary<int, string> m_GpuText = new Dictionary<int, string>();
        private readonly HashSet<int> m_CollapsedGpu = new HashSet<int>();
        private ulong m_GpuFrameId = ulong.MaxValue;
        private readonly List<int> m_VisibleGpuEvents = new List<int>();
        public int VisibleGpuEventCount => m_VisibleGpuEvents.Count;
        public int GpuLayoutVersion { get; private set; }
        public int GetVisibleGpuEvent(int row) => m_VisibleGpuEvents[row];

        private void RebuildGpuRows()
        {
            m_VisibleGpuEvents.Clear();
            int hiddenDepth = -1;
            for (int i = 0; i < m_GpuEvents.Length; i++)
            {
                int depth = m_GpuEvents[i].Depth;
                if (hiddenDepth >= 0 && depth > hiddenDepth)
                    continue;
                hiddenDepth = -1;
                m_VisibleGpuEvents.Add(i);
                if (m_CollapsedGpu.Contains(i))
                    hiddenDepth = depth;
            }
            GpuLayoutVersion++;
        }
        public bool IsGpuVisible { get; private set; }
        public GpuViewInfo GpuView { get; private set; }
        public ReadOnlySpan<GpuFrame> GpuFrames => m_GpuFrames;
        public ReadOnlySpan<GpuEvent> GpuEvents => m_GpuEvents;
        public int SelectedGpuEvent { get; private set; } = -1;
        public string GpuEventDetails { get; private set; } = "Select a GPU event to inspect it.";

        public void ShowGpuPage()
        {
            IsMemoryVisible = false;
            if (!IsGpuVisible)
            {
                IsGpuVisible = true;
                m_GpuFrameId = ulong.MaxValue;
                m_ViewDirty = true;
                Refresh();
            }
        }

        public CaptureResult SelectGpuFrame(ulong frameId)
        {
            if (!IsGpuVisible || (Info.State != CaptureState.Capturing && Info.State != CaptureState.Stopped))
                return CaptureResult.NotReady;
            Pause();
            m_GpuFrameId = frameId;
            m_ViewDirty = true;
            Refresh();
            return ViewResult;
        }

        public void BrowseGpuFrame(bool newer)
        {
            int next = GpuView.SelectedIndex + (newer ? 1 : -1);
            if (next >= 0 && next < m_GpuFrames.Length)
                SelectGpuFrame(m_GpuFrames[next].FrameId);
        }

        public string GetGpuText(int offset) => m_GpuText.TryGetValue(offset, out string text) ? text : string.Empty;
        public bool IsGpuEventCollapsed(int index) => m_CollapsedGpu.Contains(index);

        public void ToggleGpuEvent(ulong snapshotId, int index)
        {
            if (snapshotId != GpuView.SnapshotId || index < 0 || index >= m_GpuEvents.Length)
                return;
            if (!m_CollapsedGpu.Remove(index))
                m_CollapsedGpu.Add(index);
            RebuildGpuRows();
            Changed?.Invoke();
        }

        public CaptureResult SelectGpuEvent(ulong snapshotId, int index)
        {
            if (snapshotId == 0 || snapshotId != GpuView.SnapshotId)
                return CaptureResult.InvalidSnapshot;
            if (index < 0 || index >= m_GpuEvents.Length)
                return CaptureResult.InvalidRange;
            Pause();
            SelectedGpuEvent = index;
            GpuEventDetails = DescribeGpuEvent(index);
            Changed?.Invoke();
            return CaptureResult.Ok;
        }

        public string DescribeGpuEvent(int index)
        {
            if (index < 0 || index >= m_GpuEvents.Length)
                return string.Empty;
            GpuEvent item = m_GpuEvents[index];
            string text = $"{GetGpuText(item.NameOffset)}\nEngine frame: {GpuView.SelectedFrameId}\nSnapshot: {GpuView.SnapshotId}    Event: {index}\nDepth: {item.Depth}    Parent: {item.ParentIndex}\n";
            text += item.TimingValid != 0 ? $"GPU duration: {item.DurationMs:0.##} ms\n" : "GPU duration unavailable.\n";
            text += item.ParentPercent >= 0 ? $"Parent duration ratio: {item.ParentPercent:0.##}%\n" : "Parent duration ratio unavailable.\n";
            if (item.Complete == 0)
                text += "Event did not finish normally; a forced query end is not its measured scope end.\n";
            text += item.StatsValid != 0 ? $"Draw calls: {item.DrawCalls}\nDispatch calls: {item.DispatchCalls}\nVertices: {item.Vertices}\nTriangles: {item.Triangles}\nPipeline changes: {item.PipelineStateChanges}\n" :
                "Drawing statistics unavailable for this incomplete event.\n";
            return text + "Durations have no absolute execution positions here. GPU frame IDs retain their engine identity independently of CPU sample indices.";
        }

        public int GetVisibleStatisticIndex(int row) => m_VisibleStatistics[row];
        public int GetVisibleStatisticPosition(int index) => m_VisibleStatistics.IndexOf(index);

        public void ShowCpuPage(bool statistics)
        {
            bool changedPage = IsGpuVisible || IsMemoryVisible;
            IsMemoryVisible = false;
            IsGpuVisible = false;
            IsCpuStatisticsVisible = statistics;
            if (changedPage)
            {
                m_FirstFrame = ulong.MaxValue;
                m_SelectionCount = 0;
                m_ViewDirty = true;
                Refresh();
            }
            Changed?.Invoke();
        }

        /// <summary>Presentation filtering uses cached source names/functions; it does not change query coverage.</summary>
        public void FilterCpuStatistics(string filter)
        {
            filter ??= string.Empty;
            if (StatisticFilter == filter)
                return;
            Pause();
            StatisticFilter = filter;
            RebuildStatisticPresentation();
            Changed?.Invoke();
        }

        public void SortCpuStatistics(StatisticSortColumn column)
        {
            Pause();
            StatisticSortDescending = column == StatisticSort ? !StatisticSortDescending : column != StatisticSortColumn.Name;
            StatisticSort = column;
            RebuildStatisticPresentation();
            Changed?.Invoke();
        }

        private void RebuildStatisticPresentation()
        {
            m_VisibleStatistics.Clear();
            for (int i = 0; i < m_CpuStatistics.Length; i++)
            {
                CpuStatistic row = m_CpuStatistics[i];
                if (StatisticFilter.Length == 0 || GetCpuText(row.NameOffset).IndexOf(StatisticFilter, StringComparison.OrdinalIgnoreCase) >= 0 ||
                    GetCpuText(row.FunctionOffset).IndexOf(StatisticFilter, StringComparison.OrdinalIgnoreCase) >= 0)
                    m_VisibleStatistics.Add(i);
            }
            m_VisibleStatistics.Sort((int left, int right) =>
            {
                CpuStatistic a = m_CpuStatistics[left];
                CpuStatistic b = m_CpuStatistics[right];
                // Unavailable timing values stay last in both directions, rather than sorting a fake zero.
                if (StatisticSort == StatisticSortColumn.Self && (a.SelfIncompleteCalls != 0) != (b.SelfIncompleteCalls != 0))
                    return a.SelfIncompleteCalls != 0 ? 1 : -1;
                if ((StatisticSort == StatisticSortColumn.Total || StatisticSort == StatisticSortColumn.Self ||
                    StatisticSort == StatisticSortColumn.Average || StatisticSort == StatisticSortColumn.Maximum) && (a.Calls == 0) != (b.Calls == 0))
                    return a.Calls == 0 ? 1 : -1;
                int comparison = StatisticSort switch
                {
                    StatisticSortColumn.Name => string.Compare(GetCpuText(a.NameOffset), GetCpuText(b.NameOffset), StringComparison.OrdinalIgnoreCase),
                    StatisticSortColumn.Calls => a.Calls.CompareTo(b.Calls),
                    StatisticSortColumn.Incomplete => a.IncompleteCalls.CompareTo(b.IncompleteCalls),
                    StatisticSortColumn.Total => a.TotalNs.CompareTo(b.TotalNs),
                    StatisticSortColumn.Self => a.SelfNs.CompareTo(b.SelfNs),
                    StatisticSortColumn.Average => a.AverageNs.CompareTo(b.AverageNs),
                    _ => a.MaxNs.CompareTo(b.MaxNs),
                };
                return comparison == 0 ? a.SourceLocationId.CompareTo(b.SourceLocationId) : StatisticSortDescending ? -comparison : comparison;
            });
            StatisticPresentationVersion++;
        }

        public string DescribeCpuStatistic(int index)
        {
            if (index < 0 || index >= m_CpuStatistics.Length)
                return string.Empty;
            CpuStatistic row = m_CpuStatistics[index];
            string text = $"{GetCpuText(row.NameOffset)}\nSource ID: {row.SourceLocationId}\n{GetCpuText(row.FunctionOffset)}\n{GetCpuText(row.FileOffset)}:{row.Line}\n";
            text += $"Snapshot: {View.SnapshotId}\nComplete calls: {row.Calls}    End unknown: {row.IncompleteCalls}\n";
            if (row.Calls > 0)
            {
                text += $"Clipped total: {row.TotalNs / 1_000_000.0:0.##} ms\nAverage: {row.AverageNs / 1_000_000.0:0.##} ms\nMaximum: {row.MaxNs / 1_000_000.0:0.##} ms\n";
                text += row.SelfIncompleteCalls == 0 ? $"Self: {row.SelfNs / 1_000_000.0:0.##} ms\n" :
                    $"Self total incomplete: {row.SelfIncompleteCalls} calls have children with unknown ends. Known subtotal: {row.SelfNs / 1_000_000.0:0.##} ms\n";
            }
            else
            {
                text += "No complete instance in this snapshot; timing aggregates are unavailable.\n";
            }
            return text + "Totals use the frame selection and explicit thread filter. They may exceed wall time and are not CPU utilization.";
        }

        public CaptureResult SelectCpuStatistic(ulong snapshotId, int index)
        {
            if (snapshotId == 0 || snapshotId != View.SnapshotId)
                return CaptureResult.InvalidSnapshot;
            if (index < 0 || index >= m_CpuStatistics.Length)
                return CaptureResult.InvalidRange;
            CpuStatistic row = m_CpuStatistics[index];
            if (row.RepresentativeEvent >= 0)
                return SelectCpuEvent(snapshotId, row.RepresentativeEvent);
            Pause();
            SelectedCpuStatistic = index;
            SelectedCpuEvent = -1;
            CpuEventDetails = DescribeCpuStatistic(index);
            Changed?.Invoke();
            return CaptureResult.Ok;
        }

        public CaptureResult FocusCpuStatistic(ulong snapshotId, int index)
        {
            CaptureResult result = SelectCpuStatistic(snapshotId, index);
            if (result != CaptureResult.Ok)
                return result;
            int instance = m_CpuStatistics[index].RepresentativeEvent;
            if (instance < 0)
                return CaptureResult.NotReady;
            FocusCpuEvent(snapshotId, instance);
            return CaptureResult.Ok;
        }

        private long m_LastRefresh;
        private bool m_NativeAvailable = true;
        private bool m_ViewDirty = true;
        private bool m_WasViewVisible;
        private ulong m_FirstFrame = ulong.MaxValue;
        private ulong m_SelectionFirst;
        private int m_SelectionCount;
        private CaptureFrame[] m_Frames = Array.Empty<CaptureFrame>();
        private CpuThread[] m_CpuThreads = Array.Empty<CpuThread>();
        private CpuEvent[] m_CpuEvents = Array.Empty<CpuEvent>();
        private Dictionary<int, string> m_CpuText = new Dictionary<int, string>();
        private readonly HashSet<ulong> m_CollapsedThreads = new HashSet<ulong>();
        private ulong m_ThreadId;
        private CpuThread[] m_ThreadOptions = Array.Empty<CpuThread>();
        private Dictionary<int, string> m_ThreadOptionText = new Dictionary<int, string>();

        public ReadOnlySpan<CpuThread> ThreadOptions => m_ThreadOptions;
        public int TotalThreadOptions { get; private set; }

        public ReadOnlySpan<CpuThread> CpuThreads => m_CpuThreads;
        public ReadOnlySpan<CpuEvent> CpuEvents => m_CpuEvents;
        public int SelectedCpuEvent { get; private set; } = -1;
        public string CpuEventDetails { get; private set; } = "Select a CPU event to inspect it.";
        public long CpuBeginNs { get; private set; }
        public long CpuEndNs { get; private set; }
        public int CpuLayoutVersion { get; private set; }

        public ProfilerModule(Editor editor) : base(editor)
        {
        }

        internal override int Order => -85;

        public CaptureInfo Info { get; private set; }
        public FrameViewInfo View { get; private set; }
        public ReadOnlySpan<CaptureFrame> Frames => new ReadOnlySpan<CaptureFrame>(m_Frames, 0, View.FrameCount);
        public CaptureFrame? Frame => View.SelectionCount == 1 ? m_Frames[(int)(View.SelectionFirst - View.FirstFrame)] : null;
        public bool IsLive { get; private set; } = true;
        public bool IsViewPending => m_ViewDirty &&
            (Info.State == CaptureState.Capturing || Info.State == CaptureState.Stopped);
        /// <summary>The last capture control result; automatic view polling cannot overwrite it.</summary>
        public CaptureResult LastResult { get; private set; }
        public CaptureResult ViewResult { get; private set; }
        public string LoadError { get; private set; } = string.Empty;
        public event Action Changed;

        public string MenuLabel => Info.State switch
        {
            CaptureState.Capturing => "Profiler (REC)",
            CaptureState.Stopped => "Profiler (stopped)",
            _ => "Profiler",
        };

        public override void OnInit()
        {
            Refresh(false);
            m_TestRunner = ProfilerModuleTestRunner.TryCreate(this);
        }

        private ProfilerModuleTestRunner m_TestRunner;

        public override void OnUpdate()
        {
            m_TestRunner?.Tick();
            long now = Stopwatch.GetTimestamp();
            if (now - m_LastRefresh >= Stopwatch.Frequency / 10)
            {
                m_LastRefresh = now;
                bool viewVisible = Editor.Windows.Profiler != null && Editor.Windows.Profiler.VisibleInHierarchy;
                // A dock tab can become visible without DockWindow.OnShow (for example, tab selection).
                if (viewVisible && !m_WasViewVisible && IsLive)
                    m_ViewDirty = true;
                m_WasViewVisible = viewVisible;
                // Idle/stopped snapshots need no polling; hidden windows need only the small state sample.
                if (IsViewPending || Info.GpuPending > 0 || Info.State == CaptureState.Capturing)
                {
                    Refresh(viewVisible);
                }
                if (m_NativeAvailable && Info.State == CaptureState.Capturing)
                {
                    long begin = Stopwatch.GetTimestamp();
                    long heap = GC.GetTotalMemory(false);
                    long allocated = GC.GetTotalAllocatedBytes(false);
                    int gc0 = GC.CollectionCount(0);
                    int gc1 = GC.CollectionCount(1);
                    int gc2 = GC.CollectionCount(2);
                    Profiling.SubmitManagedMemory(Info.SessionId, heap, allocated, gc0, gc1, gc2,
                        (Stopwatch.GetTimestamp() - begin) / (double)Stopwatch.Frequency);
                }
            }
        }

        public CaptureResult Start()
        {
            LastResult = m_NativeAvailable ? Profiling.StartCapture() : CaptureResult.Unavailable;
            if (LastResult == CaptureResult.Ok)
            {
                IsLive = true;
                m_FirstFrame = ulong.MaxValue;
                m_SelectionCount = 0;
                m_ViewDirty = true;
            }
            Refresh();
            return LastResult;
        }

        public CaptureResult Stop()
        {
            LastResult = m_NativeAvailable ? Profiling.StopCapture() : CaptureResult.Unavailable;
            Refresh();
            return LastResult;
        }

        public CaptureResult Clear()
        {
            LastResult = m_NativeAvailable ? Profiling.ClearCapture() : CaptureResult.Unavailable;
            if (LastResult == CaptureResult.Ok)
            {
                ResetView();
                m_ViewDirty = true;
            }
            Refresh();
            return LastResult;
        }

        /// <summary>Selects a complete frame, loading its historical page if needed; viewing remains paused.</summary>
        public CaptureResult ReadFrame(ulong index)
        {
            return SelectFrames(index, index);
        }

        /// <summary>Freezes the current native snapshot while state polling and background capture continue.</summary>
        public void Pause()
        {
            IsLive = false;
            if (IsMemoryVisible && MemoryOverview.SnapshotId != 0)
            {
                m_MemoryFirst = (long)MemoryOverview.First;
                m_ViewDirty = false;
            }
            if (IsGpuVisible && GpuView.SnapshotId != 0)
            {
                m_GpuFrameId = GpuView.SelectedFrameId;
                m_ViewDirty = false;
            }
            if (View.SnapshotId != 0)
            {
                m_FirstFrame = View.FirstFrame;
                m_SelectionFirst = View.SelectionFirst;
                m_SelectionCount = (int)View.SelectionCount;
                m_ViewDirty = false;
            }
            Changed?.Invoke();
        }

        /// <summary>Follows live frames, or requests only the last complete page of a stopped capture.</summary>
        public void ShowLatestFrame()
        {
            m_MemoryFirst = -1;
            m_GpuFrameId = ulong.MaxValue;
            IsLive = Info.State == CaptureState.Capturing;
            m_FirstFrame = ulong.MaxValue;
            m_SelectionCount = 0;
            m_ViewDirty = true;
            Refresh();
        }

        /// <summary>Replaces the current query explicitly, including when paused.</summary>
        public void RefreshView()
        {
            m_ViewDirty = true;
            Refresh();
        }

        /// <summary>Selects an inclusive range of up to 256 complete frames. Times use [first.Begin, last.End).</summary>
        public CaptureResult SelectFrames(ulong first, ulong last)
        {
            if (!m_NativeAvailable || Info.State == CaptureState.Unavailable)
            {
                ViewResult = CaptureResult.Unavailable;
                Changed?.Invoke();
                return ViewResult;
            }
            if (Info.State != CaptureState.Capturing && Info.State != CaptureState.Stopped)
            {
                ViewResult = CaptureResult.NotReady;
                Changed?.Invoke();
                return ViewResult;
            }
            // The view copy can observe newer complete frames than the preceding non-blocking state poll.
            ulong completeFrames = Math.Max(Info.CompleteFrames, View.TotalFrames);
            if (first < OldestFrame || last < first || last >= completeFrames)
            {
                ViewResult = CaptureResult.InvalidRange;
                Changed?.Invoke();
                return ViewResult;
            }
            if (last - first >= MaxViewFrames)
            {
                ViewResult = CaptureResult.QueryTooLarge;
                Changed?.Invoke();
                return ViewResult;
            }
            IsLive = false;
            m_FirstFrame = View.FrameCount > 0 && first >= View.FirstFrame && last - View.FirstFrame < (ulong)View.FrameCount ? View.FirstFrame : first;
            m_SelectionFirst = first;
            m_SelectionCount = (int)(last - first + 1);
            m_ViewDirty = true;
            ViewResult = CaptureResult.NotReady;
            Refresh();
            return ViewResult;
        }

        public void BrowseFrames(bool newer)
        {
            if (View.FrameCount == 0)
            {
                return;
            }
            ulong completeFrames = Math.Max(Info.CompleteFrames, View.TotalFrames);
            ulong latest = Math.Max(OldestFrame, completeFrames > MaxViewFrames ? completeFrames - MaxViewFrames : 0);
            m_FirstFrame = newer ? Math.Min(View.FirstFrame + MaxViewFrames, latest) :
                Math.Max(OldestFrame, View.FirstFrame > MaxViewFrames ? View.FirstFrame - MaxViewFrames : 0);
            IsLive = false;
            m_SelectionCount = 0;
            m_ViewDirty = true;
            Refresh();
        }

        /// <summary>Changes the semantic thread filter; folding and display navigation never query native data.</summary>
        public CaptureResult SelectCpuThread(ulong threadId)
        {
            bool found = threadId == 0;
            foreach (CpuThread thread in m_CpuThreads)
            {
                found |= thread.Id == threadId;
            }
            foreach (CpuThread thread in m_ThreadOptions)
            {
                found |= thread.Id == threadId;
            }
            if (!found)
                return CaptureResult.InvalidRange;
            Pause();
            m_ThreadId = threadId;
            m_ViewDirty = true;
            Refresh();
            return ViewResult;
        }

        public string GetCpuText(int offset) => m_CpuText.TryGetValue(offset, out string text) ? text : string.Empty;
        public string GetThreadOptionText(int offset) => m_ThreadOptionText.TryGetValue(offset, out string text) ? text : string.Empty;
        public bool IsCpuThreadCollapsed(ulong id) => m_CollapsedThreads.Contains(id);

        /// <summary>One explicit directory-page query; it neither replaces nor claims the current analysis snapshot.</summary>
        public CaptureResult LoadCpuThreads(int first)
        {
            if (!m_NativeAvailable)
                return CaptureResult.Unavailable;
            CaptureResult result = Profiling.ReadCaptureThreads(Info.SessionId, first, out int total, out CpuThread[] threads, out byte[] text);
            if (result == CaptureResult.Ok)
            {
                m_ThreadOptionText = DecodeCpuText(text);
                m_ThreadOptions = threads ?? Array.Empty<CpuThread>();
                TotalThreadOptions = total;
            }
            return result;
        }

        public void ToggleCpuThread(ulong id)
        {
            if (!m_CollapsedThreads.Remove(id))
                m_CollapsedThreads.Add(id);
            CpuLayoutVersion++;
            Changed?.Invoke();
        }

        /// <summary>Snapshot-local selection. A stale identity cannot select a new event at the same array index.</summary>
        public CaptureResult SelectCpuEvent(ulong snapshotId, int index)
        {
            if (snapshotId == 0 || snapshotId != View.SnapshotId)
                return CaptureResult.InvalidSnapshot;
            if (index < 0 || index >= m_CpuEvents.Length)
                return CaptureResult.InvalidRange;
            Pause();
            SelectedCpuEvent = index;
            SelectedCpuStatistic = m_StatisticBySource.TryGetValue(m_CpuEvents[index].SourceLocationId, out int statistic) ? statistic : -1;
            CpuEventDetails = DescribeCpuEvent(index);
            Changed?.Invoke();
            return CaptureResult.Ok;
        }

        private static string FormatCpuDuration(CpuEvent item) => item.EndValid != 0 ? $"{(item.EndNs - item.BeginNs) / 1_000_000.0:0.##} ms" : "unavailable";

        internal string GetCpuEventLabel(int index)
        {
            if (index < 0 || index >= m_CpuEvents.Length)
            {
                return string.Empty;
            }
            CpuEvent item = m_CpuEvents[index];
            string duration = item.EndValid != 0 ? FormatCpuDuration(item) : "Duration unavailable";
            return $"{GetCpuText(item.NameOffset)} - {duration}";
        }

        public string DescribeCpuEvent(int index)
        {
            if (index < 0 || index >= m_CpuEvents.Length)
                return string.Empty;
            CpuEvent item = m_CpuEvents[index];
            string threadName = string.Empty;
            ulong threadId = 0;
            foreach (CpuThread thread in m_CpuThreads)
            {
                if (index >= thread.FirstEvent && index - thread.FirstEvent < thread.EventCount)
                {
                    threadName = GetCpuText(thread.NameOffset);
                    threadId = thread.Id;
                    break;
                }
            }
            string text = $"{GetCpuEventLabel(index)}\nThread: {threadName} [#{threadId}; OS ID unavailable]\nSnapshot: {View.SnapshotId}    Event: {index}\n";
            text += $"Depth: {item.Depth}    Parent: {item.ParentIndex}\nDuration: {FormatCpuDuration(item)}\n";
            if (item.BeginNs < View.SelectionBeginNs || (item.EndValid != 0 && item.EndNs > View.SelectionEndNs))
                text += "Event crosses the selected range; only the bar is clipped.\n";
            text += $"Function: {GetCpuText(item.FunctionOffset)}\nSource ID: {item.SourceLocationId}\nFile: {GetCpuText(item.FileOffset)}\nLine: {item.Line}\n";
            text += "Missing names or source metadata may be shown as ???; they are not resolved by this view.";
            return text;
        }

        /// <summary>Changes only the cached display range, clamped to the frozen selection.</summary>
        public void SetCpuRange(long beginNs, long endNs)
        {
            if (View.SelectionEndNs <= View.SelectionBeginNs)
                return;
            long duration = Math.Clamp(endNs - beginNs, 1, View.SelectionEndNs - View.SelectionBeginNs);
            Pause();
            CpuBeginNs = Math.Clamp(beginNs, View.SelectionBeginNs, View.SelectionEndNs - duration);
            CpuEndNs = CpuBeginNs + duration;
            Changed?.Invoke();
        }

        public void FocusCpuEvent(ulong snapshotId, int index)
        {
            if (SelectCpuEvent(snapshotId, index) != CaptureResult.Ok)
                return;
            CpuEvent item = m_CpuEvents[index];
            IsCpuStatisticsVisible = false;
            foreach (CpuThread thread in m_CpuThreads)
            {
                if (index >= thread.FirstEvent && index - thread.FirstEvent < thread.EventCount && m_CollapsedThreads.Remove(thread.Id))
                    CpuLayoutVersion++;
            }
            if (item.EndValid != 0)
            {
                long begin = Math.Max(item.BeginNs, View.SelectionBeginNs);
                // A measured zero-duration call is still locatable; one ns is a display range, not its duration.
                long end = Math.Min(Math.Max(item.EndNs, begin + 1), View.SelectionEndNs);
                SetCpuRange(begin, end);
            }
            CpuFocusVersion++;
            Changed?.Invoke();
        }

        private void ResetView()
        {
            ResetMemoryView();
            GpuView = default;
            m_GpuFrames = Array.Empty<GpuFrame>();
            m_GpuEvents = Array.Empty<GpuEvent>();
            m_GpuText.Clear();
            m_VisibleGpuEvents.Clear();
            GpuLayoutVersion++;
            SelectedGpuEvent = -1;
            GpuEventDetails = "Select a GPU event to inspect it.";
            View = default;
            m_CpuStatistics = Array.Empty<CpuStatistic>();
            m_StatisticBySource.Clear();
            m_VisibleStatistics.Clear();
            SelectedCpuStatistic = -1;
            StatisticPresentationVersion++;
            m_Frames = Array.Empty<CaptureFrame>();
            m_CpuThreads = Array.Empty<CpuThread>();
            m_CpuEvents = Array.Empty<CpuEvent>();
            m_CpuText.Clear();
            m_ThreadOptions = Array.Empty<CpuThread>();
            m_ThreadOptionText.Clear();
            TotalThreadOptions = 0;
            m_CollapsedThreads.Clear();
            m_ThreadId = 0;
            SelectedCpuEvent = -1;
            CpuEventDetails = "Select a CPU event to inspect it.";
            CpuBeginNs = 0;
            CpuEndNs = 0;
        }

        internal void OnWindowShown()
        {
            if (IsLive || (IsMemoryVisible ? MemoryOverview.SnapshotId == 0 :
                IsGpuVisible ? GpuView.SnapshotId == 0 : View.SnapshotId == 0))
                RefreshView();
        }

        private static Dictionary<int, string> DecodeCpuText(byte[] text)
        {
            Dictionary<int, string> strings = new Dictionary<int, string>();
            text ??= Array.Empty<byte>();
            for (int start = 0; start < text.Length;)
            {
                int end = Array.IndexOf(text, (byte)0, start);
                if (end < 0)
                    throw new InvalidOperationException("Profiler text result is not terminated.");
                strings.Add(start, Encoding.UTF8.GetString(text, start, end - start));
                start = end + 1;
            }
            return strings;
        }

        /// <summary>
        /// Main-thread copy of a frozen native view into a separate caller-owned array. Releasing this result
        /// does not release the snapshot; reads of a replaced snapshot return InvalidSnapshot.
        /// </summary>
        public CaptureResult ReadView(ulong sessionId, ulong snapshotId, out FrameViewInfo view, out CaptureFrame[] frames)
        {
            view = default;
            frames = Array.Empty<CaptureFrame>();
            if (!m_NativeAvailable)
            {
                return CaptureResult.Unavailable;
            }
            CaptureResult result = Profiling.ReadFrameView(sessionId, snapshotId, out view, out frames);
            frames ??= Array.Empty<CaptureFrame>();
            return result;
        }

        public override void OnDispose()
        {
            if (m_NativeAvailable && (Info.State == CaptureState.Capturing))
            {
                Profiling.StopCapture();
            }
            Changed = null;
        }

        private void Refresh(bool allowLiveView = true)
        {
            if (!m_NativeAvailable)
            {
                return;
            }
            try
            {
                bool query = (m_ViewDirty || (IsLive && allowLiveView)) &&
                    (Info.State == CaptureState.Capturing || Info.State == CaptureState.Stopped);
                CaptureInfo info;
                FrameViewInfo view = default;
                CaptureFrame[] frames = Array.Empty<CaptureFrame>();
                CpuThread[] threads = Array.Empty<CpuThread>();
                CpuEvent[] events = Array.Empty<CpuEvent>();
                byte[] text = Array.Empty<byte>();
                CpuStatistic[] statistics = Array.Empty<CpuStatistic>();
                GpuViewInfo gpuView = default;
                GpuFrame[] gpuFrames = Array.Empty<GpuFrame>();
                GpuEvent[] gpuEvents = Array.Empty<GpuEvent>();
                ProfilerMemoryOverviewInfo memoryOverview = default;
                ProfilerMemorySample[] memorySamples = Array.Empty<ProfilerMemorySample>();
                CaptureResult result;
                if (query && IsMemoryVisible)
                {
                    result = Profiling.RefreshMemoryOverview(Info.SessionId, IsLive ? -1 : m_MemoryFirst,
                        out info, out memoryOverview, out memorySamples);
                }
                else if (query && IsGpuVisible)
                {
                    result = Profiling.RefreshGpuView(Info.SessionId, IsLive ? ulong.MaxValue : m_GpuFrameId,
                        out info, out gpuView, out gpuFrames, out gpuEvents, out text);
                }
                else if (query)
                {
                    // A single native call updates state and freezes all related frame/CPU values.
                    result = Profiling.RefreshFrameView(Info.SessionId, IsLive ? ulong.MaxValue : m_FirstFrame, MaxViewFrames,
                        IsLive ? 0 : m_SelectionFirst, IsLive ? 0 : m_SelectionCount, m_ThreadId,
                        out info, out view, out frames, out threads, out events, out text, out statistics);
                }
                else
                {
                    result = Profiling.GetCaptureInfo(out info);
                }
                if (Info.SessionId != info.SessionId)
                {
                    ResetView();
                    m_FirstFrame = ulong.MaxValue;
                    m_SelectionCount = 0;
                    m_GpuFrameId = ulong.MaxValue;
                    m_MemoryFirst = -1;
                    m_CollapsedGpu.Clear();
                    m_ViewDirty = true;
                }
                Info = info;
                // Preserve a final refresh when state polling observes Stop before the next view refresh.
                if (!query && allowLiveView && IsMemoryVisible && IsLive && info.State == CaptureState.Stopped)
                    m_ViewDirty = true;
                if (query)
                {
                    if (result == CaptureResult.Ok && IsMemoryVisible)
                    {
                        ResetView();
                        MemoryOverview = memoryOverview;
                        MemorySamples = memorySamples ?? Array.Empty<ProfilerMemorySample>();
                        m_ViewDirty = false;
                        ViewResult = result;
                        if (info.State == CaptureState.Stopped)
                            IsLive = false;
                        if (!IsLive)
                        {
                            m_MemoryFirst = (long)memoryOverview.First;
                        }
                    }
                    else if (result == CaptureResult.Ok && IsGpuVisible)
                    {
                        bool sameFrame = GpuView.SelectedFrameId == gpuView.SelectedFrameId;
                        Dictionary<int, string> strings = DecodeCpuText(text);
                        ResetView();
                        GpuView = gpuView;
                        m_GpuFrames = gpuFrames ?? Array.Empty<GpuFrame>();
                        m_GpuEvents = gpuEvents ?? Array.Empty<GpuEvent>();
                        m_GpuText = strings;
                        if (!sameFrame)
                            m_CollapsedGpu.Clear();
                        RebuildGpuRows();
                        m_ViewDirty = false;
                        ViewResult = CaptureResult.Ok;
                        if (info.State == CaptureState.Stopped && info.GpuPending == 0)
                            IsLive = false;
                        if (!IsLive)
                            m_GpuFrameId = gpuView.SelectedFrameId;
                    }
                    else if (result == CaptureResult.Ok)
                    {
                        ResetMemoryView();
                        GpuView = default;
                        m_GpuFrames = Array.Empty<GpuFrame>();
                        m_GpuEvents = Array.Empty<GpuEvent>();
                        m_GpuText.Clear();
                        m_VisibleGpuEvents.Clear();
                        GpuLayoutVersion++;
                        SelectedGpuEvent = -1;
                        Dictionary<int, string> strings = DecodeCpuText(text);
                        bool sameRange = View.SelectionBeginNs == view.SelectionBeginNs && View.SelectionEndNs == view.SelectionEndNs;
                        m_Frames = frames ?? Array.Empty<CaptureFrame>();
                        m_CpuThreads = threads ?? Array.Empty<CpuThread>();
                        m_CpuEvents = events ?? Array.Empty<CpuEvent>();
                        m_CpuText = strings;
                        m_CpuStatistics = statistics ?? Array.Empty<CpuStatistic>();
                        m_StatisticBySource.Clear();
                        for (int i = 0; i < m_CpuStatistics.Length; i++)
                            m_StatisticBySource.Add(m_CpuStatistics[i].SourceLocationId, i);
                        SelectedCpuStatistic = -1;
                        SelectedCpuEvent = -1;
                        CpuEventDetails = "Select a CPU event to inspect it.";
                        if (IsLive || !sameRange)
                        {
                            CpuBeginNs = view.SelectionBeginNs;
                            CpuEndNs = view.SelectionEndNs;
                        }
                        View = view;
                        RebuildStatisticPresentation();
                        m_ViewDirty = false;
                        ViewResult = CaptureResult.Ok;
                        if (info.State == CaptureState.Stopped)
                        {
                            IsLive = false;
                        }
                        if (!IsLive)
                        {
                            m_FirstFrame = view.FirstFrame;
                            m_SelectionFirst = view.SelectionFirst;
                            m_SelectionCount = (int)view.SelectionCount;
                        }
                    }
                    else
                    {
                        ViewResult = result;
                        // Busy preserves the previous complete view; an explicit request is retried by polling.
                        m_ViewDirty = result == CaptureResult.Busy;
                        if (!m_ViewDirty)
                        {
                            // Do not repeatedly rebuild an oversized CPU query at the live refresh rate.
                            IsLive = false;
                        }
                    }
                }
                else if (result != CaptureResult.Ok)
                {
                    ViewResult = result;
                }
                Changed?.Invoke();
            }
            catch (DllNotFoundException exception)
            {
                m_NativeAvailable = false;
                LoadError = exception.Message;
                ResetView();
                Info = new CaptureInfo { State = CaptureState.Unavailable, Error = CaptureResult.Unavailable };
                Changed?.Invoke();
            }
            catch (EntryPointNotFoundException exception)
            {
                m_NativeAvailable = false;
                LoadError = exception.Message;
                ResetView();
                Info = new CaptureInfo { State = CaptureState.Unavailable, Error = CaptureResult.Unavailable };
                Changed?.Invoke();
            }
        }

    }
}
