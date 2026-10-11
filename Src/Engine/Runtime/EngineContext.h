#pragma once

#include <Runtime/API.h>
#include <Runtime/Core/Types/Variable.h>
#include <Runtime/Core/Scripting/ScriptingTypeDefine.h>

namespace SE
{
	SE_CLASS(API(Static))
    class SE_API_RUNTIME EngineContext
    {
		SCRIPTING_TYPE_MIN(EngineContext);
    public:
		// Paths

		// 引擎主目录路径。
        SE_FIELD(API())
		static String StartupFolder;

		// 临时路径.
		SE_FIELD(API())
		static String TemporaryFolder;

		// 项目路径
		SE_FIELD(API())
		static String ProjectFolder;

		/// <summary>
		/// The product local data directory.
		/// </summary>
		SE_FIELD(API())
		static String ProductLocalFolder;

		/// <summary>
		/// The game executable files location.
		/// </summary>
		SE_FIELD(API())
		static String BinariesFolder;


		// 项目缓存文件路径 (editor-only).
		SE_FIELD(API())
    	static String ProjectCacheFolder;

    	// Game source code directory path (editor-only).
    	SE_FIELD(API())
    	static String ProjectSourceFolder;


		// 项目内容目录路径
		SE_FIELD(API())
		static String ProjectContentFolder;

		/// <summary>
		/// The short name of the product (can be `Flax Editor` or name of the game e.g. `My Space Shooter`).
		/// </summary>
		SE_FIELD(API())
		static String ProductName;

		// State

		// True if fatal error occurred (engine is exiting)
		SE_FIELD(API())
		static bool FatalErrorOccurred;

		// True if engine need to be closed
		SE_FIELD(API())
		static bool IsRequestingExit;

		// Exit code
		SE_FIELD(API())
		static int32 ExitCode;
    };
}