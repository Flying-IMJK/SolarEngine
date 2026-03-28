#pragma once

#include "Core/Types/Collections/List.h"
#include "Runtime/API.h"

namespace SE
{
	class WriteStream;
	class ReadStream;

	/// <summary>
	/// Graph metadata container
	/// </summary>
	class SE_API_RUNTIME GraphMeta
	{
	public:
		/// <summary>
		/// Metadata entry
		/// </summary>
		struct Entry
		{
			int32 TypeID;
			bool IsLoaded;
			List<byte> Data;
		};

	public:
		/// <summary>
		/// All meta entries
		/// </summary>
		List<Entry, FixedAllocation<8>> Entries;

	public:
		/// <summary>
		/// Load from the stream
		/// </summary>
		/// <param name="stream">Stream</param>
		/// <param name="loadData">True if load meta data</param>
		/// <returns>True if cannot load data</returns>
		bool Load(ReadStream* stream, bool loadData);

		/// <summary>
		/// Save to the stream
		/// </summary>
		/// <param name="stream">Stream</param>
		/// <param name="saveData">True if load meta data</param>
		/// <returns>True if cannot save data</returns>
		bool Save(WriteStream* stream, bool saveData) const;

		/// <summary>
		/// Release meta data
		/// </summary>
		void Release();

		/// <summary>
		/// Get entry
		/// </summary>
		/// <param name="typeID">Entry type ID</param>
		/// <returns>Entry</returns>
		const Entry* GetEntry(int32 typeID) const;

		/// <summary>
		/// Get entry
		/// </summary>
		/// <param name="typeID">Entry type ID</param>
		/// <returns>Entry</returns>
		Entry* GetEntry(int32 typeID);

		/// <summary>
		/// Add new entry
		/// </summary>
		/// <param name="typeID">Type ID</param>
		/// <param name="data">Bytes to set</param>
		/// <param name="size">Amount of bytes to assign</param>
		void AddEntry(int32 typeID, byte* data, int32 size);
	};
} // SE

