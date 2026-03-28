#include "LoadAssetTask.h"
#include "LoadAssetDataTask.h"

#include "Core/Profiler/ProfilerCPU.h"
#include "Core/TypeSystem/Types.h"
#include "Core/Types/Object.h"

#include "Runtime/Resource/AssetRef.h"
#include "Runtime/Resource/AssetContent.h"

namespace SE
{
	AssetTask::Result LoadAssetTask::Process()
	{
		PROFILE_CPU();

		// Keep valid ref to the asset
		AssetRef<::SE::Asset> ref = Asset.Get();
		if (ref == nullptr)
			return Result::MissingReferences;

		if (Platform::AtomicRead(&ref->m_LoadingTask) == 0)
		{
			// It may fail when task is cancelled and new one was created later (don't crash but just end with an error)
			return Result::AssetLoadError;
		}

		ref->Locker.Lock();

		// Load asset
		Asset::LoadResult result;
		{
			// PROFILE_CPU_ASSET(this);
			result = ref->ProcessLoadAsset();
		}
		const bool isLoaded = result == Asset::LoadResult::Ok;
		const bool failed = !isLoaded;
		Platform::AtomicStore(&ref->m_LoadState, (int64)(isLoaded ? Asset::LoadState::Loaded : Asset::LoadState::LoadFailed));
		if (failed)
		{
			LOG_ERROR("Resource", "Loading asset \'{0}\' result: {1}.", ToString(), Types::GetEnumString(result));
		}

		// Unlink task
		Platform::AtomicStore(&ref->m_LoadingTask, 0);
		ENGINE_ASSERT(failed || ref->IsLoaded() == isLoaded);

		ref->Locker.Unlock();

		// Send event
		if (isLoaded)
		{
			// Register event `OnLoaded` invoke on main thread
			// We don't want to fire it here because current thread is content loader.
			// This allows to reduce mutexes and locks (max one frame delay isn't hurting but provides more safety)
			AssetContent::OnAssetLoaded(ref.Get());
		}

		return !failed ? Result::Ok : Result::AssetLoadError;
	}

	bool LoadAssetTask::HasReference(void* obj) const
	{
		return (Object*)obj == Asset.Get();
	}

	String LoadAssetTask::ToString() const
	{
		return String::Format(SE_TEXT("Load Task ({0}) Content ({1})"), Types::GetEnumString(GetState()), Asset->GetPath());
	}

	void LoadAssetTask::OnFail()
	{
		auto asset = Asset.Get();
		if (asset)
		{
			Asset = nullptr;
			asset->Locker.Lock();
			if (Platform::AtomicRead(&asset->m_LoadingTask) == (intptr)this)
				Platform::AtomicStore(&asset->m_LoadingTask, 0);
			asset->Locker.Unlock();
		}

		// Base
		AssetTask::OnFail();
	}


	void LoadAssetTask::OnEnd()
	{
		auto asset = Asset.Get();
		if (asset)
		{
			Asset = nullptr;
			asset->Locker.Lock();
			if (Platform::AtomicRead(&asset->m_LoadingTask) == (intptr)this)
			{
				Platform::AtomicStore(&asset->m_LoadingTask, 0);
			}
			asset->Locker.Unlock();
			asset = nullptr;
		}

		// Base
		AssetTask::OnEnd();
	}

	LoadAssetTask::~LoadAssetTask()
	{
		auto asset = Asset.Get();
		if (asset)
		{
			asset->Locker.Lock();
			if (Platform::AtomicRead(&asset->m_LoadingTask) == (intptr)this)
			{
				Platform::AtomicStore(&asset->m_LoadState, (int64)Asset::LoadState::LoadFailed);
				Platform::AtomicStore(&asset->m_LoadingTask, 0);
				LOG_ERROR("Resource", "Loading asset \'{0}\' result: {1}.", ToString(), Types::GetEnumString<Result>(Result::TaskFailed));
			}
			asset->Locker.Unlock();
		}
	}


	bool LoadAssetDataTask::HasReference(void* obj) const
	{
		return obj == _asset;
	}

	AssetTask::Result LoadAssetDataTask::Process()
	{
		if (IsCancelRequested())
			return Result::Ok;PROFILE_CPU();

		AssetRef<BinaryAsset> ref = _asset.Get();
		if (ref == nullptr)
		{
			return Result::MissingReferences;
		}
#if SGE_PROFILER
		const StringView name(ref->GetPath());
#endif

		// Load chunks
		for (int32 i = 0; i < ASSET_FILE_DATA_CHUNKS; i++)
		{
			if (GET_CHUNK_FLAG(i) & _chunks)
			{
				const auto chunk = ref->GetChunk(i);
				if (chunk != nullptr)
				{
					if (IsCancelRequested())
						return Result::Ok;
#if SGE_PROFILER
					ZoneScoped;
					ZoneName(*name, name.Length());
#endif
					if (!ref->storage->LoadAssetChunk(chunk))
					{
						LOG_WARNING("Resource", "Cannot load asset \'{0}\' chunk {1}.", ref->ToString(), i);
						return Result::LoadDataError;
					}
				}
			}
		}

		return Result::Ok;
	}

	void LoadAssetDataTask::OnEnd()
	{
		_dataLock.Release();
		_asset = nullptr;

		// Base
		AssetTask::OnEnd();
	}
}
