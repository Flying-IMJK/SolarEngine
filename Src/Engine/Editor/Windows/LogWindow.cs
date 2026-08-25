using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
using System.Text.RegularExpressions;
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
        private enum LogGroup
        {
            Error = 0,
            Warning,
            Info,
            Max,
        }

        public struct LogEntryDescription
        {
            public LogType Level;
            public string Title;
            public string Description;
            public Guid ContextObject;
            public string LocationFile;
            public int LocationLine;
        }

        private const float ToolStripHeight = 22.0f;
        private const float MarkerWidth = 4.0f;

        private readonly ToolStrip m_ToolStrip;
        private readonly ToolStripButton m_ClearOnPlayButton;
        private readonly ToolStripButton m_CollapseLogsButton;
        private readonly ToolStripButton m_PauseOnErrorButton;
        private readonly ToolStripButton[] m_GroupButtons = new ToolStripButton[3];
        private readonly SplitPanel m_Split;
        private readonly Label m_LogInfo;
        private readonly VerticalPanel m_EntriesPanel;
        private readonly List<LogEntry> m_Entries = new();
        private readonly List<LogEntryDescription> m_PendingEntries = new();
        private readonly object m_PendingEntriesLock = new();
        private readonly int[] m_LogCountPerGroup = new int[(int)LogGroup.Max];
        private readonly Regex m_LogRegex = new("at (.*) in (.*):(line (\\d*)|(\\d*))", RegexOptions.Compiled);
        private LogEntry? m_Selected;

        public LogWindow(Editor editor)
            : base(editor, "Log", "Log", ScrollBars.None)
        {
            m_ToolStrip = new ToolStrip(ToolStripHeight, 0.0f, Width);
            AddChild(m_ToolStrip);
            m_ToolStrip.AddButton("Clear", Clear).TooltipText = "Clears all log entries";
            m_ClearOnPlayButton = m_ToolStrip.AddButton("Clear on Play").SetAutoCheck(true).SetChecked(true);
            m_ClearOnPlayButton.TooltipText = "Clears all log entries on enter playmode";
            m_CollapseLogsButton = m_ToolStrip.AddButton("Collapse").SetAutoCheck(true).SetChecked(true);
            m_CollapseLogsButton.TooltipText = "Collapses similar logs";
            m_PauseOnErrorButton = m_ToolStrip.AddButton("Pause on Error").SetAutoCheck(true);
            m_PauseOnErrorButton.TooltipText = "Performs auto pause on error";
            m_ToolStrip.AddSeparator();
            m_GroupButtons[(int)LogGroup.Error] = m_ToolStrip.AddButton("0 Errors", () => UpdateLogTypeVisibility(LogGroup.Error, m_GroupButtons[(int)LogGroup.Error].Checked)).SetAutoCheck(true).SetChecked(true);
            m_GroupButtons[(int)LogGroup.Error].TooltipText = "Shows/hides error messages";
            m_GroupButtons[(int)LogGroup.Warning] = m_ToolStrip.AddButton("0 Warnings", () => UpdateLogTypeVisibility(LogGroup.Warning, m_GroupButtons[(int)LogGroup.Warning].Checked)).SetAutoCheck(true).SetChecked(true);
            m_GroupButtons[(int)LogGroup.Warning].TooltipText = "Shows/hides warning messages";
            m_GroupButtons[(int)LogGroup.Info] = m_ToolStrip.AddButton("0 Messages", () => UpdateLogTypeVisibility(LogGroup.Info, m_GroupButtons[(int)LogGroup.Info].Checked)).SetAutoCheck(true).SetChecked(true);
            m_GroupButtons[(int)LogGroup.Info].TooltipText = "Shows/hides info messages";

            m_Split = new SplitPanel(Orientation.Vertical)
            {
                SplitterValue = 0.8f,
            };
            AddChild(m_Split);
            m_Split.Panel1.ScrollBars = SE.GUI.ScrollBars.Vertical;
            m_Split.Panel2.ScrollBars = SE.GUI.ScrollBars.Both;

            m_LogInfo = new Label
            {
                HorizontalAlignment = TextAlignment.Near,
                VerticalAlignment = TextAlignment.Near,
                TextWrapping = TextWrapping.WrapWords,
                TextColor = Style.Current.Foreground,
            };
            m_Split.Panel2.AddChild(m_LogInfo);

            m_EntriesPanel = new VerticalPanel
            {
                Margin = new Margin(0.0f),
                Spacing = 0.0f,
                AutoSize = false,
                ScrollBars = SE.GUI.ScrollBars.None,
                IsScrollable = true,
            };
            m_Split.Panel1.AddChild(m_EntriesPanel);

            UpdateCount();
            Debug.Logger.LogHandler.SendLog += LogHandlerOnSendLog;
            Debug.Logger.LogHandler.SendExceptionLog += LogHandlerOnSendExceptionLog;
        }

        public int EntryCount => m_Entries.Count;

        public void Clear()
        {
            Selected = null;
            for (int i = m_Entries.Count - 1; i >= 0; i--)
            {
                m_Entries[i].Dispose();
            }

            m_Entries.Clear();
            Array.Clear(m_LogCountPerGroup, 0, m_LogCountPerGroup.Length);
            lock (m_PendingEntriesLock)
            {
                m_PendingEntries.Clear();
            }
            m_EntriesPanel.Height = 0.0f;
            m_EntriesPanel.PerformLayout();
            UpdateCount();
        }

        public override void OnUpdate()
        {
            List<LogEntryDescription> pendingEntries;
            lock (m_PendingEntriesLock)
            {
                if (m_PendingEntries.Count == 0)
                    return;

                pendingEntries = new List<LogEntryDescription>(m_PendingEntries);
                m_PendingEntries.Clear();
            }

            Panel panelScroll = m_Split.Panel1;
            bool scrollView = panelScroll.MaximumScrollOffset.Y - panelScroll.ScrollOffset.Y < LogEntry.DefaultHeight * 1.5f;
            LogEntry? lastEntry = null;
            bool anyVisible = false;
            for (int i = 0; i < pendingEntries.Count; i++)
            {
                LogEntryDescription desc = pendingEntries[i];
                LogEntry entry = AddEntry(ref desc);
                lastEntry = entry;
                anyVisible |= entry.Visible;
            }

            UpdateCount();
            if (scrollView && anyVisible && lastEntry != null)
            {
                panelScroll.ScrollViewTo(lastEntry);
                if (panelScroll.MaximumScrollOffset.Y - panelScroll.ScrollOffset.Y >= LogEntry.DefaultHeight * 1.5f)
                    panelScroll.ScrollViewTo(new Float2(float.MaxValue, float.MaxValue));
            }
        }

        public override void OnExit()
        {
            Debug.Logger.LogHandler.SendLog -= LogHandlerOnSendLog;
            Debug.Logger.LogHandler.SendExceptionLog -= LogHandlerOnSendExceptionLog;
        }

        public override void OnStartContainsFocus()
        {
            // TODO: reset the dock tab icon once the managed EditorIcons/Icon API is exposed.
            base.OnStartContainsFocus();
        }

        protected override void OnLayoutChildren()
        {
            base.OnLayoutChildren();
            if (m_ToolStrip == null || m_Split == null || m_LogInfo == null || m_EntriesPanel == null)
                return;

            m_ToolStrip.SetBounds(0.0f, 0.0f, Width, ToolStripHeight);
            m_Split.SetBounds(0.0f, ToolStripHeight, Width, MathF.Max(0.0f, Height - ToolStripHeight));
            m_LogInfo.SetBounds(4.0f, 4.0f, MathF.Max(0.0f, m_Split.Panel2.Width - 8.0f), MathF.Max(0.0f, m_Split.Panel2.Height - 8.0f));
            LayoutEntries(m_Split.Panel1.Width);
        }

        private LogEntry? Selected
        {
            get => m_Selected;
            set
            {
                if (ReferenceEquals(m_Selected, value))
                    return;

                m_Selected = value;
                m_LogInfo.Text = m_Selected?.Info ?? string.Empty;
            }
        }

        private void Add(ref LogEntryDescription desc)
        {
            // TODO: apply editor timestamp preferences once the managed EditorOptions API is exposed.
            lock (m_PendingEntriesLock)
            {
                m_PendingEntries.Add(desc);
            }

            // TODO: update the dock tab icon and pause simulation on errors once the managed APIs exist:
            // Icon = Editor.Icons.Error32/Warning32/Info32;
            // if (entry.Group == LogGroup.Error && m_PauseOnErrorButton.Checked) Editor.Simulation.RequestPausePlay();
        }

        private LogEntry AddEntry(ref LogEntryDescription desc)
        {
            if (m_CollapseLogsButton.Checked)
            {
                for (int i = 0; i < m_Entries.Count; i++)
                {
                    LogEntry existing = m_Entries[i];
                    if (existing.Matches(ref desc))
                    {
                        existing.LogCount++;
                        existing.Visible = m_GroupButtons[(int)existing.Group].Checked;
                        return existing;
                    }
                }
            }

            LogEntry entry = new(this, ref desc)
            {
                Visible = m_GroupButtons[(int)LogGroupFromLogType(desc.Level)].Checked,
            };
            m_Entries.Add(entry);
            m_EntriesPanel.AddChild(entry);
            m_LogCountPerGroup[(int)entry.Group]++;
            LayoutEntries(m_Split.Panel1.Width);
            return entry;
        }

        private void LayoutEntries(float width)
        {
            float entryWidth = MathF.Max(1.0f, width);
            float y = 0.0f;
            for (int i = 0; i < m_Entries.Count; i++)
            {
                LogEntry entry = m_Entries[i];
                entry.SetBounds(0.0f, y, entryWidth, LogEntry.DefaultHeight);
                if (entry.Visible)
                    y += LogEntry.DefaultHeight;
            }
            m_EntriesPanel.SetBounds(0.0f, 0.0f, entryWidth, y);
            m_EntriesPanel.PerformLayout();
        }

        private void UpdateLogTypeVisibility(LogGroup group, bool isVisible)
        {
            for (int i = 0; i < m_Entries.Count; i++)
            {
                LogEntry entry = m_Entries[i];
                if (entry.Group == group)
                    entry.Visible = isVisible;
            }

            LayoutEntries(m_Split.Panel1.Width);
        }

        private void UpdateCount()
        {
            UpdateCount((int)LogGroup.Error, " Error");
            UpdateCount((int)LogGroup.Warning, " Warning");
            UpdateCount((int)LogGroup.Info, " Message");
        }

        private void UpdateCount(int group, string text)
        {
            if (m_LogCountPerGroup[group] != 1)
                text += "s";
            m_GroupButtons[group].Text = m_LogCountPerGroup[group] + text;
        }

        private void LogHandlerOnSendLog(LogType level, string message, SE.Object context, string stackTrace)
        {
            LogEntryDescription desc = new()
            {
                Level = level,
                Title = message ?? string.Empty,
                Description = string.Empty,
                ContextObject = context?.ID ?? Guid.Empty,
                LocationFile = string.Empty,
            };

            if (!string.IsNullOrEmpty(stackTrace))
            {
                MatchCollection matches = m_LogRegex.Matches(stackTrace);
                bool foundStart = false;
                bool noLocation = true;
                StringBuilder stringBuilder = new();
                for (int i = 0; i < matches.Count; i++)
                {
                    Match match = matches[i];
                    string matchLocation = match.Groups[1].Value.Trim();
                    if (matchLocation.StartsWith("SE.Log.Debug", StringComparison.Ordinal) ||
                        matchLocation.StartsWith("SE.Debug", StringComparison.Ordinal))
                    {
                        foundStart = true;
                    }
                    else if (foundStart)
                    {
                        if (noLocation)
                        {
                            desc.LocationFile = match.Groups[2].Value;
                            desc.LocationLine = ParseLocationLine(match);
                            noLocation = false;
                        }
                        stringBuilder.AppendLine(match.Groups[0].Value);
                    }
                }

                desc.Description = stringBuilder.Length != 0 ? stringBuilder.ToString() : stackTrace;
            }

            Add(ref desc);
        }

        private void LogHandlerOnSendExceptionLog(Exception exception, SE.Object context)
        {
            LogEntryDescription desc = new()
            {
                Level = LogType.Error,
                Title = exception.Message,
                Description = exception.StackTrace ?? string.Empty,
                ContextObject = context?.ID ?? Guid.Empty,
                LocationFile = string.Empty,
            };

            if (!string.IsNullOrEmpty(exception.StackTrace))
            {
                Match match = m_LogRegex.Match(exception.StackTrace);
                if (match.Success)
                {
                    desc.LocationFile = match.Groups[2].Value;
                    desc.LocationLine = ParseLocationLine(match);
                }
            }

            Add(ref desc);
        }

        private static LogGroup LogGroupFromLogType(LogType level)
        {
            return level switch
            {
                LogType.Warning => LogGroup.Warning,
                LogType.Info => LogGroup.Info,
                _ => LogGroup.Error,
            };
        }

        private static int ParseLocationLine(Match match)
        {
            if (int.TryParse(match.Groups[4].Value, out int line))
                return line;
            return int.TryParse(match.Groups[5].Value, out line) ? line : 0;
        }

        private sealed class LogEntry : Control
        {
            public const float DefaultHeight = 24.0f;

            private readonly LogWindow m_Window;
            private bool m_IsRightMouseDown;

            public LogEntry(LogWindow window, ref LogEntryDescription desc)
                : base(0.0f, 0.0f, 120.0f, DefaultHeight)
            {
                m_Window = window;
                Desc = desc;
                Group = LogGroupFromLogType(desc.Level);
                IsScrollable = true;
                AutoFocus = true;
                TooltipText = desc.Description;
            }

            public LogGroup Group { get; }
            public LogEntryDescription Desc { get; }
            public int LogCount { get; set; } = 1;
            public string Info => string.IsNullOrEmpty(Desc.Description) ? Desc.Title : Desc.Title + Environment.NewLine + Desc.Description;

            public bool Matches(ref LogEntryDescription desc)
            {
                return Desc.Level == desc.Level &&
                       Desc.LocationLine == desc.LocationLine &&
                       string.Equals(Desc.Title, desc.Title, StringComparison.Ordinal) &&
                       string.Equals(Desc.Description, desc.Description, StringComparison.Ordinal) &&
                       string.Equals(Desc.LocationFile, desc.LocationFile, StringComparison.Ordinal);
            }

            public override void Draw()
            {
                base.Draw();

                Style style = Style.Current;
                Rectangle clientRect = ScreenBounds;
                int index = IndexInParent;
                if (ReferenceEquals(m_Window.Selected, this))
                {
                    Color selected = IsFocused ? style.BackgroundSelected : style.BackgroundNormal;
                    Render2D.FillRectangle(ref clientRect, ref selected);
                }
                else if (IsMouseOver)
                {
                    Color highlighted = style.BackgroundHighlighted;
                    Render2D.FillRectangle(ref clientRect, ref highlighted);
                }
                else if (index % 2 == 0)
                {
                    Color striped = style.Background * 0.9f;
                    Render2D.FillRectangle(ref clientRect, ref striped);
                }

                Color markerColor = Group switch
                {
                    LogGroup.Error => new Color(0.95f, 0.22f, 0.18f, 1.0f),
                    LogGroup.Warning => new Color(0.95f, 0.78f, 0.18f, 1.0f),
                    _ => style.Foreground,
                };
                Rectangle marker = new(clientRect.X, clientRect.Y, MarkerWidth, clientRect.Height);
                Render2D.FillRectangle(ref marker, ref markerColor);

                // TODO: replace the marker with EditorIcons Info/Warning/Error sprites
                // once managed EditorIcons are exposed:
                // Render2D.DrawSprite(ref icon, ref iconRect, ref markerColor);

                string title = LogCount <= 1 ? Desc.Title : $"{Desc.Title} ({LogCount})";
                Rectangle textRect = new(clientRect.X + 8.0f, clientRect.Y, MathF.Max(0.0f, clientRect.Width - 10.0f), clientRect.Height);
                Font? font = style.FontMedium;
                if (ReferenceEquals(font, null))
                    return;

                Color textColor = style.Foreground;
                Render2D.RenderText(font, title, ref textRect, ref textColor, TextAlignment.Near, TextAlignment.Center, TextWrapping.NoWrap);
            }

            public override void OnGetFocus()
            {
                base.OnGetFocus();
                m_Window.Selected = this;
            }

            public override bool OnKeyDown(KeyboardKeys key)
            {
                if (key == KeyboardKeys.ArrowUp)
                {
                    FocusSibling(-1);
                    return true;
                }

                if (key == KeyboardKeys.ArrowDown)
                {
                    FocusSibling(1);
                    return true;
                }

                if (key == KeyboardKeys.Return)
                {
                    Open();
                    return true;
                }

                // TODO: wire Ctrl+C through the managed input action API once it exists.
                return base.OnKeyDown(key);
            }

            public override bool OnMouseDoubleClick(Float2 location, MouseButton button)
            {
                Open();
                return true;
            }

            public override bool OnMouseDown(Float2 location, MouseButton button)
            {
                if (base.OnMouseDown(location, button))
                    return true;

                if (button == MouseButton.Left)
                {
                    Focus();
                    return true;
                }

                if (button == MouseButton.Right)
                {
                    Focus();
                    m_IsRightMouseDown = true;
                    return true;
                }

                return false;
            }

            public override bool OnMouseUp(Float2 location, MouseButton button)
            {
                if (base.OnMouseUp(location, button))
                    return true;

                if (m_IsRightMouseDown && button == MouseButton.Right)
                {
                    Focus();
                    ContextMenu menu = new();
                    menu.AddButton("Copy", Copy).Enabled = false;
                    menu.AddButton("Open", Open).Enabled = CanOpen();
                    menu.Show(this, location.X, location.Y);
                }

                return false;
            }

            public override void OnMouseLeave()
            {
                m_IsRightMouseDown = false;
                base.OnMouseLeave();
            }

            private void FocusSibling(int offset)
            {
                if (Parent == null)
                    return;

                int index = IndexInParent + offset;
                while (index >= 0 && index < Parent.ChildrenCount)
                {
                    if (Parent.GetChild(index) is LogEntry target && target.Visible)
                    {
                        target.Focus();
                        m_Window.m_Split.Panel1.ScrollViewTo(target);
                        return;
                    }
                    index += offset;
                }
            }

            private bool CanOpen()
            {
                return !string.IsNullOrEmpty(Desc.LocationFile) && File.Exists(Desc.LocationFile);
            }

            private void Open()
            {
                if (!CanOpen())
                    return;

                // TODO: call Editor.CodeEditing.OpenFile(Desc.LocationFile, Desc.LocationLine)
                // once the managed code-editing API is exposed.
            }

            private void Copy()
            {
                // TODO: use Clipboard.Text/Clipboard.SetText once a managed clipboard wrapper exists.
                // string text = Info.Replace("\r\n", "\n").Replace("\n", Environment.NewLine);
            }
        }
    }
}
