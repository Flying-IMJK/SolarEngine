using System;
using System.Collections.Generic;
using SE.GUI;
using CpuThread = SE.ProfilerCpuThread;
using CpuEvent = SE.ProfilerCpuEvent;

namespace SE.Editor
{
    /// <summary>
    /// Pixel layout, aggregation and input for the module's frozen CPU values.
    /// </summary>
    internal sealed class ProfilerCpuTimeline : Control
    {
        private const float RowHeight = 22;
        private readonly ProfilerModule m_Profiler;
        private readonly List<Bar> m_Bars = new List<Bar>();
        private readonly List<Header> m_Headers = new List<Header>();
        private ulong m_Snapshot;
        private long m_Begin;
        private long m_End;
        private int m_LayoutVersion = -1;
        private float m_Width;
        private int m_Hover = -1;
        private bool m_Panning;
        private ulong m_PanSnapshot;
        private float m_PanX;
        private long m_PanBegin;
        private long m_PanEnd;

        private struct Header
        {
            public Rectangle Bounds;
            public int ThreadIndex;
        }

        private struct Bar
        {
            public Rectangle Bounds;
            public int EventIndex;
            public int Count;
            public long BeginNs;
            public long EndNs;
            public bool Incomplete;
        }

        public ProfilerCpuTimeline(ProfilerModule profiler)
        {
            AnchorPreset = AnchorPresets.HorizontalStretchTop;
            m_Profiler = profiler;
            TooltipText = "Click to inspect. Double-click to focus. Ctrl+wheel to zoom. Middle-drag to pan. Scroll vertically.";
        }

        public float RequiredHeight
        {
            get
            {
                float height = RowHeight;
                foreach (CpuThread thread in m_Profiler.CpuThreads)
                {
                    if (m_Profiler.View.FilterThreadId != 0 && thread.Id != m_Profiler.View.FilterThreadId)
                        continue;
                    height += RowHeight;
                    if (!m_Profiler.IsCpuThreadCollapsed(thread.Id))
                        height += Math.Max(1, thread.MaxDepth) * RowHeight;
                }
                return MathF.Max(100, height);
            }
        }

        public void ScrollToSelectedEvent()
        {
            BuildGeometry();
            int index = m_Profiler.SelectedCpuEvent;
            if (index < 0 || Parent is not Panel panel)
                return;
            foreach (Header header in m_Headers)
            {
                CpuThread thread = m_Profiler.CpuThreads[header.ThreadIndex];
                if (index >= thread.FirstEvent && index - thread.FirstEvent < thread.EventCount)
                {
                    float y = header.Bounds.Y + RowHeight * (1 + m_Profiler.CpuEvents[index].Depth);
                    panel.ScrollViewTo(new Rectangle(0, y, Width, RowHeight), true);
                    return;
                }
            }
        }

        private void BuildGeometry()
        {
            if (m_Snapshot == m_Profiler.View.SnapshotId && m_Begin == m_Profiler.CpuBeginNs && m_End == m_Profiler.CpuEndNs &&
                m_LayoutVersion == m_Profiler.CpuLayoutVersion && m_Width == Width)
                return;
            m_Snapshot = m_Profiler.View.SnapshotId;
            m_Begin = m_Profiler.CpuBeginNs;
            m_End = m_Profiler.CpuEndNs;
            m_LayoutVersion = m_Profiler.CpuLayoutVersion;
            m_Width = Width;
            m_Bars.Clear();
            m_Headers.Clear();
            m_Hover = -1;
            ReadOnlySpan<CpuThread> threads = m_Profiler.CpuThreads;
            ReadOnlySpan<CpuEvent> events = m_Profiler.CpuEvents;
            float y = RowHeight;
            for (int t = 0; t < threads.Length; t++)
            {
                CpuThread thread = threads[t];
                if (m_Profiler.View.FilterThreadId != 0 && thread.Id != m_Profiler.View.FilterThreadId)
                    continue;
                m_Headers.Add(new Header { Bounds = new Rectangle(0, y, Width, RowHeight), ThreadIndex = t });
                y += RowHeight;
                if (m_Profiler.IsCpuThreadCollapsed(thread.Id))
                    continue;
                if (m_End > m_Begin)
                {
                    for (int i = thread.FirstEvent; i < thread.FirstEvent + thread.EventCount; i++)
                    {
                        CpuEvent item = events[i];
                        long end = item.EndValid != 0 ? item.EndNs : m_End;
                        if (item.BeginNs >= m_End || end < m_Begin)
                            continue;
                        float left = 4 + (float)((Math.Max(item.BeginNs, m_Begin) - m_Begin) / (double)(m_End - m_Begin)) * MathF.Max(1, Width - 8);
                        float right = 4 + (float)((Math.Min(end, m_End) - m_Begin) / (double)(m_End - m_Begin)) * MathF.Max(1, Width - 8);
                        m_Bars.Add(new Bar
                        {
                            Bounds = new Rectangle(left, y + item.Depth * RowHeight, MathF.Max(1, right - left), RowHeight - 2),
                            EventIndex = i,
                            Count = 1,
                            BeginNs = Math.Max(item.BeginNs, m_Begin),
                            EndNs = Math.Min(end, m_End),
                            Incomplete = item.EndValid == 0,
                        });
                    }
                }
                y += Math.Max(1, thread.MaxDepth) * RowHeight;
            }
            // Group only neighbouring complete bars on the same row; nested rows remain independent.
            m_Bars.Sort((Bar left, Bar right) => left.Bounds.Y != right.Bounds.Y ? left.Bounds.Y.CompareTo(right.Bounds.Y) : left.BeginNs.CompareTo(right.BeginNs));
            int write = 0;
            for (int read = 0; read < m_Bars.Count; read++)
            {
                Bar item = m_Bars[read];
                if (write > 0 && !item.Incomplete && item.Bounds.Width < 3)
                {
                    Bar previous = m_Bars[write - 1];
                    if (!previous.Incomplete && previous.Bounds.Y == item.Bounds.Y &&
                        (previous.Count > 1 || previous.Bounds.Width < 3) && item.Bounds.X <= previous.Bounds.X + previous.Bounds.Width + 3)
                    {
                        previous.Bounds.Width = MathF.Max(previous.Bounds.Width, item.Bounds.X + item.Bounds.Width - previous.Bounds.X);
                        previous.EndNs = Math.Max(previous.EndNs, item.EndNs);
                        previous.Count += item.Count;
                        previous.EventIndex = -1;
                        m_Bars[write - 1] = previous;
                        continue;
                    }
                }
                m_Bars[write++] = item;
            }
            if (write < m_Bars.Count)
                m_Bars.RemoveRange(write, m_Bars.Count - write);
        }

        public override void Draw()
        {
            base.Draw();
            BuildGeometry();
            Style style = Style.Current;
            Render2D.FillRectangle(new Rectangle(Float2.Zero, Size), style.LightBackground);
            float top = Parent != null ? PointFromParent(Float2.Zero).Y : 0;
            float bottom = top + (Parent != null ? Parent.Height : Height);
            Render2D.RenderText(style.FontMedium, $"{m_Begin / 1_000_000.0:F6} - {m_End / 1_000_000.0:F6} ms", new Rectangle(4, 0, Width - 8, RowHeight), style.TextColor);
            foreach (Header header in m_Headers)
            {
                if (header.Bounds.Y + RowHeight < top || header.Bounds.Y > bottom)
                    continue;
                CpuThread thread = m_Profiler.CpuThreads[header.ThreadIndex];
                string prefix = m_Profiler.IsCpuThreadCollapsed(thread.Id) ? ">" : "v";
                Render2D.FillRectangle(header.Bounds, style.BackgroundNormal);
                Render2D.RenderText(style.FontMedium, $"{prefix} {m_Profiler.GetCpuText(thread.NameOffset)} [{thread.Id}]  ({thread.EventCount})",
                    header.Bounds, style.TextColor);
            }
            foreach (Bar item in m_Bars)
            {
                if (item.Bounds.Y + RowHeight < top || item.Bounds.Y > bottom)
                    continue;
                Color color = item.EventIndex >= 0 && item.EventIndex == m_Profiler.SelectedCpuEvent ? style.BackgroundSelected : style.Foreground;
                if (item.Incomplete)
                {
                    // The outlined extension is a display boundary, not a measured end time.
                    Render2D.DrawRectangle(item.Bounds, color);
                }
                else
                {
                    Render2D.FillRectangle(item.Bounds, color);
                }
                if (item.Bounds.Width > 24)
                {
                    string name = item.Count > 1 ? $"{item.Count} events" : m_Profiler.GetCpuText(m_Profiler.CpuEvents[item.EventIndex].NameOffset);
                    if (item.Incomplete)
                        name += " [end unknown]";
                    Render2D.PushClip(item.Bounds);
                    Render2D.RenderText(style.FontMedium, name, item.Bounds, style.TextColor);
                    Render2D.PopClip();
                }
            }
            // A selected instance remains visible even when pixel aggregation contains its bar.
            int selected = m_Profiler.SelectedCpuEvent;
            if (selected >= 0 && m_End > m_Begin)
            {
                CpuEvent item = m_Profiler.CpuEvents[selected];
                long end = item.EndValid != 0 ? item.EndNs : m_End;
                if (item.BeginNs < m_End && end >= m_Begin)
                {
                    foreach (Header header in m_Headers)
                    {
                        CpuThread thread = m_Profiler.CpuThreads[header.ThreadIndex];
                        if (selected < thread.FirstEvent || selected - thread.FirstEvent >= thread.EventCount || m_Profiler.IsCpuThreadCollapsed(thread.Id))
                            continue;
                        float y = header.Bounds.Y + (1 + item.Depth) * RowHeight;
                        if (y + RowHeight < top || y > bottom)
                            break;
                        float left = 4 + (float)((Math.Max(item.BeginNs, m_Begin) - m_Begin) / (double)(m_End - m_Begin)) * MathF.Max(1, Width - 8);
                        float right = 4 + (float)((Math.Min(end, m_End) - m_Begin) / (double)(m_End - m_Begin)) * MathF.Max(1, Width - 8);
                        Render2D.DrawRectangle(new Rectangle(left, y, MathF.Max(2, right - left), RowHeight - 2), style.BorderSelected, 2);
                        break;
                    }
                }
            }
            if (m_Headers.Count == 0)
                Render2D.RenderText(style.FontMedium, "No CPU events in this snapshot", new Rectangle(4, RowHeight, Width - 8, RowHeight), style.TextColor);
        }

        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            BuildGeometry();
            if (button == MouseButton.Left)
            {
                foreach (Header header in m_Headers)
                {
                    if (header.Bounds.Contains(location))
                    {
                        m_Profiler.ToggleCpuThread(m_Profiler.CpuThreads[header.ThreadIndex].Id);
                        return true;
                    }
                }
                foreach (Bar item in m_Bars)
                {
                    if (!item.Bounds.Contains(location))
                        continue;
                    if (item.Count > 1)
                        m_Profiler.SetCpuRange(item.BeginNs, Math.Max(item.BeginNs + 1, item.EndNs));
                    else
                        m_Profiler.SelectCpuEvent(m_Snapshot, item.EventIndex);
                    return true;
                }
            }
            if (button == MouseButton.Middle && m_End > m_Begin)
            {
                m_Profiler.Pause();
                m_Panning = true;
                m_PanSnapshot = m_Snapshot;
                m_PanX = location.X;
                m_PanBegin = m_Begin;
                m_PanEnd = m_End;
                StartMouseCapture();
                return true;
            }
            return base.OnMouseDown(location, button);
        }

        public override bool OnMouseDoubleClick(Float2 location, MouseButton button)
        {
            BuildGeometry();
            if (button == MouseButton.Left)
            {
                foreach (Bar item in m_Bars)
                {
                    if (item.Bounds.Contains(location) && item.EventIndex >= 0)
                    {
                        m_Profiler.FocusCpuEvent(m_Snapshot, item.EventIndex);
                        return true;
                    }
                }
            }
            return base.OnMouseDoubleClick(location, button);
        }

        public override void OnMouseMove(Float2 location)
        {
            if (m_Panning && m_PanSnapshot != m_Profiler.View.SnapshotId)
            {
                m_Panning = false;
                EndMouseCapture();
            }
            if (m_Panning)
            {
                long offset = (long)((m_PanX - location.X) / Math.Max(1, Width - 8) * (m_PanEnd - m_PanBegin));
                m_Profiler.SetCpuRange(m_PanBegin + offset, m_PanEnd + offset);
            }
            BuildGeometry();
            m_Hover = -1;
            for (int i = 0; i < m_Bars.Count; i++)
            {
                if (m_Bars[i].Bounds.Contains(location))
                {
                    m_Hover = i;
                    break;
                }
            }
            base.OnMouseMove(location);
        }

        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Middle && m_Panning)
            {
                m_Panning = false;
                EndMouseCapture();
                return true;
            }
            return base.OnMouseUp(location, button);
        }

        public override void OnEndMouseCapture()
        {
            m_Panning = false;
            base.OnEndMouseCapture();
        }

        public override bool OnMouseWheel(Float2 location, float delta)
        {
            if (Root != null && Root.GetKey(KeyboardKeys.Control) && delta != 0 && m_Profiler.CpuEndNs > m_Profiler.CpuBeginNs)
            {
                double fraction = Math.Clamp((location.X - 4) / Math.Max(1, Width - 8), 0, 1);
                long duration = m_Profiler.CpuEndNs - m_Profiler.CpuBeginNs;
                long nextDuration = Math.Clamp((long)(duration * (delta > 0 ? 0.8 : 1.25)), 1, m_Profiler.View.SelectionEndNs - m_Profiler.View.SelectionBeginNs);
                long anchor = m_Profiler.CpuBeginNs + (long)(duration * fraction);
                long begin = anchor - (long)(nextDuration * fraction);
                m_Profiler.SetCpuRange(begin, begin + nextDuration);
                return true;
            }
            return base.OnMouseWheel(location, delta);
        }

        public override bool OnShowTooltip(out string text, out Float2 location, out Rectangle area)
        {
            bool show = base.OnShowTooltip(out text, out location, out area);
            if (m_Snapshot == m_Profiler.View.SnapshotId && m_Hover >= 0 && m_Hover < m_Bars.Count)
            {
                Bar item = m_Bars[m_Hover];
                text = item.Count > 1 ? $"{item.Count} complete events aggregated at this scale. Click to zoom; this is not a single call." :
                    m_Profiler.DescribeCpuEvent(item.EventIndex);
            }
            return show;
        }
    }
}
