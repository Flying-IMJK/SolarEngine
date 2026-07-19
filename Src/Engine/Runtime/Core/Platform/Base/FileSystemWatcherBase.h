#pragma once

#include "Runtime/Core/Types/Delegate.h"
#include "Runtime/Core/Types/Strings/String.h"

namespace SE
{

	/// <summary>
	/// Action types that file system watcher can listen for.
	/// </summary>
	enum class FileSystemAction
	{
		Unknown,
		Create,
		Delete,
		Modify,
		Rename
	};

	struct FileWatcherEvent
	{
		String path;
		// only use width Rename
		String oldPath;
		FileSystemAction action;
	};

	/// <summary>
	/// Base class for file system watcher objects.
	/// </summary>
	class SE_API_RUNTIME FileSystemWatcherBase
	{
		NON_COPYABLE(FileSystemWatcherBase)
	public:

		FileSystemWatcherBase(const String& directory, bool withSubDirs)
			: Directory(directory), WithSubDirs(withSubDirs), Enabled(true)
		{
		}

	public:

		/// <summary>
		/// The watcher directory path.
		/// </summary>
		const String Directory;

		/// <summary>
		/// The value whenever watcher is tracking changes in subdirectories.
		/// </summary>
		const bool WithSubDirs;

		/// <summary>
		/// The current watcher enable state.
		/// </summary>
		bool Enabled;

		/// <summary>
		/// 更改目录或文件时触发的操作。可以从 main 或其他线程调用，具体取决于平台。
		/// </summary>
		Delegate<FileWatcherEvent&> OnEvent;
	};

}