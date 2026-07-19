#pragma once

#include "Mesh.h"
#include "Runtime/Core/Types/Object.h"
#include "Runtime/API.h"

namespace SE
{
    class MaterialBase;
    class MemoryReadStream;

    /// <summary>
    /// Represents single Level Of Detail for the model. Contains a collection of the meshes.
    /// </summary>
    class SE_API_RUNTIME ModelLOD : public Object
    {
        friend Model;
        friend Mesh;
    private:
        Model* _model = nullptr;
        int32 _lodIndex = 0;
        uint32 _verticesCount;

    public:
        /// <summary>
        /// The screen size to switch LODs. Bottom limit of the model screen size to render this LOD.
        /// </summary>
        float ScreenSize = 1.0f;

        /// <summary>
        /// The meshes array.
        /// </summary>
        List<Mesh> Meshes;

        /// <summary>
        /// Determines whether any mesh has been initialized.
        /// </summary>
        /// <returns>True if any mesh has been initialized, otherwise false.</returns>
        FORCE_INLINE bool HasAnyMeshInitialized() const
        {
            // Note: we initialize all meshes at once so the last one can be used to check it.
            return Meshes.HasItems() && Meshes.Last().IsInitialized();
        }

        /// <summary>
        /// Gets the model LOD index.
        /// </summary>
        FORCE_INLINE int32 GetLODIndex() const
        {
            return _lodIndex;
        }

        /// <summary>
        /// Gets the vertex count for this model LOD level.
        /// </summary>
        FORCE_INLINE int32 GetVertexCount() const
        {
            return _verticesCount;
        }

    public:
        /// <summary>
        /// Initializes the LOD from the data stream.
        /// </summary>
        /// <param name="stream">The stream.</param>
        /// <returns>True if fails, otherwise false.</returns>
        bool Load(MemoryReadStream& stream);

        /// <summary>
        /// Unloads the LOD meshes data (vertex buffers and cache). It won't dispose the meshes collection. The opposite to Load.
        /// </summary>
        void Unload();

        /// <summary>
        /// Cleanups the data.
        /// </summary>
        void Dispose();

    public:
        /// <summary>
        /// Determines if there is an intersection between the Model and a Ray in given world using given instance
        /// </summary>
        /// <param name="ray">The ray to test</param>
        /// <param name="world">World to test</param>
        /// <param name="distance">When the method completes, contains the distance of the intersection (if any valid).</param>
        /// <param name="normal">When the method completes, contains the intersection surface normal vector (if any valid).</param>
        /// <param name="mesh">Mesh, or null</param>
        /// <returns>True whether the two objects intersected</returns>
        bool Intersects(const Ray& ray, const Matrix& world, float& distance, Float3& normal, Mesh** mesh);

        /// <summary>
        /// Determines if there is an intersection between the Model and a Ray in given world using given instance
        /// </summary>
        /// <param name="ray">The ray to test</param>
        /// <param name="transform">The instance transformation.</param>
        /// <param name="distance">When the method completes, contains the distance of the intersection (if any valid).</param>
        /// <param name="normal">When the method completes, contains the intersection surface normal vector (if any valid).</param>
        /// <param name="mesh">Mesh, or null</param>
        /// <returns>True whether the two objects intersected</returns>
        bool Intersects(const Ray& ray, const Transform& transform, float& distance, Float3& normal, Mesh** mesh);

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
        BoundingBox GetBox(const Transform& transform, const MeshDeformation* deformation = nullptr) const;

        /// <summary>
        /// Gets the bounding box combined for all meshes in this model LOD.
        /// </summary>
        BoundingBox GetBox() const;

        /// <summary>
        /// Draws the meshes. Binds vertex and index buffers and invokes the draw calls.
        /// </summary>
        /// <param name="context">The GPU context to draw with.</param>
        void Render(GPUContext* context);

        /// <summary>
        /// Draws the meshes from the model LOD.
        /// </summary>
        /// <param name="renderContext">The rendering context.</param>
        /// <param name="material">The material to use for rendering.</param>
        /// <param name="world">The world transformation of the model.</param>
        /// <param name="flags">The object static flags.</param>
        /// <param name="receiveDecals">True if rendered geometry can receive decals, otherwise false.</param>
        /// <param name="drawModes">The draw passes to use for rendering this object.</param>
        /// <param name="perInstanceRandom">The random per-instance value (normalized to range 0-1).</param>
        /// <param name="sortOrder">Object sorting key.</param>
        void Draw(const RenderContext& renderContext, MaterialBase* material, const Matrix& world, EnumFlags<StaticMask> flags = StaticMask::None,
            bool receiveDecals = true, DrawPass drawModes = DrawPass::Default, float perInstanceRandom = 0.0f, int16 sortOrder = 0) const;

        /// <summary>
        /// Draws all the meshes from the model LOD.
        /// </summary>
        /// <param name="renderContext">The rendering context.</param>
        /// <param name="info">The packed drawing info data.</param>
        /// <param name="lodDitherFactor">The LOD transition dither factor.</param>
        void Draw(const RenderContext& renderContext, const Mesh::DrawInfo& info, float lodDitherFactor) const;

        /// <summary>
        /// Draws all the meshes from the model LOD.
        /// </summary>
        /// <param name="renderContextBatch">The rendering context batch.</param>
        /// <param name="info">The packed drawing info data.</param>
        /// <param name="lodDitherFactor">The LOD transition dither factor.</param>
        void Draw(const RenderContextBatch& renderContextBatch, const Mesh::DrawInfo& info, float lodDitherFactor) const;
    };

} // SE

