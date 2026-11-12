using System;
using System.Diagnostics;
using SE.GUI;
using CaptureInfo = SE.ProfilerCaptureInfo;
using CaptureState = SE.ProfilerCaptureState;
using CaptureResult = SE.ProfilerCaptureResult;

namespace SE.Editor
{
    /// <summary>Process, GPU and CLR memory history, sampling and inspection.</summary>
    public sealed class ProfilerMemoryMode : ProfilerMode
    {
        public ProfilerMemoryOverviewInfo MemoryOverview { get; private set; }
        public ProfilerMemorySample[] MemorySamples { get; private set; } = Array.Empty<ProfilerMemorySample>();
        public string MemoryDetails { get; private set; } = "Select a memory sample.";
        private long m_MemoryFirst = -1;
        public void BrowseMemory(bool newer, bool end)
        {
            if (Profiler.ActiveMode != this)
            {
                return;
            }
            Profiler.Pause();
            long oldest = (long)MemoryOverview.OldestSample;
            long last = Math.Max(oldest, oldest + MemoryOverview.TotalSamples - ProfilerModule.MaxViewFrames);
            m_MemoryFirst = end ? newer ? last : oldest :
                Math.Clamp((long)MemoryOverview.First + (newer ? ProfilerModule.MaxViewFrames : -ProfilerModule.MaxViewFrames), oldest, last);
            Dirty = true;
            Profiler.RefreshView();
        }

        public void SelectMemorySample(ulong snapshot, int index)
        {
            if (snapshot != MemoryOverview.SnapshotId || index < 0 || index >= MemorySamples.Length)
                return;
            Profiler.Pause();
            m_SelectedSample = index;
            ProfilerMemorySample sample = MemorySamples[index];
            string Value(ulong bytes, int mask) => (sample.ValidMask & mask) != 0 ? $"{bytes:N0} bytes ({bytes / 1048576.0:0.##} MiB)" : "Unavailable";
            MemoryDetails = $"Sample {MemoryOverview.First + (ulong)index} submitted at {sample.TimeSeconds:0.##} s, frame {sample.FrameId}; OS counters read at {sample.NativeReadSeconds:0.##} s\n" +
                $"Process working set: {Value(sample.ProcessPhysicalBytes, 1)}\nProcess private committed: {Value(sample.ProcessPrivateBytes, 1)}\n" +
                $"System physical used: {Value(sample.SystemUsedBytes, 2)}\nSystem physical total: {Value(sample.SystemTotalBytes, 2)}\n" +
                $"Driver-reported GPU heap total: {Value(sample.GpuTotalBytes, 4)}\nEngine GPU resource estimate: {Value(sample.GpuUsedBytes, 4)}\n";
            if ((sample.ValidMask & 8) != 0)
                MemoryDetails += $"Managed heap estimate: {sample.ManagedHeapBytes:N0} bytes\nManaged allocated, process cumulative: {sample.ManagedAllocatedBytes:N0} bytes\n" +
                    $"GC collections (process cumulative): {sample.Gc0}/{sample.Gc1}/{sample.Gc2}\n" +
                    $"Managed receipt elapsed: {sample.ManagedReceiptSeconds:0.##} s; counter read took {sample.ManagedReadSeconds:0.##} s.\n";
            else
                MemoryDetails += "Managed sample unavailable.\n";
            MemoryDetails += "Sources are sampled separately. Receipt time is not atomic native/managed measurement. Values overlap and must not be summed.";
            Profiler.NotifyChanged();
        }

        internal override void Reset()
        {
            MemoryOverview = default;
            MemorySamples = Array.Empty<ProfilerMemorySample>();
            MemoryDetails = "Select a memory sample.";
            m_MemoryFirst = -1;
            m_SelectedSample = -1;
            Dirty = true;
            ViewResult = CaptureResult.Ok;
        }

        private int m_SelectedSample = -1;
        public ProfilerMemoryMode(ProfilerModule profiler) : base(profiler)
        {
        }

        public override string Title => "Memory";
        public override bool HasSnapshot => MemoryOverview.SnapshotId != 0;

        internal override void Freeze()
        {
            if (HasSnapshot)
            {
                m_MemoryFirst = (long)MemoryOverview.First;
                Dirty = false;
            }
        }

        internal override void ShowLatest()
        {
            m_MemoryFirst = -1;
            Dirty = true;
        }

        internal override CaptureResult Query(out CaptureInfo info)
        {
            CaptureResult result = Profiling.RefreshMemoryOverview(Profiler.Info.SessionId, Profiler.IsLive ? -1 : m_MemoryFirst,
                out info, out ProfilerMemoryOverviewInfo overview, out ProfilerMemorySample[] samples);
            if (result != CaptureResult.Ok)
            {
                return result;
            }
            bool samePage = HasSnapshot && MemoryOverview.First == overview.First;
            MemoryOverview = overview;
            MemorySamples = samples ?? Array.Empty<ProfilerMemorySample>();
            if (!samePage || m_SelectedSample >= MemorySamples.Length)
            {
                m_SelectedSample = -1;
                MemoryDetails = "Select a memory sample.";
            }
            if (!Profiler.IsLive || info.State == CaptureState.Stopped)
            {
                Freeze();
            }
            return result;
        }

        internal void SubmitSample()
        {
            long begin = Stopwatch.GetTimestamp();
            long heap = GC.GetTotalMemory(false);
            long allocated = GC.GetTotalAllocatedBytes(false);
            int gc0 = GC.CollectionCount(0);
            int gc1 = GC.CollectionCount(1);
            int gc2 = GC.CollectionCount(2);
            Profiling.SubmitManagedMemory(Profiler.Info.SessionId, heap, allocated, gc0, gc1, gc2,
                (Stopwatch.GetTimestamp() - begin) / (double)Stopwatch.Frequency);
        }

        private ContainerControl m_Navigation;
        private Button m_First;
        private Button m_Previous;
        private Button m_Next;
        private Button m_Last;
        private ProfilerMemoryView m_View;

        protected override void CreateControls()
        {
            m_Navigation = m_Content.AddChild(new ContainerControl());
            m_First = m_Navigation.AddChild(new Button { Text = "<<", TooltipText = "PreviousPage" });
            m_First.Clicked += () => BrowseMemory(false, true);
            m_Previous = m_Navigation.AddChild(new Button { Text = "<", TooltipText = "Previous" });
            m_Previous.Clicked += () => BrowseMemory(false, false);
            m_Next = m_Navigation.AddChild(new Button { Text = ">", TooltipText = "Next" });
            m_Next.Clicked += () => BrowseMemory(true, false);
            m_Last = m_Navigation.AddChild(new Button { Text = ">>", TooltipText = "NextPage" });
            m_Last.Clicked += () => BrowseMemory(true, true);
        }

        protected override void UpdateControls()
        {
            if (m_View == null)
            {
                m_View = m_Split.Panel1.AddChild(new ProfilerMemoryView(this));
            }
            ulong first = MemoryOverview.First;
            int count = MemoryOverview.Count;
            ulong total = MemoryOverview.OldestSample + (ulong)MemoryOverview.TotalSamples;
            m_First.Enabled = m_Previous.Enabled = first > MemoryOverview.OldestSample;
            m_Last.Enabled = m_Next.Enabled = first + (ulong)count < total;
            m_EventDetails.Text = MemoryDetails;
            m_Status.Text = $"Overview snapshot {MemoryOverview.SnapshotId}; samples {first}-{first + (ulong)count} of {total}\n" +
                "Engine elapsed seconds; managed receipt time is separate. Click a sample for exact values.\nSources overlap: do not sum process, system, GPU and managed bytes.";
        }

        protected override float LayoutControls(float width)
        {
            m_Navigation.SetBounds(8, 4, width, 24);
            float buttonWidth = MathF.Min(28, width / 4);
            m_First.SetBounds(0, 2, buttonWidth, 20);
            m_Previous.SetBounds(buttonWidth, 2, buttonWidth, 20);
            m_Next.SetBounds(width - buttonWidth * 2, 2, buttonWidth, 20);
            m_Last.SetBounds(width - buttonWidth, 2, buttonWidth, 20);
            return 32;
        }

        protected override void LayoutContent()
        {
            if (m_View == null || m_EventDetails == null)
            {
                return;
            }
            float width = MathF.Max(m_View.RequiredWidth, m_Split.Panel1.GetDesireClientArea().Width);
            m_View.SetBounds(0, 0, width, m_View.RequiredHeight);
        }

        protected override string GetDataInfo()
        {
            return $"Memory query: {ViewResult}; capture: {Profiler.Info.State}\n" +
                $"Overview retains {Profiler.HistoryCapacity} engine samples. Paused samples remain available until refreshed or cleared.\n" +
                "Process, system, GPU and CLR measurements overlap and are sampled independently. GC counters are process cumulative.\n" +
                $"Last operation {Profiler.LastResult}; capture error {Profiler.Info.Error}.\n{Profiler.LoadError}";
        }
    }
}
