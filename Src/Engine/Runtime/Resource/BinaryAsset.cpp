
#include "BinaryAsset.h"
#include "AssetContent.h"
#include "Loading/AssetTask.h"
#include "Runtime/Core/Thread/Threading.h"
#include "Runtime/Core/Serialization/JsonTools.h"
#include "Runtime/Core/Platform/FileSystem.h"
#include "Runtime/Core/Logging/Exceptions/ExceptionInclude.h"

#include "AssetRef.h"
#include "Runtime/EngineContext.h"
#include "Runtime/Resource/Loading/Tasks/LoadAssetDataTask.h"
#include "Runtime/Resource/Storage/AssetStorages.h"
#include "Runtime/Resource/Importers/AssetsImportingSystem.h"
#include "Runtime/Resource/Factories/BinaryAssetFactory.h"


namespace SE
{
	BinaryAsset::BinaryAsset(const SpawnParams& params, const AssetInfo* info)
		: Asset(params, info)
		, m_StorageRef(nullptr) // We link storage container later
		, m_IsSaving(false)
		, storage(nullptr)
	{
	}


	BinaryAsset::BinaryAsset() : Asset(nullptr), m_StorageRef(nullptr) // We link storage container later
		, m_IsSaving(false), storage(nullptr)
	{
	}

	BinaryAsset::~BinaryAsset()
	{
#if SE_EDITOR
		if (storage)
			storage->OnReloaded.Unbind<BinaryAsset, &BinaryAsset::OnStorageReloaded>(this);
#endif
	}

	bool BinaryAsset::Init(const StorageReference& newStorage, AssetHeader& header)
	{
		// We allow to init asset only once like that
		ENGINE_ASSERT(storage == nullptr && m_Header.ID.IsValid() == false);

		// Block initialization with a different storage
		bool isChanged = m_StorageRef != newStorage;
		if (storage != nullptr && isChanged)
		{
			LOG_ERROR("Resource", "Asset \'{0}\' has been already initialized.", GetPath());
			return false;
		}

		// Get data
		m_StorageRef = newStorage;
		this->storage = newStorage.Get();
		m_Header = header;

#if SE_EDITOR
		// Link for storage reload event
		if (storage && isChanged)
		{
			storage->OnReloaded.Bind<BinaryAsset, &BinaryAsset::OnStorageReloaded>(this);
		}
#endif
		return true;
	}

	bool BinaryAsset::Init(AssetInitData& initData)
	{
		// Validate serialized version
		if (initData.SerializedVersion != GetSerializedVersion())
		{
			LOG_ERROR("Resource", "Asset \'{0}\' is using different serialized version. Loaded: {1}, Runtime: {2}.", GetPath(), initData.SerializedVersion, GetSerializedVersion());
			return false;
		}

		// Get asset data
		m_Header = initData.Header;
#if SE_EDITOR
		metadata.Copy(initData.Metadata);
		ClearDependencies();
		dependencies = initData.Dependencies;
		for (auto& e : dependencies)
		{
			auto asset = TypeCast<BinaryAsset>(AssetContent::GetAsset(e.First));
			if (asset)
			{
				asset->m_DependantAssets.Add(this);
			}
		}
#endif

		return OnInit(initData);
	}

	bool BinaryAsset::InitVirtual(AssetInitData& initData)
	{
		// Be virtual
		m_IsVirtual = true;

		return Init(initData);
	}

#if SE_EDITOR

	void BinaryAsset::Reimport() const
	{
		const String importPath = GetImportPath();
		if (importPath.HasChars())
		{
			AssetsImporting::Import(importPath, GetPath());
		}
	}

	void BinaryAsset::GetImportMetadata(String& path, String& username) const
	{
		if (metadata.IsInvalid())
			return;

		// Parse metadata and try to get import info
		Json::Document document;
		document.Parse((const char*)metadata.Get(), metadata.Length());
		if (document.HasParseError() == false)
		{
			path = JsonTools::GetString(document, "ImportPath");
			username = JsonTools::GetString(document, "ImportUsername");
			if (path.HasChars() && FileSystem::IsRelative(path))
			{
				// Convert path back to thr absolute (eg. if stored in relative format)
				path = EngineContext::ProjectFolder / path;
				FileSystem ::PathRemoveRelativeParts(path);
			}
		}
		else
		{
			Log::JsonParseException(document.GetParseError(), document.GetErrorOffset(), GetPath());
		}
	}

	String BinaryAsset::GetImportPath() const
	{
		String path, username;
		GetImportMetadata(path, username);
		return path;
	}

	void BinaryAsset::ClearDependencies()
	{
		for (auto& e : dependencies)
		{
			auto asset = TypeCast<BinaryAsset>(AssetContent::GetAsset(e.First));
			if (asset)
				asset->m_DependantAssets.Remove(this);
		}
		dependencies.Clear();
	}

	void BinaryAsset::AddDependency(BinaryAsset* asset)
	{
		ASSERT_LOW_LAYER(asset);
		const UID id = asset->GetID();
		for (auto& e : dependencies)
		{
			if (e.First == id)
				return;
		}
		ENGINE_ASSERT(!asset->m_DependantAssets.Contains(asset));
		dependencies.Add(ToPair(id, FileSystem::GetFileLastEditTime(asset->GetPath())));
		asset->m_DependantAssets.Add(this);
	}

	bool BinaryAsset::HasDependenciesModified() const
	{
		AssetInfo info;
		for (const auto& e : dependencies)
		{
			if (AssetContent::GetAssetInfo(e.First, info))
			{
				const auto editTime = FileSystem::GetFileLastEditTime(info.path);
				if (editTime > e.Second)
				{
					LOG_INFO("Resource", "Asset {0} was modified - dependency of {1}", info.path, GetPath());
					return true;
				}
			}
		}
		return false;
	}

#endif

	StorageChunk* BinaryAsset::GetOrCreateChunk(int32 index)
	{
		ENGINE_ASSERT(Math::RangeInclusive(index, 0, ASSET_FILE_DATA_CHUNKS - 1));

		// Try get
		auto chunk = m_Header.Chunks[index];
		if (chunk)
		{
			chunk->RegisterUsage();
			return chunk;
		}

		// Allocate
		ENGINE_ASSERT(storage);
		m_Header.Chunks[index] = chunk = storage->AllocateChunk();
		if (chunk)
			chunk->RegisterUsage();

		return chunk;
	}

	void BinaryAsset::SetChunk(int32 index, const Span<byte>& data)
	{
		auto chunk = GetOrCreateChunk(index);
		if (chunk)
			chunk->Data.Copy(data.Get(), data.Length());
	}

	void BinaryAsset::ReleaseChunks() const
	{
		for (int32 i = 0; i < ASSET_FILE_DATA_CHUNKS; i++)
			ReleaseChunk(i);
	}

	void BinaryAsset::ReleaseChunk(int32 index) const
	{
		auto chunk = GetChunk(index);
		if (chunk)
			chunk->Data.Release();
	}

	AssetTask* BinaryAsset::RequestChunkDataAsync(int32 index)
	{
		auto chunk = GetChunk(index);
		if (chunk != nullptr && chunk->IsValid())
		{
			// Data already here
			chunk->RegisterUsage();
			return nullptr;
		}

		// Spawn loading task
		return New<LoadAssetDataTask>(this, GET_CHUNK_FLAG(index));
	}

	void BinaryAsset::GetChunkData(int32 index, BytesContainer& data) const
	{
		//ScopeLock lock(Locker);

		// Check if has data missing
		if (!HasChunkLoaded(index))
		{
			// Missing data
			data.Release();
			return;
		}

		// Get data
		auto chunk = GetChunk(index);
		data.Link(chunk->Data);
	}

	bool BinaryAsset::LoadChunk(int32 chunkIndex)
	{
		ENGINE_ASSERT(storage);

		const auto chunk = m_Header.Chunks[chunkIndex];
		if (chunk != nullptr
			&& chunk->IsValid()
			&& chunk->ExistsInFile())
		{
			if (!storage->LoadAssetChunk(chunk))
			{
				return false;
			}
		}

		return true;
	}

	bool BinaryAsset::LoadChunks(AssetChunksFlag chunks)
	{
		ENGINE_ASSERT(storage);

		// Check if skip loading
		if (chunks == 0)
			return true;

		// Load all missing marked chunks
		for (int32 i = 0; i < ASSET_FILE_DATA_CHUNKS; i++)
		{
			auto chunk = m_Header.Chunks[i];
			if (chunk != nullptr
				&& chunks & GET_CHUNK_FLAG(i)
				&& !chunk->IsValid()
				&& chunk->ExistsInFile())
			{
				if (!storage->LoadAssetChunk(chunk))
				{
					return false;
				}
			}
		}

		return true;
	}

#if SE_EDITOR

	bool BinaryAsset::SaveAsset(AssetInitData& data, bool silentMode) const
	{
		return SaveAsset(GetPath(), data, silentMode);
	}

	bool BinaryAsset::SaveAsset(const StringView& path, AssetInitData& data, bool silentMode) const
	{
		data.Header = m_Header;
		data.Metadata.Link(metadata);
		data.Dependencies = dependencies;
		return SaveToAsset(path, data, silentMode);
	}

	bool BinaryAsset::SaveToAsset(const StringView& path, AssetInitData& data, bool silentMode)
	{
		// Ensure path is in a valid format
		String pathNorm(path);
		AssetStorages::FormatPath(pathNorm);
		const StringView filePath = pathNorm;

		// Find target storage container and the asset
		auto storage = AssetStorages::TryGetStorage(filePath);
		auto asset = AssetContent::GetAsset(filePath);
		auto binaryAsset = dynamic_cast<BinaryAsset*>(asset);
		if (asset && !binaryAsset)
		{
			LOG_WARNING("Resource", "Cannot write to the non-binary asset location.");
			return false;
		}
		if (!binaryAsset && !storage && FileSystem::FileExists(filePath))
		{
			// Force-resolve storage (asset at that path could be not yet loaded into registry)
			storage = AssetStorages::GetStorage(filePath);
		}

		// Check if can perform write operation to the asset container
		if (storage && !storage->AllowDataModifications())
		{
			LOG_WARNING("Resource", "Cannot write to the asset storage container.");
			return false;
		}

		// Initialize data container
		ENGINE_ASSERT(data.SerializedVersion > 0);
		if (binaryAsset)
		{
			// Use the same asset ID
			data.Header.ID = binaryAsset->GetID();
		}
		else if (storage && storage->GetEntriesCount())
		{
			// Use the same file ID
			data.Header.ID = storage->GetEntry(0).ID;
		}
		else
		{
			// Randomize ID
			data.Header.ID = UID::New();
		}

		// Save (set flag to lock reloads on storage modified)
		if (binaryAsset)
			binaryAsset->m_IsSaving = true;
		bool result;
		if (storage)
		{
			// HACK: file is locked by some tasks (e.g material asset loaded some data and is updating the asset)
			// Let's hide these locks just for the saving
			const auto locks = storage->_chunksLock;
			storage->_chunksLock = 0;
			result = storage->Save(data, silentMode);
			storage->_chunksLock = locks;
		}
		else
		{
			ENGINE_ASSERT(filePath.HasChars());
			result = Storage::Create(filePath, data, silentMode);
		}
		if (binaryAsset)
			binaryAsset->m_IsSaving = false;

		if (binaryAsset)
		{
			// Inform dependant asset (use cloned version because it might be modified by assets when they got reloaded)
			auto dependantAssets = binaryAsset->m_DependantAssets;
			for (auto& e : dependantAssets)
			{
				e->OnDependencyModified(binaryAsset);
			}
		}

		return result;
	}

	void BinaryAsset::OnStorageReloaded(Storage* storage, bool failed)
	{
		ENGINE_ASSERT(storage != nullptr && storage == storage);

		LOG_INFO("Resource", "Reload Asset: \'{0}\'.", ToString());

		// Clear header (prevent from using old chunks)
		auto oldHeader = m_Header;
		Platform::MemoryClear(m_Header.Chunks, sizeof(m_Header.Chunks));

		// Check if reload failed
		if (failed)
		{
			LOG_ERROR("Resource", "Asset storage reloading failed. Asset: \'{0}\'.", ToString());
			return;
		}

		// Gather updated asset init data
		AssetInitData initData;
		if (!storage->LoadAssetHeader(GetID(), initData))
		{
			LOG_ERROR("Resource", "Asset header loading failed. Asset: \'{0}\'.", ToString());
			return;
		}
		if (oldHeader.ID != initData.Header.ID || oldHeader.TypeID != initData.Header.TypeID)
		{
			LOG_WARNING("Resource",
				"Asset reloading data mismatch. Old ID:{0},TypeName:{1}, New ID:{2},TypeName:{3}. Asset: \'{4}\'.",
				oldHeader.ID,
				oldHeader.TypeID,
				initData.Header.ID,
				initData.Header.TypeID,
				GetPath());

			// Unload asset (file contains different asset data)
			// For eg. texture has been changed into sprite atlas on reimport
			AssetContent::UnloadAsset(this);

			// Delete managed object now because it way fail when we recreate the asset object and want to register the new managed object (IDs will overlap)
			DeleteManaged();

			return;
		}

		// Reinitialize (file may modify some data so it needs to be flushed)
		if (!Init(initData))
		{
			LOG_ERROR("Resource", "Asset reloading failed. Asset: \'{0}\'.", ToString());
		}

		// Don't reload on save
		if (m_IsSaving == false)
		{
			Reload();
		}

		// Inform dependant asset (use cloned version because it might be modified by assets when they got reloaded)
		auto dependantAssets = m_DependantAssets;
		for (auto& e : dependantAssets)
		{
			e->OnDependencyModified(this);
		}
	}

	void BinaryAsset::OnDeleteObject()
	{
		// Clear dependencies stuff
		ClearDependencies();
		m_DependantAssets.Clear();

//		Asset::OnDeleteObject();
	}

#endif

	const String& BinaryAsset::GetPath() const
	{
#if SE_EDITOR
		return storage ? storage->GetPath() : String::Empty;
#else
		// In build all assets are packed into packages so use ID for original path lookup
		return Content::GetRegistry()->GetEditorAssetPath(_id);
#endif
	}

	uint64 BinaryAsset::GetMemoryUsage() const
	{
		Locker.Lock();
		uint64 result = Asset::GetMemoryUsage();
		result += sizeof(BinaryAsset) - sizeof(Asset);
		result += m_DependantAssets.Capacity() * sizeof(BinaryAsset*);
		for (int32 i = 0; i < ASSET_FILE_DATA_CHUNKS; i++)
		{
			auto chunk = m_Header.Chunks[i];
			if (chunk != nullptr && chunk->IsValid())
				result += chunk->Size();
		}
		Locker.Unlock();
		return result;
	}

	/// <summary>
	/// Helper task used to initialize binary asset and upgrade it if need to in background.
	/// </summary>
	/// <seealso cref="ContentLoadTask" />
	class InitAssetTask : public AssetTask
	{
	private:
		WeakAssetRef<BinaryAsset> _asset;
		Storage::LockData _dataLock;

	public:
		/// <summary>
		/// Initializes a new instance of the <see cref="InitAssetTask"/> class.
		/// </summary>
		/// <param name="asset">The asset.</param>
		InitAssetTask(BinaryAsset* asset)
			: _asset(asset), _dataLock(asset->storage->Lock())
		{
		}

	public:
		// [AssetLoadTask]
		bool HasReference(void* obj) const override
		{
			return obj == _asset;
		}

	protected:
		// [AssetLoadTask]
		Result Process() override
		{
			AssetRef<BinaryAsset> ref = _asset.Get();
			if (ref == nullptr)
				return Result::MissingReferences;
			auto storage = ref->storage;
			auto factory = (BinaryAssetFactoryBase*)AssetContent::GetAssetFactory(ref->GetType());
			ENGINE_ASSERT(factory);

			// Here we should open storage and extract AssetInitData
			// This would also allow to convert/upgrade data
			if (!storage->IsLoaded() && !storage->Load())
				return Result::AssetLoadError;
			if (!factory->Init(ref.Get()))
				return Result::AssetLoadError;

			return Result::Ok;
		}

		void OnEnd() override
		{
			_dataLock.Release();
			_asset = nullptr;

			AssetTask::OnEnd();
		}
	};

	AssetTask* BinaryAsset::CreateLoadAssetTask()
	{
		AssetTask* loadTask = Asset::CreateLoadAssetTask();

		// Check if asset need any just to be preloaded
		auto chunksToPreload = GetChunksToPreload();
		if (chunksToPreload != 0)
		{
			// Inject loading chunks task
			auto preLoadChunksTask = New<LoadAssetDataTask>(this, chunksToPreload);
			preLoadChunksTask->ContinueWith(loadTask);
			loadTask = preLoadChunksTask;
		}

		// Before asset loading we have to initialize storage
		// TODO: maybe in build game we could do it in place?
		// This step is only for opening asset files in background and upgrading them
		// In build game we have only a few packages which are ready to use
		auto initTask = New<InitAssetTask>(this);
		initTask->ContinueWith(loadTask);
		loadTask = initTask;

		return loadTask;
	}

	Asset::LoadResult BinaryAsset::ProcessLoadAsset()
	{
		// 确保资源已初始化
		ENGINE_ASSERT(storage != nullptr && m_Header.ID.IsValid() && m_Header.TypeID != TypeID::Invalid);

		auto lock = storage->Lock();
		auto chunksToPreload = GetChunksToPreload();
		if (chunksToPreload != 0)
		{
			// 确保之前请求的任何块都加载到内存中（以防 streaming 在超时后将它们刷新）
			for (int32 i = 0; i < ASSET_FILE_DATA_CHUNKS; i++)
			{
				const auto chunk = m_Header.Chunks[i];
				if (GET_CHUNK_FLAG(i) & chunksToPreload && chunk && !chunk->IsValid())
				{
					storage->LoadAssetChunk(chunk);
				}
			}
		}
		const LoadResult result = load();
#if !BUILD_RELEASE
		if (result == LoadResult::MissingDataChunk)
		{
			// Provide more insights on potentially missing asset data chunk
			Char chunksBitMask[ASSET_FILE_DATA_CHUNKS + 1];
			Char chunksExistBitMask[ASSET_FILE_DATA_CHUNKS + 1];
			Char chunksLoadBitMask[ASSET_FILE_DATA_CHUNKS + 1];
			for (int32 i = 0; i < ASSET_FILE_DATA_CHUNKS; i++)
			{
				if (const StorageChunk* chunk = m_Header.Chunks[i])
				{
					chunksBitMask[i] = '1';
					chunksExistBitMask[i] = chunk->ExistsInFile() ? '1' : '0';
					chunksLoadBitMask[i] = chunk->IsValid() ? '1' : '0';
				}
				else
				{
					chunksBitMask[i] = chunksExistBitMask[i] = chunksLoadBitMask[i] = '0';
				}
			}
			chunksBitMask[ASSET_FILE_DATA_CHUNKS] = chunksExistBitMask[ASSET_FILE_DATA_CHUNKS] = chunksLoadBitMask[ASSET_FILE_DATA_CHUNKS] = 0;
			LOG_WARNING("Resource",
				"Asset reports missing data chunk. Chunks bitmask: {}, existing chunks: {} loaded chunks: {}. '{}'",
				chunksBitMask,
				chunksExistBitMask,
				chunksLoadBitMask,
				ToString());
		}
#endif
		return result;
	}

	void BinaryAsset::ReleaseStorage()
	{
#if SE_EDITOR
		// Close file
		if (storage)
			storage->CloseFileHandles();
#endif
	}

#if SE_EDITOR
	void BinaryAsset::onRename(const StringView& newPath)
	{
		Threading::ScopeLock lock(Locker);

		// We don't support packages now
		ENGINE_ASSERT(!storage->IsPackage() && storage->AllowDataModifications() && storage->GetEntriesCount() == 1);

		// Rename storage
		storage->OnRename(newPath);
	}
#endif

} // SE