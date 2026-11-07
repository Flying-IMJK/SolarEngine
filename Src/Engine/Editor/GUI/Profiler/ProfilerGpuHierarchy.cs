using System;
using SE.GUI;
using GpuEvent = SE.ProfilerGpuEvent;

namespace SE.Editor
{
    /// <summary>
    /// Hierarchy durations and drawing counters, without invented GPU execution positions.
    /// </summary>
    internal sealed class ProfilerGpuHierarchy : Control
    {
        public const float MinimumWidth = 900;
        private const float RowHeight = 24;
        private static readonly string[] s_Headers = { "GPU event", "GPU ms", "Parent %", "Draw", "Dispatch", "Vertices", "Triangles", "PS changes" };
        private readonly ProfilerModule m_Profiler;
        private ulong m_Snapshot;
        private int m_Version;
        private int m_Hover = -1;

        public ProfilerGpuHierarchy(ProfilerModule profiler)
        {
            m_Profiler = profiler;
            TooltipText = "Click to inspect. Double-click to fold/unfold. GPU durations have no absolute positions in this view.";
        }

        public float RequiredHeight => MathF.Max(96, (m_Profiler.VisibleGpuEventCount + 1) * RowHeight);

        private Rectangle Cell(int row, int column)
        {
            float nameWidth = MathF.Max(260, Width - 7 * 90);
            return column == 0 ? new Rectangle(0, row * RowHeight, nameWidth, RowHeight) :
                new Rectangle(nameWidth + (column - 1) * 90, row * RowHeight, 90, RowHeight);
        }

        public override void Draw()
        {
            base.Draw();
            if (m_Snapshot != m_Profiler.GpuView.SnapshotId || m_Version != m_Profiler.GpuLayoutVersion)
                m_Hover = -1;
            m_Snapshot = m_Profiler.GpuView.SnapshotId;
            m_Version = m_Profiler.GpuLayoutVersion;
            Style style = Style.Current;
            Render2D.FillRectangle(new Rectangle(Float2.Zero, Size), style.LightBackground);
            for (int column = 0; column < s_Headers.Length; column++)
            {
                Rectangle cell = Cell(0, column);
                Render2D.FillRectangle(cell, style.BackgroundNormal);
                Render2D.PushClip(cell);
                Render2D.RenderText(style.FontMedium, s_Headers[column], cell, style.TextColor);
                Render2D.PopClip();
            }
            float top = Parent != null ? PointFromParent(Float2.Zero).Y : 0;
            float bottom = top + (Parent != null ? Parent.Height : Height);
            int first = Math.Clamp((int)(top / RowHeight) - 1, 0, m_Profiler.VisibleGpuEventCount);
            int end = Math.Min(m_Profiler.VisibleGpuEventCount, (int)Math.Ceiling(bottom / RowHeight));
            for (int row = first; row < end; row++)
            {
                int index = m_Profiler.GetVisibleGpuEvent(row);
                GpuEvent item = m_Profiler.GpuEvents[index];
                if (index == m_Profiler.SelectedGpuEvent)
                    Render2D.FillRectangle(new Rectangle(0, (row + 1) * RowHeight, Width, RowHeight), style.BackgroundSelected);
                bool hasChildren = index + 1 < m_Profiler.GpuEvents.Length && m_Profiler.GpuEvents[index + 1].Depth > item.Depth;
                for (int column = 0; column < s_Headers.Length; column++)
                {
                    string text = column switch
                    {
                        0 => $"{(hasChildren ? m_Profiler.IsGpuEventCollapsed(index) ? "> " : "v " : string.Empty)}{m_Profiler.GetGpuText(item.NameOffset)} [d{item.Depth}]",
                        1 => item.TimingValid != 0 ? $"{item.DurationMs:F3}" : "--",
                        2 => item.ParentPercent >= 0 ? $"{item.ParentPercent:F2}" : "--",
                        3 => item.StatsValid != 0 ? item.DrawCalls.ToString() : "--",
                        4 => item.StatsValid != 0 ? item.DispatchCalls.ToString() : "--",
                        5 => item.StatsValid != 0 ? item.Vertices.ToString() : "--",
                        6 => item.StatsValid != 0 ? item.Triangles.ToString() : "--",
                        _ => item.StatsValid != 0 ? item.PipelineStateChanges.ToString() : "--",
                    };
                    Rectangle cell = Cell(row + 1, column);
                    Render2D.PushClip(cell);
                    if (column == 0)
                    {
                        float indent = Math.Clamp(item.Depth * 12.0f, 0, cell.Width - 60);
                        cell.X += indent;
                        cell.Width -= indent;
                    }
                    Render2D.RenderText(style.FontMedium, text, cell, style.TextColor);
                    Render2D.PopClip();
                }
            }
            if (m_Profiler.VisibleGpuEventCount == 0)
            {
                Render2D.RenderText(style.FontMedium, "No resolved GPU event data in this frozen selection. See frame status.", new Rectangle(4, RowHeight, Width - 8, RowHeight), style.TextColor);
            }
        }

        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            int row = (int)(location.Y / RowHeight) - 1;
            if (button == MouseButton.Left && m_Snapshot == m_Profiler.GpuView.SnapshotId && m_Version == m_Profiler.GpuLayoutVersion && row >= 0 && row < m_Profiler.VisibleGpuEventCount)
            {
                m_Profiler.SelectGpuEvent(m_Snapshot, m_Profiler.GetVisibleGpuEvent(row));
                return true;
            }
            return base.OnMouseDown(location, button);
        }

        public override bool OnMouseDoubleClick(Float2 location, MouseButton button)
        {
            int row = (int)(location.Y / RowHeight) - 1;
            if (button == MouseButton.Left && m_Snapshot == m_Profiler.GpuView.SnapshotId && m_Version == m_Profiler.GpuLayoutVersion && row >= 0 && row < m_Profiler.VisibleGpuEventCount)
            {
                m_Profiler.ToggleGpuEvent(m_Snapshot, m_Profiler.GetVisibleGpuEvent(row));
                return true;
            }
            return base.OnMouseDoubleClick(location, button);
        }

        public override void OnMouseMove(Float2 location)
        {
            m_Hover = (int)(location.Y / RowHeight) - 1;
            base.OnMouseMove(location);
        }

        public override bool OnShowTooltip(out string text, out Float2 location, out Rectangle area)
        {
            bool show = base.OnShowTooltip(out text, out location, out area);
            if (m_Snapshot == m_Profiler.GpuView.SnapshotId && m_Version == m_Profiler.GpuLayoutVersion && m_Hover >= 0 && m_Hover < m_Profiler.VisibleGpuEventCount)
                text = m_Profiler.DescribeGpuEvent(m_Profiler.GetVisibleGpuEvent(m_Hover));
            return show;
        }
    }
}
