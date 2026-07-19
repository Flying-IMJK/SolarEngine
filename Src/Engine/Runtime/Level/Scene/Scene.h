#pragma once

#include "../Actor.h"
#include "../../Render/SceneRendering.h"

#include "SceneTicking.h"

namespace SE
{
    /// <summary>
    /// The scene root object that contains a hierarchy of actors.
    /// </summary>
    SE_CLASS(Reflect)
    class SE_API_RUNTIME Scene final : public Actor
    {
        friend class Level;
        friend class LevelSystem;

        SE_DEFINE_CLASS(Scene, Actor);

    public:
        Scene();
        ~Scene() override;

        /// <summary>
        /// The scene metadata.
        /// </summary>
        // SceneInfo Info;

        /// <summary>
        /// The scene rendering manager.
        /// </summary>
        SceneRendering Rendering;

        /// <summary>
        /// The scene ticking manager.
        /// </summary>
        SceneTicking Ticking;

        /// <summary>
        /// The navigation data.
        /// </summary>
        // SceneNavigation Navigation;

        /// <summary>
        /// The static light manager for this scene.
        /// </summary>
        // SceneLightmapsData LightmapsData;

        /// <summary>
        /// The CSG data container for this scene.
        /// </summary>
        // CSG::SceneCSGData CSGData;

        /// <summary>
        /// Gets the lightmap settings (per scene).
        /// </summary>
        // LightmapSettings GetLightmapSettings() const;

        /// <summary>
        /// Sets the lightmap settings (per scene).
        /// </summary>
        // void SetLightmapSettings(const LightmapSettings& value);

    public:
        /// <summary>
        /// Removes all baked lightmap textures from the scene.
        /// </summary>
        // void ClearLightmaps();

        /// <summary>
        /// Builds the CSG geometry for the given scene.
        /// </summary>
        /// <remarks>Requests are enqueued till the next game scripts update.</remarks>
        /// <param name="timeoutMs">The timeout to wait before building CSG (in milliseconds).</param>
        // void BuildCSG(float timeoutMs = 50);

#if SE_EDITOR

        /// <summary>
        /// Gets path to the scene file
        /// </summary>
        String GetPath() const;

        /// <summary>
        /// Gets filename of the scene file
        /// </summary>
        String GetFilename() const;

        /// <summary>
        /// Gets path to the scene data folder
        /// </summary>
        String GetDataFolderPath() const;

        /// <summary>
        /// Gets the asset references (scene asset). Supported only in Editor.
        /// </summary>
        /// <seealso cref="Asset.GetReferences"/>
        /// <returns>The collection of the asset ids referenced by this asset.</returns>
        List<UID, HeapAllocation> GetAssetReferences() const;

#endif

    private:
        // MeshCollider* TryGetCsgCollider();
        // StaticModel* TryGetCsgModel();
        // void CreateCsgCollider();
        // void CreateCsgModel();
        // void OnCsgCollisionDataChanged();
        // void OnCsgModelChanged();
#if COMPILE_WITH_CSG_BUILDER
        void OnCSGBuildEnd();
#endif

    public:
        // [Actor]
        void Serialize(SerializeContext& context) override;
        void Deserialize(DeserializeContext& context) override;
        void OnDeleteObject() override;
        // void EndPlay() override;

    protected:
        // [Actor]
        void Initialize() override;
        // void BeginPlay(SceneBeginData* data) override;
        void OnTransformChanged() override;
    };
} // SE
