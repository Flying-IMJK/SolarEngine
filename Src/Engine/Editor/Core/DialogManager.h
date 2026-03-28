#pragma once

#include "Editor/API.h"
#include "Core/Types/Strings/String.h"
#include "Core/Types/Delegate.h"
#include "Core/Math/Math.h"

#include "Imgui/imgui.h"

//-------------------------------------------------------------------------
namespace SE
{
    class UpdateContext;
}


namespace SE::Editor
{

    //-------------------------------------------------------------------------

    class SE_API_EDITOR DialogManager final
    {
        friend class EditorWindowOld;
        friend class EditorUI;

        //-------------------------------------------------------------------------

        // Abstraction for a modal dialog
        class ModalDialog
        {
            friend class EditorWindowOld;
            friend class EditorUI;
            friend class DialogManager;

        public:

            bool HasWindowConstraints() const
            {
                return m_windowConstraints[0].x > 0 && m_windowConstraints[0].y > 0 && m_windowConstraints[1].x > 0 && m_windowConstraints[1].y > 0;
            }

        private:

            String                                          m_title;
            Function<bool(UpdateContext const&)>            m_drawFunction;
            Float2                                          m_windowSize = Float2( 0, 0 );
            Float2                                          m_windowConstraints[2] = { Float2( -1, -1 ), Float2( -1, -1 ) };
            Float2                                          m_windowPadding;
            ImGuiCond                                       m_windowSizeCond = ImGuiCond_Always;
            ImGuiWindowFlags                                m_windowFlags = ImGuiWindowFlags_NoSavedSettings;
            bool                                            m_isOpen = true;
        };

    public:

        DialogManager() = default;
        DialogManager( DialogManager const& ) = default;
        ~DialogManager() ;

        DialogManager& operator=( DialogManager const& rhs ) = default;

        // Do we have a currently active modal dialog
        inline bool HasActiveModalDialog() const { return m_pActiveDialog != nullptr; }

        // Draw dialog
        bool DrawDialog( UpdateContext const& context );

        // Create a new modal dialog
        void CreateModalDialog( String const& title, Function<bool(UpdateContext const&)> const& drawFunction, Float2 const& windowSize = Float2( 0, 0 ), bool isResizable = false );

        // Set the current active dialog's size constraints
        void SetActiveModalDialogSizeConstraints( Float2 const& min, Float2 const& max );

    private:

        ModalDialog* m_pActiveDialog = nullptr;
    };
}