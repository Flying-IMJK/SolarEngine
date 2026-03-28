

#include "Mesh.h"

#include <Core/Math/Transform.h>

#include "MeshDeformation.h"
#include "Model.h"
#include "ModelInstanceEntry.h"
#include "ModelLOD.h"
#include "Core/Profiler/ProfilerCPU.h"
#include "Core/Serialization/MemoryReadStream.h"
#include "Runtime/Graphics/GPUContext.h"
#include "Runtime/Graphics/GPUDevice.h"
#include "Runtime/Graphics/Base/GPUBuffer.h"
#include "Runtime/Render/RenderContext.h"
#include "Runtime/Render/RenderDrawCall.h"
#include "Runtime/Render/RenderList.h"
#include "Runtime/Resource/Assets/Materials/MaterialBase.h"
#include "Runtime/Render/Assets/Material/MaterialSlot.h"
#include "Runtime/Render/Utils/RenderUtils.h"

namespace SE
{
    namespace Internal
    {
        template<class IndexType>
        bool UpdateMesh(Mesh* mesh, uint32 vertexCount, uint32 triangleCount, const Float3* vertices, const IndexType* triangles,
            const Float3* normals, const Float3* tangents, const Float2* uvs, const Color32* colors)
        {
            auto model = mesh->GetModel();
            if (!model || !model->IsVirtual())
            {
                return false;
            }
            if (!triangles || !vertices)
            {
                LOG_ERROR("Render", "Update Mesh triangles or vertices is nullptr");
                return false;
            }

            // Pack mesh data into vertex buffers
            List<VB1ElementType> vb1;
            List<VB2ElementType> vb2;
            vb1.Resize(vertexCount);
            if (normals)
            {
                if (tangents)
                {
                    for (uint32 i = 0; i < vertexCount; i++)
                    {
                        const Float3 normal = normals[i];
                        const Float3 tangent = tangents[i];
                        auto& v = vb1.Get()[i];
                        RenderUtils::CalculateTangentFrame(v.Normal, v.Tangent, normal, tangent);
                    }
                }
                else
                {
                    for (uint32 i = 0; i < vertexCount; i++)
                    {
                        const Float3 normal = normals[i];
                        auto& v = vb1.Get()[i];
                        RenderUtils::CalculateTangentFrame(v.Normal, v.Tangent, normal);
                    }
                }
            }
            else
            {
                // Set default tangent frame
                const auto n = Float1010102(Float3::UnitZ);
                const auto t = Float1010102(Float3::UnitX);
                for (uint32 i = 0; i < vertexCount; i++)
                {
                    vb1.Get()[i].Normal = n;
                    vb1.Get()[i].Tangent = t;
                }
            }
            if (uvs)
            {
                for (uint32 i = 0; i < vertexCount; i++)
                    vb1.Get()[i].TexCoord = Half2(uvs[i]);
            }
            else
            {
                auto v = Half2::Zero;
                for (uint32 i = 0; i < vertexCount; i++)
                    vb1.Get()[i].TexCoord = v;
            }
            {
                auto v = Half2::Zero;
                for (uint32 i = 0; i < vertexCount; i++)
                    vb1.Get()[i].LightmapUVs = v;
            }
            if (colors)
            {
                vb2.Resize(vertexCount);
                for (uint32 i = 0; i < vertexCount; i++)
                    vb2.Get()[i].Color = colors[i];
            }

            return mesh->UpdateMesh(vertexCount, triangleCount, (VB0ElementType*)vertices, vb1.Get(), vb2.HasItems() ? vb2.Get() : nullptr, triangles);
        }


    }

    bool Mesh::HasVertexColors() const
    {
        return _vertexBuffers[2] != nullptr && _vertexBuffers[2]->IsAllocated();
    }

    bool Mesh::UpdateMesh(uint32 vertexCount, uint32 triangleCount, const VB0ElementType* vb0, const VB1ElementType* vb1, const VB2ElementType* vb2, const void* ib, bool use16BitIndices)
    {
        auto model = (Model*)_model;

        Unload();

        // Setup GPU resources
        model->LODs[_lodIndex]._verticesCount -= _vertices;
        const bool failed = Load(vertexCount, triangleCount, vb0, vb1, vb2, ib, use16BitIndices);
        if (!failed)
        {
            model->LODs[_lodIndex]._verticesCount += _vertices;

            // Calculate mesh bounds
            BoundingBox bounds;
            BoundingBox::FromPoints((const Float3*)vb0, vertexCount, bounds);
            SetBounds(bounds);

            // Send event (actors using this model can update bounds, etc.)
            model->OnLoaded();
        }

        return failed;
    }

    bool Mesh::UpdateMesh(uint32 vertexCount, uint32 triangleCount, const Float3* vertices, const uint16* triangles, const Float3* normals,
        const Float3* tangents, const Float2* uvs, const Color32* colors)
    {
        return Internal::UpdateMesh<uint16>(this, vertexCount, triangleCount, vertices, triangles, normals, tangents, uvs, colors);
    }

    bool Mesh::UpdateMesh(uint32 vertexCount, uint32 triangleCount, const Float3* vertices, const uint32* triangles, const Float3* normals,
        const Float3* tangents, const Float2* uvs, const Color32* colors)
    {
        return Internal::UpdateMesh<uint32>(this, vertexCount, triangleCount, vertices, triangles, normals, tangents, uvs, colors);
    }

    bool Mesh::UpdateTriangles(uint32 triangleCount, const void* ib, bool use16BitIndices)
    {
        // Cache data
        uint32 indicesCount = triangleCount * 3;
        uint32 ibStride = use16BitIndices ? sizeof(uint16) : sizeof(uint32);

        // Create index buffer
        GPUBuffer* indexBuffer = GPUDevice::instance->CreateBuffer(String::Empty);
        if (indexBuffer->Init(GPUBufferDescription::Index(ibStride, indicesCount, ib)))
        {
            Delete(indexBuffer);
            return true;
        }

        // TODO: update collision proxy

        // Initialize
        DeleteObjectSafe(_indexBuffer);
        _indexBuffer = indexBuffer;
        _triangles = triangleCount;
        _use16BitIndexBuffer = use16BitIndices;

        return false;
    }

    void Mesh::Init(Model* model, int32 lodIndex, int32 index, int32 materialSlotIndex, const BoundingBox& box, const BoundingSphere& sphere, bool hasLightmapUVs)
    {
        _model = model;
        _lodIndex = lodIndex;
        _index = index;
        _materialSlotIndex = materialSlotIndex;
        _use16BitIndexBuffer = false;
        _hasLightmapUVs = hasLightmapUVs;
        _box = box;
        _sphere = sphere;
        _vertices = 0;
        _triangles = 0;
        _vertexBuffers[0] = nullptr;
        _vertexBuffers[1] = nullptr;
        _vertexBuffers[2] = nullptr;
        _indexBuffer = nullptr;
    }

    Mesh::Mesh(): _hasLightmapUVs(false), _cachedIndexBufferCount(0)
    {
    }
    Mesh::~Mesh()
    {
        // Release buffers
        DeleteObjectSafe(_vertexBuffers[0]);
        DeleteObjectSafe(_vertexBuffers[1]);
        DeleteObjectSafe(_vertexBuffers[2]);
        DeleteObjectSafe(_indexBuffer);
    }

    String Mesh::ToString() const
    {
        return m_Name;
    }

    bool Mesh::Load(uint32 vertices, uint32 triangles, const void* vb0, const void* vb1, const void* vb2, const void* ib, bool use16BitIndexBuffer)
    {
        // Cache data
        uint32 indicesCount = triangles * 3;
        uint32 ibStride = use16BitIndexBuffer ? sizeof(uint16) : sizeof(uint32);

        GPUBuffer* vertexBuffer0 = nullptr;
        GPUBuffer* vertexBuffer1 = nullptr;
        GPUBuffer* vertexBuffer2 = nullptr;
        GPUBuffer* indexBuffer = nullptr;

        // Create GPU buffers
#if GPU_ENABLE_RESOURCE_NAMING
#define MESH_BUFFER_NAME(postfix) GetModel()->GetPath() + SE_TEXT(postfix)
#else
#define MESH_BUFFER_NAME(postfix) String::Empty
#endif
        
        vertexBuffer0 = GPUDevice::instance->CreateBuffer(MESH_BUFFER_NAME(".VB0"));
        if (!vertexBuffer0->Init(GPUBufferDescription::Vertex(sizeof(VB0ElementType), vertices, vb0)))
        {
            goto ERROR_LOAD_END;
        }
        vertexBuffer1 = GPUDevice::instance->CreateBuffer(MESH_BUFFER_NAME(".VB1"));
        if (!vertexBuffer1->Init(GPUBufferDescription::Vertex(sizeof(VB1ElementType), vertices, vb1)))
        {
            goto ERROR_LOAD_END;
        }
        if (vb2)
        {
            vertexBuffer2 = GPUDevice::instance->CreateBuffer(MESH_BUFFER_NAME(".VB2"));
            if (!vertexBuffer2->Init(GPUBufferDescription::Vertex(sizeof(VB2ElementType), vertices, vb2)))
            {
                goto ERROR_LOAD_END;
            }
        }
        indexBuffer = GPUDevice::instance->CreateBuffer(MESH_BUFFER_NAME(".IB"));
        if (!indexBuffer->Init(GPUBufferDescription::Index(ibStride, indicesCount, ib)))
        {
            goto ERROR_LOAD_END;
        }

        // Init collision proxy
#if USE_PRECISE_MESH_INTERSECTS
        if (!_collisionProxy.HasData())
        {
            if (use16BitIndexBuffer)
                _collisionProxy.Init<uint16>(vertices, triangles, (Float3*)vb0, (uint16*)ib);
            else
                _collisionProxy.Init<uint32>(vertices, triangles, (Float3*)vb0, (uint32*)ib);
        }
#endif

        // Initialize
        _vertexBuffers[0] = vertexBuffer0;
        _vertexBuffers[1] = vertexBuffer1;
        _vertexBuffers[2] = vertexBuffer2;
        _indexBuffer = indexBuffer;
        _triangles = triangles;
        _vertices = vertices;
        _use16BitIndexBuffer = use16BitIndexBuffer;
        _cachedVertexBuffer[0].Clear();
        _cachedVertexBuffer[1].Clear();
        _cachedVertexBuffer[2].Clear();

        return true;

#undef MESH_BUFFER_NAME
        ERROR_LOAD_END:

        DeleteObjectSafe(vertexBuffer0);
        DeleteObjectSafe(vertexBuffer1);
        DeleteObjectSafe(vertexBuffer2);
        DeleteObjectSafe(indexBuffer);
        return false;
    }

    void Mesh::Unload()
    {
        DeleteObjectSafe(_vertexBuffers[0]);
        DeleteObjectSafe(_vertexBuffers[1]);
        DeleteObjectSafe(_vertexBuffers[2]);
        DeleteObjectSafe(_indexBuffer);
        _triangles = 0;
        _vertices = 0;
        _use16BitIndexBuffer = false;
        _cachedIndexBuffer.Resize(0);
        _cachedVertexBuffer[0].Clear();
        _cachedVertexBuffer[1].Clear();
        _cachedVertexBuffer[2].Clear();
    }

    bool Mesh::Intersects(const Ray& ray, const Matrix& world, float& distance, Float3& normal) const
    {
        // Get bounding box of the mesh bounds transformed by the instance world matrix
        Float3 corners[8];
        _box.GetCorners(corners);
        Float3 tmp;
        Float3::Transform(corners[0], world, tmp);
        Float3 min = tmp;
        Float3 max = tmp;
        for (int32 i = 1; i < 8; i++)
        {
            Float3::Transform(corners[i], world, tmp);
            min = Float3::Min(min, tmp);
            max = Float3::Max(max, tmp);
        }
        const BoundingBox transformedBox(min, max);

        // Test ray on box
#if USE_PRECISE_MESH_INTERSECTS
        if (transformedBox.Intersects(ray, distance))
        {
            // Use exact test on raw geometry
            return _collisionProxy.Intersects(ray, world, distance, normal);
        }
        distance = 0;
        normal = Float3::Up;
        return false;
#else
        return transformedBox.Intersects(ray, distance, normal);
#endif
    }

    bool Mesh::Intersects(const Ray& ray, const Transform& transform, float& distance, Float3& normal) const
    {
        // Get bounding box of the mesh bounds transformed by the instance world matrix
        Float3 corners[8];
        _box.GetCorners(corners);
        Float3 tmp;
        transform.LocalToWorld(corners[0], tmp);
        Float3 min = tmp;
        Float3 max = tmp;
        for (int32 i = 1; i < 8; i++)
        {
            transform.LocalToWorld(corners[i], tmp);
            min = Float3::Min(min, tmp);
            max = Float3::Max(max, tmp);
        }
        const BoundingBox transformedBox(min, max);

        // Test ray on box
#if USE_PRECISE_MESH_INTERSECTS
        if (transformedBox.Intersects(ray, distance))
        {
            // Use exact test on raw geometry
            return _collisionProxy.Intersects(ray, transform, distance, normal);
        }
        distance = 0;
        normal = Float3::Up;
        return false;
#else
        return transformedBox.Intersects(ray, distance, normal);
#endif
    }

    void Mesh::GetDrawCallGeometry(DrawCall& drawCall) const
    {
        drawCall.Geometry.IndexBuffer = _indexBuffer;
        drawCall.Geometry.VertexBuffers[0] = _vertexBuffers[0];
        drawCall.Geometry.VertexBuffers[1] = _vertexBuffers[1];
        drawCall.Geometry.VertexBuffers[2] = _vertexBuffers[2];
        drawCall.Geometry.VertexBuffersOffsets[0] = 0;
        drawCall.Geometry.VertexBuffersOffsets[1] = 0;
        drawCall.Geometry.VertexBuffersOffsets[2] = 0;
        drawCall.Draw.StartIndex = 0;
        drawCall.Draw.IndicesCount = _triangles * 3;
    }

    void Mesh::Render(GPUContext* context) const
    {
        if (!IsInitialized())
            return;

        context->BindVB(ToSpan((GPUBuffer**)_vertexBuffers, 3));
        context->BindIB(_indexBuffer);
        context->DrawIndexedInstanced(_triangles * 3, 1, 0, 0, 0);
    }

    void Mesh::Draw(const RenderContext& renderContext, MaterialBase* material, const Matrix& world, EnumFlags<StaticMask> flags,
        bool receiveDecals, EnumFlags<DrawPass> drawModes, float perInstanceRandom, int16 sortOrder) const
    {
        if (!material || !material->IsDeformable(MaterialDomain::Surface) || !IsInitialized())
        {
            return;
        }
        drawModes.Marge(material->GetDrawModes());
        
        if (drawModes.IsFlag(DrawPass::None))
        {
            return;
        }

        // Setup draw call
        DrawCall drawCall;
        drawCall.Geometry.IndexBuffer = _indexBuffer;
        drawCall.Geometry.VertexBuffers[0] = _vertexBuffers[0];
        drawCall.Geometry.VertexBuffers[1] = _vertexBuffers[1];
        drawCall.Geometry.VertexBuffers[2] = _vertexBuffers[2];
        drawCall.Draw.IndicesCount = _triangles * 3;
        drawCall.InstanceCount = 1;
        drawCall.Material = material;
        drawCall.World = world;
        drawCall.ObjectPosition = drawCall.World.GetTranslation();
        drawCall.ObjectRadius = (float)_sphere.Radius * drawCall.World.GetScaleVector().GetAbsolute().MaxValue();
        drawCall.Surface.GeometrySize = _box.GetSize();
        drawCall.Surface.PrevWorld = world;
        // drawCall.Surface.Lightmap = nullptr;
        drawCall.Surface.LightmapUVsArea = Rectangle::Empty;
        // drawCall.Surface.Skinning = nullptr;
        drawCall.Surface.LODDitherFactor = 0.0f;
        drawCall.WorldDeterminantSign = world.RotDeterminant() >= 0.0f ? 1.0 : -1.0;
        drawCall.PerInstanceRandom = perInstanceRandom;
#if SE_EDITOR
        /*const ViewMode viewMode = renderContext.view.Mode;
        if (viewMode == ViewMode::LightmapUVsDensity || viewMode == ViewMode::LODPreview)
        {
            GBufferPass::AddIndexBufferToModelLOD(_indexBuffer, &((Model*)_model)->LODs[_lodIndex]);
        }*/
#endif

        // Push draw call to the render list
        renderContext.list->AddDrawCall(renderContext, drawModes, flags, drawCall, receiveDecals, sortOrder);
    }

    void Mesh::Draw(const RenderContext& renderContext, const DrawInfo& info, float lodDitherFactor) const
    {
        const auto& entry = info.Buffer->At(_materialSlotIndex);
        if (!entry.Visible || !IsInitialized())
            return;
        const MaterialSlot& slot = _model->MaterialSlots[_materialSlotIndex];

        // Select material
        MaterialBase* material;
        if (entry.Material && entry.Material->IsLoaded())
        {
            material = entry.Material;
        }
        else if (slot.Material && slot.Material->IsLoaded())
        {
            material = slot.Material;
        }
        else
        {
            material = GPUDevice::instance->GetMainContext()->GetDefaultMaterial();
        }
        if (!material || !material->IsDeformable(MaterialDomain::Surface))
            return;

        // Check if skip rendering
        EnumFlags<ShadowsCastingMode> shadowsMode = entry.ShadowsMode;
        shadowsMode.Marge(slot.ShadowsMode);
        EnumFlags<DrawPass> drawModes = info.DrawModes;
        drawModes.Marge(renderContext.view.Pass);
        drawModes.Marge(renderContext.view.GetShadowsDrawPassMask(shadowsMode));
        drawModes.Marge(material->GetDrawModes());

        if (drawModes.IsFlag(DrawPass::None))
            return;

        // Setup draw call
        DrawCall drawCall;
        drawCall.Geometry.IndexBuffer = _indexBuffer;
        drawCall.Geometry.VertexBuffers[0] = _vertexBuffers[0];
        drawCall.Geometry.VertexBuffers[1] = _vertexBuffers[1];
        drawCall.Geometry.VertexBuffers[2] = _vertexBuffers[2];
        /*if (info.Deformation)
        {
            info.Deformation->RunDeformers(this, MeshBufferType::Vertex0, drawCall.Geometry.VertexBuffers[0]);
            info.Deformation->RunDeformers(this, MeshBufferType::Vertex1, drawCall.Geometry.VertexBuffers[1]);
        }*/
        if (info.VertexColors && info.VertexColors[_lodIndex])
        {
            // TODO: cache vertexOffset within the model LOD per-mesh
            uint32 vertexOffset = 0;
            for (int32 meshIndex = 0; meshIndex < _index; meshIndex++)
                vertexOffset += ((Model*)_model)->LODs[_lodIndex].Meshes[meshIndex].GetVertexCount();
            drawCall.Geometry.VertexBuffers[2] = info.VertexColors[_lodIndex];
            drawCall.Geometry.VertexBuffersOffsets[2] = vertexOffset * sizeof(VB2ElementType);
        }
        drawCall.Draw.IndicesCount = _triangles * 3;
        drawCall.InstanceCount = 1;
        drawCall.Material = material;
        drawCall.World = *info.World;
        drawCall.ObjectPosition = drawCall.World.GetTranslation();
        drawCall.ObjectRadius = (float)info.Bounds.Radius; // TODO: should it be kept in sync with ObjectPosition?
        drawCall.Surface.GeometrySize = _box.GetSize();
        drawCall.Surface.PrevWorld = info.DrawState->PrevWorld;
        // drawCall.Surface.Lightmap = (info.Flags & StaticFlags::Lightmap) != StaticMaskFlags::None ? info.Lightmap : nullptr;
        drawCall.Surface.LightmapUVsArea = info.LightmapUVs ? *info.LightmapUVs : Rectangle::Empty;
        // drawCall.Surface.Skinning = nullptr;
        drawCall.Surface.LODDitherFactor = lodDitherFactor;
        drawCall.WorldDeterminantSign = drawCall.World.RotDeterminant() >= 0.0f ? 1 : -1;
        drawCall.PerInstanceRandom = info.PerInstanceRandom;
#if SE_EDITOR
        /*const ViewMode viewMode = renderContext.view.Mode;
        if (viewMode == ViewMode::LightmapUVsDensity || viewMode == ViewMode::LODPreview)
            GBufferPass::AddIndexBufferToModelLOD(_indexBuffer, &((Model*)_model)->LODs[_lodIndex]);*/
#endif

        // Push draw call to the render list
        renderContext.list->AddDrawCall(renderContext, drawModes, info.Flags, drawCall, entry.ReceiveDecals, info.SortOrder);
    }

    void Mesh::Draw(const RenderContextBatch& renderContextBatch, const DrawInfo& info, float lodDitherFactor) const
    {
        const auto& entry = info.Buffer->At(_materialSlotIndex);
        if (!entry.Visible || !IsInitialized())
        {
            return;
        }
        const MaterialSlot& slot = _model->MaterialSlots[_materialSlotIndex];

        // Select material
        IMaterial* material = nullptr;
        if (entry.Material && entry.Material->IsLoaded())
        {
            material = entry.Material;
        }
        else if (slot.Material && slot.Material->IsLoaded())
        {
            material = slot.Material;
        }
        else
        {
            material = GPUDevice::instance->GetMainContext()->GetDefaultMaterial();
        }
        if (!material || !material->IsDeformable(MaterialDomain::Surface))
        {
            return;
        }

        // Setup draw call
        DrawCall drawCall;
        drawCall.Geometry.IndexBuffer = _indexBuffer;
        drawCall.Geometry.VertexBuffers[0] = _vertexBuffers[0];
        drawCall.Geometry.VertexBuffers[1] = _vertexBuffers[1];
        drawCall.Geometry.VertexBuffers[2] = _vertexBuffers[2];
        if (info.Deformation)
        {
            info.Deformation->RunDeformers(this, MeshBufferType::Vertex0, drawCall.Geometry.VertexBuffers[0]);
            info.Deformation->RunDeformers(this, MeshBufferType::Vertex1, drawCall.Geometry.VertexBuffers[1]);
        }
        if (info.VertexColors && info.VertexColors[_lodIndex])
        {
            // TODO: cache vertexOffset within the model LOD per-mesh
            uint32 vertexOffset = 0;
            for (int32 meshIndex = 0; meshIndex < _index; meshIndex++)
                vertexOffset += ((Model*)_model)->LODs[_lodIndex].Meshes[meshIndex].GetVertexCount();
            drawCall.Geometry.VertexBuffers[2] = info.VertexColors[_lodIndex];
            drawCall.Geometry.VertexBuffersOffsets[2] = vertexOffset * sizeof(VB2ElementType);
        }
        drawCall.Draw.IndicesCount = _triangles * 3;
        drawCall.InstanceCount = 1;
        drawCall.Material = material;
        drawCall.World = *info.World;
        drawCall.ObjectPosition = drawCall.World.GetTranslation();
        drawCall.ObjectRadius = (float)info.Bounds.Radius; // TODO: should it be kept in sync with ObjectPosition?
        drawCall.Surface.GeometrySize = _box.GetSize();
        drawCall.Surface.PrevWorld = info.DrawState->PrevWorld;
        // drawCall.Surface.Lightmap = (info.Flags & StaticFlags::Lightmap) != StaticFlags::None ? info.Lightmap : nullptr;
        drawCall.Surface.LightmapUVsArea = info.LightmapUVs != nullptr ? *info.LightmapUVs : Rectangle::Empty;
        // drawCall.Surface.Skinning = nullptr;
        drawCall.Surface.LODDitherFactor = lodDitherFactor;
        drawCall.WorldDeterminantSign = drawCall.World.RotDeterminant() >= 0.0f ? 1 : -1;
        drawCall.PerInstanceRandom = info.PerInstanceRandom;
#if SE_EDITOR
        /*const ViewMode viewMode = renderContextBatch.GetMainContext().View.Mode;
        if (viewMode == ViewMode::LightmapUVsDensity || viewMode == ViewMode::LODPreview)
            GBufferPass::AddIndexBufferToModelLOD(_indexBuffer, &((Model*)_model)->LODs[_lodIndex]);*/
#endif

        // Push draw call to the render lists
        auto shadowsMode = entry.ShadowsMode;
        shadowsMode.Marge(slot.ShadowsMode);

        auto drawModes = info.DrawModes;
        drawModes.Marge(material->GetDrawModes());

        if (!drawModes.Is(DrawPass::None))
        {
            renderContextBatch.GetMainContext().list->AddDrawCall(renderContextBatch, drawModes, info.Flags, shadowsMode, info.Bounds, drawCall, entry.ReceiveDecals, info.SortOrder);
        }
    }

    bool Mesh::DownloadDataGPU(MeshBufferType type, BytesContainer& result) const
    {
        GPUBuffer* buffer = nullptr;
        switch (type)
        {
        case MeshBufferType::Index:
            buffer = _indexBuffer;
            break;
        case MeshBufferType::Vertex0:
            buffer = _vertexBuffers[0];
            break;
        case MeshBufferType::Vertex1:
            buffer = _vertexBuffers[1];
            break;
        case MeshBufferType::Vertex2:
            buffer = _vertexBuffers[2];
            break;
        }
        return buffer && buffer->DownloadData(result);
    }

    Threading::Task* Mesh::DownloadDataGPUAsync(MeshBufferType type, BytesContainer& result) const
    {
        GPUBuffer* buffer = nullptr;
        switch (type)
        {
        case MeshBufferType::Index:
            buffer = _indexBuffer;
            break;
        case MeshBufferType::Vertex0:
            buffer = _vertexBuffers[0];
            break;
        case MeshBufferType::Vertex1:
            buffer = _vertexBuffers[1];
            break;
        case MeshBufferType::Vertex2:
            buffer = _vertexBuffers[2];
            break;
        }
        return buffer ? buffer->DownloadDataAsync(result) : nullptr;
    }

    bool Mesh::DownloadDataCPU(MeshBufferType type, BytesContainer& result, int32& count) const
    {
        if (_cachedVertexBuffer[0].IsEmpty())
        {
            PROFILE_CPU();
            auto model = GetModel();
            Threading::ScopeLock lock(model->Locker);
            if (model->IsVirtual())
            {
                LOG_ERROR("Render", "Cannot access CPU data of virtual models. Use GPU data download.");
                return true;
            }

            // Fetch chunk with data from drive/memory
            const auto chunkIndex = MODEL_LOD_TO_CHUNK_INDEX(GetLODIndex());
            if (model->LoadChunk(chunkIndex))
                return true;
            const auto chunk = model->GetChunk(chunkIndex);
            if (!chunk)
            {
                LOG_ERROR("Render", "Missing chunk.");
                return true;
            }

            MemoryReadStream stream(chunk->Get(), chunk->Size());

            // Seek to find mesh location
            for (int32 i = 0; i <= _index; i++)
            {
                // #MODEL_DATA_FORMAT_USAGE
                uint32 vertices;
                stream.ReadUint32(&vertices);
                uint32 triangles;
                stream.ReadUint32(&triangles);
                uint32 indicesCount = triangles * 3;
                bool use16BitIndexBuffer = indicesCount <= Max_uint16;
                uint32 ibStride = use16BitIndexBuffer ? sizeof(uint16) : sizeof(uint32);
                if (vertices == 0 || triangles == 0)
                {
                    LOG_ERROR("Render", "Invalid mesh data.");
                    return true;
                }
                auto vb0 = stream.Move<VB0ElementType>(vertices);
                auto vb1 = stream.Move<VB1ElementType>(vertices);
                bool hasColors = stream.ReadBool();
                VB2ElementType* vb2 = nullptr;
                if (hasColors)
                {
                    vb2 = stream.Move<VB2ElementType>(vertices);
                }
                auto ib = stream.Move<byte>(indicesCount * ibStride);

                if (i != _index)
                    continue;

                // Cache mesh data
                _cachedIndexBufferCount = indicesCount;
                _cachedIndexBuffer.Set(ib, indicesCount * ibStride);
                _cachedVertexBuffer[0].Set((const byte*)vb0, vertices * sizeof(VB0ElementType));
                _cachedVertexBuffer[1].Set((const byte*)vb1, vertices * sizeof(VB1ElementType));
                if (hasColors)
                    _cachedVertexBuffer[2].Set((const byte*)vb2, vertices * sizeof(VB2ElementType));
                break;
            }
        }

        switch (type)
        {
        case MeshBufferType::Index:
            result.Link(_cachedIndexBuffer);
            count = _cachedIndexBufferCount;
            break;
        case MeshBufferType::Vertex0:
            result.Link(_cachedVertexBuffer[0]);
            count = _cachedVertexBuffer[0].Count() / sizeof(VB0ElementType);
            break;
        case MeshBufferType::Vertex1:
            result.Link(_cachedVertexBuffer[1]);
            count = _cachedVertexBuffer[1].Count() / sizeof(VB1ElementType);
            break;
        case MeshBufferType::Vertex2:
            result.Link(_cachedVertexBuffer[2]);
            count = _cachedVertexBuffer[2].Count() / sizeof(VB2ElementType);
            break;
        default:
            return true;
        }
        return false;
    }

    /*Object* Mesh::GetParentModel()
    {
        return _model;
    }*/
} // SE