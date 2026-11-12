using System;
using System.Collections.Generic;
using SE.Editor.GUI;
using SE.GUI;
using CaptureInfo = SE.ProfilerCaptureInfo;
using CaptureFrame = SE.ProfilerCaptureFrame;
using CaptureState = SE.ProfilerCaptureState;
using CaptureResult = SE.ProfilerCaptureResult;
using StopReason = SE.ProfilerStopReason;
using FrameViewInfo = SE.ProfilerFrameViewInfo;
using CpuThread = SE.ProfilerCpuThread;
using CpuEvent = SE.ProfilerCpuEvent;
using CpuStatistic = SE.ProfilerCpuStatistic;

namespace SE.Editor
{
    /// <summary>CPU history, timeline, source statistics and their page interactions.</summary>
    public sealed class ProfilerCpuMode : ProfilerMode
    {
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
        public int GetVisibleStatisticIndex(int row) => m_VisibleStatistics[row];
        public int GetVisibleStatisticPosition(int index) => m_VisibleStatistics.IndexOf(index);
        /// <summary>Presentation filtering uses cached source names/functions; it does not change query coverage.</summary>
        public void FilterCpuStatistics(string filter)
        {
            filter ??= string.Empty;
            if (StatisticFilter == filter)
                return;
            Profiler.Pause();
            StatisticFilter = filter;
            RebuildStatisticPresentation();
            Profiler.NotifyChanged();
        }

        public void SortCpuStatistics(StatisticSortColumn column)
        {
            Profiler.Pause();
            StatisticSortDescending = column == StatisticSort ? !StatisticSortDescending : column != StatisticSortColumn.Name;
            StatisticSort = column;
            RebuildStatisticPresentation();
            Profiler.NotifyChanged();
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
            Profiler.Pause();
            SelectedCpuStatistic = index;
            SelectedCpuEvent = -1;
            CpuEventDetails = DescribeCpuStatistic(index);
            Profiler.NotifyChanged();
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
        public FrameViewInfo View { get; private set; }
        public ReadOnlySpan<CaptureFrame> Frames => new ReadOnlySpan<CaptureFrame>(m_Frames, 0, View.FrameCount);
        public CaptureFrame? Frame => View.SelectionCount == 1 ? m_Frames[(int)(View.SelectionFirst - View.FirstFrame)] : null;
        /// <summary>Selects a complete frame, loading its historical page if needed; viewing remains paused.</summary>
        public CaptureResult ReadFrame(ulong index)
        {
            return SelectFrames(index, index);
        }
        /// <summary>Selects an inclusive range of up to 256 complete frames. Times use [first.Begin, last.End).</summary>
        public CaptureResult SelectFrames(ulong first, ulong last)
        {
            if (Profiler.ActiveMode != this)
            {
                return CaptureResult.NotReady;
            }
            if (!Profiler.NativeAvailable || Profiler.Info.State == CaptureState.Unavailable)
            {
                ViewResult = CaptureResult.Unavailable;
                Profiler.NotifyChanged();
                return ViewResult;
            }
            if (Profiler.Info.State != CaptureState.Capturing && Profiler.Info.State != CaptureState.Stopped)
            {
                ViewResult = CaptureResult.NotReady;
                Profiler.NotifyChanged();
                return ViewResult;
            }
            // The view copy can observe newer complete frames than the preceding non-blocking state poll.
            ulong completeFrames = Math.Max(Profiler.Info.CompleteFrames, View.TotalFrames);
            if (first < Profiler.OldestFrame || last < first || last >= completeFrames)
            {
                ViewResult = CaptureResult.InvalidRange;
                Profiler.NotifyChanged();
                return ViewResult;
            }
            if (last - first >= ProfilerModule.MaxViewFrames)
            {
                ViewResult = CaptureResult.QueryTooLarge;
                Profiler.NotifyChanged();
                return ViewResult;
            }
            Profiler.Pause();
            m_FirstFrame = View.FrameCount > 0 && first >= View.FirstFrame && last - View.FirstFrame < (ulong)View.FrameCount ? View.FirstFrame : first;
            m_SelectionFirst = first;
            m_SelectionCount = (int)(last - first + 1);
            Dirty = true;
            ViewResult = CaptureResult.NotReady;
            Profiler.RefreshView();
            return ViewResult;
        }

        public void BrowseFrames(bool newer)
        {
            if (Profiler.ActiveMode != this || View.FrameCount == 0)
            {
                return;
            }
            Profiler.Pause();
            ulong completeFrames = Math.Max(Profiler.Info.CompleteFrames, View.TotalFrames);
            ulong latest = Math.Max(Profiler.OldestFrame, completeFrames > ProfilerModule.MaxViewFrames ? completeFrames - ProfilerModule.MaxViewFrames : 0);
            m_FirstFrame = newer ? Math.Min(View.FirstFrame + ProfilerModule.MaxViewFrames, latest) :
                Math.Max(Profiler.OldestFrame, View.FirstFrame > ProfilerModule.MaxViewFrames ? View.FirstFrame - ProfilerModule.MaxViewFrames : 0);
            m_SelectionCount = 0;
            Dirty = true;
            Profiler.RefreshView();
        }

        /// <summary>Changes the semantic thread filter; folding and display navigation never query native data.</summary>
        public CaptureResult SelectCpuThread(ulong threadId)
        {
            if (Profiler.ActiveMode != this)
            {
                return CaptureResult.NotReady;
            }
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
            Profiler.Pause();
            m_ThreadId = threadId;
            Dirty = true;
            Profiler.RefreshView();
            return ViewResult;
        }

        public string GetCpuText(int offset) => m_CpuText.TryGetValue(offset, out string text) ? text : string.Empty;
        public string GetThreadOptionText(int offset) => m_ThreadOptionText.TryGetValue(offset, out string text) ? text : string.Empty;
        public bool IsCpuThreadCollapsed(ulong id) => m_CollapsedThreads.Contains(id);

        /// <summary>One explicit directory-page query; it neither replaces nor claims the current analysis snapshot.</summary>
        public CaptureResult LoadCpuThreads(int first)
        {
            if (!Profiler.NativeAvailable)
                return CaptureResult.Unavailable;
            CaptureResult result = Profiling.ReadCaptureThreads(Profiler.Info.SessionId, first, out int total, out CpuThread[] threads, out byte[] text);
            if (result == CaptureResult.Ok)
            {
                m_ThreadOptionText = DecodeText(text);
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
            Profiler.NotifyChanged();
        }

        /// <summary>Snapshot-local selection. A stale identity cannot select a new event at the same array index.</summary>
        public CaptureResult SelectCpuEvent(ulong snapshotId, int index)
        {
            if (snapshotId == 0 || snapshotId != View.SnapshotId)
                return CaptureResult.InvalidSnapshot;
            if (index < 0 || index >= m_CpuEvents.Length)
                return CaptureResult.InvalidRange;
            Profiler.Pause();
            SelectedCpuEvent = index;
            SelectedCpuStatistic = m_StatisticBySource.TryGetValue(m_CpuEvents[index].SourceLocationId, out int statistic) ? statistic : -1;
            CpuEventDetails = DescribeCpuEvent(index);
            Profiler.NotifyChanged();
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
            Profiler.Pause();
            CpuBeginNs = Math.Clamp(beginNs, View.SelectionBeginNs, View.SelectionEndNs - duration);
            CpuEndNs = CpuBeginNs + duration;
            Profiler.NotifyChanged();
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
            Profiler.NotifyChanged();
        }

        /// <summary>
        /// Main-thread copy of a frozen native view into a separate caller-owned array. Releasing this result
        /// does not release the snapshot; reads of a replaced snapshot return InvalidSnapshot.
        /// </summary>
        public CaptureResult ReadView(ulong sessionId, ulong snapshotId, out FrameViewInfo view, out CaptureFrame[] frames)
        {
            view = default;
            frames = Array.Empty<CaptureFrame>();
            if (!Profiler.NativeAvailable)
            {
                return CaptureResult.Unavailable;
            }
            CaptureResult result = Profiling.ReadFrameView(sessionId, snapshotId, out view, out frames);
            frames ??= Array.Empty<CaptureFrame>();
            return result;
        }

        public ProfilerCpuMode(ProfilerModule profiler) : base(profiler)
        {
        }

        public override string Title => "CPU";
        public override bool HasSnapshot => View.SnapshotId != 0;
        protected override ScrollBars ContentScrollBars => ScrollBars.None;

        public void ShowStatistics(bool statistics)
        {
            IsCpuStatisticsVisible = statistics;
            Profiler.NotifyChanged();
        }

        internal override void Freeze()
        {
            if (HasSnapshot)
            {
                m_FirstFrame = View.FirstFrame;
                m_SelectionFirst = View.SelectionFirst;
                m_SelectionCount = (int)View.SelectionCount;
                Dirty = false;
            }
        }

        internal override void ShowLatest()
        {
            m_FirstFrame = ulong.MaxValue;
            m_SelectionCount = 0;
            Dirty = true;
        }

        internal override void Reset()
        {
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
            m_FirstFrame = ulong.MaxValue;
            m_SelectionFirst = 0;
            m_SelectionCount = 0;
            Dirty = true;
            ViewResult = CaptureResult.Ok;
        }

        internal override CaptureResult Query(out CaptureInfo info)
        {
            CaptureResult result = Profiling.RefreshFrameView(Profiler.Info.SessionId, Profiler.IsLive ? ulong.MaxValue : m_FirstFrame,
                ProfilerModule.MaxViewFrames, Profiler.IsLive ? 0 : m_SelectionFirst, Profiler.IsLive ? 0 : m_SelectionCount, m_ThreadId,
                out info, out FrameViewInfo view, out CaptureFrame[] frames, out CpuThread[] threads, out CpuEvent[] events,
                out byte[] text, out CpuStatistic[] statistics);

            if (result != CaptureResult.Ok)
            {
                return result;
            }

            Dictionary<int, string> strings = DecodeText(text);
            bool sameRange = View.SelectionBeginNs == view.SelectionBeginNs && View.SelectionEndNs == view.SelectionEndNs;
            m_Frames = frames ?? Array.Empty<CaptureFrame>();
            m_CpuThreads = threads ?? Array.Empty<CpuThread>();
            m_CpuEvents = events ?? Array.Empty<CpuEvent>();
            m_CpuText = strings;

            int mainThreadIndex = -1;
            for (int i = 0; i < m_CpuThreads.Length; i++)
            {
                if (GetCpuText(m_CpuThreads[i].NameOffset) == "Main")
                {
                    mainThreadIndex = i;
                    break;
                }
            }

            if (mainThreadIndex > 0)
            {
                CpuThread main = m_CpuThreads[mainThreadIndex];
                Array.Copy(m_CpuThreads, 0, m_CpuThreads, 1, mainThreadIndex);
                m_CpuThreads[0] = main;
            }

            m_CpuStatistics = statistics ?? Array.Empty<CpuStatistic>();
            m_StatisticBySource.Clear();
            for (int i = 0; i < m_CpuStatistics.Length; i++)
            {
                m_StatisticBySource.Add(m_CpuStatistics[i].SourceLocationId, i);
            }
            SelectedCpuStatistic = -1;
            SelectedCpuEvent = -1;
            CpuEventDetails = "Select a CPU event to inspect it.";
            if (Profiler.IsLive || !sameRange)
            {
                CpuBeginNs = view.SelectionBeginNs;
                CpuEndNs = view.SelectionEndNs;
            }

            View = view;
            RebuildStatisticPresentation();

            if (!Profiler.IsLive || info.State == CaptureState.Stopped)
            {
                Freeze();
            }

            return result;
        }

        private FrameOverview m_Overview;
        private ContainerControl m_Navigation;
        private Button m_Older;
        private Button m_Previous;
        private Button m_Next;
        private Button m_Newer;
        private Button m_TimelinePage;
        private Button m_StatisticsPage;
        private Button m_ThreadFilter;
        private Button m_FitCpu;
        private TextBox m_StatisticFilter;
        private ProfilerCpuTimeline m_CpuTimeline;
        private ProfilerCpuStatistics m_CpuStatisticsControl;
        private bool m_LastStatisticsPage;
        private int m_LastCpuFocusVersion;

        protected override void CreateControls()
        {
            m_TimelinePage = m_Content.AddChild(new Button { Text = "Timeline" });
            m_TimelinePage.Clicked += () => ShowStatistics(false);
            m_StatisticsPage = m_Content.AddChild(new Button { Text = "Statistics" });
            m_StatisticsPage.Clicked += () => ShowStatistics(true);
            m_Overview = m_Content.AddChild(new FrameOverview(this));
            // Chart-owned controls inherit visibility and scrolling from the frame overview.
            m_Navigation = m_Overview.AddChild(new ContainerControl());
            m_Older = m_Navigation.AddChild(new Button { Text = "<<", TooltipText = "PreviousPage" });
            m_Older.Clicked += () => BrowseFrames(false);
            m_Previous = m_Navigation.AddChild(new Button { Text = "<", TooltipText = "Previous" });
            m_Previous.Clicked += () =>
            {
                FrameViewInfo view = View;
                if (view.SelectionCount > 0 && view.SelectionFirst > Profiler.OldestFrame)
                {
                    ReadFrame(view.SelectionFirst - 1);
                }
            };
            m_Next = m_Navigation.AddChild(new Button { Text = ">", TooltipText = "Next" });
            m_Next.Clicked += () =>
            {
                FrameViewInfo view = View;
                if (view.SelectionCount > 0)
                {
                    ReadFrame(view.SelectionFirst + view.SelectionCount);
                }
            };
            m_Newer = m_Navigation.AddChild(new Button { Text = ">>", TooltipText = "NextPage" });
            m_Newer.Clicked += () => BrowseFrames(true);
            m_StatisticFilter = m_Content.AddChild(new TextBox { WatermarkText = "Filter source name/function" });
            m_StatisticFilter.TextChanged += () => FilterCpuStatistics(m_StatisticFilter.Text);
            m_ThreadFilter = m_Content.AddChild(new Button { Text = "Threads: All Threads" });
            m_ThreadFilter.Clicked += () => ShowThreadFilterPage(0);
            m_FitCpu = m_Content.AddChild(new Button { Text = "Fit range", TooltipText = "Fit the frozen frame selection without querying native data." });
            m_FitCpu.Clicked += () => SetCpuRange(View.SelectionBeginNs, View.SelectionEndNs);
        }

        protected override void UpdateControls()
        {
            if (m_CpuTimeline == null)
            {
                m_CpuTimeline = m_Split.Panel1.AddChild(new ProfilerCpuTimeline(this));
                m_CpuStatisticsControl = m_Split.Panel1.AddChild(new ProfilerCpuStatistics(this));
            }
            bool statistics = IsCpuStatisticsVisible;
            if (m_StatisticFilter.Text != StatisticFilter)
            {
                m_StatisticFilter.Text = StatisticFilter;
            }
            m_CpuStatisticsControl.Visible = statistics;
            m_CpuTimeline.Visible = !statistics;
            m_StatisticFilter.Visible = statistics;
            m_FitCpu.Visible = !statistics;
            m_TimelinePage.Enabled = statistics;
            m_StatisticsPage.Enabled = !statistics;
            string threadFilter = "All Threads";
            foreach (CpuThread thread in CpuThreads)
            {
                if (thread.Id == View.FilterThreadId)
                {
                    threadFilter = $"{GetCpuText(thread.NameOffset)} [#{thread.Id}; OS ID unavailable]";
                }
            }
            m_ThreadFilter.Text = $"Threads: {threadFilter}";
            m_ThreadFilter.Enabled = Profiler.Info.State == CaptureState.Capturing || Profiler.Info.State == CaptureState.Stopped;
            m_FitCpu.Enabled = View.SelectionEndNs > View.SelectionBeginNs;
            m_Status.Text = $"Snapshot: {View.SnapshotId}    CPU events: {CpuEvents.Length}    End unknown: {View.IncompleteCpuEvents}\n" +
                $"Statistics: {VisibleStatisticCount} / {CpuStatistics.Length} sources    Range: {View.SelectionBeginNs / 1_000_000.0:0.##} - {View.SelectionEndNs / 1_000_000.0:0.##} ms\n" +
                (statistics ? "Complete calls only; header: sort, row: inspect, double-click: locate. Not CPU utilization." :
                    "Ctrl+wheel: zoom    Middle-drag: pan    Click thread: fold    Incomplete: Duration unavailable");
            m_EventDetails.Text = CpuEventDetails;
            ulong completeFrames = Math.Max(Profiler.Info.CompleteFrames, View.TotalFrames);
            m_Older.Enabled = View.FrameCount > 0 && View.FirstFrame > Profiler.OldestFrame;
            m_Newer.Enabled = View.FrameCount > 0 && View.FirstFrame + (ulong)View.FrameCount < completeFrames;
            m_Previous.Enabled = View.SelectionCount > 0 && View.SelectionFirst > Profiler.OldestFrame;
            m_Next.Enabled = View.SelectionCount > 0 && View.SelectionFirst + View.SelectionCount < completeFrames;
            if (m_LastStatisticsPage != statistics)
            {
                m_LastStatisticsPage = statistics;
                m_Split.Panel1.SetViewOffset(Float2.Zero);
                int row = GetVisibleStatisticPosition(SelectedCpuStatistic);
                if (statistics && row >= 0)
                {
                    m_Split.Panel1.ScrollViewTo(new Rectangle(0, (row + 1) * ProfilerCpuStatistics.RowHeight,
                        m_CpuStatisticsControl.Width, ProfilerCpuStatistics.RowHeight), true);
                }
            }
            if (m_LastCpuFocusVersion != CpuFocusVersion)
            {
                m_LastCpuFocusVersion = CpuFocusVersion;
                m_CpuTimeline.ScrollToSelectedEvent();
            }
        }

        protected override float LayoutControls(float width)
        {
            float tabWidth = MathF.Min(140, width / 2);
            m_TimelinePage.SetBounds(8, 4, tabWidth, 24);
            m_StatisticsPage.SetBounds(8 + tabWidth, 4, tabWidth, 24);
            m_Overview.SetBounds(8, 32, width, Math.Clamp(m_Content.Height * 0.35f, 100, 180));
            m_Navigation.SetBounds(0, 0, width, 24);
            float buttonWidth = MathF.Min(28, width / 4);
            m_Older.SetBounds(0, 2, buttonWidth, 20);
            m_Previous.SetBounds(buttonWidth, 2, buttonWidth, 20);
            m_Next.SetBounds(width - buttonWidth * 2, 2, buttonWidth, 20);
            m_Newer.SetBounds(width - buttonWidth, 2, buttonWidth, 20);
            float y = m_Overview.Bottom + 8;
            float filterWidth = IsCpuStatisticsVisible ? width * 0.5f : MathF.Min(76, width * 0.3f);
            m_ThreadFilter.SetBounds(8, y, MathF.Max(1, width - filterWidth - 4), 24);
            m_StatisticFilter.SetBounds(8 + width - filterWidth, y, filterWidth, 24);
            m_FitCpu.SetBounds(8 + width - filterWidth, y, filterWidth, 24);
            return y + 28;
        }

        protected override void LayoutContent()
        {
            if (m_CpuTimeline == null || m_EventDetails == null)
            {
                return;
            }
            float width = MathF.Max(1, m_Split.Panel1.GetDesireClientArea().Width);
            m_CpuTimeline.SetBounds(0, 0, width, m_CpuTimeline.RequiredHeight);
            m_CpuStatisticsControl.SetBounds(0, 0, MathF.Max(ProfilerCpuStatistics.MinimumWidth, width), m_CpuStatisticsControl.RequiredHeight);
        }

        protected override string GetDataInfo()
        {
            ProfilerModule profiler = Profiler;
            CaptureInfo info = profiler.Info;
            FrameViewInfo view = View;
            string text = $"Snapshot: {view.SnapshotId}    Complete frames at snapshot: {view.TotalFrames}\n";
            if (info.StopReason != StopReason.None)
            {
                text += $"Stop reason: {info.StopReason}\n";
            }
            text += $"Retained history: up to {profiler.HistoryCapacity} engine samples; oldest identity {profiler.OldestFrame}.\n";
            if (view.FrameCount > 0)
            {
                text += $"Visible frames: {view.FirstFrame} - {view.FirstFrame + (ulong)view.FrameCount - 1} ({view.FrameCount} of {view.TotalFrames})\n";
                text += $"View time: {view.BeginNs / 1_000_000.0:0.##} - {view.EndNs / 1_000_000.0:0.##} ms\n";
                text += $"Selected frames: {view.SelectionFirst} - {view.SelectionFirst + view.SelectionCount - 1}\n";
                text += $"Selection time: {view.SelectionBeginNs / 1_000_000.0:0.##} - {view.SelectionEndNs / 1_000_000.0:0.##} ms\n";
                long duration = view.SelectionEndNs - view.SelectionBeginNs;
                long sampleDuration = 0;
                foreach (CaptureFrame frame in Frames)
                {
                    if (frame.Index >= view.SelectionFirst && frame.Index - view.SelectionFirst < view.SelectionCount)
                        sampleDuration += Math.Max(0, frame.EndNs - frame.BeginNs);
                }
                text += $"Selection duration: {duration / 1_000_000.0:0.##} ms\n";
                text += sampleDuration > 0 ?
                    $"Average frame: {sampleDuration / 1_000_000.0 / view.SelectionCount:0.##} ms    FPS: {view.SelectionCount * 1_000_000_000.0 / sampleDuration:0.##}\n" :
                    "FPS unavailable for a zero-duration interval.\n";
                text += "Bars show sampling intervals; completed CPU calls may start before their extraction batch.\n";
            }
            else
            {
                text += "No complete captured frame is available.\n";
            }
            if (view.HasPartialFrame != 0)
            {
                text += "Final open frame is incomplete and excluded from the bars and selection.\n";
            }
            if (info.State == CaptureState.Stopped)
            {
                text += $"Recording cutoff (local monotonic time): {info.LastTimeNs / 1_000_000.0:0.##} ms\nNative history remains available until Clear.\n";
                if (info.CompleteFrames > 0)
                {
                    text += $"Retained complete frames: {profiler.OldestFrame} - {info.CompleteFrames - 1}\n";
                    text += $"Complete frame coverage: {info.CompleteBeginNs / 1_000_000.0:0.##} - {info.CompleteEndNs / 1_000_000.0:0.##} ms\n";
                }
            }
            if (info.TailMayBeIncomplete != 0)
            {
                text += "The final partial frame and events may be incomplete.\n";
            }
            if (Profiler.IsViewPending)
            {
                text += "View refresh pending; the previous complete snapshot remains displayed.\n";
            }
            text += $"Query page limit: {ProfilerModule.MaxViewFrames} complete frames. Native snapshot, temporary List, interop arrays and managed copies cost additional memory.\n";
            text += "CPU query limits: 65536 events, 256 threads, 128 levels, 1 MiB UTF-8 text (candidate limits; not performance measured).\n";
            text += "CPU events belong to the selected extraction batches and explicit thread filter. Folding changes display only.\n";
            text += "Native source statistics cover every CPU event in this successful snapshot, independently of display aggregation and name filtering.\n";
            text += "Total/average/max use complete calls with original durations; unknown ends are counted separately. Self subtracts direct-child coverage once; missing child ends make self incomplete.\n";
            text += "Cross-thread and nested inclusive totals can exceed elapsed time; they are not CPU utilization.\n";
            if (ViewResult == CaptureResult.QueryTooLarge)
            {
                text += "Query exceeds frame/CPU capacity; the previous snapshot is retained. Select fewer frames or filter a thread, then Refresh.\n";
            }
            else if (ViewResult == CaptureResult.InvalidRange)
            {
                text += "Query range is invalid; the previous snapshot is retained.\n";
            }
            else if (ViewResult == CaptureResult.InvalidSnapshot)
            {
                text += "The requested snapshot has expired. Refresh to establish a new snapshot.\n";
            }
            if (info.Error != CaptureResult.Ok || profiler.LastResult != CaptureResult.Ok || ViewResult != CaptureResult.Ok)
            {
                text += $"Capture error: {info.Error}    Last capture operation: {profiler.LastResult}    View read: {ViewResult}\n";
            }
            if (!string.IsNullOrEmpty(profiler.LoadError))
            {
                text += $"Native profiler could not be loaded: {profiler.LoadError}\n";
            }
            return text;
        }

        private void ShowThreadFilterPage(int first)
        {
            ContextMenu menu = new ContextMenu();
            CaptureResult result = LoadCpuThreads(first);
            if (result != CaptureResult.Ok)
            {
                menu.AddButton($"Thread directory: {result}").Enabled = false;
                menu.AddButton("Retry", () => ShowThreadFilterPage(first));
                menu.Show(m_ThreadFilter, new Float2(0, m_ThreadFilter.Height));
                return;
            }
            menu.AddButton("All Threads", () => SelectCpuThread(0));
            menu.AddButton($"Threads {first} - {first + ThreadOptions.Length} of {TotalThreadOptions}").Enabled = false;
            if (first > 0)
                menu.AddButton("Previous thread page", () => ShowThreadFilterPage(Math.Max(0, first - 256)));
            if (first + ThreadOptions.Length < TotalThreadOptions)
                menu.AddButton("Next thread page", () => ShowThreadFilterPage(first + 256));
            foreach (CpuThread thread in ThreadOptions)
            {
                ulong id = thread.Id;
                menu.AddButton($"{GetThreadOptionText(thread.NameOffset)} [{id}]", () => SelectCpuThread(id));
            }
            menu.Show(m_ThreadFilter, new Float2(0, m_ThreadFilter.Height));
        }

        private sealed class FrameOverview : ContainerControl
        {
            private readonly ProfilerCpuMode m_Profiler;
            private bool m_Dragging;
            private int m_First;
            private int m_Last;
            private int m_Hover = -1;
            private ulong m_DragSnapshot;

            public FrameOverview(ProfilerCpuMode profiler)
            {
                m_Profiler = profiler;
                TooltipText = "Click a frame or drag a range. Scroll to browse historical pages.";
            }

            private Rectangle Plot => new Rectangle(4, 28, MathF.Max(1, Width - 8), MathF.Max(1, Height - 48));

            public override void DrawSelf()
            {
                base.DrawSelf();
                Style style = Style.Current;
                Rectangle plot = Plot;
                Render2D.FillRectangle(new Rectangle(Float2.Zero, Size), style.LightBackground);
                ReadOnlySpan<CaptureFrame> frames = m_Profiler.Frames;
                FrameViewInfo view = m_Profiler.View;
                if (frames.Length == 0)
                {
                    Render2D.RenderText(style.FontMedium, "No complete frame in this view", Plot, style.TextColor);
                    return;
                }
                double maximum = 0.001;
                for (int i = 0; i < frames.Length; i++)
                {
                    maximum = Math.Max(maximum, (frames[i].EndNs - frames[i].BeginNs) / 1_000_000.0);
                }
                float navigationWidth = MathF.Min(56, Width / 2);
                Rectangle scale = new Rectangle(navigationWidth + 4, 2, MathF.Max(0, Width - navigationWidth * 2 - 8), 20);
                Render2D.PushClip(scale);
                Render2D.RenderText(style.FontMedium, $"{maximum:0.##} ms", scale, style.TextColor);
                Render2D.PopClip();
                float step = plot.Width / frames.Length;
                for (int i = 0; i < frames.Length; i++)
                {
                    bool selected = frames[i].Index >= view.SelectionFirst && frames[i].Index - view.SelectionFirst < view.SelectionCount;
                    if (m_Dragging && m_DragSnapshot == view.SnapshotId)
                    {
                        selected = i >= Math.Min(m_First, m_Last) && i <= Math.Max(m_First, m_Last);
                    }
                    float barHeight = MathF.Max(1, (float)((frames[i].EndNs - frames[i].BeginNs) / 1_000_000.0 / maximum * plot.Height));
                    Render2D.FillRectangle(new Rectangle(plot.X + i * step, plot.Y + plot.Height - barHeight, MathF.Max(0.25f, step - 1), barHeight),
                        selected ? style.BackgroundSelected : style.Foreground);
                }
                Render2D.RenderText(style.FontMedium, $"{view.FirstFrame} - {view.FirstFrame + (ulong)frames.Length - 1}    Click / drag to select",
                    new Rectangle(4, Height - 18, Width - 8, 18), style.TextColor);
            }

            public override bool OnMouseDown(Float2 location, MouseButton button)
            {
                if (button == MouseButton.Left && m_Profiler.View.FrameCount > 0 && Plot.Contains(location))
                {
                    m_Profiler.Profiler.Pause();
                    Focus();
                    m_First = m_Last = Math.Clamp((int)((location.X - Plot.X) / Plot.Width * m_Profiler.View.FrameCount), 0, m_Profiler.View.FrameCount - 1);
                    m_DragSnapshot = m_Profiler.View.SnapshotId;
                    m_Dragging = true;
                    StartMouseCapture();
                    return true;
                }
                return base.OnMouseDown(location, button);
            }

            public override void OnMouseEnter(Float2 location)
            {
                int count = m_Profiler.View.FrameCount;
                m_Hover = count > 0 && Plot.Contains(location) ? Math.Clamp((int)((location.X - Plot.X) / Plot.Width * count), 0, count - 1) : -1;
                base.OnMouseEnter(location);
            }

            public override void OnMouseMove(Float2 location)
            {
                int count = m_Profiler.View.FrameCount;
                int hover = count > 0 && Plot.Contains(location) ? Math.Clamp((int)((location.X - Plot.X) / Plot.Width * count), 0, count - 1) : -1;
                if (hover != m_Hover)
                {
                    if (m_Hover >= 0)
                    {
                        GetTooltip()?.OnMouseLeaveControl(this);
                    }
                    m_Hover = hover;
                    if (hover >= 0 && m_TooltipUpdate != null)
                    {
                        // Treat another bar as a fresh hover; text is requested only when the tooltip opens.
                        GetTooltip()?.OnMouseEnterControl(this);
                    }
                }
                if (m_Dragging && count > 0)
                {
                    m_Last = Math.Clamp((int)((location.X - Plot.X) / Plot.Width * count), 0, count - 1);
                }
                base.OnMouseMove(location);
            }

            public override void OnMouseLeave()
            {
                m_Hover = -1;
                base.OnMouseLeave();
            }

            public override bool OnTestTooltipOverControl(Float2 location)
            {
                return m_Profiler.View.FrameCount > 0 && Plot.Contains(location) && base.OnTestTooltipOverControl(location);
            }

            public override bool OnMouseUp(Float2 location, MouseButton button)
            {
                if (button == MouseButton.Left && m_Dragging)
                {
                    FrameViewInfo view = m_Profiler.View;
                    // Clear or another semantic operation can invalidate a drag that was already in progress.
                    if (view.SnapshotId == m_DragSnapshot)
                    {
                        m_Last = Math.Clamp((int)((location.X - Plot.X) / Plot.Width * view.FrameCount), 0, view.FrameCount - 1);
                        m_Profiler.SelectFrames(view.FirstFrame + (ulong)Math.Min(m_First, m_Last), view.FirstFrame + (ulong)Math.Max(m_First, m_Last));
                    }
                    m_Dragging = false;
                    EndMouseCapture();
                    return true;
                }
                return base.OnMouseUp(location, button);
            }

            public override void OnEndMouseCapture()
            {
                m_Dragging = false;
                base.OnEndMouseCapture();
            }

            public override bool OnMouseWheel(Float2 location, float delta)
            {
                if (!m_Dragging && m_Profiler.View.FrameCount > 0 && delta != 0)
                {
                    m_Profiler.BrowseFrames(delta < 0);
                    return true;
                }
                return base.OnMouseWheel(location, delta);
            }

            public override bool OnShowTooltip(out string text, out Float2 location, out Rectangle area)
            {
                bool show = base.OnShowTooltip(out text, out location, out area);
                ReadOnlySpan<CaptureFrame> frames = m_Profiler.Frames;
                if (m_Hover >= 0 && m_Hover < frames.Length)
                {
                    float step = Plot.Width / frames.Length;
                    area = new Rectangle(Plot.X + m_Hover * step, Plot.Y, step, Plot.Height);
                    CaptureFrame frame = frames[m_Hover];
                    long duration = frame.EndNs - frame.BeginNs;
                    text = $"Frame {frame.Index}\nDuration: {duration / 1_000_000.0:0.##} ms\n";
                    text += duration > 0 ? $"FPS: {1_000_000_000.0 / duration:0.##}" : "FPS unavailable";
                    return show;
                }
                return false;
            }
        }
    }
}
