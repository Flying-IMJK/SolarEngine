#pragma once

#include "SceneObject.h"
#include "Runtime/Core/Thread/ThreadLocal.h"
#include "Runtime/Core/Types/UID.h"
#include "Runtime/Core/Types/Variable.h"
#include "Runtime/Core/Platform/CriticalSection.h"
#include "Runtime/Core/Serialization/ISerializeModifier.h"
#include "Runtime/Core/Types/Collections/CollectionPoolCache.h"
#include "Runtime/API.h"

namespace SE
{
    class Prefab;

    class SerializeModifierCache
    {
    public:
        static void ISerializeModifierClearCallback(ISerializeModifier* obj);

        /// <summary>
        /// Gets the ISerializeModifier lookup cache. Safe allocation, per thread, uses caching.
        /// </summary>
        static CollectionPoolCache<ISerializeModifier, ISerializeModifierClearCallback> Modifier;

        /// <summary>
        /// Releases all the allocated resources (existing in the pool that are not during usage).
        /// </summary>
        static void Release();
    };


    /// <summary>
    /// Helper class for scene objects creation and deserialization utilities.
    /// </summary>
    class SE_API_RUNTIME SceneObjectsFactory
    {
    public:
        struct PrefabInstance
        {
            int32 StatIndex;
            int32 RootIndex;
            UID RootId;
            Prefab* Prefab;
            bool FixRootParent = false;
            Dictionary<UID, UID> IdsMapping;
        };

        struct Context
        {
            ISerializeModifier* Modifier;
            bool Async = false;
            List<PrefabInstance> Instances;
            Dictionary<UID, int32> ObjectToInstance;
            CriticalSection Locker;
            Threading::ThreadLocal<ISerializeModifier*> Modifiers;

            Context(ISerializeModifier* modifier);
            ~Context();

            ISerializeModifier* GetModifier();
            void SetupIdsMapping(const SceneObject* obj, ISerializeModifier* modifier) const;
        };

        /// <summary>
        /// Creates the scene object from the specified data value. Does not perform deserialization.
        /// </summary>
        /// <param name="context">The serialization context.</param>
        /// <param name="stream">The serialized data stream.</param>
        static SceneObject* Spawn(Context& context, const DeserializeStream& stream);

        /// <summary>
        /// Deserializes the scene object from the specified data value.
        /// </summary>
        /// <param name="context">The serialization context.</param>
        /// <param name="obj">The instance to deserialize.</param>
        /// <param name="stream">The serialized data stream.</param>
        static void Deserialize(Context& context, SceneObject* obj, DeserializeStream& stream);

        /// <summary>
        /// Handles the object deserialization error.
        /// </summary>
        /// <param name="value">The value.</param>
        static void HandleObjectDeserializationError(const DeserializeStream& value);

    public:
        struct PrefabSyncData
        {
            friend SceneObjectsFactory;
            friend class PrefabManager;

            // The created scene objects. Collection can be modified (eg. for spawning missing objects).
            List<SceneObject*>& SceneObjects;
            // The scene objects data.
            const DeserializeStream& Data;
            // The objects deserialization modifier. Collection will be modified (eg. for spawned objects mapping).
            ISerializeModifier* Modifier;

            PrefabSyncData(List<SceneObject*>& sceneObjects, const DeserializeStream& data, ISerializeModifier* modifier);
            void InitNewObjects();

        private:
            struct NewObj
            {
                Prefab* Prefab;
                const DeserializeStream* PrefabData;
                UID PrefabObjectId;
                UID Id;
            };

            int32 InitialCount;
            List<NewObj> NewObjects;
        };

        /// <summary>
        /// Initializes the prefab instances inside the scene objects for proper references deserialization.
        /// </summary>
        /// <remarks>
        /// Should be called after spawning scene objects but before scene objects deserialization.
        /// </remarks>
        /// <param name="context">The serialization context.</param>
        /// <param name="data">The sync data.</param>
        static void SetupPrefabInstances(Context& context, const PrefabSyncData& data);

        /// <summary>
        /// Synchronizes the new prefab instances by spawning missing objects that were added to prefab but were not saved with scene objects collection.
        /// </summary>
        /// <remarks>
        /// Should be called after spawning scene objects but before scene objects deserialization and PostLoad event when scene objects hierarchy is ready (parent-child relation exists). But call it before Init and BeginPlay events.
        /// </remarks>
        /// <param name="context">The serialization context.</param>
        /// <param name="data">The sync data.</param>
        static void SynchronizeNewPrefabInstances(Context& context, PrefabSyncData& data);

        /// <summary>
        /// Synchronizes the prefab instances. Prefabs may have objects removed so deserialized instances need to synchronize with it. Handles also changing prefab object parent in the instance.
        /// </summary>
        /// <remarks>
        /// Should be called after scene objects deserialization and PostLoad event when scene objects hierarchy is ready (parent-child relation exists). But call it before Init and BeginPlay events.
        /// </remarks>
        /// <param name="context">The serialization context.</param>
        /// <param name="data">The sync data.</param>
        static void SynchronizePrefabInstances(Context& context, PrefabSyncData& data);

    private:
        static void SynchronizeNewPrefabInstances(Context& context, PrefabSyncData& data, Prefab* prefab, Actor* actor, const UID& actorPrefabObjectId, int32 i, const DeserializeStream& stream);
        static void SynchronizeNewPrefabInstance(Context& context, PrefabSyncData& data, Prefab* prefab, Actor* actor, const UID& prefabObjectId);
    };
} // SE

