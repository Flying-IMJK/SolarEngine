
#include "AssetStorages.h"
#include "FileStorage.h"
#include "PackageStorage.h"
#include "Core/Logging/Logging.h"
#include "Core/Systems.h"
#include "Core/Types/Collections/List.h"
#include "Core/Platform/FileSystem.h"
#include "Core/Profiler/ProfilerCPU.h"
#include "Core/Thread/TaskGraph.h"
#include "Core/Thread/Threading.h"
#include "Runtime/EngineContext.h"
#include "Runtime/Engine.h"

namespace SE
{

	struct AssetStorageSystemData
	{
		CriticalSection Locker;
#if SE_EDITOR
		List<FileStorage*> Files = List<FileStorage*>(1024);
		List<PackageStorage*> Packages;
#else
		List<FlaxFile*> Files;
		List<FlaxPackage*> Packages = List<FlaxPackage*>(64);
#endif
		Dictionary<String, Storage*> StorageMap = Dictionary<String, Storage*>(2048);

		Threading::TaskGraphSystem* System = nullptr;
	} *systemData;

	class AssetStorageGraphSystem : public Threading::TaskGraphSystem
	{
	public:
		void Job(int32 index)
		{
			PROFILE_CPU_NAMED("AssetStorage.Job");

			const double time = Platform::GetTimeSeconds();
			Threading::ScopeLock lock(systemData->Locker);
			for (auto i = systemData->StorageMap.begin(); i.IsNotEnd(); ++i)
			{
				auto storage = i->Value;
				if (storage->ShouldDispose())
				{
					// Remove from collections
					if (storage->IsPackage())
						systemData->Packages.Remove((PackageStorage*)storage);
					else
						systemData->Files.Remove((FileStorage*)storage);
					systemData->StorageMap.Remove(i);

					// Delete
					storage->Dispose();
					Delete(storage);
				}
				else
				{
					storage->Tick(time);
				}
			}
		}
		
		void Execute(Threading::TaskGraph* graph) override
		{
			Threading::ScopeLock lock(systemData->Locker);
			if (systemData->StorageMap.Count() == 0)
				return;

			// Schedule work to update all storage containers in async
			Function<void(int32)> job;
			job.Bind<AssetStorageGraphSystem, &AssetStorageGraphSystem::Job>(this);
			graph->DispatchJob(job, 1);
		}
	};

	class AssetStorageSystem : public ISystem
	{
	public:
		ENGINE_SYSTEM(AssetStorageSystem)

		AssetStorageSystem() : ISystem(SE_TEXT("ContentStorage"), -800)
		{
			systemData = New<AssetStorageSystemData>();
		};
		bool OnInit() override
		{
			systemData->System = New<AssetStorageGraphSystem>();
			Engine::UpdateGraph->AddSystem(systemData->System);
			return false;
		}

		void OnDispose() override
		{
			Threading::ScopeLock lock(systemData->Locker);
			for (auto i = systemData->StorageMap.begin(); i.IsNotEnd(); ++i)
				i->Value->Dispose();

			systemData->Files.ClearDelete();
			systemData->Packages.ClearDelete();
			systemData->StorageMap.Clear();
			ENGINE_ASSERT(systemData->Files.IsEmpty() && systemData->Packages.IsEmpty());
			Delete<Threading::TaskGraphSystem>(systemData->System);
		}
	};
	ENGINE_SYSTEM_REGISTER(AssetStorageSystem);


	TimeSpan AssetStorages::UnusedDataChunksLifetime = TimeSpan::FromSeconds(10);

	StorageReference AssetStorages::GetStorage(const StringView& path, bool loadIt)
	{
		systemData->Locker.Lock();

		// Try fast lookup
		Storage* storage;
		if (!systemData->StorageMap.TryGet(path, storage))
		{
			// Detect storage type and create object
			const bool isPackage = path.EndsWith(StringView(PACKAGE_FILES_EXTENSION));
			if (isPackage)
			{
				auto package = New<PackageStorage>(path);
				systemData->Packages.Add(package);
				storage = package;
			}
			else
			{
				auto file = New<FileStorage>(path);
				systemData->Files.Add(file);
				storage = file;
			}

			// Register storage container
			systemData->StorageMap.Add(path, storage);
		}

		// Build reference (before releasing the lock so AssetStorageSystem::Job won't delete it when running from async thread)
		StorageReference result(storage);

		systemData->Locker.Unlock();

		if (loadIt)
		{
			// Initialize storage container
			storage->LockChunks();
			const bool loadSuccess = storage->Load();
			storage->UnlockChunks();
			if (!loadSuccess)
			{
				LOG_ERROR("Resource", "Failed to load {0}.", path);
				systemData->Locker.Lock();
				systemData->StorageMap.Remove(path);
				if (storage->IsPackage())
					systemData->Packages.Remove((PackageStorage*)storage);
				else
					systemData->Files.Remove((FileStorage*)storage);
				systemData->Locker.Unlock();
				result = nullptr;
				Delete(storage);
			}
		}

		return result;
	}

	StorageReference AssetStorages::TryGetStorage(const StringView& path)
	{
		Threading::ScopeLock lock(systemData->Locker);
		Storage* result = nullptr;
		systemData->StorageMap.TryGet(path, result);
		return result;
	}

	StorageReference AssetStorages::EnsureAccess(const StringView& path)
	{
		// Note: because we want to create new storage package it may exists.
		// So let's check if any storage container is referencing that location and try to close it.
		auto storage = TryGetStorage(path);
		if (storage && storage->IsLoaded())
		{
			LOG_INFO("Resource", "File \'{0}\' is in use. Trying to release handle to it.", path);
			storage->CloseFileHandles();
		}
		return storage;
	}

	uint32 AssetStorages::GetMemoryUsage()
	{
		Threading::ScopeLock lock(systemData->Locker);
		uint32 result = 0;
		for (auto i = systemData->StorageMap.begin(); i.IsNotEnd(); ++i)
		{
			result += i->Value->GetMemoryUsage();
		}
		return result;
	}

	bool AssetStorages::HasAsset(const UID& id)
	{
		Threading::ScopeLock lock(systemData->Locker);
		for (auto i = systemData->StorageMap.begin(); i.IsNotEnd(); ++i)
		{
			if (i->Value->HasAsset(id))
				return true;
		}
		return false;
	}

	bool AssetStorages::GetAssetEntry(const String& path, Storage::Entry& output)
	{
		// Load storage container
		auto storage = GetStorage(path);
		if (!storage)
			return true;

		// Check entries
		if (storage->GetEntriesCount() != 1)
			return true;

		// Pick up the first entry
		storage->GetEntry(0, output);
		return false;
	}

	void AssetStorages::OnRenamed(const StringView& oldPath, const StringView& newPath)
	{
		Threading::ScopeLock lock(systemData->Locker);

		// Update cached path key
		auto i = systemData->StorageMap.Find(oldPath);
		if (i != systemData->StorageMap.end())
		{
			ENGINE_ASSERT(!systemData->StorageMap.ContainsKey(newPath));
			const auto value = i->Value;
			systemData->StorageMap.Remove(i);
			systemData->StorageMap.Add(newPath, value);
		}
	}

	void AssetStorages::EnsureUnlocked()
	{
		systemData->Locker.Lock();
		systemData->Locker.Unlock();
	}

	void AssetStorages::FormatPath(String& path)
	{
		FileSystem::PathRemoveRelativeParts(path);
		if (FileSystem::IsRelative(path))
		{
			// Convert local-project paths into absolute format which is used by Storage system
			path = EngineContext::ProjectFolder / path;
		}
	}

	bool AssetStorages::IsStoragePath(const String& path)
	{
		auto extension = FileSystem::GetExtension(path).ToLower();
		return IsStorageExtension(extension);
	}

	bool AssetStorages::IsStorageExtension(const String& extension)
	{
		return extension == ASSET_FILES_EXTENSION || extension == PACKAGE_FILES_EXTENSION;
	}

	void AssetStorages::GetPackages(List<PackageStorage*>& result)
	{
		result.Add(systemData->Packages);
	}

	void AssetStorages::GetFiles(List<FileStorage*>& result)
	{
		result.Add(systemData->Files);
	}

	void AssetStorages::GetStorage(List<Storage*>& result)
	{
		result.EnsureCapacity(systemData->Packages.Count() + systemData->Files.Count());
		for (int32 i = 0; i < systemData->Packages.Count(); i++)
			result.Add(systemData->Packages[i]);
		for (int32 i = 0; i < systemData->Files.Count(); i++)
			result.Add(systemData->Files[i]);
	}
}