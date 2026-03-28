
#include "Prefab.h"
#include "PrefabManager.h"

#include "Core/Logging/Exception.h"
#include "Core/Logging/Exceptions/ArgumentNullException.h"
#include "Core/Profiler/ProfilerCPU.h"
#include "Core/Serialization/JsonTools.h"
#include "Core/Thread/Threading.h"
#include "Core/Types/DateTime.h"
#include "Runtime/Level/ActorsCache.h"
#include "Runtime/Level/SceneObjectsFactory.h"
#include "Runtime/Level/SceneQuery.h"
#include "Runtime/Resource/AssetsCache.h"
#include "Runtime/Resource/Importers/CreateJson.h"

namespace SE
{
    // Apply flow:
// - collect all prefabs using this prefab (load and create default instances)
// - serialize target actors (get actual changes including modifications and new objects or removed objects)
// - cache prefab instances state
// - create pure default instance and apply changes
// - save pure default instance
// - update prefab asset
// - sync prefab instances
// - sync nested prefabs

// Sync flow:
// - cache prefab instances state
// - create pure default instance and apply local prefab changes
// - save pure default instance
// - update prefab asset
// - sync prefab instances
// - sync nested prefabs

// Just typedef these monster things
using SceneObjectsLookupCacheType = CollectionPoolCache<ActorsCache::ActorsLookupType>::ScopeCache;
using SceneObjectsListCacheType = CollectionPoolCache<ActorsCache::SceneObjectsListType>::ScopeCache;
using ISerializeModifierCacheType = CollectionPoolCache<ISerializeModifier, SerializeModifierCache::ISerializeModifierClearCallback>::ScopeCache;
using IdToDataLookupType = Dictionary<UID, const DeserializeStream*>;

struct AutoActorCleanup
{
    Actor* Ptr;

    AutoActorCleanup(Actor* ptr)
    {
        Ptr = ptr;
    }

    ~AutoActorCleanup()
    {
        Ptr->DeleteObject();
    }
};

namespace
{
    Actor* FindActorWithPrefabObjectId(Actor* a, const UID& prefabObjectId)
    {
        if (a->GetPrefabObjectID() == prefabObjectId)
        {
            return a;
        }

        for (int i = 0; i < a->GetChildrenCount(); i++)
        {
            auto c = a->GetChild(i);
            auto r = FindActorWithPrefabObjectId(c, prefabObjectId);
            if (r)
            {
                return r;
            }
        }
        return nullptr;
    }
};

/// <summary>
/// The temporary data container for the prefab instance to restore its local changes after prefab synchronization.
/// </summary>
class PrefabInstanceData
{
public:
    // Don't copy or anything
    PrefabInstanceData()
    {
    }

    PrefabInstanceData(const PrefabInstanceData&)
    {
#if BUILD_DEBUG
        CRASH;
#endif
    }

    PrefabInstanceData& operator=(const PrefabInstanceData&)
    {
#if BUILD_DEBUG
        CRASH;
#endif
        return *this;
    }

    ~PrefabInstanceData()
    {
    }

public:
    /// <summary>
    /// The prefab instance root actor.
    /// </summary>
    Actor* TargetActor;

    /// <summary>
    /// The cached order in parent of the target actor. Used to preserve it after prefab changes synchronization.
    /// </summary>
    int32 OrderInParent;

    /// <summary>
    /// The serialized array of scene objects from the prefab instance (the first item is a root actor).
    /// </summary>
    Json::Document Data;

    /// <summary>
    /// The mapping from prefab instance object id to serialized objects array index (in Data).
    /// </summary>
    Dictionary<UID, int32> PrefabInstanceIdToDataIndex;

public:
    typedef List<PrefabInstanceData> PrefabInstancesData;

    /// <summary>
    /// Collects all the valid prefab instances to update on prefab data synchronization.
    /// </summary>
    /// <param name="prefabInstancesData">The prefab instances data (result buffer).</param>
    /// <param name="prefabId">The prefab asset identifier.</param>
    /// <param name="defaultInstance">The default instance (prefab internal, can be null).</param>
    /// <param name="targetActor">The target actor (optional actor to skip for counting, can be null).</param>
    static void CollectPrefabInstances(PrefabInstancesData& prefabInstancesData, const UID& prefabId, Actor* defaultInstance, Actor* targetActor);

    /// <summary>
    /// Serializes all the prefab instances local changes to restore on prefab data synchronization.
    /// </summary>
    /// <param name="prefabInstancesData">The prefab instances data.</param>
    /// <param name="tmpBuffer">The temporary json buffer (cleared before and after usage).</param>
    /// <param name="prefab">Source prefab.</param>
    static void SerializePrefabInstances(PrefabInstancesData& prefabInstancesData, Json::StringBuffer& tmpBuffer, const Prefab* prefab);

    /// <summary>
    /// Synchronizes the prefab instances by applying changes from the diff data and restoring the local changes captured by SerializePrefabInstances.
    /// </summary>
    /// <param name="prefabInstancesData">The prefab instances data.</param>
    /// <param name="defaultInstance">The new default instance of the prefab.</param>
    /// <param name="sceneObjects">The scene objects (temporary cache from defaultInstance).</param>
    /// <param name="prefabId">The prefab asset identifier.</param>
    /// <param name="prefabObjectIdToDiffData">The hash table that maps the prefab object id to json data for the given prefab object.</param>
    /// <param name="newPrefabObjectIds">The collection of the new prefab objects ids added to prefab during changes synchronization or modifications apply.</param>
    /// <returns>True if failed, otherwise false.</returns>
    static bool SynchronizePrefabInstances(PrefabInstancesData& prefabInstancesData, Actor* defaultInstance, SceneObjectsListCacheType& sceneObjects, const UID& prefabId, const IdToDataLookupType& prefabObjectIdToDiffData, const List<UID>& newPrefabObjectIds);

    /// <summary>
    /// Synchronizes the prefab instances by applying changes from the diff data and restoring the local changes captured by SerializePrefabInstances.
    /// </summary>
    /// <param name="prefabInstancesData">The prefab instances data.</param>
    /// <param name="defaultInstance">The new default instance of the prefab.</param>
    /// <param name="sceneObjects">The scene objects (temporary cache from defaultInstance).</param>
    /// <param name="prefabId">The prefab asset identifier.</param>
    /// <param name="tmpBuffer">The temporary json buffer (cleared before and after usage).</param>
    /// <param name="oldObjectsIds">Collection with ids of the objects (actors and scripts) from the prefab before changes apply. Used to find new objects or old objects and use this information during changes sync (eg. generate ids for the new objects to prevent ids collisions).</param>
    /// <param name="newObjectIds">Collection with ids of the objects (actors and scripts) from the prefab after changes apply. Used to find new objects or old objects and use this information during changes sync (eg. generate ids for the new objects to prevent ids collisions).</param>
    /// <returns>True if failed, otherwise false.</returns>
    static bool SynchronizePrefabInstances(PrefabInstancesData& prefabInstancesData, Actor* defaultInstance, SceneObjectsListCacheType& sceneObjects, const UID& prefabId, Json::StringBuffer& tmpBuffer, const List<UID>& oldObjectsIds, const List<UID>& newObjectIds);

    static void DeletePrefabObject(SceneObject* obj)
    {
        obj->SetParent(nullptr);
        obj->DeleteObject();
    }
};

void PrefabInstanceData::CollectPrefabInstances(PrefabInstancesData& prefabInstancesData, const UID& prefabId, Actor* defaultInstance, Actor* targetActor)
{
    Threading::ScopeLock lock(PrefabManager::PrefabsReferencesLocker);
    if (PrefabManager::PrefabsReferences.ContainsKey(prefabId))
    {
        auto& instances = PrefabManager::PrefabsReferences[prefabId];
        int32 usedCount = 0;
        for (int32 instanceIndex = 0; instanceIndex < instances.Count(); instanceIndex++)
        {
            const auto instance = instances[instanceIndex];
            /*if (EnumHasAnyFlags(instance->Flags, ObjectFlags::WasMarkedToDelete))
                continue;*/
            if (instance != defaultInstance && targetActor != instance/* && !targetActor->HasActorInHierarchy(instance)*/)
                usedCount++;
        }
        prefabInstancesData.Resize(usedCount);
        int32 dataIndex = 0;
        for (int32 instanceIndex = 0; instanceIndex < instances.Count(); instanceIndex++)
        {
            // Skip default instance because it will be recreated, skip input actor because it needs just to be linked
            Actor* instance = instances[instanceIndex];
            /*if (EnumHasAnyFlags(instance->Flags, ObjectFlags::WasMarkedToDelete))
                continue;*/
            if (instance != defaultInstance && targetActor != instance/* && !targetActor->GetHasActorInHierarchy(instance)*/)
            {
                auto& data = prefabInstancesData[dataIndex++];
                data.TargetActor = instance;
                data.OrderInParent = instance->GetOrderInParent();
            }
        }
    }
}

void PrefabInstanceData::SerializePrefabInstances(PrefabInstancesData& prefabInstancesData, Json::StringBuffer& tmpBuffer, const Prefab* prefab)
{
    if (prefabInstancesData.IsEmpty())
        return;
    CollectionPoolCache<ActorsCache::SceneObjectsListType>::ScopeCache sceneObjects = ActorsCache::SceneObjectsListCache.Get();
    sceneObjects->EnsureCapacity(prefab->ObjectsCount * 4);
    for (int32 dataIndex = 0; dataIndex < prefabInstancesData.Count(); dataIndex++)
    {
        auto& instance = prefabInstancesData[dataIndex];

        // Get scene objects in the prefab instance
        sceneObjects->Clear();
        SceneQuery::GetAllSerializableSceneObjects(instance.TargetActor, *sceneObjects.Value);

        // TODO: could be optimized by doing serialization and changes restore only for scene objects with a prefab linkage to this prefab

        // Serialize
        tmpBuffer.Clear();
        CompactJsonWriter writerObj(tmpBuffer);
        JsonWriter& writer = writerObj;
        writer.StartArray();
        SerializeContext serializeContext(writer);
        for (int32 i = 0; i < sceneObjects->Count(); i++)
        {
            SceneObject* obj = sceneObjects.Value->At(i);
            Serialization::Serialize(serializeContext, obj);
        }
        writer.EndArray();

        // Parse json to get DOM
        {
            PROFILE_CPU_NAMED("Json.Parse");
            instance.Data.Parse(tmpBuffer.GetString(), tmpBuffer.GetSize());
        }
        if (instance.Data.HasParseError())
        {
            LOG_WARNING("Level", "Failed to parse serialized scene objects data.");
            continue;
        }

        // Build acceleration table
        instance.PrefabInstanceIdToDataIndex.EnsureCapacity(sceneObjects->Count() * 4);
        for (int32 i = 0; i < sceneObjects->Count(); i++)
        {
            SceneObject* obj = sceneObjects.Value->At(i);
            instance.PrefabInstanceIdToDataIndex[obj->GetSceneObjectId()] = i;
        }
    }
    tmpBuffer.Clear();
}

bool PrefabInstanceData::SynchronizePrefabInstances(PrefabInstancesData& prefabInstancesData, Actor* defaultInstance, SceneObjectsListCacheType& sceneObjects, const UID& prefabId, const IdToDataLookupType& prefabObjectIdToDiffData, const List<UID>& newPrefabObjectIds)
{
    for (int32 instanceIndex = 0; instanceIndex < prefabInstancesData.Count(); instanceIndex++)
    {
        auto& instance = prefabInstancesData[instanceIndex];
        ISerializeModifierCacheType modifier = SerializeModifierCache::Modifier.Get();
        // Scripting::ObjectsLookupIdMapping.Set(&modifier.Value->IdsMapping);

        // If prefab object root was changed during changes apply then update the TargetActor to point a valid object
        Actor* oldTargetActor = instance.TargetActor;
        if (!oldTargetActor/* || EnumHasAnyFlags(oldTargetActor->Flags, ObjectFlags::WasMarkedToDelete)*/)
            continue;
        Actor* newTargetActor = FindActorWithPrefabObjectId(instance.TargetActor, defaultInstance->GetInstanceID());
        if (!newTargetActor)
        {
            LOG_ERROR("Level", "Missing root object {0} for prefab instance {1}", defaultInstance->ToString(), oldTargetActor->ToString());
        }
        else if (oldTargetActor != newTargetActor)
        {
            LOG_INFO("Level", "Changing root object of prefab instance from {0} to {1}", oldTargetActor->ToString(), newTargetActor->ToString());
            newTargetActor->SetParent(oldTargetActor->GetParent(), true, false);
            oldTargetActor->SetParent(newTargetActor, true, false);
            instance.TargetActor = newTargetActor;
        }

        // Get scene objects in the prefab instance
        sceneObjects->Clear();
        SceneQuery::GetAllSerializableSceneObjects(instance.TargetActor, *sceneObjects.Value);

        int32 existingObjectsCount = sceneObjects->Count();
        modifier->IdsMapping.EnsureCapacity((existingObjectsCount + newPrefabObjectIds.Count()) * 4);

        // Map prefab objects to the prefab instance objects
        for (int32 i = 0; i < existingObjectsCount; i++)
        {
            SceneObject* obj = sceneObjects.Value->At(i);
            if (obj->HasPrefabLink())
            {
                // Special case for nested prefabs if one of the objects in nested prefab gets reparented then prefab using it gets duplicated objects due to waterfall synchronization
                if (modifier.Value->IdsMapping.ContainsKey(obj->GetPrefabObjectID()))
                {
                    // Remove object
                    LOG_INFO("Level", "Removing object {0} from instance {1} (prefab: {2})", obj->GetSceneObjectId(), instance.TargetActor->ToString(), prefabId);
                    DeletePrefabObject(obj);
                    sceneObjects.Value->RemoveAtKeepOrder(i);
                    existingObjectsCount--;
                    i--;
                    continue;
                }

                modifier.Value->IdsMapping[obj->GetPrefabObjectID()] = obj->GetSceneObjectId();
            }
        }

        // Generate new IDs for the added objects (objects in prefab has to have a unique Ids, other than the targetActor instance objects to prevent Id collisions)
        for (int32 i = 0; i < newPrefabObjectIds.Count(); i++)
            modifier->IdsMapping[newPrefabObjectIds[i]] = UID::New();

        // Create new objects added to prefab
        int32 deserializeSceneObjectIndex = sceneObjects->Count();
        SceneObjectsFactory::Context context(modifier.Value);
        for (int32 i = 0; i < newPrefabObjectIds.Count(); i++)
        {
            const UID prefabObjectId = newPrefabObjectIds[i];
            const DeserializeStream* data;
            if (!prefabObjectIdToDiffData.TryGet(prefabObjectId, data))
            {
                LOG_WARNING("Level", "Missing object linkage to the prefab object diff data.");
                continue;
            }

            SceneObject* obj = SceneObjectsFactory::Spawn(context, *data);
            if (!obj)
                continue;
            // obj->RegisterObject();
            sceneObjects->Add(obj);
        }

        // Apply modifications
        for (int32 i = existingObjectsCount - 1; i >= 0; i--)
        {
            SceneObject* obj = sceneObjects->At(i);

            if (obj->HasPrefabLink() && obj->GetPrefabID() == prefabId)
            {
                const DeserializeStream* data;
                if (prefabObjectIdToDiffData.TryGet(obj->GetPrefabObjectID(), data))
                {
                    // Apply prefab changes
                    DeserializeContext deserializeContext(*(DeserializeStream*)data, modifier.Value);
                    obj->Deserialize(deserializeContext);
                }
                else
                {
                    // Remove object removed from the prefab
                    LOG_INFO("Level", "Removing prefab instance object {0} from instance {1} (prefab object: {2}, prefab: {3})", obj->GetSceneObjectId(), instance.TargetActor->ToString(), obj->GetPrefabObjectID(), prefabId);
                    DeletePrefabObject(obj);
                    sceneObjects.Value->RemoveAtKeepOrder(i);
                    deserializeSceneObjectIndex--;
                    existingObjectsCount--;
                }
            }
        }

        // Deserialize new objects added to prefab
        for (int32 i = 0; i < newPrefabObjectIds.Count(); i++)
        {
            const UID prefabObjectId = newPrefabObjectIds[i];
            const DeserializeStream* data;
            if (!prefabObjectIdToDiffData.TryGet(prefabObjectId, data))
                continue;

            SceneObject* obj = sceneObjects->At(deserializeSceneObjectIndex);
            SceneObjectsFactory::Deserialize(context, obj, *(DeserializeStream*)data);

            // Link new prefab instance to prefab and prefab object
            obj->LinkPrefab(prefabId, prefabObjectId);

            deserializeSceneObjectIndex++;
        }

        // ObjectsRemovalService::Flush();

        // Restore local changes (for the existing scene objects)
        for (int32 i = 0; i < sceneObjects->Count(); i++)
        {
            SceneObject* obj = sceneObjects->At(i);

            int32 dataIndex;
            if (instance.PrefabInstanceIdToDataIndex.TryGet(obj->GetSceneObjectId(), dataIndex))
            {
                auto& data = instance.Data[dataIndex];
#if 0
                if (oldTargetActor != newTargetActor && (obj == oldTargetActor || obj == newTargetActor))
                {
                    // Prevent from changing parent of the new/old instance roots when changing prefab root instance (already did it)
                    data.RemoveMember("ParentID");
                }
#else
                // Preserve hierarchy (values from prefab are used)
                data.RemoveMember("ParentID");
#endif
                DeserializeContext deserializeContext(data, modifier.Value);
                obj->Deserialize(deserializeContext);

                // Preserve order in parent (values from prefab are used)
                if (obj != instance.TargetActor)
                {
                    auto prefab = AssetContent::Load<Prefab>(prefabId);
                    const auto defaultInstance = prefab ? prefab->GetDefaultInstance(obj->GetPrefabObjectID()) : nullptr;
                    if (defaultInstance)
                    {
                        obj->SetOrderInParent(defaultInstance->GetOrderInParent());
                    }
                }
            }
        }

        // Scripting::ObjectsLookupIdMapping.Set(nullptr);

        // Setup new objects after deserialization
        for (int32 i = existingObjectsCount; i < sceneObjects->Count(); i++)
        {
            SceneObject* obj = sceneObjects.Value->At(i);
            obj->Initialize();
        }

        // Synchronize existing objects logic with deserialized state (fire events)
        for (int32 i = 0; i < existingObjectsCount; i++)
        {
            SceneObject* obj = sceneObjects->At(i);
            Actor* actor = dynamic_cast<Actor*>(obj);
            if (actor)
            {
                const bool shouldBeActiveInHierarchy = actor->GetIsActive() && (!actor->GetParent() || actor->GetParent()->IsActiveInHierarchy());
                if (shouldBeActiveInHierarchy != actor->IsActiveInHierarchy())
                {
                    actor->m_IsActiveInHierarchy = shouldBeActiveInHierarchy;
                    actor->OnActiveInTreeChanged();
                    Level::CallActorEvent(Level::ActorEventType::OnActorActiveChanged, actor, nullptr);
                }
                Level::CallActorEvent(Level::ActorEventType::OnActorNameChanged, actor, nullptr);
                Level::CallActorEvent(Level::ActorEventType::OnActorOrderInParentChanged, actor, nullptr);
                if (!actor->IsDuringPlay() && actor->GetParent())
                    Level::CallActorEvent(Level::ActorEventType::OnActorParentChanged, actor, actor->GetParent());
            }
        }

        // Restore order in parent
        instance.TargetActor->SetOrderInParent(instance.OrderInParent);

        // Update transformations
        instance.TargetActor->OnTransformChanged();

        // Spawn new objects (add to gameplay)
        {
            SceneBeginData beginData;
            for (int32 i = existingObjectsCount; i < sceneObjects->Count(); i++)
            {
                SceneObject* obj = sceneObjects.Value->At(i);
                if (!obj->IsDuringPlay() && sceneObjects->Find(obj->GetParent()) < i)
                {
                    obj->BeginPlay(&beginData);

                    /*if (Script* script = dynamic_cast<Script*>(obj))
                    {
                        if (script->GetParent() && !script->_wasEnableCalled && script->GetParent()->IsActiveInHierarchy() && script->GetParent()->GetScene())
                            script->Enable();
                    }*/
                }
            }
            beginData.OnDone();
            for (int32 i = existingObjectsCount; i < sceneObjects->Count(); i++)
            {
                Actor* actor = dynamic_cast<Actor*>(sceneObjects.Value->At(i));
                if (actor)
                    Level::CallActorEvent(Level::ActorEventType::OnActorSpawned, actor, nullptr);
            }
        }
    }
    
    LOG_INFO("Level", "Prefab synced! ({0} instances)", prefabInstancesData.Count());

    return false;
}

bool PrefabInstanceData::SynchronizePrefabInstances(PrefabInstancesData& prefabInstancesData, Actor* defaultInstance, SceneObjectsListCacheType& sceneObjects, const UID& prefabId, Json::StringBuffer& tmpBuffer, const List<UID>& oldObjectsIds, const List<UID>& newObjectIds)
{
    if (prefabInstancesData.IsEmpty())
        return false;

    // Fully serialize default instance scene objects (accumulate all prefab and nested prefabs changes into a single linear list of objects)
    Json::Document defaultInstanceData;
    {
        // Serialize
        tmpBuffer.Clear();
        CompactJsonWriter writerObj(tmpBuffer);
        JsonWriter& writer = writerObj;
        writer.StartArray();
        for (int32 i = 0; i < sceneObjects->Count(); i++)
        {
            SceneObject* obj = sceneObjects.Value->At(i);

            // Full serialization - no prefab diff, always all non-default properties
            writer.StartObject();
            SerializeContext serializeContext(writer);
            obj->Serialize(serializeContext);
            writer.EndObject();
        }
        writer.EndArray();

        // Parse json to get DOM
        {
            PROFILE_CPU_NAMED("Json.Parse");
            defaultInstanceData.Parse(tmpBuffer.GetString(), tmpBuffer.GetSize());
        }
        if (defaultInstanceData.HasParseError())
        {
            LOG_WARNING("Level", "Failed to parse serialized scene objects data.");
            return true;
        }
    }

    // Find new objects
    List<UID> newPrefabObjectIds;
    newPrefabObjectIds.EnsureCapacity(Math::Max(32, Math::Abs(newObjectIds.Count() - oldObjectsIds.Count()) * 4));
    for (int32 i = 0; i < newObjectIds.Count(); i++)
    {
        const UID id = newObjectIds[i];
        if (!oldObjectsIds.Contains(id))
        {
            newPrefabObjectIds.Add(id);
        }
    }

#if 0
    // Strip unwanted properties from the default instance (extract only diff to apply)
    {
        const auto array = defaultInstanceData.GetArray();
        int32 i = 0;
        for (auto it = array.Begin(); it != array.End(); ++it, i++)
        {
            auto data = it->GetObject();

            // Skip for added new objects (need to link them and get all of their data)
            SceneObject* obj = sceneObjects.Value->At(i);
            if (newPrefabObjectIds.Contains(obj->GetSceneObjectId()))
                continue;

            // Strip unwanted data
            data.RemoveMember("ParentID");
        }
    }
#endif

    // Build cache data
    IdToDataLookupType prefabObjectIdToDiffData;
    prefabObjectIdToDiffData.EnsureCapacity(defaultInstanceData.Size() * 3);
    for (int32 i = 0; i < sceneObjects->Count(); i++)
    {
        SceneObject* obj = sceneObjects.Value->At(i);
        prefabObjectIdToDiffData.Add(obj->GetSceneObjectId(), &defaultInstanceData[i]);
    }

    // Process prefab instances to synchronize changes
    return SynchronizePrefabInstances(prefabInstancesData, defaultInstance, sceneObjects, prefabId, prefabObjectIdToDiffData, newPrefabObjectIds);
}



    
    bool FindCyclicReferences(Actor* actor, const UID& prefabRootId)
    {
        for (int32 i = 0; i < actor->GetChildrenCount(); i++)
        {
            const auto child = actor->GetChild(i);

            if (child->GetPrefabObjectID() == prefabRootId || FindCyclicReferences(child, prefabRootId))
                return true;
        }

        return false;
    }

    bool Prefab::ApplyAll(Actor* targetActor)
    {
        PROFILE_CPU();
        const auto startTime = DateTime::NowUTC();

        // Perform validation
        if (!IsLoaded())
        {
            Log::Exception(SE_TEXT("Cannot apply changes on not loaded prefab asset."));
            return true;
        }
        if (targetActor == nullptr)
        {
            Log::ArgumentNullException();
            return true;
        }
        if (targetActor->GetPrefabID() != GetID())
        {
            Log::Exception(SE_TEXT("Cannot apply changes to the prefab. Prefab instance root object has link to the other prefab."));
            return true;
        }
        if (GetDefaultInstance() == nullptr)
        {
            LOG_WARNING("Level", "Failed to create default prefab instance for the prefab asset.");
            return true;
        }
        if (targetActor->GetPrefabObjectID() != GetRootObjectId())
        {
            LOG_WARNING("Level", "Applying prefab changes with modified root object. Root object id: {0}, new root: {1} (prefab object id: {2})", GetRootObjectId().ToString(), targetActor->ToString(), targetActor->GetPrefabObjectID());
            SceneObject* newRootDefault = GetDefaultInstance(targetActor->GetPrefabObjectID());
            const DeserializeStream** newRootDataPtr = ObjectsDataCache.TryGet(targetActor->GetPrefabObjectID());
            if (!newRootDefault || !newRootDataPtr || !*newRootDataPtr)
            {
                LOG_ERROR("Level", "Cannot change the prefab root object to the actor that is not yet added to the prefab.");
                return true;
            }
            const DeserializeStream& newRootData = **newRootDataPtr;
            UID prefabId, prefabObjectID;
            if (JsonTools::GetGuidIfValid(prefabId, newRootData, "PrefabID") && JsonTools::GetGuidIfValid(prefabObjectID, newRootData, "PrefabObjectID"))
            {
                const auto nestedPrefab = AssetContent::Load<Prefab>(prefabId);
                if (nestedPrefab && nestedPrefab->GetRootObjectId() != prefabObjectID)
                {
                    LOG_ERROR("Level", "Cannot change the prefab root object is from other nested prefab (excluding root of that nested prefab prefab).");
                    return true;
                }
            }
        }
        if (!Threading::IsMainThread())
        {
            // Prefabs cannot be updated on async thread so sync it with a Main Thread
            bool result = true;
            Function<void()> action = [&]
            {
                result = ApplyAll(targetActor);
            };
            const auto task = Threading::Task::StartNew(New<Threading::MainThreadActionTask>(action));
            if (task->Wait(TimeSpan::FromSeconds(10)))
                result = true;
            return result;
        }

        // Prevent cyclic references
        {
            PROFILE_CPU_NAMED("Prefab.FindCyclicReferences");

            ENGINE_ASSERT(GetDefaultInstance() != nullptr);
            if (FindCyclicReferences(targetActor, targetActor->GetPrefabObjectID()))
            {
                Log::Exception(SE_TEXT("Cannot apply changes to the prefab. Cyclic reference found in the actor."));
                return true;
            }
        }

        // Collect all prefabs that use this prefab, load them and create default instance for each prefab
        // To apply changes in a proper way the default instance is required to preserve the local modification applied to the nested prefab
        NestedPrefabsList allPrefabs;
        {
            PROFILE_CPU_NAMED("Prefab.CollectNestedPrefabs");

            // Get all prefab assets ids from project
            List<UID> nestedPrefabIds;
            AssetContent::GetRegistry()->GetAllByTypeName(Typeof<Prefab>(), nestedPrefabIds);

            // Assign references to the prefabs
            allPrefabs.EnsureCapacity(Math::NextPowerOfTwo(Math::Max(30, nestedPrefabIds.Count())));
            const Dictionary<UID, Asset*, HeapAllocation>& assetsRaw = AssetContent::GetAssetsRaw();
            for (auto& e : assetsRaw)
            {
                if (e.Value->GetType() == Typeof<Prefab>())
                    nestedPrefabIds.AddUnique(e.Key);
            }
            for (int32 i = 0; i < nestedPrefabIds.Count(); i++)
            {
                const auto nestedPrefab = AssetContent::LoadAsync<Prefab>(nestedPrefabIds[i]);
                if (nestedPrefab && nestedPrefab != this/* && (nestedPrefab->Flags & ObjectFlags::WasMarkedToDelete) == ObjectFlags::None*/)
                {
                    allPrefabs.Add(nestedPrefab);
                }
            }

            // Setup default instances (skip invalid prefabs)
            for (int32 i = allPrefabs.Count() - 1; i >= 0; i--)
            {
                Prefab* prefab = allPrefabs[i];
                if (prefab->WaitForLoaded() || prefab->GetDefaultInstance() == nullptr)
                    allPrefabs.RemoveAt(i);
            }
        }

        // ObjectsRemovalService::Flush();

        // Collect existing prefab instances (this and nested ones) to cache 'before' state used later to restore it
        PrefabInstancesData thisPrefabInstancesData;
        List<PrefabInstancesData> allPrefabsInstancesData;
        {
            PROFILE_CPU_NAMED("Prefab.CachePrefabInstancesData");

            Json::StringBuffer dataBuffer;
            PrefabInstanceData::CollectPrefabInstances(thisPrefabInstancesData, GetID(), _defaultInstance, targetActor);
            PrefabInstanceData::SerializePrefabInstances(thisPrefabInstancesData, dataBuffer, this);

            allPrefabsInstancesData.Resize(allPrefabs.Count());
            for (int32 i = 0; i < allPrefabs.Count(); i++)
            {
                Prefab* prefab = allPrefabs[i];
                PrefabInstanceData::CollectPrefabInstances(allPrefabsInstancesData[i], prefab->GetID(), prefab->GetDefaultInstance(), prefab->GetDefaultInstance());
                PrefabInstanceData::SerializePrefabInstances(allPrefabsInstancesData[i], dataBuffer, prefab);
            }
        }

        // Use internal call to improve shared collections memory sharing
        if (ApplyAllInternal(targetActor, true, thisPrefabInstancesData))
            return true;

        SyncNestedPrefabs(allPrefabs, allPrefabsInstancesData);

        const auto endTime = DateTime::NowUTC();
        LOG_INFO("Level", "Prefab updated! {0} ms", (int32)(endTime - startTime).GetTotalMilliseconds());
        return false;
    }

    bool Prefab::ApplyAllInternal(Actor* targetActor, bool linkTargetActorObjectToPrefab, PrefabInstancesData& prefabInstancesData)
    {
        PROFILE_CPU_NAMED("Prefab.Apply");
        Threading::ScopeLock lock(Locker);
        const auto prefabId = GetID();

        // Gather all scene objects in target instance (reused later)
        CollectionPoolCache<ActorsCache::SceneObjectsListType>::ScopeCache targetObjects = ActorsCache::SceneObjectsListCache.Get();
        targetObjects->EnsureCapacity(ObjectsCount * 4);
        SceneQuery::GetAllSerializableSceneObjects(targetActor, *targetObjects.Value);
        if (PrefabManager::FilterPrefabInstancesToSave(prefabId, *targetObjects.Value))
            return true;
        LOG_INFO("Level", "Applying prefab changes from actor {0} (total objects count: {2}) to {1}...", targetActor->ToString(), ToString(), targetObjects->Count());
        List<UID> oldObjectsIds = ObjectsIds;

        // Serialize to json data
        Json::StringBuffer dataBuffer;
        {
            CompactJsonWriter writerObj(dataBuffer);
            JsonWriter& writer = writerObj;
            writer.StartArray();
            SerializeContext serializeContext(writer);
            for (int32 i = 0; i < targetObjects->Count(); i++)
            {
                SceneObject* obj = targetObjects.Value->At(i);
                Serialization::Serialize(serializeContext, obj);
            }
            writer.EndArray();
        }

        // Parse json document and modify serialized data to extract only modified properties
        Json::Document diffDataDocument;
        Dictionary<UID, int32> diffPrefabObjectIdToDataIndex; // Maps Prefab Object Id -> Actor Data index in diffDataDocument json array (for actors/scripts to modify prefab)
        Dictionary<UID, int32> newPrefabInstanceIdToDataIndex; // Maps Prefab Instance Id -> Actor Data index in diffDataDocument json array (for new actors/scripts to add to prefab), maps to -1 for scripts
        diffPrefabObjectIdToDataIndex.EnsureCapacity(ObjectsCount * 4);
        newPrefabInstanceIdToDataIndex.EnsureCapacity(ObjectsCount * 4);
        {
            // Parse json to DOM document
            {
                PROFILE_CPU_NAMED("Json.Parse");
                diffDataDocument.Parse(dataBuffer.GetString(), dataBuffer.GetSize());
            }
            if (diffDataDocument.HasParseError())
            {
                LOG_WARNING("Level", "Failed to parse serialized scene objects data.");
                return true;
            }

            // Process json
            const auto array = diffDataDocument.GetArray();
            int32 i = 0;
            for (auto it = array.Begin(); it != array.End(); ++it, i++)
            {
                SceneObject* obj = targetObjects.Value->At(i);
                auto data = it->GetObject();

                // Check if object is from that prefab
                if (obj->GetPrefabID() == prefabId)
                {
                    if (!obj->GetPrefabObjectID().IsValid())
                    {
                        LOG_WARNING("Level", "One of the target instance objects has missing link to prefab object.");
                        return true;
                    }
                    if (!ObjectsIds.Contains(obj->GetPrefabObjectID()))
                    {
                        LOG_WARNING("Level", "One of the target instance objects has link to prefab object that does not exist in prefab.");
                        return true;
                    }

                    // Cache connection for fast lookup
                    diffPrefabObjectIdToDataIndex[obj->GetPrefabObjectID()] = i;

                    // Strip unwanted data
                    data.RemoveMember("ID");
                    data.RemoveMember("PrefabID");
                    data.RemoveMember("PrefabObjectID");
                }
                else
                {
                    // Object if a new thing
                    newPrefabInstanceIdToDataIndex[obj->GetSceneObjectId()] = i;
                }
            }

            // Change object ids to match the prefab objects ids (helps with linking references in scripts)
            Dictionary<UID, UID> objectInstanceIdToPrefabObjectId;
            objectInstanceIdToPrefabObjectId.EnsureCapacity(ObjectsCount * 3);
            i = 0;
            for (auto it = array.Begin(); it != array.End(); ++it, i++)
            {
                SceneObject* obj = targetObjects.Value->At(i);
                auto data = it->GetObject();
                if (obj->GetPrefabID() == prefabId)
                {
                    objectInstanceIdToPrefabObjectId.Add(obj->GetSceneObjectId(), obj->GetPrefabObjectID());
                }
            }
            // TODO: what if user applied prefab with references to the other objects from scene? clear them or what?
            JsonTools::ChangeIds(diffDataDocument, objectInstanceIdToPrefabObjectId);
        }
        dataBuffer.Clear();
        CollectionPoolCache<ActorsCache::SceneObjectsListType>::ScopeCache sceneObjects = ActorsCache::SceneObjectsListCache.Get();

        // Destroy default instance and some cache data in Prefab
        DeleteDefaultInstance();

        // Create default instance of the prefab (but without a link to this prefab) and apply modifications during deserialization
        Actor* defaultInstance;
        Dictionary<UID, UID> newPrefabInstanceIdToPrefabObjectId; // Maps Prefab Instance Id -> Prefab Object Id (for new actors/scripts to add to prefab), value is a new generated Id of the prefab object for prefab links usage
        {
            // Prepare
            sceneObjects->EnsureCapacity(diffPrefabObjectIdToDataIndex.Count() + newPrefabInstanceIdToDataIndex.Count());
            ISerializeModifierCacheType modifier = SerializeModifierCache::Modifier.Get();
            // Scripting::ObjectsLookupIdMapping.Set(&modifier.Value->IdsMapping);

            // Generate new IDs for the added objects (objects in prefab has to have a unique Ids, other than the targetActor instance objects to prevent Id collisions)
            newPrefabInstanceIdToPrefabObjectId.EnsureCapacity(newPrefabInstanceIdToDataIndex.Count() * 4);
            for (auto i = newPrefabInstanceIdToDataIndex.begin(); i.IsNotEnd(); ++i)
            {
                const auto prefabObjectId = UID::New();
                newPrefabInstanceIdToPrefabObjectId[i->Key] = prefabObjectId;
                modifier->IdsMapping[i->Key] = prefabObjectId;
            }

            // Add inverse IDs mapping to link added objects and references inside them to the prefab objects
            for (int32 i = 0; i < targetObjects->Count(); i++)
            {
                const SceneObject* obj = targetObjects->At(i);

                // Check if object is from that prefab
                if (obj->GetPrefabID() == prefabId)
                {
                    // Map prefab instance to existing prefab object
                    modifier->IdsMapping.Add(obj->GetSceneObjectId(), obj->GetPrefabObjectID());
                }
                else
                {
                    // Map prefab instance to new prefab object
                    // <already added via newPrefabInstanceIdToDataIndex>
                }
            }

            // Create prefab objects
            auto& data = *Data;
            sceneObjects->Resize(ObjectsCount + newPrefabInstanceIdToDataIndex.Count());
            SceneObjectsFactory::Context context(modifier.Value);
            for (int32 i = 0; i < ObjectsCount; i++)
            {
                SceneObject* obj = SceneObjectsFactory::Spawn(context, data[i]);
                sceneObjects->At(i) = obj;
                if (!obj)
                {
                    // This may happen if nested prefab has missing or invalid object but it still exists in this prefab (need to skip it)
                    SceneObjectsFactory::HandleObjectDeserializationError(data[i]);
                    continue;
                }
                // obj->RegisterObject();
            }

            // Create new prefab objects
            int32 newPrefabInstanceIdToDataIndexCounter = 0;
            int32 newPrefabInstanceIdToDataIndexStart = ObjectsCount;
            for (auto i = newPrefabInstanceIdToDataIndex.begin(); i.IsNotEnd(); ++i)
            {
                const int32 dataIndex = i->Value;
                SceneObject* obj = SceneObjectsFactory::Spawn(context, diffDataDocument[dataIndex]);
                sceneObjects->At(newPrefabInstanceIdToDataIndexStart + newPrefabInstanceIdToDataIndexCounter++) = obj;
                if (!obj)
                {
                    // This should not happen but who knows
                    SceneObjectsFactory::HandleObjectDeserializationError(diffDataDocument[dataIndex]);
                    continue;
                }
                // obj->RegisterObject();
            }

            // Deserialize prefab objects and apply modifications
            for (int32 i = 0; i < ObjectsCount; i++)
            {
                SceneObject* obj = sceneObjects->At(i);
                if (!obj)
                    continue;
                SceneObjectsFactory::Deserialize(context, obj, data[i]);

                int32 dataIndex;
                if (diffPrefabObjectIdToDataIndex.TryGet(obj->GetSceneObjectId(), dataIndex))
                {
                    DeserializeContext deserializeContext(diffDataDocument[dataIndex], modifier.Value);
                    obj->Deserialize(deserializeContext);

                    // Synchronize order of the scene objects with the serialized data (eg. user reordered actors in prefab editor and applied changes)
                    if (i != 0)
                    {
                        for (int32 j = 0; j < targetObjects->Count(); j++)
                        {
                            SceneObject* targetObject = targetObjects->At(j);
                            if (targetObject->GetPrefabObjectID() == obj->GetInstanceID())
                            {
                                obj->SetOrderInParent(targetObject->GetOrderInParent());
                                break;
                            }
                        }
                    }
                }
                else
                {
                    // Remove object removed from the prefab
                    LOG_INFO("Level", "Removing object {0} from prefab default instance", obj->GetSceneObjectId());
                    PrefabInstanceData::DeletePrefabObject(obj);
                    sceneObjects->At(i) = nullptr;
                }
            }

            // Deserialize new prefab objects
            newPrefabInstanceIdToDataIndexCounter = 0;
            for (auto i = newPrefabInstanceIdToDataIndex.begin(); i.IsNotEnd(); ++i)
            {
                const int32 dataIndex = i->Value;
                SceneObject* obj = sceneObjects->At(newPrefabInstanceIdToDataIndexStart + newPrefabInstanceIdToDataIndexCounter++);
                if (!obj)
                    continue;
                SceneObjectsFactory::Deserialize(context, obj, diffDataDocument[dataIndex]);
            }
            for (int32 j = 0; j < targetObjects->Count(); j++)
            {
                auto obj = targetObjects->At(j);
                UID prefabObjectId;
                if (newPrefabInstanceIdToPrefabObjectId.TryGet(obj->GetSceneObjectId(), prefabObjectId))
                {
                    newPrefabInstanceIdToDataIndexCounter = 0;
                    for (auto i = newPrefabInstanceIdToDataIndex.begin(); i.IsNotEnd(); ++i)
                    {
                        SceneObject* e = sceneObjects->At(newPrefabInstanceIdToDataIndexStart + newPrefabInstanceIdToDataIndexCounter++);
                        if (e->GetInstanceID() == prefabObjectId)
                        {
                            // Synchronize order of new objects with the order in target instance
                            e->SetOrderInParent(obj->GetOrderInParent());
                            break;
                        }
                    }
                }
            }
            for (int32 i = 0; i < sceneObjects->Count(); i++)
            {
                if (sceneObjects->At(i) == nullptr)
                    sceneObjects->RemoveAtKeepOrder(i);
            }

            // Scripting::ObjectsLookupIdMapping.Set(nullptr);
            if (sceneObjects.Value->IsEmpty())
            {
                LOG_WARNING("Level", "No valid objects in prefab.");
                return true;
            }

            // Find the prefab root object (the root is usually serialized first)
            auto root = dynamic_cast<Actor*>(sceneObjects.Value->At(0));
            if (root && root->GetParent())
            {
                // When changing prefab root the target actor is a new root so try to find it in the objects
                int32 targetActorIdx = oldObjectsIds.Find(targetActor->GetPrefabObjectID());
                if (targetActorIdx > 0 && targetActorIdx < sceneObjects.Value->Count() && dynamic_cast<Actor*>(sceneObjects.Value->At(targetActorIdx)))
                {
                    root = dynamic_cast<Actor*>(sceneObjects.Value->At(targetActorIdx));
                }

                // Try using the first actor without a parent as a new ro0t
                for (int32 i = 1; i < sceneObjects->Count(); i++)
                {
                    SceneObject* obj = sceneObjects.Value->At(i);
                    auto actor = dynamic_cast<Actor*>(obj);
                    if (actor && !actor->GetParent())
                    {
                        root = actor;
                        break;
                    }
                }

                // Keep root unlinked
                if (root->GetParent())
                {
                    root->GetParent()->Children.Remove(root);
                    root->SetParent(nullptr);
                }
            }
            if (!root)
            {
                LOG_WARNING("Level", "No valid objects in prefab.");
                return true;
            }

            // Link objects hierarchy
            for (int32 i = 0; i < sceneObjects->Count(); i++)
            {
                auto obj = sceneObjects.Value->At(i);
                if (obj)
                    obj->Initialize();
            }

            // Update transformations
            root->OnTransformChanged();

            defaultInstance = root;
        }

        // Ensure to delete the spawned default instance with diff applied
        AutoActorCleanup cleanupDefaultInstance(defaultInstance);

        // Gather all default instance actors
        sceneObjects->Clear();
        SceneQuery::GetAllSerializableSceneObjects(defaultInstance, *sceneObjects.Value);

        // Refresh asset data
        if (UpdateInternal(*sceneObjects.Value, dataBuffer))
            return true;

        // Refresh all prefab instances (using the cached data)
        LOG_INFO("Level", "Reloading prefab instances");
        if (PrefabInstanceData::SynchronizePrefabInstances(prefabInstancesData, defaultInstance, sceneObjects, prefabId, dataBuffer, oldObjectsIds, ObjectsIds))
            return true;

        // Link the input objects to the prefab objects (prefab and instance are even, only links for the new objects added to prefab are required)
        if (linkTargetActorObjectToPrefab)
        {
            for (int32 i = 0; i < targetObjects->Count(); i++)
            {
                auto obj = targetObjects->At(i);

                if (obj->GetPrefabID() != prefabId)
                {
                    UID prefabObjectId;
                    if (!newPrefabInstanceIdToPrefabObjectId.TryGet(obj->GetSceneObjectId(), prefabObjectId))
                    {
                        LOG_WARNING("Level", "Missing prefab object linkage in 'NewPrefabInstanceIdToPrefabObjectId' cache table.");
                        return true;
                    }

                    obj->LinkPrefab(prefabId, prefabObjectId);
                }
            }
        }

        return false;
    }

    bool Prefab::UpdateInternal(const List<SceneObject*>& defaultInstanceObjects, Json::StringBuffer& tmpBuffer)
    {
        PROFILE_CPU_NAMED("Prefab.UpdateData");

        // Serialize to json data
        {
            tmpBuffer.Clear();
            PrettyJsonWriter writerObj(tmpBuffer);
            JsonWriter& writer = writerObj;
            writer.StartArray();

            SerializeContext serializeContext(writer);
            for (int32 i = 0; i < defaultInstanceObjects.Count(); i++)
            {
                auto obj = defaultInstanceObjects.At(i);

                Serialization::Serialize(serializeContext, obj);
            }
            writer.EndArray();
        }

        LOG_INFO("Level", "Updating prefab data");

        // Reload prefab data
        if (IsVirtual())
        {
            return Init(Typeof<Prefab>(), StringAnsiView(tmpBuffer.GetString(), (int32)tmpBuffer.GetSize()));
        }
#define COMPILE_WITH_ASSETS_IMPORTER 1
#if 1 // Set to 0 to use memory-only reload that does not modifies the source file - useful for testing and debugging prefabs apply
#if COMPILE_WITH_ASSETS_IMPORTER
        Locker.Unlock();

        // Save to file
        if (!CreateJson::Create(GetPath(), tmpBuffer, Typeof<Prefab>()))
        {
            Locker.Lock();
            LOG_WARNING("Level", "Failed to serialize prefab data to the asset.");
            return false;
        }

        // Ensure to be loaded
        if (!WaitForLoaded())
        {
            Locker.Lock();
            LOG_WARNING("Level", "Waiting for prefab asset reload failed.");
            return false;
        }

        Locker.Lock();
#else
#error "Cannot support prefabs creating without assets importing enabled."
#endif
#else
        {
            // Create object data
            Json::StringBuffer buffer;
            PrettyJsonWriter writerObj(buffer);
            JsonWriter& writer = writerObj;
            writer.StartObject();
            {
                // Json resource header
                writer.JKEY("ID");
                writer.UID(GetID());
                writer.JKEY("TypeName");
                writer.String(TypeName);
                writer.JKEY("EngineBuild");
                writer.Int(FLAXENGINE_VERSION_BUILD);

                // Json resource data
                writer.JKEY("Data");
                writer.RawValue(tmpBuffer.GetString(), (int32)tmpBuffer.GetSize());
            }
            writer.EndObject();

            // Unload
            ObjectsCount = 0;
            ObjectsIds.Resize(0);
            NestedPrefabs.Resize(0);
            ObjectsDataCache.Clear();
            ObjectsDataCache.SetCapacity(0);
            ObjectsCache.Clear();
            ObjectsCache.SetCapacity(0);
            if (_defaultInstance)
            {
                _defaultInstance->DeleteObject();
                _defaultInstance = nullptr;
            }
            _isLoaded = false;

            // Update prefab data manually (to prevent updating source asset file - just for testing)
            Document.Parse(buffer.GetString(), buffer.GetSize());
            if (Document.HasParseError())
                return true;
            const auto id = JsonTools::GetGuid(Document, "ID");
            if (id != _id)
                return true;
            DataTypeName = JsonTools::GetString(Document, "TypeName");
            DataEngineBuild = JsonTools::GetInt(Document, "EngineBuild", FLAXENGINE_VERSION_BUILD);
            auto dataMember = Document.FindMember("Data");
            if (dataMember == Document.MemberEnd())
                return true;
            Data = &dataMember->value;
            if (!Data->IsArray())
                return true;
            const int32 objectsCount = Data->GetArray().Size();
            if (objectsCount <= 0)
                return true;
            ObjectsIds.EnsureCapacity(objectsCount * 2);
            NestedPrefabs.EnsureCapacity(objectsCount);
            ObjectsDataCache.EnsureCapacity(objectsCount * 3);
            const auto& data = *Data;
            for (int32 objectIndex = 0; objectIndex < objectsCount; objectIndex++)
            {
                auto& objData = data[objectIndex];

                UID objectId = JsonTools::GetGuid(objData, "ID");
                if (!objectId.IsValid())
                    return true;

                ObjectsIds.Add(objectId);
                ASSERT(!ObjectsDataCache.ContainsKey(objectId));
                ObjectsDataCache.Add(objectId, &objData);
                ObjectsCount++;

                UID prefabId = JsonTools::GetGuid(objData, "PrefabID");
                if (prefabId.IsValid() && !NestedPrefabs.Contains(prefabId))
                {
                    if (prefabId == _id)
                    {
                        LOG_ERROR("Level", "Circural reference in prefab.");
                        continue;
                    }

                    NestedPrefabs.Add(prefabId);
                }
            }
            _isLoaded = true;
        }
#endif

        return true;
    }

    bool Prefab::SyncChangesInternal(PrefabInstancesData& prefabInstancesData)
    {
        PROFILE_CPU_NAMED("Prefab.SyncChanges");

        LOG_INFO("Level", "Syncing prefab {0}", ToString());

        // Ensure to be loaded
        if (WaitForLoaded())
        {
            LOG_WARNING("Level", "Waiting for prefab asset load failed.");
            return true;
        }

        // Recreate default instance but with synchronization since otherwise it might contain old data (eg. nested prefab hierarchy could be changed)
        DeleteDefaultInstance();
        // ObjectsRemovalService::Flush();
        {
            Threading::ScopeLock lock(Locker);
            _isCreatingDefaultInstance = true;
            _defaultInstance = PrefabManager::SpawnPrefab(this, nullptr, &ObjectsCache, true);
            _isCreatingDefaultInstance = false;
        }

        // Instantiate prefab instance from prefab (default spawning logic)
        // Note: it will get any added or removed objects from the nested prefabs
        // TODO: try to optimize by using recreated default instance to ApplyAllInternal (will need special path there if apply is done with default instance to unlink it instead of destroying)
        const auto targetActor = PrefabManager::SpawnPrefab(this, nullptr, nullptr, true);
        if (targetActor == nullptr)
        {
            LOG_WARNING("Level", "Failed to instantiate default prefab instance from changes synchronization.");
            return true;
        }

        // Ensure to delete the spawned objects instance with diff applied
        AutoActorCleanup cleanupDefaultInstance(targetActor);

        // Apply changes
        return ApplyAllInternal(targetActor, false, prefabInstancesData);
    }

    void Prefab::SyncNestedPrefabs(const NestedPrefabsList& allPrefabs, List<PrefabInstancesData>& allPrefabsInstancesData) const
    {
        PROFILE_CPU();
        LOG_INFO("Level", "Updating referencing prefabs");

        // TODO: this may not work well for very complex prefab nesting -> loop order matters, maybe build a graph of dependencies?

        // Call recursive for all referencing prefab assets to refresh nested prefabs
        for (int32 i = 0; i < allPrefabs.Count(); i++)
        {
            auto nestedPrefab = allPrefabs[i].Get();
            if (nestedPrefab)
            {
                if (nestedPrefab->WaitForLoaded())
                {
                    LOG_WARNING("Level", "Waiting for prefab asset load failed.");
                    continue;
                }

                // Sync only if prefab is used by this prefab (directly) and it has been captured before
                const int32 nestedPrefabIndex = nestedPrefab->NestedPrefabs.Find(GetID());
                if (nestedPrefabIndex != -1)
                {
                    if (nestedPrefab->SyncChangesInternal(allPrefabsInstancesData[i]))
                        continue;
                    nestedPrefab->SyncNestedPrefabs(allPrefabs, allPrefabsInstancesData);
                    // ObjectsRemovalService::Flush();
                }
            }
        }
    }
} // SE