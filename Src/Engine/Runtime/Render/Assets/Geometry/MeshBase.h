#pragma once

#include "Runtime/Core/Math/BoundingVolumes.h"
#include "Runtime/Core/Scripting/ScriptingObject.h"
#include "Runtime/Core/Types/Collections/DataContainer.h"
#include "Runtime/Render/RenderEnum.h"

namespace SE
{
    namespace Threading
    {
        class Task;
    }

    struct GeometryDrawStateData;
    struct RenderContext;
    struct RenderContextBatch;

    class ModelBase;
    class Lightmap;
    class GPUBuffer;
    class SkinnedMeshDrawData;
    class BlendShapesInstance;
    class ModelInstanceEntries;
    class MeshDeformation;

    /// <summary>
    /// Base class for model resources meshes.
    /// </summary>
    SE_CLASS(API(Abstract, NoSpawn))
    class SE_API_RUNTIME MeshBase : public ScriptingObject
    {
        SCRIPTING_TYPE_MIN(MeshBase)
    protected:
        ModelBase* _model = nullptr;
        BoundingBox _box = BoundingBox::Zero;
        BoundingSphere _sphere = BoundingSphere::Empty;
        int32 _index = 0;
        int32 _lodIndex = 0;
        uint32 _vertices = 0;
        uint32 _triangles = 0;
        int32 _materialSlotIndex = 0;
        bool _use16BitIndexBuffer = false;

        GPUBuffer* _vertexBuffers[3] = {};
        GPUBuffer* _indexBuffer = nullptr;
#if USE_PRECISE_MESH_INTERSECTS
        CollisionProxy _collisionProxy;
#endif
        mutable BytesContainer _cachedVertexBuffer[3];
        mutable BytesContainer _cachedIndexBuffer;
        mutable int32 _cachedIndexBufferCount = 0;
        mutable int32 _cachedVertexBufferCount = 0;

    public:
        explicit MeshBase(const SpawnParams& params): ScriptingObject(params)
        {
        }

        /// <summary>
        /// Gets the model owning this mesh.
        /// </summary>
        SE_FUNCTION(API(Prop))
        FORCE_INLINE ModelBase* GetModelBase() const
        {
            return _model;
        }

        /// <summary>
        /// Gets the mesh parent LOD index.
        /// </summary>
        SE_FUNCTION(API(Prop))
        FORCE_INLINE int32 GetLODIndex() const
        {
            return _lodIndex;
        }

        /// <summary>
        /// Gets the mesh index.
        /// </summary>
        SE_FUNCTION(API(Prop))
        FORCE_INLINE int32 GetIndex() const
        {
            return _index;
        }

        /// <summary>
        /// Gets the triangle count.
        /// </summary>
        SE_FUNCTION(API(Prop))
        FORCE_INLINE int32 GetTriangleCount() const
        {
            return _triangles;
        }

        /// <summary>
        /// Gets the vertex count.
        /// </summary>
        SE_FUNCTION(API(Prop))
        FORCE_INLINE int32 GetVertexCount() const
        {
            return _vertices;
        }

        /// <summary>
        /// Gets the box.
        /// </summary>
        SE_FUNCTION(API(Prop))
        FORCE_INLINE const BoundingBox& GetBox() const
        {
            return _box;
        }

        /// <summary>
        /// Gets the sphere.
        /// </summary>
        SE_FUNCTION(API(Prop))
        FORCE_INLINE const BoundingSphere& GetSphere() const
        {
            return _sphere;
        }

        /// <summary>
        /// Determines whether this mesh is using 16 bit index buffer, otherwise it's 32 bit.
        /// </summary>
        SE_FUNCTION(API(Prop))
        FORCE_INLINE bool Use16BitIndexBuffer() const
        {
            return _use16BitIndexBuffer;
        }

        /// <summary>
        /// Gets the index of the material slot to use during this mesh rendering.
        /// </summary>
        SE_FUNCTION(API(Prop))
        FORCE_INLINE int32 GetMaterialSlotIndex() const
        {
            return _materialSlotIndex;
        }

        /// <summary>
        /// Sets the index of the material slot to use during this mesh rendering.
        /// </summary>
        SE_FUNCTION(API(Prop))
        void SetMaterialSlotIndex(int32 value);

        /// <summary>
        /// Sets the mesh bounds.
        /// </summary>
        /// <param name="box">The bounding box.</param>
        SE_FUNCTION(API())
        void SetBounds(const BoundingBox& box);

        /// <summary>
        /// Gets the index buffer.
        /// </summary>
        SE_FUNCTION(API())
        FORCE_INLINE GPUBuffer* GetIndexBuffer() const
        {
            return _indexBuffer;
        }

        /// <summary>
        /// Gets the vertex buffer.
        /// </summary>
        /// <param name="index">The index.</param>
        /// <returns>The buffer.</returns>
        SE_FUNCTION(API())
        FORCE_INLINE GPUBuffer* GetVertexBuffer(int32 index) const
        {
            return _vertexBuffers[index];
        }

    public:
        /// <summary>
        /// Extract mesh buffer data from GPU. Cannot be called from the main thread.
        /// </summary>
        /// <param name="type">Buffer type</param>
        /// <param name="result">The result data</param>
        /// <returns>True if failed, otherwise false</returns>
        virtual bool DownloadDataGPU(MeshBufferType type, BytesContainer& result) const;

        /// <summary>
        /// Extracts mesh buffer data from GPU in the async task.
        /// </summary>
        /// <param name="type">Buffer type</param>
        /// <param name="result">The result data</param>
        /// <returns>Created async task used to gather the buffer data.</returns>
        virtual Threading::Task* DownloadDataGPUAsync(MeshBufferType type, BytesContainer& result) const;

        /// <summary>
        /// Copies mesh buffer data from the internal CPU cache into caller-owned storage.
        /// </summary>
        /// <param name="type">Buffer type</param>
        /// <param name="result">The result data</param>
        /// <param name="count">The amount of items inside the result buffer.</param>
        /// <returns>True if successful, otherwise false.</returns>
        virtual bool DownloadDataCPU(MeshBufferType type, BytesContainer& result, int32& count) const;

    public:
        /// <summary>
        /// Model instance drawing packed data.
        /// </summary>
        struct DrawInfo
        {
            /// <summary>
            /// The instance buffer to use during model rendering.
            /// </summary>
            ModelInstanceEntries* Buffer = nullptr;

            /// <summary>
            /// The world transformation of the model.
            /// </summary>
            Matrix* World = nullptr;

            /// <summary>
            /// The instance drawing state data container. Used for LOD transition handling and previous world transformation matrix updating.
            /// </summary>
            GeometryDrawStateData* DrawState = nullptr;

            /// <summary>
            /// The instance deformation utility.
            /// </summary>
            MeshDeformation* Deformation = nullptr;

            union
            {
                /*struct
                {
                    /// <summary>
                    /// The skinning.
                    /// </summary>
                    SkinnedMeshDrawData* Skinning;
                };*/

                struct
                {
                    /// <summary>
                    /// The lightmap.
                    /// </summary>
                    const Lightmap* Lightmap;

                    /// <summary>
                    /// The lightmap UVs.
                    /// </summary>
                    const Rectangle* LightmapUVs;
                };
            };

            /// <summary>
            /// The model instance vertex colors buffers (per-lod all meshes packed in a single allocation, array length equal to model lods count).
            /// </summary>
            GPUBuffer** VertexColors = nullptr;

            /// <summary>
            /// The object static flags.
            /// </summary>
            StaticMask Flags;

            /// <summary>
            /// The object draw modes.
            /// </summary>
            DrawPass DrawModes;

            /// <summary>
            /// The bounds of the model (used to select a proper LOD during rendering).
            /// </summary>
            BoundingSphere Bounds;

            /// <summary>
            /// The per-instance random value.
            /// </summary>
            float PerInstanceRandom;

            /// <summary>
            /// The LOD bias value.
            /// </summary>
            char LODBias;

            /// <summary>
            /// The forced LOD to use. Value -1 disables this feature.
            /// </summary>
            char ForcedLOD;

            /// <summary>
            /// The object sorting key.
            /// </summary>
            int16 SortOrder;
        };
    };
}
