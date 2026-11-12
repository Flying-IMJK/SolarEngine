using System;
using SE.Editor.GUI;
using SE.GUI;
using CaptureInfo = SE.ProfilerCaptureInfo;
using CaptureState = SE.ProfilerCaptureState;
using CaptureResult = SE.ProfilerCaptureResult;
using StopReason = SE.ProfilerStopReason;

namespace SE.Editor
{
    /// <summary>Common capture controls, mode navigation and the active profiler page.</summary>
    public sealed class ProfilerWindow : EditorWindow
    {
        private ToolStrip m_ToolStrip;
        private ToolStripButton m_Start;
        private ToolStripButton m_Stop;
        private ToolStripButton m_Clear;
        private ToolStripButton m_Pause;
        private ToolStripButton m_Live;
        private ToolStripButton m_Refresh;
        private Panel m_ModeList;
        private ContainerControl m_ModeHost;
        private Button[] m_ModeButtons;
        private ProfilerMode m_DisplayedMode;
        private Panel m_ModeControl;
        private Label m_Status;
        private Label m_Details;

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
            m_Status = AddChild(new Label { HorizontalAlignment = TextAlignment.Near, VerticalAlignment = TextAlignment.Near });
            m_ModeList = AddChild(new Panel(ScrollBars.Vertical) { IsScrollable = false });
            m_ModeHost = AddChild(new ContainerControl { IsScrollable = false });
            m_ModeButtons = new Button[Editor.Profiler.Modes.Length];
            for (int i = 0; i < m_ModeButtons.Length; i++)
            {
                ProfilerMode mode = Editor.Profiler.Modes[i];
                Button button = m_ModeList.AddChild(new Button { Text = mode.Title });
                button.Clicked += () => Editor.Profiler.SelectMode(mode);
                m_ModeButtons[i] = button;
            }
            m_Details = AddChild(new Label
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
            ProfilerMode active = profiler.ActiveMode;
            if (m_DisplayedMode != active)
            {
                // The module owns page controls; the window only attaches the displayed page.
                m_DisplayedMode?.DetachControl();
                m_DisplayedMode = active;
                m_ModeControl = active.GetControl();
                m_ModeControl.Parent = m_ModeHost;
            }
            for (int i = 0; i < m_ModeButtons.Length; i++)
            {
                m_ModeButtons[i].Enabled = profiler.Modes[i] != active;
            }
            bool readable = info.State == CaptureState.Capturing || info.State == CaptureState.Stopped;
            m_Start.Enabled = info.State == CaptureState.Idle;
            m_Stop.Enabled = info.State == CaptureState.Capturing;
            m_Clear.Enabled = info.State == CaptureState.Stopped || info.State == CaptureState.Failed;
            m_Pause.Enabled = profiler.IsLive && active.HasSnapshot;
            m_Pause.Checked = !profiler.IsLive;
            m_Live.Enabled = readable;
            m_Live.Checked = profiler.IsLive;
            m_Refresh.Enabled = readable;
            string mode = profiler.IsLive ? "Live" : "Paused";
            string background = !profiler.IsLive && info.State == CaptureState.Capturing ? " (capture continues)" : string.Empty;
            m_Status.Text = $"Capture: {info.State}    View: {mode}{background}\n" +
                $"Session: {info.SessionId}    Published: {info.CompleteFrames}    Pending: {info.PendingRequests}\n" +
                $"Dropped requests: {info.DroppedRequests}    CPU ring loss: unavailable    Publish delay: {info.PublishDelayMs:0.##} ms";
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
            if (readable && active.ViewResult != CaptureResult.Ok)
            {
                summary += active.ViewResult switch
                {
                    CaptureResult.QueryTooLarge => "\nQuery exceeds capacity; the previous view is retained. Adjust the range or filter and retry.",
                    CaptureResult.InvalidRange => "\nInvalid query range; the previous view is retained.",
                    CaptureResult.InvalidSnapshot => "\nThe snapshot has expired. Refresh to read a new view.",
                    CaptureResult.Busy => "\nView refresh pending; the previous view is retained.",
                    _ => $"\nView read: {active.ViewResult}",
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
        }

        protected override void OnShow()
        {
            base.OnShow();
            if (m_ModeHost != null)
            {
                Editor.Profiler.OnWindowShown();
            }
        }

        protected override void PerformLayoutBeforeChildren()
        {
            base.PerformLayoutBeforeChildren();
            if (m_Details == null || m_ModeButtons == null)
            {
                return;
            }
            m_ToolStrip.SetBounds(0, 0, Width, 24);
            m_Status.SetBounds(8, 28, MathF.Max(1, Width - 16), 54);
            float footerHeight = MathF.Max(24, m_Details.Height);
            m_Details.SetBounds(8, MathF.Max(m_Status.Bottom + 4, Height - footerHeight - 4), MathF.Max(1, Width - 16), footerHeight);
            float y = m_Status.Bottom + 4;
            float height = MathF.Max(0, m_Details.Top - y - 4);
            float listWidth = MathF.Min(120, MathF.Max(1, Width * 0.25f));
            m_ModeList.SetBounds(0, y, listWidth, height);
            for (int i = 0; i < m_ModeButtons.Length; i++)
            {
                m_ModeButtons[i].SetBounds(4, 4 + i * 36, MathF.Max(1, listWidth - m_ModeList.GetScrollBarsSize() - 8), 32);
            }
            m_ModeHost.SetBounds(listWidth + 4, y, MathF.Max(0, Width - listWidth - 4), height);
            m_ModeControl?.SetBounds(0, 0, m_ModeHost.Width, m_ModeHost.Height);
        }

        public override void OnExit()
        {
            Editor.Profiler.Changed -= UpdateCapture;
            m_DisplayedMode?.DetachControl();
            base.OnExit();
        }
    }
}
