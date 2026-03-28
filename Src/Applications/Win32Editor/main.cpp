

#include "Core/Platform/Win32/IncludeWindowsHeaders.h"
#include "Runtime/Engine.h"
#include "Editor/EditorApp.h"

#include "Resource.h"

int Run(HINSTANCE hInstance, LPTSTR lpCmdLine)
{
	SE::Platform::PreInit(hInstance, IDI_SAMPLER);

	return SE::Engine::Main(lpCmdLine, &SE::Editor::EditorApp::Ins());
}

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	SetDllDirectoryA(EngineLoadLibDirList);

	__try
	{
		return Run(hInstance, lpCmdLine);
	}
	__except (SE::Platform::SehExceptionHandler(GetExceptionInformation()))
	{
		return -1;
	}
}
