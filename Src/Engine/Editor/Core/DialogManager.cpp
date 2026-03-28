#include "DialogManager.h"
#include "Runtime/SGUI/GUILayout.h"
#include "Core/Memory/Memory.h"
//-------------------------------------------------------------------------

namespace SE::Editor
{
    DialogManager::~DialogManager()
    {
        if (m_pActiveDialog != nullptr)
        {
            Delete(m_pActiveDialog);
        }
    }

    bool DialogManager::DrawDialog(UpdateContext const &context)
    {
        bool hasOpenModalDialog = false;

        //-------------------------------------------------------------------------

        if (m_pActiveDialog == nullptr)
        {
            return hasOpenModalDialog;
        }

        //-------------------------------------------------------------------------

        if (!m_pActiveDialog->m_isOpen)
        {
            Delete(m_pActiveDialog);
            return hasOpenModalDialog;
        }

        //-------------------------------------------------------------------------

        if (m_pActiveDialog->HasWindowConstraints())
        {
            ::ImGui::SetNextWindowSizeConstraints(m_pActiveDialog->m_windowConstraints[0], m_pActiveDialog->m_windowConstraints[1]);
        }

        if (ImGui::BeginViewportPopupModal(m_pActiveDialog->m_title, &m_pActiveDialog->m_isOpen, m_pActiveDialog->m_windowSize, m_pActiveDialog->m_windowSizeCond, m_pActiveDialog->m_windowFlags))
        {
            hasOpenModalDialog = true;

            m_pActiveDialog->m_isOpen = m_pActiveDialog->m_drawFunction(context);

            if (::ImGui::IsKeyPressed(ImGuiKey_Escape))
            {
                m_pActiveDialog->m_isOpen = false;
            }

            if (!m_pActiveDialog->m_isOpen)
            {
                ::ImGui::CloseCurrentPopup();
            }

            ::ImGui::EndPopup();
        }

        return hasOpenModalDialog;
    }

    void DialogManager::CreateModalDialog(String const &title, Function<bool(UpdateContext const &)> const &drawFunction, Float2 const &windowSize, bool isResizable)
    {
        ENGINE_ASSERT(m_pActiveDialog == nullptr);
        m_pActiveDialog = New<ModalDialog>();
        m_pActiveDialog->m_title = title;
        m_pActiveDialog->m_drawFunction = drawFunction;
        m_pActiveDialog->m_windowSize = windowSize;

        // If we provide any auto size values, then switch to an auto sized window
        if (windowSize.x <= 0 && windowSize.y <= 0)
        {
            m_pActiveDialog->m_windowSizeCond = ImGuiCond_FirstUseEver;
            m_pActiveDialog->m_windowFlags = ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize;
        }
        else // We provided a valid window size
        {
            m_pActiveDialog->m_windowSizeCond = isResizable ? ImGuiCond_FirstUseEver : ImGuiCond_Always;
            m_pActiveDialog->m_windowFlags = isResizable ? ImGuiWindowFlags_NoSavedSettings : ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoResize;
        }

        m_pActiveDialog->m_isOpen = true;
    }

    void DialogManager::SetActiveModalDialogSizeConstraints(Float2 const &min, Float2 const &max)
    {
        ENGINE_ASSERT(min.x >= 0 && min.x <= max.x);
        ENGINE_ASSERT(min.y >= 0 && min.y <= max.y);
        m_pActiveDialog->m_windowConstraints[0] = min;
        m_pActiveDialog->m_windowConstraints[1] = max;
    }
}