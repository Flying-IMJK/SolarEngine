

#include "MainContentTreeNode.h"

#include "ProjectTreeNode.h"
#include "Core/Platform/Windows/WindowsFileSystemWatcher.h"
#include "Editor/EditorApp.h"
#include "Editor/Modules/AssetDatabaseModule.h"

namespace SE::Editor
{
	void MainContentTreeNode::OnEvent(FileWatcherEvent& event)
	{
		EditorApp::Ins().databaseModule->OnDirectoryEvent(this, &event);
	}

	MainContentTreeNode::MainContentTreeNode(ProjectTreeNode* parent, ContentFolderType type, StringView path) : ContentTreeNode(parent, type, path)
	{
		_watcher = New<FileSystemWatcher>(path, true);
		_watcher->OnEvent.Bind<MainContentTreeNode, &MainContentTreeNode::OnEvent>(this);
	}

	void MainContentTreeNode::OnDestroy()
	{
		// _watcher.EnableRaisingEvents = false;
		Delete(_watcher);
		_watcher = nullptr;

		ContentTreeNode::OnDestroy();
	}

} // SE