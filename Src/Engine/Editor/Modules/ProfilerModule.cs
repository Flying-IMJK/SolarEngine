using System;
using System.Diagnostics;
using CaptureInfo = SE.ProfilerCaptureInfo;
using CaptureState = SE.ProfilerCaptureState;
using CaptureResult = SE.ProfilerCaptureResult;

namespace SE.Editor
{
    /// <summary>Owns the local capture session and schedules the active page on the editor main thread.</summary>
    public sealed class ProfilerModule : EditorModule
    {
        public const int MaxViewFrames = 256;
        private readonly ProfilerMode[] m_Modes;
        private long m_LastRefresh;
        private bool m_NativeAvailable = true;
        private bool m_WasViewVisible;

        public ProfilerModule(Editor editor) : base(editor)
        {
            Cpu = new ProfilerCpuMode(this);
            Gpu = new ProfilerGpuMode(this);
            Memory = new ProfilerMemoryMode(this);
            m_Modes = new ProfilerMode[] { Cpu, Gpu, Memory };
            ActiveMode = Cpu;
        }

        internal override int Order => -85;
        internal bool NativeAvailable => m_NativeAvailable;
        public int HistoryCapacity => m_NativeAvailable ? Profiling.GetHistoryCapacity() : 0;
        public ulong OldestFrame => Info.OldestFrame;
        public ProfilerCpuMode Cpu { get; }
        public ProfilerGpuMode Gpu { get; }
        public ProfilerMemoryMode Memory { get; }
        public ReadOnlySpan<ProfilerMode> Modes => m_Modes;
        public ProfilerMode ActiveMode { get; private set; }
        public CaptureInfo Info { get; private set; }
        public bool IsLive { get; private set; } = true;
        public bool IsViewPending => ActiveMode.Dirty && (Info.State == CaptureState.Capturing || Info.State == CaptureState.Stopped);
        /// <summary>The last capture control result; automatic view polling cannot overwrite it.</summary>
        public CaptureResult LastResult { get; private set; }
        public string LoadError { get; private set; } = string.Empty;
        public event Action Changed;

        public string MenuLabel => Info.State switch
        {
            CaptureState.Capturing => "Profiler (REC)",
            CaptureState.Stopped => "Profiler (stopped)",
            _ => "Profiler",
        };

        public override void OnInit()
        {
            Refresh(false);
        }

        public override void OnUpdate()
        {
            long now = Stopwatch.GetTimestamp();
            if (now - m_LastRefresh < Stopwatch.Frequency / 10)
            {
                return;
            }
            m_LastRefresh = now;
            bool viewVisible = Editor.Windows.Profiler != null && Editor.Windows.Profiler.VisibleInHierarchy;
            if (viewVisible && !m_WasViewVisible && IsLive)
            {
                ActiveMode.Dirty = true;
            }
            m_WasViewVisible = viewVisible;
            if (IsViewPending || Info.GpuPending > 0 || Info.State == CaptureState.Capturing)
            {
                Refresh(viewVisible);
            }
            if (m_NativeAvailable && Info.State == CaptureState.Capturing)
            {
                // Capture continues independently of the active page and window visibility.
                Memory.SubmitSample();
            }
        }

        public CaptureResult Start()
        {
            LastResult = m_NativeAvailable ? Profiling.StartCapture() : CaptureResult.Unavailable;
            if (LastResult == CaptureResult.Ok)
            {
                IsLive = true;
                ResetModes();
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
                ResetModes();
            }
            Refresh();
            return LastResult;
        }

        /// <summary>Displays a page's owned copies. Switching a paused page does not read an expired native snapshot.</summary>
        public void SelectMode(ProfilerMode mode)
        {
            if (mode == ActiveMode)
            {
                return;
            }
            if (Array.IndexOf(m_Modes, mode) < 0)
            {
                throw new ArgumentException("The profiler mode does not belong to this module.", nameof(mode));
            }
            ActiveMode = mode;
            if (IsLive || !mode.HasSnapshot)
            {
                mode.Dirty = true;
            }
            else if (!mode.Dirty)
            {
                // A page last displayed in Live still needs its cached query position frozen on return.
                mode.Freeze();
            }
            Refresh();
        }

        /// <summary>Freezes the active page while native capture and state polling continue.</summary>
        public void Pause()
        {
            IsLive = false;
            ActiveMode.Freeze();
            NotifyChanged();
        }

        public void ShowLatestFrame()
        {
            IsLive = Info.State == CaptureState.Capturing;
            ActiveMode.ShowLatest();
            Refresh();
        }

        public void RefreshView()
        {
            ActiveMode.Dirty = true;
            Refresh();
        }

        internal void OnWindowShown()
        {
            if (IsLive || !ActiveMode.HasSnapshot)
            {
                RefreshView();
            }
        }

        internal void NotifyChanged()
        {
            ActiveMode.UpdateView();
            Changed?.Invoke();
        }

        private void ResetModes()
        {
            foreach (ProfilerMode mode in m_Modes)
            {
                mode.Reset();
            }
        }

        private void Refresh(bool allowLiveView = true)
        {
            if (!m_NativeAvailable)
            {
                return;
            }
            try
            {
                ProfilerMode mode = ActiveMode;
                bool query = (mode.Dirty || (IsLive && allowLiveView)) &&
                    (Info.State == CaptureState.Capturing || Info.State == CaptureState.Stopped);
                CaptureInfo info;
                CaptureResult result = query ? mode.Query(out info) : Profiling.GetCaptureInfo(out info);
                bool changedSession = Info.SessionId != info.SessionId;
                Info = info;
                if (changedSession)
                {
                    ResetModes();
                }
                else if (query)
                {
                    mode.ViewResult = result;
                    mode.Dirty = result == CaptureResult.Busy;
                    if (result == CaptureResult.Ok)
                    {
                        if (mode.IsFinal)
                        {
                            IsLive = false;
                        }
                    }
                    else if (!mode.Dirty)
                    {
                        // Keep the last owned view; an invalid/oversized request requires an explicit retry.
                        IsLive = false;
                    }
                }
                else if (result != CaptureResult.Ok)
                {
                    mode.ViewResult = result;
                }
                if (!query && IsLive && info.State == CaptureState.Stopped)
                {
                    mode.Dirty = true;
                }
                NotifyChanged();
            }
            catch (DllNotFoundException exception)
            {
                m_NativeAvailable = false;
                LoadError = exception.Message;
                ResetModes();
                Info = new CaptureInfo { State = CaptureState.Unavailable, Error = CaptureResult.Unavailable };
                NotifyChanged();
            }
            catch (EntryPointNotFoundException exception)
            {
                m_NativeAvailable = false;
                LoadError = exception.Message;
                ResetModes();
                Info = new CaptureInfo { State = CaptureState.Unavailable, Error = CaptureResult.Unavailable };
                NotifyChanged();
            }
        }

        public override void OnDispose()
        {
            if (m_NativeAvailable && Info.State == CaptureState.Capturing)
            {
                Profiling.StopCapture();
            }
            Changed = null;
            foreach (ProfilerMode mode in m_Modes)
            {
                mode.DetachControl();
                mode.DisposeControl();
            }
        }
    }
}
