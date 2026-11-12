using System;
using System.Collections.Generic;
using System.Text;
using SE.GUI;
using CaptureInfo = SE.ProfilerCaptureInfo;
using CaptureResult = SE.ProfilerCaptureResult;
using CaptureState = SE.ProfilerCaptureState;

namespace SE.Editor
{
    /// <summary>
    /// Owns one profiler page and its managed snapshot on the editor main thread.
    /// The module retains the page across window lifetimes; native snapshot reads still validate their identity.
    /// </summary>
    public abstract class ProfilerMode
    {
        internal readonly ProfilerModule Profiler;
        protected Panel m_Content;
        protected SplitPanel m_Split;
        protected Label m_Status;
        protected Label m_EventDetails;
        private Label m_DataInfo;
        private DataInfoButton m_DataInfoToggle;

        protected ProfilerMode(ProfilerModule profiler)
        {
            Profiler = profiler;
        }

        public abstract string Title { get; }
        public abstract bool HasSnapshot { get; }
        protected virtual ScrollBars ContentScrollBars => ScrollBars.Vertical;
        public CaptureResult ViewResult { get; internal set; }
        internal bool Dirty { get; set; } = true;
        internal virtual bool IsFinal => Profiler.Info.State == CaptureState.Stopped;

        internal abstract CaptureResult Query(out CaptureInfo info);
        internal abstract void Freeze();
        internal abstract void ShowLatest();
        internal abstract void Reset();
        protected abstract void CreateControls();
        protected abstract void UpdateControls();
        protected abstract float LayoutControls(float width);
        protected abstract void LayoutContent();
        protected abstract string GetDataInfo();

        internal Panel GetControl()
        {
            if (m_Content != null)
            {
                return m_Content;
            }
            // GUI allocation is deferred until a window displays this page.
            m_Content = new Panel(ContentScrollBars) { IsScrollable = false };
            CreateControls();
            m_Status = m_Content.AddChild(new Label { HorizontalAlignment = TextAlignment.Near, VerticalAlignment = TextAlignment.Near });
            m_Status.SetAutoHeight(true);
            m_Split = m_Content.AddChild(new SplitPanel(Orientation.Horizontal, ScrollBars.Both, ScrollBars.Vertical) { SplitterValue = 0.7f });
            m_EventDetails = m_Split.Panel2.AddChild(new Label
            {
                AnchorPreset = AnchorPresets.HorizontalStretchTop,
                HorizontalAlignment = TextAlignment.Near,
                VerticalAlignment = TextAlignment.Near,
                Wrapping = TextWrapping.WrapWords,
                IsScrollable = true,
            });
            m_EventDetails.SetAutoHeight(true);
            m_Split.Panel1.SizeChanged += (Control control) => LayoutPanels();
            m_Split.Panel2.SizeChanged += (Control control) => LayoutPanels();
            m_DataInfoToggle = m_Content.AddChild(new DataInfoButton());
            Panel dataInfoParent = m_Content.GetScrollBars() == ScrollBars.None ? m_Split.Panel2 : m_Content;
            m_DataInfo = dataInfoParent.AddChild(new Label
            {
                HorizontalAlignment = TextAlignment.Near,
                VerticalAlignment = TextAlignment.Near,
                Wrapping = TextWrapping.WrapWords,
                Visible = false,
            });
            m_DataInfo.SetAutoHeight(true);
            m_DataInfoToggle.Clicked += () =>
            {
                m_DataInfo.Visible = !m_DataInfo.Visible;
                m_DataInfoToggle.IsExpanded = m_DataInfo.Visible;
                UpdateView();
            };
            m_Content.SizeChanged += (Control control) => LayoutView();
            UpdateView();
            return m_Content;
        }

        internal void UpdateView()
        {
            if (m_DataInfo == null)
            {
                return;
            }
            UpdateControls();
            if (m_DataInfo.Visible)
            {
                m_DataInfo.Text = GetDataInfo();
            }
            LayoutView();
        }

        private void LayoutView()
        {
            if (m_DataInfo == null)
            {
                return;
            }
            bool scrollPage = m_Content.GetScrollBars() != ScrollBars.None;
            float width = MathF.Max(1, m_Content.Width - (scrollPage ? m_Content.GetScrollBarsSize() : 0) - 16);
            float y = LayoutControls(width);
            m_Status.SetBounds(8, y, width, MathF.Max(50, m_Status.Height));
            y = m_Status.Bottom + 4;
            // Fixed pages give the remaining viewport to the independently scrolling analysis panels.
            float splitHeight = scrollPage ? Math.Clamp(m_Content.Height * 0.6f, 220, 420) : MathF.Max(0, m_Content.Height - y - 40);
            m_Split.SetBounds(8, y, width, splitHeight);
            LayoutPanels();
            y = m_Split.Bottom + 8;
            m_DataInfoToggle.SetBounds(8, y, MathF.Min(160, width), 24);
            if (scrollPage)
            {
                m_DataInfo.SetBounds(8, m_DataInfoToggle.Bottom + 4, width, m_DataInfo.Height);
            }
            m_Content.PerformLayout();
        }

        private void LayoutPanels()
        {
            if (m_EventDetails == null)
            {
                return;
            }
            LayoutContent();
            m_EventDetails.SetBounds(8, 4, MathF.Max(1, m_Split.Panel2.GetDesireClientArea().Width - 16), m_EventDetails.Height);
            if (m_DataInfo != null && m_DataInfo.Parent == m_Split.Panel2)
            {
                // Keep expanded information reachable without making the chart's parent scroll again.
                m_DataInfo.SetBounds(8, m_EventDetails.Bottom + 8,
                    MathF.Max(1, m_Split.Panel2.GetDesireClientArea().Width - 16), m_DataInfo.Height);
            }
        }

        internal void DetachControl()
        {
            if (m_Content != null)
            {
                m_Content.Parent = null;
            }
        }

        internal void DisposeControl()
        {
            m_Content?.Dispose();
        }

        protected static Dictionary<int, string> DecodeText(byte[] text)
        {
            Dictionary<int, string> strings = new Dictionary<int, string>();
            text ??= Array.Empty<byte>();
            for (int start = 0; start < text.Length;)
            {
                int end = Array.IndexOf(text, (byte)0, start);
                if (end < 0)
                {
                    throw new InvalidOperationException("Profiler text result is not terminated.");
                }
                strings.Add(start, Encoding.UTF8.GetString(text, start, end - start));
                start = end + 1;
            }
            return strings;
        }

        private sealed class DataInfoButton : Button
        {
            public bool IsExpanded { get; set; }

            public override void DrawSelf()
            {
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
    }
}
