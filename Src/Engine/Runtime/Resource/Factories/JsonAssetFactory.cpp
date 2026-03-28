
#include "JsonAssetFactory.h"
#include "Runtime/Resource/JsonAsset.h"

namespace SE
{
	Asset* JsonAssetFactoryBase::New(const AssetInfo* info)
	{
		return Create(info);
	}

	Asset* JsonAssetFactoryBase::NewVirtual(const AssetInfo* info)
	{
		return Create(info);
	}
}
