#pragma once

#include "Runtime/Core/Types/Strings/String.h"
#include "Runtime/Core/Types/Collections/List.h"
#include "Runtime/Core/Types/Collections/Dictionary.h"

namespace SE
{
	/// <summary>
	/// Settings for new process.
	/// </summary>
	SE_STRUCT(API(NoConstructor))
	struct CreateProcessSettings
	{
		SCRIPTING_TYPE_MIN(CreateProcessSettings)
		/// <summary>
		/// The path to the executable file.
		/// </summary>
		SE_FIELD(API())
		String FileName;

		/// <summary>
		/// The custom arguments for command line.
		/// </summary>
		SE_FIELD(API())
		String Arguments;

		/// <summary>
		/// The custom folder path where start process. Empty if unused.
		/// </summary>
		SE_FIELD(API())
		String WorkingDirectory;

		/// <summary>
		/// True if capture process output and print to the log.
		/// </summary>
		SE_FIELD(API())
		bool LogOutput = true;

		/// <summary>
		/// True if capture process output and store it as Output text array.
		/// </summary>
		SE_FIELD(API())
		bool SaveOutput = false;

		/// <summary>
		/// True if wait for the process execution end.
		/// </summary>
		SE_FIELD(API())
		bool WaitForEnd = true;

		/// <summary>
		/// True if hint process to hide window. Supported only on Windows platform.
		/// </summary>
		SE_FIELD(API())
		bool HiddenWindow = true;

		/// <summary>
		/// True if use operating system shell to start the process. Supported only on Windows platform.
		/// </summary>
		SE_FIELD(API())
		bool ShellExecute = false;

		/// <summary>
		/// Custom environment variables to set for the process. Empty if unused. Additionally newly spawned process inherits this process vars which can be overriden here.
		/// </summary>
		Dictionary<String, String> Environment;

		/// <summary>
		/// Output process contents.
		/// </summary>
		SE_FIELD(API())
		List<Char> Output;
	};

}
