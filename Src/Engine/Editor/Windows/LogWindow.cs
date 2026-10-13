using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
using System.Text.RegularExpressions;
using SE.Editor.GUI;
using SE.GUI;
using SE.Log;

namespace SE.Editor
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
            public string Description;
            public Guid ContextObject;
            public string LocationFile;
            public int LocationLine;
        }

        private const float ToolStripHeight = 22.0f;

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
            : base(editor, true, ScrollBars.None)
        {
            Title = "Log";
            Icon = EditorIcons.LogWindow32;


            m_ToolStrip = new ToolStrip(ToolStripHeight, 0.0f);
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
            m_Split.Panel1.SetScrollBars(SE.GUI.ScrollBars.Vertical);
            m_Split.Panel2.SetScrollBars(SE.GUI.ScrollBars.Both);

            m_LogInfo = new Label
            {
                HorizontalAlignment = TextAlignment.Near,
                VerticalAlignment = TextAlignment.Near,
                Wrapping = TextWrapping.WrapWords,
                TextColor = Style.Current.Foreground,
            };
            m_Split.Panel2.AddChild(m_LogInfo);

            m_EntriesPanel = new VerticalPanel
            {
                Margin = new Margin(0.0f),
                Spacing = 0.0f,
                AutoSize = false,
                IsScrollable = true,
            };
            m_Split.Panel1.AddChild(m_EntriesPanel);

            UpdateCount();
            Debug.Logger.LogHandler.SendLog += LogHandlerOnSendLog;
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
                {
                    return;
                }

                pendingEntries = new List<LogEntryDescription>(m_PendingEntries);
                m_PendingEntries.Clear();
            }

            Panel panelScroll = m_Split.Panel1;
            VScrollBar verticalScrollBar = panelScroll.vScrollBar;
            float maximumScrollOffsetY = 0.0f;
            if (verticalScrollBar != null)
            {
                maximumScrollOffsetY = verticalScrollBar.Maximum;
            }
            bool scrollView = maximumScrollOffsetY + panelScroll.GetViewOffset().Y < LogEntry.DefaultHeight * 1.5f;
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
                verticalScrollBar = panelScroll.vScrollBar;
                maximumScrollOffsetY = 0.0f;
                if (verticalScrollBar != null)
                {
                    maximumScrollOffsetY = verticalScrollBar.Maximum;
                }
                if (maximumScrollOffsetY + panelScroll.GetViewOffset().Y >= LogEntry.DefaultHeight * 1.5f)
                {
                    panelScroll.ScrollViewTo(new Float2(float.MaxValue, float.MaxValue));
                }
            }
        }

        public override void OnExit()
        {
            Debug.Logger.LogHandler.SendLog -= LogHandlerOnSendLog;
        }

        public override void OnStartContainsFocus()
        {
            // TODO: reset the dock tab icon once the managed EditorIcons/Icon API is exposed.
            base.OnStartContainsFocus();
        }

        protected override void PerformLayoutBeforeChildren()
        {
            base.PerformLayoutBeforeChildren();
            if (m_ToolStrip == null || m_Split == null || m_LogInfo == null || m_EntriesPanel == null)
            {
                return;
            }

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
                if (m_Selected == value)
                {
                    return;
                }

                m_Selected = value;
                m_LogInfo.Text = m_Selected != null ? m_Selected.GetInfo() : string.Empty;
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
                    if (string.Equals(existing.Desc.LocationFile, desc.LocationFile, StringComparison.Ordinal) &&
                        existing.Desc.Level == desc.Level &&
                        string.Equals(existing.Desc.Description, desc.Description, StringComparison.Ordinal) &&
                        existing.Desc.LocationLine == desc.LocationLine)
                    {
                        existing.LogCount++;
                        existing.Visible = m_GroupButtons[(int)existing.Group].Checked;
                        return existing;
                    }
                }
            }

            LogEntry entry = new(this, desc)
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
                {
                    y += LogEntry.DefaultHeight;
                }
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
                {
                    entry.Visible = isVisible;
                }
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
            {
                text += "s";
            }
            m_GroupButtons[group].Text = m_LogCountPerGroup[group] + text;
        }

        private void LogHandlerOnSendLog(LogType level, string message, SE.Object context, string stackTrace)
        {
            LogEntryDescription desc = new()
            {
                Level = level,
                Description = message ?? string.Empty,
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
            {
                return line;
            }
            return int.TryParse(match.Groups[5].Value, out line) ? line : 0;
        }

        private class LogEntry : Control
        {
            private bool _isRightMouseDown;
            private LogWindow _window;

            /// <summary>
            /// The default height of the entries.
            /// </summary>
            public const float DefaultHeight = 32.0f;

            public LogGroup Group;
            public LogEntryDescription Desc;
            public SpriteHandle Icon;
            public int LogCount = 1;

            public LogEntry(LogWindow window, LogEntryDescription desc)
                : base(0.0f, 0.0f, 120.0f, DefaultHeight)
            {
                AnchorPreset = AnchorPresets.HorizontalStretchTop;
                IsScrollable = true;
                AutoFocus = true;

                _window = window;
                Desc = desc;
                switch (desc.Level)
                {
                    case LogType.Warning:
                        Group = LogGroup.Warning;
                        break;
                    case LogType.Info:
                        Group = LogGroup.Info;
                        break;
                    default:
                        Group = LogGroup.Error;
                        break;
                }
            }

            /// <summary>
            /// Gets the information text.
            /// </summary>
            public string GetInfo()
            {
                return Desc.Description;
            }

            /// <inheritdoc />
            public override void Draw()
            {
                base.Draw();

                // Cache data
                Style style = Style.Current;
                int index = IndexInParent;
                Rectangle clientRect = new(Float2.Zero, Size);

                // Background
                if (_window.Selected == this)
                {
                    Render2D.FillRectangle(clientRect, IsFocused ? style.BackgroundSelected : style.LightBackground);
                }
                else if (IsMouseOver)
                {
                    Render2D.FillRectangle(clientRect, style.BackgroundHighlighted);
                }
                else if (index % 2 == 0)
                {
                    Render2D.FillRectangle(clientRect, style.Background * 0.9f);
                }

                // The precolored status PNGs retain their colors with a white tint.
                Color iconColor = default;
                SpriteHandle icon = default;
                switch (Group)
                {
                    case LogGroup.Error:
                        iconColor = Color.White;
                        icon = EditorIcons.Error32;
                        break;
                    case LogGroup.Warning:
                        iconColor = Color.White;
                        icon = EditorIcons.Warning32;
                        break;
                    case LogGroup.Info:
                        iconColor = Color.White;
                        icon = EditorIcons.Info32;
                        break;
                }
                Render2D.DrawSprite(icon, new Rectangle(5, 0, 32, 32), iconColor);

                // Title
                Rectangle textRect = new(38, 0, clientRect.Width - 40, clientRect.Height - 10);
                Render2D.PushClip(clientRect);

                string text = string.Empty;
                if (LogCount == 1)
                {
                    text = Desc.Description;
                }
                else if (LogCount > 1)
                {
                    text = string.Format("{0} ({1})", Desc.Description, LogCount);
                }

                Render2D.RenderText(style.FontMedium, text, textRect, style.TextColor, TextAlignment.Near, TextAlignment.Center);

                Render2D.PopClip();
            }

            /// <inheritdoc />
            public override void OnGetFocus()
            {
                base.OnGetFocus();

                _window.Selected = this;
            }

            /// <inheritdoc />
            public override bool OnKeyDown(KeyboardKeys key)
            {
                // InputOptions options = FlaxEditor.Editor.Instance.Options.Options.Input;

                // Up
                if (key == KeyboardKeys.ArrowUp)
                {
                    int index = IndexInParent - 1;
                    if (index >= 0)
                    {
                        Control target = Parent.GetChild(index);
                        target.Focus();
                        Panel panel = (Panel)Parent.Parent;
                        panel.ScrollViewTo(target);
                        return true;
                    }
                }
                // Down
                else if (key == KeyboardKeys.ArrowDown)
                {
                    int index = IndexInParent + 1;
                    if (index < Parent.ChildrenCount)
                    {
                        Control target = Parent.GetChild(index);
                        target.Focus();
                        Panel panel = (Panel)Parent.Parent;
                        panel.ScrollViewTo(target);
                        return true;
                    }
                }
                // Enter
                else if (key == KeyboardKeys.Return)
                {
                    Open();
                }

                /*// Ctrl+C
                else if (options.Copy.Process(this))
                {
                    Copy();
                    return true;
                }*/

                return base.OnKeyDown(key);
            }

            /// <summary>
            /// Opens the entry location.
            /// </summary>
            public void Open()
            {
                /*if (!string.IsNullOrEmpty(Desc.LocationFile) && File.Exists(Desc.LocationFile))
                {
                    Editor.Instance.CodeEditing.OpenFile(Desc.LocationFile, Desc.LocationLine);
                }*/
            }

            /// <summary>
            /// Copies the entry information to the system clipboard (as text).
            /// </summary>
            public void Copy()
            {
                // Clipboard.Text = GetInfo().Replace("\r\n", "\n").Replace("\n", Environment.NewLine);
            }

            public override bool OnMouseDoubleClick(Float2 location, MouseButton button)
            {
                Open();
                return true;
            }

            /// <inheritdoc />
            public override bool OnMouseDown(Float2 location, MouseButton button)
            {
                if (base.OnMouseDown(location, button))
                {
                    return true;
                }

                if (button == MouseButton.Left)
                {
                    Focus();
                    return true;
                }

                if (button == MouseButton.Right)
                {
                    Focus();

                    _isRightMouseDown = true;
                    return true;
                }

                return false;
            }

            /// <inheritdoc />
            public override bool OnMouseUp(Float2 location, MouseButton button)
            {
                if (base.OnMouseUp(location, button))
                {
                    return true;
                }

                if (_isRightMouseDown && button == MouseButton.Right)
                {
                    Focus();

                    ContextMenu menu = new();
                    menu.AddButton("Copy", Copy);
                    menu.AddButton("Open", Open).Enabled = Desc.LocationFile.Length > 0 && File.Exists(Desc.LocationFile);
                    menu.Show(this, location);
                }

                return false;
            }

            /// <inheritdoc />
            public override void OnMouseLeave()
            {
                _isRightMouseDown = false;

                base.OnMouseLeave();
            }

            /// <inheritdoc />
            protected override void OnVisibleChanged()
            {
                // Deselect on hide
                if (!Visible && _window.Selected == this)
                {
                    _window.Selected = null;
                }

                base.OnVisibleChanged();
            }
        }
    }
}
