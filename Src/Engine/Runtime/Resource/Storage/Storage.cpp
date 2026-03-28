
#include "Storage.h"
#include "FileStorage.h"
#include "PackageStorage.h"
#include "AssetStorages.h"
#include "Core/Logging/Logging.h"
#include "Core/Types/TimeSpan.h"
#include "Core/Platform/File.h"
#include "Core/Profiler/ProfilerCPU.h"
#include "Core/Serialization/FileWriteStream.h"
#include "Core/Thread/Threading.h"
#include "Core/Types/BitFlags.h"
#include "Runtime/Resource/AssetContent.h"
#include "Runtime/EngineContext.h"
#if SE_EDITOR
#include "Core/Serialization/JsonWriter.h"
#endif
#include "Runtime/ThirdParty/LZ4/lz4.h"

namespace SE
{
	int32 AssetHeader::GetChunksCount() const
	{
		int32 result = 0;
		for (int32 i = 0; i < ASSET_FILE_DATA_CHUNKS; i++)
		{
			if (Chunks[i] != nullptr)
				result++;
		}
		return result;
	}

	void AssetHeader::DeleteChunks()
	{
		for (int32 i = 0; i < ASSET_FILE_DATA_CHUNKS; i++)
		{
			Delete(Chunks[i]);
		}
	}

	void AssetHeader::UnlinkChunks()
	{
		Platform::MemoryClear(Chunks, sizeof(Chunks));
	}

	String AssetHeader::ToString() const
	{
		return String::Format(SE_TEXT("ID: {0}, TypeName: {1}, Chunks Count: {2}"), ID, TypeID, GetChunksCount());
	}

	void StorageChunk::RegisterUsage()
	{
		LastAccessTime = Platform::GetTimeSeconds();
	}

	const int32 Storage::MagicCode = 1180124739;

	Storage::LockData Storage::LockData::Invalid(nullptr);

	struct Header
	{
		int32 MagicCode;
		uint32 Version;
		Storage::CustomData CustomData;
	};

	struct SerializedTypeNameV9
	{
		Char Data[64];

		SerializedTypeNameV9()
		{
			Platform::MemoryClear(Data, sizeof(Data));
		}

		SerializedTypeNameV9(const TypeID& data)
		{
			StringView typeName = data.ToString();
			const int32 length = typeName.Length();
			ENGINE_ASSERT(length < ARRAY_SIZE(Data));
			Platform::MemoryCopy(Data, *typeName, length * sizeof(Char));
			Platform::MemoryClear(Data + length, (ARRAY_SIZE(Data) - length) * sizeof(Char));
		}
	};

	struct SerializedEntryV9
	{
		UID ID;
		SerializedTypeNameV9 Type;
		uint32 Address;

		SerializedEntryV9()
			: ID(UID::Empty), Address(0)
		{
		}

		SerializedEntryV9(const UID& id, const TypeID& typeID, uint32 address)
			: ID(id), Type(typeID), Address(address)
		{
		}
	};

	Storage::Storage(const StringView& path)
		: _path(path)
	{
	}

	Storage::~Storage()
	{
		// Validate if has been disposed
		ENGINE_ASSERT(IsDisposed());
//		CHECK(_chunksLock == 0);
//		CHECK(_refCount == 0);
		ENGINE_ASSERT(_chunks.IsEmpty());

#if SE_EDITOR
		// Ensure to close any outstanding file handles to prevent file locking in case it failed to load
		List<FileReadStream*> streams;
		_file.GetValues(streams);
		for (FileReadStream* stream : streams)
		{
			if (stream)
				Delete(stream);
		}
		Platform::AtomicStore(&_files, 0);
#endif
	}

	Storage::LockData Storage::LockSafe()
	{
		auto lock = LockData(this);
		AssetStorages::EnsureUnlocked();
		return lock;
	}

	uint32 Storage::GetRefCount() const
	{
		return (uint32)Platform::AtomicRead((int64*)&_refCount);
	}

	bool Storage::ShouldDispose() const
	{
		return Platform::AtomicRead((int64*)&_refCount) == 0 &&
			Platform::AtomicRead((int64*)&_chunksLock) == 0 &&
			Platform::GetTimeSeconds() - _lastRefLostTime >= 0.5; // TTL in seconds
	}

	uint32 Storage::GetMemoryUsage() const
	{
		uint32 result = sizeof(Storage);
		for (int32 i = 0; i < _chunks.Count(); i++)
			result += _chunks[i]->Data.Length();
		return result;
	}

	bool Storage::Load()
	{
		// Check if was already loaded
		if (IsLoaded())
		{
			return true;
		}

		// Prevent loading by more than one thread
		Threading::ScopeLock lock(_loadLocker);
		if (IsLoaded())
		{
			// Other thread loaded it
			return true;
		}
		ENGINE_ASSERT(GetEntriesCount() == 0);

		// Open file
		auto stream = OpenFile();
		if (stream == nullptr)
			return false;

		// Magic Code
		uint32 magicCode;
		stream->ReadUint32(&magicCode);
		if (magicCode != MagicCode)
		{
			LOG_WARNING("Resource", "Invalid asset magic code in {0}", ToString());
			return false;
		}

		// Version
		uint32 version;
		stream->ReadUint32(&version);
		switch (version)
		{
		case 1:
		{
			// Custom storage data
			CustomData customData;
			stream->Read(customData);

#if SE_EDITOR
			// Block loading packaged games content
			if (customData.ContentKey != 0)
#else
			// Block load content from unknown sources
			if (customData.ContentKey != EngineContext::ContentKey)
#endif
			{
				LOG_WARNING("Resource", "Invalid asset {0}.", ToString());
				return false;
			}

			// Entries
			int32 assetsCount;
			stream->ReadInt32(&assetsCount);
			for (int32 i = 0; i < assetsCount; i++)
			{
				SerializedEntryV9 se;
				stream->ReadBytes(&se, sizeof(se));
				Entry e(se.ID, TypeID(se.Type.Data), se.Address);
				AddEntry(e);
			}

			// Chunks
			int32 chunksCount;
			stream->ReadInt32(&chunksCount);
			for (int32 i = 0; i < chunksCount; i++)
			{
				StorageChunk::Location e;
				stream->ReadBytes(&e, sizeof(e));
				if (e.Size == 0)
				{
					LOG_WARNING("Resource", "Empty chunk found.");
					return false;
				}
				auto chunk = New<StorageChunk>();
				chunk->LocationInFile = e;
				stream->ReadInt32(reinterpret_cast<int32*>(&chunk->Flags));
				AddChunk(chunk);
			}

			break;
		}
		default:
			LOG_WARNING("Resource", "{0} Unsupported storage format version: {1}. {0}", ToString(), version, _version);
			return false;
		}

		// Mark as loaded (version number describes 'isLoaded' state)
		_version = version;

		return true;
	}

#if SE_EDITOR

	bool Storage::Reload()
	{
		if (!IsLoaded())
		{
			return false;
		}
		PROFILE_CPU();

		OnReloading(this);

		// Perform clean reloading
		Dispose();
		const bool failed = !Load();

		OnReloaded(this, failed);

		return failed;
	}

#endif

	bool Storage::LoadAssetHeader(const UID& id, AssetInitData& data)
	{
		ENGINE_ASSERT(IsLoaded());

		// Get asset location in file
		Entry e;
		if (!GetEntry(id, e))
		{
			LOG_ERROR("Resource", "Cannot find asset \'{0}\' within {1}", id, ToString());
			return false;
		}

		// Load header
		return LoadAssetHeader(e, data);
	}

	bool Storage::LoadAssetChunk(StorageChunk* chunk)
	{
		ENGINE_ASSERT(IsLoaded());
		ENGINE_ASSERT(chunk != nullptr && _chunks.Contains(chunk));

		// Check if already loaded
		if (chunk->IsValid())
		{
			return true;
		}

		// Ensure that asset is in a file
		if (chunk->ExistsInFile() == false)
		{
			LOG_WARNING("Resource", "Cannot load chunk from {0}. It doesn't exist in storage.", ToString());
			return false;
		}

		LockChunks();

		// Open file
		auto stream = OpenFile();
		bool failed = stream == nullptr;
		if (!failed)
		{
			// Seek
			stream->SetPosition(chunk->LocationInFile.Address);

			if (stream->HasError())
			{
				// Sometimes stream->HasError() from setposition. result in a crash or missing media in release (stream _file._handle = nullptr).
				// When retrying, it looks like it works and we can continue. We need this to success.

				for (int retry = 0; retry < 5; retry++)
				{
					Platform::Sleep(50);
					stream = OpenFile();
					failed = stream == nullptr;
					if (!failed)
					{
						stream->SetPosition(chunk->LocationInFile.Address);
					}
					if (!stream->HasError())
						break;
				}
			}

			if (stream->HasError())
			{
				failed = true;
				UnlockChunks();
				LOG_WARNING("Resource", "SetPosition failed on chunk {0}.", ToString());
				return !failed;
			}

			// Load data
			auto size = chunk->LocationInFile.Size;
			if (chunk->Flags.IsFlag(AssetChunkFlags::CompressedLZ4))
			{
				// Compressed
				size -= sizeof(int32); // Don't count original size int
				int32 originalSize;
				stream->ReadInt32(&originalSize);
				List<byte> tmpBuf;
				tmpBuf.Resize(size); // TODO: maybe use thread local or content loading pool with sharable temp buffers for the decompression?
				stream->ReadBytes(tmpBuf.Get(), size);

				// Decompress data
				PROFILE_CPU_NAMED("DecompressLZ4");
				chunk->Data.Allocate(originalSize);
				const int32 res = LZ4_decompress_safe((const char*)tmpBuf.Get(), chunk->Data.Get<char>(), size, originalSize);
				if (res <= 0)
				{
					UnlockChunks();
					LOG_WARNING("Resource", "Cannot load chunk from {0}. Failed to decompress it data. Result: {1}.", ToString(), res);
					return false;
				}
				chunk->Data.SetLength(res);
			}
			else
			{
				// Raw data
				chunk->Data.Read(stream, size);
			}
			ENGINE_ASSERT(chunk->IsValid());
			chunk->RegisterUsage();
		}

		UnlockChunks();

		return !failed;
	}

#if SE_EDITOR

	bool Storage::ChangeAssetID(Entry& e, const UID& newId)
	{
		ENGINE_ASSERT(IsLoaded());

		// TODO: validate entry
		ENGINE_ASSERT(newId.IsValid());
		ENGINE_ASSERT(AllowDataModifications());

		LOG_INFO("Resource", "Changing asset \'{0}\' id to \'{1}\' (storage: \'{2}\')", e.ID, newId, _path);

		// Ensure to be loaded
		if (!IsLoaded())
		{
			if (!Load())
			{
				return false;
			}
		}

		// Load asset entries data
		int32 entriesCount = GetEntriesCount();
		List<AssetInitData> data;
		data.Resize(entriesCount);
		for (int32 i = 0; i < entriesCount; i++)
		{
			if (!LoadAssetHeader(i, data[i]))
			{
				LOG_WARNING("Resource", "Cannot load asset data.");
				return false;
			}
		}

		// Load all chunks
		for (int32 i = 0; i < _chunks.Count(); i++)
		{
			if (!LoadAssetChunk(_chunks[i]))
			{
				LOG_WARNING("Resource", "Cannot load asset chunk.");
				return false;
			}
		}

		// Close file
		if (!CloseFileHandles())
		{
			LOG_ERROR("Resource", "Cannot close file access for '{}'", _path);
			return false;
		}

		// Change ID
		// TODO: here we could extend it and load assets from the storage and call asset ID change event to change references
		List<Entry> entries;
		GetEntries(entries);
		for (int32 i = 0; i < entriesCount; i++)
		{
			Entry* myE = &entries[i];
			if (myE->ID == e.ID)
			{
				// Change ID
				e.ID = newId;
				myE->ID = newId;
				data[i].Header.ID = newId;
				break;
			}
		}

		// Repack container
		if (!Create(_path, data))
		{
			LOG_WARNING("Resource", "Cannot repack storage.");
			return false;
		}

		return true;
	}

#endif

	StorageChunk* Storage::AllocateChunk()
	{
		if (AllowDataModifications())
		{
			auto chunk = New<StorageChunk>();
			_chunks.Add(chunk);
			return chunk;
		}

		LOG_WARNING("Resource", "Cannot allocate chunk in {0}", ToString());
		return nullptr;
	}

#if SE_EDITOR

	bool Storage::Create(const StringView& path, const AssetInitData* data, int32 dataCount, bool silentMode, const CustomData* customData)
	{
		PROFILE_CPU();
		ZoneText(*path, path.Length());
		LOG_INFO("Resource", "Creating package at \'{0}\'. Silent Mode: {1}", path, silentMode);

		// Prepare to have access to the file
		auto storage = AssetStorages::EnsureAccess(path);

		// Open file
		auto stream = FileWriteStream::Open(path);
		if (stream == nullptr)
			return false;

		// Create package
		bool result = Create(stream, data, dataCount, customData);

		// Close file
		Delete(stream);

		// Reload storage container (only if not in silent mode)
		if (storage && !silentMode)
		{
			storage->Reload();
		}

		return result;
	}

	bool Storage::Create(WriteStream* stream, const AssetInitData* data, int32 dataCount, const CustomData* customData)
	{
		// Validate inputs
		if (data == nullptr || dataCount <= 0)
		{
			LOG_WARNING("Resource", "Cannot create new package. No assets to write.");
			return false;
		}

		// Prepare data
		List<SerializedEntryV9, InlinedAllocation<1>> entries;
		entries.Resize(dataCount);
		List<StorageChunk*> chunks;

		// Get all chunks
		for (int32 i = 0; i < dataCount; i++)
		{
			data[i].Header.GetLoadedChunks(chunks);
		}
		int32 chunksCount = chunks.Count();

		// TODO: sort chunks by size? smaller ones first?
		// Calculate start address of the first asset header location
		// 0 -> Header -> Entries Count -> Entries -> Chunks Count -> Chunk Locations
		int32 currentAddress = sizeof(Header)
			+ sizeof(int32)
			+ sizeof(SerializedEntryV9) * dataCount
			+ sizeof(int32)
			+ (sizeof(StorageChunk::Location) + sizeof(int32)) * chunksCount;

		// Initialize entries offsets in the file
		for (int32 i = 0; i < dataCount; i++)
		{
			auto& asset = data[i];
			entries[i] = SerializedEntryV9(asset.Header.ID, asset.Header.TypeID, currentAddress);

			// Move forward by asset header data size
			currentAddress += sizeof(UID) // ID
				+ sizeof(SerializedTypeNameV9) // Type Name
				+ sizeof(uint32) // Serialized Version
				+ sizeof(int32) * 16 // Chunks mapping
				+ sizeof(int32) + asset.CustomData.Length()
#if SE_EDITOR
				+ sizeof(int32) + asset.Metadata.Length()
				+ sizeof(int32) + asset.Dependencies.Count() * sizeof(Pair<UID, DateTime>)
#else
				+ sizeof(int32)
				+ sizeof(int32)
#endif
				+ sizeof(int32); // Header Hash Code
		}

		// Compress chunks
		List<List<byte>> compressedChunks;
		compressedChunks.Resize(chunksCount);
		for (int32 i = 0; i < chunksCount; i++)
		{
			const StorageChunk* chunk = chunks[i];
			if (chunk->Flags.IsFlag(AssetChunkFlags::CompressedLZ4))
			{
				PROFILE_CPU_NAMED("CompressLZ4");
				const int32 srcSize = chunk->Data.Length();
				const int32 maxSize = LZ4_compressBound(srcSize);
				auto& chunkCompressed = compressedChunks[i];
				chunkCompressed.Resize(maxSize);
				const int32 dstSize = LZ4_compress_default(chunk->Data.Get<char>(), (char*)chunkCompressed.Get(), srcSize, maxSize);
				if (dstSize <= 0)
				{
					chunkCompressed.Resize(0);
					LOG_WARNING("Resource", "Chunk data LZ4 compression failed.");
					return false;
				}
				chunkCompressed.Resize(dstSize);
			}
		}

		// Initialize chunks locations in file
		for (int32 i = 0; i < chunksCount; i++)
		{
			int32 size = chunks[i]->Size();
			if (compressedChunks[i].HasItems())
			{
				size = compressedChunks[i].Count() + sizeof(int32); // Add original data size
			}
			ENGINE_ASSERT(size > 0);
			chunks[i]->LocationInFile = StorageChunk::Location(currentAddress, size);
			currentAddress += size;
		}

		// Write header
		Header mainHeader;
		mainHeader.MagicCode = MagicCode;
		mainHeader.Version = 1;
		if (customData)
		{
			mainHeader.CustomData = *customData;
		}
		else
		{
			Platform::MemoryClear(&mainHeader.CustomData, sizeof(CustomData));
		}
		stream->Write(mainHeader);

		// Write asset entries
		stream->WriteInt32(dataCount);
		stream->WriteBytes(entries.Get(), sizeof(SerializedEntryV9) * dataCount);

		// Write chunk locations and meta
		stream->WriteInt32(chunksCount);
		for (int32 i = 0; i < chunksCount; i++)
		{
			StorageChunk* chunk = chunks[i];
			stream->WriteBytes(&chunk->LocationInFile, sizeof(chunk->LocationInFile));
			stream->WriteInt32(chunk->Flags.Get());
		}

#if ASSETS_LOADING_EXTRA_VERIFICATION

		// Check calculated position of first asset header
		if (dataCount > 0 && stream->GetPosition() != entries[0].Address)
		{
			LOG_WARNING("Resource", "Error while asset header location computation.");
			return false;
		}

#endif

		// Write asset headers
		for (int32 i = 0; i < dataCount; i++)
		{
			auto& header = data[i];

			// ID
			stream->Write(header.Header.ID);

			// Type Name
			SerializedTypeNameV9 typeName(header.Header.TypeID);
			stream->WriteBytes(typeName.Data, sizeof(SerializedTypeNameV9));

			// Serialized Version
			stream->WriteUint32(header.SerializedVersion);

			// Chunks mapping
			for (int32 chunkIndex = 0; chunkIndex < ARRAY_SIZE(header.Header.Chunks); chunkIndex++)
			{
				const int32 index = chunks.Find(header.Header.Chunks[chunkIndex]);
				stream->WriteInt32(index);
			}

			// Custom Data
			stream->WriteInt32(header.CustomData.Length());
			header.CustomData.Write(stream);

			// Header Hash Code
			stream->WriteUint32(header.GetHashCode());

			// Json Metadata
			stream->WriteInt32(header.Metadata.Length());
			header.Metadata.Write(stream);

			// Asset Dependencies
			stream->WriteInt32(header.Dependencies.Count());
			stream->WriteBytes(header.Dependencies.Get(), header.Dependencies.Count() * sizeof(Pair<UID, DateTime>));
			static_assert(sizeof(Pair<UID, DateTime>) == sizeof(UID) + sizeof(DateTime), "Invalid data size.");
		}

#if ASSETS_LOADING_EXTRA_VERIFICATION

		// Check calculated position of first asset chunk
		if (chunksCount > 0 && stream->GetPosition() != chunks[0]->LocationInFile.Address)
		{
			LOG_WARNING("Resource", "Error while asset data chunk location computation.");
			return false;
		}

#endif

		// Write chunks data
		for (int32 i = 0; i < chunksCount; i++)
		{
			if (compressedChunks[i].HasItems())
			{
				// Compressed chunk data (write additional size of the original data)
				stream->WriteInt32(chunks[i]->Data.Length());
				stream->WriteBytes(compressedChunks[i].Get(), compressedChunks[i].Count());
			}
			else
			{
				// Raw chunk data
				chunks[i]->Data.Write(stream);
			}
		}

		if (stream->HasError())
		{
			LOG_WARNING("Resource", "Stream has error.");
			return false;
		}

		return true;
	}

	bool Storage::Save(AssetInitData& data, bool silentMode)
	{
		// Check if can modify the storage
		if (!AllowDataModifications())
			return true;

		// Note: we support saving only single asset, to save more assets in single package use FlaxStorage::Create(..)

		return Create(_path, data, silentMode);
	}

#endif

	bool Storage::LoadAssetHeader(const Entry& e, AssetInitData& data)
	{
		ENGINE_ASSERT(IsLoaded());

		auto lock = Lock();

		// Open file
		auto stream = OpenFile();
		if (stream == nullptr)
		{
			return false;
		}

		// Seek
		stream->SetPosition(e.Address);

		// Switch version
		switch (_version)
		{
		case 1:
		{
			// ID
			stream->Read(data.Header.ID);

			// Type name
			SerializedTypeNameV9 typeName;
			stream->ReadBytes(typeName.Data, sizeof(SerializedTypeNameV9));
			data.Header.TypeID = TypeID(typeName.Data);
			if (stream->HasError())
			{
				LOG_WARNING("Resource", "Data stream error.");
				return false;
			}

			// Serialized Version
			stream->ReadUint32(&data.SerializedVersion);

			// Chunks mapping
			for (int32 i = 0; i < 16; i++)
			{
				int32 chunkIndex;
				stream->ReadInt32(&chunkIndex);
				if (chunkIndex >= _chunks.Count())
				{
					LOG_WARNING("Resource", "Invalid chunks mapping.");
					return false;
				}
				data.Header.Chunks[i] = chunkIndex == INVALID_INDEX ? nullptr : _chunks[chunkIndex];
			}

			// Custom data
			int32 customDataSize;
			stream->ReadInt32(&customDataSize);
			data.CustomData.Read(stream, customDataSize);

			// Header hash code
			uint32 headerHashCode;
			stream->ReadUint32(&headerHashCode);
			if (headerHashCode != data.GetHashCode())
			{
				LOG_WARNING("Resource", "Asset header data is corrupted.");
				return false;
			}

#if SE_EDITOR
			// Metadata
			int32 metadataSize;
			stream->ReadInt32(&metadataSize);
			data.Metadata.Read(stream, metadataSize);

			// Asset Dependencies
			int32 dependencies;
			stream->ReadInt32(&dependencies);
			data.Dependencies.Resize(dependencies);
			stream->ReadBytes(data.Dependencies.Get(), dependencies * sizeof(Pair<UID, DateTime>));
#endif
			break;
		}
		default:
			return false;
		}

#if ASSETS_LOADING_EXTRA_VERIFICATION
		// Validate loaded header (asset ID and type ID must be the same)
		if (e.ID != data.Header.ID)
		{
			LOG_ERROR("Resource", "Loading asset header data mismatch! Expected ID: {0}, loaded header: {1}.\nSource: {2}", e.ID, data.Header.ToString(), ToString());
		}
		if (e.Type != data.Header.TypeID)
		{
			LOG_ERROR("Resource", "Loading asset header data mismatch! Expected Type Name: {0}, loaded header: {1}.\nSource: {2}", e.Type, data.Header.ToString(), ToString());
		}
#endif

		return true;
	}

	void Storage::AddChunk(StorageChunk* chunk)
	{
		_chunks.Add(chunk);
	}

	FileReadStream* Storage::OpenFile()
	{
		auto& stream = _file.Get();
		if (stream == nullptr)
		{
			// Open file
			auto file = File::Open(_path, FileMode::OpenExisting, FileAccess::Read, FileShare::Read);
			if (file == nullptr)
			{
				LOG_ERROR("Resource", "Cannot open Storage file \'{0}\'.", _path);
				return nullptr;
			}
			Platform::AtomicIncrement(&_files);

			// Create file reading stream
			stream = New<FileReadStream>(file);
		}
		return stream;
	}

	bool Storage::CloseFileHandles()
	{
		if (Platform::AtomicRead(&_chunksLock) == 0 && Platform::AtomicRead(&_files) == 0)
		{
			return true;
		}
		PROFILE_CPU();

		// Note: this is usually called by the content manager when this file is not used or on exit
		// In those situations all the async tasks using this storage should be cancelled externally

		// Ensure that no one is using this resource
		int32 waitTime = 100;
		while (Platform::AtomicRead(&_chunksLock) != 0 && waitTime-- > 0)
		{
			Platform::Sleep(1);
		}
		if (Platform::AtomicRead(&_chunksLock) != 0)
		{
			// File can be locked by some streaming tasks (eg. AudioClip::StreamingTask or StreamModelLODTask)
			Entry e;
			for (int32 i = 0; i < GetEntriesCount(); i++)
			{
				GetEntry(i, e);
				Asset* asset = AssetContent::GetAsset(e.ID);
				if (asset)
				{
					LOG_INFO("Resource", "Canceling streaming for asset {0}", asset->ToString());
					asset->CancelStreaming();
				}
			}
		}
		waitTime = 100;
		while (Platform::AtomicRead(&_chunksLock) != 0 && waitTime-- > 0)
		{
			Platform::Sleep(1);
		}
		if (Platform::AtomicRead(&_chunksLock) != 0)
		{
			return false; // Failed, someone is still accessing the file
		}

		// Close file handles (from all threads)
		List<FileReadStream*, InlinedAllocation<8>> streams;
		_file.GetValues(streams);
		for (FileReadStream* stream : streams)
		{
			if (stream)
			{
				Delete(stream);
			}
		}
		_file.Clear();
		Platform::AtomicStore(&_files, 0);
		return true;
	}

	void Storage::Dispose()
	{
		if (IsDisposed())
			return;

		// Close file
		if (!CloseFileHandles())
		{
			LOG_ERROR("Resource", "Cannot close file access for '{}'", _path);
		}

		// Release data
		_chunks.ClearDelete();
		_version = 0;
	}

	void Storage::Tick(double time)
	{
		// Skip if file is in use
		if (Platform::AtomicRead(&_chunksLock) != 0)
			return;

		bool wasAnyUsed = false;
		const float unusedDataChunksLifetime = AssetStorages::UnusedDataChunksLifetime.GetTotalSeconds();
		for (int32 i = 0; i < _chunks.Count(); i++)
		{
			auto chunk = _chunks.Get()[i];
			const bool wasUsed = (time - chunk->LastAccessTime) < unusedDataChunksLifetime;
			if (!wasUsed && chunk->IsValid())
			{
				chunk->Unload();
			}
			wasAnyUsed |= wasUsed;
		}

		// Release file handles in none of chunks is in use
		if (!wasAnyUsed && Platform::AtomicRead(&_chunksLock) == 0)
		{
			CloseFileHandles();
		}
	}

#if SE_EDITOR

	void Storage::OnRename(const StringView& newPath)
	{
		ENGINE_ASSERT(AllowDataModifications());
		_path = newPath;
	}

#endif

	FileStorage::FileStorage(const StringView& path)
		: Storage(path)
	{
		_asset.ID = UID::Empty;
	}

	String FileStorage::ToString() const
	{
		return String::Format(SE_TEXT("Asset \'{0}\'"), _path);
	}

	bool FileStorage::IsPackage() const
	{
		return false;
	}

	bool FileStorage::AllowDataModifications() const
	{
		return true;
	}

	bool FileStorage::HasAsset(const UID& id) const
	{
		return _asset.ID == id;
	}

	bool FileStorage::HasAsset(const AssetInfo& info) const
	{
#if SE_EDITOR
		if (_path != info.path)
			return false;
#endif
		return _asset.ID == info.id && _asset.Type == info.typeID;
	}

	int32 FileStorage::GetEntriesCount() const
	{
		return _asset.ID.IsValid() ? 1 : 0;
	}

	void FileStorage::GetEntry(int32 index, Entry& output) const
	{
		ENGINE_ASSERT(index == 0);
		output = _asset;
	}

	void FileStorage::GetEntries(List<Entry>& output) const
	{
		if (_asset.ID.IsValid())
			output.Add(_asset);
	}

	void FileStorage::Dispose()
	{
		Storage::Dispose();

		_asset.ID = UID::Empty;
	}

	bool FileStorage::GetEntry(const UID& id, Entry& e)
	{
		e = _asset;
		return id == _asset.ID;
	}

	void FileStorage::AddEntry(Entry& e)
	{
		ENGINE_ASSERT(_asset.ID.IsValid() == false);
		_asset = e;
	}

	PackageStorage::PackageStorage(const StringView& path)
		: Storage(path), _entries(256)
	{
	}

	String PackageStorage::ToString() const
	{
		return String::Format(SE_TEXT("Package \'{0}\'"), _path);
	}

	bool PackageStorage::IsPackage() const
	{
		return true;
	}

	bool PackageStorage::AllowDataModifications() const
	{
		return false;
	}

	bool PackageStorage::HasAsset(const UID& id) const
	{
		return _entries.ContainsKey(id);
	}

	bool PackageStorage::HasAsset(const AssetInfo& info) const
	{
		ENGINE_ASSERT(_path == info.path);
		const Entry* e = _entries.TryGet(info.id);
		return e && e->Type == info.typeID;
	}

	int32 PackageStorage::GetEntriesCount() const
	{
		return _entries.Count();
	}

	void PackageStorage::GetEntry(int32 index, Entry& output) const
	{
		ENGINE_ASSERT(index >= 0 && index < _entries.Count());
		for (auto i = _entries.begin(); i.IsNotEnd(); ++i)
		{
			if (index-- <= 0)
			{
				output = i->Value;
				return;
			}
		}
	}

	void PackageStorage::GetEntries(List<Entry>& output) const
	{
		_entries.GetValues(output);
	}

	void PackageStorage::Dispose()
	{
		Storage::Dispose();

		_entries.Clear();
	}

	bool PackageStorage::GetEntry(const UID& id, Entry& e)
	{
		return _entries.TryGet(id, e);
	}

	void PackageStorage::AddEntry(Entry& e)
	{
		ENGINE_ASSERT(HasAsset(e.ID) == false);
		_entries.Add(e.ID, e);
	}
}