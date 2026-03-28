#pragma once

#include "Core/Types/Variable.h"

namespace SE
{
    /// <summary>
    /// The objects drawing pass types. Used as a flags for objects drawing masking.
    /// </summary>
    enum class DrawPass : int32
    {
        /// <summary>
        /// The none.
        /// </summary>
        None = 0,

        /// <summary>
        /// The hardware depth rendering to the depth buffer (used for shadow maps rendering).
        /// </summary>
        Depth = 1,

        /// <summary>
        /// The base pass rendering to the GBuffer (for opaque materials).
        /// </summary>
        GBuffer = 1 << 1,

        /// <summary>
        /// The forward pass rendering (for transparent materials).
        /// </summary>
        Forward = 1 << 2,

        /// <summary>
        /// The transparent objects distortion vectors rendering (with blending).
        /// </summary>
        Distortion = 1 << 3,

        /// <summary>
        /// The motion vectors (velocity) rendering pass (for movable objects).
        /// </summary>
        MotionVectors = 1 << 4,

        /// <summary>
        /// The Global Sign Distance Field (SDF) rendering pass. Used for software raytracing though the scene on a GPU.
        /// </summary>
        GlobalSDF = 1 << 5,

        /// <summary>
        /// The Global Surface Atlas rendering pass. Used for software raytracing though the scene on a GPU to evaluate the object surface material properties.
        /// </summary>
        GlobalSurfaceAtlas = 1 << 6,

        /// <summary>
        /// The debug quad overdraw rendering (editor-only).
        /// </summary>
        QuadOverdraw = 1 << 20,

        /// <summary>
        /// The default set of draw passes for the scene objects.
        /// </summary>
        Default = Depth | GBuffer | Forward | Distortion | MotionVectors | GlobalSDF | GlobalSurfaceAtlas,

        /// <summary>
        /// The all draw passes combined into a single mask.
        /// </summary>
        All = Depth | GBuffer | Forward | Distortion | MotionVectors | GlobalSDF | GlobalSurfaceAtlas,
    };

    /// Describes frame rendering modes.
    enum class ViewMode
    {
        /// <summary>
        /// Full rendering
        /// </summary>
        Default = 0,

        /// <summary>
        /// Without post-process pass
        /// </summary>
        NoPostFx = 1,

        /// <summary>
        /// Draw Diffuse
        /// </summary>
        Diffuse = 2,

        /// <summary>
        /// Draw Normals
        /// </summary>
        Normals = 3,

        /// <summary>
        /// Draw Emissive
        /// </summary>
        Emissive = 4,

        /// <summary>
        /// Draw Depth
        /// </summary>
        Depth = 5,

        /// <summary>
        /// Draw Ambient Occlusion
        /// </summary>
        AmbientOcclusion = 6,

        /// <summary>
        /// Draw Material's Metalness
        /// </summary>
        Metalness = 7,

        /// <summary>
        /// Draw Material's Roughness
        /// </summary>
        Roughness = 8,

        /// <summary>
        /// Draw Material's Specular
        /// </summary>
        Specular = 9,

        /// <summary>
        /// Draw Material's Specular Color
        /// </summary>
        SpecularColor = 10,

        /// <summary>
        /// Draw Shading Model
        /// </summary>
        ShadingModel = 11,

        /// <summary>
        /// Draw Lights buffer
        /// </summary>
        LightBuffer = 12,

        /// <summary>
        /// Draw reflections buffer
        /// </summary>
        Reflections = 13,

        /// <summary>
        /// Draw scene objects in wireframe mode
        /// </summary>
        Wireframe = 14,

        /// <summary>
        /// Draw motion vectors debug view
        /// </summary>
        MotionVectors = 15,

        /// <summary>
        /// Draw materials subsurface color debug view
        /// </summary>
        SubsurfaceColor = 16,

        /// <summary>
        /// Draw materials colors with ambient occlusion
        /// </summary>
        Unlit = 17,

        /// <summary>
        /// Draw meshes lightmaps coordinates density
        /// </summary>
        LightmapUVsDensity = 18,

        /// <summary>
        /// Draw meshes vertex colors
        /// </summary>
        VertexColors = 19,

        /// <summary>
        /// Draw physics colliders debug view
        /// </summary>
        PhysicsColliders = 20,

        /// <summary>
        /// Draw Level Of Detail number as colors to debug LOD switches.
        /// </summary>
        LODPreview = 21,

        /// <summary>
        /// Draw material shaders complexity to visualize performance of pixels rendering.
        /// </summary>
        MaterialComplexity = 22,

        /// <summary>
        /// Draw geometry overdraw to visualize performance of pixels rendering.
        /// </summary>
        QuadOverdraw = 23,

        /// <summary>
        /// Draw Global Sign Distant Field (SDF) preview.
        /// </summary>
        GlobalSDF = 24,

        /// <summary>
        /// Draw Global Surface Atlas preview.
        /// </summary>
        GlobalSurfaceAtlas = 25,

        /// <summary>
        /// Draw Global Illumination debug preview (eg. irradiance probes).
        /// </summary>
        GlobalIllumination = 26,
    };

    /// <summary>
    /// Rendering quality levels.
    /// </summary>
    enum class Quality : byte
    {
        /// <summary>
        /// The low quality.
        /// </summary>
        Low = 0,

        /// <summary>
        /// The medium quality.
        /// </summary>
        Medium = 1,

        /// <summary>
        /// The high quality.
        /// </summary>
        High = 2,

        /// <summary>
        /// The ultra, mega, fantastic quality!
        /// </summary>
        Ultra = 3,

        MAX
    };

    /// <summary>
    /// Static flags for the actor object.
    /// </summary>
    enum class StaticMask
    {
        /// <summary>
        /// Non-static object.
        /// </summary>
        None = 0,

        /// <summary>
        /// Object is considered to be static for reflection probes offline caching.
        /// </summary>
        ReflectionProbe = 1 << 0,

        /// <summary>
        /// Object is considered to be static for static lightmaps.
        /// </summary>
        Lightmap = 1 << 1,

        /// <summary>
        /// Object is considered to have static transformation in space (no dynamic physics and any movement at runtime).
        /// </summary>
        Transform = 1 << 2,

        /// <summary>
        /// Object is considered to affect navigation (static occluder or walkable surface).
        /// </summary>
        Navigation = 1 << 3,

        /// <summary>
        /// Object is fully static in the scene.
        /// </summary>
        FullyStatic = Transform | ReflectionProbe | Lightmap | Navigation,

        /// <summary>
        /// Maximum value of the enum (force to int).
        /// </summary>
        MAX = 1 << 31,
    };


    /// <summary>
    /// The mesh buffer types.
    /// </summary>
    enum class MeshBufferType
    {
        /// <summary>
        /// The index buffer.
        /// </summary>
        Index = 0,

        /// <summary>
            /// The vertex buffer (first).
            /// </summary>
        Vertex0 = 1,

        /// <summary>
            /// The vertex buffer (second).
            /// </summary>
        Vertex1 = 2,

        /// <summary>
            /// The vertex buffer (third).
            /// </summary>
        Vertex2 = 3,
    };

    /// <summary>
    /// Shadows casting modes by visual elements.
    /// </summary>
    enum class ShadowsCastingMode
    {
        /// <summary>
        /// Never render shadows.
        /// </summary>
        None = 0,

        /// <summary>
        /// Render shadows only in static views (env probes, lightmaps, etc.).
        /// </summary>
        StaticOnly = 1,

        /// <summary>
        /// Render shadows only in dynamic views (game, editor, etc.).
        /// </summary>
        DynamicOnly = 2,

        /// <summary>
        /// Always render shadows.
        /// </summary>
        All = StaticOnly | DynamicOnly,
    };

    /// <summary>
    /// The partitioning mode for shadow cascades.
    /// </summary>
    enum class PartitionMode
    {
        /// <summary>
        /// Internally defined cascade splits.
        /// </summary>
        Manual = 0,

        /// <summary>
        /// Logarithmic cascade splits.
        /// </summary>
        Logarithmic = 1,

        /// <summary>
        /// PSSM cascade splits.
        /// </summary>
        PSSM = 2,
    };


}
