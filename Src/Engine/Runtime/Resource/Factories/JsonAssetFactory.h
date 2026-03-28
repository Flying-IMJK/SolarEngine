#pragma once
#include "IAssetFactory.h"

namespace SE
{
	class JsonAssetBase;

	/// <summary>
	/// The Json assets factory base class.
	/// </summary>
	/// <seealso cref="IAssetFactory" />
	class SE_API_RUNTIME JsonAssetFactoryBase : public IAssetFactory
	{
	protected:
		virtual JsonAssetBase* Create(const AssetInfo* info) = 0;

	public:
		// [IAssetFactory]
		Asset* New(const AssetInfo* info) override;

		Asset* NewVirtual(const AssetInfo* info) override;
	};


	/// <summary>
	/// The Json assets factory.
	/// </summary>
	/// <seealso cref="JsonAssetFactoryBase" />
	template<typename T>
	class JsonAssetFactory : public JsonAssetFactoryBase
	{
	protected:
		TypeID GetTypeID() override
		{
			return Typeof<T>();
		}

		// [JsonAssetFactoryBase]
		JsonAssetBase* Create(const AssetInfo* info) override
		{
			return ::SE::New<T>(info);
		}
	};
}
