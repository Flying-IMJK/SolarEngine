#pragma once

#include "Core/TypeSystem/TypeID.h"
#include "Core/TypeSystem/Types.h"
#include "Runtime/API.h"


namespace SE
{
	struct AssetInfo;
	class Asset;
	class AssetFactoryRegister;

	/// <summary>
	/// The asset objects factory.
	/// </summary>
	class SE_API_RUNTIME IAssetFactory
	{
	public:
		typedef Dictionary<TypeID, IAssetFactory*> Collection;

		/// <summary>
		/// Gets the all registered assets factories. Key is asset typename, value is the factory object.
		/// </summary>
		static List<AssetFactoryRegister*>& GetRegisterFactoryList();

	public:
		/// <summary>
		/// Finalizes an instance of the <see cref="IAssetFactory"/> class.
		/// </summary>
		virtual ~IAssetFactory()
		{
		}

	public:
		/// <summary>
		/// Determines whenever the virtual assets are supported by this asset tpe factory.
		/// </summary>
		/// <returns>True if can create virtual assets, otherwise false.</returns>
		virtual bool SupportsVirtualAssets() const
		{
			return false;
		}

		/// <summary>
		/// Creates new asset instance.
		/// </summary>
		/// <param name="info">The asset info structure.</param>
		/// <returns>Created asset object.</returns>
		virtual Asset* New(const AssetInfo* info) = 0;

		/// <summary>
		/// Creates new virtual asset instance. Virtual assets are temporary and exist until application exit.
		/// </summary>
		/// <param name="info">The asset info structure.</param>
		/// <returns>Created asset object.</returns>
		virtual Asset* NewVirtual(const AssetInfo* info) = 0;

	protected:
		virtual TypeID GetTypeID() = 0;
	};

	class AssetFactoryRegister
	{
	public:
		virtual ~AssetFactoryRegister() = default;
		AssetFactoryRegister()
		{
			IAssetFactory::GetRegisterFactoryList().Add(this);
		};
		virtual IAssetFactory* Create() = 0;
		virtual TypeID GetAssetType() = 0;
	};

	template<typename TFactory, typename TAsset>
	class TAssetFactoryRegister final : public AssetFactoryRegister
	{
	public:
		IAssetFactory* Create() override
		{
			return New<TFactory>();
		}

		TypeID GetAssetType() override
		{
			return Typeof<TAsset>();
		}
	};

	#define BINARY_ASSET_FACTORY(Asset, supportsVirtual) TAssetFactoryRegister<BinaryAssetFactory<Asset, supportsVirtual>, Asset> factory##Asset

	#define JSON_ASSET_FACTORY(Asset) TAssetFactoryRegister<JsonAssetFactory<Asset>, Asset> factory##Asset
}