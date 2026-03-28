
#pragma once

#include "../AssetTask.h"
#include "Runtime/Resource/Asset.h"
#include "Runtime/Resource/AssetRef.h"

namespace SE
{
	class Asset;

	/// <summary>
	/// Asset loading task object.
	/// </summary>
	class LoadAssetTask : public AssetTask
	{
	public:
		/// <summary>
		/// Initializes a new instance of the <see cref="LoadAssetTask"/> class.
		/// </summary>
		/// <param name="asset">The asset to load.</param>
		LoadAssetTask(Asset* asset) : Asset(asset)
		{
		}

		~LoadAssetTask() override;

	public:
		WeakAssetRef<Asset> Asset;

		// [AssetLoadTask]
		bool HasReference(void * obj) const override;

		String ToString() const override;

	protected:
		// [AssetLoadTask]
		Result Process() override;

		void OnFail() override;

		void OnEnd() override;
	};


}
