using System;
using SE.Editor.GUI;
using SE.GUI;
using CaptureInfo = SE.ProfilerCaptureInfo;
using CaptureState = SE.ProfilerCaptureState;
using CaptureResult = SE.ProfilerCaptureResult;
using CaptureFrame = SE.ProfilerCaptureFrame;
using FrameViewInfo = SE.ProfilerFrameViewInfo;
using StopReason = SE.ProfilerStopReason;

namespace SE.Editor
{
    /// <summary>Capture controls and a bounded, selectable view of complete recorded frames.</summary>
    public sealed class ProfilerWindow : EditorWindow
    {
        private readonly ToolStrip m_ToolStrip;
        private readonly ContainerControl m_Navigation;
        private readonly Panel m_Content;
        private readonly ToolStripButton m_Start;
        private readonly ToolStripButton m_Stop;
        private readonly ToolStripButton m_Clear;
        private readonly ToolStripButton m_Pause;
        private readonly ToolStripButton m_Live;
        private readonly ToolStripButton m_Refresh;
        private readonly Button m_Older;
        private readonly Button m_Newer;
        private readonly Button m_Previous;
        private readonly Button m_Next;
        private readonly Label m_Status;
        private readonly Label m_Details;
        private readonly FrameOverview m_Overview;

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
            m_Navigation = m_Content.AddChild(new ContainerControl
            {
                BackgroundColor = Style.Current.LightBackground,
            });
            m_Older = m_Navigation.AddChild(new Button { Text = "<<", TooltipText = "PreviousPage" });
            m_Older.Clicked += () => Editor.Profiler.BrowseFrames(false);
            m_Previous = m_Navigation.AddChild(new Button { Text = "<", TooltipText = "Previous" });
            m_Previous.Clicked += () =>
            {
                FrameViewInfo view = Editor.Profiler.View;
                if (view.SelectionCount > 0 && view.SelectionFirst > 0)
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
            m_Status = m_Content.AddChild(new Label
            {
                HorizontalAlignment = TextAlignment.Near,
                VerticalAlignment = TextAlignment.Near,
            });
            m_Overview = m_Content.AddChild(new FrameOverview(Editor.Profiler));
            m_Details = m_Content.AddChild(new Label
            {
                HorizontalAlignment = TextAlignment.Near,
                VerticalAlignment = TextAlignment.Near,
                Wrapping = TextWrapping.WrapWords,
            });
            m_Details.SetAutoHeight(true);
            Editor.Profiler.Changed += UpdateCapture;
            UpdateCapture();
        }

        private void UpdateCapture()
        {
            ProfilerModule profiler = Editor.Profiler;
            CaptureInfo info = profiler.Info;
            FrameViewInfo view = profiler.View;
            ulong completeFrames = Math.Max(info.CompleteFrames, view.TotalFrames);
            bool readable = info.State == CaptureState.Capturing || info.State == CaptureState.Stopping || info.State == CaptureState.Stopped;
            m_Start.Enabled = info.State == CaptureState.Idle;
            m_Stop.Enabled = info.State == CaptureState.Connecting || info.State == CaptureState.Capturing;
            m_Stop.Text = info.State == CaptureState.Connecting ? "Cancel" : "Stop";
            m_Clear.Enabled = info.State == CaptureState.Stopped || info.State == CaptureState.Failed;
            m_Pause.Enabled = profiler.IsLive && view.SnapshotId != 0;
            m_Pause.Checked = !profiler.IsLive;
            m_Live.Enabled = readable;
            m_Live.Checked = profiler.IsLive;
            m_Refresh.Enabled = readable;
            m_Older.Enabled = view.FrameCount > 0 && view.FirstFrame > 0;
            m_Newer.Enabled = view.FrameCount > 0 && view.FirstFrame + (ulong)view.FrameCount < completeFrames;
            m_Previous.Enabled = view.SelectionCount > 0 && view.SelectionFirst > 0;
            m_Next.Enabled = view.SelectionCount > 0 && view.SelectionFirst + view.SelectionCount < completeFrames;

            string mode = profiler.IsLive ? "Live" : "Paused";
            string background = !profiler.IsLive && info.State == CaptureState.Capturing ? " (capture continues)" : string.Empty;
            m_Status.Text = $"Capture: {info.State}    View: {mode}{background}\n" +
                $"Session: {info.SessionId}    Process: {info.ProcessId}\nComplete frames (latest state sample): {info.CompleteFrames}";
            string text = $"Snapshot: {view.SnapshotId}    Complete frames at snapshot: {view.TotalFrames}\n";
            if (info.StopReason != StopReason.None)
            {
                text += $"Stop reason: {info.StopReason}\n";
                if (info.StopReason == StopReason.ReceiverEnded)
                {
                    text += "The receiver ended; the cause has not been confirmed.\n";
                }
            }
            if (info.State == CaptureState.Stopping)
            {
                text += "Stop requested; receiving has not yet been confirmed stopped. Cutoff may still change.\n";
            }
            if (info.MemoryLimitBytes > 0)
            {
                text += $"Tracy Server counted memory (global sample): {info.ServerMemoryBytes / 1048576.0:F2} MiB\n";
                text += $"Capture soft threshold: {info.MemoryLimitBytes / 1048576.0:F0} MiB (candidate; not performance measured)\n";
                if (info.StopReason == StopReason.CapacityLimit)
                {
                    text += $"Capacity stop requested at: {info.CapacityStopBytes / 1048576.0:F2} MiB ({info.CapacityStopBytes} bytes)\n";
                }
                text += "Checked on system updates; allocation and receiver shutdown may overshoot the threshold.\n";
                text += "This counter is not process memory or a hard process limit.\n";
            }
            if (view.FrameCount > 0)
            {
                text += $"Visible frames: {view.FirstFrame} - {view.FirstFrame + (ulong)view.FrameCount - 1} ({view.FrameCount} of {view.TotalFrames})\n";
                text += $"View time: {view.BeginNs / 1_000_000.0:F3} - {view.EndNs / 1_000_000.0:F3} ms\n";
                text += $"Selected frames: {view.SelectionFirst} - {view.SelectionFirst + view.SelectionCount - 1}\n";
                text += $"Selection time: {view.SelectionBeginNs / 1_000_000.0:F3} - {view.SelectionEndNs / 1_000_000.0:F3} ms\n";
                long duration = view.SelectionEndNs - view.SelectionBeginNs;
                text += $"Selection duration: {duration / 1_000_000.0:F3} ms\n";
                text += duration > 0 ? $"Average frame: {duration / 1_000_000.0 / view.SelectionCount:F3} ms    FPS: {view.SelectionCount * 1_000_000_000.0 / duration:F2}\n" :
                    "FPS unavailable for a zero-duration interval.\n";
                text += "Frame timing covers the selected complete intervals; this page is not a whole-session event statistic.\n";
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
                text += $"Recording cutoff (Tracy time): {info.LastTimeNs / 1_000_000.0:F3} ms\nNative history remains available until Clear.\n";
                if (info.CompleteFrames > 0)
                {
                    text += $"Retained complete frames: 0 - {info.CompleteFrames - 1}\n";
                    text += $"Complete frame coverage: {info.CompleteBeginNs / 1_000_000.0:F3} - {info.CompleteEndNs / 1_000_000.0:F3} ms\n";
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
            if (profiler.ViewResult == CaptureResult.QueryTooLarge)
            {
                text += "Query exceeds the page capacity; the previous snapshot is retained. Select a smaller range.\n";
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
            m_Details.Text = text;
        }

        protected override void PerformLayoutBeforeChildren()
        {
            base.PerformLayoutBeforeChildren();
            if (m_Details == null || m_Overview == null || m_Content == null)
            {
                return;
            }
            m_ToolStrip.SetBounds(0, 0, Width, 24);
            m_Content.SetBounds(0, 24, Width, MathF.Max(0, Height - 24));
            float width = MathF.Max(1, m_Content.Width - m_Content.GetScrollBarsSize());
            float chartHeight = Math.Clamp(m_Content.Height * 0.35f, 100, 180);
            m_Status.SetBounds(8, 4, MathF.Max(1, width - 16), 50);
            m_Navigation.SetBounds(8, 58, MathF.Max(1, width - 16), 24);
            float buttonWidth = MathF.Min(28, m_Navigation.Width / 4);
            m_Older.SetBounds(0, 2, buttonWidth, 20);
            m_Previous.SetBounds(buttonWidth, 2, buttonWidth, 20);
            m_Next.SetBounds(m_Navigation.Width - buttonWidth * 2, 2, buttonWidth, 20);
            m_Newer.SetBounds(m_Navigation.Width - buttonWidth, 2, buttonWidth, 20);
            m_Overview.SetBounds(8, 86, MathF.Max(1, width - 16), chartHeight);
            m_Details.SetBounds(8, 94 + chartHeight, MathF.Max(1, width - 16), m_Details.Height);
        }

        public override void OnExit()
        {
            Editor.Profiler.Changed -= UpdateCapture;
            base.OnExit();
        }

        private sealed class FrameOverview : Control
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

            private Rectangle Plot => new Rectangle(4, 20, MathF.Max(1, Width - 8), MathF.Max(1, Height - 40));

            public override void Draw()
            {
                base.Draw();
                Style style = Style.Current;
                Rectangle plot = Plot;
                Render2D.FillRectangle(new Rectangle(Float2.Zero, Size), style.LightBackground);
                ReadOnlySpan<CaptureFrame> frames = m_Profiler.Frames;
                FrameViewInfo view = m_Profiler.View;
                if (frames.Length == 0)
                {
                    Render2D.RenderText(style.FontMedium, "No complete frame in this view", new Rectangle(Float2.Zero, Size), style.TextColor);
                    return;
                }
                double maximum = 0.001;
                for (int i = 0; i < frames.Length; i++)
                {
                    maximum = Math.Max(maximum, (frames[i].EndNs - frames[i].BeginNs) / 1_000_000.0);
                }
                Render2D.RenderText(style.FontMedium, $"{maximum:F3} ms", new Rectangle(4, 0, Width - 8, 18), style.TextColor);
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

            public override void OnMouseMove(Float2 location)
            {
                int count = m_Profiler.View.FrameCount;
                m_Hover = count > 0 && Plot.Contains(location) ? Math.Clamp((int)((location.X - Plot.X) / Plot.Width * count), 0, count - 1) : -1;
                if (m_Dragging && count > 0)
                {
                    m_Last = Math.Clamp((int)((location.X - Plot.X) / Plot.Width * count), 0, count - 1);
                }
                base.OnMouseMove(location);
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
                    CaptureFrame frame = frames[m_Hover];
                    long duration = frame.EndNs - frame.BeginNs;
                    text = $"Frame {frame.Index}\nBegin: {frame.BeginNs / 1_000_000.0:F3} ms\nEnd: {frame.EndNs / 1_000_000.0:F3} ms\nDuration: {duration / 1_000_000.0:F3} ms\n";
                    text += duration > 0 ? $"FPS: {1_000_000_000.0 / duration:F2}" : "FPS unavailable";
                }
                return show;
            }
        }
    }
}
