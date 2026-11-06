using System;
using System.Diagnostics;
using CaptureInfo = SE.ProfilerCaptureInfo;
using CaptureFrame = SE.ProfilerCaptureFrame;
using CaptureState = SE.ProfilerCaptureState;
using CaptureResult = SE.ProfilerCaptureResult;
using StopReason = SE.ProfilerStopReason;
using FrameViewInfo = SE.ProfilerFrameViewInfo;

namespace SE.Editor
{
    /// <summary>
    /// Owns the editor's local capture workflow. All methods run on the editor main thread.
    /// Transport records are explicit copies, never views of Tracy's internal objects.
    /// </summary>
    public sealed class ProfilerModule : EditorModule
    {
        public const int MaxViewFrames = 256;

        private long m_LastRefresh;
        private bool m_NativeAvailable = true;
        private bool m_ViewDirty = true;
        private ulong m_FirstFrame = ulong.MaxValue;
        private ulong m_SelectionFirst;
        private int m_SelectionCount;
        private CaptureFrame[] m_Frames = Array.Empty<CaptureFrame>();

        public ProfilerModule(Editor editor) : base(editor)
        {
        }

        internal override int Order => -85;

        public CaptureInfo Info { get; private set; }
        public FrameViewInfo View { get; private set; }
        public ReadOnlySpan<CaptureFrame> Frames => new ReadOnlySpan<CaptureFrame>(m_Frames, 0, View.FrameCount);
        public CaptureFrame? Frame => View.SelectionCount == 1 ? m_Frames[(int)(View.SelectionFirst - View.FirstFrame)] : null;
        public bool IsLive { get; private set; } = true;
        public bool IsViewPending => m_ViewDirty &&
            (Info.State == CaptureState.Capturing || Info.State == CaptureState.Stopping || Info.State == CaptureState.Stopped);
        /// <summary>The last capture control result; automatic view polling cannot overwrite it.</summary>
        public CaptureResult LastResult { get; private set; }
        public CaptureResult ViewResult { get; private set; }
        public string LoadError { get; private set; } = string.Empty;
        public event Action Changed;

        public string MenuLabel => Info.State switch
        {
            CaptureState.Connecting => "Profiler (connecting)",
            CaptureState.Capturing => "Profiler (REC)",
            CaptureState.Stopping when Info.StopReason == StopReason.CapacityLimit => "Profiler (capacity stopping)",
            CaptureState.Stopping => "Profiler (stopping)",
            CaptureState.Stopped when Info.StopReason == StopReason.CapacityLimit => "Profiler (capacity stopped)",
            CaptureState.Stopped when Info.StopReason == StopReason.ReceiverEnded => "Profiler (receiver ended)",
            CaptureState.Stopped => "Profiler (stopped)",
            CaptureState.Clearing => "Profiler (cleaning)",
            _ => "Profiler",
        };

        public override void OnInit()
        {
            Refresh();
        }

        public override void OnUpdate()
        {
            long now = Stopwatch.GetTimestamp();
            if (now - m_LastRefresh >= Stopwatch.Frequency / 10)
            {
                m_LastRefresh = now;
                Refresh();
            }
        }

        public CaptureResult Start()
        {
            LastResult = m_NativeAvailable ? Profiling.StartCapture() : CaptureResult.Unavailable;
            if (LastResult == CaptureResult.Ok)
            {
                IsLive = true;
                m_FirstFrame = ulong.MaxValue;
                m_SelectionCount = 0;
                m_ViewDirty = true;
            }
            Refresh();
            return LastResult;
        }

        public CaptureResult Stop()
        {
            LastResult = m_NativeAvailable ? Profiling.StopCapture() : CaptureResult.Unavailable;
            Refresh();
            return LastResult;
        }

        public CaptureResult Clear()
        {
            LastResult = m_NativeAvailable ? Profiling.ClearCapture() : CaptureResult.Unavailable;
            if (LastResult == CaptureResult.Ok)
            {
                View = default;
                m_ViewDirty = true;
            }
            Refresh();
            return LastResult;
        }

        /// <summary>Selects a complete frame, loading its historical page if needed; viewing remains paused.</summary>
        public CaptureResult ReadFrame(ulong index)
        {
            return SelectFrames(index, index);
        }

        /// <summary>Freezes the current native snapshot while state polling and background capture continue.</summary>
        public void Pause()
        {
            IsLive = false;
            if (View.SnapshotId != 0)
            {
                m_FirstFrame = View.FirstFrame;
                m_SelectionFirst = View.SelectionFirst;
                m_SelectionCount = (int)View.SelectionCount;
                m_ViewDirty = false;
            }
            Changed?.Invoke();
        }

        /// <summary>Follows live frames, or requests only the last complete page of a stopped capture.</summary>
        public void ShowLatestFrame()
        {
            IsLive = Info.State == CaptureState.Capturing || Info.State == CaptureState.Connecting || Info.State == CaptureState.Stopping;
            m_FirstFrame = ulong.MaxValue;
            m_SelectionCount = 0;
            m_ViewDirty = true;
            Refresh();
        }

        /// <summary>Replaces the current query explicitly, including when paused.</summary>
        public void RefreshView()
        {
            m_ViewDirty = true;
            Refresh();
        }

        /// <summary>Selects an inclusive range of up to 256 complete frames. Times use [first.Begin, last.End).</summary>
        public CaptureResult SelectFrames(ulong first, ulong last)
        {
            if (!m_NativeAvailable || Info.State == CaptureState.Unavailable)
            {
                ViewResult = CaptureResult.Unavailable;
                Changed?.Invoke();
                return ViewResult;
            }
            if (Info.State != CaptureState.Capturing && Info.State != CaptureState.Stopping && Info.State != CaptureState.Stopped)
            {
                ViewResult = CaptureResult.NotReady;
                Changed?.Invoke();
                return ViewResult;
            }
            // The view copy can observe newer complete frames than the preceding non-blocking state poll.
            ulong completeFrames = Math.Max(Info.CompleteFrames, View.TotalFrames);
            if (last < first || last >= completeFrames)
            {
                ViewResult = CaptureResult.InvalidRange;
                Changed?.Invoke();
                return ViewResult;
            }
            if (last - first >= MaxViewFrames)
            {
                ViewResult = CaptureResult.QueryTooLarge;
                Changed?.Invoke();
                return ViewResult;
            }
            IsLive = false;
            m_FirstFrame = View.FrameCount > 0 && first >= View.FirstFrame && last - View.FirstFrame < (ulong)View.FrameCount ? View.FirstFrame : first;
            m_SelectionFirst = first;
            m_SelectionCount = (int)(last - first + 1);
            m_ViewDirty = true;
            ViewResult = CaptureResult.NotReady;
            Refresh();
            return ViewResult;
        }

        public void BrowseFrames(bool newer)
        {
            if (View.FrameCount == 0)
            {
                return;
            }
            ulong completeFrames = Math.Max(Info.CompleteFrames, View.TotalFrames);
            ulong latest = completeFrames > MaxViewFrames ? completeFrames - MaxViewFrames : 0;
            m_FirstFrame = newer ? Math.Min(View.FirstFrame + MaxViewFrames, latest) :
                View.FirstFrame > MaxViewFrames ? View.FirstFrame - MaxViewFrames : 0;
            IsLive = false;
            m_SelectionCount = 0;
            m_ViewDirty = true;
            Refresh();
        }

        /// <summary>
        /// Main-thread copy of a frozen native view into a separate caller-owned array. Releasing this result
        /// does not release the snapshot; reads of a replaced snapshot return InvalidSnapshot.
        /// </summary>
        public CaptureResult ReadView(ulong sessionId, ulong snapshotId, out FrameViewInfo view, out CaptureFrame[] frames)
        {
            view = default;
            frames = Array.Empty<CaptureFrame>();
            if (!m_NativeAvailable)
            {
                return CaptureResult.Unavailable;
            }
            CaptureResult result = Profiling.ReadFrameView(sessionId, snapshotId, out view, out frames);
            frames ??= Array.Empty<CaptureFrame>();
            return result;
        }

        public override void OnDispose()
        {
            if (m_NativeAvailable && (Info.State == CaptureState.Capturing || Info.State == CaptureState.Connecting))
            {
                Profiling.StopCapture();
            }
            // ProfilingSystem owns final shutdown and joins before the runtime facilities exit.
            Changed = null;
        }

        private void Refresh()
        {
            if (!m_NativeAvailable)
            {
                return;
            }
            try
            {
                CaptureResult result = Profiling.GetCaptureInfo(out CaptureInfo info);
                if (result != CaptureResult.Ok)
                {
                    ViewResult = result;
                    Changed?.Invoke();
                    return;
                }
                if (Info.SessionId != info.SessionId || info.State == CaptureState.Clearing)
                {
                    View = default;
                    m_FirstFrame = ulong.MaxValue;
                    m_SelectionCount = 0;
                    m_ViewDirty = true;
                }
                Info = info;
                if ((IsLive || m_ViewDirty) &&
                    (info.State == CaptureState.Capturing || info.State == CaptureState.Stopping || info.State == CaptureState.Stopped))
                {
                    result = Profiling.RefreshFrameView(info.SessionId, IsLive ? ulong.MaxValue : m_FirstFrame, MaxViewFrames,
                        IsLive ? 0 : m_SelectionFirst, IsLive ? 0 : m_SelectionCount, out FrameViewInfo view, out CaptureFrame[] frames);
                    if (result == CaptureResult.Ok)
                    {
                        m_Frames = frames ?? Array.Empty<CaptureFrame>();
                        View = view;
                        m_ViewDirty = false;
                        ViewResult = CaptureResult.Ok;
                        if (info.State == CaptureState.Stopped)
                        {
                            IsLive = false;
                        }
                        if (!IsLive)
                        {
                            m_FirstFrame = view.FirstFrame;
                            m_SelectionFirst = view.SelectionFirst;
                            m_SelectionCount = (int)view.SelectionCount;
                        }
                    }
                    else
                    {
                        ViewResult = result;
                        // Busy preserves the previous complete view; an explicit request is retried by polling.
                        if (result != CaptureResult.Busy)
                        {
                            m_ViewDirty = false;
                        }
                    }
                }
                Changed?.Invoke();
            }
            catch (DllNotFoundException exception)
            {
                m_NativeAvailable = false;
                LoadError = exception.Message;
                View = default;
                Info = new CaptureInfo { State = CaptureState.Unavailable, Error = CaptureResult.Unavailable };
                Changed?.Invoke();
            }
            catch (EntryPointNotFoundException exception)
            {
                m_NativeAvailable = false;
                LoadError = exception.Message;
                View = default;
                Info = new CaptureInfo { State = CaptureState.Unavailable, Error = CaptureResult.Unavailable };
                Changed?.Invoke();
            }
        }

    }
}
