#pragma once
#include "Runtime/Resource/AssetRef.h"
#include "Runtime/Resource/JsonAsset.h"

namespace SE
{
    class SceneObject;
    class Actor;

    /// <summary>
    /// Json asset that stores the collection of scene objects including actors and scripts. In general it can serve as any grouping of scene objects (for example a level) or be used as a form of a template instantiated and reused throughout the scene.
    /// </summary>
    /// <seealso cref="JsonAssetBase" />
    SE_CLASS(Reflect, API, NoSpawn)
    class SE_API_RUNTIME Prefab : public JsonAssetBase
    {
        SE_DEFINE_CLASS(Prefab, JsonAssetBase);
        ASSET_HEADER(Prefab);
    private:
        bool _isCreatingDefaultInstance;
        Actor* _defaultInstance;

    public:
        /// <summary>
        /// The serialized scene objects amount (actors and scripts).
        /// </summary>
        int32 ObjectsCount;

        /// <summary>
        /// The objects ids contained within the prefab asset. Valid only if asset is loaded.
        /// </summary>
        List<UID> ObjectsIds;

        /// <summary>
        /// The prefab assets ids contained within the prefab asset. Valid only if asset is loaded. Remember that each nested prefab can contain deeper references to the other assets.
        /// </summary>
        List<UID> NestedPrefabs;

        /// <summary>
        /// The objects data cache maps the id of the object contained in the prefab asset (actor or script) to the json data node for its data. Valid only if asset is loaded.
        /// </summary>
        Dictionary<UID, const DeserializeStream*> ObjectsDataCache;

        /// <summary>
        /// The objects cache maps the id of the object contained in the prefab asset (actor or script) to the default instance deserialized from prefab data. Valid only if asset is loaded and GetDefaultInstance was called.
        /// </summary>
        Dictionary<UID, SceneObject*> ObjectsCache;

        Prefab();
        Prefab(const AssetInfo* info);

    public:
        /// <summary>
        /// Gets the root object identifier (prefab object ID). Asset must be loaded.
        /// </summary>
        UID GetRootObjectId() const;

        /// <summary>
        /// Requests the default prefab object instance. Deserializes the prefab objects from the asset. Skips if already done.
        /// </summary>
        /// <returns>The root of the prefab object loaded from the prefab. Contains the default values. It's not added to gameplay but deserialized with postLoad and init event fired.</returns>
        Actor* GetDefaultInstance();

        /// <summary>
        /// Requests the default prefab object instance. Deserializes the prefab objects from the asset. Skips if already done.
        /// </summary>
        /// <param name="objectId">The ID of the object to get from prefab default object. It can be one of the child-actors or any script that exists in the prefab. Methods returns root if id is empty.</param>
        /// <returns>The object of the prefab loaded from the prefab. Contains the default values. It's not added to gameplay but deserialized with postLoad and init event fired.</returns>
        SceneObject* GetDefaultInstance(const UID& objectId);

#if SE_EDITOR
        /// <summary>
        /// Applies the difference from the prefab object instance, saves the changes and synchronizes them with the active instances of the prefab asset.
        /// </summary>
        /// <remarks>
        /// Applies all the changes from not only the given actor instance but all actors created within that prefab instance.
        /// </remarks>
        /// <param name="targetActor">The root actor of spawned prefab instance to use as modified changes sources.</param>
        bool ApplyAll(Actor* targetActor);
#endif

    private:
#if SE_EDITOR
        typedef List<class PrefabInstanceData> PrefabInstancesData;
        typedef List<AssetRef<Prefab>> NestedPrefabsList;
        bool ApplyAllInternal(Actor* targetActor, bool linkTargetActorObjectToPrefab, PrefabInstancesData& prefabInstancesData);
        bool UpdateInternal(const List<SceneObject*>& defaultInstanceObjects, Json::StringBuffer& tmpBuffer);
        bool SyncChangesInternal(PrefabInstancesData& prefabInstancesData);
        void SyncNestedPrefabs(const NestedPrefabsList& allPrefabs, List<PrefabInstancesData>& allPrefabsInstancesData) const;
#endif
        void DeleteDefaultInstance();

    protected:
        // [JsonAssetBase]
        LoadResult ProcessLoadAsset() override;
        void Unload(bool isReloading) override;
    };
} // SE

