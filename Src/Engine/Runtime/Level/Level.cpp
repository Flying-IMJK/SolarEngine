

#include "Level.h"

#include "ActorsCache.h"
#include "SceneObjectsFactory.h"
#include "SceneQuery.h"
#include "Core/Systems.h"
#include "Core/Logging/Exceptions/ArgumentException.h"
#include "Core/Logging/Exceptions/ArgumentNullException.h"
#include "Core/Logging/Exceptions/InvalidOperationException.h"
#include "Core/Logging/Exceptions/JsonParseException.h"
#include "Core/Profiler/ProfilerCPU.h"
#include "Core/Serialization/JsonWriters.hpp"
#include "Core/Serialization/JsonTools.h"
#include "Core/Thread/Threading.h"
#include "Core/Types/Stopwatch.h"
#include "Core/TypeSystem/Types.h"
#include "Core/Platform/File.h"
#include "Core/Thread/JobSystem.h"
#include "Runtime/Resource/AssetContent.h"
#include "Runtime/Resource/AssetRef.h"
#include "Runtime/Resource/Jsonasset.h"
#include "Scene/SceneAsset.h"

namespace SE
{
	enum class SceneEventType
	{
		OnSceneSaving = 0,
		OnSceneSaved = 1,
		OnSceneSaveError = 2,
		OnSceneLoading = 3,
		OnSceneLoaded = 4,
		OnSceneLoadError = 5,
		OnSceneUnloading = 6,
		OnSceneUnloaded = 7,
	};

	class SceneAction
	{
	public:
		virtual ~SceneAction()
		{
		}

		virtual bool CanDo() const
		{
			return true;
		}

		virtual bool Do() const
		{
			return true;
		}
	};

	struct LevelSystemData
	{
		CriticalSection sceneActionsLocker;
		List<SceneAction*> sceneActions;
		CriticalSection ScenesLock;
		DateTime lastSceneLoadTime;
	} *systemData;

	class LevelSystem final : public ISystem
	{
		ENGINE_SYSTEM(LevelSystem)
	public:
		LevelSystem() : ISystem(SE_TEXT("Scene Manager"), 200)
		{
		}

	protected:
		bool OnInit() override
		{
			systemData = New<LevelSystemData>();
			return true;
		}

		void OnUpdate() override
		{

		}

		void OnLateUpdate() override
		{
			Threading::ScopeLock lock(systemData->sceneActionsLocker);

			while (systemData->sceneActions.HasItems() && systemData->sceneActions.First()->CanDo())
			{
				const auto action = systemData->sceneActions.Dequeue();
				if (action != nullptr)
				{
					action->Do();
					Delete(action);
				}
			}
		}

		// void OnFixedUpdate() override;
		// void OnLateFixedUpdate() override;
		void OnDispose() override
		{
			Delete(systemData);
		}

	public:
		static void CallSceneEvent(SceneEventType eventType, Scene* scene, UID sceneId);

		static bool SpawnActor(Actor* actor, Actor* parent);
		static bool DeleteActor(Actor* actor);
		static bool UnloadScene(Scene* scene);
		static bool UnloadScenes();
		static bool SaveScene(Scene* scene, SerializeStream& writer);
		static bool SaveScene(Scene* scene, Json::StringBuffer& outBuffer, bool prettyJson);
		static bool SaveScene(Scene* scene, String path);
		static bool LoadScene(JsonAsset* sceneAsset);
		static bool LoadScene(const BytesContainer& sceneData, Scene** outScene = nullptr);
		static bool LoadScene(Json::Document& document, Scene** outScene = nullptr);
		static bool LoadScene(Json::Value& data, int32 engineBuild, Scene** outScene = nullptr);
	};

	ENGINE_SYSTEM_REGISTER(LevelSystem)

	void LevelSystem::CallSceneEvent(SceneEventType eventType, Scene* scene, UID sceneId)
	{
		PROFILE_CPU_NAMED("Level::CallSceneEvent");

		// Call event
		switch (eventType)
		{
		case SceneEventType::OnSceneSaving:
			Level::SceneSaving(scene, sceneId);
			break;
		case SceneEventType::OnSceneSaved:
			Level::SceneSaved(scene, sceneId);
			break;
		case SceneEventType::OnSceneSaveError:
			Level::SceneSaveError(scene, sceneId);
			break;
		case SceneEventType::OnSceneLoading:
			Level::SceneLoading(scene, sceneId);
			break;
		case SceneEventType::OnSceneLoaded:
			Level::SceneLoaded(scene, sceneId);
			break;
		case SceneEventType::OnSceneLoadError:
			Level::SceneLoadError(scene, sceneId);
			break;
		case SceneEventType::OnSceneUnloading:
			Level::SceneUnloading(scene, sceneId);
			break;
		case SceneEventType::OnSceneUnloaded:
			Level::SceneUnloaded(scene, sceneId);
			break;
		}
	}

	bool LevelSystem::SpawnActor(Actor* actor, Actor* parent)
	{
		if (actor == nullptr)
		{
			Log::ArgumentNullException(SE_TEXT("Cannot spawn null actor."));
			return false;
		}

		if (actor->GetTypeInfo()->IsAbstractType())
		{
			Log::Exception(SE_TEXT("Cannot spawn abstract actor type."));
			return false;
		}

		Scene* scene = TypeTryCast<Scene>(actor);
		if (scene != nullptr)
		{
			// Spawn scene
			actor->InitializeHierarchy();
			actor->OnTransformChanged();
			{
				SceneBeginData beginData;
				actor->BeginPlay(&beginData);
				beginData.OnDone();
			}
			CallSceneEvent(SceneEventType::OnSceneLoaded, scene, actor->GetInstanceID());
		}
		else
		{
			// Spawn actor
			if (Level::Scenes.IsEmpty())
			{
				Log::InvalidOperationException(SE_TEXT("Cannot spawn actor. No scene loaded."));
				return false;
			}
			if (parent == nullptr)
				parent = Level::Scenes[0];

			// actor->SetPhysicsScene(parent->GetPhysicsScene());
			actor->SetParent(parent, true, true);
		}

		return true;
	}

	bool LevelSystem::DeleteActor(Actor* actor)
	{
		if (actor == nullptr)
		{
			Log::ArgumentNullException(SE_TEXT("Cannot delete null actor."));
			return true;
		}

		actor->DeleteObject();

		return false;
	}

	bool LevelSystem::UnloadScene(Scene* scene)
	{
		if (scene == nullptr)
		{
			Log::ArgumentNullException();
			return false;
		}
		const auto sceneId = scene->GetInstanceID();

		PROFILE_CPU_NAMED("Level.UnloadScene");

		// Fire event
		CallSceneEvent(SceneEventType::OnSceneUnloading, scene, sceneId);

		// Call end play
		/*if (scene->IsDuringPlay())
			scene->EndPlay();*/

		// Remove from scenes list
		Level::Scenes.Remove(scene);

		// Fire event
		CallSceneEvent(SceneEventType::OnSceneUnloaded, scene, sceneId);

		// Simple enqueue scene root object to be deleted
		scene->DeleteObject();

		// Force flush deleted objects so we actually delete unloaded scene objects (prevent from reloading their managed objects, etc.)
		// ObjectsRemovalService::Flush();

		return true;
	}

	bool LevelSystem::UnloadScenes()
	{
		auto scenes = Level::Scenes;
		for (int32 i = scenes.Count() - 1; i >= 0; i--)
		{
			if (!UnloadScene(scenes[i]))
				return false;
		}
		return true;
	}

	bool LevelSystem::SaveScene(Scene* scene, SerializeStream& writer)
	{
		ENGINE_ASSERT(scene);
		const auto sceneId = scene->GetInstanceID();

		// Fire event
		CallSceneEvent(SceneEventType::OnSceneSaving, scene, sceneId);

		// Get all objects in the scene
		List<SceneObject*> allObjects;
		SceneQuery::GetAllSerializableSceneObjects(scene, allObjects);

		// Serialize to json
		writer.StartObject();
		{
			PROFILE_CPU_NAMED("Serialize");

			// Json resource header
			writer.JKEY("ID");
			writer.UUID(sceneId);
			writer.JKEY("TypeID");
			writer.Uint(Typeof<SceneAsset>());
			/*writer.JKEY("EngineBuild");
			writer.Int(VERSION_BUILD);*/

			// Json resource data
			writer.JKEY("Data");
			writer.StartArray();
			SceneObject** objects = allObjects.Get();
			SerializeContext serializeContext(writer);
			for (int32 i = 0; i < allObjects.Count(); i++)
			{
				Serialization::Serialize(serializeContext, objects[i]);
			}
			writer.EndArray();
		}
		writer.EndObject();

		return true;
	}

	bool LevelSystem::SaveScene(Scene* scene, Json::StringBuffer& outBuffer, bool prettyJson)
	{
		PROFILE_CPU_NAMED("Level.SaveScene");
		if (prettyJson)
		{
			PrettyJsonWriter writerObj(outBuffer);
			return SaveScene(scene, writerObj);
		}
		else
		{
			CompactJsonWriter writerObj(outBuffer);
			return SaveScene(scene, writerObj);
		}
	}

	bool LevelSystem::SaveScene(Scene* scene, String path)
	{
		auto sceneId = scene->GetInstanceID();

		LOG_INFO("Level", "Saving scene {0} to \'{1}\'", scene->GetName(), path);
		Stopwatch stopwatch;

		// Serialize to json
		Json::StringBuffer buffer;
		if (!SaveScene(scene, buffer, true) && buffer.GetSize() > 0)
		{
			CallSceneEvent(SceneEventType::OnSceneSaveError, scene, sceneId);
			return false;
		}

		// Save json to file
		if (!File::WriteAllBytes(path, (byte*)buffer.GetString(), (int32)buffer.GetSize()))
		{
			LOG_ERROR("Level", "Cannot save scene file");
			CallSceneEvent(SceneEventType::OnSceneSaveError, scene, sceneId);
			return true;
		}

		stopwatch.Stop();
		LOG_INFO("Level", "Scene saved! Time {0}ms", stopwatch.GetMilliseconds());

#if SE_EDITOR
		// Reload asset at the target location if is loaded
		Asset* asset = AssetContent::GetAsset(sceneId);
		if (!asset)
		{
			asset = AssetContent::GetAsset(path);
		}

		if (asset)
		{
			asset->Reload();
		}
#endif

		// Fire event
		CallSceneEvent(SceneEventType::OnSceneSaved, scene, sceneId);

		return true;
	}

	bool LevelSystem::LoadScene(JsonAsset* sceneAsset)
	{
		if (sceneAsset == nullptr || !sceneAsset->WaitForLoaded())
		{
			LOG_ERROR("Level", "Cannot load scene asset.");
			return false;
		}

		return LoadScene(*sceneAsset->Data, sceneAsset->DataEngineBuild);
	}

	bool LevelSystem::LoadScene(const BytesContainer& sceneData, Scene** outScene)
	{
		if (sceneData.IsInvalid())
		{
			LOG_ERROR("Level","Missing scene data.");
			return true;
		}

		// Parse scene JSON file
		Json::Document document;
		{
			PROFILE_CPU_NAMED("Json.Parse");
			document.Parse(sceneData.Get<char>(), sceneData.Length());
		}
		if (document.HasParseError())
		{
			Log::JsonParseException(document.GetParseError(), document.GetErrorOffset());
			return true;
		}

		Threading::ScopeLock lock(systemData->ScenesLock);
		return LoadScene(document, outScene);
	}

	bool LevelSystem::LoadScene(Json::Document& document, Scene** outScene)
	{
		auto data = document.FindMember("Data");
		if (data == document.MemberEnd())
		{
			LOG_ERROR("Level", "Missing Data member.");
			return true;
		}
		const int32 saveEngineBuild = JsonTools::GetInt(document, "EngineBuild", 0);
		return LoadScene(data->value, saveEngineBuild, outScene);
	}

	bool LevelSystem::LoadScene(Json::Value& data, int32 engineBuild, Scene** outScene)
	{
		PROFILE_CPU_NAMED("Level.LoadScene");
		if (outScene)
			*outScene = nullptr;
		LOG_INFO("Level", "Loading scene...");
		Stopwatch stopwatch;
		systemData->lastSceneLoadTime = DateTime::Now();

		// Here whole scripting backend should be loaded for current project
		// Later scripts will setup attached scripts and restore initial vars
		/*if (!Scripting::HasGameModulesLoaded())
		{
			LOG_ERROR("Level", "Cannot load scene without game modules loaded.");
#if SE_EDITOR
			if (!CommandLine::Options.Headless.IsTrue())
			{
				if (ScriptsBuilder::LastCompilationFailed())
					MessageBox::Show(TEXT("Scripts compilation failed. Cannot load scene without game script modules. Please fix the compilation issues. See logs for more info."), TEXT("Failed to compile scripts"), MessageBoxButtons::OK, MessageBoxIcon::Error);
				else
					MessageBox::Show(TEXT("Failed to load scripts. Cannot load scene without game script modules. See logs for more info."), TEXT("Missing game modules"), MessageBoxButtons::OK, MessageBoxIcon::Error);
			}
#endif
			return false;
		}*/

		// Peek meta
		/*if (engineBuild < 6000)
		{
			LOG_ERROR("Level", "Invalid serialized engine build.");
			return false;
		}*/
		if (!data.IsArray())
		{
			LOG_ERROR("Level", "Invalid Data member.");
			return false;
		}

		// Peek scene node value (it's the first actor serialized)
		auto sceneId = JsonTools::GetGuid(data[0], "ID");
		if (!sceneId.IsValid())
		{
			LOG_ERROR("Level", "Invalid scene id.");
			return false;
		}
		auto modifier = SerializeModifierCache::Modifier.Get();
		modifier->EngineBuild = engineBuild;

		// Skip is that scene is already loaded
		if (Level::FindScene(sceneId) != nullptr)
		{
			LOG_INFO("Level", "Scene {0} is already loaded.", sceneId);
			return true;
		}

		// Create scene actor
		// Note: the first object in the scene file data is a Scene Actor
		auto scene = New<Scene>(/*sceneId*/);
		// scene->RegisterObject();
		DeserializeContext deserializeContext(data[0], modifier.Value);
		scene->Deserialize(deserializeContext);

		// Fire event
		CallSceneEvent(SceneEventType::OnSceneLoading, scene, sceneId);

		// Loaded scene objects list
		CollectionPoolCache<ActorsCache::SceneObjectsListType>::ScopeCache sceneObjects = ActorsCache::SceneObjectsListCache.Get();
		const int32 dataCount = (int32)data.Size();
		sceneObjects->Resize(dataCount);
		sceneObjects->At(0) = scene;

		// Spawn all scene objects
		SceneObjectsFactory::Context context(modifier.Value);
		context.Async = Threading::JobSystem::GetThreadsCount() > 1 && dataCount > 10;
		{
			PROFILE_CPU_NAMED("Spawn");
			SceneObject** objects = sceneObjects->Get();
			if (context.Async)
			{
				systemData->ScenesLock.Unlock(); // Unlock scenes from Main Thread so Job Threads can use it to safely setup actors hierarchy (see Actor::Deserialize)
				Threading::JobSystem::Execute([&](int32 i)
				{
					i++; // Start from 1. at index [0] was scene
					auto& stream = data[i];
					auto obj = SceneObjectsFactory::Spawn(context, stream);
					objects[i] = obj;
					if (obj)
					{
						// obj->RegisterObject();
	#if SE_EDITOR
						// Auto-create C# objects for all actors in Editor during scene load when running in async (so main thread already has all of them)
						// obj->CreateManaged();
	#endif
					}
					else
						SceneObjectsFactory::HandleObjectDeserializationError(stream);
				}, dataCount - 1);
				systemData->ScenesLock.Lock();
			}
			else
			{
				for (int32 i = 1; i < dataCount; i++) // start from 1. at index [0] was scene
				{
					auto& stream = data[i];
					auto obj = SceneObjectsFactory::Spawn(context, stream);
					sceneObjects->At(i) = obj;
					if (obj)
					{
						// obj->RegisterObject();
					}
					else
						SceneObjectsFactory::HandleObjectDeserializationError(stream);
				}
			}
		}

		// Capture prefab instances in a scene to restore any missing objects (eg. newly added objects to prefab that are missing in scene file)
		SceneObjectsFactory::PrefabSyncData prefabSyncData(*sceneObjects.Value, data, modifier.Value);
		SceneObjectsFactory::SetupPrefabInstances(context, prefabSyncData);
		// TODO: resave and force sync scenes during game cooking so this step could be skipped in game
		SceneObjectsFactory::SynchronizeNewPrefabInstances(context, prefabSyncData);

		// /\ all above this has to be done on an any thread
		// \/ all below this has to be done on multiple threads at once

		// Load all scene objects
		{
			PROFILE_CPU_NAMED("Deserialize");
			SceneObject** objects = sceneObjects->Get();
			bool wasAsync = context.Async;
			context.Async = false; // TODO: before doing full async for scene objects fix:
			// TODO: - fix Actor's Scripts and Children order when loading objects data out of order via async jobs
			// TODO: - add _loadNoAsync flag to SceneObject or Actor to handle non-async loading for those types (eg. UIControl/UICanvas)
			if (context.Async)
			{
				systemData->ScenesLock.Unlock(); // Unlock scenes from Main Thread so Job Threads can use it to safely setup actors hierarchy (see Actor::Deserialize)
				Threading::JobSystem::Execute([&](int32 i)
				{
					i++; // Start from 1. at index [0] was scene
					auto obj = objects[i];
					if (obj)
					{
						/*auto& idMapping = Scripting::ObjectsLookupIdMapping.Get();
						idMapping = &context.GetModifier()->IdsMapping;*/
						SceneObjectsFactory::Deserialize(context, obj, data[i]);
						// idMapping = nullptr;
					}
				}, dataCount - 1);
				systemData->ScenesLock.Lock();
			}
			else
			{
				// Scripting::ObjectsLookupIdMapping.Set(&modifier.Value->IdsMapping);
				for (int32 i = 1; i < dataCount; i++) // start from 1. at index [0] was scene
				{
					auto& objData = data[i];
					auto obj = objects[i];
					if (obj)
						SceneObjectsFactory::Deserialize(context, obj, objData);
				}
				// Scripting::ObjectsLookupIdMapping.Set(nullptr);
			}
			context.Async = wasAsync;
		}

		// /\ all above this has to be done on multiple threads at once
		// \/ all below this has to be done on an any thread

		// Synchronize prefab instances (prefab may have objects removed or reordered so deserialized instances need to synchronize with it)
		// TODO: resave and force sync scenes during game cooking so this step could be skipped in game
		SceneObjectsFactory::SynchronizePrefabInstances(context, prefabSyncData);

		// Cache transformations
		{
			PROFILE_CPU_NAMED("Cache Transform");

			scene->OnTransformChanged();
		}

		// Initialize scene objects
		{
			PROFILE_CPU_NAMED("Initialize");

			SceneObject** objects = sceneObjects->Get();
			for (int32 i = 0; i < dataCount; i++)
			{
				SceneObject* obj = objects[i];
				if (obj)
				{
					obj->Initialize();

					// Delete objects without parent
					if (i != 0 && obj->GetParent() == nullptr)
					{
						LOG_WARNING("Level", "Scene object {0} {1} has missing parent object after load. Removing it.", obj->GetInstanceID(), obj->ToString());
						obj->DeleteObject();
					}
				}
			}
			prefabSyncData.InitNewObjects();
		}

		// /\ all above this has to be done on an any thread
		// \/ all below this has to be done on a main thread

		// Link scene and call init
		{
			PROFILE_CPU_NAMED("BeginPlay");

			Threading::ScopeLock lock(systemData->ScenesLock);
			Level::Scenes.Add(scene);
			SceneBeginData beginData;
			scene->BeginPlay(&beginData);
			beginData.OnDone();
		}

		// Fire event
		CallSceneEvent(SceneEventType::OnSceneLoaded, scene, sceneId);

		stopwatch.Stop();
		LOG_INFO("Level", "Scene loaded in {0}ms", stopwatch.GetMilliseconds());
		if (outScene)
		{
			*outScene = scene;
		}
		return true;
	}

	//*******************************************************************

	class LoadSceneAction : public SceneAction
	{
	public:
		UID sceneId;
		AssetRef<JsonAsset> sceneAsset;

		LoadSceneAction(const UID& sceneId, JsonAsset* sceneAsset)
		{
			this->sceneId = sceneId;
			this->sceneAsset = sceneAsset;
		}

		bool CanDo() const override
		{
			return sceneAsset == nullptr || sceneAsset->IsLoaded();
		}

		bool Do() const override
		{
			// Now to deserialize scene in a proper way we need to load scripting
			/*if (!Scripting::IsEveryAssemblyLoaded())
			{
				LOG_ERROR("Level", "Scripts must be compiled without any errors in order to load a scene.");
#if SE_EDITOR
				Platform::Error(TEXT("Scripts must be compiled without any errors in order to load a scene. Please fix it."));
#endif
				LevelSystem::CallSceneEvent(SceneEventType::OnSceneLoadError, nullptr, SceneId);
				return false;
			}*/

			// Load scene
			if (!LevelSystem::LoadScene(sceneAsset))
			{
				LOG_ERROR("Level", "Failed to deserialize scene {0}", sceneId);
				LevelSystem::CallSceneEvent(SceneEventType::OnSceneLoadError, nullptr, sceneId);
				return false;
			}

			return true;
		}
	};

	class UnloadSceneAction : public SceneAction
	{
	public:
		UID TargetScene;

		UnloadSceneAction(Scene* scene)
		{
			TargetScene = scene->GetInstanceID();
		}

		bool Do() const override
		{
			auto scene = Level::FindScene(TargetScene);
			if (!scene)
			{
				return true;
			}
			return LevelSystem::UnloadScene(scene);
		}
	};

	class SaveSceneAction : public SceneAction
	{
	public:
		Scene* TargetScene;
		bool PrettyJson;

		SaveSceneAction(Scene* scene, bool prettyJson = true)
		{
			TargetScene = scene;
			PrettyJson = prettyJson;
		}

		bool Do() const override
		{
			if (!LevelSystem::SaveScene(TargetScene, TargetScene->GetPath()))
			{
				LOG_ERROR("Level", "Failed to save scene {0}", TargetScene ? TargetScene->GetName() : String::Empty);
				return false;
			}
			return true;
		}
	};

	//*******************************************************************
	List<Scene*> Level::Scenes;

	Delegate<Actor*> Level::ActorSpawned;
	Delegate<Actor*> Level::ActorDeleted;
	Delegate<Actor*, Actor*> Level::ActorParentChanged;
	Delegate<Actor*> Level::ActorOrderInParentChanged;
	Delegate<Actor*> Level::ActorNameChanged;
	Delegate<Actor*> Level::ActorActiveChanged;
	Delegate<> Level::OnActorGizmos;

	Delegate<Scene*, const UID&> Level::SceneSaving;
	Delegate<Scene*, const UID&> Level::SceneSaved;
	Delegate<Scene*, const UID&> Level::SceneSaveError;
	Delegate<Scene*, const UID&> Level::SceneLoading;
	Delegate<Scene*, const UID&> Level::SceneLoaded;
	Delegate<Scene*, const UID&> Level::SceneLoadError;
	Delegate<Scene*, const UID&> Level::SceneUnloading;
	Delegate<Scene*, const UID&> Level::SceneUnloaded;

	CriticalSection& Level::GetScenesLock()
	{
		return systemData->ScenesLock;
	}

	Scene* Level::FindScene(const UID& id)
	{
		Threading::ScopeLock lock(GetScenesLock());
		for (int32 i = 0; i < Scenes.Count(); i++)
		{
			if (Scenes[i]->GetInstanceID() == id)
			{
				return Scenes[i];
			}
		}
		return nullptr;
	}

	bool Level::SpawnActor(Actor* actor, Actor* parent)
	{
		ENGINE_ASSERT(actor);
		Threading::ScopeLock lock(systemData->sceneActionsLocker);
		return LevelSystem::SpawnActor(actor, parent);
	}

	bool Level::DeleteActor(Actor* actor)
	{
		ENGINE_ASSERT(actor);
		Threading::ScopeLock lock(systemData->sceneActionsLocker);
		return LevelSystem::DeleteActor(actor);
	}

	void Level::DrawActors(RenderContextBatch& renderContextBatch, SceneRendering::DrawCategory category)
	{
		PROFILE_CPU();

		for (Scene* scene : Scenes)
		{
			if (scene->IsActiveInHierarchy())
			{
				scene->Rendering.Draw(renderContextBatch, category);
			}
		}
	}

	bool Level::LoadSceneAsync(const UID& id)
	{
		// Check ID
		if (!id.IsValid())
		{
			Log::ArgumentException();
			return false;
		}

		// Preload scene asset
		const auto sceneAsset = AssetContent::LoadAsync<JsonAsset>(id);
		if (sceneAsset == nullptr)
		{
			LOG_ERROR("Scene", "Cannot load scene asset.");
			return false;
		}

		Threading::ScopeLock lock(systemData->sceneActionsLocker);
		systemData->sceneActions.Enqueue(New<LoadSceneAction>(id, sceneAsset));

		return true;
	}

	void Level::SaveSceneAsync(Scene* scene)
	{
		Threading::ScopeLock lock(systemData->sceneActionsLocker);
		systemData->sceneActions.Enqueue(New<SaveSceneAction>(scene));
	}

	bool Level::SaveSceneToBytes(Scene* scene, Json::StringBuffer& outData, bool prettyJson)
	{
		ENGINE_ASSERT(scene);
		Threading::ScopeLock lock(systemData->sceneActionsLocker);
		Stopwatch stopwatch;
		LOG_INFO("Level", "Saving scene {0} to bytes", scene->GetName());

		// Serialize to json
		if (!LevelSystem::SaveScene(scene, outData, prettyJson))
		{
			LevelSystem::CallSceneEvent(SceneEventType::OnSceneSaveError, scene, scene->GetInstanceID());
			return false;
		}

		stopwatch.Stop();
		LOG_INFO("Level", "Scene saved! Time {0}ms", stopwatch.GetMilliseconds());

		// Fire event
		LevelSystem::CallSceneEvent(SceneEventType::OnSceneSaved, scene, scene->GetInstanceID());

		return true;
	}

	bool Level::SaveSceneToBytes(List<byte> &bytes, Scene* scene, bool prettyJson)
	{
		Json::StringBuffer sceneData;
		if (!SaveSceneToBytes(scene, sceneData, prettyJson))
		{
			return false;
		}

		bytes.Set((const byte*)sceneData.GetString(), (int32)sceneData.GetSize() * sizeof(Json::StringBuffer::Ch));
		return true;
	}

	void Level::CallActorEvent(ActorEventType eventType, Actor* a, Actor* b)
	{
		PROFILE_CPU();

		ENGINE_ASSERT(a);

		// Call event
		switch (eventType)
		{
		case ActorEventType::OnActorSpawned:
			ActorSpawned(a);
			break;
		case ActorEventType::OnActorDeleted:
			ActorDeleted(a);
			break;
		case ActorEventType::OnActorParentChanged:
			ActorParentChanged(a, b);
			break;
		case ActorEventType::OnActorOrderInParentChanged:
			ActorOrderInParentChanged(a);
			break;
		case ActorEventType::OnActorNameChanged:
			ActorNameChanged(a);
			break;
		case ActorEventType::OnActorActiveChanged:
			ActorActiveChanged(a);
			break;
		}
	}

} // SE