#pragma once
#include "AssetRef.h"
#include "Core/Types/Collections/List.h"

namespace SE
{
	/// <summary>
	/// Assets Container allows to load collection of assets and keep references to them.
	/// </summary>
	class AssetsContainer : public List<AssetRef<Asset>>
	{
	public:
		/// <summary>
		/// Loads an asset.
		/// </summary>
		/// <param name="id">The asset id.</param>
		/// <returns>Loaded asset of null.</returns>
		template<typename T>
		T* LoadAsync(const UID& id)
		{
			for (auto& e : *this)
			{
				if (e.GetID() == id)
				{
					return (T*)e.Get();
				}
			}
			auto asset = (T*)AssetContent::LoadAsync<T>(id);
			if (asset)
				Add(asset);
			return asset;
		}

		/// <summary>
		/// Release all referenced assets.
		/// </summary>
		void ReleaseAll()
		{
			Resize(0);
		}
	};
}
