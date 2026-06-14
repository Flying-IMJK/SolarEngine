#pragma once

#include "Mesh.h"
#include "ModelBase.h"
#include "ModelLOD.h"

#include "Core/Math/BoundingVolumes.h"
#include "Runtime/Render/RenderEnum.h"

namespace SE
{
    class GPUContext;
    class MaterialBase;
    struct RenderContext;
    // Maximum amount of levels of detail for the model
    #define MODEL_MAX_LODS 6
    #define MODEL_MAX_MESHES 4096

    struct Ray;
    class Mesh;
    class StreamModelLODTask;

    #define MODE_VERSION 1

    /// <summary>
    /// Model asset that contains model object made of meshes which can rendered on the GPU.
    /// </summary>
    SE_CLASS(Reflect, API, NoSpawn)
    class SE_API_RUNTIME Model : public ModelBase
    {
        friend Mesh;
        friend StreamModelLODTask;

        SE_DEFINE_CLASS_DEFAULT(Model, ModelBase);
        ASSET_HEADER(Model);

    private:
        int32 m_LoadedLODs = 0;
        StreamModelLODTask* m_StreamingTask = nullptr;

    public:
        /// <summary>
        /// Model level of details. The first entry is the highest quality LOD0 followed by more optimized versions.
        /// </summary>
        List<ModelLOD, FixedAllocation<MODEL_MAX_LODS>> LODs;

        /// <summary>
        /// The generated Sign Distant Field (SDF) for this model (merged all meshes). Use GenerateSDF to update it.
        /// </summary>
        // SDFData SDF;

    public:
        /// <summary>
        /// Finalizes an instance of the <see cref="Model"/> class.
        /// </summary>
        ~Model() override;

    public:
        /// <summary>
        /// Gets a value indicating whether this instance is initialized.
        /// </summary>
        FORCE_INLINE bool IsInitialized() const
        {
            return LODs.HasItems();
        }

        /// <summary>
        /// Gets the amount of loaded model LODs.
        /// </summary>
        FORCE_INLINE int32 GetLoadedLODs() const
        {
            return m_LoadedLODs;
        }

        /// <summary>
        /// Clamps the index of the LOD to be valid for rendering (only loaded LODs).
        /// </summary>
        /// <param name="index">The index.</param>
        /// <returns>The resident LOD index.</returns>
        FORCE_INLINE int32 ClampLODIndex(int32 index) const
        {
            return Math::Clamp(index, HighestResidentLODIndex(), LODs.Count() - 1);
        }

        /// <summary>
        /// Gets index of the highest resident LOD (may be equal to LODs.Count if no LOD has been uploaded). Note: LOD=0 is the highest (top quality)
        /// </summary>
        FORCE_INLINE int32 HighestResidentLODIndex() const
        {
            return LODs.Count() - m_LoadedLODs;
        }

        /// <summary>
        /// Determines whether any LOD has been initialized.
        /// </summary>
        bool HasAnyLODInitialized() const;

        /// <summary>
        /// Determines whether this model can be rendered.
        /// </summary>
        FORCE_INLINE bool CanBeRendered() const
        {
            return m_LoadedLODs > 0;
        }

    public:
        /// <summary>
        /// Requests the LOD data asynchronously (creates task that will gather chunk data or null if already here).
        /// </summary>
        /// <param name="lodIndex">Index of the LOD.</param>
        /// <returns>Task that will gather chunk data or null if already here.</returns>
        AssetTask* RequestLODDataAsync(int32 lodIndex)
        {
            const int32 chunkIndex = MODEL_LOD_TO_CHUNK_INDEX(lodIndex);
            return RequestChunkDataAsync(chunkIndex);
        }

        /// <summary>
        /// Gets the model LOD data (links bytes).
        /// </summary>
        /// <param name="lodIndex">Index of the LOD.</param>
        /// <param name="data">The data (may be missing if failed to get it).</param>
        void GetLODData(int32 lodIndex, BytesContainer& data) const
        {
            const int32 chunkIndex = MODEL_LOD_TO_CHUNK_INDEX(lodIndex);
            GetChunkData(chunkIndex, data);
        }

    public:
        /// <summary>
        /// Determines if there is an intersection between the Model and a Ray in given world using given instance.
        /// </summary>
        /// <param name="ray">The ray to test</param>
        /// <param name="world">World to test</param>
        /// <param name="distance">When the method completes, contains the distance of the intersection (if any valid).</param>
        /// <param name="normal">When the method completes, contains the intersection surface normal vector (if any valid).</param>
        /// <param name="mesh">Mesh, or null</param>
        /// <param name="lodIndex">Level Of Detail index</param>
        /// <returns>True whether the two objects intersected</returns>
        bool Intersects(const Ray& ray, const Matrix& world, float& distance, Float3& normal, Mesh** mesh, int32 lodIndex = 0);

        /// <summary>
        /// Determines if there is an intersection between the Model and a Ray in given world using given instance.
        /// </summary>
        /// <param name="ray">The ray to test</param>
        /// <param name="transform">The instance transformation.</param>
        /// <param name="distance">When the method completes, contains the distance of the intersection (if any valid).</param>
        /// <param name="normal">When the method completes, contains the intersection surface normal vector (if any valid).</param>
        /// <param name="mesh">Mesh, or null</param>
        /// <param name="lodIndex">Level Of Detail index</param>
        /// <returns>True whether the two objects intersected</returns>
        bool Intersects(const Ray& ray, const Transform& transform, float& distance, Float3& normal, Mesh** mesh, int32 lodIndex = 0);

        /// <summary>
        /// Gets the model bounding box in custom matrix world space.
        /// </summary>
        /// <param name="world">The transformation matrix.</param>
        /// <param name="lodIndex">The Level Of Detail index.</param>
        /// <returns>The bounding box.</returns>
        BoundingBox GetBox(const Matrix& world, int32 lodIndex = 0) const;

        /// <summary>
        /// Gets the model bounding box in custom transformation.
        /// </summary>
        /// <param name="transform">The instance transformation.</param>
        /// <param name="lodIndex">The Level Of Detail index.</param>
        /// <returns>The bounding box.</returns>
        BoundingBox GetBox(const Transform& transform, int32 lodIndex = 0) const;

        /// <summary>
        /// Gets the model bounding box in local space.
        /// </summary>
        /// <param name="lodIndex">The Level Of Detail index.</param>
        /// <returns>The bounding box.</returns>
        BoundingBox GetBox(int32 lodIndex = 0) const;

    public:
        /// <summary>
        /// Draws the meshes. Binds vertex and index buffers and invokes the draw calls.
        /// </summary>
        /// <param name="context">GPU context to draw with.</param>
        /// <param name="lodIndex">The Level Of Detail index.</param>
        void Render(GPUContext* context, int32 lodIndex = 0);

        /// <summary>
        /// Draws the model.
        /// </summary>
        /// <param name="renderContext">The rendering context.</param>
        /// <param name="material">The material to use for rendering.</param>
        /// <param name="world">The world transformation of the model.</param>
        /// <param name="flags">The object static flags.</param>
        /// <param name="receiveDecals">True if rendered geometry can receive decals, otherwise false.</param>
        /// <param name="sortOrder">Object sorting key.</param>
        void Draw(const RenderContext& renderContext, MaterialBase* material, const Matrix& world, EnumFlags<StaticMask> flags = StaticMask::None, bool receiveDecals = true, int16 sortOrder = 0) const;

        /// <summary>
        /// Draws the model.
        /// </summary>
        /// <param name="renderContext">The rendering context.</param>
        /// <param name="info">The packed drawing info data.</param>
        void Draw(const RenderContext& renderContext, const Mesh::DrawInfo& info);

        /// <summary>
        /// Draws the model.
        /// </summary>
        /// <param name="renderContextBatch">The rendering context batch.</param>
        /// <param name="info">The packed drawing info data.</param>
        void Draw(const RenderContextBatch& renderContextBatch, const Mesh::DrawInfo& info);

    public:
        /// <summary>
        /// Setups the model LODs collection including meshes creation.
        /// </summary>
        /// <param name="meshesCountPerLod">The meshes count per lod array (amount of meshes per LOD).</param>
        /// <returns>True if failed, otherwise false.</returns>
        bool SetupLODs(const Span<int32>& meshesCountPerLod);

#if SE_EDITOR

        /// <summary>
        /// Saves this asset to the file. Supported only in Editor.
        /// </summary>
        /// <remarks>If you use saving with the GPU mesh data then the call has to be provided from the thread other than the main game thread.</remarks>
        /// <param name="withMeshDataFromGpu">True if save also GPU mesh buffers, otherwise will keep data in storage unmodified. Valid only if saving the same asset to the same location and it's loaded.</param>
        /// <param name="path">The custom asset path to use for the saving. Use empty value to save this asset to its own storage location. Can be used to duplicate asset. Must be specified when saving virtual asset.</param>
        /// <returns>True if cannot save data, otherwise false.</returns>
        bool Save(bool withMeshDataFromGpu = false, const StringView& path = StringView::Empty);

#endif

        /// <summary>
        /// Generates the Sign Distant Field for this model.
        /// </summary>
        /// <remarks>Can be called in async in case of SDF generation on a CPU (assuming model is not during rendering).</remarks>
        /// <param name="resolutionScale">The SDF texture resolution scale. Use higher values for more precise data but with significant performance and memory overhead.</param>
        /// <param name="lodIndex">The index of the LOD to use for the SDF building.</param>
        /// <param name="cacheData">If true, the generated SDF texture data will be cached on CPU (in asset chunk storage) to allow saving it later, otherwise it will be runtime for GPU-only. Ignored for virtual assets or in build.</param>
        /// <param name="backfacesThreshold">Custom threshold (in range 0-1) for adjusting mesh internals detection based on the percentage of test rays hit triangle backfaces. Use lower value for more dense mesh.</param>
        /// <returns>True if failed, otherwise false.</returns>
        // bool GenerateSDF(float resolutionScale = 1.0f, int32 lodIndex = 6, bool cacheData = true, float backfacesThreshold = 0.6f);

        /// <summary>
        /// Sets set SDF data (releases the current one).
        /// </summary>
        // void SetSDF(const SDFData& sdf);

    private:
        /// <summary>
        /// Initializes this model to an empty collection of LODs with meshes.
        /// </summary>
        /// <param name="meshesCountPerLod">The meshes count per lod array (amount of meshes per LOD).</param>
        /// <returns>True if failed, otherwise false.</returns>
        bool Init(const Span<int32>& meshesCountPerLod);

    public:
        // [ModelBase]
        void SetupMaterialSlots(int32 slotsCount) override;
        int32 GetLODsCount() const override;
        void GetMeshes(List<MeshBase*>& meshes, int32 lodIndex = 0) override;
        void InitAsVirtual() override;
        void CancelStreaming() override;
#if SE_EDITOR
        void GetReferences(List<UID>& output) const override;
#endif
        uint32 GetSerializedVersion() const override;

        // [StreamableResource]
        int32 GetMaxResidency() const override;
        int32 GetCurrentResidency() const override;
        int32 GetAllocatedResidency() const override;
        bool CanBeUpdated() const override;
        Threading::Task* UpdateAllocation(int32 residency) override;
        Threading::Task* CreateStreamingTask(int32 residency) override;
        void CancelStreamingTasks() override;

    protected:
        // [ModelBase]
        LoadResult load() override;
        void Unload(bool isReloading) override;
        bool OnInit(AssetInitData& initData) override;

    protected:
        AssetChunksFlag GetChunksToPreload() const override;
    };

} // SE
