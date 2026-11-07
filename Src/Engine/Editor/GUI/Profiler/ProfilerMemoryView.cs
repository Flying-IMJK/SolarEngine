using System;
using SE.GUI;

namespace SE.Editor
{
    /// <summary>Cached memory trends, exact bucket envelopes and a frozen allocation page.</summary>
    internal sealed class ProfilerMemoryView : Control
    {
        private readonly ProfilerModule m_Profiler;
        private ulong m_Snapshot;
        private int m_DragStart = -1;
        private ulong m_DragSnapshot;
        private int m_OverviewFirst;
        private int m_AllocationFirst;
        private const float ChartHeight = 180;
        private const float RowHeight = 24;
        private static readonly string[] s_Titles =
        {
            "Process: working set / private committed (bytes)",
            "System: physical used / total (bytes)",
            "GPU: engine resource estimate / driver heap total (bytes)",
            "Managed: heap estimate (bytes)",
            "Managed: allocated bytes, process cumulative",
            "GC: gen0 / gen1 / gen2, process cumulative"
        };

        public ProfilerMemoryView(ProfilerModule profiler)
        {
            m_Profiler = profiler;
            TooltipText = "Overview: click a sample for exact values. Native curve: click inspection time, drag to query a range. List: click allocation.";
        }

        public float RequiredHeight => m_Profiler.IsNativeMemoryVisible ? ChartHeight + (m_Profiler.Allocations.Length + 2) * RowHeight : 6 * 110;
        public float RequiredWidth => m_Profiler.IsNativeMemoryVisible ? 1450 : 600;

        private static double Metric(ProfilerMemorySample sample, int lane, int series)
        {
            int mask = lane == 0 ? 1 : lane == 1 ? 2 : lane == 2 ? 4 : 8;
            if ((sample.ValidMask & mask) == 0)
                return double.NaN;
            return lane switch
            {
                0 => series == 0 ? sample.ProcessPhysicalBytes : sample.ProcessPrivateBytes,
                1 => series == 0 ? sample.SystemUsedBytes : sample.SystemTotalBytes,
                2 => series == 0 ? sample.GpuUsedBytes : sample.GpuTotalBytes,
                3 => sample.ManagedHeapBytes,
                4 => sample.ManagedAllocatedBytes,
                _ => series == 0 ? sample.Gc0 : series == 1 ? sample.Gc1 : sample.Gc2
            };
        }

        public override void Draw()
        {
            base.Draw();
            Style style = Style.Current;
            Render2D.FillRectangle(new Rectangle(Float2.Zero, Size), style.LightBackground);
            m_Snapshot = m_Profiler.IsNativeMemoryVisible ? m_Profiler.AllocationView.SnapshotId : m_Profiler.MemoryOverview.SnapshotId;
            m_OverviewFirst = m_Profiler.MemoryOverview.First;
            m_AllocationFirst = m_Profiler.AllocationPage.First;
            if (!m_Profiler.IsNativeMemoryVisible)
            {
                ProfilerMemorySample[] samples = m_Profiler.MemorySamples;
                for (int lane = 0; lane < 6; lane++)
                {
                    int seriesCount = lane < 3 ? 2 : lane == 5 ? 3 : 1;
                    double maximum = 1;
                    for (int i = 0; i < samples.Length; i++)
                        for (int s = 0; s < seriesCount; s++)
                        {
                            double value = Metric(samples[i], lane, s);
                            if (!double.IsNaN(value))
                                maximum = Math.Max(maximum, value);
                        }
                    Render2D.RenderText(style.FontMedium, s_Titles[lane] + $"   max {maximum:N0}",
                        new Rectangle(4, lane * 110, Width - 8, 24), style.TextColor);
                    Rectangle plot = new Rectangle(8, lane * 110 + 28, Width - 16, 72);
                    Render2D.DrawRectangle(plot, style.BackgroundNormal);
                    if (samples.Length == 0)
                        Render2D.RenderText(style.FontMedium, "No recorded samples", plot, style.TextColor);
                    else
                    {
                        double first = samples[0].TimeSeconds;
                        double duration = Math.Max(0.001, samples[samples.Length - 1].TimeSeconds - first);
                        bool hasValue = false;
                        for (int s = 0; s < seriesCount; s++)
                        {
                            bool previous = false;
                            Float2 from = Float2.Zero;
                            Color color = s == 0 ? style.TextColor : s == 1 ? style.BackgroundSelected : style.BackgroundHighlighted;
                            for (int i = 0; i < samples.Length; i++)
                            {
                                double value = Metric(samples[i], lane, s);
                                if (double.IsNaN(value)) { previous = false; continue; }
                                hasValue = true;
                                Float2 to = new Float2(plot.X + (float)((samples[i].TimeSeconds - first) / duration) * plot.Width,
                                    plot.Bottom - (float)(value / maximum) * plot.Height);
                                if (previous) Render2D.DrawLine(from, to, color, 2);
                                else Render2D.FillRectangle(new Rectangle(to.X, to.Y, 2, 2), color);
                                from = to;
                                previous = true;
                            }
                        }
                        if (!hasValue) Render2D.RenderText(style.FontMedium, "Unavailable", plot, style.TextColor);
                    }
                }
                return;
            }

            ProfilerMemoryBucket[] buckets = m_Profiler.MemoryBuckets;
            ulong peak = 1;
            foreach (ProfilerMemoryBucket bucket in buckets) peak = Math.Max(peak, bucket.MaxBytes);
            Render2D.RenderText(style.FontMedium, $"Tracked live bytes: min/max envelope and bucket end; peak {peak:N0}. Click: inspect, drag: range.",
                new Rectangle(4, 0, Width - 8, 24), style.TextColor);
            Rectangle chart = new Rectangle(8, 28, Width - 16, 126);
            for (int i = 0; i < buckets.Length; i++)
            {
                float x = chart.X + i * chart.Width / buckets.Length;
                float high = chart.Bottom - (float)(buckets[i].MaxBytes / (double)peak) * chart.Height;
                float low = chart.Bottom - (float)(buckets[i].MinBytes / (double)peak) * chart.Height;
                float last = chart.Bottom - (float)(buckets[i].EndBytes / (double)peak) * chart.Height;
                Render2D.DrawLine(new Float2(x, high), new Float2(x, low), style.BackgroundSelected, MathF.Max(1, chart.Width / buckets.Length));
                Render2D.FillRectangle(new Rectangle(x, last, MathF.Max(1, chart.Width / buckets.Length), 2), style.TextColor);
            }
            if (buckets.Length == 0)
                Render2D.RenderText(style.FontMedium, "No successful frozen memory query. Adjust range or retry.", chart, style.TextColor);
            Render2D.RenderText(style.FontMedium, "Address                 Bytes       Alloc ms        Free ms       Alloc thread / Free thread       Pool / Record       Active at T       Lifetime at T ms",
                new Rectangle(4, ChartHeight, Width - 8, RowHeight), style.TextColor);
            float top = Parent != null ? PointFromParent(Float2.Zero).Y : 0;
            float bottom = top + (Parent != null ? Parent.Height : Height);
            int firstRow = Math.Clamp((int)((top - ChartHeight) / RowHeight) - 1, 0, m_Profiler.Allocations.Length);
            int lastRow = Math.Clamp((int)Math.Ceiling((bottom - ChartHeight) / RowHeight), 0, m_Profiler.Allocations.Length);
            for (int i = firstRow; i < lastRow; i++)
            {
                ProfilerAllocation row = m_Profiler.Allocations[i];
                Rectangle area = new Rectangle(0, ChartHeight + (i + 1) * RowHeight, Width, RowHeight);
                if (i == m_Profiler.SelectedAllocation) Render2D.FillRectangle(area, style.BackgroundSelected);
                string freed = row.FreeNs >= 0 ? $"{row.FreeNs / 1_000_000m:F6}" : "--";
                long inspection = m_Profiler.AllocationView.InspectNs;
                long lifetimeEnd = row.FreeNs >= 0 && row.FreeNs <= inspection ? row.FreeNs : inspection;
                string lifetime = row.AllocNs <= inspection ? $"{(lifetimeEnd - row.AllocNs) / 1_000_000m:F6}" : "--";
                string value = $"0x{row.Address:X16}   {row.Size:N0}   {row.AllocNs / 1_000_000m:F6}   {freed}   " +
                    $"{row.AllocThread} / {(row.FreeNs >= 0 ? row.FreeThread.ToString() : "--")}   {row.PoolId} / {row.RecordId}   " +
                    (row.AllocNs > inspection ? "Not yet allocated" : row.ActiveAtInspection != 0 ? "Yes" : "No") + $"   {lifetime}";
                Render2D.RenderText(style.FontMedium, value, area, style.TextColor);
            }
        }

        private int Bucket(float x) => Math.Clamp((int)((x - 8) / MathF.Max(1, Width - 16) * m_Profiler.MemoryBuckets.Length), 0,
            Math.Max(0, m_Profiler.MemoryBuckets.Length - 1));

        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            if (button != MouseButton.Left) return base.OnMouseDown(location, button);
            if (!m_Profiler.IsNativeMemoryVisible)
            {
                if (m_Snapshot != m_Profiler.MemoryOverview.SnapshotId || m_OverviewFirst != m_Profiler.MemoryOverview.First)
                    return base.OnMouseDown(location, button);
                int index = 0;
                ProfilerMemorySample[] samples = m_Profiler.MemorySamples;
                if (samples.Length > 0)
                {
                    double fraction = Math.Clamp((location.X - 8) / MathF.Max(1, Width - 16), 0, 1);
                    double time = samples[0].TimeSeconds + fraction * (samples[samples.Length - 1].TimeSeconds - samples[0].TimeSeconds);
                    for (int i = 1; i < samples.Length; i++)
                        if (Math.Abs(samples[i].TimeSeconds - time) < Math.Abs(samples[index].TimeSeconds - time)) index = i;
                }
                m_Profiler.SelectMemorySample(m_Snapshot, index);
                return true;
            }
            if (m_Snapshot != m_Profiler.AllocationView.SnapshotId || m_AllocationFirst != m_Profiler.AllocationPage.First)
                return base.OnMouseDown(location, button);
            if (location.Y >= 28 && location.Y < 154 && m_Profiler.MemoryBuckets.Length > 0)
            {
                m_Profiler.Pause();
                m_DragStart = Bucket(location.X);
                m_DragSnapshot = m_Snapshot;
                StartMouseCapture();
                return true;
            }
            int rowIndex = (int)((location.Y - ChartHeight) / RowHeight) - 1;
            if (location.Y >= ChartHeight + RowHeight && rowIndex >= 0 && rowIndex < m_Profiler.Allocations.Length)
            {
                m_Profiler.SelectAllocation(m_Snapshot, rowIndex);
                return true;
            }
            return base.OnMouseDown(location, button);
        }

        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Left && m_DragStart >= 0)
            {
                int start = m_DragStart;
                int end = Bucket(location.X);
                m_DragStart = -1;
                EndMouseCapture();
                if (m_Profiler.IsMemoryVisible && m_Profiler.IsNativeMemoryVisible && m_DragSnapshot == m_Profiler.AllocationView.SnapshotId && m_Profiler.MemoryBuckets.Length > 0)
                {
                    ProfilerMemoryBucket[] buckets = m_Profiler.MemoryBuckets;
                    long begin = start == end ? m_Profiler.AllocationView.BeginNs : buckets[Math.Min(start, end)].BeginNs;
                    long finish = start == end ? m_Profiler.AllocationView.EndNs : buckets[Math.Max(start, end)].EndNs;
                    long inspect = buckets[end].BeginNs;
                    m_Profiler.SelectMemoryInterval(m_DragSnapshot, begin, finish, inspect);
                }
                return true;
            }
            return base.OnMouseUp(location, button);
        }

        public override void OnEndMouseCapture()
        {
            m_DragStart = -1;
            base.OnEndMouseCapture();
        }
    }
}

