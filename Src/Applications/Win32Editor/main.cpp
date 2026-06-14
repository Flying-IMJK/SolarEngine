

#include <cassert>
#include <sstream>

#include "Core/Platform/Win32/IncludeWindowsHeaders.h"
#include "Runtime/Engine.h"
#include "Editor/EditorApp.h"

#include "Resource.h"
#include "Core/Platform/Windows/WindowsFileSystem.h"

int Run(HINSTANCE hInstance, LPTSTR lpCmdLine)
{
	SE::Platform::PreInit(hInstance, IDI_SAMPLER);

	return SE::Engine::Main(lpCmdLine, &SE::Editor::EditorApp::Ins());
}

void SetLoadLibDir()
{
	SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_APPLICATION_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32 | LOAD_LIBRARY_SEARCH_USER_DIRS);

	SE::String exeDir = SE::FileSystem::GetCurrentProcessDirectory();

	// 解析用逗号分隔的路径列表
	SE::List<SE::String> splitLib;
	SE::String(EngineLoadLibDirList).Split(',', splitLib);

	for (auto lib : splitLib)
	{
		if (!lib.IsEmpty())
		{
			SE::String path = exeDir + SE_TEXT("/") + lib;
			if (SE::FileSystem::DirectoryExists(path))
			{
				assert(AddDllDirectory(path.Get()) != nullptr);
			}
		}
	}
}

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	SetLoadLibDir();

	__try
	{
		return Run(hInstance, lpCmdLine);
	}
	__except (SE::Platform::SehExceptionHandler(GetExceptionInformation()))
	{
		return -1;
	}
}
