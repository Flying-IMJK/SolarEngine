#ifdef PLATFORM_WIN32
#pragma once

#include "Runtime/API.h"

#include "Runtime/Core/Types/Strings/String.h"
#include "Runtime/Core/Types/BitFlags.h"
#include <windows.h>

//-------------------------------------------------------------------------

namespace SE
{
    namespace Math { class Rectangle; }

    //-------------------------------------------------------------------------

    class SE_API_RUNTIME Win32Application
    {
    protected:

        enum class InitOptions
        {
            StartMinimized = 1 < 0,
            Borderless = 1 << 1,
        };

    public:

        Win32Application( HINSTANCE hInstance, char const* applicationName, int32 iconResourceID, EnumFlags<InitOptions> options = EnumFlags<InitOptions>() );
        virtual ~Win32Application();

        int32 Run( int32 argc, char** argv );

        inline bool IsInitialized() const { return m_initialized;}

        // Win32 Window process
        virtual LRESULT WindowMessageProcessor( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );

        // Called whenever we receive an application exit request. Return true to allow the exit
        virtual bool OnUserExitRequest() { return true; }

        // Get the application icon
        inline HICON GetIcon() const { return m_windowIcon; }

    protected:

        bool FatalError( String const& error );

        //-------------------------------------------------------------------------

        // Called just before destroying the window
        virtual void ProcessWindowDestructionMessage();

        // Handle windows resize messages
        virtual void ProcessWindowResizeMessage( Int2 const& newWindowSize ) = 0;

        // Handle user application input messages
        virtual void ProcessInputMessage( UINT message, WPARAM wParam, LPARAM lParam ) {};

        // Get title bar region for border less windows
        virtual void GetBorderlessTitleBarInfo(Rectangle& outTitlebarRect, bool& isInteractibleWidgetHovered ) const {};

        //-------------------------------------------------------------------------

        // This function allows the application to process all command line settings. Will be called before initialize.
        virtual bool ProcessCommandline( int32 argc, char** argv ) { return true; }

        // These function allows the application to read/write any layout/positioning specific settings it needs
        virtual void WriteLayoutSettings();
        virtual void ReadLayoutSettings();

        // Initialize/Shutdown
        virtual bool Initialize() = 0;
        virtual bool Shutdown() = 0;

        // The actual application loop
        virtual bool ApplicationLoop() = 0;

        //-------------------------------------------------------------------------

        void RequestApplicationExit() { m_applicationRequestedExit = true; }

    private:

        bool TryCreateWindow();

        LRESULT BorderlessWindowHitTest( POINT cursor );

    protected:

        String const                    m_applicationName;
        String const                    m_applicationNameNoWhitespace;
        int32                           m_applicationIconResourceID = -1;
        WNDCLASSEX                      m_windowClass;
        HINSTANCE                       m_pInstance = nullptr;
        HICON                           m_windowIcon = nullptr;
        HWND                            m_windowHandle = nullptr;
        RECT                            m_windowRect = { 0, 0, 640, 480 };

        // Custom flags that user applications can set to specify what modes were enabled or what windows were open (saved in the layout.ini)
        uint64_t                        m_userFlags = 0;

    private:

        bool                            m_wasMaximized = false; // Read from the layout settings
        bool                            m_startMinimized = false; // Specifies the initial state of the application
        bool                            m_initialized = false;
        bool                            m_applicationRequestedExit = false;
        bool                            m_isBorderLess = false;
    };
}
#endif