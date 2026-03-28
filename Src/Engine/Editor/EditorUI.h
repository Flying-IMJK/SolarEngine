#pragma once

#include "Editor/Core/EditorContext.h"
#include "Runtime/SGUI/ToolsUI/IDevelopmentGUI.h"
#include "Runtime/SGUI/GUILayout.h"
//#include "Engine/DebugViews/DebugView_System.h"
//#include "Runtime/ThirdParty/Imgui/imgui.h"
#include "Core/Types/Event.h"

//-------------------------------------------------------------------------
namespace SE
{
    class EntityWorldManager;
    class GamePreviewer;
    namespace EntityModel { class EntityMapEditor; }
    namespace Render{ class RenderingSystem; }
}

namespace SE::Editor
{
    class EditorWindow;
    class ResourceServer;
    //-------------------------------------------------------------------------

    class EditorUI final : public ImGui::IDevelopmentGUI, public EditorContext
    {
        struct ToolCreationRequest
        {
            enum Type
            {
                MapEditor,
                GamePreview,
                ResourceEditor,
                UninitializedTool,
            };

			ToolCreationRequest() : pEditorTool(), type( UninitializedTool ) {}

            ToolCreationRequest( EditorWindow* pEditorTool ) : pEditorTool( pEditorTool ), type( UninitializedTool ) { ENGINE_ASSERT( pEditorTool != nullptr ); }

			ToolCreationRequest( EditorWindow* pEditorTool, Type type) : pEditorTool( pEditorTool ), type( type ) { ENGINE_ASSERT( pEditorTool != nullptr ); }

        public:

            ResID           resourceID;
            EditorWindow*       pEditorTool = nullptr;
            Type                type = ResourceEditor;
        };

    public:

        ~EditorUI();

        void SetStartupMap( ResID  const& mapID );

        void Initialize( UpdateContext const& context) override;
        void Shutdown( UpdateContext const& context ) override;

        void GetBorderlessTitleBarInfo(Math::ScreenSpaceRectangle& outTitlebarRect, bool& isInteractibleWidgetHovered) const;

    private:

        virtual void StartFrame( UpdateContext const& context ) override final;
        virtual void EndFrame( UpdateContext const& context ) override final;
        virtual void Update( UpdateContext const& context ) override final;
        virtual EntityWorldManager* GetWorldManager() const override final { return nullptr;/*m_pWorldManager;*/ }
        virtual bool TryOpenResource( ResID  const& resourceID ) const override;
        virtual bool TryFindInResourceBrowser( ResID  const& resourceID ) const override;

        // Title bar
        //-------------------------------------------------------------------------

        void DrawTitleBarMenu( UpdateContext const& context );
        void DrawTitleBarInfoStats( UpdateContext const& context );

        // Hot Reload
        //-------------------------------------------------------------------------

        virtual void HotReload_UnloadResources(List<ResourceRequesterID> const& usersToBeReloaded, List<ResID > const& resourcesToBeReloaded ) override;
        virtual void HotReload_ReloadResources() override;

        // Resource Management
        //-------------------------------------------------------------------------

        void OnResourceDeleted( ResID  const& resourceID );

        // Editor Tool Management
        //-------------------------------------------------------------------------

        // Immediately destroy a editor tool
        void DestroyTool( UpdateContext const& context, EditorWindow* pEditorTool, bool isEditorShutdown = false );

        // Queues a editor tool destruction request till the next update
        void QueueDestroyTool( EditorWindow* pEditorTool );

        // Tries to immediately create a editor tool
        bool TryCreateTool( UpdateContext const& context, ToolCreationRequest const& request );

        // Queues a editor tool creation request till the next update
        void QueueCreateTool( ResID  const& resourceID );

        // Submit a editor tool so we can retrieve/update its docking location
        bool SubmitToolMainWindow( UpdateContext const& context, EditorWindow* pEditorTool, ImGuiID editorDockspaceID );

        // Draw editor tool child windows
        void DrawToolContents( UpdateContext const& context, EditorWindow* pEditorTool );

        // Create a game preview editor tool
        void CreateGamePreviewTool( UpdateContext const& context );

        // Queues the preview editor tool for destruction
        void DestroyGamePreviewTool( UpdateContext const& context );

        // Copy the layout from one editor tool to the other
        void ToolLayoutCopy( EditorWindow* pSourceTool );

        // Get the first created editor tool of a specified type
        template<typename T>
        inline T* GetTool() const
        {
            static_assert( std::is_base_of<SE::Editor::EditorWindow, T>::value, "T is not derived from EditorWindow" );
            for ( auto pEditorTool : m_editorTools )
            {
                if ( pEditorTool->GetUniqueTypeID() == T::s_toolTypeID )
                {
                    return static_cast<T*>( pEditorTool );
                }
            }

            return nullptr;
        }

        // Create a new editor tool
        template<typename T, typename... ConstructorParams>
        inline T* CreateTool(ConstructorParams&&... params )
        {
            static_assert( std::is_base_of<SE::Editor::EditorWindow, T>::value, "T is not derived from EditorWindow" );

            if ( T::s_isSingleton )
            {
                auto pExistingTool = GetTool<T>();
                if( pExistingTool != nullptr )
                {
                    return pExistingTool;
                }

                // Check for already queued creation request
                for ( auto const& creationRequest : m_editorToolCreationRequests )
                {
                    if ( creationRequest.pEditorTool == nullptr )
                    {
                        continue;
                    }

                    if ( creationRequest.pEditorTool->GetUniqueTypeID() == T::s_toolTypeID )
                    {
                        return static_cast<T*>( creationRequest.pEditorTool );
                    }
                }
            }

            //-------------------------------------------------------------------------

            T* pEditorTool = New<T>(Forward<ConstructorParams>( params )...);
            ToolCreationRequest creationRequest(pEditorTool);
			m_editorToolCreationRequests.Add(creationRequest);
            return pEditorTool;
        }

        // Misc
        //-------------------------------------------------------------------------

        void DrawUITestWindow();

    private:

        ResID                          m_startupMapResourceID;
        ImGui::ApplicationTitleBar          m_titleBar;
        ImGui::ImageInfo                    m_editorIcon;

        // Systems
        Render::RenderingSystem*           m_pRenderingSystem = nullptr;
        // EntityWorldManager*                m_pWorldManager = nullptr;

        // Window Management
        ImGuiWindowClass                   m_editorWindowClass;
        bool                               m_isImguiDemoWindowOpen = false;
        bool                               m_isImguiPlotDemoWindowOpen = false;
        bool                               m_isUITestWindowOpen = false;

        // Resource Browser
        ResourceDatabase                   m_resourceDB;
        EventBindingID                     m_resourceDeletedEventID;
        float                              m_resourceBrowserViewWidth = 150;

        // Tools
        List<EditorWindow*>                m_editorTools;
        List<ToolCreationRequest>          m_editorToolCreationRequests;
        List<EditorWindow*>                m_editorToolDestructionRequests;
        void*                              m_pLastActiveTool = nullptr;
        bool                               m_hasOpenModalDialog = false;

        // Map Editor and Game Preview
        // EntityModel::EntityMapEditor*      m_pMapEditor = nullptr;
        // GamePreviewer*                     m_pGamePreviewer = nullptr;
        EventBindingID                     m_gamePreviewStartRequestEventBindingID;
        EventBindingID                     m_gamePreviewStopRequestEventBindingID;
    };
}