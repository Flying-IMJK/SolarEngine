#pragma once

#include "Core/Math/Vector3.h"
#include "Runtime/Render/Assets/Material/MaterialSlot.h"
#include "Runtime/Resource/BinaryAsset.h"
#include "Runtime/Resource/Streaming/StreamableResource.h"

namespace SE
{
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
    /// Base class for asset types that can contain a model resource.
    /// </summary>
    class SE_API_RUNTIME ModelBase : public BinaryAsset, public StreamableResource
    {
        SE_CLASS_DEFAULT(ModelBase, BinaryAsset);

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
        explicit ModelBase(const AssetInfo* info, StreamingGroup* group)
            : BinaryAsset(info)
            , StreamableResource(group)
        {
        }

    public:
        /// <summary>
        /// The minimum screen size to draw this model (the bottom limit). Used to cull small models. Set to 0 to disable this feature.
        /// </summary>
        float MinScreenSize = 0.0f;

        /// <summary>
        /// The list of material slots.
        /// </summary>
        List<MaterialSlot> MaterialSlots;

        /// <summary>
        /// Gets the amount of the material slots used by this model asset.
        /// </summary>
        int32 GetMaterialSlotsCount() const
        {
            return MaterialSlots.Count();
        }

        /// <summary>
        /// Resizes the material slots collection. Updates meshes that were using removed slots.
        /// </summary>
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
        virtual int32 GetLODsCount() const = 0;

        /// <summary>
        /// Gets the meshes for a particular LOD index.
        /// </summary>
        virtual void GetMeshes(List<MeshBase*>& meshes, int32 lodIndex = 0) = 0;
    };
}
