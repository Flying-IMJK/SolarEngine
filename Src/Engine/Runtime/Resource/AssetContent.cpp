
#include "AssetContent.h"
#include "AssetsCache.h"
#include "spirv_common.hpp"

#include "Runtime/Core/Systems.h"
#include "Runtime/Core/Profiler/Profiler.h"
#include "Runtime/Core/Thread/Threading.h"
#include "Runtime/Core/Platform/FileSystem.h"
#include "../Utilities/Time.h"
#include "Runtime/Resource/Storage/AssetStorages.h"
#include "Runtime/Resource/Storage/JsonStorageProxy.h"
#include "Runtime/EngineContext.h"
#include "Runtime/Resource/Factories/IAssetFactory.h"
#include "Runtime/Core/TypeSystem/Types.h"

namespace SE
{
	TimeSpan AssetContent::AssetsUpdateInterval = TimeSpan::FromMilliseconds(500);
	TimeSpan AssetContent::AssetsUnloadInterval = TimeSpan::FromSeconds(10);
	Delegate<Asset*> AssetContent::AssetDisposing;
	Delegate<Asset*> AssetContent::AssetReloading;

	List<AssetFactoryRegister*>& IAssetFactory::GetRegisterFactoryList()
	{
		static List<AssetFactoryRegister*> factoryRegister;

		return factoryRegister;
	}


	struct AssetContentData
	{
		// Assets
		CriticalSection AssetsLocker;
		Dictionary<UID, Asset*> Assets = Dictionary<UID, Asset*>(2048);
		List<UID> LoadCallAssets = List<UID>(PLATFORM_THREADS_LIMIT);
		CriticalSection LoadedAssetsToInvokeLocker;
		List<Asset*> LoadedAssetsToInvoke = List<Asset*>(64);
		List<Asset*> ToUnload;

		// Assets Registry Stuff
		AssetsCache Cache;

		// Unloading assets
		Dictionary<Asset*, TimeSpan> UnloadQueue;
		TimeSpan LastUnloadCheckTime = TimeSpan(0);
		bool IsExiting = false;

		IAssetFactory::Collection Factories = IAssetFactory::Collection(1024);

#if ENABLE_ASSETS_DISCOVERY
		DateTime LastWorkspaceDiscovery;
		CriticalSection WorkspaceDiscoveryLocker;
#endif
	} * contentData;

	class AssetContentSystem : public ISystem
	{
		ENGINE_SYSTEM(AssetContentSystem)
	public:
		AssetContentSystem() : ISystem(SE_TEXT("AssetContent"), -600)
		{
		}

		bool OnInit() override;
		void OnUpdate() override;
		void OnLateUpdate() override;
		void OnDispose() override;
	};

	ENGINE_SYSTEM_REGISTER(AssetContentSystem);


	bool AssetContentSystem::OnInit()
	{
		contentData = New<AssetContentData>();

		// 注册所有资源Factory
		for (AssetFactoryRegister* factoryRegister : IAssetFactory::GetRegisterFactoryList())
		{
			if (factoryRegister == nullptr)
			{
				continue;
			}
			TypeID assetType = factoryRegister->GetAssetType();
			IAssetFactory* factory = factoryRegister->Create();

			LOG_INFO("Assets", "Register Asset {0} Factory", assetType.ToString());

			contentData->Factories.Add(assetType, factory);
		}

		// Load assets registry
		contentData->Cache.Init();

		return false;
	}

	void AssetContentSystem::OnUpdate()
	{
		PROFILE_CPU();

		Threading::ScopeLock lock(contentData->LoadedAssetsToInvokeLocker);

		// Broadcast `OnLoaded` events
		while (contentData->LoadedAssetsToInvoke.HasItems())
		{
			auto asset = contentData->LoadedAssetsToInvoke.Dequeue();
			asset->onLoaded_MainThread();
		}
	}

	void AssetContentSystem::OnLateUpdate()
	{
		PROFILE_CPU();

		// Check if need to perform an update of unloading assets
		const TimeSpan timeNow = Time::Update.UnscaledTime;
		if (timeNow - contentData->LastUnloadCheckTime < AssetContent::AssetsUpdateInterval)
			return;
		contentData->LastUnloadCheckTime = timeNow;
		contentData->AssetsLocker.Lock();

		// Verify all assets
		for (auto i = contentData->Assets.begin(); i.IsNotEnd(); ++i)
		{
			Asset* asset = i->Value;

			// Check if has no references and is not during unloading
			if (asset->GetReferencesCount() <= 0 && !contentData->UnloadQueue.ContainsKey(asset))
			{
				// Add to removes
				contentData->UnloadQueue.Add(asset, timeNow);
			}
		}

		// Find assets to unload in unload queue
		contentData->ToUnload.Clear();
		for (auto i = contentData->UnloadQueue.begin(); i != contentData->UnloadQueue.end(); ++i)
		{
			// Check if asset gain any new reference or if need to unload it
			if (i->Key->GetReferencesCount() > 0 || timeNow - i->Value >= AssetContent::AssetsUnloadInterval)
			{
				contentData->ToUnload.Add(i->Key);
			}
		}

		// Unload marked assets
		for (int32 i = 0; i < contentData->ToUnload.Count(); i++)
		{
			Asset* asset = contentData->ToUnload[i];

			// Check if has no references
			if (asset->GetReferencesCount() <= 0)
			{
				AssetContent::UnloadAsset(asset);
			}

			// Remove from unload queue
			contentData->UnloadQueue.Remove(asset);
		}

		contentData->AssetsLocker.Unlock();

		// Update cache (for longer sessions it will help to reduce cache misses)
		contentData->Cache.Save();
	}

	void AssetContentSystem::OnDispose()
	{
		contentData->IsExiting = true;

		// Save assets registry before engine closing
		contentData->Cache.Save();

		// Flush objects (some asset-related objects/references may be pending to delete)
//		ObjectsRemovalService::Flush();

		// Unload all remaining assets
		{
			Threading::ScopeLock lock(contentData->AssetsLocker);

			for (auto i = contentData->Assets.begin(); i.IsNotEnd(); ++i)
			{
				i->Value->DeleteObject();
			}
		}

		// Flush objects (some assets may be pending to delete)
//		ObjectsRemovalService::Flush();

		// NOW dispose graphics device - where there is no loaded assets at all
//		Graphics::DisposeDevice();
	}

	AssetsCache* AssetContent::GetRegistry()
	{
		return &contentData->Cache;
	}

	bool AssetContent::GetAssetInfo(const UID& id, AssetInfo& info)
	{
		if (!id.IsValid())
			return false;

#if ENABLE_ASSETS_DISCOVERY
		// Find asset in registry
		if (contentData->Cache.FindAsset(id, info))
			return true;
		PROFILE_CPU();

		// Locking injects some stalls but we need to make it safe (only one thread can pass though it at once)
		Threading::ScopeLock lock(contentData->WorkspaceDiscoveryLocker);

		// Check if we can search workspace
		// Note: we want to limit searching frequency due to high I/O usage and thread stall
		// We also perform full workspace discovery so all new assets will be found
		auto now = DateTime::NowUTC();
		auto diff = now - contentData->LastWorkspaceDiscovery;
		if (diff <= TimeSpan::FromSeconds(5))
		{
			//LOG_WARNING("Resource", "Cannot perform workspace scan for '{1}'. Too often call. Time diff: {0} ms", static_cast<int32>(diff.GetTotalMilliseconds()), id);
			return false;
		}
		contentData->LastWorkspaceDiscovery = now;

		// Try to find an asset within the project, engine, plugins workspace folders
		DateTime startTime = now;
		int32 startCount = contentData->Cache.Size();
		List<String> tmpCache(1024);
#if SE_EDITOR
//		HashSet<const ProjectInfo*> projects;
		bool found = false; //FindAssets(Editor::Project, projects, id, tmpCache, info);
#else
		bool found = findAsset(id, EngineContext::ProjectContentFolder, tmpCache, info);
#endif
		if (found)
		{
			LOG_INFO("Resource",
				"Workspace searching time: {0} ms, new assets found: {1}",
				static_cast<int32>((DateTime::NowUTC() - startTime).GetTotalMilliseconds()),
				contentData->Cache.Size() - startCount);
			return true;
		}

		LOG_WARNING("Resource", "Cannot find {0}.", id);
		return false;
#else
		// Find asset in registry
		return contentData->Cache.FindAsset(id, info);
#endif
	}

	bool AssetContent::GetAssetInfo(const StringView& path, AssetInfo& info)
	{
#if ENABLE_ASSETS_DISCOVERY
		// Find asset in registry
		if (contentData->Cache.FindAsset(path, info))
		{
			return true;
		}

		if (!FileSystem::FileExists(path))
		{
			return false;
		}
		PROFILE_CPU();

		const auto extension = FileSystem::GetExtension(path).ToLower();

		// Check if it's a binary asset
		if (AssetStorages::IsStorageExtension(extension))
		{
			// Skip packages in editor (results in conflicts with build game packages if deployed inside project folder)
#if SE_EDITOR
			if (extension == PACKAGE_FILES_EXTENSION)
				return false;
#endif

			// Open storage
			auto storage = AssetStorages::GetStorage(path);
			if (storage)
			{
				// Register assets from the storage container (will handle duplicated IDs)
				contentData->Cache.RegisterAssets(storage);
				return contentData->Cache.FindAsset(path, info);
			}
		}
		// Check for json resource
		else if (JsonStorageProxy::IsValidExtension(extension))
		{
			// Check Json storage layer
			UID jsonId;
			TypeID jsonTypeID;
			if (JsonStorageProxy::GetAssetInfo(path, jsonId, jsonTypeID))
			{
				// Register asset
				contentData->Cache.RegisterAsset(jsonId, jsonTypeID, path);
				return contentData->Cache.FindAsset(path, info);
			}
		}

    	return false;
#else
		// Find asset in registry
		return contentData->Cache.FindAsset(path, info);
#endif
	}

	String AssetContent::GetEditorAssetPath(const UID& id)
	{
		return contentData->Cache.GetEditorAssetPath(id);
	}

	List<UID> AssetContent::GetAllAssets()
	{
		List<UID> result;
		contentData->Cache.GetAll(result);
		return result;
	}

	List<UID> AssetContent::GetAllAssetsByType(const TypeID type)
	{
		List<UID> result;
		contentData->Cache.GetAllByTypeName(type, result);
		return result;
	}

	IAssetFactory* AssetContent::GetAssetFactory(const TypeID type)
	{
		IAssetFactory* result = nullptr;
		contentData->Factories.TryGet(type, result);
		return result;
	}

	IAssetFactory* AssetContent::GetAssetFactory(const AssetInfo& assetInfo)
	{
		StringView t = assetInfo.typeID.ToString();
		IAssetFactory* result = nullptr;
		if (!contentData->Factories.TryGet(assetInfo.typeID, result))
		{
			// Check if it's a json asset (in editor only)
			// In build game all asset factories are valid and json assets are cooked into binary packages
#if SE_EDITOR
			if (assetInfo.path.EndsWith(SE_TEXT(".json")))
#endif
			{
//				IAssetFactory::Get().TryGet(JsonAsset::TypeName, result);
			}
		}

		return result;
	}

	String AssetContent::CreateTemporaryAssetPath()
	{
		return EngineContext::TemporaryFolder / (UID::New().ToString(UID::FormatType::N) + ASSET_FILES_EXTENSION_WITH_DOT);
	}

	ContentStats AssetContent::GetStats()
	{
		ContentStats stats;
		contentData->AssetsLocker.Lock();
		stats.AssetsCount = contentData->Assets.Count();
		int32 loadFailedCount = 0;
		for (const auto& e : contentData->Assets)
		{
			if (e.Value->IsLoaded())
				stats.LoadedAssetsCount++;
			else if (e.Value->LastLoadFailed())
				loadFailedCount++;
			if (e.Value->IsVirtual())
				stats.VirtualAssetsCount++;
		}
		stats.LoadingAssetsCount = stats.AssetsCount - loadFailedCount - stats.LoadedAssetsCount;
		contentData->AssetsLocker.Unlock();
		return stats;
	}

/*
	Asset* AssetContent::LoadAsyncInternal(const StringView& internalPath, const MClass* type)
	{
		CHECK_RETURN(type, nullptr);
		const auto scriptingType = Scripting::FindScriptingType(type->GetFullName());
		if (scriptingType)
			return LoadAsyncInternal(internalPath, scriptingType);
		LOG_ERROR("Resource", "Failed to find asset type '{0}'.", String(type->GetFullName()));
		return nullptr;
	}
	*/

	Asset* AssetContent::LoadAsyncInternal(StringView internalPath, const TypeID& type)
	{
#if SE_EDITOR
		const String path = EngineContext::ProjectCacheFolder / internalPath + ASSET_FILES_EXTENSION_WITH_DOT;
	    if (!FileSystem::FileExists(path))
	    {
	        LOG_ERROR("Resource", "Missing file \'{0}\'", path);
	        return nullptr;
	    }
#else
		const String path = EngineContext::ProjectContentFolder / internalPath + ASSET_FILES_EXTENSION_WITH_DOT;
#endif

		const auto asset = LoadAsync(path, type);
		if (asset == nullptr)
		{
			LOG_ERROR("Resource", "Failed to load \'{0}\' (type: {1})", internalPath, type.ToString());
		}

		return asset;
	}


	Asset* AssetContent::LoadAsyncInternal(const Char* internalPath, const TypeID& type)
	{
		return LoadAsyncInternal(StringView(internalPath), type);
	}

	/*Asset* LoadAsset(const SGUID& id, const ScriptingTypeHandle& type)
	{
		return AssetContent::LoadAsync(id, type);
	}*/


/*	Asset* AssetContent::LoadAsync(const StringView& path, const MClass* type)
	{
		CHECK_RETURN(type, nullptr);
		const auto scriptingType = Scripting::FindScriptingType(type->GetFullName());
		if (scriptingType)
			return LoadAsync(path, scriptingType);
		LOG_ERROR("Resource", "Failed to find asset type '{0}'.", String(type->GetFullName()));
		return nullptr;
	}*/

	Asset* AssetContent::LoadAsync(const StringView& path, const TypeID& type)
	{
		// Ensure path is in a valid format
		String pathNorm(path);
		AssetStorages::FormatPath(pathNorm);
		const StringView filePath = pathNorm;

#if SE_EDITOR
		if (!FileSystem::FileExists(filePath))
		{
			LOG_ERROR("Resource", "Missing file \'{0}\'", filePath);
			return nullptr;
		}
#endif

		AssetInfo assetInfo;
		if (GetAssetInfo(filePath, assetInfo))
		{
			return LoadAsync(assetInfo.id, type);
		}

		return nullptr;
	}

	List<Asset*> AssetContent::GetAssets()
	{
		List<Asset*> assets;
		contentData->Assets.GetValues(assets);
		contentData->AssetsLocker.Unlock();
		return assets;
	}

	const Dictionary<UID, Asset*>& AssetContent::GetAssetsRaw()
	{
		contentData->AssetsLocker.Lock();
		contentData->AssetsLocker.Unlock();
		return contentData->Assets;
	}

/*	Asset* AssetContent::LoadAsync(const SGUID& id, const MClass* type)
	{
		CHECK_RETURN(type, nullptr);
		const auto scriptingType = Scripting::FindScriptingType(type->GetFullName());
		if (scriptingType)
			return LoadAsync(id, scriptingType);
		LOG_ERROR("Resource", "Failed to find asset type '{0}'.", String(type->GetFullName()));
		return nullptr;
	}*/

	Asset* AssetContent::GetAsset(const StringView& outputPath)
	{
		if (outputPath.IsEmpty())
			return nullptr;

		Threading::ScopeLock lock(contentData->AssetsLocker);
		for (auto i = contentData->Assets.begin(); i.IsNotEnd(); ++i)
		{
			if (i->Value->GetPath() == outputPath)
			{
				return i->Value;
			}
		}
		return nullptr;
	}

	Asset* AssetContent::GetAsset(const UID& id)
	{
		Asset* result = nullptr;
		contentData->AssetsLocker.Lock();
		contentData->Assets.TryGet(id, result);
		contentData->AssetsLocker.Unlock();
		return result;
	}

	void AssetContent::DeleteAsset(Asset* asset)
	{
		if (asset == nullptr || asset->m_DeleteFileOnUnload)
			return;

		LOG_INFO("Resource", "Deleting asset {0}...", asset->ToString());

		// Ensure that asset is loaded (easier than cancel in-flight loading)
		asset->WaitForLoaded();

		// Mark asset for delete queue (delete it after auto unload)
		asset->m_DeleteFileOnUnload = true;

		// Unload
		asset->DeleteObject();
	}

	void AssetContent::DeleteAsset(const StringView& path)
	{
		PROFILE_CPU();

		// Try to delete already loaded asset
		Asset* asset = GetAsset(path);
		if (asset != nullptr)
		{
			DeleteAsset(asset);
			return;
		}

		Threading::ScopeLock locker(contentData->AssetsLocker);

		// Remove from registry
		AssetInfo info;
		if (contentData->Cache.DeleteAsset(path, &info))
		{
			LOG_INFO("Resource", "Deleting asset '{0}':{1}({2})", path, info.id, info.typeID);
		}
		else
		{
			LOG_INFO("Resource", "Deleting asset '{0}':{1}({2})", path, SE_TEXT("?"), SE_TEXT("?"));
			info.id = UID::Empty;
		}

		// Delete file
		DeleteFileSafety(path, info.id);
	}

	void AssetContent::DeleteFileSafety(const StringView& path, const UID& id)
	{
		if (!id.IsValid())
		{
			LOG_WARNING("Resource", "Cannot remove file \'{0}\'. Given ID is invalid.", path);
			return;
		}
		PROFILE_CPU();

		// Ensure that file has the same ID (prevent from deleting different assets)
		auto storage = AssetStorages::TryGetStorage(path);
		if (storage)
		{
			storage->CloseFileHandles(); // Close file handle to allow removing it
			if (!storage->HasAsset(id))
			{
				LOG_WARNING("Resource", "Cannot remove file \'{0}\'. It doesn\'t contain asset {1}.", path, id);
				return;
			}
		}

#if PLATFORM_WINDOWS || PLATFORM_LINUX
		// Safety way - move file to the recycle bin
		if (FileSystem::MoveFileToRecycleBin(path))
		{
			LOG_WARNING("Resource", "Failed to move file to Recycle Bin. Path: \'{0}\'", path);
		}
#else
		// Remove file
		if (FileSystem::DeleteFile(path))
		{
			LOG_WARNING("Resource", "Failed to delete file Path: \'{0}\'", path);
		}
#endif
	}

#if SE_EDITOR

	bool AssetContent::RenameAsset(const StringView& oldPath, const StringView& newPath)
	{
		ENGINE_ASSERT(Threading::IsMainThread());
	
		// contentData->Cache data
		Asset* oldAsset = GetAsset(oldPath);
		Asset* newAsset = GetAsset(newPath);
	
		// Validate name
		if (newAsset != nullptr && newAsset != oldAsset)
		{
			LOG_ERROR("Resource", "Invalid name '{0}' when trying to rename '{1}'.", newPath, oldPath);
			return false;
		}
	
		// Ensure asset is ready for renaming
		if (oldAsset)
		{
			// Wait for data loaded
			if (!oldAsset->WaitForLoaded())
			{
				LOG_ERROR("Resource", "Failed to load asset '{0}'.", oldAsset->ToString());
				return false;
			}
	
			// Unload
			// Don't unload asset fully, only release ref to file, don't call OnUnload so managed asset and all refs will remain alive
			oldAsset->ReleaseStorage();
			//oldAsset->onUnload_MainThread();
			//ScopeLock lock(oldAsset->Locker);
			//oldAsset->unload(true);
		}
	
		// Ensure to unlock file
		AssetStorages::EnsureAccess(oldPath);
	
		// Move file
		if (!FileSystem::MoveFile(newPath, oldPath))
		{
			LOG_ERROR("Resource", "Cannot move file '{0}' to '{1}'", oldPath, newPath);
			return false;
		}
	
		// Update cache
		contentData->Cache.RenameAsset(oldPath, newPath);
		AssetStorages::OnRenamed(oldPath, newPath);
	
		// Check if is loaded
		if (oldAsset)
		{
			// Rename internal call
			oldAsset->onRename(newPath);
	
			// Load
			// Threading::ScopeLock lock(oldAsset->Locker);
			// oldAsset->StartLoadAsset();
		}
	
		return true;
	}
	
	bool AssetContent::FastTmpAssetClone(const StringView& path, String& resultPath)
	{
		ENGINE_ASSERT(path.HasChars());
	
		const String dstPath = EngineContext::TemporaryFolder / UID::New().ToString(UID::FormatType::D) + ASSET_FILES_EXTENSION_WITH_DOT;
	
		if (CloneAssetFile(dstPath, path, UID::New()))
			return true;
	
		resultPath = dstPath;
		return false;
	}
	
	class CloneAssetFileTask : public Threading::MainThreadTask
	{
	public:
		StringView dstPath;
		StringView srcPath;
		UID dstId;
		bool* output;
	
	protected:
		bool Run() override
		{
			*output = AssetContent::CloneAssetFile(dstPath, srcPath, dstId);
			return false;
		}
	};
	
	bool AssetContent::CloneAssetFile(const StringView& dstPath, const StringView& srcPath, const UID& dstId)
	{
		// Best to run this on the main thread to avoid clone conflicts.
		if (Threading::IsMainThread())
		{
			PROFILE_CPU();
			ENGINE_ASSERT(FileSystem::AreFilePathsEqual(srcPath, dstPath) == false && dstId.IsValid());
	
			LOG_INFO("Resource", "Cloning asset \'{0}\' to \'{1}\'({2}).", srcPath, dstPath, dstId);
	
			// Check source file
			if (!FileSystem::FileExists(srcPath))
			{
				LOG_WARNING("Resource", "Missing source file.");
				return true;
			}
	
			// Special case for json resources
			if (JsonStorageProxy::IsValidExtension(FileSystem::GetExtension(srcPath).ToLower()))
			{
				if (FileSystem::CopyFile(dstPath, srcPath))
				{
					LOG_WARNING("Resource", "Cannot copy file to destination.");
					return true;
				}
				if (JsonStorageProxy::ChangeId(dstPath, dstId))
				{
					LOG_WARNING("Resource", "Cannot change asset ID.");
					return true;
				}
				return false;
			}
	
			// Check if destination file is missing
			if (!FileSystem::FileExists(dstPath))
			{
				// Use quick file copy
				if (FileSystem::CopyFile(dstPath, srcPath))
				{
					LOG_WARNING("Resource", "Cannot copy file to destination.");
					return true;
				}
	
				// Change ID
				auto storage = AssetStorages::GetStorage(dstPath);
				Storage::Entry e;
				storage->GetEntry(0, e);
				if (storage == nullptr || storage->ChangeAssetID(e, dstId))
				{
					LOG_WARNING("Resource", "Cannot change asset ID.");
					return true;
				}
			}
			else
			{
				// Use temporary file
				String tmpPath = EngineContext::TemporaryFolder / UID::New().ToString(UID::FormatType::D);
				if (FileSystem::CopyFile(tmpPath, srcPath))
				{
					LOG_WARNING("Resource", "Cannot copy file.");
					return true;
				}
	
				// Change asset ID
				{
					auto storage = AssetStorages::GetStorage(tmpPath);
					if (!storage)
					{
						LOG_WARNING("Resource", "Cannot change asset ID.");
						return true;
					}
					Storage::Entry e;
					storage->GetEntry(0, e);
					if (storage->ChangeAssetID(e, dstId))
					{
						LOG_WARNING("Resource", "Cannot change asset ID.");
						return true;
					}
				}
	
				// Unlock destination file
				AssetStorages::EnsureAccess(dstPath);
	
				// Copy temp file to the destination
				if (FileSystem::CopyFile(dstPath, tmpPath))
				{
					LOG_WARNING("Resource", "Cannot copy file to destination.");
					return true;
				}
	
				// Cleanup
				FileSystem::DeleteFile(tmpPath);
	
				// Reload storage
				if (auto storage = AssetStorages::GetStorage(dstPath))
				{
					storage->Reload();
				}
			}
		}
		else
		{
			CloneAssetFileTask* task = New<CloneAssetFileTask>();
			task->dstId = dstId;
			task->dstPath = dstPath;
			task->srcPath = srcPath;
	
			bool result = false;
			task->output = &result;
			task->Start();
			task->Wait();
	
			return result;
		}
	
		return false;
	}
	
	#endif

	void AssetContent::UnloadAsset(Asset* asset)
	{
		if (asset == nullptr)
			return;
		asset->DeleteObject();
	}

	/*	Asset* AssetContent::CreateVirtualAsset(const MClass* type)
	{
		CHECK_RETURN(type, nullptr);
		const auto scriptingType = Scripting::FindScriptingType(type->GetFullName());
		if (scriptingType)
			return CreateVirtualAsset(scriptingType);
		LOG_ERROR("Resource", "Failed to find asset type '{0}'.", String(type->GetFullName()));
		return nullptr;
	}*/

	Asset* AssetContent::CreateVirtualAsset(const TypeID& type)
	{
		PROFILE_CPU();

		// Init mock asset info
		AssetInfo info;
		info.id = UID::New();
		info.typeID = type;
		info.path = CreateTemporaryAssetPath();

		// Find asset factory based in its type
		auto factory = GetAssetFactory(info);
		if (factory == nullptr)
		{
			LOG_ERROR("Resource", "Cannot find virtual asset factory.");
			return nullptr;
		}
		if (!factory->SupportsVirtualAssets())
		{
			LOG_ERROR("Resource", "Cannot create virtual asset of type '{0}'.", info.typeID);
			return nullptr;
		}

		// Create asset object
		auto asset = factory->NewVirtual(&info);
		if (asset == nullptr)
		{
			LOG_ERROR("Resource", "Cannot create virtual asset object.");
			return nullptr;
		}

		// Call initializer function
		asset->InitAsVirtual();

		// Register asset
		contentData->AssetsLocker.Lock();
		ENGINE_ASSERT(!contentData->Assets.ContainsKey(asset->GetID()));
		contentData->Assets.Add(asset->GetID(), asset);
		contentData->AssetsLocker.Unlock();

		return asset;
	}

	void AssetContent::TryCallOnLoaded(Asset* asset)
	{
		Threading::ScopeLock lock(contentData->LoadedAssetsToInvokeLocker);
		const int32 index = contentData->LoadedAssetsToInvoke.Find(asset);
		if (index != -1)
		{
			contentData->LoadedAssetsToInvoke.RemoveAtKeepOrder(index);
			asset->onLoaded_MainThread();
		}
	}

	void AssetContent::OnAssetLoaded(Asset* asset)
	{
		// This is called by the asset on loading end
		ENGINE_ASSERT(asset && asset->IsLoaded());

		Threading::ScopeLock locker(contentData->LoadedAssetsToInvokeLocker);

		contentData->LoadedAssetsToInvoke.Add(asset);
	}

	void AssetContent::OnAssetUnload(Asset* asset)
	{
		// This is called by the asset on unloading

		Threading::ScopeLock locker(contentData->AssetsLocker);

		contentData->Assets.Remove(asset->GetID());
		contentData->UnloadQueue.Remove(asset);
		contentData->LoadedAssetsToInvoke.Remove(asset);
	}

	void AssetContent::OnAssetChangeId(Asset* asset, const UID& oldId, const UID& newId)
	{
		Threading::ScopeLock locker(contentData->AssetsLocker);
		contentData->Assets.Remove(oldId);
		contentData->Assets.Add(newId, asset);
	}

	bool AssetContent::IsAssetTypeIdInvalid(const TypeID& typeID, const TypeID& assetTypeID)
	{
		// Skip if no restrictions for the type
		if (typeID == TypeID::Invalid || assetTypeID == TypeID::Invalid)
		{
			return false;
		}

		// Early out if type matches
		if (typeID == assetTypeID)
		{
			return true;
		}

		return Types::AreTypesInTheSameHierarchy(assetTypeID, typeID);
	}

	Asset* AssetContent::LoadAsync(const UID& id, const TypeID type)
	{
		if (!id.IsValid())
			return nullptr;

		// Check if asset has been already loaded
		Asset* result = nullptr;
		contentData->AssetsLocker.Lock();
		contentData->Assets.TryGet(id, result);
		if (result)
		{
			contentData->AssetsLocker.Unlock();

			// Validate type
			if (!IsAssetTypeIdInvalid(type, result->GetType()))
			{
				LOG_WARNING("Resource", "Different loaded asset type! Asset: \'{0}\'. Expected type: {1}", result->ToString(), type.ToString());
				return nullptr;
			}
			return result;
		}

		// Check if that asset is during loading
		if (contentData->LoadCallAssets.Contains(id))
		{
			contentData->AssetsLocker.Unlock();

			// Wait for loading end by other thread
			bool contains = true;
			while (contains)
			{
				Platform::Sleep(1);
				contentData->AssetsLocker.Lock();
				contains = contentData->LoadCallAssets.Contains(id);
				contentData->AssetsLocker.Unlock();
			}
			contentData->Assets.TryGet(id, result);
			return result;
		}

		// Mark asset as loading and release lock so other threads can load other assets
		contentData->LoadCallAssets.Add(id);
		contentData->AssetsLocker.Unlock();

#define LOAD_FAILED() contentData->AssetsLocker.Lock(); contentData->LoadCallAssets.Remove(id); contentData->AssetsLocker.Unlock(); return nullptr

		// Get cached asset info (from registry)
		AssetInfo assetInfo;
		if (!GetAssetInfo(id, assetInfo))
		{
			LOG_WARNING("Resource", "Invalid or missing asset ({0}, {1}).", id, type.ToString());
			LOAD_FAILED();
		}
#if ASSETS_LOADING_EXTRA_VERIFICATION
		if (!FileSystem::FileExists(assetInfo.path))
		{
			LOG_ERROR("Resource", "Cannot find file '{0}'", assetInfo.path);
			LOAD_FAILED();
		}
#endif

		// Find asset factory based in its type
		auto factory = GetAssetFactory(assetInfo);
		if (factory == nullptr)
		{
			LOG_ERROR("Resource", "Cannot find asset factory. Info: {0}", assetInfo.ToString());
			LOAD_FAILED();
		}

		// Create asset object
		result = factory->New(&assetInfo);
		if (result == nullptr)
		{
			LOG_ERROR("Resource", "Cannot create asset object. Info: {0}", assetInfo.ToString());
			LOAD_FAILED();
		}
		ENGINE_ASSERT(result->GetID() == id);
#if ASSETS_LOADING_EXTRA_VERIFICATION
		if (!IsAssetTypeIdInvalid(type, result->GetType()))
		{
			LOG_WARNING("Resource", "Different loaded asset type! Asset: '{0}'. Expected type: {1}", assetInfo.ToString(), type.ToString());
			result->DeleteObject();
			LOAD_FAILED();
		}
#endif

		// Register asset
		contentData->AssetsLocker.Lock();
#if ASSETS_LOADING_EXTRA_VERIFICATION
		ENGINE_ASSERT(!contentData->Assets.ContainsKey(id));
#endif
		contentData->Assets.Add(id, result);

		// Start asset loading
		result->StartLoadAsset();

		// Remove from the loading queue and release lock
		contentData->LoadCallAssets.Remove(id);
		contentData->AssetsLocker.Unlock();

#undef LOAD_FAILED

		return result;
	}

#if ENABLE_ASSETS_DISCOVERY
	bool findAsset(const UID& id, const String& directory, List<String>& tmpCache, AssetInfo& info)
	{
		// Get all asset files
		tmpCache.Clear();
		if (FileSystem::DirectoryGetFiles(tmpCache, directory, SE_TEXT("*"), DirectorySearchOption::All))
		{
			if (FileSystem::DirectoryExists(directory))
				LOG_ERROR("Resource", "Cannot query files in folder '{0}'.", directory);
			return false;
		}

		// Start searching for asset with given ID
		bool result = false;
		LOG_INFO("Resource", "Start searching asset with ID: {0} in '{1}'. {2} potential files to check...", id, directory, tmpCache.Count());
		for (int32 i = 0; i < tmpCache.Count(); i++)
		{
			String& path = tmpCache[i];

			// Check if not already in registry
			// Note: maybe we could disable this check? it would slow down searching but we will find more workspace problems
			if (!contentData->Cache.HasAsset(path))
			{
				auto extension = FileSystem::GetExtension(path).ToLower();

				// Check if it's a binary asset
				if (AssetStorages::IsStorageExtension(extension))
				{
					// Skip packages in editor (results in conflicts with build game packages if deployed inside project folder)
#if SE_EDITOR
					if (extension == PACKAGE_FILES_EXTENSION)
						continue;
#endif

					// Open storage
					auto storage = AssetStorages::GetStorage(path);
					if (storage)
					{
						// Register assets
						contentData->Cache.RegisterAssets(storage);

						// Check if that is a missing asset
						if (storage->HasAsset(id))
						{
							// Found
							result = contentData->Cache.FindAsset(id, info);
							LOG_INFO("Resource", "Found {1} at '{0}'!", id, path);
						}
					}
					else
					{
						LOG_ERROR("Resource", "Cannot open file '{0}' error code: {1}", path, 0);
					}
				}
					// Check for json resource
				else if (JsonStorageProxy::IsValidExtension(extension))
				{
					// Check Json storage layer
					UID jsonId;
					TypeID jsonTypeID;
					if (JsonStorageProxy::GetAssetInfo(path, jsonId, jsonTypeID))
					{
						// Register asset
						contentData->Cache.RegisterAsset(jsonId, jsonTypeID, path);

						// Check if that is a missing asset
						if (id == jsonId)
						{
							// Found
							result = contentData->Cache.FindAsset(id, info);
							LOG_INFO("Resource", "Found {1} at '{0}'!", id, path);
						}
					}
				}
			}
		}

		return result;
	}

#endif

} // SE