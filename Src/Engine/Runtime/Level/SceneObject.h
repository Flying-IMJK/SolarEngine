#pragma once

#include "Runtime/Core/Types/Object.h"
#include "Runtime/Core/Serialization/ISerializable.h"
#include "Runtime/API.h"
#include "Runtime/Core/Scripting/ScriptingObject.h"

namespace SE
{
	class Actor;

	/// <summary>
	/// Scene objects setup data container used for BeginPlay callback.
	/// </summary>
	class SceneBeginData
	{
	public:
		/// <summary>
		/// The joints to create after setup.
		/// </summary>
		// List<Joint*> JointsToCreate;

		/// <summary>
		/// Called when scene objects setup is done.
		/// </summary>
		void OnDone();
	};

	SE_CLASS(Reflect, API(NoSpawn, Abstract))
	class SE_API_RUNTIME SceneObject : public ScriptingObject, public ISerializable
	{
		SCRIPTING_TYPE_NO_SPAWN(SceneObject);
		SE_DEFINE_CLASS(SceneObject, ScriptingObject);

		friend class PrefabInstanceData;
		friend class PrefabManager;
		friend class Actor;
		friend class Level;
	protected:
		Actor* m_Parent;
		UID m_PrefabID;
		UID m_PrefabObjectID;
		bool m_IsDuringPlay;
	public:
		/// <summary>
		/// Initializes a new instance of the <see cref="SceneObject"/> class.
		/// </summary>
		/// <param name="params">The object initialization parameters.</param>
		SceneObject(const SpawnParams& params);
		~SceneObject() override;


		/// <summary>
		/// Determines whether object is during play (spawned/loaded and fully initialized).
		/// </summary>
		SE_FUNCTION(API(Prop, ReadOnly))
		bool IsDuringPlay() const
		{
			return m_IsDuringPlay;
		}

		/// <summary>
		/// Returns true if object has a parent assigned.
		/// </summary>
		SE_FUNCTION(API(Prop, ReadOnly))
		FORCE_INLINE bool HasParent() const
		{
			return m_Parent != nullptr;
		}

		/// <summary>
		/// Gets the parent actor (or null if object has no parent).
		/// </summary>
		SE_FUNCTION(API(Prop))
		FORCE_INLINE Actor* GetParent() const
		{
			return m_Parent;
		}

		/// <summary>
		/// Sets the parent actor.
		/// </summary>
		/// <param name="value">The new parent.</param>
		SE_FUNCTION(API(Prop))
		FORCE_INLINE void SetParent(Actor* value)
		{
			SetParent(value, true);
		}

		/// <summary>
		/// Sets the parent actor.
		/// </summary>
		/// <param name="value">The new parent.</param>
		/// <param name="canBreakPrefabLink">True if can break prefab link on changing the parent.</param>
		SE_FUNCTION(API())
		virtual void SetParent(Actor* value, bool canBreakPrefabLink) = 0;

		/// <summary>
		/// Gets the scene object ID.
		/// </summary>
		/// <returns>The scene object ID.</returns>
        SE_FUNCTION(API(ReadOnly, Prop))
		virtual UID GetSceneObjectId() const = 0;

		/// <summary>
		/// Gets zero-based index in parent actor children list (scripts or child actors).
		/// </summary>
		/// <returns>The order in parent.</returns>
		SE_FUNCTION(API(Prop))
		virtual int32 GetOrderInParent() const = 0;

		/// <summary>
		/// Sets zero-based index in parent actor children list (scripts or child actors).
		/// </summary>
		/// <param name="index">The new order in parent.</param>
		SE_FUNCTION(API(Prop))
		virtual void SetOrderInParent(int32 index) = 0;
	public:
		/// <summary>
		/// Gets a value indicating whether this object has a valid linkage to the prefab asset.
		/// </summary>
		SE_FUNCTION(API(Prop, ReadOnly))
		FORCE_INLINE bool HasPrefabLink() const
		{
			return m_PrefabID.IsValid();
		}

		/// <summary>
		/// Gets the prefab asset ID. Empty if no prefab link exists.
		/// </summary>
		SE_FUNCTION(API(Prop, ReadOnly))
		FORCE_INLINE UID GetPrefabID() const
		{
			return m_PrefabID;
		}

		/// <summary>
		/// Gets the ID of the object within a prefab that is used for synchronization with this object. Empty if no prefab link exists.
		/// </summary>
		SE_FUNCTION(API(Prop, ReadOnly))
		FORCE_INLINE UID GetPrefabObjectID() const
		{
			return m_PrefabObjectID;
		}

		/// <summary>
		/// Links scene object instance to the prefab asset and prefab object. Warning! This applies to the only this object (not scripts or child actors).
		/// </summary>
		/// <param name="prefabId">The prefab asset identifier.</param>
		/// <param name="prefabObjectId">The prefab object identifier.</param>
		SE_FUNCTION(API(Attributes="NoAnimate"))
		virtual void LinkPrefab(const UID& prefabId, const UID& prefabObjectId);

		/// <summary>
		/// Breaks the prefab linkage for this object, all its scripts, and all child actors.
		/// </summary>
		SE_FUNCTION(API(Attributes="NoAnimate"))
		virtual void BreakPrefabLink();

		/// <summary>
		/// Gets the path containing name of this object and all parent objects in tree hierarchy separated with custom separator character (/ by default). Can be used to identify this object in logs.
		/// </summary>
		/// <param name="separatorChar">The character to separate the names.</param>
		/// <returns>The full name path.</returns>
		SE_FUNCTION(API())
		String GetNamePath(StringView separatorChar = SE_TEXT("/")) const;
	public:

		/// <summary>
		/// Called after object loading or spawning to initialize the object (eg. call OnAwake for scripts) but before BeginPlay. Initialization should be performed only within a single SceneObject (use BeginPlay to initialize with a scene).
		/// </summary>
		virtual void Initialize() = 0;

		/// <summary>
		/// Called when adding object to the game.
		/// </summary>
		/// <param name="data">The initialization data (e.g. used to collect joints to link after begin).</param>
		virtual void BeginPlay(SceneBeginData* data) = 0;

		/// <summary>
		/// Called when removing object from the game.
		/// </summary>
		virtual void EndPlay() = 0;

	public:
		// [ISerializable]
		void Serialize(SerializeContext& context) override;
		void Deserialize(DeserializeContext& context) override;
	};

	namespace Serialization
	{
		bool ShouldSerialize(const SceneObject* v, const void* otherObj);

		void Serialize(SerializeContext& context, SceneObject* v);

		void Deserialize(DeserializeContext& context, SceneObject* v);
	}
} // SE
