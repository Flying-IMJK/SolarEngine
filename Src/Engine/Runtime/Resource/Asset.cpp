
#include "Asset.h"
#include "AssetContent.h"
#include "AssetRef.h"

#include "Core/Thread/Threading.h"
#include "Core/Thread/Task.h"
#include "Core/Profiler/Profiler.h"
#include "Runtime/Resource/AssetsCache.h"
#include "Runtime/Resource/Loading/AssetTask.h"
#include "Runtime/Resource/Loading/Tasks/LoadAssetTask.h"
#include "Runtime/Resource/Loading/AssetLoading.h"
#include "Runtime/Engine.h"


namespace SE
{
	AssetRefBase::~AssetRefBase()
	{
		Asset* asset = _asset;
		if (asset)
		{
			_asset = nullptr;
			asset->OnLoadedEvent.Unbind<AssetRefBase, &AssetRefBase::OnLoaded>(this);
			asset->OnUnloadedEvent.Unbind<AssetRefBase, &AssetRefBase::OnUnloaded>(this);
			asset->RemoveReference();
		}
	}

	String AssetRefBase::ToString() const
	{
		return _asset ? _asset->ToString() : SE_TEXT("<null>");
	}

	void AssetRefBase::OnSet(Asset* asset)
	{
		auto temp = _asset;
		if (temp != asset)
		{
			if (temp)
			{
				temp->OnLoadedEvent.Unbind<AssetRefBase, &AssetRefBase::OnLoaded>(this);
				temp->OnUnloadedEvent.Unbind<AssetRefBase, &AssetRefBase::OnUnloaded>(this);
				temp->RemoveReference();
			}
			_asset = temp = asset;
			if (temp)
			{
				temp->AddReference();
				temp->OnLoadedEvent.Bind<AssetRefBase, &AssetRefBase::OnLoaded>(this);
				temp->OnUnloadedEvent.Bind<AssetRefBase, &AssetRefBase::OnUnloaded>(this);
			}
			Changed();
			if (temp && temp->IsLoaded())
				Loaded();
		}
	}

	void AssetRefBase::OnLoaded(Asset* asset)
	{
		if (_asset != asset)
			return;
		Loaded();
	}

	void AssetRefBase::OnUnloaded(Asset* asset)
	{
		if (_asset != asset)
			return;
		Unload();
		OnSet(nullptr);
	}

	SoftAssetRefBase::~SoftAssetRefBase()
	{
		Asset* asset = _asset;
		if (asset)
		{
			_asset = nullptr;
			asset->OnUnloadedEvent.Unbind<SoftAssetRefBase, &SoftAssetRefBase::OnUnloaded>(this);
			asset->RemoveReference();
		}
#if !BUILD_RELEASE
		_id = UID::Empty;
#endif
	}

	String SoftAssetRefBase::ToString() const
	{
		return _asset ? _asset->ToString() : (_id.IsValid() ? _id.ToString() : SE_TEXT("<null>"));
	}

	void SoftAssetRefBase::OnSet(Asset* asset)
	{
		if (_asset == asset)
			return;
		if (_asset)
		{
			_asset->OnUnloadedEvent.Unbind<SoftAssetRefBase, &SoftAssetRefBase::OnUnloaded>(this);
			_asset->RemoveReference();
		}
		_asset = asset;
		_id = asset ? asset->GetID() : UID::Empty;
		if (asset)
		{
			asset->AddReference();
			asset->OnUnloadedEvent.Bind<SoftAssetRefBase, &SoftAssetRefBase::OnUnloaded>(this);
		}
		Changed();
	}

	void SoftAssetRefBase::OnSet(const UID& id)
	{
		if (_id == id)
			return;
		if (_asset)
		{
			_asset->OnUnloadedEvent.Unbind<SoftAssetRefBase, &SoftAssetRefBase::OnUnloaded>(this);
			_asset->RemoveReference();
		}
		_asset = nullptr;
		_id = id;
		Changed();
	}

	void SoftAssetRefBase::OnUnloaded(Asset* asset)
	{
		if (_asset != asset)
			return;
		_asset->RemoveReference();
		_asset->OnUnloadedEvent.Unbind<SoftAssetRefBase, &SoftAssetRefBase::OnUnloaded>(this);
		_asset = nullptr;
		_id = UID::Empty;
		Changed();
	}

	WeakAssetRefBase::~WeakAssetRefBase()
	{
		Asset* asset = _asset;
		if (asset)
		{
			_asset = nullptr;
			asset->OnUnloadedEvent.Unbind<WeakAssetRefBase, &WeakAssetRefBase::OnUnloaded>(this);
		}
	}

	String WeakAssetRefBase::ToString() const
	{
		return _asset ? _asset->ToString() : SE_TEXT("<null>");
	}

	void WeakAssetRefBase::OnSet(Asset* asset)
	{
		auto e = _asset;
		if (e != asset)
		{
			if (e)
				e->OnUnloadedEvent.Unbind<WeakAssetRefBase, &WeakAssetRefBase::OnUnloaded>(this);
			_asset = e = asset;
			if (e)
				e->OnUnloadedEvent.Bind<WeakAssetRefBase, &WeakAssetRefBase::OnUnloaded>(this);
		}
	}

	void WeakAssetRefBase::OnUnloaded(Asset* asset)
	{
		if (_asset != asset)
			return;
		Unload();
		asset->OnUnloadedEvent.Unbind<WeakAssetRefBase, &WeakAssetRefBase::OnUnloaded>(this);
		_asset = nullptr;
	}


	Asset::Asset(const SpawnParams& params, const AssetInfo* info) : ManagedScriptingObject(params)
		, m_RefCount(0)
		, m_LoadState(0)
		, m_LoadingTask(0)
		, m_DeleteFileOnUnload(false)
		, m_IsVirtual(false)
	{
	}

	String AssetInfo::ToString() const
	{
		return String::Format(SE_TEXT("ID: {0}, TypeName: {1}, Path: \'{2}\'"), id, typeID.ToString(), path);
	}

	int32 Asset::GetReferencesCount() const
	{
		return (int32)Platform::AtomicRead(const_cast<int64 volatile*>(&m_RefCount));
	}

	void Asset::OnDeleteObject()
	{
		ENGINE_ASSERT(Threading::IsMainThread());

		// Send event to the gameplay so it can release handle to this asset
		// This may happen when asset gets removed but sth is still referencing it (eg. in managed code)
		if (!IsInternalType())
			AssetContent::AssetDisposing(this);

		const bool wasMarkedToDelete = m_DeleteFileOnUnload != 0;
#if SE_EDITOR
		const String path = wasMarkedToDelete ? GetPath() : String::Empty;
#endif
		const UID id = GetID();

		// Fire unload event (every object referencing this asset or it's data should release reference so later actions are safe)
		onUnload_MainThread();

		// Remove from pool
		AssetContent::OnAssetUnload(this);

		// Unload asset data (in a safe way to protect asset data)
		Locker.Lock();
		if (IsLoaded())
		{
			Unload(false);
			Platform::AtomicStore(&m_LoadState, (int64)LoadState::Unloaded);
		}
		Locker.Unlock();

#if BUILD_DEBUG
		// Ensure no object is referencing it (except managed instance if exists)
		//ENGINE_ASSERT(_refCount == (HasManagedInstance() ? 1 : 0));
#endif

		// Base (after it `this` is invalid)
//		ManagedScriptingObject::OnDeleteObject();

#if SE_EDITOR
		if (wasMarkedToDelete)
		{
			LOG_INFO("Resource", "Deleting asset '{0}':{1}.", path, id.ToString());

			// Remove from registry
			AssetContent::GetRegistry()->DeleteAsset(id, nullptr);

			// Delete file
			if (!IsVirtual())
				AssetContent::DeleteFileSafety(path, id);
		}
#endif
	}

	void Asset::ChangeID(const UID& newId)
	{
		// Only virtual asset can change ID
		if (!IsVirtual())
		{
			ENGINE_UNREACHABLE_CODE()
		}
//			CRASH;

		// ID has to be unique
		if (AssetContent::GetAsset(newId) != nullptr)
		{
			ENGINE_UNREACHABLE_CODE()
		}
//			CRASH;

		const UID oldId = m_Id;
//		ManagedScriptingObject::ChangeID(newId);
		AssetContent::OnAssetChangeId(this, oldId, newId);
	}

#if SE_EDITOR

	bool Asset::ShouldDeleteFileOnUnload() const
	{
		return m_DeleteFileOnUnload != 0;
	}

#endif

	uint64 Asset::GetMemoryUsage() const
	{
		uint64 result = sizeof(Asset);
		Locker.Lock();
		if (Platform::AtomicRead(&m_LoadingTask))
			result += sizeof(AssetTask);
		result += (OnLoadedEvent.Capacity() + OnReloadingEvent.Capacity() + OnUnloadedEvent.Capacity()) * sizeof(EventType::FunctionType);
		Locker.Unlock();
		return result;
	}

	void Asset::Reload()
	{
		// Virtual assets are memory-only so reloading them makes no sense
		if (IsVirtual())
			return;

		// It's better to call it from the main thread
		if (Threading::IsMainThread())
		{
			LOG_INFO("Resource", "Reloading asset {0}", ToString());

			WaitForLoaded();

			// Fire event
			if (!IsInternalType())
			{
				AssetContent::AssetReloading(this);
			}
			OnReloadingEvent(this);

			Threading::ScopeLock lock(Locker);

			if (IsLoaded())
			{
				// Unload current data
				Unload(true);
				Platform::AtomicStore(&m_LoadState, (int64)LoadState::Unloaded);
			}

			// Start reloading process
			StartLoadAsset();
		}
		else
		{
			Function<void()> action = CreateFunc<Asset, &Asset::Reload>(this);
			Threading::Task::StartNew(New<Threading::MainThreadActionTask>(action, this));
		}
	}

	bool Asset::WaitForLoaded(double timeoutInMilliseconds) const
	{
		// This function is used many time when some parts of the engine need to wait for asset loading end (it may fail but has to end).
		// But it cannot be just a simple active-wait loop.
		// Imagine following situation:
		// AssetContent Pool has 2 loading threads.
		// Both start to load layered materials which need to be recompiled (Material Generator work).
		// Every of these materials is made of a few layers.
		// To load child layer Material Generator is requesting AssetContent Pool to load it fully.
		// However this cannot be done because Pool has limited threads and all of them may requesting more loads to be done.
		//
		// To solve that problem we have different solutions:
		// 1) add more loading threads (bad idea)
		// 2) content loading could use thread pool to enqueue single tasks (still risky due to many tasks queued and limited Thread Pool size (need to build graph of dependencies?))
		// 3) content loading could detect asset load calls from content loader threads and load requested asset without stalls
		// 4) every asset could expose dependencies and content system could load required dependencies earlier
		//
		// 3) and 4) are good solutions. 4) would require more work but will be needed in future for building system to gather assets for game packages.
		// But WaitForLoaded could detect if is called from the Loading Thead and manually load dependent asset. It's fairly easy to do and will work out.

		// Early out if asset has been already loaded
		if (IsLoaded())
		{
			// If running on a main thread we can flush asset `Loaded` event
			if (Threading::IsMainThread())
			{
				AssetContent::TryCallOnLoaded((Asset*)this);
			}

			return true;
		}

		// Check if loading failed
		Platform::MemoryBarrier();
		if (LastLoadFailed())
			return false;

		// Check if has missing loading task
		Platform::MemoryBarrier();
		const auto loadingTask = (AssetTask*)Platform::AtomicRead(&m_LoadingTask);
		if (loadingTask == nullptr)
		{
			LOG_WARNING("Resource", "WaitForLoaded asset \'{0}\' failed. No loading task attached and asset is not loaded.", ToString());
			return false;
		}

		PROFILE_CPU();

		// Check if call is made from the Loading Thread and task has not been taken yet
		auto thread = AssetLoading::GetCurrentLoadThread();
		if (thread != nullptr)
		{
			// Note: to reproduce this case just include material into material (use layering).
			// So during loading first material it will wait for child materials loaded calling this function

			const double timeoutInSeconds = timeoutInMilliseconds * 0.001;
			const double startTime = Platform::GetTimeSeconds();
			Threading::Task* task = loadingTask;
			List<AssetTask *, InlinedAllocation<64>> localQueue;
#define CHECK_CONDITIONS() (!Engine::ShouldExit() && (timeoutInSeconds <= 0.0 || Platform::GetTimeSeconds() - startTime < timeoutInSeconds))
			do
			{
				// Try to execute content tasks
				while (task->IsState(Threading::TaskState::Queued) && CHECK_CONDITIONS())
				{
					// Dequeue task from the loading queue
					AssetTask* tmp;
					if (AssetLoading::GetAssetLoadTaskQueue().try_dequeue(tmp))
					{
						if (tmp == task)
						{
							if (localQueue.Count() != 0)
							{
								// Put back queued tasks
								AssetLoading::GetAssetLoadTaskQueue().enqueue_bulk(localQueue.Get(), localQueue.Count());
								localQueue.Clear();
							}

							thread->Execute(tmp);
						}
						else
						{
							localQueue.Add(tmp);
						}
					}
					else
					{
						// No task in queue but it's queued so other thread could have stolen it into own local queue
						break;
					}
				}
				if (localQueue.Count() != 0)
				{
					// Put back queued tasks
					AssetLoading::GetAssetLoadTaskQueue().enqueue_bulk(localQueue.Get(), localQueue.Count());
					localQueue.Clear();
				}

				// Check if task is done
				if (task->IsEnded())
				{
					// 如果没问题，则等待下一个任务
					if (task->IsState(Threading::TaskState::Finished))
					{
						task = task->GetContinueWithTask();
						if (!task)
						{
							break;
						}
					}
					else
					{
						// Failed or cancelled so this wait also fails
						break;
					}
				}
			} while (CHECK_CONDITIONS());
#undef CHECK_CONDITIONS
		}
		else
		{
			// Wait for task end
			loadingTask->Wait(timeoutInMilliseconds);
		}

		// If running on a main thread we can flush asset `Loaded` event
		if (Threading::IsMainThread() && IsLoaded())
		{
			AssetContent::TryCallOnLoaded((Asset*)this);
		}

		return IsLoaded();
	}

	void Asset::InitAsVirtual()
	{
		// Be a virtual thing
		m_IsVirtual = true;

		// Be a loaded thing
		Platform::AtomicStore(&m_LoadState, (int64)LoadState::Loaded);
	}

	void Asset::CancelStreaming()
	{
		// Cancel loading task but go over asset locker to prevent case if other load threads still loads asset while it's reimported on other thread
		Locker.Lock();
		auto loadTask = (AssetTask*)Platform::AtomicRead(&m_LoadingTask);
		Locker.Unlock();
		if (loadTask)
		{
			loadTask->Cancel();
		}
	}

#if SE_EDITOR

	void Asset::GetReferences(List<UID>& output) const
	{
		// No refs by default
	}

	List<UID> Asset::GetReferences() const
	{
		List<UID> result;
		GetReferences(result);
		return result;
	}

#endif

	void Asset::DeleteManaged()
	{
/*		if (HasManagedInstance())
		{
			if (IsRegistered())
				UnregisterObject();
			DestroyManaged();
		}*/
	}

	AssetTask* Asset::CreateLoadAssetTask()
	{
		// Spawn new task and cache pointer
		return New<LoadAssetTask>(this);
	}

	void Asset::StartLoadAsset()
	{
		ENGINE_ASSERT(!IsLoaded());
		ENGINE_ASSERT(Platform::AtomicRead(&m_LoadingTask) == 0);
		auto loadingTask = CreateLoadAssetTask();
		ENGINE_ASSERT(loadingTask != nullptr);
		Platform::AtomicStore(&m_LoadingTask, (intptr)loadingTask);
		loadingTask->Start();
	}

	void Asset::ReleaseStorage()
	{
	}

	bool Asset::IsInternalType() const
	{
		return false;
	}

	void Asset::OnLoaded()
	{
		if (Threading::IsMainThread())
		{
			onLoaded_MainThread();
		}
		else if (OnLoadedEvent.IsBinded())
		{
			Function<void()> action;
			action.Bind<Asset, &Asset::OnLoaded>(this);
			Threading::Task::StartNew(New<Threading::MainThreadActionTask>(action, this));
		}
	}

	void Asset::onLoaded_MainThread()
	{
		ENGINE_ASSERT(Threading::IsMainThread());

		// Send event
		OnLoadedEvent(this);
	}

	void Asset::onUnload_MainThread()
	{
		// Note: asset should not be locked now (Locker should be free) so other thread won't be locked.

		ENGINE_ASSERT(Threading::IsMainThread());

		// Send event
		OnUnloadedEvent(this);

		// Check if is during loading
		auto loadingTask = (AssetTask*)Platform::AtomicRead(&m_LoadingTask);
		if (loadingTask != nullptr)
		{
			// Cancel loading
			Platform::AtomicStore(&m_LoadingTask, 0);
			LOG_WARNING("Resource", "Cancel loading task for \'{0}\'", String("ToString()"));
			loadingTask->Cancel();
		}
	}

	String Asset::ToString() const
	{
		return String::Format(SE_TEXT("Type: {0}, ID: {1}, Path: {2}"), GetType().ToString(), GetID(), GetPath());
	}

} // SE