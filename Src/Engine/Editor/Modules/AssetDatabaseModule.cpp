
#include "AssetDatabaseModule.h"
#include "WindowsModule.h"

#include "Runtime/Core/Logging/Exception.h"
#include "Runtime/Core/Platform/FileSystem.h"
#include "Runtime/Core/Platform/FileSystemWatcher.h"
#include "Runtime/Core/Types/Collections/ListExtensions.h"
#include "Editor/EditorApp.h"
#include "Editor/Resource/Items/ContentFolder.h"
#include "Editor/Resource/Items/ContentItem.h"
#include "Editor/Resource/Items/BinaryAssetItem.h"
#include "Editor/Resource/Items/FileItem.h"
#include "Editor/Resource/Opreate/ContentOperate.h"
#include "Editor/Resource/Opreate/AssetOperate.h"
#include "Editor/Resource/Opreate/ModelOperate.h"
#include "Editor/Resource/Opreate/SceneOperate.h"
#include "Editor/Resource/Opreate/TextureOperate.h"
#include "Editor/Resource/Tree/MainContentTreeNode.h"
#include "Editor/Resource/Tree/ProjectTreeNode.h"
#include "Runtime/Resource/AssetContent.h"
#include "Runtime/EngineContext.h"
#include "Runtime/Project/ProjectInfo.h"

namespace SE::Editor
{
	void AssetDatabaseModule::OnContentAssetDisposing(Asset* asset)
	{
		// Handle deleted asset
		if (asset->ShouldDeleteFileOnUnload())
		{
			ContentItem* item = Find(asset->GetID());
			if (item != nullptr)
			{
				// Close all asset editors
				editor->windowsModule->CloseAllEditors(item);

				// Dispose
				item->Dispose();
			}
		}
	}

	AssetDatabaseModule::AssetDatabaseModule(EditorApp* editor) : EditorModule(editor)
	{
	}

	int AssetDatabaseModule::InitOrder()
	{
		return -80;
	}

	ProjectTreeNode* AssetDatabaseModule::GetProjectWorkspace(ProjectInfo* project)
	{
		auto predicate = CreateFunc([project](ProjectTreeNode* const &x)
		{
			return x->Project == project;
		});

		int index = ListExtensions::IndexOf(Projects, predicate);

		if (index == INVALID_INDEX)
		{
			return nullptr;
		}

		return Projects[index];
	}

	ContentOperate* AssetDatabaseModule::GetProxy(ContentItem* item)
	{
		if (item != nullptr)
		{
			for (int i = 0; i < Operate.Count(); i++)
			{
				if (Operate[i]->IsProxyFor(item))
				{
					return Operate[i];
				}
			}
		}

		return nullptr;
	}

	ContentOperate* AssetDatabaseModule::GetProxy(StringView extension)
	{
		if (extension.Length() <= 0)
		{
			return nullptr;
		}

		for (int i = 0; i < Operate.Count(); i++)
		{
			if (Operate[i]->FileExtension == extension)
			{
				return Operate[i];
			}
		}

		return nullptr;
	}

	AssetOperate* AssetDatabaseModule::GetAssetOperate(TypeID typeID, StringView path)
	{
		for (int i = 0; i < Operate.Count(); i++)
		{
			AssetOperate* operate = TypeTryCast<AssetOperate>(Operate[i]);
			if (operate != nullptr && operate->AcceptsAsset(typeID, path))
			{
				return operate;
			}
		}

		return nullptr;
	}

	AssetOperate* AssetDatabaseModule::GetAssetVirtualOpreate(StringView path)
	{
		for (int i = 0; i < Operate.Count(); i++)
		{
			AssetOperate* operate = TypeTryCast<AssetOperate>(Operate[i]);
			if (operate != nullptr && operate->IsVirtualOperate() && path.EndsWith(operate->FileExtension.operator->()))
			{
				return operate;
			}
		}

		return nullptr;
	}

	void AssetDatabaseModule::RefreshFolder(ContentItem* item, bool checkSubDirs)
	{
		// Peek folder to refresh
		ContentFolder* folder = item->IsFolder ? TypeTryCast<ContentFolder>(item) : item->ParentFolder;
		if (folder == nullptr)
			return;

		// Update
		LoadFolder(folder->Node, checkSubDirs);
	}

	ContentItem* AssetDatabaseModule::Find(StringView path)
	{
		if (path.Length() <= 0)
			return nullptr;

		// Ensure path is normalized to the Flax format
		String tempPath = path;
		FileSystem::NormalizePath(tempPath);

		// TODO: if it's a bottleneck try to optimize searching by spiting path

		for (auto project : Projects)
		{
			ContentItem* result = project->GetFolder()->Find(path);
			if (result != nullptr)
				return result;
		}

		return nullptr;
	}

	ContentItem* AssetDatabaseModule::Find(UID id)
	{
		if (id == UID::Empty)
			return nullptr;

		// TODO: use AssetInfo via Content manager to get asset path very quickly (it's O(1))

		// TODO: if it's a bottleneck try to optimize searching by caching items IDs

		for (auto project : Projects)
		{
			ContentItem* result = project->GetFolder()->Find(id);
			if (result != nullptr)
				return result;
		}
		return nullptr;
	}

	AssetItem* AssetDatabaseModule::FindAsset(UID id)
	{
		if (id == UID::Empty)
			return nullptr;

		// TODO: use AssetInfo via Content manager to get asset path very quickly (it's O(1))

		// TODO: if it's a bottleneck try to optimize searching by caching items IDs

		for (auto project : Projects)
		{
			if (project->Content != nullptr)
			{
				AssetItem* result = TypeTryCast<AssetItem>(project->Content->GetFolder()->Find(id));
				return result;
			}
		}
		return nullptr;
	}

	void AssetDatabaseModule::Move(List<ContentItem*> items, ContentFolder* newParent)
	{
		for (int i = 0; i < items.Count(); i++)
		{
			Move(items[i], newParent);
		}
	}

	void AssetDatabaseModule::Move(ContentItem* item, ContentFolder* newParent)
	{
		ENGINE_ASSERT(newParent != nullptr && item != nullptr)

		// Skip nothing to change
		if (item->ParentFolder == newParent)
			return;

		String extension = FileSystem::GetExtension(item->Path);
		String newPath = String::Format(SE_TEXT("{0}/{1}"), newParent->Path, item->ShortName + extension);
		Move(item, newPath);
	}

	void AssetDatabaseModule::Move(ContentItem* item, StringView newPath)
	{
		ENGINE_ASSERT(!(item == nullptr || newPath.Length() <= 0))

		if (item->IsFolder && FileSystem::DirectoryExists(newPath))
		{
			// MessageBox.Show("Cannot move folder. Target location already exists.");
			return;
		}

		// Find target parent
		StringView newDirPath = FileSystem::GetDirectoryName(newPath);
		ContentFolder* newParent = TypeTryCast<ContentFolder>(Find(newDirPath));
		if (newParent == nullptr)
		{
			// MessageBox.Show("Cannot move item. Missing target location.");
			return;
		}

		// Perform renaming
		{
			String oldPath = item->Path;

			// Special case for folders
			if (item->IsFolder)
			{
				// Cache data
				ContentFolder* folder = static_cast<ContentFolder*>(item);

				// Create new folder
				if (!FileSystem::CreateDirectory(newPath))
				{
					LOG_ERROR("Asset", "Cannot move folder \'{0}\' to \'{1}\'", oldPath, newPath);
					return;
				}

				// Change path
				item->UpdatePath(newPath);

				// Rename all child elements
				for (int i = 0; i < folder->Children.Count(); i++)
					UpdateAssetNewNameTree(folder->Children[i]);

				// Delete old folder
				if (!FileSystem::DeleteDirectory(oldPath, true))
				{
					LOG_ERROR("Asset", "Cannot remove folder \'{0}\'", oldPath);
				}
			}
			else
			{
				if (RenameAsset(item, newPath))
				{
					// MessageBox.Show("Cannot rename item.");
					return;
				}
			}

			if (item->ParentFolder != nullptr)
				item->ParentFolder->Node->SortChildren();
		}

		// Link item
		item->ParentFolder = newParent;

		if (_enableEvents)
			WorkspaceModified();
	}

	void AssetDatabaseModule::Copy(ContentItem* item, StringView targetPath)
	{
		if (item == nullptr || !item->Exists)
		{
			// MessageBox.Show("Cannot move item. It's missing.");
			return;
		}

		// Perform copy
		{
			StringView sourcePath = item->Path;

			// Special case for folders
			if (item->IsFolder)
			{
				// Cache data
				ContentFolder* folder = static_cast<ContentFolder*>(item);

				// Create new folder if missing
				if (!FileSystem::DirectoryExists(targetPath) && !FileSystem::CreateDirectory(targetPath))
				{
					LOG_ERROR("Asset", "Cannot copy folder \'{0}\' to \'{1}\'", sourcePath, targetPath);
				}

				// Copy all child elements
				for (int i = 0; i < folder->Children.Count(); i++)
				{
					ContentItem* child = folder->Children[i];
					String childExtension = FileSystem::GetExtension(child->Path);
					String childTargetPath = String::Format(SE_TEXT("{0}/{1}"), targetPath, child->ShortName + childExtension);
					Copy(folder->Children[i], childTargetPath);
				}
			}
			else
			{
				// Check if use content pool
				if (item->IsAsset || item->ItemType == ContentItemType::Scene)
				{
					// Rename asset
					// Note: we use content backend because file may be in use or sth, it's safe
					/*if (editor.ContentEditing.CloneAssetFile(sourcePath, targetPath, UID::New()))
					{
						LOG_ERROR("Asset", "Cannot copy asset \'{0}\' to \'{1}\'", sourcePath, targetPath);
					}*/
				}
				else
				{
					// Copy file
					if (!FileSystem::CopyFile(targetPath, sourcePath))
					{
						LOG_ERROR("Asset", "Cannot copy asset \'{0}\' to \'{1}\'", sourcePath, targetPath);
					}
				}
			}
		}
	}

	void AssetDatabaseModule::Delete(ContentItem* item, bool deletedByUser)
	{
		ENGINE_ASSERT(item != nullptr)

		// Fire events
		if (_enableEvents)
			ItemRemoved(item);

		item->OnDelete();
		_itemsDeleted++;

		StringView path = item->Path;

		// Special case for folders
		ContentFolder* folder = TypeTryCast<ContentFolder>(item);
		if (folder != nullptr)
		{
			// Delete all children
			if (folder->Children.Count() > 0)
			{
				List<ContentItem*> children = folder->Children;
				for (int i = 0; i < children.Count(); i++)
				{
					Delete(children[i], deletedByUser);
				}
			}

			// Remove directory
			if (deletedByUser && FileSystem::DirectoryExists(path))
			{
				// Flush files removal before removing folder (loaded assets remove file during object destruction in Asset::OnDeleteObject)
				// FlaxEngine.Scripting.FlushRemovedObjects();

				if (!FileSystem::DeleteDirectory(path, true))
				{
					LOG_ERROR("Asset", "Cannot remove folder \'{0}\'", path);
				}
			}

			// Unlink from the parent
			item->ParentFolder = nullptr;

			// Delete tree node
			folder->Node->Dispose();
		}
		else
		{
			// Check if it's an asset
			if (item->IsAsset)
			{
				// Delete asset by using content pool
				AssetContent::DeleteAsset(path);
			}
			else if (deletedByUser)
			{
				// Delete file
				if (FileSystem::FileExists(path))
					FileSystem::DeleteFile(path);
			}

			// Unlink from the parent
			item->ParentFolder = nullptr;

			// Delete item
			item->Dispose();
		}

		if (_enableEvents)
			WorkspaceModified();
	}

	void AssetDatabaseModule::AddProxy(ContentOperate* proxy, bool rebuild)
	{
		Operate.Insert(0, proxy);
		if (rebuild)
			Rebuild();
	}

	void AssetDatabaseModule::RemoveProxy(ContentOperate* proxy, bool rebuild)
	{
		Operate.Remove(proxy);
		if (rebuild)
			Rebuild();
	}

	void AssetDatabaseModule::Rebuild(bool immediate)
	{
		_rebuildFlag = true;
		if (immediate)
		{
			RebuildInternal();
		}
	}

	void AssetDatabaseModule::OnInit()
	{
		_CriticalSection = New<CriticalSection>();

		AssetContent::AssetDisposing.Bind<AssetDatabaseModule, &AssetDatabaseModule::OnContentAssetDisposing>(this);

		// Setup content proxies
		for (auto operateRegister : ContentOperateRegister::registers)
		{
			Operate.Add(operateRegister->Create());
		}

		// Settings
		/*Operate.Add(new SettingsProxy(typeof(GameSettings), Editor.Instance.Icons.GameSettings128));
		Operate.Add(new SettingsProxy(typeof(TimeSettings), Editor.Instance.Icons.TimeSettings128));
		Operate.Add(new SettingsProxy(typeof(LayersAndTagsSettings), Editor.Instance.Icons.LayersTagsSettings128));
		Operate.Add(new SettingsProxy(typeof(PhysicsSettings), Editor.Instance.Icons.PhysicsSettings128));
		Operate.Add(new SettingsProxy(typeof(GraphicsSettings), Editor.Instance.Icons.GraphicsSettings128));
		Operate.Add(new SettingsProxy(typeof(NetworkSettings), Editor.Instance.Icons.Document128));
		Operate.Add(new SettingsProxy(typeof(NavigationSettings), Editor.Instance.Icons.NavigationSettings128));
		Operate.Add(new SettingsProxy(typeof(LocalizationSettings), Editor.Instance.Icons.LocalizationSettings128));
		Operate.Add(new SettingsProxy(typeof(AudioSettings), Editor.Instance.Icons.AudioSettings128));
		Operate.Add(new SettingsProxy(typeof(BuildSettings), Editor.Instance.Icons.BuildSettings128));
		Operate.Add(new SettingsProxy(typeof(InputSettings), Editor.Instance.Icons.InputSettings128));
		Operate.Add(new SettingsProxy(typeof(StreamingSettings), Editor.Instance.Icons.BuildSettings128));
		Operate.Add(new SettingsProxy(typeof(WindowsPlatformSettings), Editor.Instance.Icons.WindowsSettings128));
		Operate.Add(new SettingsProxy(typeof(UWPPlatformSettings), Editor.Instance.Icons.UWPSettings128));
		Operate.Add(new SettingsProxy(typeof(LinuxPlatformSettings), Editor.Instance.Icons.LinuxSettings128));
		Operate.Add(new SettingsProxy(typeof(AndroidPlatformSettings), Editor.Instance.Icons.AndroidSettings128));
		Operate.Add(new SettingsProxy(typeof(MacPlatformSettings), Editor.Instance.Icons.AppleSettings128));
		Operate.Add(new SettingsProxy(typeof(iOSPlatformSettings), Editor.Instance.Icons.AppleSettings128));*/

		/*var typePS4PlatformSettings = TypeUtils.GetManagedType(GameSettings.PS4PlatformSettingsTypename);
		if (typePS4PlatformSettings != nullptr)
			Operate.Add(new SettingsProxy(typePS4PlatformSettings, Editor.Instance.Icons.PlaystationSettings128));

		var typeXboxOnePlatformSettings = TypeUtils.GetManagedType(GameSettings.XboxOnePlatformSettingsTypename);
		if (typeXboxOnePlatformSettings != nullptr)
			Operate.Add(new SettingsProxy(typeXboxOnePlatformSettings, Editor.Instance.Icons.XBOXSettings128));

		var typeXboxScarlettPlatformSettings = TypeUtils.GetManagedType(GameSettings.XboxScarlettPlatformSettingsTypename);
		if (typeXboxScarlettPlatformSettings != nullptr)
			Operate.Add(new SettingsProxy(typeXboxScarlettPlatformSettings, Editor.Instance.Icons.XBOXSettings128));

		var typeSwitchPlatformSettings = TypeUtils.GetManagedType(GameSettings.SwitchPlatformSettingsTypename);
		if (typeSwitchPlatformSettings != nullptr)
			Operate.Add(new SettingsProxy(typeSwitchPlatformSettings, Editor.Instance.Icons.SwitchSettings128));

		var typePS5PlatformSettings = TypeUtils.GetManagedType(GameSettings.PS5PlatformSettingsTypename);
		if (typePS5PlatformSettings != nullptr)
			Operate.Add(new SettingsProxy(typePS5PlatformSettings, Editor.Instance.Icons.PlaystationSettings128));

		// Last add generic json (won't override other json proxies)
		Operate.Add(new GenericJsonAssetProxy());*/

		// Create content folders nodes
		ProjectNode = New<ProjectTreeNode>(editor->Project);
		ProjectNode->Content = New<MainContentTreeNode>(ProjectNode, ContentFolderType::Content, EngineContext::ProjectContentFolder);

		/*if (editor->GameProject != editor->EngineProject)
		{
			GameNode = New<ProjectTreeNode>(editor->GameProject);
			GameNode->Content = New<MainContentTreeNode>(GameNode, ContentFolderType::Content, EngineContext::ProjectContentFolder);
			GameNode->Source = New<MainContentTreeNode>(GameNode, ContentFolderType::Source, EngineContext::ProjectSourceFolder);

			// TODO: why it's required? the code above should work for linking the nodes hierarchy
			GameNode->Content->GetFolder()->ParentFolder = GameNode->GetFolder();
			GameNode->Source->GetFolder()->ParentFolder = GameNode->GetFolder();
			Projects.Add(GameNode);
		}*/
		ProjectNode->Content->GetFolder()->ParentFolder = ProjectNode->GetFolder();
		Projects.Add(ProjectNode);

		RebuildInternal();

		/*Editor.ContentImporting.ImportFileEnd += (obj, failed) =>
		{
			var path = obj.ResultUrl;
			if (!failed)
				FlaxEngine.Scripting.InvokeOnUpdate(() => OnImportFileDone(path));
		};*/
		_enableEvents = true;
	}

	void AssetDatabaseModule::OnDirectoryEvent(MainContentTreeNode* node, FileWatcherEvent* event)
	{
		// Ensure to be ready for external events
		if (_isDuringFastSetup)
			return;

		// TODO: maybe we could make it faster! since we have a path so it would be easy to just create or delete given file. but remember about subdirectories

		// Switch type
		switch (event->action)
		{
		case FileSystemAction::Create:
		case FileSystemAction::Delete:
		case FileSystemAction::Rename:
		{
			_CriticalSection->Lock();
			_dirtyNodes.Add(node);
			_CriticalSection->Unlock();
			break;
		}
		}
	}

	void AssetDatabaseModule::OnUpdate()
	{
		// Update all dirty content tree nodes
		_CriticalSection->Lock();
		for (auto node : _dirtyNodes)
		{
			LoadFolder(node, true);

			if (_enableEvents)
				WorkspaceModified();
		}
		_dirtyNodes.Clear();
		_CriticalSection->Unlock();

		// Lazy-rebuilds
		if (_rebuildFlag)
			RebuildInternal();
	}

	void AssetDatabaseModule::OnExit()
	{
		AssetContent::AssetDisposing.Unbind<AssetDatabaseModule, &AssetDatabaseModule::OnContentAssetDisposing>(this);;

		// Disable events
		_enableEvents = false;

		// Cleanup
		for (auto operate : Operate)
		{
			operate->Dispose();
		}

		if (ProjectNode != nullptr)
		{
			ProjectNode->Dispose();
			ProjectNode = nullptr;
		}
		Operate.Clear();
	}

	bool AssetDatabaseModule::RenameAsset(ContentItem* el, StringView newPath)
	{
		String oldPath = el->Path;

		// Check if use content pool
		if (el->IsAsset)
		{
			// Rename asset
			// Note: we use content backend because file may be in use or sth, it's safe
			if (!AssetContent::RenameAsset(oldPath, newPath))
			{
				LOG_ERROR("Asset", "Cannot rename asset \'{0}\' to \'{1}\'", oldPath, newPath);
				return false;
			}
		}
		else
		{
			// Rename file
			if (!FileSystem::MoveFile(newPath, oldPath))
			{
				LOG_ERROR("Asset", "Cannot rename asset \'{0}\' to \'{1}\'", oldPath, newPath);
				return false;
			}
		}

		// Change path
		el->UpdatePath(newPath);
		return true;
	}

	void AssetDatabaseModule::UpdateAssetNewNameTree(ContentItem* el)
	{
		String extension = FileSystem::GetExtension(el->Path);
		String newPath = String::Format(SE_TEXT("{0}/{1}"), el->ParentFolder->Path, el->ShortName + extension);

		// Special case for folders
		if (el->IsFolder)
		{
			// Cache data
			StringView oldPath = el->Path;
			ContentFolder* folder = (ContentFolder*)el;

			// Create new folder
			if (!FileSystem::CreateDirectory(newPath))
			{
				LOG_ERROR("Asset", "Cannot move folder \'{0}\' to \'{1}\'", oldPath, newPath);
				return;
			}

			// Change path
			el->UpdatePath(newPath);

			// Rename all child elements
			for (int i = 0; i < folder->Children.Count(); i++)
			{
				UpdateAssetNewNameTree(folder->Children[i]);
			}
		}
		else
		{
			RenameAsset(el, newPath);
		}
	}

	void AssetDatabaseModule::OnImportFileDone(StringView path)
	{
		// Check if already has that element
		ContentItem* item = Find(path);
		BinaryAssetItem* binaryAssetItem = TypeTryCast<BinaryAssetItem>(item);
		if (binaryAssetItem != nullptr)
		{
			// Get asset info from the registry (content layer will update cache it just after import)
			AssetInfo assetInfo;
			if (AssetContent::GetAssetInfo(binaryAssetItem->Path, assetInfo))
			{
				// If asset type id has been changed we HAVE TO close all windows that use it
				// For eg. change texture to sprite atlas on reimport
				if (binaryAssetItem->typeID.operator->() != assetInfo.typeID)
				{
					ContentFolder* toRefresh = binaryAssetItem->ParentFolder;
					OnAssetTypeInfoChanged(binaryAssetItem, assetInfo);

					// Refresh the parent folder to find the new asset (it should have different type or some other format)
					RefreshFolder(toRefresh, false);
				}
				else
				{
					// Refresh element data that could change during importing
					binaryAssetItem->OnReimport(assetInfo.id);
				}
			}

			// Refresh content view (not the best design because window could also track this event but it gives better performance)
			// editor->windowsModule.ContentWin?.RefreshView();
		}
	}

	void AssetDatabaseModule::OnAssetTypeInfoChanged(AssetItem* assetItem, AssetInfo& assetInfo)
	{
		// Asset type has been changed!
		LOG_WARNING("Asset", "Asset \'{0}\' changed type from {1} to {2}", assetItem->Path, assetItem->typeID.operator->(), assetInfo.typeID);

		editor->windowsModule->CloseAllEditors(assetItem);

		// Remove this item from the database and some related data
		assetItem->Dispose();
		assetItem->ParentFolder->Children.Remove(assetItem);

		// Delete old thumbnail and remove it from the cache
		if (!assetItem->HasDefaultThumbnail)
		{
			// editor.Thumbnails.DeletePreview(assetItem);
		}
	}

	void AssetDatabaseModule::RebuildInternal()
	{
		bool enableEvents = _enableEvents;
		if (enableEvents)
		{
			WorkspaceRebuilding();
		}

		// Profiler.BeginEvent("ContentDatabase.Rebuild");
		double startTime = Platform::GetTimeSeconds();
		_rebuildFlag = false;
		_enableEvents = false;

		// Load all folders
		// TODO: we should create async task for gathering content and whole workspace contents if it takes too long
		// TODO: create progress bar in content window and after end we should enable events and update it
		_isDuringFastSetup = true;
		int startItems = _itemsCreated;
		for (ProjectTreeNode* project : Projects)
		{
			if (project->Content != nullptr)
				LoadFolder(project->Content, true);
			if (project->Source != nullptr)
				LoadFolder(project->Source, true);
		}
		_isDuringFastSetup = false;

		_enableEvents = enableEvents;
		double endTime = Platform::GetTimeSeconds();
		LOG_INFO("Asset", "Project database created in {0} ms. Items count: {1}", (int)((endTime - startTime) * 1000.0), _itemsCreated - startItems);
		// Profiler.EndEvent();

		if (enableEvents)
		{
			WorkspaceModified();
			WorkspaceRebuilt();
		}
	}

	void AssetDatabaseModule::Dispose(ContentItem* item)
	{
		if (_enableEvents)
			ItemRemoved(item);

		ContentFolder* folder = TypeTryCast<ContentFolder>(item);
		if (folder != nullptr)
		{
			if (folder->Children.Count() > 0)
			{
				List<ContentItem*>& children = folder->Children;
				for (int i = 0; i < children.Count(); i++)
				{
					Dispose(children[i]);
				}
			}

			item->ParentFolder = nullptr;
			folder->Node->Dispose();
		}
		else
		{
			item->ParentFolder = nullptr;
			item->Dispose();
		}
	}

	void AssetDatabaseModule::LoadFolder(ContentTreeNode* node, bool checkSubDirs)
	{
		if (node == nullptr)
			return;
		ContentFolder* folder = node->GetFolder();
		StringView path = folder->Path;
		bool canHaveAssets = node->GetCanHaveAssets();

		if (_isDuringFastSetup)
		{
			// Remove any spawned children
			for (int i = 0; i < folder->Children.Count(); i++)
			{
				Dispose(folder->Children[i]);
				Delete(folder->Children[i]);
				i--;
			}
		}
		else
		{
			// Check for missing files/folders (skip it during fast tree setup)
			for (int i = 0; i < folder->Children.Count(); i++)
			{
				AssetItem* childAsset;
				ContentItem* child = folder->Children[i];
				if (!child->Exists)
				{
					// Item doesn't exist anymore
					LOG_INFO("Asset", "Content item \'{0}\' has been removed", child->Path);
					Delete(child, false);
					i--;
				}
				else if (canHaveAssets && TypeTryCast<AssetItem>(child, childAsset))
				{
					// Check if asset type doesn't match the item proxy (eg. item reimported as Material Instance instead of Material)
					AssetInfo assetInfo;
					if (AssetContent::GetAssetInfo(child->Path, assetInfo))
					{
						bool changed = assetInfo.id != childAsset->id;
						if (!changed && assetInfo.typeID != childAsset->typeID)
						{
							// Use proxy check (eg. scene asset might accept different typename than AssetInfo reports)
							AssetOperate* proxy = GetAssetOperate(childAsset->typeID, child->Path);
							if (proxy == nullptr)
								proxy = GetAssetOperate(assetInfo.typeID, child->Path);
							changed = !proxy->AcceptsAsset(assetInfo.typeID, child->Path);
						}
						if (changed)
						{
							OnAssetTypeInfoChanged(childAsset, assetInfo);
							i--;
						}
					}
				}
			}
		}

		// Find files
		List<String> files;
		FileSystem::DirectoryGetFiles(files, path, SE_TEXT("*.*"), DirectorySearchOption::TopOnly);
		if (canHaveAssets)
		{
			LoadAssets(node, files);
		}
		/*if (node->CanHaveScripts)
		{
			LoadScripts(node, files);
		}*/

		// Get child directories
		List<String> childFolders;
		FileSystem::GetChildDirectories(childFolders, path);

		// Load child folders
		bool sortChildren = false;
		for (int i = 0; i < childFolders.Count(); i++)
		{
			String& childPath = childFolders[i];
			FileSystem::NormalizePath(childPath);

			// Check if node already has that element (skip during init when we want to walk project dir very fast)
			ContentFolder* childFolderNode = _isDuringFastSetup ? nullptr : TypeTryCast<ContentFolder>(node->GetFolder()->FindChild(childPath));
			if (childFolderNode == nullptr)
			{
				// Create node
				ContentTreeNode* n = New<ContentTreeNode>(node, childPath);
				if (!_isDuringFastSetup)
					sortChildren = true;

				// Load child folder
				LoadFolder(n, true);

				// Fire event
				if (_enableEvents)
				{
					ItemAdded(n->GetFolder());
					WorkspaceModified();
				}
				_itemsCreated++;
			}
			else if (checkSubDirs)
			{
				// Update child folder
				LoadFolder(childFolderNode->Node, true);
			}
		}
		if (sortChildren)
			node->SortChildren();

		// Ignore some special folders
		MainContentTreeNode* mainNode;
		if (TypeTryCast<MainContentTreeNode>(node, mainNode) && mainNode->GetFolder()->ShortName == SE_TEXT("Source"))
		{
			ContentFolder* mainNodeChild = TypeTryCast<ContentFolder>(mainNode->GetFolder()->Find(String::Format(SE_TEXT("{0}/{1}"), mainNode->GetPath(), SE_TEXT("obj"))));
			if (mainNodeChild != nullptr)
			{
				mainNodeChild->Visible = false;
				mainNodeChild->Node->Visible = false;
			}
			mainNodeChild = TypeTryCast<ContentFolder>(mainNode->GetFolder()->Find(String::Format(SE_TEXT("{0}/{1}"), mainNode->GetPath(), SE_TEXT("Properties"))));
			if (mainNodeChild != nullptr)
			{
				mainNodeChild->Visible = false;
				mainNodeChild->Node->Visible = false;
			}
		}
	}

	/*void AssetDatabaseModule::LoadScripts(ContentTreeNode* parent, String files[])
	{
		for (int i = 0; i < files->Length(); i++)
		{
			String path = files[i];
			FileSystem::NormalizePath(path);

			// Check if node already has that element (skip during init when we want to walk project dir very fast)
			if (_isDuringFastSetup || !parent->Folder->ContainsChild(path))
			{
#if PLATFORM_MAC
				if (path.EndsWith(".DS_Store", StringComparison.Ordinal))
					continue;
#endif

				// Create file item
				ContentItem item;
				if (path.EndsWith(".cs"))
					item = new CSharpScriptItem(path);
				else if (path.EndsWith(".cpp") || path.EndsWith(".h"))
					item = new CppScriptItem(path);
				else if (path.EndsWith(".shader") || path.EndsWith(".hlsl"))
					item = new ShaderSourceItem(path);
				else
					item = new FileItem(path);

				// Link
				item.ParentFolder = parent.Folder;

				// Fire event
				if (_enableEvents)
				{
					ItemAdded?.Invoke(item);
					WorkspaceModified?.Invoke();
					if (!path.EndsWith(".Gen.cs"))
					{
						if (item is ScriptItem)
							ScriptsBuilder.MarkWorkspaceDirty();
						if (item is ScriptItem || item is ShaderSourceItem)
							editor.CodeEditing.SelectedEditor.OnFileAdded(path);
					}
				}
				_itemsCreated++;
			}
		}
	}*/

	void AssetDatabaseModule::LoadAssets(ContentTreeNode* parent, List<String> &files)
	{
		for (int i = 0; i < files.Count(); i++)
		{
			String path = files[i];
			FileSystem::NormalizePath(path);

			// Check if node already has that element (skip during init when we want to walk project dir very fast)
			if (_isDuringFastSetup || !parent->GetFolder()->ContainsChild(path))
			{
#if PLATFORM_MAC
				if (path.EndsWith(".DS_Store", StringComparison.Ordinal))
					continue;
#endif

				// Create file item
				ContentItem* item = nullptr;
				AssetInfo assetInfo;
				if (AssetContent::GetAssetInfo(path, assetInfo))
				{
					StringView typeName = assetInfo.typeID.ToString();
					AssetOperate* proxy = GetAssetOperate(assetInfo.typeID, path);
					if (proxy != nullptr)
					{
						item = proxy->ConstructItem(path, assetInfo.typeID, assetInfo.id);
					}
				}
				if (item == nullptr)
				{
					AssetOperate* proxy = GetAssetVirtualOpreate(path);
					if (proxy != nullptr)
					{
						item = proxy->ConstructItem(path, assetInfo.typeID, assetInfo.id);
					}

					if (item == nullptr)
					{
						item = New<FileItem>(path);
					}
				}

				// Link
				item->ParentFolder = parent->GetFolder();

				// Fire event
				if (_enableEvents)
				{
					ItemAdded(item);
					WorkspaceModified();
				}
				_itemsCreated++;
			}
		}
	}

	void AssetDatabaseModule::LoadProjects(ProjectInfo* project)
	{
		ProjectTreeNode* workspace = GetProjectWorkspace(project);
		if (workspace == nullptr)
		{
			workspace = New<ProjectTreeNode>(project);
			Projects.Add(workspace);

			String contentFolder = FileSystem::CombinePaths(project->ProjectFolderPath, SE_TEXT("Content"));
			if (FileSystem::DirectoryExists(contentFolder))
			{
				workspace->Content = New<MainContentTreeNode>(workspace, ContentFolderType::Content, contentFolder);
				workspace->Content->GetFolder()->ParentFolder = workspace->GetFolder();
			}

			String sourceFolder = FileSystem::CombinePaths(project->ProjectFolderPath, SE_TEXT("Source"));
			if (FileSystem::DirectoryExists(sourceFolder))
			{
				workspace->Source = New<MainContentTreeNode>(workspace, ContentFolderType::Source, sourceFolder);
				workspace->Source->GetFolder()->ParentFolder = workspace->GetFolder();
			}
		}

		for (ProjectInfo::Reference& reference : project->References)
		{
			LoadProjects(reference.Project);
		}
	}
} // SE