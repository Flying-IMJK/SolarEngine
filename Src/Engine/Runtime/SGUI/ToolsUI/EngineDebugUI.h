#pragma once

#include "IDevelopmentGUI.h"
// #include "Runtime/DebugViews/DebugView_System.h"
#include "Runtime/UpdateContext.h"
#include "Core/Types/Collections/List.h"

//-------------------------------------------------------------------------

struct ImDrawList;
struct ImGuiWindowClass;

// Debug UI for the Game World
//-------------------------------------------------------------------------

#ifdef SE_DEVELOPMENT
namespace SE
{
    class DebugView;
    class EntityWorld;

    namespace Render { class RenderViewPort; }

    //-------------------------------------------------------------------------

    class SE_API_RUNTIME EngineDebugUI final : public SGUI::IDevelopmentGUI
    {
        // Helper to sort and categorize all the various menus that the debug views can register
        struct Menu
        {
            using MenuPath = List<String, InlinedAllocation<5>>;
            static MenuPath CreatePathFromString( String const& pathString );

        public:

            Menu( String const& title ) : m_title( title ) { ENGINE_ASSERT( !m_title.IsEmpty() ); }

            inline bool IsEmpty() const { return m_childMenus.IsEmpty() && m_debugViews.IsEmpty(); }
            
            void Clear();

            void AddChildMenu( DebugView* pDebugView );
            void RemoveChildMenu( DebugView* pDebugView );
            void RemoveEmptyChildMenus();
            void Draw(/*EntityWorldUpdateContext const& context */);

        private:

            Menu& FindOrAddMenu( MenuPath const& path );
            bool TryFindMenu( DebugView* pDebugView );
            bool TryFindAndRemoveMenu( DebugView* pDebugView );

        public:

            String                                             m_title;
            List<Menu>                                       m_childMenus;
            List<DebugView*>                                 m_debugViews;
        };

    public:

        virtual void Initialize( UpdateContext const& context) override final;
        virtual void Shutdown( UpdateContext const& context ) override final;
        virtual void HotReload_UnloadResources(List<ResourceRequesterID> const& usersToReload, List<ResID> const& resourcesToBeReloaded ) override;
        virtual void HotReload_ReloadResources() override;

        void DrawMenu( UpdateContext const& context );
        void DrawOverlayElements( UpdateContext const& context, Render::RenderViewPort const* pViewport );

        // Locks the game overlay to a given imgui window by ID
        void EditorPreviewUpdate( UpdateContext const& context, ImGuiWindowClass* pWindowClass );

    private:

        virtual void EndFrame( UpdateContext const& context ) override final;
        void HandleUserInput( UpdateContext const& context );

        void ToggleWorldPause();
        void SetWorldTimeScale( float newTimeScale );
        void ResetWorldTimeScale();
        void RequestWorldTimeStep();

        void DrawPlayerDebugOptionsMenu( UpdateContext const& context );

        template<typename T>
        T* FindDebugView() const
        {
            for ( auto pDebugView : m_debugViews )
            {
                if ( auto pDesiredDebugView = TryCast<T>( pDebugView ) )
                {
                    return pDesiredDebugView;
                }
            }

            return nullptr;
        }

    protected:

        EntityWorld*                                        m_pGameWorld = nullptr;
        List<DebugView*>                                  m_debugViews;
        Menu                                                m_mainMenu = Menu("Main Menu");
        ImGuiWindowClass*                                   m_pWindowClass = nullptr;
        bool                                                m_isInEditorPreviewMode = false;

        Seconds                                             m_averageDeltaTime = 0.0f;
        float                                               m_timeScale = 1.0f;
        bool                                                m_debugOverlayEnabled = false;
    };
}
#endif