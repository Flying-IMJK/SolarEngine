
#include "FolderImportEntry.h"

#include "ImportRequest.h"
#include "Runtime/Core/Platform/Windows/WindowsFileSystem.h"
#include "Editor/EditorApp.h"
#include "Editor/Modules/AssetDatabaseModule.h"
#include "Editor/Modules/AssetImportingModule.h"
#include "Editor/Resource/Items/Contentfolder.h"

namespace SE::Editor
{
	FolderImportEntry::FolderImportEntry(ImportRequest& request): ImportFileEntry(request)
	{
		SkipSettingsDialog = request.SkipSettingsDialog;
	}

	bool FolderImportEntry::Import()
	{
		AssetDatabaseModule* assetDatabase = EditorApp::Ins().databaseModule;

		if (!FileSystem::DirectoryExists(ResultUrl))
		{
			FileSystem::CreateDirectory(ResultUrl);
			StringView parentPath = FileSystem::GetDirectoryName(ResultUrl);
			ContentItem* parent = assetDatabase->Find(parentPath);
			if (parent == nullptr)
			{
				LOG_WARNING("Asset", "Failed to find the parent folder for the imported directory.");
				return false;
			}
			assetDatabase->RefreshFolder(parent, true);
		}
		ContentFolder* target = static_cast<ContentFolder*>(assetDatabase->Find(ResultUrl));

		// Import all sub elements
		List<String> files;
		FileSystem::DirectoryGetFiles(files, SourceUrl, nullptr);
		EditorApp::Ins().importingModule->Import(files.Get(), files.Count(), target, SkipSettingsDialog);

		// Import all sub dirs
		List<String> folders;
		FileSystem::GetChildDirectories(folders, SourceUrl);
		EditorApp::Ins().importingModule->Import(folders.Get(), folders.Count(), target, SkipSettingsDialog);

		return true;
	}
} // SE