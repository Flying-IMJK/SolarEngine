#pragma once

#include "AssetInfo.h"
#include "Asset.h"
#include "Runtime/Core/TypeSystem/Types.h"

namespace SE
{
	class Engine;
	class AssetsCache;
	class IAssetFactory;
	struct ScriptingTypeHandle;

	// Content and assets statistics container.
	SE_STRUCT(API())
	struct SE_API_RUNTIME ContentStats
	{
		SCRIPTING_TYPE_MIN(ContentStats);

		// Amount of asset objects in memory.
		SE_FIELD(API())
		int32 AssetsCount = 0;
		// Amount of loaded assets.
		SE_FIELD(API())
		int32 LoadedAssetsCount = 0;
		// Amount of loading assets. Zero if all assets are loaded in.
		SE_FIELD(API())
		int32 LoadingAssetsCount = 0;
		// Amount of virtual assets (don't have representation in file).
		SE_FIELD(API())
		int32 VirtualAssetsCount = 0;


	};

	/// <summary>
	/// 加载和管理资源
	/// </summary>
	SE_CLASS(API(Static))
	class SE_API_RUNTIME AssetContent
	{
		SCRIPTING_TYPE_NO_SPAWN(AssetContent);
		friend Engine;
		friend Asset;
		friend AssetTask;
		friend LoadAssetTask;
	public:
		/// <summary>
		/// The time between content pool updates.
		/// </summary>
		static TimeSpan AssetsUpdateInterval;

		/// <summary>
		/// The time after asset with no references will be unloaded.
		/// </summary>
		static TimeSpan AssetsUnloadInterval;

	public:
		/// <summary>
		/// Gets the assets registry.
		/// </summary>
		/// <returns>The assets cache.</returns>
		static AssetsCache* GetRegistry();

	public:
		/// <summary>
		/// Finds the asset info by id.
		/// </summary>
		/// <param name="id">The asset id.</param>
		/// <param name="info">The output asset info. Filled with valid values only if method returns true.</param>
		/// <returns>True if found any asset, otherwise false.</returns>
		SE_FUNCTION(API())
		static bool GetAssetInfo(const UID& id, AssetInfo& info);

		/// <summary>
		/// Finds the asset info by path.
		/// </summary>
		/// <param name="path">The asset path.</param>
		/// <param name="info">The output asset info. Filled with valid values only if method returns true.</param>
		/// <returns>True if found any asset, otherwise false.</returns>
		SE_FUNCTION(API())
		static bool GetAssetInfo(const StringView& path, AssetInfo& info);

		/// <summary>
		/// Finds the asset path by id. In editor it returns the actual asset path, at runtime it returns the mapped asset path.
		/// </summary>
		/// <param name="id">The asset id.</param>
		/// <returns>The asset path, or empty if failed to find.</returns>
		SE_FUNCTION(API())
		static String GetEditorAssetPath(const UID& id);

		/// <summary>
		/// Finds all the asset IDs. Uses asset registry.
		/// </summary>
		/// <returns>The list of all asset IDs.</returns>
		SE_FUNCTION(API())
		static List<UID, HeapAllocation> GetAllAssets();

		/// <summary>
		/// Finds all the asset IDs by type (exact type, without inheritance checks). Uses asset registry.
		/// </summary>
		/// <param name="type">The asset type.</param>
		/// <returns>The list of asset IDs that match the given type.</returns>
		SE_FUNCTION(API())
		static List<UID, HeapAllocation> GetAllAssetsByType(const TypeID type);

	public:
		/// <summary>
		/// Gets the asset factory used by the given asset type id.
		/// </summary>
		/// <param name="typeName">The asset type name identifier.</param>
		/// <returns>Asset factory or null if not found.</returns>
		static IAssetFactory* GetAssetFactory(const TypeID type);

		/// <summary>
		/// Gets the asset factory used by the given asset type id.
		/// </summary>
		/// <param name="assetInfo">The asset info.</param>
		/// <returns>Asset factory or null if not found.</returns>
		static IAssetFactory* GetAssetFactory(const AssetInfo& assetInfo);

	public:
		/// <summary>
		/// Generates temporary asset path.
		/// </summary>
		/// <returns>Asset path for a temporary usage.</returns>
		SE_FUNCTION(API())
		static String CreateTemporaryAssetPath();

	public:
		/// <summary>
		/// Gets content statistics.
		/// </summary>
		SE_FUNCTION(API(Prop))
		static ContentStats GetStats();

		/// <summary>
		/// Gets the assets (loaded or during load).
		/// </summary>
		/// <returns>The collection of assets.</returns>
		SE_FUNCTION(API())
		static List<Asset*, HeapAllocation> GetAssets();

		/// <summary>
		/// Gets the raw dictionary of assets (loaded or during load).
		/// </summary>
		/// <returns>The collection of assets.</returns>
		static const Dictionary<UID, Asset*, HeapAllocation>& GetAssetsRaw();

		/// <summary>
		/// Loads asset and holds it until it won't be referenced by any object. Returns null if asset is missing. Actual asset data loading is performed on a other thread in async.
		/// </summary>
		/// <param name="id">Asset unique ID</param>
		/// <param name="type">The asset type. If loaded object has different type (excluding types derived from the given) the loading fails.</param>
		/// <returns>Loaded asset or null if cannot</returns>
		static Asset* LoadAsync(const UID& id, const CLRClass* type);

		/// <summary>
		/// Loads asset and holds it until it won't be referenced by any object. Returns null if asset is missing. Actual asset data loading is performed on a other thread in async.
		/// </summary>
		/// <param name="id">Asset unique ID</param>
		/// <param name="type">The asset scripting type. If loaded object has different type (excluding types derived from the given) the loading fails.</param>
		/// <returns>Loaded asset or null if cannot</returns>
		SE_FUNCTION(API())
		static Asset* LoadAsync(const UID& id, const ScriptingTypeHandle& type);

		/// <summary>
		/// Loads asset and holds it until it won't be referenced by any object. Returns null if asset is missing. Actual asset data loading is performed on a other thread in async.
		/// </summary>
		/// <param name="id">Asset unique ID</param>
		/// <param name="type">The asset type. If loaded object has different type (excluding types derived from the given) the loading fails.</param>
		/// <returns>Loaded asset or null if cannot</returns>
		static Asset* LoadAsync(const UID& id, const TypeID type);

		/// <summary>
		/// Loads asset and holds it until it won't be referenced by any object. Returns null if asset is missing. Actual asset data loading is performed on a other thread in async.
		/// </summary>
		/// <param name="id">Asset unique ID</param>
		/// <typeparam name="T">Type of the asset to load. Includes any asset types derived from the type.</typeparam>
		/// <returns>Loaded asset or null if cannot</returns>
		template<typename T>
		FORCE_INLINE static T* LoadAsync(const UID& id)
		{
			return static_cast<T*>(LoadAsync(id, Typeof<T>()));
		}

		/// <summary>
		/// Loads asset and holds it until it won't be referenced by any object. Returns null if asset is missing. Actual asset data loading is performed on a other thread in async.
		/// </summary>
		/// <param name="path">The path of the asset (absolute or relative to the current workspace directory).</param>
		/// <param name="type">The asset type. If loaded object has different type (excluding types derived from the given) the loading fails.</param>
		/// <returns>Loaded asset or null if cannot</returns>
		static Asset* LoadAsync(const StringView& path, const CLRClass* type);

		/// <summary>
		/// Loads asset and holds it until it won't be referenced by any object. Returns null if asset is missing. Actual asset data loading is performed on a other thread in async.
		/// </summary>
		/// <param name="path">The path of the asset (absolute or relative to the current workspace directory).</param>
		/// <param name="type">The asset scripting type. If loaded object has different type (excluding types derived from the given) the loading fails.</param>
		/// <returns>Loaded asset or null if cannot</returns>
		SE_FUNCTION(API())
		static Asset* LoadAsync(const StringView& path, const ScriptingTypeHandle& type);

		/// <summary>
		/// Loads asset and holds it until it won't be referenced by any object. Returns null if asset is missing. Actual asset data loading is performed on a other thread in async.
		/// </summary>
		/// <param name="path">The path of the asset (absolute or relative to the current workspace directory).</param>
		/// <param name="type">The asset type. If loaded object has different type (excluding types derived from the given) the loading fails.</param>
		/// <returns>Loaded asset or null if cannot</returns>
		static Asset* LoadAsync(const StringView& path, const TypeID& type);

		/// <summary>
		/// Loads asset and holds it until it won't be referenced by any object. Returns null if asset is missing. Actual asset data loading is performed on a other thread in async.
		/// </summary>
		/// <param name="path">The path of the asset (absolute or relative to the current workspace directory).</param>
		/// <typeparam name="T">Type of the asset to load. Includes any asset types derived from the type.</typeparam>
		/// <returns>Loaded asset or null if cannot</returns>
		template<typename T>
		FORCE_INLINE static T* LoadAsync(const StringView& path)
		{
			return static_cast<T*>(LoadAsync(path, Typeof<T>()));
		}

		/// <summary>
		/// Loads internal engine asset and holds it until it won't be referenced by any object. Returns null if asset is missing. Actual asset data loading is performed on a other thread in async.
		/// </summary>
		/// <param name="internalPath">The path of the asset relative to the engine internal content (excluding the extension).</param>
		/// <param name="type">The asset type. If loaded object has different type (excluding types derived from the given) the loading fails.</param>
		/// <returns>The loaded asset or null if failed.</returns>
		static Asset* LoadAsyncInternal(const StringView& internalPath, const CLRClass* type);

		/// <summary>
		/// Loads internal engine asset and holds it until it won't be referenced by any object. Returns null if asset is missing. Actual asset data loading is performed on a other thread in async.
		/// </summary>
		/// <param name="internalPath">The path of the asset relative to the engine internal content (excluding the extension).</param>
		/// <param name="type">The asset scripting type. If loaded object has different type (excluding types derived from the given) the loading fails.</param>
		/// <returns>The loaded asset or null if failed.</returns>
		SE_FUNCTION(API())
		static Asset* LoadAsyncInternal(const StringView& internalPath, const ScriptingTypeHandle& type);

		/// <summary>
		/// Loads internal engine asset and holds it until it won't be referenced by any object. Returns null if asset is missing. Actual asset data loading is performed on a other thread in async.
		/// </summary>
		/// <param name="internalPath">The path of the asset relative to the engine internal content (excluding the extension).</param>
		/// <param name="type">The asset type. If loaded object has different type (excluding types derived from the given) the loading fails.</param>
		/// <returns>The loaded asset or null if failed.</returns>
		static Asset* LoadAsyncInternal(StringView internalPath, const TypeID& type);

		/// <summary>
		/// Loads internal engine asset and holds it until it won't be referenced by any object. Returns null if asset is missing. Actual asset data loading is performed on a other thread in async.
		/// </summary>
		/// <param name="internalPath">The path of the asset relative to the engine internal content (excluding the extension).</param>
		/// <param name="type">The asset type. If loaded object has different type (excluding types derived from the given) the loading fails.</param>
		/// <returns>The loaded asset or null if failed.</returns>
		static Asset* LoadAsyncInternal(const Char* internalPath, const TypeID& type);

		/// <summary>
		/// Loads internal engine asset and holds it until it won't be referenced by any object. Returns null if asset is missing. Actual asset data loading is performed on a other thread in async.
		/// </summary>
		/// <param name="internalPath">The path of the asset relative to the engine internal content (excluding the extension).</param>
		/// <param name="type">The asset scripting type. If loaded object has different type (excluding types derived from the given) the loading fails.</param>
		/// <returns>The loaded asset or null if failed.</returns>
		static Asset* LoadAsyncInternal(const Char* internalPath, const ScriptingTypeHandle& type);

		/// <summary>
		/// Loads internal engine asset and holds it until it won't be referenced by any object. Returns null if asset is missing. Actual asset data loading is performed on a other thread in async.
		/// </summary>
		/// <param name="internalPath">The path of the asset relative to the engine internal content (excluding the extension).</param>
		/// <returns>The loaded asset or null if failed.</returns>
		template<typename T>
		FORCE_INLINE static T* LoadAsyncInternal(const Char* internalPath)
		{
			return static_cast<T*>(LoadAsyncInternal(internalPath, Typeof<T>()));
		}

		template<typename T>
		FORCE_INLINE static T* LoadAsyncInternal(const StringView internalPath)
		{
			return static_cast<T*>(LoadAsyncInternal(internalPath, Typeof<T>()));
		}

		/// <summary>
		/// Loads asset to the Content Pool and holds it until it won't be referenced by any object. Returns null if asset is missing. Actual asset data loading is performed on a other thread in async.
		/// Waits until asset will be loaded. It's equivalent to LoadAsync + WaitForLoaded.
		/// </summary>
		/// <param name="id">Asset unique ID.</param>
		/// <param name="timeoutInMilliseconds">Custom timeout value in milliseconds.</param>
		/// <typeparam name="T">Type of the asset to load. Includes any asset types derived from the type.</typeparam>
		/// <returns>Asset instance if loaded, null otherwise.</returns>
		template<typename T>
		static T* Load(const UID& id, double timeoutInMilliseconds = 30000.0)
		{
			auto asset = LoadAsync<T>(id);
			if (asset && !asset->WaitForLoaded(timeoutInMilliseconds))
				return asset;
			return nullptr;
		}

		/// <summary>
		/// Loads asset to the Content Pool and holds it until it won't be referenced by any object. Returns null if asset is missing. Actual asset data loading is performed on a other thread in async.
		/// Waits until asset will be loaded. It's equivalent to LoadAsync + WaitForLoaded.
		/// </summary>
		/// <param name="path">The path of the asset (absolute or relative to the current workspace directory).</param>
		/// <param name="timeoutInMilliseconds">Custom timeout value in milliseconds.</param>
		/// <typeparam name="T">Type of the asset to load. Includes any asset types derived from the type.</typeparam>
		/// <returns>Asset instance if loaded, null otherwise.</returns>
		template<typename T>
		static T* Load(const StringView& path, double timeoutInMilliseconds = 30000.0)
		{
			auto asset = LoadAsync<T>(path);
			if (asset && asset->WaitForLoaded(timeoutInMilliseconds))
			{
				return asset;
			}
			return nullptr;
		}

		/// <summary>
		/// Determines whether input asset type id is invalid.
		/// </summary>
		/// <param name="type">The requested type of the asset to be.</param>
		/// <param name="assetType">The actual type of the asset.</param>
		/// <returns><c>true</c> if asset type identifier is invalid otherwise, <c>false</c>.</returns>
		static bool IsAssetTypeIdInvalid(const TypeID& typeID, const TypeID& assetTypeID);

		/// <summary>
		/// Determines whether input asset scripting type is invalid.
		/// </summary>
		/// <param name="type">The requested type of the asset to be.</param>
		/// <param name="assetType">The actual type of the asset.</param>
		/// <returns><c>true</c> if asset type identifier is invalid otherwise, <c>false</c>.</returns>
		static bool IsAssetTypeIdInvalid(const ScriptingTypeHandle& type, const ScriptingTypeHandle& assetType);

	public:
		/// <summary>
		/// Finds the asset with at given path. Checks all loaded assets.
		/// </summary>
		/// <param name="path">The path.</param>
		/// <returns>The found asset or null if not loaded.</returns>
		SE_FUNCTION(API())
		static Asset* GetAsset(const StringView& path);

		/// <summary>
		/// Finds the asset with given ID. Checks all loaded assets.
		/// </summary>
		/// <param name="id">The id.</param>
		/// <returns>The found asset or null if not loaded.</returns>
		SE_FUNCTION(API())
		static Asset* GetAsset(const UID& id);

	public:
		/// <summary>
		/// Deletes the specified asset.
		/// </summary>
		/// <param name="asset">The asset.</param>
		SE_FUNCTION(API())
		static void DeleteAsset(Asset* asset);

		/// <summary>
		/// Deletes the asset at the specified path.
		/// </summary>
		/// <param name="path">The asset path.</param>
		SE_FUNCTION(API())
		static void DeleteAsset(const StringView& path);

	public:
#if SE_EDITOR

		/// <summary>
		/// Renames the asset.
		/// </summary>
		/// <param name="oldPath">The old asset path.</param>
		/// <param name="newPath">The new asset path.</param>
		/// <returns>True if failed, otherwise false.</returns>
		SE_FUNCTION(API())
		static bool RenameAsset(const StringView& oldPath, const StringView& newPath);

		/// <summary>
		/// Performs the fast temporary asset clone to the temporary folder.
		/// </summary>
		/// <param name="path">The source path.</param>
		/// <param name="resultPath">The result path.</param>
		/// <returns>True if failed, otherwise false.</returns>
		static bool FastTmpAssetClone(const StringView& path, String& resultPath);

		/// <summary>
		/// Clones the asset file.
		/// </summary>
		/// <param name="dstPath">The destination path.</param>
		/// <param name="srcPath">The source path.</param>
		/// <param name="dstId">The destination id.</param>
		/// <returns>True if failed, otherwise false.</returns>
		static bool CloneAssetFile(const StringView& dstPath, const StringView& srcPath, const UID& dstId);

#endif

		/// <summary>
		/// Unloads the specified asset.
		/// </summary>
		/// <param name="asset">The asset.</param>
		SE_FUNCTION(API())
		static void UnloadAsset(Asset* asset);

		/// <summary>
		/// Creates temporary and virtual asset of the given type.
		/// </summary>
		/// <returns>Created asset or null if failed.</returns>
		template<typename T>
		FORCE_INLINE static T* CreateVirtualAsset()
		{
			return static_cast<T*>(CreateVirtualAsset(Typeof<T>()));
		}

		/// <summary>
		/// Creates temporary and virtual asset of the given type.
		/// </summary>
		/// <param name="type">The asset type klass.</param>
		/// <returns>Created asset or null if failed.</returns>
		static Asset* CreateVirtualAsset(const CLRClass* type);

		/// <summary>
		/// Creates temporary and virtual asset of the given type.
		/// </summary>
		/// <param name="type">The asset type.</param>
		/// <returns>Created asset or null if failed.</returns>
		static Asset* CreateVirtualAsset(const TypeID& type);

		/// <summary>
		/// Occurs when asset is being disposed and will be unloaded (by force). All references to it should be released.
		/// </summary>
		SE_EVENT(API())
		static Delegate<Asset*> AssetDisposing;

		/// <summary>
		/// Occurs when asset is being reloaded and will be unloaded (by force) to be loaded again (e.g. after reimport). Always called from the main thread.
		/// </summary>
		SE_EVENT(API())
		static Delegate<Asset*> AssetReloading;

	private:
		static void TryCallOnLoaded(Asset* asset);
		static void OnAssetLoaded(Asset* asset);
		static void OnAssetUnload(Asset* asset);
		static void OnAssetChangeId(Asset* asset, const UID& oldId, const UID& newId);

	private:
		static void DeleteFileSafety(const StringView& path, const UID& id);
	};
}
