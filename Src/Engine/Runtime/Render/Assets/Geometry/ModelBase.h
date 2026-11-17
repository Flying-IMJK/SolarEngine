#pragma once

#include <Runtime/Core/Math/BoundingVolumes.h>
#include <Runtime/Core/Math/Vector2.h>
#include <Runtime/Core/Math/Vector3.h>
#include <Runtime/Render/Assets/Material/MaterialSlot.h>
#include <Runtime/Resource/BinaryAsset.h>
#include <Runtime/Resource/Streaming/StreamableResource.h>

namespace SE
{
    class StreamModelLODTask;
    // Note: we use the first chunk as a header, next is the highest quality lod and then lower ones
    //
    // Example:
    // Chunk 0: Header
    // Chunk 1: LOD0
    // Chunk 2: LOD1
    // ..
    // Chunk 15: SDF
    #define MODEL_LOD_TO_CHUNK_INDEX(lod) (lod + 1)

    class GPUTexture;
    class MeshBase;


    /// <summary>
    /// Base class for mesh LOD objects. Contains a collection of the meshes.
    /// </summary>
    SE_CLASS(API(Abstract, NoSpawn)) 
    class SE_API_RUNTIME ModelLODBase : public ScriptingObject
    {
        SCRIPTING_TYPE_MIN(ModelLODBase);

    protected:
        int32 m_LodIndex = 0;

        explicit ModelLODBase(const SpawnParams& params): ScriptingObject(params)
        {
        }

    public:
        /// <summary>
        /// The screen size to switch LODs. Bottom limit of the model screen size to render this LOD.
        /// </summary>
        SE_FIELD(API()) float ScreenSize = 1.0f;

        /// <summary>
        /// Gets the model LOD index.
        /// </summary>
        SE_FUNCTION(API(Prop))
        FORCE_INLINE int32 GetLODIndex() const
        {
            return m_LodIndex;
        }

        /// <summary>
        /// Determines whether any mesh has been initialized.
        /// </summary>
        bool HasAnyMeshInitialized() const;

    public:
        /// <summary>
        /// Gets the bounding box combined for all meshes in this model LOD.
        /// </summary>
        SE_FUNCTION(API()) BoundingBox GetBox() const;

        /// <summary>
        /// Get model bounding box in transformed world matrix.
        /// </summary>
        /// <param name="world">World matrix</param>
        /// <returns>Bounding box</returns>
        BoundingBox GetBox(const Matrix& world) const;

        /// <summary>
        /// Get model bounding box in transformed world.
        /// </summary>
        /// <param name="transform">The instance transformation.</param>
        /// <param name="deformation">The meshes deformation container (optional).</param>
        /// <returns>Bounding box</returns>
        BoundingBox GetBox(const Transform& transform, const class MeshDeformation* deformation = nullptr) const;

    public:
        /// <summary>
        /// Gets the amount of meshes in this LOD.
        /// </summary>
        virtual int32 GetMeshesCount() const = 0;

        /// <summary>
        /// Gets the specific mesh in this LOD.
        /// </summary>
        virtual const MeshBase* GetMesh(int32 index) const = 0;

        /// <summary>
        /// Gets the specific mesh in this LOD.
        /// </summary>
        SE_FUNCTION(API(Sealed)) virtual MeshBase* GetMesh(int32 index) = 0;

        /// <summary>
        /// Gets the meshes in this LOD.
        /// </summary>
        virtual void GetMeshes(List<const MeshBase*>& meshes) const = 0;

        /// <summary>
        /// Gets the meshes in this LOD.
        /// </summary>
        SE_FUNCTION(API(Sealed)) 
        virtual void GetMeshes(SE_PARAM(API(Out)) List<MeshBase*>& meshes) = 0;
    };


    /// <summary>
    /// Base class for asset types that can contain a model resource.
    /// </summary>
    SE_CLASS(Reflect, API(NoSpawn))
    class SE_API_RUNTIME ModelBase : public BinaryAsset, public StreamableResource
    {
        SE_DEFINE_CLASS(ModelBase, BinaryAsset);
        ASSET_HEADER(ModelBase);
    public:
        /// <summary>
        /// The Sign Distant Field (SDF) data for the model.
        /// </summary>
        struct SDFData
        {
            /// <summary>
            /// The SDF volume texture (merged all meshes).
            /// </summary>
            GPUTexture* Texture = nullptr;

            /// <summary>
            /// The transformation scale from model local-space to the generated SDF texture space (local-space -> uv).
            /// </summary>
            Float3 LocalToUVWMul;

            /// <summary>
            /// Amount of world-units per SDF texture voxel.
            /// </summary>
            float WorldUnitsPerVoxel;

            /// <summary>
            /// The transformation offset from model local-space to the generated SDF texture space (local-space -> uv).
            /// </summary>
            Float3 LocalToUVWAdd;

            /// <summary>
            /// The maximum distance stored in the SDF texture. Used to rescale normalized SDF into world-units (in model local space).
            /// </summary>
            float MaxDistance;

            /// <summary>
            /// The bounding box of the SDF texture in the model local-space.
            /// </summary>
            Float3 LocalBoundsMin;

            /// <summary>
            /// The SDF texture resolution scale used for building texture.
            /// </summary>
            float ResolutionScale = 1.0f;

            /// <summary>
            /// The bounding box of the SDF texture in the model local-space.
            /// </summary>
            Float3 LocalBoundsMax;

            /// <summary>
            /// The model LOD index used for the building.
            /// </summary>
            int32 LOD = 6;
        };

    protected:
        int32 m_LoadedLODs = 0;
        StreamModelLODTask* m_StreamingTask = nullptr;

        explicit ModelBase(const SpawnParams& params, const AssetInfo* info, StreamingGroup* group) : BinaryAsset(params, info), StreamableResource(group)
        {
        }

    public:
        /// <summary>
        /// The minimum screen size to draw this model (the bottom limit). Used to cull small models. Set to 0 to disable this feature.
        /// </summary>
        SE_FIELD(API())
        float MinScreenSize = 0.0f;

        /// <summary>
        /// The list of material slots.
        /// </summary>
        SE_FIELD(API(ReadOnly))
        List<MaterialSlot> MaterialSlots;

        /// <summary>
        /// Gets the amount of the material slots used by this model asset.
        /// </summary>
        /// Query after loading, on the thread that owns asset edits. Does not wait for loading.
        SE_FUNCTION(API())
        int32 GetMaterialSlotsCount() const
        {
            return MaterialSlots.Count();
        }

        /// <summary>
        /// Gets a borrowed material reference from a valid slot of a loaded model, or null for an empty slot.
        /// Call on the thread that owns asset edits; keep the model alive while querying.
        /// </summary>
        SE_FUNCTION(API())
        MaterialBase* GetMaterial(int32 slotIndex) const;

        SE_FUNCTION(API())
        void SetMaterial(int32 slotIndex, MaterialBase* material);

        /// <summary>
        /// Clamps the index of the LOD to be valid for rendering (only loaded LODs).
        /// </summary>
        /// <param name="index">The index.</param>
        /// <returns>The resident LOD index.</returns>
        FORCE_INLINE int32 ClampLODIndex(int32 index) const
        {
            return Math::Clamp(index, HighestResidentLODIndex(), GetLODsCount() - 1);
        }

        /// <summary>
        /// Gets index of the highest resident LOD (it may be equal to LODs.Count if no LOD has been uploaded). Note: LOD=0 is the highest (top quality)
        /// </summary>
        FORCE_INLINE int32 HighestResidentLODIndex() const
        {
            return GetLODsCount() - m_LoadedLODs;
        }

        /// <summary>
        /// Gets the amount of loaded model LODs.
        /// </summary>
        SE_FUNCTION(API(Prop)) 
        FORCE_INLINE int32 GetLoadedLODs() const
        {
            return m_LoadedLODs;
        }

        /// <summary>
        /// Determines whether this model can be rendered.
        /// </summary>
        FORCE_INLINE bool CanBeRendered() const
        {
            return m_LoadedLODs > 0;
        }

        /// <summary>
        /// Resizes the material slots collection. Updates meshes that were using removed slots.
        /// </summary>
        SE_FUNCTION(API())
        virtual void SetupMaterialSlots(int32 slotsCount);

        /// <summary>
        /// Gets the material slot by the name.
        /// </summary>
        /// <param name="name">The slot name.</param>
        /// <returns>The material slot with the given name or null if cannot find it (asset may be not loaded yet).</returns>
        MaterialSlot* GetSlot(const StringView& name);

        /// <summary>
        /// Gets amount of the level of details in the model.
        /// </summary>
        /// Query after loading, on the thread that owns asset edits. Does not wait for loading.
        SE_FUNCTION(API())
        virtual int32 GetLODsCount() const = 0;

        /// <summary>
        /// Gets the mesh for a particular LOD index.
        /// </summary>
        virtual const ModelLODBase* GetLOD(int32 lodIndex) const = 0;

        /// <summary>
        /// Gets the mesh for a particular LOD index.
        /// </summary>
        SE_FUNCTION(API(Sealed))
        virtual ModelLODBase* GetLOD(int32 lodIndex) = 0;

        /// <summary>
        /// Gets the mesh for a particular LOD index.
        /// </summary>
        virtual const MeshBase* GetMesh(int32 meshIndex, int32 lodIndex = 0) const = 0;

        /// <summary>
        /// Gets the mesh for a particular LOD index.
        /// </summary>
        SE_FUNCTION(API(Sealed))
        virtual MeshBase* GetMesh(int32 meshIndex, int32 lodIndex = 0) = 0;

        /// <summary>
        /// Gets the meshes for a particular LOD index.
        /// </summary>
        virtual void GetMeshes(List<const MeshBase*>& meshes, int32 lodIndex = 0) const = 0;

        /// <summary>
        /// Gets the meshes for a particular LOD index.
        /// </summary>
        SE_FUNCTION(API())
        virtual void GetMeshes(SE_PARAM(API(Out)) List<MeshBase*>& meshes, int32 lodIndex = 0) = 0;
    };
}
