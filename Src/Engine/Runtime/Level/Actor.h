#pragma once

#include "SceneObject.h"
#include "Runtime/Core/Math/BoundingVolumes.h"
#include "Runtime/Core/Math/Transform.h"
#include "Runtime/Render/RenderEnum.h"
#include "Runtime/API.h"
#include "Runtime/Render/SceneRendering.h"

namespace SE
{
	class Scene;

	SE_CLASS(Reflect)
	class SE_API_RUNTIME Actor : public SceneObject
	{
		friend class SceneRendering;
		friend class SceneQuery;
		friend class Level;
		friend class PrefabManager;
		friend class Scene;
		friend class SceneRendering;
		friend class Prefab;
		friend class PrefabInstanceData;

		SE_DEFINE_CLASS(Actor, SceneObject);
	protected:
		uint16 m_IsActive : 1;
		uint16 m_IsActiveInHierarchy : 1;
		uint16 m_IsPrefabRoot : 1;
		uint16 m_IsEnabled : 1;
		uint16 m_DrawNoCulling : 1;
		uint16 m_DrawCategory : 4;
		byte m_Layer;
		EnumFlags<StaticMask> m_StaticFlags;
		Transform m_LocalTransform;
		Transform m_Transform;
		BoundingSphere m_Sphere;
		BoundingBox m_Box;
		String m_Name;
		Scene* m_Scene;
	private:
		// Disable copying
		Actor(Actor const&) = delete;
		Actor& operator=(Actor const&) = delete;

		/// <summary>
		/// List with all child actors attached to the actor (readonly). All items are valid (not null).
		/// </summary>
		List<Actor*> Children;

	public:
		Actor();
		~Actor() override;

		/// <summary>
		/// Gets actor bounding sphere that defines 3D space intersecting with the actor (for determination of the visibility for actor).
		/// </summary>
		FORCE_INLINE const BoundingSphere& GetSphere() const
		{
			return m_Sphere;
		}

		/// <summary>
		/// Gets actor bounding box that defines 3D space intersecting with the actor (for determination of the visibility for actor).
		/// </summary>
		FORCE_INLINE const BoundingBox& GetBox() const
		{
			return m_Box;
		}


		FORCE_INLINE int32 GetLayer() const
		{
			return m_Layer;
		}

		/// <summary>
		/// Gets the layer mask (with single bit set).
		/// </summary>
		FORCE_INLINE int32 GetLayerMask() const
		{
			return 1 << static_cast<int32>(m_Layer);
		}

		/// <summary>
		/// Sets the layer.
		/// </summary>
		/// <param name="layerIndex">The index of the layer.</param>
		void SetLayer(int32 layerIndex);

		/// <summary>
		/// Sets the layer recursively for all underlying children.
		/// </summary>
		/// <param name="layerIndex">The index of the layer.</param>
		void SetLayerRecursive(int32 layerIndex);

		/// <summary>
		/// Gets the name of the layer.
		/// </summary>
		const String& GetLayerName() const;

		/// <summary>
		/// Sets the name of the layer.
		/// </summary>
		void SetLayerName(const StringView& value);

		/// <summary>
		/// Sets the name of the layer recursively for actor and for all underlying child actors.
		/// </summary>
		void SetLayerNameRecursive(const StringView& value);


		FORCE_INLINE const String& GetName() const
		{
			return m_Name;
		}

		/// <summary>
		/// Sets the actor name.
		/// </summary>
		/// <param name="value">The value to set.</param>
		void SetName(const StringView& value);

		/// <summary>
		/// Gets the scene object which contains this actor.
		/// </summary>
		FORCE_INLINE Scene* GetScene() const
		{
			return m_Scene;
		}

		FORCE_INLINE float GetPerInstanceRandom() const
		{
			return GetInstanceID().C * (1.0f / (float)Max_uint32);
		}
	public:
		
		/// <summary>
		/// Sets a actor parent.
		/// </summary>
		/// <param name="value">New parent</param>
		/// <param name="worldPositionsStays">Should actor world positions remain the same after parent change?</param>
		/// <param name="canBreakPrefabLink">True if can break prefab link on changing the parent.</param>
		void SetParent(Actor* value, bool worldPositionsStays, bool canBreakPrefabLink);

		/// <summary>
		/// Gets amount of child actors.
		/// </summary>
		FORCE_INLINE int32 GetChildrenCount() const
		{
			return Children.Count();
		}

		/// <summary>
		/// Gets the child actor at the given index.
		/// </summary>
		/// <param name="index">The child actor index.</param>
		/// <returns>The child actor (always valid).</returns>
		Actor* GetChild(int32 index) const;

		/// <summary>
		/// Gets the child actor with the given name.
		/// </summary>
		/// <param name="name">The child actor name.</param>
		/// <returns>The child actor or null.</returns>
		Actor* GetChild(const StringView& name) const;

		/*/// <summary>
		/// Gets the child actor of the given type.
		/// </summary>
		/// <param name="type">Type of the actor to search for. Includes any actors derived from the type.</param>
		/// <returns>The child actor or null.</returns>
		Actor* GetChild(const MClass* type) const;

		/// <summary>
		/// Gets the child actor of the given type.
		/// </summary>
		/// <returns>The child actor or null.</returns>
		template<typename T>
		FORCE_INLINE T* GetChild() const
		{
			return (T*)GetChild(T::GetStaticClass());
		}

		/// <summary>
		/// Finds the child actor of the given type or creates a new one.
		/// </summary>
		/// <returns>The child actor.</returns>
		template<typename T>
		T* GetOrAddChild()
		{
			T* result = (T*)GetChild(T::GetStaticClass());
			if (!result)
			{
				result = New<T>();
				result->SetParent(this, false, false);
			}
			return result;
		}

		/// <summary>
		/// Gets the child actors of the given type.
		/// </summary>
		/// <param name="type">Type of the actor to search for. Includes any actors derived from the type.</param>
		/// <returns>The child actors.</returns>
		List<Actor*> GetChildren(const MClass* type) const;

		/// <summary>
		/// Gets the child actors of the given type.
		/// </summary>
		/// <returns>The child actors.</returns>
		template<typename T>
		List<T*> GetChildren() const
		{
			const MClass* type = T::GetStaticClass();
			List<T*> result;
			for (Actor* child : Children)
				if (IsSubClassOf(child, type))
					result.Add((T*)child);
			return result;
		}*/

		/// <summary>
		/// Destroys the children. Calls Object.Destroy on every child actor and unlinks them for this actor.
		/// </summary>
		/// <param name="timeLeft">The time left to destroy object (in seconds).</param>
		void DestroyChildren(float timeLeft = 0.0f);

	public:
		/// <summary>
		/// Gets value indicating if actor is active in the scene.
		/// </summary>
		FORCE_INLINE bool GetIsActive() const
		{
			return m_IsActive != 0;
		}

		/// <summary>
		/// Sets value indicating if actor is active in the scene.
		/// </summary>
		/// <param name="value">The value to set.</param>
		virtual void SetIsActive(bool value);

		/// <summary>
		/// Gets value indicating if actor is active in the scene graph. It must be active as well as that of all it's parents.
		/// </summary>
		FORCE_INLINE bool IsActiveInHierarchy() const
		{
			return m_IsActiveInHierarchy != 0;
		}

		/// <summary>
		/// Gets value indicating if actor is in a scene.
		/// </summary>
		FORCE_INLINE bool HasScene() const
		{
			return m_Scene != nullptr;
		}

		/// <summary>
		/// Returns true if object is fully static on the scene, otherwise false.
		/// </summary>
		FORCE_INLINE bool IsStatic() const
		{
			return m_StaticFlags.IsFlag(StaticMask::FullyStatic);
		}

		/// <summary>
		/// Returns true if object has static transform.
		/// </summary>
		FORCE_INLINE bool IsTransformStatic() const
		{
			return m_StaticFlags.IsFlag(StaticMask::Transform);
		}

		/// <summary>
		/// Gets the actor static flags.
		/// </summary>
		FORCE_INLINE EnumFlags<StaticMask> GetStaticFlags() const
		{
			return m_StaticFlags;
		}

		/// <summary>
		/// Returns true if object has given flag(s) set.
		/// </summary>
		FORCE_INLINE bool HasStaticFlag(StaticMask flag) const
		{
			return m_StaticFlags.IsFlag(flag);
		}

		/// <summary>
		/// Sets a single static flag to the desire value.
		/// </summary>
		/// <param name="flag">The flag to change.</param>
		/// <param name="value">The target value of the flag.</param>
		void SetStaticFlag(StaticMask flag, bool value);

	public:
		/// <summary>
		/// Gets the actor's world transformation.
		/// </summary>
		FORCE_INLINE Transform& GetTransform()
		{
			return m_Transform;
		}

		FORCE_INLINE const Transform& GetTransform() const
		{
			return m_Transform;
		}

		/// <summary>
		/// Sets the actor's world transformation.
		/// </summary>
		/// <param name="value">The value to set.</param>
		void SetTransform(const Transform& value);

		/// <summary>
		/// Gets the actor's world transform position.
		/// </summary>
		FORCE_INLINE Float3 GetPosition() const
		{
			return m_Transform.Translation;
		}

		/// <summary>
		/// Sets the actor's world transform position.
		/// </summary>
		/// <param name="value">The value to set.</param>
		void SetPosition(const Float3& value);

		/// <summary>
		/// Gets actor summary> in 3D space
		/// </summary>
		FORCE_INLINE Quaternion GetOrientation() const
		{
			return m_Transform.Orientation;
		}

		/// <summary>
		/// Sets actor quaternion in 3D space.
		/// </summary>
		/// <param name="value">The value to set.</param>
		void SetOrientation(const Quaternion& value);

		/// <summary>
		/// Gets actor scale in 3D space.
		/// </summary>
		FORCE_INLINE Float3 GetScale() const
		{
			return m_Transform.Scale;
		}

		/// <summary>
		/// Sets actor scale in 3D space
		/// </summary>
		/// <param name="value">The value to set.</param>
		void SetScale(const Float3& value);

		/// <summary>
		/// Gets actor rotation matrix.
		/// </summary>
		Matrix GetRotation() const;

		/// <summary>
		/// Sets actor rotation matrix.
		/// </summary>
		/// <param name="value">The value to set.</param>
		void SetRotation(const Matrix& value);

		/// <summary>
		/// Gets actor direction vector (forward vector).
		/// </summary>
		FORCE_INLINE Float3 GetForward() const
		{
			return Float3::Transform(Float3::Forward, GetOrientation());
		}

		/// <summary>
		/// Sets actor direction vector (forward)
		/// </summary>
		/// <param name="value">The value to set.</param>
		void SetForward(const Float3& value);

		virtual bool IntersectsItself(const Ray& ray, float& distance, Float3& normal);
	public:
		/// <summary>
		/// Resets the actor local transform.
		/// </summary>
		void ResetLocalTransform();

		/// <summary>
		/// Gets local transform of the actor in parent actor space.
		/// </summary>
		FORCE_INLINE Transform GetLocalTransform() const
		{
			return m_LocalTransform;
		}

		/// <summary>
		/// Sets local transform of the actor in parent actor space.
		/// </summary>
		/// <param name="value">The value to set.</param>
		void SetLocalTransform(const Transform& value);

		/// <summary>
		/// Gets local position of the actor in parent actor space.
		/// </summary>
		FORCE_INLINE Float3 GetLocalPosition() const
		{
			return m_LocalTransform.Translation;
		}

		/// <summary>
		/// Sets local position of the actor in parent actor space.
		/// </summary>
		/// <param name="value">The value to set.</param>
		void SetLocalPosition(const Float3& value);

		/// <summary>
		/// Gets local rotation of the actor in parent actor space.
		/// </summary>
		/// <code>Actor.LocalOrientation *= Quaternion.Euler(0, 10 * Time.DeltaTime, 0)</code>
		FORCE_INLINE Quaternion GetLocalQuaternion() const
		{
			return m_LocalTransform.Orientation;
		}

		/// <summary>
		/// Sets local rotation of the actor in parent actor space.
		/// </summary>
		/// <param name="value">The value to set.</param>
		void SetLocalQuaternion(const Quaternion& value);

		/// <summary>
		/// Gets local scale vector of the actor in parent actor space.
		/// </summary>
		FORCE_INLINE Float3 GetLocalScale() const
		{
			return m_LocalTransform.Scale;
		}

		/// <summary>
		/// Sets local scale vector of the actor in parent actor space.
		/// </summary>
		/// <param name="value">The value to set.</param>
		void SetLocalScale(const Float3& value);


		/// <summary>
		/// Gets the matrix that transforms a point from the world space to local space of the actor.
		/// </summary>
		/// <param name="worldToLocal">The world to local matrix.</param>
		void GetWorldToLocalMatrix(Matrix& worldToLocal) const;

		/// <summary>
		/// Gets the matrix that transforms a point from the local space of the actor to world space.
		/// </summary>
		/// <param name="localToWorld">The world to local matrix.</param>
		void GetLocalToWorldMatrix(Matrix& localToWorld) const;
	public:

		/// <summary>
		/// Calls Initialize for all objects in the actor hierarchy.
		/// </summary>
		void InitializeHierarchy();

		/// <summary>
		/// Called when actor gets added to game systems. Occurs on BeginPlay event or when actor gets activated in hierarchy. Use this event to register object to other game system (eg. audio).
		/// </summary>
		virtual void OnEnable();

		/// <summary>
		/// Called when actor gets removed from game systems. Occurs on EndPlay event or when actor gets inactivated in hierarchy. Use this event to unregister object from other game system (eg. audio).
		/// </summary>
		virtual void OnDisable();

	private:
		void SetSceneInHierarchy(Scene* scene);
		void OnEnableInHierarchy();
		void OnDisableInHierarchy();

	public:

		/// <summary>
		/// Called when actor transform gets changed.
		/// </summary>
		virtual void OnTransformChanged();

		/// <summary>
		/// Called when actor active state gets changed.
		/// </summary>
		virtual void OnActiveChanged();

		/// <summary>
		/// Called when actor static flag gets changed.
		/// </summary>
		virtual void OnStaticFlagsChanged();

		/// <summary>
		/// Called when layer gets changed.
		/// </summary>
		virtual void OnLayerChanged();

		/// <summary>
		/// Called when actor active in tree state gets changed.
		/// </summary>
		virtual void OnActiveInTreeChanged();

		/// <summary>
		/// Called when adding object to the game.
		/// </summary>
		virtual void OnBeginPlay()
		{
		}

		/// <summary>
		/// Called when removing object from the game.
		/// </summary>
		virtual void OnEndPlay()
		{
		}

		/// <summary>
		/// Execute custom action on actors tree.
		/// </summary>
		/// <param name="action">Actor to call on every actor in the tree. Returns true if keep calling deeper.</param>
		/// <param name="args">Custom arguments for the function</param>
		template<typename... Params>
		void TreeExecute(Function<bool(Actor*, Params ...)>& action, Params ... args)
		{
			if (action(this, args...))
			{
				for (int32 i = 0; i < Children.Count(); i++)
				{
					Children.Get()[i]->TreeExecute<Params...>(action, args...);
				}
			}
		}

		/// <summary>
		/// Execute custom action on actor children tree.
		/// </summary>
		/// <param name="action">Actor to call on every actor in the tree. Returns true if keep calling deeper.</param>
		/// <param name="args">Custom arguments for the function</param>
		template<typename... Params>
		void TreeExecuteChildren(Function<bool(Actor*, Params ...)>& action, Params ... args)
		{
			for (int32 i = 0; i < Children.Count(); i++)
			{
				Children.Get()[i]->TreeExecute<Params...>(action, args...);
			}
		}

	public:
		void Initialize() override;
		void BeginPlay(SceneBeginData* data) override;
		void EndPlay() override;

		const UID& GetSceneObjectId() const override;
		void SetParent(Actor* value, bool canBreakPrefabLink = true) override;
		int32 GetOrderInParent() const override;
		void SetOrderInParent(int32 index) override;

		void Serialize(SerializeContext& context) override;
		void Deserialize(DeserializeContext& context) override;
	};

	SE_CLASS(Reflect)
	class SE_API_RUNTIME RenderActor : public Actor, public IRender
	{
		SE_DEFINE_CLASS(RenderActor, Actor);
	public:
		RenderActor() {}
		// [IRender]
		int RenderGetDrawCategory() override { return m_DrawCategory; }
		int32 RenderGetLayerMask() override { return 1 << static_cast<int32>(m_Layer); };
		BoundingSphere RenderGetSphere() override { return m_Sphere;}
		EnumFlags<StaticMask> RenderGetStaticFlags() override { return m_StaticFlags; }
		void RenderDraw(RenderContext& context) override { }
		void RenderDraw(RenderContextBatch& contextBatch) override { }
	};

} // SE

