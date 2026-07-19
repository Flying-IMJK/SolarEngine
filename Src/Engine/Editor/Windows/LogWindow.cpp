
#include "LogWindow.h"

#include "Editor/EditorIcons.h"
#include "Editor/GUI/ContextMenu/ContextMenu.h"
#include "Editor/GUI/ToolStrip.h"
#include "Editor/GUI/ToolStripButton.h"
#include "Editor/GUI/ContextMenu/ContextMenuButton.h"
#include "Runtime/Render/2D/Render2D.h"
#include "Runtime/UI/GUI/Style.h"
#include "Runtime/UI/GUI/Common/Label.h"
#include "Runtime/UI/GUI/Panels/SplitPanel.h"
#include "Runtime/UI/GUI/Panels/VerticalPanel.h"
#include "Runtime/UI/GUI/Panels/Panel.h"
#include "Runtime/Core/Thread/Threading.h"
#include "Runtime/Core/Logging/LoggingSystem.h"
#include "Runtime/Core/Platform/FileSystem.h"
#include "Editor/EditorApp.h"

namespace SE::Editor
{
    LogWindow::LogEntry::LogEntry(LogWindow* window, LogEntryDescription desc) : Control(0, 0, 120, DefaultHeight)
    {
        AnchorPreset = AnchorPresets::HorizontalStretchTop;
        IsScrollable = true;
        AutoFocus = true;

        _window = window;
        Desc = desc;
        switch (desc.Level)
        {
        case Log::Severity::Warning:
            Group = LogGroup::Warning;
            break;
        case Log::Severity::Info:
            Group = LogGroup::Info;
            break;
        default:
            Group = LogGroup::Error;
            break;
        }
    }

    String LogWindow::LogEntry::GetInfo()
    {
        return Desc.Description;
    }

    void LogWindow::LogEntry::Draw()
    {
        Control::Draw();

        // Cache data
        Style* style = Style::Current;
        int index = IndexInParent;
        Rectangle clientRect = Rectangle(Float2::Zero, Size);

        // Background
        if (_window->_selected == this)
            Render2D::FillRectangle(clientRect, IsFocused ? style->BackgroundSelected : style->LightBackground);
        else if (IsMouseOver)
            Render2D::FillRectangle(clientRect, style->BackgroundHighlighted);
        else if (index % 2 == 0)
            Render2D::FillRectangle(clientRect, style->Background * 0.9f);

        // Icon
        Color iconColor;
        SpriteHandle icon;
        switch (Group)
        {
        case LogGroup::Error:
            iconColor = Colors::Red;
            icon = EditorIcons::Error32;
            break;
        case LogGroup::Warning:
            iconColor = Colors::Yellow;
            icon = EditorIcons::Warning32;
            break;
        case LogGroup::Info:
            iconColor = style->Foreground;
            icon = EditorIcons::Info32;
            break;
        }
        Render2D::DrawSprite(icon, Rectangle(5, 0, 32, 32), iconColor);

        // Title
        Rectangle textRect = Rectangle(38, 0, clientRect.GetWidth() - 40, clientRect.GetHeight() - 10);
        Render2D::PushClip(clientRect);

        String text;
        if (LogCount == 1)
        {
            text = String::Format(SE_TEXT("{0} ({1})"), Desc.Title, Desc.Description);
        }
        else if (LogCount > 1)
        {
            text = String::Format(SE_TEXT("{0} ({1})"), Desc.Title, LogCount);
        }
        Render2D::RenderText(style->FontMedium, text, textRect, style->TextColor, TextAlignment::Near, TextAlignment::Center);

        Render2D::PopClip();
    }

    void LogWindow::LogEntry::OnGetFocus()
    {
        Control::OnGetFocus();

        _window->Selected = this;
    }

    bool LogWindow::LogEntry::OnKeyDown(KeyboardKeys key)
    {
        // InputOptions options = FlaxEditor.Editor.Instance.Options.Options.Input;

        // Up
        if (key == KeyboardKeys::ArrowUp)
        {
            int index = IndexInParent - 1;
            if (index >= 0)
            {
                Control* target = Parent->GetChild(index);
                target->Focus();
                Panel* panel = static_cast<Panel*>(Parent->Parent.operator->());
                panel->ScrollViewTo(target);
                return true;
            }
        }
        // Down
        else if (key == KeyboardKeys::ArrowDown)
        {
            int index = IndexInParent + 1;
            if (index < Parent->ChildrenCount())
            {
                Control* target = Parent->GetChild(index);
                target->Focus();
                Panel* panel = static_cast<Panel*>(Parent->Parent.operator->());
                panel->ScrollViewTo(target);
                return true;
            }
        }
        // Enter
        else if (key == KeyboardKeys::Return)
        {
            Open();
        }
        /*// Ctrl+C
        else if (options.Copy.Process(this))
        {
            Copy();
            return true;
        }*/

        return Control::OnKeyDown(key);
    }

    void LogWindow::LogEntry::Open()
    {
        /*if (!String.IsNullOrEmpty(Desc.LocationFile) && File.Exists(Desc.LocationFile))
        {
            Editor.Instance.CodeEditing.OpenFile(Desc.LocationFile, Desc.LocationLine);
        }*/
    }

    void LogWindow::LogEntry::Copy()
    {
        // Clipboard.Text = Info.Replace("\r\n", "\n").Replace("\n", Environment.NewLine);
    }

    bool LogWindow::LogEntry::OnMouseDoubleClick(Float2 location, MouseButton button)
    {
        Open();
        return true;
    }

    bool LogWindow::LogEntry::OnMouseDown(Float2 location, MouseButton button)
    {
        if (Control::OnMouseDown(location, button))
            return true;

        if (button == MouseButton::Left)
        {
            Focus();
            return true;
        }
        if (button == MouseButton::Right)
        {
            Focus();

            _isRightMouseDown = true;
            return true;
        }

        return false;
    }

    bool LogWindow::LogEntry::OnMouseUp(Float2 location, MouseButton button)
    {
        if (Control::OnMouseUp(location, button))
            return true;

        if (_isRightMouseDown && button == MouseButton::Right)
        {
            Focus();

            ContextMenu* menu = New<ContextMenu>();
            menu->AddButton(SE_TEXT("Copy"), CreateFunc<LogEntry, &LogEntry::Copy>(this));
            menu->AddButton(SE_TEXT("Open"), CreateFunc<LogEntry, &LogEntry::Open>(this))->Enabled = Desc.LocationFile.Length() > 0 && FileSystem::FileExists(Desc.LocationFile);
            menu->Show(this, location);
        }

        return false;
    }

    void LogWindow::LogEntry::OnMouseLeave()
    {
        _isRightMouseDown = false;

        Control::OnMouseLeave();
    }

    void LogWindow::LogEntry::OnVisibleChanged()
    {
        // Deselect on hide
        if (!Visible && _window->Selected == this)
            _window->Selected = nullptr;

        Control::OnVisibleChanged();
    }

    LogWindow::LogWindow() : EditorWindow(&EditorApp::Ins(), true, ScrollBars::None)
    {
        m_AppendedCountIndex = 0;
        Title = SE_TEXT("Log");
        Icon = EditorIcons::LogWindow32;

        // OnEditorOptionsChanged(Editor.Options.Options);
        // FlaxEditor.Utilities.Utils.SetupCommonInputActions(this);

        _logCountPerGroup.Resize((int)LogGroup::Max);
        _groupButtons.Resize(3);

        // Toolstrip
        ToolStrip* toolStrip = New<ToolStrip>(22.0f, 0.0f);
        toolStrip->Parent = this;

        toolStrip->AddButton(SE_TEXT("Clear"), CreateFunc<LogWindow, &LogWindow::Clear>(this))->LinkTooltip(SE_TEXT("Clears all log entries"));
        _clearOnPlayButton = toolStrip->AddButton(SE_TEXT("Clear on Play"));
        _clearOnPlayButton->SetAutoCheck(true)->SetChecked(true)->LinkTooltip(SE_TEXT("Clears all log entries on enter playmode"));

        bool collapse = true;
        /*
         if (Editor.ProjectCache.TryGetCustomData("DebugLogCollapse", out var collapseString))
            if (bool.TryParse(collapseString, out var setCollapse))
                collapse = setCollapse;
        */

        // collapse = setCollapse;
        _collapseLogsButton = toolStrip->AddButton(SE_TEXT("Collapse"),
            [this]()
            {
                // Editor.ProjectCache.SetCustomData("DebugLogCollapse", _collapseLogsButton.Checked.ToString());
            });
        _collapseLogsButton->SetAutoCheck(true)->SetChecked(collapse)->LinkTooltip(SE_TEXT("Collapses similar logs."));

        _pauseOnErrorButton = toolStrip->AddButton(SE_TEXT("Pause on Error"));
        _pauseOnErrorButton->SetAutoCheck(true)->LinkTooltip(SE_TEXT("Performs auto pause on error"));
        toolStrip->AddSeparator();


        _groupButtons[0] = toolStrip->AddButton(EditorIcons::Error32, [this]()
        {
            UpdateLogTypeVisibility(LogGroup::Error, _groupButtons[0]->Checked);
        });
        _groupButtons[0]->SetAutoCheck(true)->SetChecked(true)->LinkTooltip(SE_TEXT("Shows/hides error messages"));

        _groupButtons[1] = toolStrip->AddButton(EditorIcons::Warning32, [this]()
        {
            UpdateLogTypeVisibility(LogGroup::Warning, _groupButtons[1]->Checked);
        });
        _groupButtons[1]->SetAutoCheck(true)->SetChecked(true)->LinkTooltip(SE_TEXT("Shows/hides warning messages"));

        _groupButtons[2] = toolStrip->AddButton(EditorIcons::Info32, [this]()
        {
            UpdateLogTypeVisibility(LogGroup::Info, _groupButtons[2]->Checked);
        });
        _groupButtons[2]->SetAutoCheck(true)->SetChecked(true)->LinkTooltip(SE_TEXT("Shows/hides info messages"));

        UpdateCount();

        // Split panel
        _split = New<SplitPanel>(Orientation::Vertical, ScrollBars::Vertical, ScrollBars::Both);
        _split->AnchorPreset = AnchorPresets::StretchAll,
        _split->Offsets = Margin(0, 0, toolStrip->Bottom, 0),
        _split->SplitterValue = 0.8f,
        _split->Parent = this;

        // Info detail info
        _logInfo = New<Label>();
        _logInfo->Parent = _split->Panel2;
        _logInfo->SetAutoWidth(true);
        _logInfo->SetAutoHeight(true);
        _logInfo->Margin = Margin(4);
        _logInfo->VerticalAlignment = TextAlignment::Near;
        _logInfo->HorizontalAlignment = TextAlignment::Near;
        _logInfo->Offsets = Margin::Zero;

        // Entries panel
        _entriesPanel = New<VerticalPanel>();
        _entriesPanel->AnchorPreset = AnchorPresets::HorizontalStretchTop;
        _entriesPanel->Offsets = Margin::Zero;
        _entriesPanel->IsScrollable = true;
        _entriesPanel->Parent = _split->Panel1;
    }

    void LogWindow::Clear()
    {
        if (_entriesPanel == nullptr)
            return;

        RemoveEntries();
    }

    void LogWindow::Add(LogEntryDescription& desc)
    {
        if (_entriesPanel == nullptr)
            return;

        // Create new entry
        /*switch (_timestampsFormats)
        {
        case InterfaceOptions.TimestampsFormats.Utc:
            desc.Title = $"[{DateTime.UtcNow}] {desc.Title}";
            break;
        case InterfaceOptions.TimestampsFormats.LocalTime:
            desc.Title = $"[{DateTime.Now}] {desc.Title}";
            break;
        case InterfaceOptions.TimestampsFormats.TimeSinceStartup:
            desc.Title = string.Format("[{0:g}] ", TimeSpan.FromSeconds(Time.TimeSinceStartup)) + desc.Title;
            break;
        }*/
        LogEntry* newEntry = New<LogEntry>(this, desc);

        // Enqueue
        {
            Threading::ScopeLock lock(_locker);
            _pendingEntries.Add(newEntry);
        }

        if (newEntry->Group == LogGroup::Warning && _iconType < Log::Severity::Warning)
        {
            _iconType = Log::Severity::Warning;
        }

        if (newEntry->Group == LogGroup::Error && _iconType < Log::Severity::Error)
        {
            _iconType = Log::Severity::Error;
        }

        // Pause on Error (we should do it as fast as possible)
        /*if (newEntry->Group == LogGroup::Error && _pauseOnErrorButton->Checked && Editor.StateMachine.CurrentState == Editor.StateMachine.PlayingState)
        {
            Editor.Simulation.RequestPausePlay();
        }*/
    }

    void LogWindow::Update(float deltaTime)
    {
        {
            Threading::ScopeLock lock(_locker);

            auto &logEntries = Log::System::GetLogEntries();

            for (int i = m_AppendedCountIndex; i < logEntries.Count(); i++)
            {
                auto logInfo = &logEntries[i];
                bool newEntry = true;
                if (_collapseLogsButton->Checked)
                {
                    for(Control* child : _entriesPanel->Children())
                    {
                        LogEntry* entry;
                        if (TypeTryCast<LogEntry>(child, entry ))
                        {
                            if (StringUtils::Compare(entry->Desc.Title.Get(), logInfo->category.Get()) &&
                                StringUtils::Compare(entry->Desc.LocationFile.Get(), logInfo->filename.Get()) &&
                                entry->Desc.Level == logInfo->severity &&
                                StringUtils::Compare(entry->Desc.Description.Get(), logInfo->message.Get()) &&
                                entry->Desc.LocationLine == logInfo->lineNumber)
                            {
                                entry->LogCount += 1;
                                newEntry = false;
                                break;
                            }
                        }
                    }
                }

                if (newEntry)
                {
                    LogEntryDescription desc;
                    desc.Level = logInfo->severity;
                    desc.Title = logInfo->category;
                    desc.Description = logInfo->message;
                    // desc.ContextObject = o?.ID ?? Guid.Empty;

                    /*if (stackTrace.Length() > 0)
                    {
                        // Detect code location and remove leading internal stack trace part
                        var matches = _logRegex.Matches(stackTrace);
                        bool foundStart = false, noLocation = true;
                        var fineStackTrace = _stringBuilder.Value;
                        fineStackTrace.Clear();
                        fineStackTrace.Capacity = Mathf.Max(fineStackTrace.Capacity, stackTrace.Length);
                        for (int i = 0; i < matches.Count; i++)
                        {
                            var match = matches[i];
                            var matchLocation = match.Groups[1].Value.Trim();
                            if (matchLocation.StartsWith("FlaxEngine.Debug.", StringComparison.Ordinal))
                            {
                                // C# start
                                foundStart = true;
                            }
                            else if (matchLocation.StartsWith("DebugLog::", StringComparison.Ordinal))
                            {
                                // C++ start
                                foundStart = true;
                            }
                            else if (foundStart)
                            {
                                if (noLocation)
                                {
                                    desc.LocationFile = match.Groups[2].Value;
                                    int.TryParse(match.Groups[4].Value, out desc.LocationLine);
                                    noLocation = false;
                                }
                                fineStackTrace.AppendLine(match.Groups[0].Value);
                            }
                        }
                        desc.Description = fineStackTrace.ToString();
                    }*/

                    Add(desc);
                }
            }

            m_AppendedCountIndex = logEntries.Count();


            if (_pendingEntries.Count() > 0)
            {
                // Check if user want's to scroll view by var (or is viewing earlier entry)
                Panel* panelScroll = static_cast<Panel*>(_entriesPanel->Parent.operator->());
                bool scrollView = (panelScroll->vScrollBar->GetMaximum() - panelScroll->vScrollBar->GetTargetValue()) < LogEntry::DefaultHeight * 1.5f;

                // Add pending entries
                LogEntry* newEntry = nullptr;
                bool anyVisible = false;
                _entriesPanel->SetIsLayoutLocked(true);
                float spacing = _entriesPanel->Spacing;
                Margin margin = _entriesPanel->Margin;
                Float2 offset = _entriesPanel->Offset;
                float width = _entriesPanel->Width - margin.GetWidth();
                float top = margin.Top;
                if (_entriesPanel->ChildrenCount() > 0)
                {
                    top = _entriesPanel->GetChild(_entriesPanel->ChildrenCount() - 1)->Bottom + spacing;
                }

                for (int i = 0; i < _pendingEntries.Count(); i++)
                {
                    newEntry = _pendingEntries[i];
                    newEntry->Visible = _groupButtons[(int)newEntry->Group]->Checked;
                    anyVisible |= newEntry->Visible;
                    newEntry->Parent = _entriesPanel;
                    newEntry->Bounds = Rectangle(margin.Left + offset.x, top + offset.y, width, newEntry->Height);
                    top = newEntry->Bottom + spacing;
                    _logCountPerGroup[(int)newEntry->Group]++;
                }
                _entriesPanel->Height = top + margin.Bottom;
                _entriesPanel->SetIsLayoutLocked(false);
                _pendingEntries.Clear();
                UpdateCount();
                ENGINE_ASSERT(newEntry != nullptr);

                // Scroll to the new entry (if any added to view)
                if (scrollView && anyVisible)
                {
                    panelScroll->ScrollViewTo(newEntry);

                    bool scrollViewNew = (panelScroll->vScrollBar->GetMaximum() - panelScroll->vScrollBar->GetTargetValue()) < LogEntry::DefaultHeight * 1.5f;
                    if (scrollViewNew != scrollView)
                    {
                        // Make sure scrolling doesn't stop in case too many entries were added at once
                        panelScroll->ScrollViewTo(Float2(Max_float, Max_float));
                    }
                }
            }
        }

        EditorWindow::Update(deltaTime);
    }
    
    void LogWindow::OnPlayBegin()
    {
        // Clear on Play
        if (_clearOnPlayButton->Checked)
        {
            Clear();
        }
    }

    void LogWindow::OnStartContainsFocus()
    {
        _iconType = Log::Severity::Info;
        EditorWindow::OnStartContainsFocus();
    }

    void LogWindow::OnDestroy()
    {
        if (IsDisposing)
            return;

        EditorWindow::OnDestroy();
    }

    void LogWindow::OnLayoutDeserialize()
    {
        _split->SplitterValue = 0.8f;
    }

    void LogWindow::UpdateLogTypeVisibility(LogGroup group, bool isVisible)
    {
        _entriesPanel->SetIsLayoutLocked(true);

        auto children = _entriesPanel->Children();
        for (int i = 0; i < children.Count(); i++)
        {
            LogEntry* logEntry;
            if (TypeTryCast<LogEntry>(children[i], logEntry) && logEntry->Group == group)
                logEntry->Visible = isVisible;
        }

        _entriesPanel->SetIsLayoutLocked(false);
        _entriesPanel->PerformLayout();
    }

    void LogWindow::UpdateCount()
    {
        UpdateCount((int)LogGroup::Error, SE_TEXT(" Error"));
        UpdateCount((int)LogGroup::Warning, SE_TEXT(" Warning"));
        UpdateCount((int)LogGroup::Info, SE_TEXT(" Message"));

        if (_logCountPerGroup[(int)LogGroup::Error] == 0)
        {
            _iconType = _logCountPerGroup[(int)LogGroup::Warning] == 0 ? Log::Severity::Info : Log::Severity::Warning;
        }
    }

    void LogWindow::UpdateCount(int group, String msg)
    {
        if (_logCountPerGroup[group] != 1)
            msg += 's';
        _groupButtons[group]->Text = String::Format(SE_TEXT("{0}{1}"), _logCountPerGroup[group], msg);
    }


    void LogWindow::RemoveEntries()
    {
        _entriesPanel->SetIsLayoutLocked(true);

        Selected = nullptr;
        for (int i = _entriesPanel->ChildrenCount() - 1; i >= 0; i--)
        {
            LogEntry* entry;
            if (TypeTryCast<LogEntry>(_entriesPanel->GetChild(i), entry))
            {
                _logCountPerGroup[(int)entry->Group]--;
                entry->Dispose();
                Delete(entry);
                i--;
            }
        }
        UpdateCount();

        _entriesPanel->Children().Clear();
        _entriesPanel->SetIsLayoutLocked(false);
        _entriesPanel->PerformLayout();
    }

    void LogWindow::__SetSelected(LogEntry* value)
    {
        // Check if value will change
        if (_selected != value)
        {
            // Select
            _selected = value;
            if (_selected != nullptr && _selected->GetInfo().Length() > 0)
            {
                _logInfo->Text = _selected->GetInfo();
            }else
            {
                _logInfo->Text = String::Empty;
            }
        }
    }
} // SE