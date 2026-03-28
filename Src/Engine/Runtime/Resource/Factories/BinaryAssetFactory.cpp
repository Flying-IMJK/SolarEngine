
#include "BinaryAssetFactory.h"
#include "Runtime/Resource/Storage/AssetHeader.h"
#include "Runtime/Resource/Storage/AssetStorages.h"
#include "Runtime/Resource/BinaryAsset.h"

namespace SE
{
	bool BinaryAssetFactoryBase::Init(BinaryAsset* asset)
	{
		ENGINE_ASSERT(asset && asset->storage);
		auto storage = asset->storage;

		// Load serialized asset data
		AssetInitData initData;
		if (!storage->LoadAssetHeader(asset->GetID(), initData))
		{
			LOG_ERROR("Resource", "Cannot load asset header.\nInfo: {0}", AssetInfo(asset->GetID(), asset->GetType(), storage->GetPath()).ToString());
			return false;
		}
		
		// Check if serialized asset version is supported
		if (!IsVersionSupported(initData.SerializedVersion))
		{
			LOG_WARNING("Resource", "Asset version {1} is not supported.\nInfo: {0}", AssetInfo(asset->GetID(), asset->GetType(), storage->GetPath()).ToString(), initData.SerializedVersion);
			return false;
		}

		// Initialize asset
		if (!asset->Init(initData))
		{
			LOG_ERROR("Resource", "Cannot initialize asset.\nInfo: {0}", AssetInfo(asset->GetID(), asset->GetType(), storage->GetPath()).ToString());
			return false;
		}

		return true;
	}

	Asset* BinaryAssetFactoryBase::New(const AssetInfo* info)
	{
		// Get the asset storage container but don't load it now
		const auto storage = AssetStorages::GetStorage(info->path, false);
		if (!storage)
		{
			// Note: missing file situation should be handled before asset creation
			LOG_WARNING("Resource", "Missing asset storage container at \'{0}\'!\nInfo: ", info->path, info->ToString());
			return nullptr;
		}

		// Create asset object
		auto result = Create(info);

		// Perform fast init, we assume that given AssetInfo is valid 
		// and we can create asset object now without further verification
		// which will be done during asset loading on content pool thread.
		// Then we will perform asset storage upgrading and loading.
		AssetHeader header;
		header.ID = info->id;
		header.TypeID = info->typeID;
		if (!result->Init(storage, header))
		{
			LOG_WARNING("Resource", "Cannot initialize asset.\nInfo: {0}", info->ToString());
			Delete(result);
			result = nullptr;
		}

		return result;
	}

	Asset* BinaryAssetFactoryBase::NewVirtual(const AssetInfo* info)
	{
		// Create asset object
		auto result = Create(info);

		// Initialize with virtual data
		AssetInitData initData;
		initData.Header.ID = info->id;
		initData.Header.TypeID = info->typeID;
		initData.SerializedVersion = result->GetSerializedVersion();
		if (!result->InitVirtual(initData))
		{
			LOG_WARNING("Resource", "Cannot initialize asset.\nInfo: {0}", info->ToString());
			Delete(result);
			result = nullptr;
		}

		return result;
	}
} // SE