using System;
using System.Collections.Generic;
using SE.Editor.GUI;
using SE.GUI;
using CaptureInfo = SE.ProfilerCaptureInfo;
using CaptureState = SE.ProfilerCaptureState;
using CaptureResult = SE.ProfilerCaptureResult;
using GpuFrame = SE.ProfilerGpuFrame;
using GpuEvent = SE.ProfilerGpuEvent;
using GpuViewInfo = SE.ProfilerGpuViewInfo;

namespace SE.Editor
{
    /// <summary>GPU frame snapshots, event hierarchy and drawing statistics.</summary>
    public sealed class ProfilerGpuMode : ProfilerMode
    {
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
        public GpuViewInfo GpuView { get; private set; }
        public ReadOnlySpan<GpuFrame> GpuFrames => m_GpuFrames;
        public ReadOnlySpan<GpuEvent> GpuEvents => m_GpuEvents;
        public int SelectedGpuEvent { get; private set; } = -1;
        public string GpuEventDetails { get; private set; } = "Select a GPU event to inspect it.";
        public CaptureResult SelectGpuFrame(ulong frameId)
        {
            if (Profiler.ActiveMode != this || (Profiler.Info.State != CaptureState.Capturing && Profiler.Info.State != CaptureState.Stopped))
                return CaptureResult.NotReady;
            Profiler.Pause();
            m_GpuFrameId = frameId;
            Dirty = true;
            Profiler.RefreshView();
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
            Profiler.NotifyChanged();
        }

        public CaptureResult SelectGpuEvent(ulong snapshotId, int index)
        {
            if (snapshotId == 0 || snapshotId != GpuView.SnapshotId)
                return CaptureResult.InvalidSnapshot;
            if (index < 0 || index >= m_GpuEvents.Length)
                return CaptureResult.InvalidRange;
            Profiler.Pause();
            SelectedGpuEvent = index;
            GpuEventDetails = DescribeGpuEvent(index);
            Profiler.NotifyChanged();
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

        public ProfilerGpuMode(ProfilerModule profiler) : base(profiler)
        {
        }

        public override string Title => "GPU";
        public override bool HasSnapshot => GpuView.SnapshotId != 0;
        internal override bool IsFinal => base.IsFinal && Profiler.Info.GpuPending == 0;

        internal override void Freeze()
        {
            if (HasSnapshot)
            {
                m_GpuFrameId = GpuView.SelectedFrameId;
                Dirty = false;
            }
        }

        internal override void ShowLatest()
        {
            m_GpuFrameId = ulong.MaxValue;
            Dirty = true;
        }

        internal override void Reset()
        {
            GpuView = default;
            m_GpuFrames = Array.Empty<GpuFrame>();
            m_GpuEvents = Array.Empty<GpuEvent>();
            m_GpuText.Clear();
            m_VisibleGpuEvents.Clear();
            GpuLayoutVersion++;
            SelectedGpuEvent = -1;
            GpuEventDetails = "Select a GPU event to inspect it.";
            m_CollapsedGpu.Clear();
            m_GpuFrameId = ulong.MaxValue;
            Dirty = true;
            ViewResult = CaptureResult.Ok;
        }

        internal override CaptureResult Query(out CaptureInfo info)
        {
            CaptureResult result = Profiling.RefreshGpuView(Profiler.Info.SessionId, Profiler.IsLive ? ulong.MaxValue : m_GpuFrameId,
                out info, out GpuViewInfo view, out GpuFrame[] frames, out GpuEvent[] events, out byte[] text);
            if (result != CaptureResult.Ok)
            {
                return result;
            }
            Dictionary<int, string> strings = DecodeText(text);
            bool sameFrame = HasSnapshot && GpuView.SelectedFrameId == view.SelectedFrameId;
            int selected = sameFrame ? SelectedGpuEvent : -1;
            GpuView = view;
            m_GpuFrames = frames ?? Array.Empty<GpuFrame>();
            m_GpuEvents = events ?? Array.Empty<GpuEvent>();
            m_GpuText = strings;
            if (!sameFrame)
            {
                m_CollapsedGpu.Clear();
            }
            RebuildGpuRows();
            SelectedGpuEvent = selected >= 0 && selected < m_GpuEvents.Length ? selected : -1;
            GpuEventDetails = SelectedGpuEvent >= 0 ? DescribeGpuEvent(SelectedGpuEvent) : "Select a GPU event to inspect it.";
            if (!Profiler.IsLive || (info.State == CaptureState.Stopped && info.GpuPending == 0))
            {
                Freeze();
            }
            return result;
        }

        private Button m_GpuFrame;
        private Button m_GpuPrevious;
        private Button m_GpuNext;
        private ProfilerGpuHierarchy m_Hierarchy;

        protected override void CreateControls()
        {
            m_GpuFrame = m_Content.AddChild(new Button { Text = "GPU engine frame" });
            m_GpuFrame.Clicked += () => ShowGpuFrames(0);
            m_GpuPrevious = m_Content.AddChild(new Button { Text = "<", TooltipText = "Previous GPU engine frame" });
            m_GpuPrevious.Clicked += () => BrowseGpuFrame(false);
            m_GpuNext = m_Content.AddChild(new Button { Text = ">", TooltipText = "Next GPU engine frame" });
            m_GpuNext.Clicked += () => BrowseGpuFrame(true);
        }

        protected override void UpdateControls()
        {
            if (m_Hierarchy == null)
            {
                m_Hierarchy = m_Split.Panel1.AddChild(new ProfilerGpuHierarchy(this));
            }
            m_GpuFrame.Enabled = GpuFrames.Length > 0;
            m_GpuFrame.Text = HasSnapshot && GpuView.SelectedIndex >= 0 ? $"GPU engine frame: {GpuView.SelectedFrameId}" : "GPU engine frame: none";
            m_GpuPrevious.Enabled = GpuView.SelectedIndex > 0;
            m_GpuNext.Enabled = HasSnapshot && GpuView.SelectedIndex >= 0 && GpuView.SelectedIndex + 1 < GpuFrames.Length;
            string frameStatus = "No GPU frame has been admitted to this session.";
            int selected = GpuView.SelectedIndex;
            if (HasSnapshot && selected >= 0 && selected < GpuFrames.Length)
            {
                GpuFrame frame = GpuFrames[selected];
                frameStatus = $"GPU frame {frame.FrameId}: {frame.State}    Events: {frame.EventCount}";
                if (frame.ResolvedAtFrame != 0)
                {
                    frameStatus += $"    First resolved at frame {frame.ResolvedAtFrame} (+{frame.ResolvedAtFrame - frame.FrameId})";
                }
                frameStatus += frame.PresentAvailable != 0 ? $"\nCPU Present/flush segment: {frame.PresentMs:0.##} ms (not GPU time)" :
                    "\nCPU Present/flush segment unavailable.";
            }
            m_Status.Text = frameStatus + $"\nSnapshot: {GpuView.SnapshotId}    Background GPU pending: {Profiler.Info.GpuPending}. No inferred CPU/GPU frame mapping.";
            m_EventDetails.Text = GpuEventDetails;
        }

        protected override float LayoutControls(float width)
        {
            m_GpuFrame.SetBounds(8, 4, MathF.Max(1, width - 68), 24);
            m_GpuPrevious.SetBounds(8 + MathF.Max(1, width - 64), 4, 28, 24);
            m_GpuNext.SetBounds(8 + MathF.Max(1, width - 32), 4, 28, 24);
            return 32;
        }

        protected override void LayoutContent()
        {
            if (m_Hierarchy == null || m_EventDetails == null)
            {
                return;
            }
            float width = MathF.Max(ProfilerGpuHierarchy.MinimumWidth, m_Split.Panel1.GetDesireClientArea().Width);
            m_Hierarchy.SetBounds(0, 0, width, m_Hierarchy.RequiredHeight);
        }

        protected override string GetDataInfo()
        {
            CaptureInfo info = Profiler.Info;
            return $"Capture: {info.State}    Stop reason: {info.StopReason}\nGPU history: {info.GpuFrames} frames, {info.GpuPending} pending\n" +
                $"History follows the latest {Profiler.HistoryCapacity} engine samples. GPU frame IDs retain their real engine identity.\n" +
                "Pending and unavailable timings are not measured zero durations. Refresh a paused snapshot to observe resolution.\n" +
                "After Stop, admitted pending results may finish on later render ticks. Clear releases history.\n" +
                $"Capture error: {info.Error}    Last operation: {Profiler.LastResult}    View read: {ViewResult}\n{Profiler.LoadError}";
        }

        private void ShowGpuFrames(int first)
        {
            ContextMenu menu = new ContextMenu();
            ReadOnlySpan<GpuFrame> frames = GpuFrames;
            menu.AddButton($"GPU frames {first} - {Math.Min(first + 64, frames.Length)} of {frames.Length}").Enabled = false;
            if (first > 0)
                menu.AddButton("Previous page", () => ShowGpuFrames(Math.Max(0, first - 64)));
            if (first + 64 < frames.Length)
                menu.AddButton("Next page", () => ShowGpuFrames(first + 64));
            for (int i = first; i < Math.Min(first + 64, frames.Length); i++)
            {
                ulong id = frames[i].FrameId;
                menu.AddButton($"Frame {id} ({frames[i].State})", () => SelectGpuFrame(id));
            }
            menu.Show(m_GpuFrame, new Float2(0, m_GpuFrame.Height));
        }
    }
}
