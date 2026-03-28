
#pragma once

#include "Storage.h"

namespace SE
{
	/// <summary>
	/// Flax Game Engine asset files container object.
	/// </summary>
	class SE_API_RUNTIME FileStorage : public Storage
	{
	protected:
		Entry _asset;

	public:
		/// <summary>
		/// Initializes a new instance of the <see cref="FlaxFile"/> class.
		/// </summary>
		/// <param name="path">The path.</param>
		FileStorage(const StringView& path);

	public:
		// [FlaxStorage]
		String ToString() const override;
		bool IsPackage() const override;
		bool AllowDataModifications() const override;
		bool HasAsset(const UID& id) const override;
		bool HasAsset(const AssetInfo& info) const override;
		int32 GetEntriesCount() const override;
		void GetEntry(int32 index, Entry& output) const override;
		void GetEntries(List<Entry>& output) const override;
		void Dispose() override;

	protected:
		// [FlaxStorage]
		bool GetEntry(const UID& id, Entry& e) override;
		void AddEntry(Entry& e) override;
	};
}