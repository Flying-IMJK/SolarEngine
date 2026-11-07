using System;
using SE.GUI;
using CpuStatistic = SE.ProfilerCpuStatistic;
using SortColumn = SE.Editor.ProfilerModule.StatisticSortColumn;

namespace SE.Editor
{
    /// <summary>Visible-row rendering and hit testing for the module's cached native aggregates.</summary>
    internal sealed class ProfilerCpuStatistics : Control
    {
        public const float RowHeight = 24;
        public const float MinimumWidth = 800;
        private static readonly string[] s_Headers = { "Source", "Calls", "End unknown", "Total (ms)", "Self (ms)", "Average (ms)", "Max (ms)" };
        private readonly ProfilerModule m_Profiler;
        private ulong m_DrawnSnapshot;
        private int m_DrawnVersion = -1;
        private int m_Hover = -1;

        public ProfilerCpuStatistics(ProfilerModule profiler)
        {
            m_Profiler = profiler;
            TooltipText = "Click a header to sort. Click a source to inspect its largest complete instance. Double-click to locate it on the timeline.";
        }

        public float RequiredHeight => MathF.Max(96, (m_Profiler.VisibleStatisticCount + 1) * RowHeight);

        private Rectangle Cell(int row, int column)
        {
            float nameWidth = MathF.Max(220, Width - 6 * 92);
            return column == 0 ? new Rectangle(0, row * RowHeight, nameWidth, RowHeight) :
                new Rectangle(nameWidth + (column - 1) * 92, row * RowHeight, 92, RowHeight);
        }

        public override void Draw()
        {
            base.Draw();
            if (m_DrawnSnapshot != m_Profiler.View.SnapshotId || m_DrawnVersion != m_Profiler.StatisticPresentationVersion)
                m_Hover = -1;
            m_DrawnSnapshot = m_Profiler.View.SnapshotId;
            m_DrawnVersion = m_Profiler.StatisticPresentationVersion;
            Style style = Style.Current;
            Render2D.FillRectangle(new Rectangle(Float2.Zero, Size), style.LightBackground);
            for (int column = 0; column < s_Headers.Length; column++)
            {
                Rectangle cell = Cell(0, column);
                string text = s_Headers[column];
                if ((int)m_Profiler.StatisticSort == column)
                    text += m_Profiler.StatisticSortDescending ? " v" : " ^";
                Render2D.FillRectangle(cell, style.BackgroundNormal);
                Render2D.PushClip(cell);
                Render2D.RenderText(style.FontMedium, text, cell, style.TextColor);
                Render2D.PopClip();
            }
            float top = Parent != null ? PointFromParent(Float2.Zero).Y : 0;
            float bottom = top + (Parent != null ? Parent.Height : Height);
            int first = Math.Clamp((int)(top / RowHeight) - 1, 0, m_Profiler.VisibleStatisticCount);
            int end = Math.Min(m_Profiler.VisibleStatisticCount, (int)Math.Ceiling(bottom / RowHeight));
            for (int i = first; i < end; i++)
            {
                int index = m_Profiler.GetVisibleStatisticIndex(i);
                CpuStatistic row = m_Profiler.CpuStatistics[index];
                if (index == m_Profiler.SelectedCpuStatistic)
                    Render2D.FillRectangle(new Rectangle(0, (i + 1) * RowHeight, Width, RowHeight), style.BackgroundSelected);
                for (int column = 0; column < s_Headers.Length; column++)
                {
                    string text = column switch
                    {
                        0 => $"{m_Profiler.GetCpuText(row.NameOffset)} [{row.SourceLocationId}]",
                        1 => row.Calls.ToString(),
                        2 => row.IncompleteCalls.ToString(),
                        3 => row.Calls > 0 ? $"{row.TotalNs / 1_000_000.0:F3}" : "--",
                        4 => row.SelfIncompleteCalls != 0 ? "incomplete" : row.Calls > 0 ? $"{row.SelfNs / 1_000_000.0:F3}" : "--",
                        5 => row.Calls > 0 ? $"{row.AverageNs / 1_000_000.0:F3}" : "--",
                        _ => row.Calls > 0 ? $"{row.MaxNs / 1_000_000.0:F3}" : "--",
                    };
                    Rectangle cell = Cell(i + 1, column);
                    Render2D.PushClip(cell);
                    Render2D.RenderText(style.FontMedium, text, cell, style.TextColor);
                    Render2D.PopClip();
                }
            }
            if (m_Profiler.VisibleStatisticCount == 0)
                Render2D.RenderText(style.FontMedium, "No matching source statistics in this snapshot.", new Rectangle(4, RowHeight, Width - 8, RowHeight), style.TextColor);
        }

        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Left && m_DrawnSnapshot == m_Profiler.View.SnapshotId &&
                m_DrawnVersion == m_Profiler.StatisticPresentationVersion)
            {
                if (location.Y >= 0 && location.Y < RowHeight)
                {
                    float nameWidth = Cell(0, 0).Width;
                    int column = location.X < nameWidth ? 0 : Math.Clamp((int)((location.X - nameWidth) / 92) + 1, 1, 6);
                    m_Profiler.SortCpuStatistics((SortColumn)column);
                    return true;
                }
                int position = (int)(location.Y / RowHeight) - 1;
                if (position >= 0 && position < m_Profiler.VisibleStatisticCount)
                {
                    m_Profiler.SelectCpuStatistic(m_DrawnSnapshot, m_Profiler.GetVisibleStatisticIndex(position));
                    return true;
                }
            }
            return base.OnMouseDown(location, button);
        }

        public override bool OnMouseDoubleClick(Float2 location, MouseButton button)
        {
            int position = (int)(location.Y / RowHeight) - 1;
            if (button == MouseButton.Left && m_DrawnSnapshot == m_Profiler.View.SnapshotId &&
                m_DrawnVersion == m_Profiler.StatisticPresentationVersion && position >= 0 && position < m_Profiler.VisibleStatisticCount)
            {
                m_Profiler.FocusCpuStatistic(m_DrawnSnapshot, m_Profiler.GetVisibleStatisticIndex(position));
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
            if (m_DrawnSnapshot == m_Profiler.View.SnapshotId && m_DrawnVersion == m_Profiler.StatisticPresentationVersion &&
                m_Hover >= 0 && m_Hover < m_Profiler.VisibleStatisticCount)
                text = m_Profiler.DescribeCpuStatistic(m_Profiler.GetVisibleStatisticIndex(m_Hover));
            return show;
        }
    }
}
