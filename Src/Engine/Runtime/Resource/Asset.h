#pragma once

#include "Core/Types/Delegate.h"
#include "Core/Platform/CriticalSection.h"
#include "Core/Types/Object.h"

#include "Runtime/API.h"
#include "AssetInfo.h"
#include "Runtime/Scripting/ScriptingObject.h"

namespace SE
{
	class AssetTask;
	class LoadAssetTask;
	class AssetContent;

#define ASSET_HEADER(type)	\
	SCRIPTING_TYPE_NO_SPAWN(type);	\
	public:							\
	explicit type(const SpawnParams& params, const AssetInfo* info)

	/// <summary>
	/// Asset objects base class.
	/// </summary>
	SE_CLASS(Reflect, API, NoSpawn)
	class SE_API_RUNTIME Asset : public ManagedScriptingObject
	{
		SE_DEFINE_CLASS(Asset, ManagedScriptingObject);
		SCRIPTING_TYPE_NO_SPAWN(Asset);


		friend class AssetContent;
		friend class LoadAssetTask;
		friend class AssetContentSystem;
	public:
		/// <summary>
		/// The asset loading result.
		/// </summary>
		SE_ENUM(Reflect)
		enum class LoadResult
		{
			Ok, Failed, MissingDataChunk, CannotLoadData, CannotLoadStorage, CannotLoadInitData, InvalidData
		};

	protected:
		enum class LoadState : int32
		{
			Unloaded,
			Loaded,
			LoadFailed,
		};

		volatile int64 m_RefCount = 0;
		volatile int64 m_LoadState = 0;
		volatile intptr m_LoadingTask = 0;

		bool m_DeleteFileOnUnload = false; // Indicates that asset source file should be removed on asset unload
		bool m_IsVirtual = false; // Indicates that asset is pure virtual (generated or temporary, has no storage so won't be saved)
	public:
		/// <summary>
		/// Initializes a new instance of the <see cref="Asset"/> class.
		/// </summary>
		/// <param name="params">The object initialization parameters.</param>
		/// <param name="info">The asset object information.</param>
		explicit Asset(const SpawnParams& params, const AssetInfo* info);

	public:
		typedef Delegate<Asset*> EventType;

		/// <summary>
		/// Action called when asset gets loaded
		/// </summary>
		EventType OnLoadedEvent;

		/// <summary>
		/// Action called when asset start reloading (e.g. after reimport). Always called from the main thread.
		/// </summary>
		EventType OnReloadingEvent;

		/// <summary>
		/// Action called when asset gets unloaded
		/// </summary>
		EventType OnUnloadedEvent;

		/// <summary>
		/// 资源对象通用的互斥锁。应保护大多数资源功能以确保安全。
		/// </summary>
		CriticalSection Locker;

	public:
		/// <summary>
		/// Gets asset's reference count. Asset will be automatically unloaded when this reaches zero.
		/// </summary>
		SE_FUNCTION(API) int32 GetReferencesCount() const;

		/// <summary>
		/// Adds reference to that asset.
		/// </summary>
		FORCE_INLINE void AddReference()
		{
			Platform::AtomicIncrement(&m_RefCount);
		}

		/// <summary>
		/// Removes reference from that asset.
		/// </summary>
		FORCE_INLINE void RemoveReference()
		{
			Platform::AtomicDecrement(&m_RefCount);
		}

	public:
		/// <summary>
		/// Gets the path to the asset storage file. In Editor, it reflects the actual file, in cooked Game, it fakes the Editor path to be informative for developers.
		/// </summary>
		virtual const String& GetPath() const = 0;

		/// <summary>
		/// Returns true if asset is loaded, otherwise false.
		/// </summary>
		FORCE_INLINE bool IsLoaded() const
		{
			return Platform::AtomicRead(&m_LoadState) == (int64)LoadState::Loaded;
		}

		/// <summary>
		/// 上次资源加载失败，则返回 true，否则返回 false。
		/// </summary>
		bool LastLoadFailed() const
		{
			return Platform::AtomicRead(&m_LoadState) == (int64)LoadState::LoadFailed;
		}

		/// <summary>
		/// 确定此资源是否为虚拟资源（生成的或临时的，不会保存）。
		/// </summary>
		FORCE_INLINE bool IsVirtual() const
		{
			return m_IsVirtual != 0;
		}

#if SE_EDITOR
		/// <summary>
		/// Determines whether this asset was marked to be deleted on unload.
		/// </summary>
		bool ShouldDeleteFileOnUnload() const;
#endif

		/// <summary>
		/// Gets amount of CPU memory used by this resource (in bytes). It's a rough estimation. Memory may be fragmented, compressed or sub-allocated so the actual memory pressure from this resource may vary.
		/// </summary>
		virtual uint64 GetMemoryUsage() const;

	public:
		/// <summary>
		/// Reloads the asset.
		/// </summary>
		void Reload();

		/// <summary>
		/// Stops the current thread execution and waits until asset will be loaded (loading will fail, success or be cancelled).
		/// </summary>
		/// <param name="timeoutInMilliseconds">Custom timeout value in milliseconds.</param>
		/// <returns>True if cannot load that asset (failed or has been cancelled), otherwise false.</returns>
		bool WaitForLoaded(double timeoutInMilliseconds = 30000.0) const;

		/// <summary>
		/// Initializes asset data as virtual asset.
		/// </summary>
		virtual void InitAsVirtual();

		/// <summary>
		/// Cancels any asynchronous content streaming by this asset (eg. mesh data streaming into GPU memory). Will release any locks for asset storage container.
		/// </summary>
		virtual void CancelStreaming();

#if SE_EDITOR
		/// <summary>
		/// Gets the asset references. Supported only in Editor.
		/// </summary>
		/// <remarks>
		/// For some asset types (e.g. scene or prefab) it may contain invalid asset ids due to not perfect gather method,
		/// which is optimized to perform scan very quickly. Before using those ids perform simple validation via Content cache API.
		/// The result collection contains only 1-level-deep references (only direct ones) and is invalid if asset is not loaded.
		/// Also the output data may have duplicated asset ids or even invalid ids (Guid::Empty).
		/// </remarks>
		/// <param name="output">The output collection of the asset ids referenced by this asset.</param>
		virtual void GetReferences(List<UID, HeapAllocation>& output) const;
	
		/// <summary>
		/// Gets the asset references. Supported only in Editor.
		/// </summary>
		/// <remarks>
		/// For some asset types (e.g. scene or prefab) it may contain invalid asset ids due to not perfect gather method,
		/// which is optimized to perform scan very quickly. Before using those ids perform simple validation via Content cache API.
		/// The result collection contains only 1-level-deep references (only direct ones) and is invalid if asset is not loaded.
		/// Also the output data may have duplicated asset ids or even invalid ids (Guid::Empty).
		/// </remarks>
		/// <returns>The collection of the asset ids referenced by this asset.</returns>
		List<UID, HeapAllocation> GetReferences() const;
#endif

		/// <summary>
		/// Deletes the managed object.
		/// </summary>
		void DeleteManaged();

	protected:

		/// <summary>
		/// 开始加载资源
		/// </summary>
		virtual void StartLoadAsset();

		/// <summary>
		/// Releases the storage file/container handle to prevent issues when renaming or moving the asset.
		/// </summary>
		virtual void ReleaseStorage();

		/// <summary>
		/// 创建加载任务序列（允许将自定义任务加入到资源加载逻辑中）
		/// </summary>
		/// <returns>加载时调用的第一个任务</returns>
		virtual AssetTask* CreateLoadAssetTask();

		/// <summary>
		/// 加载资源
		/// </summary>
		/// <returns>Loading result</returns>
		virtual LoadResult ProcessLoadAsset() = 0;

		/// <summary>
		/// Unloads asset data
		/// </summary>
		/// <param name="isReloading">True if asset is reloading data, otherwise false.</param>
		virtual void Unload(bool isReloading) = 0;

		void OnLoaded();
		virtual void onLoaded_MainThread();
		virtual void onUnload_MainThread();

	protected:
		virtual bool IsInternalType() const;

#if SE_EDITOR
		virtual void onRename(const StringView& newPath) = 0;
#endif

	public:
		// [ManagedScriptingObject]
		String ToString() const override;
		virtual void OnDeleteObject() override;
/*		bool CreateManaged() override;
		void DestroyManaged() override;
		void OnManagedInstanceDeleted() override;
		void OnScriptingDispose() override;*/
		void ChangeID(const UID& newId) override;
	};

} // SE