#pragma once

#include "Runtime/Core/Types/Strings/String.h"
#include "Runtime/Core/Types/Collections/List.h"
#include "Runtime/Core/Types/Collections/Dictionary.h"

namespace SE
{
	/// <summary>
	/// Settings for new process.
	/// </summary>
	struct CreateProcessSettings
	{
		/// <summary>
		/// The path to the executable file.
		/// </summary>
		String FileName;

		/// <summary>
		/// The custom arguments for command line.
		/// </summary>
		String Arguments;

		/// <summary>
		/// The custom folder path where start process. Empty if unused.
		/// </summary>
		String WorkingDirectory;

		/// <summary>
		/// True if capture process output and print to the log.
		/// </summary>
		bool LogOutput = true;

		/// <summary>
		/// True if capture process output and store it as Output text array.
		/// </summary>
		bool SaveOutput = false;

		/// <summary>
		/// True if wait for the process execution end.
		/// </summary>
		bool WaitForEnd = true;

		/// <summary>
		/// True if hint process to hide window. Supported only on Windows platform.
		/// </summary>
		bool HiddenWindow = true;

		/// <summary>
		/// True if use operating system shell to start the process. Supported only on Windows platform.
		/// </summary>
		bool ShellExecute = false;

		/// <summary>
		/// Custom environment variables to set for the process. Empty if unused. Additionally newly spawned process inherits this process vars which can be overriden here.
		/// </summary>
		Dictionary<String, String> Environment;

		/// <summary>
		/// Output process contents.
		/// </summary>
		List<Char> Output;
	};

}