#pragma once

#include "Scene/Scene.h"
#include "Runtime/Core/Platform/CriticalSection.h"
#include "Runtime/Core/Types/Collections/List.h"

namespace SE
{

	SE_CLASS(API(Static))
	class SE_API_RUNTIME Level
	{
		SCRIPTING_TYPE_MIN(Level)
		friend class LevelSystem;
		friend class Actor;
	public:
		/// <summary>
		/// The scenes collection lock.
		/// </summary>
		static CriticalSection& GetScenesLock();

		/// <summary>
		/// The loaded scenes collection.
		/// </summary>
		static List<Scene*> Scenes;

	public:
		/// <summary>
		/// Gets the scenes count.
		/// </summary>
		SE_FUNCTION(API())
		static int32 GetScenesCount()
		{
			return Scenes.Count();
		}

		SE_FUNCTION(API())
		static bool IsAnySceneLoaded()
		{
			return Scenes.HasItems();
		}

		/// <summary>
		/// Gets the scene.
		/// </summary>
		/// <param name="index">The index.</param>
		/// <returns>The scene object (loaded).</returns>
		SE_FUNCTION(API())
		static Scene* GetScene(int32 index)
		{
			return Scenes[index];
		}

		SE_FUNCTION(API())
		static Scene* FindScene(const UID& id);

		/// <summary>
		/// Gets a loaded scene name by its current level index.
		/// </summary>
		SE_FUNCTION(API())
		static String GetSceneName(int32 index)
		{
			Scene* scene = index >= 0 && index < Scenes.Count() ? Scenes[index] : nullptr;
			return scene != nullptr ? scene->GetName() : String();
		}

		/// <summary>
		/// Queues saving a loaded scene selected by its current level index.
		/// </summary>
		SE_FUNCTION(API())
		static bool SaveSceneAt(int32 index)
		{
			Scene* scene = index >= 0 && index < Scenes.Count() ? Scenes[index] : nullptr;
			if (scene == nullptr)
				return false;
			SaveSceneAsync(scene);
			return true;
		}

		////////////////////// Scene Event
		/// <summary>
		/// Fired when scene starts saving.
		/// </summary>
		SE_EVENT(API())
		static Delegate<Scene*, const UID&> SceneSaving;

		/// <summary>
		/// Fired when scene gets saved.
		/// </summary>
		SE_EVENT(API())
		static Delegate<Scene*, const UID&> SceneSaved;

		/// <summary>
		/// Fired when scene gets saving error.
		/// </summary>
		SE_EVENT(API())
		static Delegate<Scene*, const UID&> SceneSaveError;

		/// <summary>
		/// Fired when scene starts loading.
		/// </summary>
		SE_EVENT(API())
		static Delegate<Scene*, const UID&> SceneLoading;

		/// <summary>
		/// Fired when scene gets loaded.
		/// </summary>
		SE_EVENT(API())
		static Delegate<Scene*, const UID&> SceneLoaded;

		/// <summary>
		/// Fired when scene cannot be loaded (argument is error number).
		/// </summary>
		SE_EVENT(API())
		static Delegate<Scene*, const UID&> SceneLoadError;

		/// <summary>
		/// Fired when scene gets unloading.
		/// </summary>
		SE_EVENT(API())
		static Delegate<Scene*, const UID&> SceneUnloading;

		/// <summary>
		/// Fired when scene gets unloaded.
		/// </summary>
		SE_EVENT(API())
		static Delegate<Scene*, const UID&> SceneUnloaded;


		////////////////////// Actor Event
		/// <summary>
		/// Occurs when new actor gets spawned to the game.
		/// </summary>
		SE_EVENT(API())
		static Delegate<Actor*> ActorSpawned;

		/// <summary>
		/// Occurs when actor is removed from the game.
		/// </summary>
		SE_EVENT(API())
		static Delegate<Actor*> ActorDeleted;

		/// <summary>
		/// Occurs when actor parent gets changed. Arguments: actor and previous parent actor.
		/// </summary>
		SE_EVENT(API())
		static Delegate<Actor*, Actor*> ActorParentChanged;

		/// <summary>
		/// Occurs when actor index in parent actor children gets changed.
		/// </summary>
		SE_EVENT(API())
		static Delegate<Actor*> ActorOrderInParentChanged;

		/// <summary>
		/// Occurs when actor name gets changed.
		/// </summary>
		SE_EVENT(API())
		static Delegate<Actor*> ActorNameChanged;

		/// <summary>
		/// Occurs when actor active state gets modified.
		/// </summary>
		SE_EVENT(API())
		static Delegate<Actor*> ActorActiveChanged;


		static Delegate<> OnActorGizmos;

	public:
		/// <summary>
		/// Spawn actor on the scene
		/// </summary>
		/// <param name="actor">Actor to spawn</param>
		/// <returns>True if action cannot be done, otherwise false.</returns>
		FORCE_INLINE static bool SpawnActor(Actor* actor)
		{
			return SpawnActor(actor, nullptr);
		}

		/// <summary>
		/// Spawns actor on the scene.
		/// </summary>
		/// <param name="actor">The actor to spawn.</param>
		/// <param name="parent">The parent actor (will link spawned actor with this parent).</param>
		/// <returns>True if action cannot be done, otherwise false.</returns>
		SE_FUNCTION(API())
		static bool SpawnActor(Actor* actor, Actor* parent);

		/// <summary>
		/// Deletes actor from the scene.
		/// </summary>
		/// <param name="actor">The actor to delete.</param>
		/// <returns>True if action cannot be done, otherwise false.</returns>
		SE_FUNCTION(API())
		static bool DeleteActor(Actor* actor);


		/// <summary>
		/// Draws all the actors.
		/// </summary>
		/// <param name="renderContextBatch">The rendering context batch.</param>
		/// <param name="category">The actors category to draw (see SceneRendering::DrawCategory).</param>
		static void DrawActors(RenderContextBatch& renderContextBatch, SceneRendering::DrawCategory category = SceneRendering::DrawCategory::SceneDraw);

	public:
		/// <summary>
		/// Loads scene from the asset. Done in the background.
		/// </summary>
		/// <param name="id">Scene ID</param>
		/// <returns>True if loading cannot be done, otherwise false.</returns>
		SE_FUNCTION(API())
		static bool LoadSceneAsync(const UID& id);

		SE_FUNCTION(API())
		static bool SaveScene(Scene* scene, bool prettyJson = true);

		SE_FUNCTION(API())
		static void SaveSceneAsync(Scene* scene);

		/// <summary>
		/// Saves scene to the bytes.
		/// </summary>
		/// <param name="scene">Scene to serialize.</param>
		/// <param name="outData">The result data.</param>
		/// <param name="prettyJson">True if use pretty Json format writer, otherwise will use the compact Json format writer that packs data to use less memory and perform the action faster.</param>
		/// <returns>True if action cannot be done, otherwise false.</returns>
		static bool SaveSceneToBytes(Scene* scene, Json::StringBuffer& outData, bool prettyJson = true);

		/// <summary>
		/// Saves scene to the bytes.
		/// </summary>
		/// <param name="scene">Scene to serialize.</param>
		/// <param name="prettyJson">True if use pretty Json format writer, otherwise will use the compact Json format writer that packs data to use less memory and perform the action faster.</param>
		/// <returns>The result data or empty if failed.</returns>
		static bool SaveSceneToBytes(List<byte> &bytes, Scene* scene, bool prettyJson = true);

		enum class ActorEventType
		{
			OnActorSpawned = 0,
			OnActorDeleted = 1,
			OnActorParentChanged = 2,
			OnActorOrderInParentChanged = 3,
			OnActorNameChanged = 4,
			OnActorActiveChanged = 5,
		};

	    static void CallActorEvent(ActorEventType eventType, Actor* a, Actor* b);
	};

} // SE
