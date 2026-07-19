#pragma once

#include "Runtime/Core/Types/Strings/String.h"
#include "Runtime/Core/Platform/FileSystem.h"
#include "Runtime/EngineContext.h"

namespace SE
{
	// Small utility that uses temporary file to properly handle non-ANSI paths for 3rd party libs.
	struct AnsiPathTempFile
	{
		StringAnsi Path;
		String TempPath;
		bool Temp;

		AnsiPathTempFile(const String& path)
		{
			if (path.IsANSI() == false)
			{
				// Use temporary file
				TempPath = EngineContext::TemporaryFolder / UID::New().ToString(UID::FormatType::N);;
				if (TempPath.IsANSI() && !FileSystem::CopyFile(TempPath, path))
				{
					Path = TempPath.ToStringAnsi();
					return;
				}
				TempPath.Clear();
			}
			Path = path.ToStringAnsi();
		}

		~AnsiPathTempFile()
		{
			// Cleanup temporary file after use
			if (TempPath.HasChars())
				FileSystem::DeleteFile(TempPath);
		}
	};
}
