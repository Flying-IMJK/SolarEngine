

#include "SceneObjectsFactory.h"

#include "Level.h"
#include "Core/Profiler/ProfilerCPU.h"
#include "Core/Serialization/JsonTools.h"
#include "Core/Serialization/Serialization.h"
#include "Core/Thread/Threading.h"
#include "Prefabs/Prefab.h"
#include "Runtime/Resource/AssetContent.h"

namespace SE
{
    CollectionPoolCache<ISerializeModifier, SerializeModifierCache::ISerializeModifierClearCallback> SerializeModifierCache::Modifier;

    void SerializeModifierCache::ISerializeModifierClearCallback(ISerializeModifier* obj)
    {
        obj->EngineBuild = 1;
        obj->CurrentInstance = -1;
        obj->IdsMapping.Clear();
    }

    void SerializeModifierCache::Release()
    {
        Modifier.Release();
    }

    SceneObjectsFactory::Context::Context(ISerializeModifier* modifier)
        : Modifier(modifier)
    {
        // Override the main thread value to not create it again in GetModifier() if called from the same thread
        Modifiers.Set(modifier);
    }

    SceneObjectsFactory::Context::~Context()
    {
        if (Async)
        {
            List<ISerializeModifier*, InlinedAllocation<PLATFORM_THREADS_LIMIT>> modifiers;
            Modifiers.GetValues(modifiers);
            for (ISerializeModifier* e : modifiers)
            {
                if (e == Modifier)
                    continue;
                SerializeModifierCache::Modifier.Put(e);
            }
        }
    }

    ISerializeModifier* SceneObjectsFactory::Context::GetModifier()
    {
        ISerializeModifier* modifier = Modifier;
        if (Async)
        {
            // When using context in async then use one ISerializeModifier per-thread
            ISerializeModifier*& modifierThread = Modifiers.Get();
            if (!modifierThread)
            {
                modifierThread = SerializeModifierCache::Modifier.GetUnscoped();
                Modifiers.Set(modifierThread);
                Locker.Lock();
                modifierThread->EngineBuild = modifier->EngineBuild;
                modifierThread->CurrentInstance = modifier->CurrentInstance;
                modifierThread->IdsMapping = modifier->IdsMapping;
                Locker.Unlock();
            }
            modifier = modifierThread;
        }
        return modifier;
    }

    void SceneObjectsFactory::Context::SetupIdsMapping(const SceneObject* obj, ISerializeModifier* modifier) const
    {
        int32 instanceIndex;
        if (ObjectToInstance.TryGet(obj->GetInstanceID(), instanceIndex) && instanceIndex != modifier->CurrentInstance)
        {
            // Apply the current prefab instance objects ids table to resolve references inside a prefab properly
            modifier->CurrentInstance = instanceIndex;
            const auto& instance = Instances[instanceIndex];
            for (const auto& e : instance.IdsMapping)
                modifier->IdsMapping[e.Key] = e.Value;
        }
    }

    SceneObject* SceneObjectsFactory::Spawn(Context& context, const DeserializeStream& stream)
    {
        // Get object id
        UID id = JsonTools::GetGuid(stream, "ID");
        ISerializeModifier* modifier = context.GetModifier();
        modifier->IdsMapping.TryGet(id, id);
        if (!id.IsValid())
        {
            LOG_WARNING("Level", "Invalid object id.");
            return nullptr;
        }
        SceneObject* obj = nullptr;

        // Check for prefab instance
        UID prefabObjectId;
        if (JsonTools::GetGuidIfValid(prefabObjectId, stream, "PrefabObjectID"))
        {
            // Get prefab asset id
            const UID prefabId = JsonTools::GetGuid(stream, "PrefabID");
            if (!prefabId.IsValid())
            {
                LOG_WARNING("Level", "Invalid prefab id.");
                return nullptr;
            }

            // Load prefab
            auto prefab = AssetContent::LoadAsync<Prefab>(prefabId);
            if (prefab == nullptr)
            {
                LOG_WARNING("Level", "Missing prefab {0}.", prefabId);
                return nullptr;
            }
            if (!prefab->WaitForLoaded())
            {
                LOG_WARNING("Level", "Failed to load prefab {0}.", prefab->ToString());
                return nullptr;
            }

            // Get prefab object data from the prefab
            const DeserializeStream* prefabData;
            if (!prefab->ObjectsDataCache.TryGet(prefabObjectId, prefabData))
            {
                LOG_WARNING("Level", "Missing object {1} data in prefab {0}.", prefab->ToString(), prefabObjectId);
                return nullptr;
            }

            // Map prefab object ID to the deserialized instance ID
            modifier->IdsMapping[prefabObjectId] = id;

            // Create prefab instance (recursive prefab loading to support nested prefabs)
            obj = Spawn(context, *prefabData);
        }
        else
        {
            const auto typeNameMember = stream.FindMember("TypeID");
            if (typeNameMember != stream.MemberEnd())
            {
                if (!typeNameMember->value.IsUint())
                {
                    LOG_WARNING("Level", "Invalid object type (TypeID must be an object type full unit).");
                    return nullptr;
                }
                const TypeID typeId(typeNameMember->value.GetUint());
                if (TypeIsValid(typeId))
                {
                    // TODO: cache per-type result in Context to boost loading of the large scenes
                    if (!Types::IsTypeDerivedFrom(typeId, Typeof<SceneObject>()))
                    {
                        LOG_WARNING("Level", "Invalid scene object type {0} (inherits from: {1}).", typeId.ToString(), Typeof<SceneObject>().ToString());
                        return nullptr;
                    }
                     
                    obj = Types::CreateInstance<SceneObject>(typeId);
                    if (obj == nullptr)
                    {
                        LOG_WARNING("Level", "Failed to spawn object of type {0}.", typeId.ToString());
                        return nullptr;
                    }
                }
                else
                {
                    LOG_WARNING("Level", "Unknown object type '{0}', ID: {1}", typeId.ToString(), id.ToString());
                    return nullptr;
                }
            }
            else
            {
                LOG_WARNING("Level", "Invalid object type.");
            }
        }

        return obj;
    }

    void SceneObjectsFactory::Deserialize(Context& context, SceneObject* obj, DeserializeStream& stream)
    {
#if ENABLE_ASSERTION
        CHECK(obj);
#endif
        ISerializeModifier* modifier = context.GetModifier();

        // Check for prefab instance
        UID prefabObjectId;
        if (JsonTools::GetGuidIfValid(prefabObjectId, stream, "PrefabObjectID"))
        {
            // Get prefab asset id
            const UID prefabId = JsonTools::GetGuid(stream, "PrefabID");
            if (!prefabId.IsValid())
            {
                LOG_WARNING("Level", "Invalid prefab id.");
                return;
            }

            // Load prefab
            auto prefab = AssetContent::LoadAsync<Prefab>(prefabId);
            if (prefab == nullptr)
            {
                LOG_WARNING("Level", "Missing prefab {0}.", prefabId);
                return;
            }
            if (prefab->WaitForLoaded())
            {
                LOG_WARNING("Level", "Failed to load prefab {0}.", prefab->ToString());
                return;
            }

            // Get prefab object data from the prefab
            const DeserializeStream* prefabData;
            if (!prefab->ObjectsDataCache.TryGet(prefabObjectId, prefabData))
            {
                LOG_WARNING("Level", "Missing object {1} data in prefab {0}.", prefab->ToString(), prefabObjectId);
                return;
            }

            // Deserialize prefab data (recursive prefab loading to support nested prefabs)
            const auto prevVersion = modifier->EngineBuild;
            modifier->EngineBuild = prefab->DataEngineBuild;
            Deserialize(context, obj, *(DeserializeStream*)prefabData);
            modifier->EngineBuild = prevVersion;
        }

        context.SetupIdsMapping(obj, modifier);

        // Load data
        DeserializeContext deserializeContext(stream, modifier);
        obj->Deserialize(deserializeContext);
    }

    void SceneObjectsFactory::HandleObjectDeserializationError(const DeserializeStream& value)
    {
#if !BUILD_RELEASE || SE_EDITOR
        // Prevent race-conditions when logging missing objects (especially when adding dummy MissingScript)
        Threading::ScopeLock lock(Level::GetScenesLock());

        // Print invalid object data contents
        Json::StringBuffer buffer;
        PrettyJsonWriter writer(buffer);
        value.Accept(writer.GetWriter());
        String bufferStr(buffer.GetString());
        LOG_WARNING("Level", "Failed to deserialize scene object from data: {0}", bufferStr);

        // Try to log some useful info about missing object (eg. it's parent name for faster fixing)
        const auto parentIdMember = value.FindMember("ParentID");
        if (parentIdMember != value.MemberEnd() && parentIdMember->value.IsString())
        {
            const UID parentId = JsonTools::GetGuid(parentIdMember->value);
            Actor* parent = nullptr;//Scripting::FindObject<Actor>(parentId);
            if (parent)
            {
#if SE_EDITOR
                // Add dummy script
                const auto typeNameMember = value.FindMember("TypeName");
                if (typeNameMember != value.MemberEnd() && typeNameMember->value.IsString())
                {
                    /*auto* dummyScript = parent->AddScript<MissingScript>();
                    dummyScript->MissingTypeName = typeNameMember->value.GetString();
                    dummyScript->Data = MoveTemp(bufferStr);*/
                }
#endif
                LOG_WARNING("Level", "Parent actor of the missing object: '{0}' ({1})", parent->GetNamePath(), parent->GetType());
            }
        }
#endif
    }

    SceneObjectsFactory::PrefabSyncData::PrefabSyncData(List<SceneObject*>& sceneObjects, const DeserializeStream& data, ISerializeModifier* modifier)
        : SceneObjects(sceneObjects)
        , Data(data)
        , Modifier(modifier)
        , InitialCount(0)
    {
    }

    void SceneObjectsFactory::PrefabSyncData::InitNewObjects()
    {
        for (int32 i = 0; i < NewObjects.Count(); i++)
        {
            SceneObject* obj = SceneObjects[InitialCount + i];
            obj->Initialize();
        }
    }

    void SceneObjectsFactory::SetupPrefabInstances(Context& context, const PrefabSyncData& data)
    {
        PROFILE_CPU_NAMED("SetupPrefabInstances");
        const int32 count = data.Data.Size();
        ENGINE_ASSERT(count <= data.SceneObjects.Count());
        for (int32 i = 0; i < count; i++)
        {
            const auto& stream = data.Data[i];
            UID prefabObjectId, prefabId;
            if (!JsonTools::GetGuidIfValid(prefabObjectId, stream, "PrefabObjectID"))
                continue;
            if (!JsonTools::GetGuidIfValid(prefabId, stream, "PrefabID"))
                continue;
            UID parentId = JsonTools::GetGuid(stream, "ParentID");
            for (int32 j = i - 1; j >= 0; j--)
            {
                // Find ID of the parent to this object (use data in json for relationship)
                if (parentId == JsonTools::GetGuid(data.Data[j], "ID") && data.SceneObjects[j])
                {
                    parentId = data.SceneObjects[j]->GetInstanceID();
                    break;
                }
            }
            const SceneObject* obj = data.SceneObjects[i];
            const UID id = obj ? obj->GetInstanceID() : JsonTools::GetGuid(stream, "ID");
            auto prefab = AssetContent::LoadAsync<Prefab>(prefabId);
            if (!prefab)
                continue;

            // Check if it's parent is in the same prefab
            int32 index;
            if (context.ObjectToInstance.TryGet(parentId, index) && context.Instances[index].Prefab == prefab)
            {
                // Use parent object as prefab instance
            }
            else
            {
                // Use new prefab instance
                index = context.Instances.Count();
                auto& e = context.Instances.AddOne();
                e.Prefab = prefab;
                e.RootId = id;
                e.RootIndex = i;
                e.StatIndex = i;
            }
            context.ObjectToInstance[id] = index;

            // Add to the prefab instance IDs mapping
            auto& prefabInstance = context.Instances[index];
            prefabInstance.IdsMapping[prefabObjectId] = id;

            // Walk over nested prefabs to link any subobjects into this object (eg. if nested prefab uses cross-object references to link them correctly)
            NESTED_PREFAB_WALK:
                const DeserializeStream* prefabData;
            if (prefab->ObjectsDataCache.TryGet(prefabObjectId, prefabData) && JsonTools::GetGuidIfValid(prefabObjectId, *prefabData, "PrefabObjectID"))
            {
                prefabId = JsonTools::GetGuid(*prefabData, "PrefabID");
                prefab = AssetContent::LoadAsync<Prefab>(prefabId);
                if (prefab && !prefab->WaitForLoaded())
                {
                    // Map prefab object ID to the deserialized instance ID
                    prefabInstance.IdsMapping[prefabObjectId] = id;
                    goto NESTED_PREFAB_WALK;
                }
            }
        }
    }

    void SceneObjectsFactory::SynchronizeNewPrefabInstances(Context& context, PrefabSyncData& data)
    {
        PROFILE_CPU_NAMED("SynchronizeNewPrefabInstances");

        // Scripting::ObjectsLookupIdMapping.Set(&data.Modifier->IdsMapping);
        data.InitialCount = data.SceneObjects.Count();

        // Recreate any missing prefab root objects that were deleted (eg. spawned prefab got its root changed and deleted so old prefab instance needs to respawn it)
        for (int32 instanceIndex = 0; instanceIndex < context.Instances.Count(); instanceIndex++)
        {
            PrefabInstance& instance = context.Instances[instanceIndex];
            SceneObject* root = data.SceneObjects[instance.RootIndex];
            if (!root && instance.Prefab)
            {
                instance.FixRootParent = true;

                // Check if current prefab root existed in the deserialized data
                const auto& oldRootData = data.Data[instance.RootIndex];
                const UID oldRootId = JsonTools::GetGuid(oldRootData, "ID");
                const UID prefabObjectId = JsonTools::GetGuid(oldRootData, "PrefabObjectID");
                const UID prefabRootId = instance.Prefab->GetRootObjectId();
                UID id;
                int32 idInstance = -1;
                bool syncNewRoot = false;
                if (instance.IdsMapping.TryGet(prefabRootId, id) && context.ObjectToInstance.TryGet(id, idInstance) && idInstance == instanceIndex)
                {
                    // Update the missing root with the valid object from this prefab instance
                    LOG_WARNING("Level", "Changed prefab instance root from ID={0}, PrefabObjectID={1} to ID={2}, PrefabObjectID={3} ({4})", instance.RootId, prefabObjectId, id, prefabRootId, instance.Prefab->ToString());
                }
                else
                {
                    LOG_WARNING("Level", "Missing prefab instance root (ID={0}, PrefabObjectID={1}, {2})", instance.RootId, prefabObjectId, instance.Prefab->ToString());

                    // Get prefab object data from the prefab
                    const DeserializeStream* prefabData;
                    if (!instance.Prefab->ObjectsDataCache.TryGet(prefabRootId, prefabData))
                    {
                        LOG_WARNING("Level", "Missing object {1} data in prefab {0}.", instance.Prefab->ToString(), prefabObjectId);
                        continue;
                    }

                    // Map prefab object ID to the new prefab object instance
                    id = UID::New();
                    data.Modifier->IdsMapping[prefabRootId] = id;

                    // Create prefab instance (recursive prefab loading to support nested prefabs)
                    root = Spawn(context, *prefabData);
                    if (!root)
                    {
                        LOG_WARNING("Level", "Failed to create object {1} from prefab {0}.", instance.Prefab->ToString(), prefabRootId);
                        continue;
                    }

                    // Register object
                    // root->RegisterObject();
                    data.SceneObjects.Add(root);
                    auto& newObj = data.NewObjects.AddOne();
                    newObj.Prefab = instance.Prefab;
                    newObj.PrefabData = prefabData;
                    newObj.PrefabObjectId = prefabRootId;
                    newObj.Id = id;
                    context.ObjectToInstance[id] = instanceIndex;
                    syncNewRoot = true;
                }

                // Update prefab root info
                instance.RootId = id;
                instance.RootIndex = -1;//data.SceneObjects.Find(Scripting::FindObject<SceneObject>(id));
                ENGINE_ASSERT(instance.RootIndex != -1)

                // Remap removed prefab root into the current root (can be different type but is needed for proper hierarchy linkage)
                instance.IdsMapping[prefabObjectId] = id;
                instance.IdsMapping[prefabRootId] = id;
                instance.IdsMapping[oldRootId] = id;
                data.Modifier->IdsMapping[prefabObjectId] = id;
                data.Modifier->IdsMapping[prefabRootId] = id;
                data.Modifier->IdsMapping[oldRootId] = id;

                // Add any sub-objects that are missing (in case new root was created)
                if (syncNewRoot)
                {
                    SynchronizeNewPrefabInstances(context, data, instance.Prefab, (Actor*)root, prefabRootId, instance.RootIndex, oldRootData);
                }
            }
        }

        // Check all actors with prefab linkage for adding missing objects
        for (int32 i = 0; i < data.InitialCount; i++)
        {
            Actor* actor = dynamic_cast<Actor*>(data.SceneObjects[i]);
            if (!actor)
                continue;
            const auto& stream = data.Data[i];
            UID actorId, actorPrefabObjectId, prefabId;
            if (!JsonTools::GetGuidIfValid(actorPrefabObjectId, stream, "PrefabObjectID"))
                continue;
            if (!JsonTools::GetGuidIfValid(prefabId, stream, "PrefabID"))
                continue;
            if (!JsonTools::GetGuidIfValid(actorId, stream, "ID"))
                continue;

            // Load prefab
            auto prefab = AssetContent::LoadAsync<Prefab>(prefabId);
            if (prefab == nullptr)
            {
                LOG_WARNING("Level", "Missing prefab {0}.", prefabId);
                continue;
            }
            if (prefab->WaitForLoaded())
            {
                LOG_WARNING("Level", "Failed to load prefab {0}.", prefab->ToString());
                continue;
            }

            SynchronizeNewPrefabInstances(context, data, prefab, actor, actorPrefabObjectId, i, stream);
        }

        // Scripting::ObjectsLookupIdMapping.Set(nullptr);
    }

    void SceneObjectsFactory::SynchronizePrefabInstances(Context& context, PrefabSyncData& data)
    {
        PROFILE_CPU_NAMED("SynchronizePrefabInstances");

        // Check all objects with prefab linkage for moving to a proper parent
        for (int32 i = 0; i < data.InitialCount; i++)
        {
            SceneObject* obj = data.SceneObjects[i];
            if (!obj)
                continue;
            SceneObject* parent = obj->GetParent();
            const UID prefabId = obj->GetPrefabID();
            if (parent == nullptr || !obj->HasPrefabLink() || !parent->HasPrefabLink() || parent->GetPrefabID() != prefabId)
                continue;
            const UID prefabObjectId = obj->GetPrefabObjectID();
            const UID parentPrefabObjectId = parent->GetPrefabObjectID();

            // Load prefab
            auto prefab = AssetContent::LoadAsync<Prefab>(prefabId);
            if (prefab == nullptr)
            {
                LOG_WARNING("Level", "Missing prefab {0}.", prefabId);
                continue;
            }
            if (prefab->WaitForLoaded())
            {
                LOG_WARNING("Level", "Failed to load prefab {0}.", prefab->ToString());
                continue;
            }

            // Get the actual parent object stored in the prefab data
            const DeserializeStream* objData;
            UID actualParentPrefabId;
            if (!prefab->ObjectsDataCache.TryGet(prefabObjectId, objData) || !JsonTools::GetGuidIfValid(actualParentPrefabId, *objData, "ParentID"))
                continue;

            // Validate
            if (actualParentPrefabId != parentPrefabObjectId)
            {
                // Invalid connection object found!
                LOG_INFO("Level", "Object {0} has invalid parent object {4} -> {5} (PrefabObjectID: {1}, PrefabID: {2}, Path: {3})", obj->GetSceneObjectId(), prefabObjectId, prefab->GetID(), prefab->GetPath(), parentPrefabObjectId, actualParentPrefabId);

                // Map actual prefab object ID to the current scene objects collection
                context.SetupIdsMapping(obj, data.Modifier);
                data.Modifier->IdsMapping.TryGet(actualParentPrefabId, actualParentPrefabId);

                // Find parent
                const auto actualParent = nullptr; //Scripting::FindObject<Actor>(actualParentPrefabId);
                if (!actualParent)
                {
                    LOG_WARNING("Level", "The actual parent is missing.");
                    continue;
                }

                // Reparent
                obj->SetParent(actualParent, false);
            }

            // Preserve order in parent (values from prefab are used)
            if (i != 0)
            {
                const auto defaultInstance = prefab->GetDefaultInstance(obj->GetPrefabObjectID());
                const int32 order = defaultInstance ? defaultInstance->GetOrderInParent() : -1;
                if (order != -1)
                    obj->SetOrderInParent(order);
            }
        }

        // Synchronize new prefab objects
        for (int32 i = 0; i < data.NewObjects.Count(); i++)
        {
            SceneObject* obj = data.SceneObjects[data.InitialCount + i];
            auto& newObj = data.NewObjects[i];

            // Deserialize object with prefab data
            // Scripting::ObjectsLookupIdMapping.Set(&data.Modifier->IdsMapping);
            Deserialize(context, obj, *(DeserializeStream*)newObj.PrefabData);
            obj->LinkPrefab(newObj.Prefab->GetID(), newObj.PrefabObjectId);

            // Preserve order in parent (values from prefab are used)
            const auto defaultInstance = newObj.Prefab->GetDefaultInstance(newObj.PrefabObjectId);
            const int32 order = defaultInstance ? defaultInstance->GetOrderInParent() : -1;
            if (order != -1)
                obj->SetOrderInParent(order);
        }

        // Setup hierarchy for the prefab instances (ensure any new objects are connected)
        for (const auto& instance : context.Instances)
        {
            const auto& prefabStartData = data.Data[instance.StatIndex];
            UID prefabStartParentId;
            if (instance.FixRootParent && JsonTools::GetGuidIfValid(prefabStartParentId, prefabStartData, "ParentID"))
            {
                auto* root = data.SceneObjects[instance.RootIndex];
                const auto rootParent = nullptr; //Scripting::FindObject<Actor>(prefabStartParentId);
                root->SetParent(rootParent, false);
            }
        }

        // Scripting::ObjectsLookupIdMapping.Set(nullptr);
    }

    void SceneObjectsFactory::SynchronizeNewPrefabInstances(Context& context, PrefabSyncData& data, Prefab* prefab, Actor* actor, const UID& actorPrefabObjectId, int32 i, const DeserializeStream& stream)
    {
        // Check for RemovedObjects list
        const auto removedObjects = SERIALIZE_FIND_MEMBER((&stream), "RemovedObjects");

        // Check if the given actor has new children or scripts added (inside the prefab that it uses)
        // TODO: consider caching prefab objects structure maybe to boost this logic?
        for (auto it = prefab->ObjectsDataCache.begin(); it.IsNotEnd(); ++it)
        {
            // Use only objects that are linked to the current actor
            const UID parentId = JsonTools::GetGuid(*it->Value, "ParentID");
            if (parentId != actorPrefabObjectId)
                continue;

            // Skip if object was marked to be removed per instance
            const UID prefabObjectId = JsonTools::GetGuid(*it->Value, "ID");
            if (removedObjects != stream.MemberEnd())
            {
                auto& list = removedObjects->value;
                const int32 size = static_cast<int32>(list.Size());
                bool removed = false;
                for (int32 j = 0; j < size; j++)
                {
                    if (JsonTools::GetGuid(list[j]) == prefabObjectId)
                    {
                        removed = true;
                        break;
                    }
                }
                if (removed)
                    continue;
            }

            // Use only objects that are missing
            bool spawned = false;
            int32 childSearchStart = i + 1; // Objects are serialized with parent followed by its children
            int32 instanceIndex = -1;
            if (context.ObjectToInstance.TryGet(actor->GetInstanceID(), instanceIndex) && context.Instances[instanceIndex].Prefab == prefab)
            {
                // Start searching from the beginning of that prefab instance (eg. in case prefab objects were reordered)
                childSearchStart = Math::Min(childSearchStart, context.Instances[instanceIndex].StatIndex);
            }
            for (int32 j = childSearchStart; j < data.InitialCount; j++)
            {
                if (JsonTools::GetGuid(data.Data[j], "PrefabObjectID") == prefabObjectId)
                {
                    // This object exists in the saved scene objects list
                    spawned = true;
                    break;
                }
            }
            if (spawned)
                continue;

            // Map prefab object ID to this actor's prefab instance so the new objects gets added to it
            context.SetupIdsMapping(actor, data.Modifier);
            data.Modifier->IdsMapping[actorPrefabObjectId] = actor->GetInstanceID();
            // Scripting::ObjectsLookupIdMapping.Set(&data.Modifier->IdsMapping);

            // Create instance (including all children)
            SynchronizeNewPrefabInstance(context, data, prefab, actor, prefabObjectId);
        }
    }

    void SceneObjectsFactory::SynchronizeNewPrefabInstance(Context& context, PrefabSyncData& data, Prefab* prefab, Actor* actor, const UID& prefabObjectId)
    {
        PROFILE_CPU_NAMED("SynchronizeNewPrefabInstance");

        // Missing object found!
        LOG_INFO("Level", "Actor {0} has missing child object (PrefabObjectID: {1}, PrefabID: {2}, Path: {3})", actor->ToString(), prefabObjectId, prefab->GetID(), prefab->GetPath());

        // Get prefab object data from the prefab
        const DeserializeStream* prefabData;
        if (!prefab->ObjectsDataCache.TryGet(prefabObjectId, prefabData))
        {
            LOG_WARNING("Level", "Missing object {1} data in prefab {0}.", prefab->ToString(), prefabObjectId);
            return;
        }

        // Map prefab object ID to the new prefab object instance
        UID id = UID::New();
        data.Modifier->IdsMapping[prefabObjectId] = id;

        // Create prefab instance (recursive prefab loading to support nested prefabs)
        auto child = Spawn(context, *prefabData);
        if (!child)
        {
            LOG_WARNING("Level", "Failed to create object {1} from prefab {0}.", prefab->ToString(), prefabObjectId);
            return;
        }

        // Register object
        // child->RegisterObject();
        data.SceneObjects.Add(child);
        auto& newObj = data.NewObjects.AddOne();
        newObj.Prefab = prefab;
        newObj.PrefabData = prefabData;
        newObj.PrefabObjectId = prefabObjectId;
        newObj.Id = id;
        int32 instanceIndex = -1;
        if (context.ObjectToInstance.TryGet(actor->GetInstanceID(), instanceIndex) && context.Instances[instanceIndex].Prefab == prefab)
        {
            // Add to the prefab instance IDs mapping
            context.ObjectToInstance[id] = instanceIndex;
            auto& prefabInstance = context.Instances[instanceIndex];
            prefabInstance.IdsMapping[prefabObjectId] = id;
        }

        // Use loop to add even more objects to added objects (prefab can have one new object that has another child, we need to add that child)
        // TODO: prefab could cache lookup object id -> children ids
        for (auto q = prefab->ObjectsDataCache.begin(); q.IsNotEnd(); ++q)
        {
            UID qParentId;
            if (JsonTools::GetGuidIfValid(qParentId, *q->Value, "ParentID") && qParentId == prefabObjectId)
            {
                const UID qPrefabObjectId = JsonTools::GetGuid(*q->Value, "ID");
                SynchronizeNewPrefabInstance(context, data, prefab, actor, qPrefabObjectId);
            }
        }
    }
} // SE