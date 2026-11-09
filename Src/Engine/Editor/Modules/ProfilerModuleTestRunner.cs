using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;
using System.Text.Json;

namespace SE.Editor
{
    /// <summary>
    /// Explicit, opt-in editor-frame test host. SOLAR_PROFILER_TEST_MODE is benchmark or integration;
    /// SOLAR_PROFILER_TEST_REPORT names the output JSON. Neither variable is set by normal editor startup.
    /// </summary>
    internal sealed partial class ProfilerModuleTestRunner
    {
        private readonly ProfilerModule m_Profiler;
        private readonly string m_Mode;
        private readonly string m_Report;
        private readonly Stopwatch m_Clock = Stopwatch.StartNew();
        private readonly HashSet<long> m_Seen = new HashSet<long>();
        private readonly List<double> m_Durations = new List<double>();
        private readonly List<double> m_QueryTimes = new List<double>();
        private readonly List<double> m_PulseTimes = new List<double>();
        private readonly List<double> m_EventPulseTimes = new List<double>();
        private readonly List<double> m_AllocationPulseTimes = new List<double>();
        private readonly List<double> m_CollectorTimes = new List<double>();
        private readonly List<double> m_PublishDelays = new List<double>();
        private readonly List<double> m_SubmitWaits = new List<double>();
        private readonly List<string> m_Steps = new List<string>();
        private readonly List<object> m_RetentionCheckpoints = new List<object>();
        private ulong m_CheckpointSession;
        private ulong m_CheckpointWindow;
        private int m_MaxPending;
        private ulong m_MaxDropped;
        private IEnumerator<string> m_Tests;
        private int m_Ticks;
        private bool m_Finished;

        [LibraryImport("SEEditor", EntryPoint = "SEProfilerTestPulse")]
        private static partial void Pulse(int events, int allocations);

        [LibraryImport("SEEditor", EntryPoint = "SEProfilerTestPulseTimes")]
        private static partial void PulseTimes(out double events, out double allocations);

        [LibraryImport("SEEditor", EntryPoint = "SEProfilerTestRequestExit")]
        private static partial void RequestExit(int exitCode);

        private ProfilerModuleTestRunner(ProfilerModule profiler, string mode, string report)
        {
            m_Profiler = profiler;
            m_Mode = mode;
            m_Report = report;
        }

        internal static ProfilerModuleTestRunner TryCreate(ProfilerModule profiler)
        {
            string mode = Environment.GetEnvironmentVariable("SOLAR_PROFILER_TEST_MODE");
            string report = Environment.GetEnvironmentVariable("SOLAR_PROFILER_TEST_REPORT");
            return (mode == "benchmark" || mode == "integration") && !string.IsNullOrEmpty(report) ?
                new ProfilerModuleTestRunner(profiler, mode, report) : null;
        }

        internal void Tick()
        {
            if (m_Finished)
            {
                return;
            }
            try
            {
                if (m_Clock.Elapsed.TotalSeconds > (m_Mode == "integration" ? 600 : 180))
                {
                    throw new TimeoutException("Profiler test timed out at " + (m_Steps.Count == 0 ? "startup" : m_Steps[m_Steps.Count - 1]));
                }
                if (++m_Ticks < 32)
                {
                    return;
                }
                if (m_Mode == "integration")
                {
                    m_Tests ??= ProfilerModuleIntegrationTests.Run(m_Profiler).GetEnumerator();
                    if (!m_Tests.MoveNext())
                    {
                        Finish(true, "Module integration checks completed");
                    }
                    else if (m_Steps.Count == 0 || m_Steps[m_Steps.Count - 1] != m_Tests.Current)
                    {
                        m_Steps.Add(m_Tests.Current);
                        File.AppendAllText(m_Report + ".progress.log", m_Clock.Elapsed.TotalSeconds.ToString("0.00") + " " + m_Tests.Current + Environment.NewLine);
                    }
                    RecordDiagnostics();
                    return;
                }
                if (m_Ticks == 32)
                {
                    if (m_Profiler.Start() != ProfilerCaptureResult.Ok)
                    {
                        throw new InvalidOperationException("Could not start benchmark capture.");
                    }
                }
                long begin = Stopwatch.GetTimestamp();
                Pulse(96, 16);
                m_PulseTimes.Add((Stopwatch.GetTimestamp() - begin) * 1000.0 / Stopwatch.Frequency);
                PulseTimes(out double eventTime, out double allocationTime);
                m_EventPulseTimes.Add(eventTime);
                m_AllocationPulseTimes.Add(allocationTime);
                RecordDiagnostics();
                if (m_Ticks % 16 == 0)
                {
                    begin = Stopwatch.GetTimestamp();
                    m_Profiler.RefreshView();
                    ulong last = m_Profiler.Info.CompleteFrames;
                    if (last > 0)
                    {
                        ulong first = Math.Max(m_Profiler.OldestFrame, last > 24 ? last - 24 : 0);
                        m_Profiler.SelectFrames(first, last - 1);
                        foreach (ProfilerCpuEvent item in m_Profiler.CpuEvents)
                        {
                            if (item.EndValid != 0 && m_Profiler.GetCpuText(item.NameOffset) == "SE::ProfilingSystem::OnUpdate" && m_Seen.Add(item.BeginNs))
                            {
                                m_Durations.Add((item.EndNs - item.BeginNs) / 1_000_000.0);
                            }
                        }
                    }
                    m_QueryTimes.Add((Stopwatch.GetTimestamp() - begin) * 1000.0 / Stopwatch.Frequency);
                }
                if (m_Ticks >= 272)
                {
                    if (m_Durations.Count < 32)
                    {
                        throw new InvalidOperationException("Insufficient native OnUpdate measurements: " + m_Durations.Count);
                    }
                    Finish(true, "Native OnUpdate benchmark completed");
                }
            }
            catch (Exception exception)
            {
                Finish(false, exception.ToString());
            }
        }

        private void RecordDiagnostics()
        {
            ProfilerCaptureInfo info = m_Profiler.Info;
            m_MaxPending = Math.Max(m_MaxPending, info.PendingRequests);
            m_MaxDropped = Math.Max(m_MaxDropped, info.DroppedRequests);
            m_CollectorTimes.Add(info.CollectMs);
            m_PublishDelays.Add(info.PublishDelayMs);
            m_SubmitWaits.Add(info.SubmitWaitMs);
            ulong window = info.CompleteFrames / (ulong)Math.Max(1, m_Profiler.HistoryCapacity);
            if (info.SessionId != m_CheckpointSession || window != m_CheckpointWindow)
            {
                m_CheckpointSession = info.SessionId;
                m_CheckpointWindow = window;
                m_RetentionCheckpoints.Add(new
                {
                    seconds = m_Clock.Elapsed.TotalSeconds, session = info.SessionId, frames = info.CompleteFrames,
                    oldest = info.OldestFrame,
                    pending = info.PendingRequests, dropped = info.DroppedRequests
                });
            }
        }

        private static double Median(List<double> values)
        {
            values.Sort();
            return values.Count == 0 ? 0 : values[values.Count / 2];
        }

        private void Finish(bool passed, string message)
        {
            m_Finished = true;
            m_Tests?.Dispose();
            if (m_Profiler.Info.State == ProfilerCaptureState.Capturing)
            {
                m_Profiler.Stop();
            }
            RecordDiagnostics();
            m_Durations.Sort();
            double Percentile(double fraction) => m_Durations.Count == 0 ? 0 : m_Durations[(int)((m_Durations.Count - 1) * fraction)];
            File.WriteAllText(m_Report, JsonSerializer.Serialize(new
            {
                passed, coverageComplete = !m_Steps.Exists(step => step.StartsWith("Not covered:")),
                message, mode = m_Mode, ticks = m_Ticks, seconds = m_Clock.Elapsed.TotalSeconds,
                measurements = m_Durations.Count, onUpdateP50Ms = Percentile(0.5), onUpdateP95Ms = Percentile(0.95),
                onUpdateMaxMs = Percentile(1), samples = m_Durations, steps = m_Steps,
                queryMedianMs = Median(m_QueryTimes), pulseMedianMs = Median(m_PulseTimes), collectorMedianMs = Median(m_CollectorTimes),
                eventPulseMedianMs = Median(m_EventPulseTimes), allocationPulseMedianMs = Median(m_AllocationPulseTimes),
                publishDelayMedianMs = Median(m_PublishDelays), submitWaitMedianMs = Median(m_SubmitWaits),
                maxPending = m_MaxPending,
                droppedRequests = m_MaxDropped, retentionCheckpoints = m_RetentionCheckpoints
            }, new JsonSerializerOptions { WriteIndented = true }));
            RequestExit(passed ? 0 : 2);
        }
    }
}
