using System;
using System.Collections.Generic;
using SE.GUI;
using SE.Log;

namespace SE.Editor.GUI
{
    /// <summary>
    /// Managed editor log window. It follows Flax DebugLogWindow's ownership
    /// model and receives managed Debug logger messages on the editor update.
    /// </summary>
    public sealed class LogWindow : EditorWindow
    {
        private const int MaxEntries = 512;

        private readonly Panel m_EntriesPanel;
        private readonly List<LogEntry> m_Entries = new();
        private readonly Queue<LogEntryData> m_PendingEntries = new();
        private readonly object m_PendingEntriesLock = new();

        public LogWindow(Editor editor)
            : base(editor, "Log", "Log", ScrollBars.None)
        {
            m_EntriesPanel = AddChild(new Panel());
            Debug.Logger.LogHandler.SendLog += OnLog;
        }

        public int EntryCount => m_Entries.Count;

        public void Clear()
        {
            foreach (LogEntry entry in m_Entries)
            {
                entry.Dispose();
            }

            m_Entries.Clear();
            m_EntriesPanel.PerformLayout();
        }

        public override void OnUpdate()
        {
            while (true)
            {
                LogEntryData entry;
                lock (m_PendingEntriesLock)
                {
                    if (m_PendingEntries.Count == 0)
                        break;
                    entry = m_PendingEntries.Dequeue();
                }
                AddEntry(entry);
            }
        }

        public override void OnExit()
        {
            Debug.Logger.LogHandler.SendLog -= OnLog;
        }

        protected override void OnLayoutChildren()
        {
            base.OnLayoutChildren();
            m_EntriesPanel.SetBounds(0.0f, 0.0f, Width, Height);
            LayoutEntries();
        }

        private void OnLog(LogType level, string message, SE.Object context, string stackTrace)
        {
            _ = context;
            lock (m_PendingEntriesLock)
                m_PendingEntries.Enqueue(new LogEntryData(level, message, stackTrace));
        }

        private void AddEntry(LogEntryData entry)
        {
            if (m_Entries.Count == MaxEntries)
            {
                LogEntry removed = m_Entries[0];
                m_Entries.RemoveAt(0);
                removed.Dispose();
            }

            LogEntry control = m_EntriesPanel.AddChild(new LogEntry(entry));
            m_Entries.Add(control);
            LayoutEntries();
        }

        private void LayoutEntries()
        {
            float y = 0.0f;
            foreach (LogEntry entry in m_Entries)
            {
                entry.SetBounds(0.0f, y, Math.Max(1.0f, m_EntriesPanel.Width), LogEntry.DefaultHeight);
                y += LogEntry.DefaultHeight;
            }
            m_EntriesPanel.ScrollMargin = new Margin(0.0f, 0.0f, 0.0f, y);
        }

        private readonly struct LogEntryData
        {
            public LogEntryData(LogType level, string message, string stackTrace)
            {
                Level = level;
                Message = message ?? string.Empty;
                StackTrace = stackTrace ?? string.Empty;
            }

            public LogType Level { get; }
            public string Message { get; }
            public string StackTrace { get; }
        }

        private sealed class LogEntry : Label
        {
            public const float DefaultHeight = 24.0f;

            public LogEntry(LogEntryData entry)
                : base(new Rectangle(0.0f, 0.0f, 320.0f, DefaultHeight), entry.Message)
            {
                TooltipText = entry.StackTrace;
                TextColor = entry.Level switch
                {
                    LogType.Warning => Color.Yellow,
                    LogType.Error or LogType.Fatal => Color.Red,
                    _ => Style.Current.Foreground,
                };
                VerticalAlignment = TextAlignment.Center;
            }
        }
    }
}
