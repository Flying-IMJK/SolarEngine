#pragma once

#include "EditorWindow.h"

namespace SE
{
    class VerticalPanel;
    class SplitPanel;
    class Label;
}

namespace SE::Editor
{
    class ToolStripButton;

    /// <summary>
    /// Editor window used to show debug info, warning and error messages. Captures <see cref="Debug"/> messages.
    /// </summary>
    class LogWindow : public EditorWindow
    {
    public:
        /// <summary>
        /// Debug log entry description;
        /// </summary>
        struct LogEntryDescription
        {
            /// <summary>
            /// The log level.
            /// </summary>
            Log::Severity Level;

            /// <summary>
            /// The message title.
            /// </summary>
            String Title;

            /// <summary>
            /// The message description.
            /// </summary>
            String Description;

            /// <summary>
            /// The target object hint id (don't store ref, object may be an actor that can be removed and restored later or sth).
            /// </summary>
            UID ContextObject;

            /// <summary>
            /// The location of the issue (file path).
            /// </summary>
            String LocationFile;

            /// <summary>
            /// The location line number (zero or less to not use it);
            /// </summary>
            int LocationLine;
        };

    private:
        enum class LogGroup
        {
            Error = 0,
            Warning,
            Info,
            Max
        };

        class LogEntry : public Control
        {
        private:
            bool _isRightMouseDown = false;
            LogWindow* _window;

        public:
            /// <summary>
            /// The default height of the entries.
            /// </summary>
            constexpr static float DefaultHeight = 32.0f;

            LogGroup Group;
            LogEntryDescription Desc;
            SpriteHandle Icon;
            int LogCount = 1;

            LogEntry(LogWindow* window, LogEntryDescription desc);

            /// <summary>
            /// Gets the information text.
            /// </summary>
            String GetInfo();

            /// <inheritdoc />
            void Draw() override;

            /// <inheritdoc />
            void OnGetFocus() override;

            /// <inheritdoc />
            bool OnKeyDown(KeyboardKeys key) override;

            /// <summary>
            /// Opens the entry location.
            /// </summary>
            void Open();

            /// <summary>
            /// Copies the entry information to the system clipboard (as text).
            /// </summary>
            void Copy();

            bool OnMouseDoubleClick(Float2 location, MouseButton button) override;

            /// <inheritdoc />
            bool OnMouseDown(Float2 location, MouseButton button) override;

            /// <inheritdoc />
            bool OnMouseUp(Float2 location, MouseButton button) override;

            /// <inheritdoc />
            void OnMouseLeave() override;

        protected:
            /// <inheritdoc />
            void OnVisibleChanged() override;
        };

        CriticalSection _locker;
        SplitPanel* _split;
        Label* _logInfo;
        VerticalPanel* _entriesPanel;
        LogEntry* _selected = nullptr;
        List<int, InlinedAllocation<(int)LogGroup::Max>> _logCountPerGroup;
        // Regex _logRegex = new Regex("at (.*) in (.*):(line (\\d*)|(\\d*))");
        // Threading::ThreadLocal<StringBuilder> _stringBuilder = Threading::ThreadLocal<StringBuilder>();
        // InterfaceOptions.TimestampsFormats _timestampsFormats;

        // object _locker = new object();
        int m_AppendedCountIndex;
        List<LogEntry*> _pendingEntries = List<LogEntry*>(32);

        ToolStripButton* _clearOnPlayButton;
        ToolStripButton* _collapseLogsButton;
        ToolStripButton* _pauseOnErrorButton;
        List<ToolStripButton*, InlinedAllocation<3>> _groupButtons;

        Log::Severity _iconType = Log::Severity::Info;

    public:

        LogWindow();

        /*private void OnEditorOptionsChanged(EditorOptions options)
        {
            _timestampsFormats = options.Interface.DebugLogTimestampsFormat;
        }*/

        /// <summary>
        /// Clears the log.
        /// </summary>
        void Clear();

        /// <summary>
        /// Adds the specified log entry.
        /// </summary>
        /// <param name="desc">The log entry description.</param>
        void Add(LogEntryDescription &desc);

        /// <inheritdoc />
        void Update(float deltaTime) override;

        /// <inheritdoc />
        void OnPlayBegin() override;

        /// <inheritdoc />
        void OnStartContainsFocus() override;

        /// <inheritdoc />
        void OnDestroy() override;


        /// <inheritdoc />
        void OnLayoutDeserialize() override;

    private:
        /// <summary>
        /// Gets or sets the selected entry.
        /// </summary>
        PRO(Selected, LogWindow, LogEntry*, __GetSelected, __SetSelected);

        void UpdateLogTypeVisibility(LogGroup group, bool isVisible);

        void UpdateCount();

        void UpdateCount(int group, String msg);

        void RemoveEntries();

    protected:
        bool __GetUseLayoutData() override { return true; }

    private:
        LogEntry* __GetSelected() { return _selected; }
        void __SetSelected(LogEntry* value);
    };

} // SE
