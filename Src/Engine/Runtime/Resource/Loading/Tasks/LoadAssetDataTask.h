
#pragma once

#include "../AssetTask.h"
#include "Runtime/Resource/AssetRef.h"
#include "Runtime/Resource/BinaryAsset.h"

namespace SE
{
	/// <summary>
	/// Asset data loading task object.
	/// </summary>
	class LoadAssetDataTask : public AssetTask
	{
	private:
		WeakAssetRef<BinaryAsset> _asset; // Don't keep ref to the asset (so it can be unloaded if none using it, task will fail then)
		AssetChunksFlag _chunks;
		Storage::LockData _dataLock;

	public:
		/// <summary>
		/// Initializes a new instance of the <see cref="LoadAssetDataTask"/> class.
		/// </summary>
		/// <param name="asset">The asset to load.</param>
		/// <param name="chunks">The chunks to load.</param>
		LoadAssetDataTask(BinaryAsset* asset, AssetChunksFlag chunks)
			: _asset(asset)
			, _chunks(chunks)
			, _dataLock(asset->storage->Lock())
		{
		}

	public:
		// [AssetLoadTask]
		bool HasReference(void* obj) const override;

	protected:
		// [AssetLoadTask]
		Result Process() override;

		void OnEnd() override;
	};






}
