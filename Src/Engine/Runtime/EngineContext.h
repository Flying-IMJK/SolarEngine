#pragma once

#include "Runtime/API.h"
#include "Core/Types/Variable.h"

namespace SE
{
    class SE_API_RUNTIME EngineContext
    {
    public:
		// Paths

		// 引擎主目录路径。
		static String StartupFolder;

		// 临时路径.
		static String TemporaryFolder;

		// 项目路径
		static String ProjectFolder;

		/// <summary>
		/// The product local data directory.
		/// </summary>
		static String ProductLocalFolder;

		/// <summary>
		/// The game executable files location.
		/// </summary>
		static String BinariesFolder;

#if SE_EDITOR

		// 项目缓存文件路径 (editor-only).
    	static String ProjectCacheFolder;

    	// Game source code directory path (editor-only).
    	static String ProjectSourceFolder;

#endif

		// 项目内容目录路径
		static String ProjectContentFolder;

		/// <summary>
		/// The short name of the product (can be `Flax Editor` or name of the game e.g. `My Space Shooter`).
		/// </summary>
		static String ProductName;

		// State

		// True if fatal error occurred (engine is exiting)
		static bool FatalErrorOccurred;

		// True if engine need to be closed
		static bool IsRequestingExit;

		// Exit code
		static int32 ExitCode;
    };
}