#pragma once

#include "DebugView.h"
#include "Runtime/SGUI/GUILayout.h"
#include "Runtime/SGUI/Widgets/FilterWidget.h"
#include "Core/Logging/LoggingSystem.h"


//-------------------------------------------------------------------------

#if ENGINE_DEVELOPMENT_TOOLS
namespace SGE
{
    class UpdateContext;

    //-------------------------------------------------------------------------

    class SE_API_RUNTIME SystemLogView
    {
    public:

        void Draw( UpdateContext const& context );

    private:

        void UpdateFilteredList( UpdateContext const& context );

    public:

        bool                                                m_showLogMessages = true;
        bool                                                m_showLogWarnings = true;
        bool                                                m_showLogErrors = true;

    private:

        SGUI::FilterWidget                                   m_filterWidget;
        List<Log::LogEntry>                               m_filteredEntries;
        size_t                                              m_numLogEntriesWhenFiltered = 0;
    };

    //-------------------------------------------------------------------------

    class SE_API_RUNTIME SystemDebugView final : public DebugView
    {
        SE_CLASS( SystemDebugView, DebugView);

        friend class EngineDebugUI;

    public:

        static void DrawFrameLimiterCombo( UpdateContext& context );

    public:

        SystemDebugView() : DebugView( "System" ) {}

    private:

        void DrawMenu( EntityWorldUpdateContext const& context ) override;

        void DrawLogWindow( EntityWorldUpdateContext const& context, bool isFocused );

    private:

        SystemLogView m_logView;
    };
}
#endif