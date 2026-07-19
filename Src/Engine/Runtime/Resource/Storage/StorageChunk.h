
#pragma once

#include "../AssetConfig.h"

#include "Runtime/API.h"
#include "Runtime/Core/Types/Collections/DataContainer.h"
#include "Runtime/Core/Types/BitFlags.h"

namespace SE
{
	/// <summary>
	/// Custom flags for the storage chunk data.
	/// </summary>
	enum class AssetChunkFlags
	{
		/// <summary>
		/// The none.
		/// </summary>
		None = 0,

		/// <summary>
		/// Compress chunk data using LZ4 algorithm.
		/// </summary>
		CompressedLZ4 = 1,
	};

	/// <summary>
	/// Represents chunks of data used by the content storage layer
	/// </summary>
	class SE_API_RUNTIME StorageChunk
	{
	public:
		/// <summary>
		/// 数据块位置信息
		/// </summary>
		struct Location
		{
			/// <summary>
			/// Address of the chunk beginning in file
			/// </summary>
			uint32 Address;

			/// <summary>
			/// 块大小（以字节为单位）
			/// Note: 大小等于 0 的 chunk 被视为不存在
			/// </summary>
			uint32 Size;

			/// <summary>
			/// Init
			/// </summary>
			Location() : Address(0), Size(0)
			{
			}

			/// <summary>
			/// Init
			/// </summary>
			/// <param name="location">The location.</param>
			/// <param name="size">The size.</param>
			Location(uint32 location, uint32 size) : Address(location), Size(size)
			{
			}
		};

	public:
		/// <summary>
		/// The chunk location in file.
		/// </summary>
		Location LocationInFile;

		/// <summary>
		/// The chunk flags.
		/// </summary>
		EnumFlags<AssetChunkFlags> Flags = EnumFlags<AssetChunkFlags>(AssetChunkFlags::None);

		/// <summary>
		/// The last usage time.
		/// </summary>
		double LastAccessTime = 0.0;

		/// <summary>
		/// The chunk data.
		/// </summary>
		BytesContainer Data;

	public:
		/// <summary>
		/// Initializes a new instance of the <see cref="AssetChunk"/> class.
		/// </summary>
		StorageChunk()
		{
		}

		/// <summary>
		/// Finalizes an instance of the <see cref="AssetChunk"/> class.
		/// </summary>
		~StorageChunk()
		{
			LocationInFile = Location();
			Flags = AssetChunkFlags::None;
			LastAccessTime = 0.0;
			Data.Release();
		}

	public:
		/// <summary>
		/// Gets this chunk data pointer.
		/// </summary>
		FORCE_INLINE byte* Get()
		{
			return Data.Get();
		}

		/// <summary>
		/// Gets this chunk data pointer.
		/// </summary>
		FORCE_INLINE const byte* Get() const
		{
			return Data.Get();
		}

		/// <summary>
		/// Gets this chunk data pointer.
		/// </summary>
		template<typename T>
		FORCE_INLINE T* Get() const
		{
			return (T*)Data.Get();
		}

		/// <summary>
		/// Gets this chunk data size (in bytes).
		/// </summary>
		FORCE_INLINE int32 Size() const
		{
			return Data.Length();
		}

		/// <summary>
		/// Determines whether this chunk is valid.
		/// </summary>
		FORCE_INLINE bool IsValid() const
		{
			return Data.IsValid();
		}

		/// <summary>
		/// Determines whether this chunk exists in a file.
		/// </summary>
		FORCE_INLINE bool ExistsInFile() const
		{
			return LocationInFile.Size > 0;
		}

		/// <summary>
		/// Registers the usage operation of chunk data.
		/// </summary>
		void RegisterUsage();

		/// <summary>
		/// Unloads this chunk data.
		/// </summary>
		void Unload()
		{
			Data.Release();
		}

		/// <summary>
		/// Clones this chunk data (doesn't copy location in file).
		/// </summary>
		/// <returns>The cloned chunk.</returns>
		StorageChunk* Clone() const
		{
			auto chunk = New<StorageChunk>();
			chunk->Data.Copy(Data);
			return chunk;
		}
	};
}
