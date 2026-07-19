
#include "SceneRendering.h"

#include "RenderList.h"
#include "Runtime/Core/Profiler/ProfilerCPU.h"
#include "Runtime/Core/Thread/JobSystem.h"
#include "Runtime/Core/Thread/Threading.h"
#include "Runtime/Level/Actor.h"

namespace SE
{
	ISceneRenderingListener::~ISceneRenderingListener()
	{
		for (SceneRendering* scene : _scenes)
		{
			scene->_listeners.Remove(this);
		}
	}

	void ISceneRenderingListener::ListenSceneRendering(SceneRendering* scene)
	{
		if (!_scenes.Contains(scene))
		{
			_scenes.Add(scene);
			scene->_listeners.Add(this);
		}
	}


	void SceneRendering::Draw(RenderContextBatch& renderContextBatch, DrawCategory category)
	{
		Threading::ScopeLock lock(Locker);
		if (category == PreRender)
		{
			// Register scene
			for (const auto& renderContext : renderContextBatch.Contexts)
				renderContext.list->Scenes.Add(this);

			// Add additional lock during scene rendering (prevents any Actors cache modifications on content streaming threads - eg. when model residency changes)
			Locker.Lock();
		}
		else if (category == PostRender)
		{
			// Release additional lock
			Locker.Unlock();
		}

		auto& view = renderContextBatch.GetMainContext().view;
		auto& list = Actors[(int32)category];
		m_DrawListData = list.Get();
		m_DrawListSize = list.Count();
		m_DrawBatch = &renderContextBatch;

		// Setup frustum data
		const int32 frustumsCount = renderContextBatch.Contexts.Count();
		m_DrawFrustumsData.Resize(frustumsCount);
		for (int32 i = 0; i < frustumsCount; i++)
			m_DrawFrustumsData.Get()[i] = renderContextBatch.Contexts.Get()[i].view.cullingFrustum;

		// Draw all visual components
		m_DrawListIndex = -1;
		if (m_DrawListSize >= 64 && category == SceneDrawAsync && renderContextBatch.EnableAsync)
		{
			// Run in async via Job System
			Function<void(int32)> func = CreateFunc<SceneRendering, &SceneRendering::DrawActorsJob>(this);
			const uint64 waitLabel = Threading::JobSystem::Dispatch(func, Threading::JobSystem::GetThreadsCount());
			renderContextBatch.WaitLabels.Add(waitLabel);
		}
		else
		{
			// Scene is small so draw on a main-thread
			DrawActorsJob(0);
		}

#if SE_EDITOR
		if (view.Pass.IsFlag(DrawPass::GBuffer) && category == SceneDraw)
		{
			// Draw physics shapes
			/*if (EnumHasAnyFlags(view.Flags, ViewFlags::PhysicsDebug) || view.Mode == ViewMode::PhysicsColliders)
			{
				const PhysicsDebugCallback* physicsDebugData = PhysicsDebug.Get();
				for (int32 i = 0; i < PhysicsDebug.Count(); i++)
				{
					physicsDebugData[i](view);
				}
			}*/

			// Draw light shapes
			/*if (EnumHasAnyFlags(view.Flags, ViewFlags::LightsDebug))
			{
				const LightsDebugCallback* lightsDebugData = LightsDebug.Get();
				for (int32 i = 0; i < LightsDebug.Count(); i++)
				{
					lightsDebugData[i](view);
				}
			}*/
		}
#endif
	}

	void SceneRendering::Clear()
	{
		Threading::ScopeLock lock(Locker);
		for (auto* listener : _listeners)
		{
			listener->OnSceneRenderingClear(this);
			listener->_scenes.Remove(this);
		}
		_listeners.Clear();
		for (auto& e : Actors)
		{
			e.Clear();
		}
	}

	void SceneRendering::AddRender(IRender* render, int32& key)
	{
		if (key != -1)
			return;
		const int32 category = render->RenderGetDrawCategory();
		Threading::ScopeLock lock(Locker);
		auto& list = Actors[category];
		// TODO: track removedCount and skip searching for free entry if there is none
		key = 0;
		for (; key < list.Count(); key++)
		{
			if (list.Get()[key].render == nullptr)
				break;
		}
		if (key == list.Count())
			list.AddOne();
		auto& e = list[key];
		e.render = render;
		e.LayerMask = render->RenderGetLayerMask();
		e.Bounds = render->RenderGetSphere();
		// e.NoCulling = a->_drawNoCulling;
		for (auto* listener : _listeners)
			listener->OnSceneRenderingAddActor(render);
	}

	void SceneRendering::UpdateRender(IRender* render, int32& key)
	{
		const int32 category = render->RenderGetDrawCategory();
		Threading::ScopeLock lock(Locker);
		auto& list = Actors[category];
		if (list.Count() <= key) // Ignore invalid key softly
			return;
		auto& e = list[key];
		if (e.render == render)
		{
			for (auto* listener : _listeners)
			{
				listener->OnSceneRenderingUpdateActor(render, e.Bounds);
			}
			e.LayerMask = render->RenderGetLayerMask();
			e.Bounds = render->RenderGetSphere();
		}
	}

	void SceneRendering::RemoveRender(IRender* render, int32& key)
	{
		const int32 category = render->RenderGetDrawCategory();
		Threading::ScopeLock lock(Locker);
		auto& list = Actors[category];
		if (list.Count() > key) // Ignore invalid key softly (eg. list after batch clear during scene unload)
		{
			auto& e = list.Get()[key];
			if (e.render == render)
			{
				for (auto* listener : _listeners)
				{
					listener->OnSceneRenderingRemoveActor(render);
				}
				e.render = nullptr;
				e.LayerMask = 0;
			}
		}
		key = -1;
	}

	FORCE_INLINE bool FrustumsListCull(const BoundingSphere& bounds, const List<BoundingFrustum>& frustums)
	{
		const int32 count = frustums.Count();
		const BoundingFrustum* data = frustums.Get();
		for (int32 i = 0; i < count; i++)
		{
			if (data[i].Intersects(bounds))
				return true;
		}
		return false;
	}

#define FOR_EACH_BATCH_ACTOR const int64 count = m_DrawListSize; while (true) { const int64 index = Platform::AtomicIncrement(&m_DrawListIndex); if (index >= count) break; auto e = m_DrawListData[index];
#define CHECK_ACTOR (/*(view.RenderLayersMask.Mask & e.LayerMask) && */(e.NoCulling || FrustumsListCull(e.Bounds, m_DrawFrustumsData)))
#define CHECK_ACTOR_SINGLE_FRUSTUM (/*(view.RenderLayersMask.Mask & e.LayerMask) && */(e.NoCulling || view.cullingFrustum.Intersects(e.Bounds)))

#if SCENE_RENDERING_USE_PROFILER_PER_ACTOR
#define DRAW_ACTOR(mode) PROFILE_CPU_ACTOR(e.Actor); e.Actor->Draw(mode)
#else
#define DRAW_ACTOR(mode) e.Actor->Draw(mode)
#endif

	void SceneRendering::DrawActorsJob(int32)
	{
		PROFILE_CPU();
		auto& mainContext = m_DrawBatch->GetMainContext();
		const auto& view = mainContext.view;
		if (view.IsOfflinePass)
		{
			// Offline pass with additional static flags culling
			const int64 count = m_DrawListSize;
			while (true)
			{
				const int64 index = Platform::AtomicIncrement(&m_DrawListIndex);
				if (index >= count)
				{
					break;
				}
				auto e = m_DrawListData[index];
				e.Bounds.Center -= view.Origin;
				if (CHECK_ACTOR && !view.StaticFlagsMask.Is(StaticMask::None) && e.render->RenderGetStaticFlags() == view.StaticFlagsMask)
				{
					e.render->RenderDraw(*m_DrawBatch);
				}
			}
		}
		else if (view.Origin.IsZero() && m_DrawFrustumsData.Count() == 1)
		{
			// Fast path for no origin shifting with a single context
			const int64 count = m_DrawListSize;
			while (true)
			{
				const int64 index = Platform::AtomicIncrement(&m_DrawListIndex);
				if (index >= count)
				{
					break;
				}
				auto e = m_DrawListData[index];
				if (CHECK_ACTOR_SINGLE_FRUSTUM)
				{
					e.render->RenderDraw(mainContext);
				}
			}
		}
		else if (view.Origin.IsZero())
		{
			// Fast path for no origin shifting
			const int64 count = m_DrawListSize;
			while (true)
			{
				const int64 index = Platform::AtomicIncrement(&m_DrawListIndex);
				if (index >= count)
				{
					break;
				}
				auto e = m_DrawListData[index];
				if (CHECK_ACTOR)
				{
					e.render->RenderDraw(*m_DrawBatch);
				}
			}
		}
		else
		{
			const int64 count = m_DrawListSize;
			while (true)
			{
				const int64 index = Platform::AtomicIncrement(&m_DrawListIndex);
				if (index >= count)
				{
					break;
				}
				auto e = m_DrawListData[index];
				e.Bounds.Center -= view.Origin;
				if (CHECK_ACTOR)
				{
					e.render->RenderDraw(*m_DrawBatch);
				}
			}
		}
	}
} // SE