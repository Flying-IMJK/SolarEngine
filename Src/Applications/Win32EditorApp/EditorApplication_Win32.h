#pragma once

#include "Editor/EditorEngine.h"
#include "Runtime/Core/Platform/Win32/Application_Win32.h"

//-------------------------------------------------------------------------

namespace SGE
{
    //-------------------------------------------------------------------------

    class EditorApplication final : public Win32Application
    {

    public:
        EditorApplication(HINSTANCE hInstance);

    private:

        virtual bool ProcessCommandline(int32_t argc, char** argv) override;
        virtual bool Initialize() override;
        virtual bool Shutdown() override;

        virtual void GetBorderlessTitleBarInfo(ScreenSpaceRectangle& outTitlebarRect, bool& isInteractibleWidgetHovered ) const override;
        virtual void ProcessWindowResizeMessage( Int2 const& newWindowSize ) override;
        virtual void ProcessInputMessage( UINT message, WPARAM wParam, LPARAM lParam ) override;

        virtual bool ApplicationLoop() override;

		virtual LRESULT WindowMessageProcessor( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );

    private:

        Editor::EditorEngine m_Engine;
    };
}