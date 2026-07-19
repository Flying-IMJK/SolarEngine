#pragma once

#include "IAssetFactory.h"
#include "Runtime/Core/TypeSystem/Types.h"
#include "Runtime/Resource/AssetInfo.h"
#include "Runtime/Core/Scripting/ScriptingType.h"

namespace SE
{

	class BinaryAsset;
	class Storage;

	/// <summary>
	/// The binary assets factory base class.
	/// </summary>
	/// <seealso cref="IAssetFactory" />
	class SE_API_RUNTIME BinaryAssetFactoryBase : public IAssetFactory
	{
	public:
		/// <summary>
		/// Initializes the specified asset. It's called in background before actual asset loading.
		/// </summary>
		/// <param name="asset">The asset.</param>
		/// <returns>True if failed, otherwise false.</returns>
		bool Init(BinaryAsset* asset);

	protected:
		virtual BinaryAsset* Create(const AssetInfo* info) = 0;
		virtual bool IsVersionSupported(uint32 serializedVersion) const = 0;
	public:
		// [IAssetFactory]
		Asset* New(const AssetInfo* info) override;
		Asset* NewVirtual(const AssetInfo* info) override;
	};

	/// <summary>
	/// The binary assets factory.
	/// </summary>
	/// <seealso cref="BinaryAssetFactoryBase" />
	template<typename T, bool supportsVirtual = false>
	class BinaryAssetFactory : public BinaryAssetFactoryBase
	{
	public:
		// [BinaryAssetFactoryBase]
		bool IsVersionSupported(uint32 serializedVersion) const override
		{
			return true;// T::SerializedVersion == serializedVersion;
		}

		bool SupportsVirtualAssets() const override
		{
			return supportsVirtual;
		}

	protected:
		TypeID GetTypeID() override
		{
			return Typeof<T>();
		}
	protected:
		// [BinaryAssetFactoryBase]
		BinaryAsset* Create(const AssetInfo* info) override
		{
			ScriptingObjectSpawnParams params(info->id, GetScriptingType<T>());
			return ::SE::New<T>(params, info);
		}
	};

}// SE
