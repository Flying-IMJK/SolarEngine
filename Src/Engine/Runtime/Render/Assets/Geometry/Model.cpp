
#include "Model.h"

#include "ModelInstanceEntry.h"
#include "ModelLOD.h"

#include "Core/Logging/Exceptions/ArgumentOutOfRangeException.h"
#include "Core/Serialization/MemoryReadStream.h"
#include "Core/Serialization/MemoryWriteStream.h"
#include "Core/Thread/Task.h"
#include "Core/Thread/ThreadPool.h"

#include "Runtime/Engine.h"
#include "Runtime/Graphics/GPUDevice.h"
#include "Runtime/Render/RenderContext.h"
#include "Runtime/Render/RenderDrawCall.h"
#include "Runtime/Render/Utils/RenderUtils.h"
#include "Runtime/Resource/AssetRef.h"
#include "Runtime/Resource/Streaming/StreamingGroup.h"
#include "Runtime/Resource/Factories/BinaryAssetFactory.h"
#include "Runtime/Resource/Loading/AssetTask.h"

namespace SE
{

    class StreamModelLODTask final : public Threading::ThreadPoolTask
    {
    private:
        WeakAssetRef<Model> _asset;
        int32 _lodIndex;
        Storage::LockData _dataLock;

    public:
        StreamModelLODTask(Model* model, int32 lodIndex)
            : _asset(model)
            , _lodIndex(lodIndex)
            , _dataLock(model->storage->Lock())
        {
        }

    public:
        bool HasReference(void* resource) const override
        {
            return _asset == resource;
        }

        bool Run() override
        {
            AssetRef<Model> model = _asset.Get();
            if (model == nullptr)
                return false;

            // Get data
            BytesContainer data;
            model->GetLODData(_lodIndex, data);
            if (data.IsInvalid())
            {
                LOG_WARNING("Render", "Missing data chunk");
                return false;
            }
            MemoryReadStream stream(data.Get(), data.Length());

            // Note: this is running on thread pool task so we must be sure that updated LOD is not used at all (for rendering)

            // Load model LOD (initialize vertex and index buffers)
            if (!model->LODs[_lodIndex].Load(stream))
            {
                LOG_WARNING("Render", "Cannot load LOD{1} for model \'{0}\'", model->ToString(), _lodIndex);
                return false;
            }

            // Update residency level
            model->m_LoadedLODs++;
            model->ResidencyChanged();

            return true;
        }

        void OnEnd() override
        {
            // Unlink
            if (_asset)
            {
                ENGINE_ASSERT(_asset->m_StreamingTask == this);
                _asset->m_StreamingTask = nullptr;
                _asset = nullptr;
            }
            _dataLock.Release();

            // Base
            Threading::ThreadPoolTask::OnEnd();
        }
    };

    BINARY_ASSET_FACTORY(Model, false);


    Model::Model(const AssetInfo* info)
        : ModelBase(info, StreamingGroups::Instance().Models())
    {
        /*if (EnableModelSDF == 0 && GPUDevice::instance)
        {
            const bool enable = GPUDevice::instance->GetFeatureLevel() >= FeatureLevel::SM5;
            EnableModelSDF = enable ? 1 : 2;
        }*/
    }

    Model::~Model()
    {
        ENGINE_ASSERT(m_StreamingTask == nullptr);
    }

    bool Model::HasAnyLODInitialized() const
    {
        return LODs.HasItems() && LODs.Last().HasAnyMeshInitialized();
    }
    
    bool Model::Intersects(const Ray& ray, const Matrix& world, float& distance, Float3& normal, Mesh** mesh, int32 lodIndex)
    {
        return LODs[lodIndex].Intersects(ray, world, distance, normal, mesh);
    }

    bool Model::Intersects(const Ray& ray, const Transform& transform, float& distance, Float3& normal, Mesh** mesh, int32 lodIndex)
    {
        return LODs[lodIndex].Intersects(ray, transform, distance, normal, mesh);
    }

    BoundingBox Model::GetBox(const Matrix& world, int32 lodIndex) const
    {
        return LODs[lodIndex].GetBox(world);
    }

    BoundingBox Model::GetBox(int32 lodIndex) const
    {
        return LODs[lodIndex].GetBox();
    }

    void Model::Render(GPUContext* context, int32 lodIndex)
    {
        LODs[lodIndex].Render(context);
    }

    void Model::Draw(const RenderContext& renderContext, MaterialBase* material, const Matrix& world, EnumFlags<StaticMask> flags, bool receiveDecals, int16 sortOrder) const
    {
        if (!CanBeRendered())
            return;

        // Select a proper LOD index (model may be culled)
        const BoundingBox box = GetBox(world);
        BoundingSphere sphere;
        BoundingSphere::FromBox(box, sphere);
        int32 lodIndex = RenderUtils::ComputeModelLOD(this, sphere.Center - renderContext.view.Origin, (float)sphere.Radius, renderContext);
        if (lodIndex == -1)
        {
            return;
        }
        lodIndex += renderContext.view.ModelLODBias;
        lodIndex = ClampLODIndex(lodIndex);

        // Draw
        LODs[lodIndex].Draw(renderContext, material, world, flags, receiveDecals, DrawPass::Default, 0, sortOrder);
    }

    template<typename ContextType>
    FORCE_INLINE void ModelDraw(Model* model, const RenderContext& renderContext, const ContextType& context, const Mesh::DrawInfo& info)
    {
        ENGINE_ASSERT(info.Buffer);
        if (!model->CanBeRendered())
            return;
        const auto frame = Engine::FrameCount;
        const auto modelFrame = info.DrawState->PrevFrame + 1;
        if (info.Buffer->IsValidFor(model) == false)
        {
            info.Buffer->Setup(model);
        }

        // Select a proper LOD index (model may be culled)
        int32 lodIndex;
        if (info.ForcedLOD != -1)
        {
            lodIndex = info.ForcedLOD;
        }
        else
        {
            lodIndex = RenderUtils::ComputeModelLOD(model, info.Bounds.Center, static_cast<float>(info.Bounds.Radius), renderContext);
            if (lodIndex == -1)
            {
                // Handling model fade-out transition
                if (modelFrame == frame && info.DrawState->PrevLOD != -1 && !renderContext.view.IsSingleFrame)
                {
                    // Check if start transition
                    if (info.DrawState->LODTransition == 255)
                    {
                        info.DrawState->LODTransition = 0;
                    }

                    RenderUtils::UpdateModelLODTransition(info.DrawState->LODTransition);

                    // Check if end transition
                    if (info.DrawState->LODTransition == 255)
                    {
                        info.DrawState->PrevLOD = lodIndex;
                    }
                    else
                    {
                        const auto prevLOD = model->ClampLODIndex(info.DrawState->PrevLOD);
                        const float normalizedProgress = static_cast<float>(info.DrawState->LODTransition) * (1.0f / 255.0f);
                        model->LODs.Get()[prevLOD].Draw(renderContext, info, normalizedProgress);
                    }
                }

                return;
            }
        }
        lodIndex += info.LODBias + renderContext.view.ModelLODBias;
        lodIndex = model->ClampLODIndex(lodIndex);

        if (renderContext.view.IsSingleFrame)
        {
        }
        // Check if it's the new frame and could update the drawing state (note: model instance could be rendered many times per frame to different viewports)
        else if (modelFrame == frame)
        {
            // Check if start transition
            if (info.DrawState->PrevLOD != lodIndex && info.DrawState->LODTransition == 255)
            {
                info.DrawState->LODTransition = 0;
            }

            RenderUtils::UpdateModelLODTransition(info.DrawState->LODTransition);

            // Check if end transition
            if (info.DrawState->LODTransition == 255)
            {
                info.DrawState->PrevLOD = lodIndex;
            }
        }
        // Check if there was a gap between frames in drawing this model instance
        else if (modelFrame < frame || info.DrawState->PrevLOD == -1)
        {
            // Reset state
            info.DrawState->PrevLOD = lodIndex;
            info.DrawState->LODTransition = 255;
        }

        // Draw
        if (info.DrawState->PrevLOD == lodIndex || renderContext.view.IsSingleFrame)
        {
            model->LODs.Get()[lodIndex].Draw(context, info, 0.0f);
        }
        else if (info.DrawState->PrevLOD == -1)
        {
            const float normalizedProgress = static_cast<float>(info.DrawState->LODTransition) * (1.0f / 255.0f);
            model->LODs.Get()[lodIndex].Draw(context, info, 1.0f - normalizedProgress);
        }
        else
        {
            const auto prevLOD = model->ClampLODIndex(info.DrawState->PrevLOD);
            const float normalizedProgress = static_cast<float>(info.DrawState->LODTransition) * (1.0f / 255.0f);
            model->LODs.Get()[prevLOD].Draw(context, info, normalizedProgress);
            model->LODs.Get()[lodIndex].Draw(context, info, normalizedProgress - 1.0f);
        }
    }

    void Model::Draw(const RenderContext& renderContext, const Mesh::DrawInfo& info)
    {
        ModelDraw(this, renderContext, renderContext, info);
    }

    void Model::Draw(const RenderContextBatch& renderContextBatch, const Mesh::DrawInfo& info)
    {
        ModelDraw(this, renderContextBatch.GetMainContext(), renderContextBatch, info);
    }

    bool Model::SetupLODs(const Span<int32>& meshesCountPerLod)
    {
        Threading::ScopeLock lock(Locker);

        // Validate input and state
        if (!IsVirtual())
        {
            LOG_ERROR("Render", "Only virtual models can be updated at runtime.");
            return true;
        }

        return Init(meshesCountPerLod);
    }

#if SE_EDITOR

    bool Model::Save(bool withMeshDataFromGpu, const StringView& path)
    {
        // Validate state
        if (WaitForLoaded())
        {
            LOG_ERROR("Render", "Asset loading failed. Cannot save it.");
            return false;
        }
        if (IsVirtual() && path.IsEmpty())
        {
            LOG_ERROR("Render", "To save virtual asset asset you need to specify the target asset path location.");
            return false;
        }
        if (withMeshDataFromGpu && Threading::IsMainThread())
        {
            LOG_ERROR("Render", "To save model with GPU mesh buffers it needs to be called from the other thread (not the main thread).");
            return false;
        }
        if (IsVirtual() && !withMeshDataFromGpu)
        {
            LOG_ERROR("Render", "To save virtual model asset you need to specify 'withMeshDataFromGpu' (it has no other storage container to get data).");
            return false;
        }

        Threading::ScopeLock lock(Locker);

        // Create model data header
        MemoryWriteStream headerStream(1024);
        {
            MemoryWriteStream* stream = &headerStream;
            // Min Screen Size
            stream->WriteFloat(MinScreenSize);

            // Amount of material slots
            stream->WriteInt32(MaterialSlots.Count());

            // For each material slot
            for (int32 materialSlotIndex = 0; materialSlotIndex < MaterialSlots.Count(); materialSlotIndex++)
            {
                auto& slot = MaterialSlots[materialSlotIndex];

                const auto id = slot.Material.GetID();
                stream->Write(id);
                stream->WriteByte(slot.ShadowsMode.Get());
                stream->WriteString(slot.Name, 11);
            }

            // Amount of LODs
            const int32 lods = LODs.Count();
            stream->WriteByte(lods);

            // For each LOD
            for (int32 lodIndex = 0; lodIndex < lods; lodIndex++)
            {
                auto& lod = LODs[lodIndex];

                // Screen Size
                stream->WriteFloat(lod.ScreenSize);

                // Amount of meshes
                const int32 meshes = lod.Meshes.Count();
                stream->WriteUint16(meshes);

                // For each mesh
                for (int32 meshIndex = 0; meshIndex < meshes; meshIndex++)
                {
                    const auto& mesh = lod.Meshes[meshIndex];

                    // Material Slot index
                    stream->WriteInt32(mesh.GetMaterialSlotIndex());

                    // Box
                    const BoundingBox box = mesh.GetBox();
                    stream->WriteVector3(box.Maximum);
                    stream->WriteVector3(box.Minimum);

                    // Sphere
                    const BoundingSphere sphere = mesh.GetSphere();
                    stream->WriteVector3(sphere.Center);
                    stream->WriteFloat(sphere.Radius);

                    // Has Lightmap UVs
                    stream->WriteBool(mesh.HasLightmapUVs());
                }
            }
        }

        // Use a temporary chunks for data storage for virtual assets
        StorageChunk* tmpChunks[ASSET_FILE_DATA_CHUNKS];
        Platform::MemoryClear(tmpChunks, sizeof(tmpChunks));
        List<StorageChunk> chunks;
        if (IsVirtual())
        {
            chunks.Resize(ASSET_FILE_DATA_CHUNKS);
        }

#define GET_CHUNK(index) (IsVirtual() ? tmpChunks[index] = &chunks[index] : GetOrCreateChunk(index))

        // Check if use data from drive or from GPU
        if (withMeshDataFromGpu)
        {
            // Download all meshes buffers
            List<Threading::Task*> tasks;
            for (int32 lodIndex = 0; lodIndex < LODs.Count(); lodIndex++)
            {
                auto& lod = LODs[lodIndex];

                const int32 meshesCount = lod.Meshes.Count();
                struct MeshData
                {
                    BytesContainer VB0;
                    BytesContainer VB1;
                    BytesContainer VB2;
                    BytesContainer IB;

                    uint32 DataSize() const
                    {
                        return VB0.Length() + VB1.Length() + VB2.Length() + IB.Length();
                    }
                };
                List<MeshData> meshesData;
                meshesData.Resize(meshesCount);
                tasks.EnsureCapacity(meshesCount * 4);

                for (int32 meshIndex = 0; meshIndex < meshesCount; meshIndex++)
                {
                    const auto& mesh = lod.Meshes[meshIndex];
                    auto& meshData = meshesData[meshIndex];

                    // Vertex Buffer 0 (required)
                    auto task = mesh.DownloadDataGPUAsync(MeshBufferType::Vertex0, meshData.VB0);
                    if (task == nullptr)
                    {
                        return false;
                    }
                    task->Start();
                    tasks.Add(task);

                    // Vertex Buffer 1 (required)
                    task = mesh.DownloadDataGPUAsync(MeshBufferType::Vertex1, meshData.VB1);
                    if (task == nullptr)
                    {
                        return false;
                    }
                    task->Start();
                    tasks.Add(task);

                    // Vertex Buffer 2 (optional)
                    task = mesh.DownloadDataGPUAsync(MeshBufferType::Vertex2, meshData.VB2);
                    if (task)
                    {
                        task->Start();
                        tasks.Add(task);
                    }

                    // Index Buffer (required)
                    task = mesh.DownloadDataGPUAsync(MeshBufferType::Index, meshData.IB);
                    if (task == nullptr)
                    {
                        return false;
                    }
                    task->Start();
                    tasks.Add(task);
                }

                // Wait for all
                if (!Threading::Task::WaitAll(tasks))
                {
                    return false;
                }
                tasks.Clear();

                // Create meshes data
                {
                    int32 dataSize = meshesCount * (2 * sizeof(uint32) + sizeof(bool));
                    for (int32 meshIndex = 0; meshIndex < meshesCount; meshIndex++)
                    {
                        dataSize += meshesData[meshIndex].DataSize();
                    }

                    MemoryWriteStream meshesStream(dataSize);

                    for (int32 meshIndex = 0; meshIndex < meshesCount; meshIndex++)
                    {
                        const auto& mesh = lod.Meshes[meshIndex];
                        const auto& meshData = meshesData[meshIndex];

                        uint32 vertices = mesh.GetVertexCount();
                        uint32 triangles = mesh.GetTriangleCount();
                        bool hasColors = meshData.VB2.IsValid();
                        uint32 vb0Size = vertices * sizeof(VB0ElementType);
                        uint32 vb1Size = vertices * sizeof(VB1ElementType);
                        uint32 vb2Size = vertices * sizeof(VB2ElementType);
                        uint32 indicesCount = triangles * 3;
                        bool shouldUse16BitIndexBuffer = indicesCount <= Max_uint16;
                        bool use16BitIndexBuffer = mesh.Use16BitIndexBuffer();
                        uint32 ibSize = indicesCount * (use16BitIndexBuffer ? sizeof(uint16) : sizeof(uint32));

                        if (vertices == 0 || triangles == 0)
                        {
                            LOG_WARNING("Render", "Cannot save model with empty meshes.");
                            return false;
                        }
                        if (meshData.VB0.Length() < vb0Size)
                        {
                            LOG_WARNING("Render", "Invalid vertex buffer 0 size.");
                            return false;
                        }
                        if (meshData.VB1.Length() < vb1Size)
                        {
                            LOG_WARNING("Render", "Invalid vertex buffer 1 size.");
                            return false;
                        }
                        if (hasColors && meshData.VB2.Length() < vb2Size)
                        {
                            LOG_WARNING("Render", "Invalid vertex buffer 2 size.");
                            return false;
                        }
                        if (meshData.IB.Length() < ibSize)
                        {
                            LOG_WARNING("Render", "Invalid index buffer size.");
                            return false;
                        }

                        meshesStream.WriteUint32(vertices);
                        meshesStream.WriteUint32(triangles);

                        meshesStream.WriteBytes(meshData.VB0.Get(), vb0Size);
                        meshesStream.WriteBytes(meshData.VB1.Get(), vb1Size);

                        meshesStream.WriteBool(hasColors);

                        if (hasColors)
                        {
                            meshesStream.WriteBytes(meshData.VB2.Get(), vb2Size);
                        }

                        if (shouldUse16BitIndexBuffer == use16BitIndexBuffer)
                        {
                            meshesStream.WriteBytes(meshData.IB.Get(), ibSize);
                        }
                        else if (shouldUse16BitIndexBuffer)
                        {
                            auto ib = reinterpret_cast<const int32*>(meshData.IB.Get());
                            for (uint32 i = 0; i < indicesCount; i++)
                            {
                                meshesStream.WriteUint16(ib[i]);
                            }
                        }
                        else
                        {
                            LOG_FATAL("Render", "IndexBuffer use 16Bit but out of range in Max_uint16");
                        }
                    }

                    // Override LOD data chunk with the fetched GPU meshes memory
                    auto lodChunk = GET_CHUNK(MODEL_LOD_TO_CHUNK_INDEX(lodIndex));
                    if (lodChunk == nullptr)
                    {
                        return false;
                    }
                    lodChunk->Data.Copy(meshesStream.GetHandle(), meshesStream.GetPosition());
                }
            }

            // Download SDF data
            /*if (SDF.Texture)
            {
                auto sdfChunk = GET_CHUNK(15);
                if (sdfChunk == nullptr)
                {
                    return false;
                }
                MemoryWriteStream sdfStream;
                sdfStream.WriteInt32(1); // Version
                ModelSDFHeader data(SDF, SDF.Texture->GetDescription());
                sdfStream.WriteBytes(&data, sizeof(data));
                TextureData sdfTextureData;
                if (SDF.Texture->DownloadData(sdfTextureData))
                    return true;
                for (int32 mipLevel = 0; mipLevel < sdfTextureData.Items[0].Mips.Count(); mipLevel++)
                {
                    auto& mip = sdfTextureData.Items[0].Mips[mipLevel];
                    ModelSDFMip mipData(mipLevel, mip);
                    sdfStream.WriteBytes(&mipData, sizeof(mipData));
                    sdfStream.WriteBytes(mip.Data.Get(), mip.Data.Length());
                }
                sdfChunk->Data.Copy(sdfStream.GetHandle(), sdfStream.GetPosition());
            }*/
        }
        else
        {
            // Load all chunks with a mesh data
            for (int32 lodIndex = 0; lodIndex < LODs.Count(); lodIndex++)
            {
                if (!LoadChunk(MODEL_LOD_TO_CHUNK_INDEX(lodIndex)))
                {
                    return false;
                }
            }

            /*if (SDF.Texture)
            {
                // SDF data from file (only if has no cached texture data)
                if (!LoadChunk(15))
                {
                    return false;
                }
            }
            else*/
            {
                // No SDF texture
                ReleaseChunk(15);
            }
        }

        // Set mesh header data
        auto headerChunk = GET_CHUNK(0);
        ENGINE_ASSERT(headerChunk != nullptr);
        headerChunk->Data.Copy(headerStream.GetHandle(), headerStream.GetPosition());

#undef GET_CHUNK

        // Save
        AssetInitData data;
        data.SerializedVersion = 0;
        if (IsVirtual())
            Platform::MemoryCopy(m_Header.Chunks, tmpChunks, sizeof(m_Header.Chunks));
        const bool saveResult = path.HasChars() ? SaveAsset(path, data) : SaveAsset(data, true);
        if (IsVirtual())
            Platform::MemoryClear(m_Header.Chunks, sizeof(m_Header.Chunks));
        if (!saveResult)
        {
            LOG_ERROR("Render", "Cannot save \'{0}\'", ToString());
            return false;
        }

        return true;
    }

#endif

    /*bool Model::GenerateSDF(float resolutionScale, int32 lodIndex, bool cacheData, float backfacesThreshold)
    {
        if (EnableModelSDF == 2)
            return true; // Not supported
        Threading::ScopeLock lock(Locker);
        if (!HasAnyLODInitialized())
            return true;
        if (Threading::IsMainThread() && IsVirtual())
        {
            // TODO: could be supported if algorithm could run on a GPU and called during rendering
            LOG_WARNING("Render", "Cannot generate SDF for virtual models on a main thread.");
            return true;
        }
        lodIndex = Math::Clamp(lodIndex, HighestResidentLODIndex(), LODs.Count() - 1);

        // Generate SDF
#if SE_EDITOR
        cacheData &= Storage != nullptr; // Cache only if has storage linked
        MemoryWriteStream sdfStream;
        MemoryWriteStream* outputStream = cacheData ? &sdfStream : nullptr;
#else
        class MemoryWriteStream* outputStream = nullptr;
#endif
        if (ModelTool::GenerateModelSDF(this, nullptr, resolutionScale, lodIndex, &SDF, outputStream, GetPath(), backfacesThreshold))
            return true;

#if SE_EDITOR
        // Set asset data
        if (cacheData)
            GetOrCreateChunk(15)->Data.Copy(sdfStream.GetHandle(), sdfStream.GetPosition());
#endif

        return false;
    }

    void Model::SetSDF(const SDFData& sdf)
    {
        Threading::ScopeLock lock(Locker);
        if (SDF.Texture == sdf.Texture)
            return;
        SAFE_DELETE_GPU_RESOURCE(SDF.Texture);
        SDF = sdf;
        ReleaseChunk(15);
    }*/

    bool Model::Init(const Span<int32>& meshesCountPerLod)
    {
        if (meshesCountPerLod.IsInvalid() || meshesCountPerLod.Length() > MODEL_MAX_LODS)
        {
            Log::ArgumentOutOfRangeException();
            return true;
        }

        // Dispose previous data and disable streaming (will start data uploading tasks manually)
        StopStreaming();

        // Setup
        MaterialSlots.Resize(1);
        MinScreenSize = 0.0f;
        // SAFE_DELETE_GPU_RESOURCE(SDF.Texture);

        // Setup LODs
        for (int32 lodIndex = 0; lodIndex < LODs.Count(); lodIndex++)
            LODs[lodIndex].Dispose();
        LODs.Resize(meshesCountPerLod.Length());

        // Setup meshes
        for (int32 lodIndex = 0; lodIndex < meshesCountPerLod.Length(); lodIndex++)
        {
            auto& lod = LODs[lodIndex];
            lod._model = this;
            lod._lodIndex = lodIndex;
            lod.ScreenSize = 1.0f;
            const int32 meshesCount = meshesCountPerLod[lodIndex];
            if (meshesCount <= 0 || meshesCount > MODEL_MAX_MESHES)
                return true;

            lod.Meshes.Resize(meshesCount);
            for (int32 meshIndex = 0; meshIndex < meshesCount; meshIndex++)
            {
                lod.Meshes[meshIndex].Init(this, lodIndex, meshIndex, 0, BoundingBox::Zero, BoundingSphere::Empty, true);
            }
        }

        // Update resource residency
        m_LoadedLODs = meshesCountPerLod.Length();
        ResidencyChanged();

        return false;
    }

    void Model::SetupMaterialSlots(int32 slotsCount)
    {
        ModelBase::SetupMaterialSlots(slotsCount);

        // Adjust meshes indices for slots
        for (int32 lodIndex = 0; lodIndex < LODs.Count(); lodIndex++)
        {
            for (int32 meshIndex = 0; meshIndex < LODs[lodIndex].Meshes.Count(); meshIndex++)
            {
                auto& mesh = LODs[lodIndex].Meshes[meshIndex];
                if (mesh.GetMaterialSlotIndex() >= slotsCount)
                {
                    mesh.SetMaterialSlotIndex(slotsCount - 1);
                }
            }
        }
    }

    int32 Model::GetLODsCount() const
    {
        return LODs.Count();
    }

    void Model::GetMeshes(List<MeshBase*>& meshes, int32 lodIndex)
    {
        auto& lod = LODs[lodIndex];
        meshes.Resize(lod.Meshes.Count());
        for (int32 meshIndex = 0; meshIndex < lod.Meshes.Count(); meshIndex++)
        {
            meshes[meshIndex] = &lod.Meshes[meshIndex];
        }
    }

    void Model::InitAsVirtual()
    {
        // Init with a single LOD and one mesh
        int32 meshesCount = 1;
        Init(ToSpan(&meshesCount, 1));

        // Base
        BinaryAsset::InitAsVirtual();
    }

    void Model::CancelStreaming()
    {
        Asset::CancelStreaming();
        CancelStreamingTasks();
    }

#if SE_EDITOR

    void Model::GetReferences(List<UID>& output) const
    {
        // Base
        BinaryAsset::GetReferences(output);

        for (int32 i = 0; i < MaterialSlots.Count(); i++)
        {
            output.Add(MaterialSlots[i].Material.GetID());
        }
    }

    uint32 Model::GetSerializedVersion() const
    {
        return MODE_VERSION;
    }

#endif

    int32 Model::GetMaxResidency() const
    {
        return LODs.Count();
    }

    int32 Model::GetCurrentResidency() const
    {
        return m_LoadedLODs;
    }

    int32 Model::GetAllocatedResidency() const
    {
        return LODs.Count();
    }

    bool Model::CanBeUpdated() const
    {
        // Check if is ready and has no streaming tasks running
        return IsInitialized() && m_StreamingTask == nullptr;
    }

    Threading::Task* Model::UpdateAllocation(int32 residency)
    {
        // Models are not using dynamic allocation feature
        return nullptr;
    }

    Threading::Task* Model::CreateStreamingTask(int32 residency)
    {
        Threading::ScopeLock lock(Locker);

        ENGINE_ASSERT(IsInitialized() && Math::RangeInclusive(residency, 0, LODs.Count()) && m_StreamingTask == nullptr);
        Threading::Task* result = nullptr;
        const int32 lodCount = residency - GetCurrentResidency();

        // Switch if go up or down with residency
        if (lodCount > 0)
        {
            // Allow only to change LODs count by 1
            ENGINE_ASSERT(Math::Abs(lodCount) == 1);

            int32 lodIndex = HighestResidentLODIndex() - 1;

            // Request LOD data
            result = DynamicCast<Threading::Task>(RequestLODDataAsync(lodIndex));

            // Add upload data task
            m_StreamingTask = New<StreamModelLODTask>(this, lodIndex);
            if (result)
            {
                result->ContinueWith(m_StreamingTask);
            }
            else
            {
                result = m_StreamingTask;
            }
        }
        else
        {
            // Do the quick data release
            for (int32 i = HighestResidentLODIndex(); i < LODs.Count() - residency; i++)
            {
                LODs[i].Unload();
            }
            m_LoadedLODs = residency;
            ResidencyChanged();
        }

        return result;
    }

    void Model::CancelStreamingTasks()
    {
        if (m_StreamingTask)
        {
            m_StreamingTask->Cancel();
            ASSERT_LOW_LAYER(_streamingTask == nullptr);
        }
    }

    Asset::LoadResult Model::load()
    {
        // Get header chunk
        auto chunk0 = GetChunk(0);
        if (chunk0 == nullptr || !chunk0->IsValid())
            return LoadResult::MissingDataChunk;
        MemoryReadStream headerStream(chunk0->Get(), chunk0->Size());
        ReadStream* stream = &headerStream;

        // Min Screen Size
        stream->ReadFloat(&MinScreenSize);

        // Amount of material slots
        int32 materialSlotsCount;
        stream->ReadInt32(&materialSlotsCount);
        if (materialSlotsCount <= 0 || materialSlotsCount > 4096)
        {
            return LoadResult::InvalidData;
        }
        MaterialSlots.Resize(materialSlotsCount, false);

        // For each material slot
        for (int32 materialSlotIndex = 0; materialSlotIndex < materialSlotsCount; materialSlotIndex++)
        {
            auto& slot = MaterialSlots[materialSlotIndex];

            // Material
            UID materialId;
            stream->Read(materialId);
            slot.Material = materialId;

            // Shadows Mode
            slot.ShadowsMode = static_cast<ShadowsCastingMode>(stream->ReadByte());

            // Name
            stream->ReadString(&slot.Name, 11);
        }

        // Amount of LODs
        byte lods;
        stream->ReadByte(&lods);
        if (lods == 0 || lods > MODEL_MAX_LODS)
            return LoadResult::InvalidData;
        LODs.Resize(lods);

        // For each LOD
        for (int32 lodIndex = 0; lodIndex < lods; lodIndex++)
        {
            auto& lod = LODs[lodIndex];
            lod._model = this;
            lod._lodIndex = lodIndex;

            // Screen Size
            stream->ReadFloat(&lod.ScreenSize);

            // Amount of meshes
            uint16 meshesCount;
            stream->ReadUint16(&meshesCount);
            if (meshesCount == 0 || meshesCount > MODEL_MAX_MESHES)
            {
                return LoadResult::InvalidData;
            }

            ENGINE_ASSERT(lodIndex == 0 || LODs[0].Meshes.Count() >= meshesCount);

            // Allocate memory
            lod.Meshes.Resize(meshesCount, false);

            // For each mesh
            for (uint16 meshIndex = 0; meshIndex < meshesCount; meshIndex++)
            {
                // Material Slot index
                int32 materialSlotIndex;
                stream->ReadInt32(&materialSlotIndex);
                if (materialSlotIndex < 0 || materialSlotIndex >= materialSlotsCount)
                {
                    LOG_WARNING("Render", "Invalid material slot index {0} for mesh {1}. Slots count: {2}.", materialSlotIndex, meshIndex, materialSlotsCount);
                    return LoadResult::InvalidData;
                }

                // Box
                BoundingBox box;
                stream->ReadVector3(&box.Maximum);
                stream->ReadVector3(&box.Minimum);

                // Sphere
                BoundingSphere sphere;
                stream->ReadVector3(&sphere.Center);
                stream->ReadFloat(&sphere.Radius);

                // Has Lightmap UVs
                bool hasLightmapUVs = stream->ReadBool();

                lod.Meshes[meshIndex].Init(this, lodIndex, meshIndex, materialSlotIndex, box, sphere, hasLightmapUVs);
            }
        }

        // Load SDF
        auto chunk15 = GetChunk(15);
        /*if (chunk15 && chunk15->IsValid() && EnableModelSDF == 1)
        {
            MemoryReadStream sdfStream(chunk15->Get(), chunk15->Size());
            int32 version;
            sdfStream.ReadInt32(&version);
            switch (version)
            {
            case 1:
            {
                ModelSDFHeader data;
                sdfStream.ReadBytes(&data, sizeof(data));
                if (!SDF.Texture)
                {
                    String name;
#if !BUILD_RELEASE
                    name = GetPath() + SE_TEXT(".SDF");
#endif
                    SDF.Texture = GPUDevice::instance->CreateTexture(name);
                }
                if (SDF.Texture->Init(GPUTextureDescription::New3D(data.Width, data.Height, data.Depth, data.Format, GPUTextureFlags::ShaderResource, data.MipLevels)))
                    return LoadResult::Failed;
                SDF.LocalToUVWMul = data.LocalToUVWMul;
                SDF.LocalToUVWAdd = data.LocalToUVWAdd;
                SDF.WorldUnitsPerVoxel = data.WorldUnitsPerVoxel;
                SDF.MaxDistance = data.MaxDistance;
                SDF.LocalBoundsMin = data.LocalBoundsMin;
                SDF.LocalBoundsMax = data.LocalBoundsMax;
                SDF.ResolutionScale = data.ResolutionScale;
                SDF.LOD = data.LOD;
                for (int32 mipLevel = 0; mipLevel < data.MipLevels; mipLevel++)
                {
                    ModelSDFMip mipData;
                    sdfStream.ReadBytes(&mipData, sizeof(mipData));
                    void* mipBytes = sdfStream.Move(mipData.SlicePitch);
                    auto task = ::New<StreamModelSDFTask>(this, SDF.Texture, Span<byte>((byte*)mipBytes, mipData.SlicePitch), mipData.MipIndex, mipData.RowPitch, mipData.SlicePitch);
                    task->Start();
                }
                break;
            }
            default:
                LOG_WARNING("Render", "Unknown SDF data version {0} in {1}", version, ToString());
                break;
            }
        }*/

#if !BUILD_RELEASE
        // Validate LODs
        for (int32 lodIndex = 1; lodIndex < LODs.Count(); lodIndex++)
        {
            const auto prevSS = LODs[lodIndex - 1].ScreenSize;
            const auto thisSS = LODs[lodIndex].ScreenSize;
            if (prevSS <= thisSS)
            {
                LOG_WARNING("Render", "Model LOD {0} has invalid screen size compared to LOD {1} (asset: {2})", lodIndex, lodIndex - 1, ToString());
            }
        }
#endif

        // Request resource streaming
        StartStreaming(true);

        return LoadResult::Ok;
    }

    void Model::Unload(bool isReloading)
    {
        // End streaming (if still active)
        if (m_StreamingTask != nullptr)
        {
            // Cancel streaming task
            m_StreamingTask->Cancel();
            m_StreamingTask = nullptr;
        }

        // Cleanup
        // SAFE_DELETE_GPU_RESOURCE(SDF.Texture);
        MaterialSlots.Resize(0);
        for (int32 i = 0; i < LODs.Count(); i++)
            LODs[i].Dispose();
        LODs.Clear();
        m_LoadedLODs = 0;
    }

    bool Model::OnInit(AssetInitData& initData)
    {
        // Validate
        if (initData.SerializedVersion != MODE_VERSION)
        {
            LOG_ERROR("Remder", "Invalid serialized model version.");
            return false;
        }

        return true;
    }

    AssetChunksFlag Model::GetChunksToPreload() const
    {
        // Note: we don't preload any LODs here because it's done by the Streaming Manager
        return GET_CHUNK_FLAG(0) | GET_CHUNK_FLAG(15);
    }

    BoundingBox Model::GetBox(const Transform& transform, int32 lodIndex) const
    {
        return LODs[lodIndex].GetBox(transform);
    }
} // SE
