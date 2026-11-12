using System;
using SE.GUI;

namespace SE.Editor
{
    /// <summary>Cached process, system, GPU and managed memory trends.</summary>
    internal sealed class ProfilerMemoryView : Control
    {
        private readonly ProfilerMemoryMode m_Profiler;
        private ulong m_Snapshot;
        private ulong m_OverviewFirst;
        private static readonly string[] s_Titles =
        {
            "Process: working set / private committed (bytes)",
            "System: physical used / total (bytes)",
            "GPU: engine resource estimate / driver heap total (bytes)",
            "Managed: heap estimate (bytes)",
            "Managed: allocated bytes, process cumulative",
            "GC: gen0 / gen1 / gen2, process cumulative"
        };

        public ProfilerMemoryView(ProfilerMemoryMode profiler)
        {
            m_Profiler = profiler;
            TooltipText = "Click a memory sample for exact values.";
        }

        public float RequiredHeight => 6 * 110;
        public float RequiredWidth => 600;

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
            m_Snapshot = m_Profiler.MemoryOverview.SnapshotId;
            m_OverviewFirst = m_Profiler.MemoryOverview.First;
            ProfilerMemorySample[] samples = m_Profiler.MemorySamples;
            for (int lane = 0; lane < 6; lane++)
            {
                int seriesCount = lane < 3 ? 2 : lane == 5 ? 3 : 1;
                double maximum = 0;
                bool available = false;
                for (int i = 0; i < samples.Length; i++)
                    for (int s = 0; s < seriesCount; s++)
                    {
                        double value = Metric(samples[i], lane, s);
                        if (!double.IsNaN(value))
                        {
                            maximum = Math.Max(maximum, value);
                            available = true;
                        }
                    }
                Render2D.RenderText(style.FontMedium, s_Titles[lane] + (available ? $"   max {maximum:N0}" : "   unavailable"),
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
                                plot.Bottom - (float)(value / Math.Max(1, maximum)) * plot.Height);
                            if (previous) Render2D.DrawLine(from, to, color, 2);
                            else Render2D.FillRectangle(new Rectangle(to.X, to.Y, 2, 2), color);
                            from = to;
                            previous = true;
                        }
                    }
                    if (!hasValue) Render2D.RenderText(style.FontMedium, "Unavailable", plot, style.TextColor);
                }
            }
        }

        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            if (button != MouseButton.Left) return base.OnMouseDown(location, button);
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
    }
}
