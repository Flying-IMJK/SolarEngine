using System;
using SE.Editor.GUI;
using SE.GUI;
using CaptureInfo = SE.ProfilerCaptureInfo;
using CaptureState = SE.ProfilerCaptureState;
using CaptureResult = SE.ProfilerCaptureResult;
using CaptureFrame = SE.ProfilerCaptureFrame;
using FrameViewInfo = SE.ProfilerFrameViewInfo;
using StopReason = SE.ProfilerStopReason;
using CpuThread = SE.ProfilerCpuThread;
using GpuFrame = SE.ProfilerGpuFrame;

namespace SE.Editor
{
    /// <summary>
    /// Capture controls and a bounded, selectable view of complete recorded frames.
    /// </summary>
    public sealed class ProfilerWindow : EditorWindow
    {
        private ToolStrip m_ToolStrip;
        private ContainerControl m_Navigation;
        private ContainerControl m_MemoryNavigation;
        private Button m_MemoryFirst;
        private Button m_MemoryPrevious;
        private Button m_MemoryNext;
        private Button m_MemoryLast;
        private Panel m_Content;
        private ToolStripButton m_Start;
        private ToolStripButton m_Stop;
        private ToolStripButton m_Clear;
        private ToolStripButton m_Pause;
        private ToolStripButton m_Live;
        private ToolStripButton m_Refresh;
        private Button m_Older;
        private Button m_Newer;
        private Button m_Previous;
        private Button m_Next;
        private Label m_Status;
        private Label m_Details;
        private DataInfoButton m_DataInfoToggle;
        private Label m_DataInfo;
        private FrameOverview m_Overview;
        private SplitPanel m_CpuSplit;
        private ProfilerCpuTimeline m_CpuTimeline;
        private ProfilerCpuStatistics m_CpuStatistics;
        private ProfilerGpuHierarchy m_GpuHierarchy;
        private ProfilerMemoryView m_MemoryView;
        private Button m_MemoryPage;
        private bool m_LastMemoryPage;
        private Button m_GpuPage;
        private Button m_GpuFrame;
        private Button m_GpuPrevious;
        private Button m_GpuNext;
        private bool m_LastGpuPage;
        private Button m_TimelinePage;
        private Button m_StatisticsPage;
        private TextBox m_StatisticFilter;
        private bool m_LastStatisticsPage;
        private int m_LastCpuFocusVersion;
        private Button m_ThreadFilter;
        private Button m_FitCpu;
        private Label m_CpuStatus;
        private Label m_EventDetails;

        public ProfilerWindow(Editor editor) : base(editor, true)
        {
            Title = "Profiler";
            m_ToolStrip = AddChild(new ToolStrip(24.0f, 0.0f));
            m_Start = m_ToolStrip.AddButton("Start", () => Editor.Profiler.Start());
            m_Stop = m_ToolStrip.AddButton("Stop", () => Editor.Profiler.Stop());
            m_Clear = m_ToolStrip.AddButton("Clear", () => Editor.Profiler.Clear());
            m_ToolStrip.AddSeparator();
            m_Pause = m_ToolStrip.AddButton("Pause", Editor.Profiler.Pause);
            m_Live = m_ToolStrip.AddButton("Live", Editor.Profiler.ShowLatestFrame);
            m_Refresh = m_ToolStrip.AddButton("Refresh", Editor.Profiler.RefreshView);
            // The content viewport keeps all scrolling information below the fixed capture toolbar.
            m_Content = AddChild(new Panel(ScrollBars.Vertical)
            {
                IsScrollable = false,
            });

            m_MemoryNavigation = m_Content.AddChild(new ContainerControl());
            m_MemoryFirst = m_MemoryNavigation.AddChild(new Button { Text = "<<", TooltipText = "PreviousPage" });
            m_MemoryFirst.Clicked += () => Editor.Profiler.BrowseMemory(false, true);
            m_MemoryPrevious = m_MemoryNavigation.AddChild(new Button { Text = "<", TooltipText = "Previous" });
            m_MemoryPrevious.Clicked += () => Editor.Profiler.BrowseMemory(false, false);
            m_MemoryNext = m_MemoryNavigation.AddChild(new Button { Text = ">", TooltipText = "Next" });
            m_MemoryNext.Clicked += () => Editor.Profiler.BrowseMemory(true, false);
            m_MemoryLast = m_MemoryNavigation.AddChild(new Button { Text = ">>", TooltipText = "NextPage" });
            m_MemoryLast.Clicked += () => Editor.Profiler.BrowseMemory(true, true);
            m_Status = m_Content.AddChild(new Label
            {
                HorizontalAlignment = TextAlignment.Near,
                VerticalAlignment = TextAlignment.Near,
            });
            m_TimelinePage = m_Content.AddChild(new Button { Text = "CPU Timeline" });
            m_TimelinePage.Clicked += () => Editor.Profiler.ShowCpuPage(false);
            m_StatisticsPage = m_Content.AddChild(new Button { Text = "CPU Statistics" });
            m_StatisticsPage.Clicked += () => Editor.Profiler.ShowCpuPage(true);
            m_GpuPage = m_Content.AddChild(new Button { Text = "GPU" });
            m_GpuPage.Clicked += Editor.Profiler.ShowGpuPage;
            m_MemoryPage = m_Content.AddChild(new Button { Text = "Memory" });
            m_MemoryPage.Clicked += Editor.Profiler.ShowMemoryPage;
            m_Overview = m_Content.AddChild(new FrameOverview(Editor.Profiler));
            // Chart-owned controls inherit visibility and scrolling from the frame overview.
            m_Navigation = m_Overview.AddChild(new ContainerControl());
            m_Older = m_Navigation.AddChild(new Button { Text = "<<", TooltipText = "PreviousPage" });
            m_Older.Clicked += () => Editor.Profiler.BrowseFrames(false);
            m_Previous = m_Navigation.AddChild(new Button { Text = "<", TooltipText = "Previous" });
            m_Previous.Clicked += () =>
            {
                FrameViewInfo view = Editor.Profiler.View;
                if (view.SelectionCount > 0 && view.SelectionFirst > Editor.Profiler.OldestFrame)
                {
                    Editor.Profiler.ReadFrame(view.SelectionFirst - 1);
                }
            };
            m_Next = m_Navigation.AddChild(new Button { Text = ">", TooltipText = "Next" });
            m_Next.Clicked += () =>
            {
                FrameViewInfo view = Editor.Profiler.View;
                if (view.SelectionCount > 0)
                {
                    Editor.Profiler.ReadFrame(view.SelectionFirst + view.SelectionCount);
                }
            };
            m_Newer = m_Navigation.AddChild(new Button { Text = ">>", TooltipText = "NextPage" });
            m_Newer.Clicked += () => Editor.Profiler.BrowseFrames(true);

            m_GpuFrame = m_Content.AddChild(new Button { Text = "GPU engine frame" });
            m_GpuFrame.Clicked += () => ShowGpuFrames(0);
            m_GpuPrevious = m_Content.AddChild(new Button { Text = "<", TooltipText = "Previous GPU engine frame" });
            m_GpuPrevious.Clicked += () => Editor.Profiler.BrowseGpuFrame(false);
            m_GpuNext = m_Content.AddChild(new Button { Text = ">", TooltipText = "Next GPU engine frame" });
            m_GpuNext.Clicked += () => Editor.Profiler.BrowseGpuFrame(true);
            m_StatisticFilter = m_Content.AddChild(new TextBox { WatermarkText = "Filter source name/function" });
            m_StatisticFilter.TextChanged += () => Editor.Profiler.FilterCpuStatistics(m_StatisticFilter.Text);
            m_ThreadFilter = m_Content.AddChild(new Button { Text = "CPU Timeline: All Threads" });
            m_ThreadFilter.Clicked += ShowThreadFilter;
            m_FitCpu = m_Content.AddChild(new Button { Text = "Fit range", TooltipText = "Fit the frozen frame selection without querying native data." });
            m_FitCpu.Clicked += () => Editor.Profiler.SetCpuRange(Editor.Profiler.View.SelectionBeginNs, Editor.Profiler.View.SelectionEndNs);
            m_CpuStatus = m_Content.AddChild(new Label { HorizontalAlignment = TextAlignment.Near, VerticalAlignment = TextAlignment.Near });
            m_CpuSplit = m_Content.AddChild(new SplitPanel(Orientation.Horizontal, ScrollBars.Both, ScrollBars.Vertical) { SplitterValue = 0.7f });
            m_CpuTimeline = m_CpuSplit.Panel1.AddChild(new ProfilerCpuTimeline(Editor.Profiler));
            m_CpuStatistics = m_CpuSplit.Panel1.AddChild(new ProfilerCpuStatistics(Editor.Profiler));
            m_GpuHierarchy = m_CpuSplit.Panel1.AddChild(new ProfilerGpuHierarchy(Editor.Profiler));
            m_MemoryView = m_CpuSplit.Panel1.AddChild(new ProfilerMemoryView(Editor.Profiler));
            m_EventDetails = m_CpuSplit.Panel2.AddChild(new Label
            {
                AnchorPreset = AnchorPresets.HorizontalStretchTop,
                HorizontalAlignment = TextAlignment.Near,
                VerticalAlignment = TextAlignment.Near,
                Wrapping = TextWrapping.WrapWords,
            });
            m_EventDetails.SetAutoHeight(true);
            m_CpuSplit.Panel1.SizeChanged += (Control control) => LayoutCpuPanels();
            m_CpuSplit.Panel2.SizeChanged += (Control control) => LayoutCpuPanels();
            m_Details = m_Content.AddChild(new Label
            {
                HorizontalAlignment = TextAlignment.Near,
                VerticalAlignment = TextAlignment.Near,
                Wrapping = TextWrapping.WrapWords,
            });
            m_Details.SetAutoHeight(true);
            m_DataInfoToggle = m_Content.AddChild(new DataInfoButton());
            m_DataInfoToggle.Clicked += () =>
            {
                m_DataInfo.Visible = !m_DataInfo.Visible;
                m_DataInfoToggle.IsExpanded = m_DataInfo.Visible;
                PerformLayout();
            };
            m_DataInfo = m_Content.AddChild(new Label
            {
                HorizontalAlignment = TextAlignment.Near,
                VerticalAlignment = TextAlignment.Near,
                Wrapping = TextWrapping.WrapWords,
                Visible = false,
            });
            m_DataInfo.SetAutoHeight(true);
            Editor.Profiler.Changed += UpdateCapture;
            UpdateCapture();
        }

        private void UpdateCapture()
        {
            ProfilerModule profiler = Editor.Profiler;
            CaptureInfo info = profiler.Info;
            FrameViewInfo view = profiler.View;
            bool gpu = profiler.IsGpuVisible;
            bool memory = profiler.IsMemoryVisible;
            bool statistics = !gpu && !memory && profiler.IsCpuStatisticsVisible;
            if (m_StatisticFilter.Text != profiler.StatisticFilter)
                m_StatisticFilter.Text = profiler.StatisticFilter;
            m_CpuStatistics.Visible = statistics;
            m_CpuTimeline.Visible = !gpu && !memory && !statistics;
            m_GpuHierarchy.Visible = gpu;
            m_MemoryView.Visible = memory;
            m_Overview.Visible = !gpu && !memory;
            m_MemoryNavigation.Visible = memory;
            m_ThreadFilter.Visible = !gpu && !memory;
            m_MemoryPage.Enabled = !memory;
            m_GpuFrame.Visible = gpu;
            m_GpuPrevious.Visible = gpu;
            m_GpuNext.Visible = gpu;
            m_StatisticFilter.Visible = statistics;
            m_FitCpu.Visible = !gpu && !memory && !statistics;
            m_TimelinePage.Enabled = gpu || memory || statistics;
            m_StatisticsPage.Enabled = gpu || memory || !statistics;
            m_GpuPage.Enabled = !gpu;
            m_GpuFrame.Enabled = profiler.GpuFrames.Length > 0;
            m_GpuFrame.Text = profiler.GpuView.SnapshotId != 0 && profiler.GpuView.SelectedIndex >= 0 ? $"GPU engine frame: {profiler.GpuView.SelectedFrameId}" : "GPU engine frame: none";
            m_GpuPrevious.Enabled = profiler.GpuView.SelectedIndex > 0;
            m_GpuNext.Enabled = profiler.GpuView.SnapshotId != 0 && profiler.GpuView.SelectedIndex >= 0 && profiler.GpuView.SelectedIndex + 1 < profiler.GpuFrames.Length;
            string threadFilter = "All Threads";
            foreach (CpuThread thread in profiler.CpuThreads)
            {
                if (thread.Id == view.FilterThreadId)
                    threadFilter = $"{profiler.GetCpuText(thread.NameOffset)} [#{thread.Id}; OS ID unavailable]";
            }
            m_ThreadFilter.Text = $"Threads: {threadFilter}";
            m_ThreadFilter.Enabled = info.State == CaptureState.Capturing || info.State == CaptureState.Stopped;
            m_FitCpu.Enabled = view.SelectionEndNs > view.SelectionBeginNs;
            m_CpuStatus.Text = $"Snapshot: {view.SnapshotId}    CPU events: {profiler.CpuEvents.Length}    End unknown: {view.IncompleteCpuEvents}\n" +
                $"Statistics: {profiler.VisibleStatisticCount} / {profiler.CpuStatistics.Length} sources    " +
                $"Range: {view.SelectionBeginNs / 1_000_000.0:0.##} - {view.SelectionEndNs / 1_000_000.0:0.##} ms\n" +
                (statistics ? "Complete calls only; header: sort, row: inspect, double-click: locate. Not CPU utilization." :
                    "Ctrl+wheel: zoom    Middle-drag: pan    Click thread: fold    Incomplete: Duration unavailable");
            m_EventDetails.Text = profiler.CpuEventDetails;
            if (gpu)
            {
                string frameStatus = "No GPU frame has been admitted to this session.";
                int selected = profiler.GpuView.SelectedIndex;
                if (profiler.GpuView.SnapshotId != 0 && selected >= 0 && selected < profiler.GpuFrames.Length)
                {
                    GpuFrame frame = profiler.GpuFrames[selected];
                    frameStatus = $"GPU frame {frame.FrameId}: {frame.State}    Events: {frame.EventCount}";
                    if (frame.ResolvedAtFrame != 0)
                        frameStatus += $"    First resolved at frame {frame.ResolvedAtFrame} (+{frame.ResolvedAtFrame - frame.FrameId})";
                    frameStatus += frame.PresentAvailable != 0 ?
                        $"\nCPU Present/flush segment: {frame.PresentMs:0.##} ms (not GPU time)" : "\nCPU Present/flush segment unavailable.";
                }
                m_CpuStatus.Text = frameStatus + $"\nSnapshot: {profiler.GpuView.SnapshotId}    Background GPU pending: {info.GpuPending}. No inferred CPU/GPU frame mapping.";
                m_EventDetails.Text = profiler.GpuEventDetails;
            }
            ulong completeFrames = Math.Max(info.CompleteFrames, view.TotalFrames);
            bool readable = info.State == CaptureState.Capturing || info.State == CaptureState.Stopped;
            m_Start.Enabled = info.State == CaptureState.Idle;
            m_Stop.Enabled = info.State == CaptureState.Capturing;
            m_Stop.Text = "Stop";
            m_Clear.Enabled = info.State == CaptureState.Stopped || info.State == CaptureState.Failed;
            m_Pause.Enabled = profiler.IsLive && (memory ? profiler.MemoryOverview.SnapshotId != 0 :
                gpu ? profiler.GpuView.SnapshotId != 0 : view.SnapshotId != 0);
            m_Pause.Checked = !profiler.IsLive;
            m_Live.Enabled = readable;
            m_Live.Checked = profiler.IsLive;
            m_Refresh.Enabled = readable;
            m_Older.Enabled = !gpu && view.FrameCount > 0 && view.FirstFrame > profiler.OldestFrame;
            m_Newer.Enabled = !gpu && view.FrameCount > 0 && view.FirstFrame + (ulong)view.FrameCount < completeFrames;
            m_Previous.Enabled = !gpu && view.SelectionCount > 0 && view.SelectionFirst > profiler.OldestFrame;
            m_Next.Enabled = !gpu && view.SelectionCount > 0 && view.SelectionFirst + view.SelectionCount < completeFrames;
            if (memory)
            {
                ulong first = profiler.MemoryOverview.First;
                int count = profiler.MemoryOverview.Count;
                ulong total = profiler.MemoryOverview.OldestSample + (ulong)profiler.MemoryOverview.TotalSamples;
                m_MemoryFirst.Enabled = m_MemoryPrevious.Enabled = first > profiler.MemoryOverview.OldestSample;
                m_MemoryLast.Enabled = m_MemoryNext.Enabled = first + (ulong)count < total;
                m_EventDetails.Text = profiler.MemoryDetails;
                m_CpuStatus.Text = $"Overview snapshot {profiler.MemoryOverview.SnapshotId}; samples {first}-{first + (ulong)count} of {total}\n" +
                    "Engine elapsed seconds; managed receipt time is separate. Click a sample for exact values.\nSources overlap: do not sum process, system, GPU and managed bytes.";
            }

            string mode = profiler.IsLive ? "Live" : "Paused";
            string background = !profiler.IsLive && info.State == CaptureState.Capturing ? " (capture continues)" : string.Empty;
            m_Status.Text = $"Capture: {info.State}    View: {mode}{background}\n" +
                $"Session: {info.SessionId}    Published: {info.CompleteFrames}    Pending: {info.PendingRequests}\n" +
                $"Dropped requests: {info.DroppedRequests}    CPU ring loss: unavailable    Publish delay: {info.PublishDelayMs:0.##} ms";
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
                foreach (CaptureFrame frame in profiler.Frames)
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
            if (profiler.IsViewPending)
            {
                text += "View refresh pending; the previous complete snapshot remains displayed.\n";
            }
            text += $"Query page limit: {ProfilerModule.MaxViewFrames} complete frames. Native snapshot, temporary List, interop arrays and managed copies cost additional memory.\n";
            text += "CPU query limits: 65536 events, 256 threads, 128 levels, 1 MiB UTF-8 text (candidate limits; not performance measured).\n";
            text += "CPU events belong to the selected extraction batches and explicit thread filter. Folding changes display only.\n";
            text += "Native source statistics cover every CPU event in this successful snapshot, independently of display aggregation and name filtering.\n";
            text += "Total/average/max use complete calls with original durations; unknown ends are counted separately. Self subtracts direct-child coverage once; missing child ends make self incomplete.\n";
            text += "Cross-thread and nested inclusive totals can exceed elapsed time; they are not CPU utilization.\n";
            if (profiler.ViewResult == CaptureResult.QueryTooLarge)
            {
                text += "Query exceeds frame/CPU capacity; the previous snapshot is retained. Select fewer frames or filter a thread, then Refresh.\n";
            }
            else if (profiler.ViewResult == CaptureResult.InvalidRange)
            {
                text += "Query range is invalid; the previous snapshot is retained.\n";
            }
            else if (profiler.ViewResult == CaptureResult.InvalidSnapshot)
            {
                text += "The requested snapshot has expired. Refresh to establish a new snapshot.\n";
            }
            if (info.Error != CaptureResult.Ok || profiler.LastResult != CaptureResult.Ok || profiler.ViewResult != CaptureResult.Ok)
            {
                text += $"Capture error: {info.Error}    Last capture operation: {profiler.LastResult}    View read: {profiler.ViewResult}\n";
            }
            if (!string.IsNullOrEmpty(profiler.LoadError))
            {
                text += $"Native profiler could not be loaded: {profiler.LoadError}\n";
            }
            m_DataInfo.Text = gpu ? $"Capture: {info.State}    Stop reason: {info.StopReason}\nGPU history: {info.GpuFrames} frames, {info.GpuPending} pending\n" +
                $"History follows the latest {profiler.HistoryCapacity} engine samples. GPU frame IDs retain their real engine identity.\n" +
                "Pending and unavailable timings are not measured zero durations. Refresh a paused snapshot to observe resolution.\n" +
                "After Stop, admitted pending results may finish on later render ticks. Clear releases history.\n" +
                $"Capture error: {info.Error}    Last operation: {profiler.LastResult}    View read: {profiler.ViewResult}\n{profiler.LoadError}" : text;
            if (memory)
                m_DataInfo.Text = $"Memory query: {profiler.ViewResult}; capture: {info.State}\n" +
                    $"Overview retains {profiler.HistoryCapacity} engine samples. Paused samples remain available until refreshed or cleared.\n" +
                    "Process, system, GPU and CLR measurements overlap and are sampled independently. GC counters are process cumulative.\n" +
                    $"Last operation {profiler.LastResult}; capture error {info.Error}.\n{profiler.LoadError}";
            string stopReason = info.StopReason == StopReason.User ? "Stopped by user" : info.StopReason.ToString();
            string summary = $"Capture: {info.State}";
            if (info.StopReason != StopReason.None)
            {
                summary += $"    Stop reason: {stopReason}";
            }
            if (info.Error != CaptureResult.Ok)
            {
                summary += $"\nCapture error: {info.Error}";
            }
            if (profiler.LastResult != CaptureResult.Ok)
            {
                summary += $"\nLast operation: {profiler.LastResult}";
            }
            if (readable && profiler.ViewResult != CaptureResult.Ok)
            {
                summary += profiler.ViewResult switch
                {
                    CaptureResult.QueryTooLarge => "\nQuery exceeds capacity; the previous view is retained. Adjust the range or filter and retry.",
                    CaptureResult.InvalidRange => "\nInvalid query range; the previous view is retained.",
                    CaptureResult.InvalidSnapshot => "\nThe snapshot has expired. Refresh to read a new view.",
                    CaptureResult.Busy => "\nView refresh pending; the previous view is retained.",
                    _ => $"\nView read: {profiler.ViewResult}",
                };
            }
            if (info.TailMayBeIncomplete != 0)
            {
                summary += "\nRecording tail may be incomplete.";
            }
            if (!string.IsNullOrEmpty(profiler.LoadError))
            {
                summary += $"\nNative profiler could not be loaded: {profiler.LoadError}";
            }
            m_Details.Text = summary;
            PerformLayout();
            if (m_LastStatisticsPage != statistics || m_LastGpuPage != gpu || m_LastMemoryPage != memory)
            {
                m_LastMemoryPage = memory;
                m_LastStatisticsPage = statistics;
                m_LastGpuPage = gpu;
                m_CpuSplit.Panel1.SetViewOffset(Float2.Zero);
                int row = profiler.GetVisibleStatisticPosition(profiler.SelectedCpuStatistic);
                if (statistics && row >= 0)
                    m_CpuSplit.Panel1.ScrollViewTo(new Rectangle(0, (row + 1) * ProfilerCpuStatistics.RowHeight, m_CpuStatistics.Width, ProfilerCpuStatistics.RowHeight), true);
            }
            if (m_LastCpuFocusVersion != profiler.CpuFocusVersion)
            {
                m_LastCpuFocusVersion = profiler.CpuFocusVersion;
                m_CpuTimeline.ScrollToSelectedEvent();
            }
        }

        private void LayoutCpuPanels()
        {
            if (m_CpuStatistics == null || m_EventDetails == null)
                return;
            float width = MathF.Max(1, m_CpuSplit.Panel1.GetDesireClientArea().Width);
            m_CpuTimeline.SetBounds(0, 0, width, m_CpuTimeline.RequiredHeight);
            m_CpuStatistics.SetBounds(0, 0, MathF.Max(ProfilerCpuStatistics.MinimumWidth, width), m_CpuStatistics.RequiredHeight);
            m_GpuHierarchy.SetBounds(0, 0, MathF.Max(ProfilerGpuHierarchy.MinimumWidth, width), m_GpuHierarchy.RequiredHeight);
            m_MemoryView.SetBounds(0, 0, MathF.Max(m_MemoryView.RequiredWidth, width), m_MemoryView.RequiredHeight);
            m_EventDetails.SetBounds(8, 4, MathF.Max(1, m_CpuSplit.Panel2.GetDesireClientArea().Width - 16), m_EventDetails.Height);
        }

        private void ShowGpuFrames(int first)
        {
            ContextMenu menu = new ContextMenu();
            ReadOnlySpan<GpuFrame> frames = Editor.Profiler.GpuFrames;
            menu.AddButton($"GPU frames {first} - {Math.Min(first + 64, frames.Length)} of {frames.Length}").Enabled = false;
            if (first > 0)
                menu.AddButton("Previous page", () => ShowGpuFrames(Math.Max(0, first - 64)));
            if (first + 64 < frames.Length)
                menu.AddButton("Next page", () => ShowGpuFrames(first + 64));
            for (int i = first; i < Math.Min(first + 64, frames.Length); i++)
            {
                ulong id = frames[i].FrameId;
                menu.AddButton($"Frame {id} ({frames[i].State})", () => Editor.Profiler.SelectGpuFrame(id));
            }
            menu.Show(m_GpuFrame, new Float2(0, m_GpuFrame.Height));
        }

        private void ShowThreadFilter()
        {
            ShowThreadFilterPage(0);
        }

        private void ShowThreadFilterPage(int first)
        {
            ContextMenu menu = new ContextMenu();
            CaptureResult result = Editor.Profiler.LoadCpuThreads(first);
            if (result != CaptureResult.Ok)
            {
                menu.AddButton($"Thread directory: {result}").Enabled = false;
                menu.AddButton("Retry", () => ShowThreadFilterPage(first));
                menu.Show(m_ThreadFilter, new Float2(0, m_ThreadFilter.Height));
                return;
            }
            menu.AddButton("All Threads", () => Editor.Profiler.SelectCpuThread(0));
            menu.AddButton($"Threads {first} - {first + Editor.Profiler.ThreadOptions.Length} of {Editor.Profiler.TotalThreadOptions}").Enabled = false;
            if (first > 0)
                menu.AddButton("Previous thread page", () => ShowThreadFilterPage(Math.Max(0, first - 256)));
            if (first + Editor.Profiler.ThreadOptions.Length < Editor.Profiler.TotalThreadOptions)
                menu.AddButton("Next thread page", () => ShowThreadFilterPage(first + 256));
            foreach (CpuThread thread in Editor.Profiler.ThreadOptions)
            {
                ulong id = thread.Id;
                menu.AddButton($"{Editor.Profiler.GetThreadOptionText(thread.NameOffset)} [{id}]", () => Editor.Profiler.SelectCpuThread(id));
            }
            menu.Show(m_ThreadFilter, new Float2(0, m_ThreadFilter.Height));
        }

        protected override void OnShow()
        {
            base.OnShow();
            if (m_CpuTimeline != null)
                Editor.Profiler.OnWindowShown();
        }

        protected override void PerformLayoutBeforeChildren()
        {
            base.PerformLayoutBeforeChildren();
            if (m_Details == null || m_DataInfoToggle == null || m_DataInfo == null || m_Overview == null || m_Content == null)
            {
                return;
            }

            const float HorizontalPadding = 8;
            const float RowHeight = 24;
            const float RowSpacing = 4;
            const float SectionSpacing = 8;
            const float StatusHeight = 50;
            bool memory = Editor.Profiler.IsMemoryVisible;
            bool gpu = Editor.Profiler.IsGpuVisible;

            // Fixed capture toolbar and the scrolling content below it.
            m_ToolStrip.SetBounds(0, 0, Width, RowHeight);
            m_Content.SetBounds(0, m_ToolStrip.Bottom, Width, MathF.Max(0, Height - m_ToolStrip.Bottom));
            float width = MathF.Max(1, m_Content.Width - m_Content.GetScrollBarsSize());
            float contentWidth = MathF.Max(1, width - HorizontalPadding * 2);
            float y = RowSpacing;

            // Capture status, followed by the four page tabs.
            m_Status.SetBounds(HorizontalPadding, y, contentWidth, StatusHeight);
            y = m_Status.Bottom + RowSpacing;

            float tabWidth = MathF.Min(140, contentWidth / 4);
            m_TimelinePage.SetBounds(HorizontalPadding, y, tabWidth, RowHeight);
            m_StatisticsPage.SetBounds(HorizontalPadding + tabWidth, y, tabWidth, RowHeight);
            m_GpuPage.SetBounds(HorizontalPadding + tabWidth * 2, y, tabWidth, RowHeight);
            m_MemoryPage.SetBounds(HorizontalPadding + tabWidth * 3, y, tabWidth, RowHeight);
            y = m_MemoryPage.Bottom + RowSpacing;

            // The CPU frame overview includes its own navigation row.
            float chartHeight = gpu || memory ? 0 : Math.Clamp(m_Content.Height * 0.35f, 100, 180);
            m_Overview.SetBounds(HorizontalPadding, y, contentWidth, chartHeight);
            m_Navigation.SetBounds(0, 0, m_Overview.Width, RowHeight);
            float buttonWidth = MathF.Min(28, m_Navigation.Width / 4);
            m_Older.SetBounds(0, 2, buttonWidth, 20);
            m_Previous.SetBounds(buttonWidth, 2, buttonWidth, 20);
            m_Next.SetBounds(m_Navigation.Width - buttonWidth * 2, 2, buttonWidth, 20);
            m_Newer.SetBounds(m_Navigation.Width - buttonWidth, 2, buttonWidth, 20);
            if (m_Overview.Visible)
            {
                y = m_Overview.Bottom + SectionSpacing;
            }

            // Each page contributes its own rows above the shared status and content.
            if (memory)
            {
                m_MemoryNavigation.SetBounds(HorizontalPadding, y, contentWidth, RowHeight);
                float memoryButtonWidth = MathF.Min(28, contentWidth / 4);
                m_MemoryFirst.SetBounds(0, 2, memoryButtonWidth, 20);
                m_MemoryPrevious.SetBounds(memoryButtonWidth, 2, memoryButtonWidth, 20);
                m_MemoryNext.SetBounds(contentWidth - memoryButtonWidth * 2, 2, memoryButtonWidth, 20);
                m_MemoryLast.SetBounds(contentWidth - memoryButtonWidth, 2, memoryButtonWidth, 20);
                y = m_MemoryNavigation.Bottom + RowSpacing;
            }
            else if (gpu)
            {
                m_GpuFrame.SetBounds(HorizontalPadding, y, MathF.Max(1, contentWidth - 68), RowHeight);
                m_GpuPrevious.SetBounds(HorizontalPadding + MathF.Max(1, contentWidth - 64), y, 28, RowHeight);
                m_GpuNext.SetBounds(HorizontalPadding + MathF.Max(1, contentWidth - 32), y, 28, RowHeight);
                y = m_GpuFrame.Bottom + RowSpacing;
            }
            else
            {
                bool statistics = Editor.Profiler.IsCpuStatisticsVisible;
                float fitWidth = MathF.Min(76, contentWidth * 0.3f);
                float filterWidth = statistics ? contentWidth * 0.5f : fitWidth;
                m_ThreadFilter.SetBounds(HorizontalPadding, y, MathF.Max(1, contentWidth - filterWidth - RowSpacing), RowHeight);
                if (statistics)
                {
                    m_StatisticFilter.SetBounds(HorizontalPadding + contentWidth - filterWidth, y, filterWidth, RowHeight);
                }
                else
                {
                    m_FitCpu.SetBounds(HorizontalPadding + contentWidth - fitWidth, y, fitWidth, RowHeight);
                }
                y = m_ThreadFilter.Bottom + RowSpacing;
            }

            // Page status and the main view with its inspector.
            m_CpuStatus.SetBounds(HorizontalPadding, y, contentWidth, memory ? 66 : StatusHeight);
            y = m_CpuStatus.Bottom + RowSpacing;

            float viewHeight = Math.Clamp(m_Content.Height * 0.6f, 220, 420);
            m_CpuSplit.SetBounds(HorizontalPadding, y, contentWidth, viewHeight);
            LayoutCpuPanels();
            y = m_CpuSplit.Bottom + SectionSpacing;

            // Selection details and the optional data information at the bottom.
            m_Details.SetBounds(HorizontalPadding, y, contentWidth, m_Details.Height);
            y = m_Details.Bottom + RowSpacing;

            m_DataInfoToggle.SetBounds(HorizontalPadding, y, MathF.Min(160, contentWidth), RowHeight);
            y = m_DataInfoToggle.Bottom + RowSpacing;
            m_DataInfo.SetBounds(HorizontalPadding, y, contentWidth, m_DataInfo.Height);
        }

        public override void OnExit()
        {
            Editor.Profiler.Changed -= UpdateCapture;
            base.OnExit();
        }

        private sealed class DataInfoButton : Button
        {
            public bool IsExpanded { get; set; }

            public override void DrawSelf()
            {
                // Keep the standard button appearance and input, with separate arrow and label areas.
                base.DrawSelf();
                Style style = Style.Current;
                Color color = EnabledInHierarchy ? TextColor : TextColor * 0.6f;
                Rectangle arrowRect = new Rectangle(4, (Height - 12) * 0.5f, 12, 12);
                Rectangle textRect = new Rectangle(20, 0, MathF.Max(0, Width - 24), Height);
                Render2D.PushClip(new Rectangle(Float2.Zero, Size));
                Render2D.DrawSprite(IsExpanded ? style.ArrowDown : style.ArrowRight, arrowRect, color);
                Render2D.RenderText(Font, TextMaterial, "数据说明", textRect, color, TextAlignment.Center, TextAlignment.Center);
                Render2D.PopClip();
            }
        }

        private sealed class FrameOverview : ContainerControl
        {
            private readonly ProfilerModule m_Profiler;
            private bool m_Dragging;
            private int m_First;
            private int m_Last;
            private int m_Hover = -1;
            private ulong m_DragSnapshot;

            public FrameOverview(ProfilerModule profiler)
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
                    m_Profiler.Pause();
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
