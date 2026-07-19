
#pragma once

#include "Runtime/Core/Types/UID.h"
#include "Runtime/Core/Types/Strings/String.h"
#include "Runtime/Core/TypeSystem/TypeID.h"
#if SE_EDITOR
#include "Runtime/Core/Types/DateTime.h"
#include "Runtime/Core/Types/Collections/List.h"
#endif
#include "Runtime/Core/Types/Collections/DataContainer.h"

#include "StorageChunk.h"


namespace SE
{

	/// <summary>
	/// Marco that computes chunk flag value from chunk zero-index
	/// </summary>
	#define GET_CHUNK_FLAG(chunkIndex) (1 << chunkIndex)

	typedef uint16 AssetChunksFlag;
	#define ALL_ASSET_CHUNKS Max_uint16

	/// <summary>
	/// Asset header
	/// </summary>
	struct SE_API_RUNTIME AssetHeader
	{
		/// <summary>
		/// Unique asset ID
		/// </summary>
		UID ID;

		/// <summary>
		/// Asset type name
		/// </summary>
		TypeID TypeID;

		/// <summary>
		/// The asset chunks.
		/// </summary>
		StorageChunk* Chunks[ASSET_FILE_DATA_CHUNKS];

	public:
		/// <summary>
		/// Initializes a new instance of the <see cref="AssetHeader"/> struct.
		/// </summary>
		AssetHeader()
		{
			// Cleanup all data
			ID = UID::Empty;
			Platform::MemoryClear(Chunks, sizeof(Chunks));
		}

	public:
		/// <summary>
		/// Gets the chunks.
		/// </summary>
		/// <param name="output">The output data.</param>
		template<typename AllocationType = HeapAllocation>
		void GetChunks(List<StorageChunk*, AllocationType>& output) const
		{
			for (int32 i = 0; i < ASSET_FILE_DATA_CHUNKS; i++)
			{
				if (Chunks[i] != nullptr)
					output.Add(Chunks[i]);
			}
		}

		/// <summary>
		/// Gets the chunks that are loaded.
		/// </summary>
		/// <param name="output">The output data.</param>
		template<typename AllocationType = HeapAllocation>
		void GetLoadedChunks(List<StorageChunk*, AllocationType>& output) const
		{
			for (int32 i = 0; i < ASSET_FILE_DATA_CHUNKS; i++)
			{
				if (Chunks[i] != nullptr && Chunks[i]->IsValid())
					output.Add(Chunks[i]);
			}
		}

		/// <summary>
		/// Gets the amount of created asset chunks.
		/// </summary>
		int32 GetChunksCount() const;

		/// <summary>
		/// Deletes all chunks. Warning! Chunks are managed internally, use with caution!
		/// </summary>
		void DeleteChunks();

		/// <summary>
		/// Unlinks all chunks.
		/// </summary>
		void UnlinkChunks();

		/// <summary>
		/// Gets string with a human-readable info about that header
		/// </summary>
		/// <returns>Header info string</returns>
		String ToString() const;
	};

	/// <summary>
	/// Asset header data
	/// </summary>
	struct SE_API_RUNTIME AssetInitData
	{
		/// <summary>
		/// The asset header.
		/// </summary>
		AssetHeader Header;

		/// <summary>
		/// The serialized asset version
		/// </summary>
		uint32 SerializedVersion = 0;

		/// <summary>
		/// The custom asset data (should be small, for eg. texture description structure).
		/// </summary>
		BytesContainer CustomData;

#if SE_EDITOR
		/// <summary>
		/// The asset metadata information. Stored in a Json format.
		/// </summary>
		BytesContainer Metadata;

		/// <summary>
		/// Asset dependencies list used by the asset for tracking (eg. material functions used by material asset). The pair of asset ID and cached file edit time (for tracking modification).
		/// </summary>
		List<Pair<UID, DateTime>> Dependencies;
#endif

	public:
		/// <summary>
		/// Gets the hash code.
		/// </summary>
		uint32 GetHashCode() const
		{
			// Note: do not use Metadata/Dependencies because it may not be loaded (it's optional)
			uint32 hashCode = GetHash(Header.ID);
			hashCode = (hashCode * 397) ^ SerializedVersion;
			hashCode = (hashCode * 397) ^ CustomData.Length();
			return hashCode;
		}
	};
}