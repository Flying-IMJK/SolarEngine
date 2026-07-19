
#include "ImportFileEntry.h"
#include "ImportRequest.h"

#include "Runtime/Core/Platform/FileSystem.h"

namespace SE::Editor
{
	std::any ImportFileEntry::empty = std::any();

	ImportFileEntry::ImportFileEntry(ImportRequest& request)
	{
		SourceUrl = request.InputPath;
		ResultUrl = request.OutputPath;
	}

	void ImportFileEntry::ModifyResultFilename(String filename)
	{
		String directory = FileSystem::GetDirectoryName(ResultUrl);
		String extension = FileSystem::GetExtension(ResultUrl);
		ResultUrl = FileSystem::CombinePaths(directory, String::Format(SE_TEXT("{0}{1}"), filename, extension));
	}

	bool ImportFileEntry::Import()
	{
		// Skip if missing
		if (!FileSystem::DirectoryExists(SourceUrl) && !FileSystem::FileExists(SourceUrl))
			return false;

		// Setup output
		String folder = FileSystem::GetDirectoryName(ResultUrl);
		if (!folder.IsEmpty() && !FileSystem::DirectoryExists(folder))
		{
			FileSystem::CreateDirectory(folder);
		}

		if (FileSystem::DirectoryExists(SourceUrl))
		{
			// Copy directory
			return FileSystem::CopyDirectory(SourceUrl, ResultUrl, true);
		}

		// Copy file
		return FileSystem::CopyFile(SourceUrl, ResultUrl);;
	}

	StringView ImportFileEntry::GetSourceUrl()
	{
		return SourceUrl;
	}

	StringView ImportFileEntry::GetResultUrl()
	{
		return ResultUrl;
	}
} // SE