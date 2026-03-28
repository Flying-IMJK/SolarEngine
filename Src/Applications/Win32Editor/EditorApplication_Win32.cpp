#include "EditorApplication_Win32.h"
#include "Resource.h"
// #include "Applications/Editor/EditorUI.h"
// #include "Applications/Shared/LivePP/LivePP.h"
#include "Core/ThirdParty/cmdParser/cmdParser.h"
#include "Core/Types/Strings/String.h"
#include "Core/FileSystem/FileSystemUtils.h"

#include "Runtime/Platform/Imgui/ImguiPlatform_Win32.h"

#include "Editor/EditorUI.h"

#include <tchar.h>
#include <windows.h>

//-------------------------------------------------------------------------

namespace SGE
{
    //-------------------------------------------------------------------------

    EditorApplication::EditorApplication(HINSTANCE hInstance)
        : Win32Application(hInstance, "Solar Editor",
            IDI_WICKEDENGINEGAME, 
            TBFlags<InitOptions>(Win32Application::InitOptions::Borderless)),
            m_Engine([this](String const &error) -> bool { return FatalError(error); })
    {
    }

    void EditorApplication::GetBorderlessTitleBarInfo(Math::ScreenSpaceRectangle &outTitlebarRect, bool &isInteractibleWidgetHovered) const
    {
        m_Engine.GetBorderlessTitleBarInfo(outTitlebarRect, isInteractibleWidgetHovered);
    }

    void EditorApplication::ProcessWindowResizeMessage(Int2 const &newWindowSize)
    {
        m_Engine.GetRenderingSystem()->ResizePrimaryRenderTarget(newWindowSize);
    }

    void EditorApplication::ProcessInputMessage(UINT message, WPARAM wParam, LPARAM lParam)
    {
        m_Engine.GetInputSystem()->ForwardInputMessageToInputDevices({message, (uintptr_t)wParam, (uintptr_t)lParam});
    }

    bool EditorApplication::ProcessCommandline(int32_t argc, char **argv)
    {
        cli::Parser cmdParser(argc, argv);
        cmdParser.set_optional<std::string>("map", "map", "", "The startup map.");

        if (!cmdParser.run())
        {
            return FatalError("Invalid command line arguments!");
        }

		std::string const map = cmdParser.get<std::string>("map");
        if (!map.empty())
        {
            m_Engine.SetResourceStartMap(ResPath(map.c_str()));
        }

        return true;
    }

    bool EditorApplication::Initialize()
    {
        Int2 const windowDimensions((m_windowRect.right - m_windowRect.left), (m_windowRect.bottom - m_windowRect.top));
        if (!m_Engine.Initialize(windowDimensions))
        {
            return false;
        }

        return true;
    }

    bool EditorApplication::Shutdown()
    {
        return m_Engine.Shutdown();
    }

    bool EditorApplication::ApplicationLoop()
    {
        return m_Engine.Update();
    }


	LRESULT EditorApplication::WindowMessageProcessor(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		if (IsInitialized())
		{
			auto const imguiResult = SGUI::Platform::WindowMessageProcessor(hWnd, message, wParam, lParam);
			if (imguiResult != 0)
			{
				return imguiResult;
			}
		}

		return Win32Application::WindowMessageProcessor(hWnd, message, wParam, lParam);
	}
}

//-------------------------------------------------------------------------

void SetLoadLibDir()
{
    SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_APPLICATION_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32 | LOAD_LIBRARY_SEARCH_USER_DIRS);

    SGE::List<SGE::String> splitLib;
    SGE::String(EngineLoadLibDirList).Split(',', splitLib);

    for (SGE::String libDir : splitLib)
    {
        SGE::FileSystem::Path libPath = SGE::FileSystem::GetCurrentProcessPath();
        libPath.Append(libDir.Get());
        libPath.EnsureCorrectPathStringFormat();

        assert(AddDllDirectory(SGE::WString(libPath.ToString()).Get()) != 0);
    }
}

int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
    int32_t result = 0;
    {
#if EE_ENABLE_LPP
        auto lppAgent = SGE::ScopedLPPAgent();
#endif
        SetLoadLibDir();

        //-------------------------------------------------------------------------

        SGE::ApplicationGlobalState globalState;
        SGE::EditorApplication editorApplication(hInstance);

        #ifdef UNICODE
		SGE::String argString = SGE::String(*__wargv);
		char* args = argString.Get();
		result = editorApplication.Run(__argc, &args);
        #else
        result = editorApplication.Run(__argc, __argv);
        #endif


    }

    return result;
}