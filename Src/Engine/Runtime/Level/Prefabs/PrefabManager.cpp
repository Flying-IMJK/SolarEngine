
#include "PrefabManager.h"

#include <Runtime/Core/Logging/Exceptions/ArgumentException.h>

#include "Prefab.h"
#include "Runtime/Core/Systems.h"
#include "Runtime/Core/Logging/Exceptions/ArgumentNullException.h"
#include "Runtime/Core/Platform/CriticalSection.h"
#include "Runtime/Core/Profiler/ProfilerCPU.h"
#include "Runtime/Core/Serialization/JsonTools.h"
#include "Runtime/Core/Types/Collections/CollectionPoolCache.h"
#include "Runtime/Level/ActorsCache.h"
#include "Runtime/Level/Level.h"
#include "Runtime/Level/SceneObjectsFactory.h"
#include "Runtime/Level/SceneQuery.h"
#include "Runtime/Resource/Importers/CreateJson.h"

namespace SE
{
#if SE_EDITOR
    bool PrefabManager::IsCreatingPrefab = false;
    Dictionary<UID, List<Actor*>> PrefabManager::PrefabsReferences;
    CriticalSection PrefabManager::PrefabsReferencesLocker;
#endif

    class PrefabManagerService : public ISystem
    {
        ENGINE_SYSTEM(PrefabManagerService)
    public:
        PrefabManagerService()
            : ISystem(SE_TEXT("Prefab System"))
        {
        }
    };

    ENGINE_SYSTEM_REGISTER(PrefabManagerService)

    Actor* PrefabManager::SpawnPrefab(Prefab* prefab)
    {
        Actor* parent = Level::Scenes.Count() != 0 ? Level::Scenes.Get()[0] : nullptr;
        return SpawnPrefab(prefab, Transform(Float3::Minimum), parent, nullptr);
    }

    Actor* PrefabManager::SpawnPrefab(Prefab* prefab, const Float3& position)
    {
        Actor* parent = Level::Scenes.Count() != 0 ? Level::Scenes.Get()[0] : nullptr;
        return SpawnPrefab(prefab, Transform(position), parent, nullptr);
    }

    Actor* PrefabManager::SpawnPrefab(Prefab* prefab, const Float3& position, const Quaternion& rotation)
    {
        Actor* parent = Level::Scenes.Count() != 0 ? Level::Scenes.Get()[0] : nullptr;
        return SpawnPrefab(prefab, Transform(position, rotation), parent, nullptr);
    }

    Actor* PrefabManager::SpawnPrefab(Prefab* prefab, const Float3& position, const Quaternion& rotation, const Float3& scale)
    {
        Actor* parent = Level::Scenes.Count() != 0 ? Level::Scenes.Get()[0] : nullptr;
        return SpawnPrefab(prefab, Transform(position, rotation, scale), parent, nullptr);
    }

    Actor* PrefabManager::SpawnPrefab(Prefab* prefab, const Transform& transform)
    {
        Actor* parent = Level::Scenes.Count() != 0 ? Level::Scenes.Get()[0] : nullptr;
        return SpawnPrefab(prefab, transform, parent, nullptr);
    }

    Actor* PrefabManager::SpawnPrefab(Prefab* prefab, Actor* parent, const Transform& transform)
    {
        return SpawnPrefab(prefab, transform, parent, nullptr);
    }

    Actor* PrefabManager::SpawnPrefab(Prefab* prefab, Actor* parent)
    {
        return SpawnPrefab(prefab, Transform(Float3::Minimum), parent, nullptr);
    }

    Actor* PrefabManager::SpawnPrefab(Prefab* prefab, Actor* parent, Dictionary<UID, SceneObject*>* objectsCache, bool withSynchronization)
    {
        return SpawnPrefab(prefab, Transform(Float3::Minimum), parent, objectsCache, withSynchronization);
    }

    Actor* PrefabManager::SpawnPrefab(Prefab* prefab, const Transform& transform, Actor* parent, Dictionary<UID, SceneObject*>* objectsCache, bool withSynchronization)
    {
        PROFILE_CPU_NAMED("Prefab.Spawn");
        if (prefab == nullptr)
        {
            Log::ArgumentNullException();
            return nullptr;
        }
        if (prefab->WaitForLoaded())
        {
            LOG_WARNING("Level", "Waiting for prefab asset be loaded failed. {0}", prefab->ToString());
            return nullptr;
        }
        const int32 dataCount = prefab->ObjectsCount;
        if (dataCount == 0)
        {
            LOG_WARNING("Level", "Prefab has no objects. {0}", prefab->ToString());
            return nullptr;
        }
        const UID prefabId = prefab->GetID();

        // Note: we need to generate unique Ids for the deserialized objects (actors and scripts) to prevent Ids collisions
        // Prefab asset during loading caches the object Ids stored inside the file

        // Prepare
        CollectionPoolCache<ActorsCache::SceneObjectsListType>::ScopeCache sceneObjects = ActorsCache::SceneObjectsListCache.Get();
        sceneObjects->Resize(dataCount);
        CollectionPoolCache<ISerializeModifier, SerializeModifierCache::ISerializeModifierClearCallback>::ScopeCache modifier = SerializeModifierCache::Modifier.Get();
        modifier->EngineBuild = prefab->DataEngineBuild;
        modifier->IdsMapping.EnsureCapacity(prefab->ObjectsIds.Count() * 4);
        for (int32 i = 0; i < prefab->ObjectsIds.Count(); i++)
        {
            modifier->IdsMapping.Add(prefab->ObjectsIds[i], UID::New());
        }
        if (objectsCache)
        {
            objectsCache->Clear();
            objectsCache->SetCapacity(prefab->ObjectsDataCache.Capacity());
        }
        auto& data = *prefab->Data;
        SceneObjectsFactory::Context context(modifier.Value);

        // Deserialize prefab objects
        // auto prevIdMapping = Scripting::ObjectsLookupIdMapping.Get();
        // Scripting::ObjectsLookupIdMapping.Set(&modifier.Value->IdsMapping);
        for (int32 i = 0; i < dataCount; i++)
        {
            auto& stream = data[i];
            SceneObject* obj = SceneObjectsFactory::Spawn(context, stream);
            sceneObjects->At(i) = obj;
            if (obj)
            {
                // obj->RegisterObject();
            }
            else
            {
                SceneObjectsFactory::HandleObjectDeserializationError(stream);
            }
        }
        SceneObjectsFactory::PrefabSyncData prefabSyncData(*sceneObjects.Value, data, modifier.Value);
        if (withSynchronization)
        {
            // Synchronize new prefab instances (prefab may have new objects added so deserialized instances need to synchronize with it)
            // TODO: resave and force sync prefabs during game cooking so this step could be skipped in game
            SceneObjectsFactory::SetupPrefabInstances(context, prefabSyncData);
            SceneObjectsFactory::SynchronizeNewPrefabInstances(context, prefabSyncData);
            // Scripting::ObjectsLookupIdMapping.Set(&modifier.Value->IdsMapping);
        }
        for (int32 i = 0; i < dataCount; i++)
        {
            auto& stream = data[i];
            SceneObject* obj = sceneObjects->At(i);
            if (obj)
            {
                SceneObjectsFactory::Deserialize(context, obj, stream);
            }
        }
        // Scripting::ObjectsLookupIdMapping.Set(prevIdMapping);

        // Synchronize prefab instances (prefab may have new objects added or some removed so deserialized instances need to synchronize with it)
        if (withSynchronization)
        {
            // TODO: resave and force sync scenes during game cooking so this step could be skipped in game
            SceneObjectsFactory::SynchronizePrefabInstances(context, prefabSyncData);
        }

        // Pick prefab root object
        Actor* root = nullptr;
        const UID prefabRootObjectId = prefab->GetRootObjectId();
        for (int32 i = 0; i < dataCount && !root; i++)
        {
            if (JsonTools::GetGuid(data[i], "ID") == prefabRootObjectId)
                root = dynamic_cast<Actor*>(sceneObjects->At(i));
        }
        if (!root)
        {
            // Fallback to the first actor that has no parent
            for (int32 i = 0; i < sceneObjects->Count() && !root; i++)
            {
                SceneObject* obj = sceneObjects->At(i);
                if (obj && !obj->GetParent())
                    root = dynamic_cast<Actor*>(obj);
            }
        }
        if (!root)
        {
            LOG_WARNING("Level", "Missing prefab root object. {0}", prefab->ToString());
            return nullptr;
        }

        // Prepare parent linkage for prefab root actor
        if (root->GetParent())
        {
            root->GetParent()->Children.Remove(root);
        }
        root->SetParent(parent);
        if (parent)
        {
            parent->Children.Add(root);
        }

        // Move root to the right location
        if (transform.Translation != Float3::Minimum)
        {
            root->SetTransform(transform);
        }

        // Link actors hierarchy
        for (int32 i = 0; i < sceneObjects->Count(); i++)
        {
            SceneObject* obj = sceneObjects->At(i);
            if (obj)
            {
                obj->Initialize();
            }
        }

        // Delete objects without parent or with invalid linkage to the prefab
        for (int32 i = 0; i < sceneObjects->Count(); i++)
        {
            SceneObject* obj = sceneObjects->At(i);
            if (!obj || obj == root)
                continue;

            // Check for missing parent (eg. parent object has been deleted)
            if (obj->GetParent() == nullptr)
            {
                LOG_WARNING("Level", "Scene object {0} {1} has missing parent object after load. Removing it.", obj->GetInstanceID(), obj->ToString());
                sceneObjects->At(i) = nullptr;
                obj->DeleteObject();
                continue;
            }

#if (SE_EDITOR && !BUILD_RELEASE) || FLAX_TESTS
            // Check for not being added to the parent (eg. invalid setup events fault on registration)
            auto actor = TypeCast<Actor>(obj);;
            /*auto script = dynamic_cast<Script*>(obj);*/
            if (obj->GetParent() == obj || (actor && !actor->GetParent()->Children.Contains(actor))/* || (script && !script->GetParent()->Scripts.Contains(script))*/)
            {
                LOG_WARNING("Level", "Scene object {0} {1} has invalid parent object linkage after load. Removing it.", obj->GetInstanceID(), obj->ToString());
                sceneObjects->At(i) = nullptr;
                obj->DeleteObject();
                continue;
            }
#endif

#if (SE_EDITOR && BUILD_DEBUG) || FLAX_TESTS
            // Check for being added to parent not from spawned prefab (eg. invalid parentId linkage fault)
            bool hasParentInInstance = false;
            for (int32 j = 0; j < sceneObjects->Count(); j++)
            {
                if (sceneObjects->At(j) == obj->GetParent())
                {
                    hasParentInInstance = true;
                    break;
                }
            }
            if (!hasParentInInstance)
            {
                LOG_WARNING("Level", "Scene object {0} {1} has invalid parent object after load. Removing it.", obj->GetID(), obj->ToString());
                sceneObjects->At(i) = nullptr;
                obj->DeleteObject();
                continue;
            }

#if FLAX_TESTS
            // Perform extensive validation of the prefab instance structure
            if (actor && actor->HasActorInHierarchy(actor))
            {
                LOG_WARNING("Level", "Scene object {0} {1} has invalid hierarchy after load. Removing it.", obj->GetID(), obj->ToString());
                sceneObjects->At(i) = nullptr;
                obj->DeleteObject();
                continue;
            }
#endif
#endif
        }

        // Link objects to prefab (only deserialized from prefab data)
        for (int32 i = 0; i < dataCount; i++)
        {
            auto& stream = data[i];
            SceneObject* obj = sceneObjects->At(i);
            if (!obj)
                continue;

            const UID prefabObjectId = JsonTools::GetGuid(stream, "ID");
            if (objectsCache)
                objectsCache->Add(prefabObjectId, obj);
            obj->LinkPrefab(prefabId, prefabObjectId);
        }

        // Update transformations
        root->OnTransformChanged();

        // Spawn if need to
        if (parent && parent->IsDuringPlay())
        {
            // Begin play
            SceneBeginData beginData;
            root->BeginPlay(&beginData);
            beginData.OnDone();

            // Send event
            Level::CallActorEvent(Level::ActorEventType::OnActorSpawned, root, nullptr);
        }

        return root;
    }

#if SE_EDITOR

    bool PrefabManager::CreatePrefab(Actor* targetActor, const StringView& outputPath, bool autoLink)
    {
        PROFILE_CPU_NAMED("Prefab.Create");

        // Validate input
        if (targetActor == nullptr)
        {
            Log::ArgumentNullException();
            return false;
        }
        if (targetActor->GetType() == Typeof<Scene>())
        {
            LOG_ERROR("Level", "Cannot create prefab from scene actor.");
            return false;
        }

        // Gather all objects
        CollectionPoolCache<ActorsCache::SceneObjectsListType>::ScopeCache sceneObjects = ActorsCache::SceneObjectsListCache.Get();
        sceneObjects->EnsureCapacity(256);
        SceneQuery::GetAllSerializableSceneObjects(targetActor, *sceneObjects.Value);

        // Filter actors for prefab
        if (!FilterPrefabInstancesToSave(UID::Empty, *sceneObjects.Value))
            return false;

        LOG_INFO("Level", "Creating prefab from actor {0} (total objects count: {2}) to {1}...", targetActor->ToString(), outputPath, sceneObjects->Count());

        // Serialize to json data
        ENGINE_ASSERT(!IsCreatingPrefab);
        IsCreatingPrefab = true;
        const UID targetPrefabId = targetActor->GetPrefabID();
        const bool hasTargetPrefabId = targetPrefabId.IsValid();
        Json::StringBuffer actorsDataBuffer;
        {
            CompactJsonWriter writerObj(actorsDataBuffer);
            JsonWriter& writer = writerObj;
            writer.StartArray();
            for (int32 i = 0; i < sceneObjects->Count(); i++)
            {
                SceneObject* obj = sceneObjects->At(i);

                // Detect when creating prefab from object that is already part of prefab then serialize it as unlinked
                const UID prefabId = obj->GetPrefabID();
                const UID prefabObjectId = obj->GetPrefabObjectID();
                bool isObjectFromPrefab = targetPrefabId == prefabId && prefabId.IsValid(); // Allow to use other nested prefabs properly (ignore only root object's prefab link)
                if (isObjectFromPrefab)
                {
                    //obj->BreakPrefabLink();
                    obj->m_PrefabID = UID::Empty;
                    obj->m_PrefabObjectID = UID::Empty;
                }

                SerializeContext serializeContext(writer);
                Serialization::Serialize(serializeContext, obj);

                // Restore broken link
                if (hasTargetPrefabId)
                {
                    //obj->LinkPrefab(prefabId, prefabObjectId);
                    obj->m_PrefabID = prefabId;
                    obj->m_PrefabObjectID = prefabObjectId;
                }
            }
            writer.EndArray();
        }
        IsCreatingPrefab = false;

        // Randomize the objects ids (prevent overlapping of the prefab instance objects ids and the prefab objects ids)
        Dictionary<UID, UID> objectInstanceIdToPrefabObjectId;
        objectInstanceIdToPrefabObjectId.EnsureCapacity(sceneObjects->Count() * 3);
        if (targetActor->HasParent())
        {
            // Unlink from parent actor
            objectInstanceIdToPrefabObjectId[targetActor->GetParent()->GetInstanceID()] = UID::Empty;
        }
        for (int32 i = 0; i < sceneObjects->Count(); i++)
        {
            // Generate new IDs for the prefab objects (other than reference instance used to create prefab)
            const SceneObject* obj = sceneObjects->At(i);
            objectInstanceIdToPrefabObjectId[obj->GetSceneObjectId()] = UID::New();
        }
        {
            // Parse json to DOM document
            Json::Document doc;
            {
                PROFILE_CPU_NAMED("Json.Parse");
                doc.Parse(actorsDataBuffer.GetString(), actorsDataBuffer.GetSize());
            }
            if (doc.HasParseError())
            {
                LOG_WARNING("Level", "Failed to parse serialized actors data.");
                return false;
            }

            // Process json
            JsonTools::ChangeIds(doc, objectInstanceIdToPrefabObjectId);

            // Save back to text
            actorsDataBuffer.Clear();
            PrettyJsonWriter writer(actorsDataBuffer);
            doc.Accept(writer.GetWriter());
        }

        // Save to file
        if (!CreateJson::Create(outputPath, actorsDataBuffer, Typeof<Prefab>()))
        {
            LOG_WARNING("Level", "Failed to serialize prefab data to the asset.");
            return false;
        }

        // Auto link objects
        if (autoLink)
        {
            LOG_INFO("Level", "Linking objects to prefab");

            AssetInfo assetInfo;
            if (!AssetContent::GetAssetInfo(outputPath, assetInfo))
                return false;

            for (int32 i = 0; i < sceneObjects->Count(); i++)
            {
                SceneObject* obj = sceneObjects->At(i);
                UID prefabObjectId;
                if (objectInstanceIdToPrefabObjectId.TryGet(obj->GetSceneObjectId(), prefabObjectId))
                {
                    obj->LinkPrefab(assetInfo.id, prefabObjectId);
                }
            }
        }

        LOG_INFO("Level", "Prefab created!");
        return true;
    }

    bool FindPrefabLink(const UID& targetPrefabId, const UID& prefabId)
    {
        // Get prefab asset
        auto prefab = AssetContent::LoadAsync<Prefab>(prefabId);
        if (prefab == nullptr)
        {
            Log::Exception(SE_TEXT("Missing prefab asset."));
            return false;
        }
        if (prefab->WaitForLoaded())
        {
            Log::Exception(SE_TEXT("Failed to load prefab asset."));
            return false;
        }

        // TODO: check if prefab has any reference to targetPrefabId, then do recursive call to nested prefabs inside the prefab

        return true;
    }

    bool PrefabManager::FilterPrefabInstancesToSave(const UID& prefabId, List<SceneObject*>& objects, bool showWarning)
    {
#if 0 // Done by collecting actors without DontSave flag
        // Remove hidden actors for saving
        for (int32 i = 0; i < actors.Count() && actors.HasItems(); i++)
        {
            auto actor = actors[i];

            if ((actor->HideFlags & HideFlags::DontSave) != 0)
            {
                actors.RemoveAt(i);
                i--;
            }
        }
#endif

        // Validate circular references
        if (prefabId.IsValid())
        {
            bool hasLoopPrefabRef = false;

            for (int32 i = 0; i < objects.Count() && objects.HasItems(); i++)
            {
                SceneObject* obj = objects[i];

                if (obj->GetPrefabID().IsValid() && FindPrefabLink(prefabId, obj->GetPrefabID()))
                {
                    hasLoopPrefabRef = true;

                    objects.RemoveAt(i);
                    i--;
                }
            }

            if (hasLoopPrefabRef && showWarning)
            {
                LOG_ERROR("Level", "Cannot setup prefab with circular reference to itself.");
            }
        }

        // Check if list is empty (after validation)
        if (objects.IsEmpty())
        {
            LOG_WARNING("Level", "Cannot create prefab. No valid objects to use.");
            return false;
        }

        return true;
    }

    bool PrefabManager::ApplyAll(Actor* instance)
    {
        PROFILE_CPU_NAMED("Prefab.ApplyAll");

        // Validate input
        if (instance == nullptr)
        {
            Log::ArgumentNullException();
            return false;
        }
        if (!instance->HasPrefabLink() || instance->GetPrefabID() == UID::Empty)
        {
            Log::ArgumentException(SE_TEXT("The modified actor instance has missing prefab link."));
            return false;
        }

        // Get prefab asset
        auto prefab = AssetContent::LoadAsync<Prefab>(instance->GetPrefabID());
        if (prefab == nullptr)
        {
            Log::Exception(SE_TEXT("Missing prefab asset."));
            return false;
        }
        if (prefab->WaitForLoaded())
        {
            Log::Exception(SE_TEXT("Failed to load prefab asset."));
            return false;
        }

        // Get root object of this prefab instance
        // Note: given actor instance can be already part of the spawned prefab or be a new actor added to the prefab instance
        // Find the actual root of the prefab instance
        const auto rootObjectId = prefab->GetRootObjectId();
        auto rootObjectInstance = instance;
        while (rootObjectInstance && rootObjectInstance->GetPrefabObjectID() != rootObjectId)
        {
            rootObjectInstance = rootObjectInstance->GetParent();
        }
        if (rootObjectInstance == nullptr)
        {
            // Use the input object as fallback
            rootObjectInstance = instance;
        }

        return prefab->ApplyAll(rootObjectInstance);
    }

#endif
} // SE