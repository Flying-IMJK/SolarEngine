
#pragma once

#include "AssetInfo.h"
#include "AssetConfig.h"
#include "Core/Types/UID.h"
#if ENABLE_ASSETS_DISCOVERY
#include "Core/Types/DateTime.h"
#endif
#include "Core/Types/Strings/String.h"
#include "Core/Types/Collections/Dictionary.h"
#include "Core/Platform/CriticalSection.h"

namespace SE
{
	struct AssetHeader;
	struct StorageReference;
	class Storage;

	/// <summary>
	/// Assets cache flags.
	/// </summary>
	enum class AssetsCacheFlags
	{
		/// <summary>
		/// The none.
		/// </summary>
		None = 0,

		/// <summary>
		/// The serialized paths are relative to the startup folder (should be converted to absolute on load).
		/// </summary>
		RelativePaths = 1,
	};

	/// <summary>
	/// 引擎资产缓存
	/// </summary>
	class SE_API_RUNTIME AssetsCache
	{
	public:
		/// <summary>
		/// 资源缓存数据
		/// </summary>
		struct Entry
		{
			/// <summary>
			/// 缓存的资源信息
			/// </summary>
			AssetInfo Info;

#if ENABLE_ASSETS_DISCOVERY
			/// <summary>
			/// 文件修改日期
			/// </summary>
			DateTime FileModified;
#endif
			Entry() {}

			Entry(const UID& id, const TypeID& typeName, const StringView& path)
				: Info(id, typeName, path)
#if ENABLE_ASSETS_DISCOVERY
			, FileModified(DateTime::NowUTC())
#endif
			{
			}
		};

		typedef Dictionary<UID, Entry> RegistryDict;
		typedef Dictionary<String, UID> PathsMappingDict;

	private:
		bool m_IsDirty = false;
		CriticalSection m_Locker;
		RegistryDict m_Registry;
		PathsMappingDict m_PathsMapping;
		String m_Path;
	public:
		/// <summary>
		/// 获取已注册资源的数量
		/// </summary>
		int32 Size() const;

		void Init();

		/// <summary>
		/// Save registry
		/// </summary>
		/// <returns>True if cannot save registry</returns>
		bool Save();

		/// <summary>
		/// Saves the registry to the given file.
		/// </summary>
		/// <param name="outputPath">The output file path.</param>
		/// <param name="entries">The registry entries.</param>
		/// <param name="pathsMapping">The assets paths mapping table.</param>
		/// <param name="flags">The custom flags.</param>
		/// <returns>True if failed, otherwise false.</returns>
		static bool Save(const StringView& outputPath, const RegistryDict& entries, const PathsMappingDict& pathsMapping, const AssetsCacheFlags flags = AssetsCacheFlags::None);

	public:
		/// <summary>
		/// 按 ID 查找资源路径。在编辑器中，它返回实际的资源路径，在运行时，它返回映射的资源路径。
		/// </summary>
		/// <param name="id">The asset id.</param>
		/// <returns>The asset path, or empty if failed to find.</returns>
		const String& GetEditorAssetPath(const UID& id) const;

		/// <summary>
		/// 按路径查找资源信息
		/// </summary>
		/// <param name="path">The asset path.</param>
		/// <param name="info">The output asset info. Filled with valid values if method returns true.</param>
		/// <returns>True if found any asset, otherwise false.</returns>
		bool FindAsset(const StringView& path, AssetInfo& info);

		/// <summary>
		/// 按 ID 查找资源信息。
		/// </summary>
		/// <param name="id">The asset id.</param>
		/// <param name="info">The output asset info. Filled with valid values if method returns true.</param>
		/// <returns>True if found any asset, otherwise false.</returns>
		bool FindAsset(const UID& id, AssetInfo& info);

		/// <summary>
		/// Checks if asset with given path is in registry.
		/// </summary>
		/// <param name="path">The asset path.</param>
		/// <returns>True if asset is in cache, otherwise false.</returns>
		bool HasAsset(const StringView& path)
		{
			AssetInfo info;
			return FindAsset(path, info);
		}

		/// <summary>
		/// Checks if asset with given ID is in registry.
		/// </summary>
		/// <param name="id">The asset id.</param>
		/// <returns>True if asset is in cache, otherwise false.</returns>
		bool HasAsset(const UID& id)
		{
			AssetInfo info;
			return FindAsset(id, info);
		}

		/// <summary>
		/// Gets the asset ids.
		/// </summary>
		/// <param name="result">The result array.</param>
		void GetAll(List<UID, HeapAllocation>& result) const;

		/// <summary>
		/// Gets the asset ids that match the given typename.
		/// </summary>
		/// <param name="typeID">The asset typeid.</param>
		/// <param name="result">The result array.</param>
		void GetAllByTypeName(const TypeID typeID, List<UID, HeapAllocation>& result) const;

		/// <summary>
		/// Register assets in the cache
		/// </summary>
		/// <param name="storage">Flax assets container reference</param>
		void RegisterAssets(const StorageReference& storage);

		/// <summary>
		/// Register assets in the cache
		/// </summary>
		/// <param name="storage">Flax assets container</param>
		void RegisterAssets(Storage* storage);

		/// <summary>
		/// Register asset in the cache
		/// </summary>
		/// <param name="header">Flax asset file header</param>
		/// <param name="path">Asset path</param>
		void RegisterAsset(const AssetHeader& header, const StringView& path);

		/// <summary>
		/// Register asset in the cache
		/// </summary>
		/// <param name="id">Asset ID</param>
		/// <param name="typeName">Asset type id</param>
		/// <param name="path">Asset path</param>
		void RegisterAsset(const UID& id, const TypeID& typeName, const StringView& path);

		/// <summary>
		/// Delete asset
		/// </summary>
		/// <param name="path">Asset path</param>
		/// <param name="info">Output asset info</param>
		/// <returns>True if asset has been deleted, otherwise false</returns>
		bool DeleteAsset(const StringView& path, AssetInfo* info);

		/// <summary>
		/// Delete asset
		/// </summary>
		/// <param name="id">Asset ID</param>
		/// <param name="info">Output asset info</param>
		/// <returns>True if asset has been deleted, otherwise false</returns>
		bool DeleteAsset(const UID& id, AssetInfo* info);

		/// <summary>
		/// Rename asset
		/// </summary>
		/// <param name="oldPath">Old path</param>
		/// <param name="newPath">New path</param>
		/// <returns>True if has been deleted, otherwise false</returns>
		bool RenameAsset(const StringView& oldPath, const StringView& newPath);

		/// <summary>
		/// Determines whether cached asset entry is valid.
		/// </summary>
		/// <param name="e">The asset entry.</param>
		/// <returns>True if is valid, otherwise false.</returns>
		bool IsEntryValid(Entry& e);
	};
}
