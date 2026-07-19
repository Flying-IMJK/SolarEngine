
#include "SceneQuery.h"

#include "Runtime/Core/Profiler/ProfilerCPU.h"

namespace SE
{

	bool GetAllSceneObjectsQuery(Actor* actor, List<SceneObject*>& objects)
	{
		objects.Add(actor);
		// objects.Add(reinterpret_cast<SceneObject* const*>(actor->Scripts.Get()), actor->Scripts.Count());
		return true;
	}

	bool GetAllSerializableSceneObjectsQuery(Actor* actor, List<SceneObject*>& objects)
	{
		/*if (EnumHasAnyFlags(actor->HideFlags, HideFlags::DontSave))
			return false;*/
		objects.Add(actor);
		// objects.Add(reinterpret_cast<SceneObject* const*>(actor->Scripts.Get()), actor->Scripts.Count());
#if SE_EDITOR
		// Skip saving Missing Script instances
		/*for (int32 i = 0; i < actor->Scripts.Count(); i++)
		{
			const int32 idx = objects.Count() - i - 1;
			if (objects.Get()[idx]->GetTypeHandle() == MissingScript::TypeInitializer)
				objects.RemoveAtKeepOrder(idx);
		}*/
#endif
		return true;
	}

	bool GetAllActorsQuery(Actor* actor, List<Actor*>& actors)
	{
		actors.Add(actor);
		return true;
	}

	void SceneQuery::GetAllSceneObjects(Actor* root, List<SceneObject*>& objects)
	{
		ENGINE_ASSERT(root);
		PROFILE_CPU();
		Function<bool(Actor*, List<SceneObject*>&)> func(GetAllSceneObjectsQuery);
		root->TreeExecuteChildren<List<SceneObject*>&>(func, objects);
	}

	void SceneQuery::GetAllSerializableSceneObjects(Actor* root, List<SceneObject*>& objects)
	{
		ENGINE_ASSERT(root);
		PROFILE_CPU();
		Function<bool(Actor*, List<SceneObject*>&)> func(GetAllSerializableSceneObjectsQuery);
		root->TreeExecute<List<SceneObject*>&>(func, objects);
	}

	void SceneQuery::GetAllActors(Actor* root, List<Actor*>& actors)
	{
		PROFILE_CPU();
		ENGINE_ASSERT(root);
		Function<bool(Actor*, List<Actor*>&)> func(GetAllActorsQuery);
		root->TreeExecuteChildren<List<Actor*>&>(func, actors);
	}

	void SceneQuery::GetAllActors(List<Actor*>& actors)
	{
		PROFILE_CPU();
#if SCENE_QUERIES_WITH_LOCK
		ScopeLock lock(Level::ScenesLock);
#endif
		for (int32 i = 0; i < Level::Scenes.Count(); i++)
		{
			GetAllActors(Level::Scenes[i], actors);
		}
	}
} // SE