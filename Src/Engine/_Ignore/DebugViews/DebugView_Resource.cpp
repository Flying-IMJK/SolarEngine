#include "DebugView_Resource.h"
#include "Runtime/System/ResourceSystem.h"
#include "Core/Systems.h"
#include "Runtime/SGUI/GUILayout.h"

//-------------------------------------------------------------------------

#ifdef SE_DEVELOPMENT
namespace SGE
{
    void ResourceDebugView::DrawResourceSystemOverview( ResourceSystem* pResourceSystem )
    {
        ENGINE_ASSERT( pResourceSystem != nullptr );

        //-------------------------------------------------------------------------

        auto DrawRow = [] ( ResRecord const* pRecord )
        {
            ImGui::TableNextRow();

            //-------------------------------------------------------------------------

            ImGui::TableSetColumnIndex( 0 );
            SGUI::Label(pRecord->GetResourceTypeID().ToString());

            //-------------------------------------------------------------------------

            ImGui::TableSetColumnIndex( 1 );
            SGUI::Label(StringUtils::ToString(pRecord->m_References.Count()));

            //-------------------------------------------------------------------------

            ImGui::TableSetColumnIndex( 2 );

            switch ( pRecord->m_LoadingStatus )
            {
                case LoadingStatus::Unloaded:
                {
                    SGUI::Label(SE_TEXT("Unloaded"));
                }
                break;

                case LoadingStatus::Loading:
                {
                    ImGui::TextColored(Colors::Yellow.ToFloat4(), "Loading" );
                }
                break;

                case LoadingStatus::Loaded:
                {
                    ImGui::TextColored(Colors::LimeGreen.ToFloat4(), "Loaded");
                }
                break;

                case LoadingStatus::Unloading:
                {
                    SGUI::Label( SE_TEXT("Unloading") );
                }
                break;

                case LoadingStatus::Failed:
                {
                    ImGui::TextColored( Colors::Red.ToFloat4(), "Failed" );
                    ImGui::SameLine();
                    SGUI::HelpMarker( pRecord->GetCompilationLog().ToStringAnsi().Get() );
                }
                break;
            }

            //-------------------------------------------------------------------------

            ImGui::TableSetColumnIndex( 3 );
            if ( ImGui::TreeNode( StringAnsi(pRecord->GetResourceID().c_str()).Get() ) )
            {
                for ( auto const& requesterID : pRecord->m_References )
                {
                    if ( requesterID.IsManualRequest() )
                    {
                        ImGui::TextColored( Colors::Cyan.ToFloat4(), "Manual Request" );
                    }
                    else if ( requesterID.IsToolsRequest() )
                    {
                        ImGui::TextColored( Colors::Cyan.ToFloat4(), "Tools Request" );
                    }
                    else if ( requesterID.IsInstallDependencyRequest() )
                    {
                        ImGui::TextColored( Colors::Coral.ToFloat4(), "Install Dependency: %u", requesterID.GetInstallDependencyResourcePathID() );
                    }
                    else // Normal request
                    {
                        ImGui::TextColored( Colors::Green.ToFloat4(), "Entity: %u", requesterID.GetID() );
                    }
                }

                ImGui::TreePop();
            }

            //-------------------------------------------------------------------------

            {
                GUI::ScopedFont const sf( GUI::Font::Tiny );

                ImGui::TableSetColumnIndex( 4 );
                SGUI::Label(String::Format(SE_TEXT("{0:.3f}ms"), pRecord->GetFileReadTime().ToFloat()).Get());
                if ( ImGui::IsItemHovered() ) { ImGui::SetTooltip( "File Read Time" ); }

                ImGui::TableSetColumnIndex( 5 );
                SGUI::Label(String::Format(SE_TEXT("{0:.3f}ms"), pRecord->GetLoadTime().ToFloat()).Get());
                if ( ImGui::IsItemHovered() ) { ImGui::SetTooltip( "Load Time" ); }

                ImGui::TableSetColumnIndex( 6 );
                SGUI::Label(String::Format(SE_TEXT("{0:.3f}ms"), pRecord->GetDependenciesWaitTime().ToFloat()).Get());
                if ( ImGui::IsItemHovered() ) { ImGui::SetTooltip( "Wait for Dependencies Time" ); }

                ImGui::TableSetColumnIndex( 7 );
                SGUI::Label(String::Format(SE_TEXT("{0:.3f}ms"), pRecord->GetInstallTime().ToFloat()).Get());
                if ( ImGui::IsItemHovered() ) { ImGui::SetTooltip( "Install Time" ); }
            }
        };

        //-------------------------------------------------------------------------

        SGUI::Label(String::Format(SE_TEXT("Num Resources Loaded: {0}"), pResourceSystem->m_resourceRecords.Count()).Get());

        ImGui::Separator();

        if ( ImGui::BeginTable( "Resource Reference Tracker Table", 8, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable ) )
        {
            ImGui::TableSetupColumn( "Type", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 30 );
            ImGui::TableSetupColumn( "Refs", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 24 );
            ImGui::TableSetupColumn( "Status", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 60 );
            ImGui::TableSetupColumn( "ID", ImGuiTableColumnFlags_WidthStretch );
            ImGui::TableSetupColumn( "FRT", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 0 );
            ImGui::TableSetupColumn( "LT", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 0 );
            ImGui::TableSetupColumn( "DWT", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 0 );
            ImGui::TableSetupColumn( "IT", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 0 );

            //-------------------------------------------------------------------------

            ImGui::TableHeadersRow();

            //-------------------------------------------------------------------------

            auto const& resourceRecords = pResourceSystem->m_resourceRecords;
            for ( auto const& recordTuple : resourceRecords )
            {
                ResRecord const* pRecord = recordTuple.Value;
                DrawRow( pRecord );
            }

            ImGui::EndTable();
        }
    }

    void ResourceDebugView::DrawRequestHistory( ResourceSystem* pResourceSystem )
    {
        if ( ImGui::BeginTable( "Resource Request History Table", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable ) )
        {
            ImGui::TableSetupColumn( "Time", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 50 );
            ImGui::TableSetupColumn( "Request", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 45 );
            ImGui::TableSetupColumn( "Type", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 30 );
            ImGui::TableSetupColumn( "ID", ImGuiTableColumnFlags_WidthStretch );

            //-------------------------------------------------------------------------

            ImGui::TableHeadersRow();

            //-------------------------------------------------------------------------

            int32 const numEntries = (int32) pResourceSystem->m_history.Count();
            int32 const lastEntryIdx = numEntries - 1;

            ImGuiListClipper clipper;
            clipper.Begin( numEntries );
            while ( clipper.Step() )
            {
                for ( int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++ )
                {
                    auto const& entry = pResourceSystem->m_history[lastEntryIdx - i];

                    ImGui::TableNextRow();

                    //-------------------------------------------------------------------------

                    ImGui::TableSetColumnIndex( 0 );
                    SGUI::Label( entry.m_time.ToString());

                    //-------------------------------------------------------------------------

                    ImGui::TableSetColumnIndex( 1 );
                    switch ( entry.m_type )
                    {
                        case ResourceSystem::PendingRequest::Type::Load:
                        {
                            ImGui::TextColored( Colors::LimeGreen.ToFloat4(), "Load" );
                        }
                        break;

                        case ResourceSystem::PendingRequest::Type::Unload:
                        {
                            ImGui::TextColored( Colors::OrangeRed.ToFloat4(), "Unload" );
                        }
                        break;
                    }

                    //-------------------------------------------------------------------------

                    ImGui::TableSetColumnIndex( 2 );
                    SGUI::Label( entry.m_ID.GetResourceTypeID().ToString());

                    //-------------------------------------------------------------------------

                    ImGui::TableSetColumnIndex( 3 );
                    SGUI::Label( entry.m_ID.c_str());
                }
            }

            // Auto scroll the table
            if ( ImGui::GetScrollY() >= ImGui::GetScrollMaxY() )
            {
                ImGui::SetScrollHereY( 1.0f );
            }

            ImGui::EndTable();
        }
    }

    //-------------------------------------------------------------------------

    void ResourceDebugView::Initialize( Systems const& systemRegistry/*, EntityWorld const* pWorld */)
    {
        DebugView::Initialize( systemRegistry); //, pWorld );
        m_pResourceSystem = systemRegistry.GetSystem<ResourceSystem>();

        m_windows.Add( Window("Resource Request History", [this] (/* EntityWorldUpdateContext const& context, */bool isFocused, uint64 )
		{
		  DrawRequestHistory( m_pResourceSystem );
		}));
        m_windows.Add( Window("Resource System Overview", [this] (/* EntityWorldUpdateContext const& context, */bool isFocused, uint64 )
        { 
            DrawResourceSystemOverview(m_pResourceSystem); 
        } ));
    }

    void ResourceDebugView::Shutdown()
    {
        m_pResourceSystem = nullptr;
        DebugView::Shutdown();
    }

    void ResourceDebugView::DrawMenu( EntityWorldUpdateContext const& context )
    {
        if ( ImGui::MenuItem( "Show Request History" ) )
        {
            m_windows[0].m_isOpen = true;
        }

        if ( ImGui::MenuItem( "Show Resource System Overview" ) )
        {
            m_windows[1].m_isOpen = true;
        }
    }
}
#endif