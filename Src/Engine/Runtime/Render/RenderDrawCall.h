#pragma once

#include <Runtime/Core/Math/Color.h>
#include <Runtime/Core/Math/Matrix.h>
#include <Runtime/Core/Math/Rectangle.h>
#include <Runtime/Core/Types/Variable.h>
#include <Runtime/Graphics/Base/PixelFormat.h>

namespace SE
{
    struct RenderView;
    struct RenderContext;
    struct DrawCall;
    struct RenderDraw;
    class IMaterial;
    class ShaderProgramInstance;
    class MaterialBase;
    class RenderBuffers;
    class GPUTextureView;
    class GPUContext;
    class GPUBuffer;
    class VertexFactoryLayout;
    class GPUTexture;

    /// <summary>
    /// Interface for objects that can render custom sky
    /// </summary>
    class ISkyRenderer
    {
    public:

        /// <summary>
        /// Returns true if sky is realtime, otherwise it's static.
        /// </summary>
        virtual bool IsDynamicSky() const = 0;

        /// <summary>
        /// Apply sky material/shader state to the GPU pipeline with custom parameters set (render to GBuffer).
        /// </summary>
        /// <param name="context">The context responsible for rendering commands.</param>
        /// <param name="renderContext">The rendering context.</param>
        /// <param name="world">The world matrix to use during sky model vertices transformations.</param>
        virtual ShaderProgramInstance* ApplySky(GPUContext* context, RenderContext& renderContext, const Matrix& world) = 0;
    };

    /// Exponential height fog settings. Distances are meters; colors use linear RGB only.
    SE_STRUCT(API())
    struct SE_API_RUNTIME ExponentialHeightFogData
    {
        SCRIPTING_TYPE_MIN(ExponentialHeightFogData);

        SE_FIELD(API(Attributes = "EditorDisplay(\"Fog\"), Limit(0, 10, 0.001f)"))
        float FogDensity = 0.02f;

        SE_FIELD(API(Attributes = "EditorDisplay(\"Fog\"), Limit(0, 2, 0.001f)"))
        float FogHeightFalloff = 0.2f;

        SE_FIELD(API(Attributes = "EditorDisplay(\"Second Fog\"), Limit(0, 10, 0.001f)"))
        float SecondFogDensity = 0.0f;

        SE_FIELD(API(Attributes = "EditorDisplay(\"Second Fog\"), Limit(0, 2, 0.001f)"))
        float SecondFogHeightFalloff = 0.2f;

        /// Height relative to the actor, in meters.
        SE_FIELD(API(Attributes = "EditorDisplay(\"Second Fog\")"))
        float SecondFogHeightOffset = 0.0f;

        SE_FIELD(API(Attributes = "EditorDisplay(\"Fog\"), HideAlphaChannel"))
        Color FogInscatteringColor = Colors::Black;

        SE_FIELD(API(Attributes = "EditorDisplay(\"Fog\"), Limit(0, 1, 0.01f)"))
        float FogMaxOpacity = 1.0f;

        SE_FIELD(API(Attributes = "EditorDisplay(\"Fog\"), Limit(0, float.MaxValue)"))
        float StartDistance = 0.0f;

        /// Elements beyond this distance receive no fog; zero disables the cutoff.
        SE_FIELD(API(Attributes = "EditorDisplay(\"Fog\"), Limit(0, float.MaxValue)"))
        float FogCutoffDistance = 0.0f;

        SE_FIELD(API(Attributes = "EditorDisplay(\"Directional Inscattering\"), HideAlphaChannel"))
        Color DirectionalInscatteringColor = Colors::Black;

        SE_FIELD(API(Attributes = "EditorDisplay(\"Directional Inscattering\"), Limit(0.000001f, 1000)"))
        float DirectionalInscatteringExponent = 4.0f;

        SE_FIELD(API(Attributes = "EditorDisplay(\"Directional Inscattering\"), Limit(0, float.MaxValue)"))
        float DirectionalInscatteringStartDistance = 100.0f;

        SE_FIELD(API(Attributes = "EditorDisplay(\"Sky Atmosphere\"), HideAlphaChannel"))
        Color SkyAtmosphereAmbientContributionColorScale = Colors::White;

        /// Runtime world height filled by the provider; not serialized as a setting.
        SE_FIELD(API(Attributes = "HideInEditor"))
        float FogHeight = 0.0f;
    };

    /// Non-owning scene provider. Called on the render thread before fog drawing.
    class IAtmosphericFogRenderer
    {
    public:
        virtual void ApplyAtmosphericFog(ExponentialHeightFogData& data) = 0;
    };

    SE_STRUCT(API())
    struct SE_API_RUNTIME SkyAtmosphereData
    {
        SCRIPTING_TYPE_MIN(SkyAtmosphereData);
        // Atmosphere lengths are km; linear color RGB multiplied by its scale gives coefficients in km^-1.
        // Alpha is unused by the atmosphere model.
        SE_FIELD(API(Attributes = "Limit(0.001f, 1000000f)"))
        float GroundRadius = 6360.0f;

        SE_FIELD(API(Attributes = "Limit(0.001f, 1000000)"))
        float AtmosphereHeight = 60.0f;

        SE_FIELD(API(Attributes = "Limit(0.001f, 1000000f)"))
        float RayleighScaleHeight = 8.0f;

        SE_FIELD(API(Attributes = "Limit(0.0f, float.MaxValue, 0.001f)"))
        float RayleighScatteringScale = 0.033100f;

        SE_FIELD(API(Attributes = "HideAlphaChannel"))
        Color RayleighScattering = Color(0.005802f / 0.033100f, 0.013558f / 0.033100f, 1.0f);

        SE_FIELD(API(Attributes = "Limit(0.001f, 1000000f)"))
        float MieScaleHeight = 1.2f;

        SE_FIELD(API(Attributes = "Limit(0.0f, float.MaxValue, 0.001f)"))
        float MieScatteringScale = 0.003996f;

        SE_FIELD(API(Attributes = "HideAlphaChannel"))
        Color MieScattering = Colors::White;

        SE_FIELD(API(Attributes = "Limit(0.0f, float.MaxValue, 0.001f)"))
        float MieAbsorptionScale = 0.000444f;

        SE_FIELD(API(Attributes = "HideAlphaChannel"))
        Color MieAbsorption = Colors::White;

        SE_FIELD(API(Attributes = "Limit(-0.999f, 0.999f)"))
        float MieAnisotropy = 0.8f;

        SE_FIELD(API(Attributes = "Limit(0.0f, float.MaxValue, 0.001f)"))
        float AbsorptionExtinctionScale = 0.001881f;

        SE_FIELD(API(Attributes = "HideAlphaChannel"))
        Color AbsorptionExtinction = Color(0.000650f / 0.001881f, 1.0f, 0.000085f / 0.001881f);

        SE_FIELD(API(Attributes = "Limit(0.0f, 1000000.0f)"))
        float AbsorptionTipAltitude = 25.0f;

        SE_FIELD(API(Attributes = "Limit(0.001f, 1000000.0f)"))
        float AbsorptionWidth = 15.0f;

        SE_FIELD(API(Attributes = "HideAlphaChannel"))
        Color GroundAlbedo = Color(0.4f, 0.4f, 0.4f);

        SE_FIELD(API(Attributes = "Limit(0.0f, 100.0f)"))
        float MultiScatteringFactor = 1.0f;

        /// Scales sky and atmospheric sunlight contributions to exponential height fog.
        SE_FIELD(API(Attributes = "Limit(0, float.MaxValue, 0.01f)"))
        float HeightFogContribution = 1.0f;

        SE_FIELD(API(Attributes = "Limit(0.0f, 1000000.0f)"))
        float AerialPerspectiveStartDepth = 0.1f;

        SE_FIELD(API(Attributes = "Limit(0.001f, 1000000.0f)"))
        float AerialPerspectiveDepth = 96.0f;

        // Runtime values must participate in the native/managed struct layout even when hidden.
        /// World-space ground reference point in meters; filled by the parameter provider.
        SE_FIELD(API(Attributes = "HideInEditor"))
        Float3 GroundPosition = Float3::Zero;

        /// Normalized direction toward the sun, in world space.
        SE_FIELD(API(Attributes = "HideInEditor"))
        Float3 SunDirection = Float3::Up;

        /// Linear light color multiplied by engine light intensity; zero disables sunlight.
        SE_FIELD(API(Attributes = "HideInEditor"))
        Float3 SunIlluminance = Float3::Zero;

        /// Multiplier of the nominal 0.545-degree sun angular diameter; zero hides the disk.
        SE_FIELD(API(Attributes = "HideInEditor"))
        float SunDiscScale = 1.0f;
    };

    class ISkyAtmosphereRenderer
    {
    public:
        /// Fills a parameter snapshot. GPU work and resources belong to the rendering pipeline.
        virtual void ApplySkyAtmosphere(SkyAtmosphereData& data) = 0;
    };


    /// <summary>
    /// Renderer draw call used for dynamic batching process.
    /// </summary>
    struct DrawCall
    {
        /// <summary>
        /// The material to use for rendering.
        /// </summary>
        IMaterial* Material;
         
        struct
        {
            const VertexFactoryLayout* vertexLayout;
            PixelFormat                IndexFormat;
            uint32                     IndexOffset;

            /// <summary>
            /// The geometry index buffer (cannot be null).
            /// </summary>
            GPUBuffer* IndexBuffer;

            /// <summary>
            /// The geometry vertex buffers.
            /// </summary>
            GPUBuffer* VertexBuffers[3];

            /// <summary>
            /// The geometry vertex buffers byte offsets.
            /// </summary>
            uint32 VertexBuffersOffsets[3];
        } Geometry;

        /// <summary>
        /// The amount of instances of the geometry to draw. Set to 0 if use indirect draw arguments buffer.
        /// </summary>
        int32 InstanceCount;

        union
        {
            struct
            {
                /// <summary>
                /// The location of the first index read by the GPU from the index buffer.
                /// </summary>
                int32 StartIndex;

                /// <summary>
                /// The indices count.
                /// </summary>
                int32 IndicesCount;
            };

            struct
            {
                /// <summary>
                /// The indirect draw arguments offset.
                /// </summary>
                uint32 IndirectArgsOffset;

                /// <summary>
                /// The indirect draw arguments buffer.
                /// </summary>
                GPUBuffer* IndirectArgsBuffer;
            };
        } Draw;

        // Per-material shader data packed into union
        union
        {
            struct
            {
                // const Lightmap* Lightmap;
                Rectangle LightmapUVsArea;
            } Features;

            struct
            {
                // const Lightmap* Lightmap;
                Rectangle LightmapUVsArea;
                // SkinnedMeshDrawData* Skinning;
                Float3 GeometrySize; // Object geometry size in the world (unscaled).
                float LODDitherFactor; // The model LOD transition dither progress.
                Matrix PrevWorld;
            } Surface;

            struct
            {
                // const Lightmap* Lightmap;
                Rectangle LightmapUVsArea;
                Float4 HeightmapUVScaleBias;
                Float4 NeighborLOD;
                Float2 OffsetUV;
                float CurrentLOD;
                float ChunkSizeNextLOD;
                float TerrainChunkSizeLOD0;
                const class TerrainPatch* Patch;
            } Terrain;

            struct
            {
                class ParticleBuffer* Particles;
                class ParticleEmitterGraphCPUNode* Module;

                struct
                {
                    int32 OrderOffset;
                    float UVTilingDistance;
                    float UVScaleX;
                    float UVScaleY;
                    float UVOffsetX;
                    float UVOffsetY;
                    uint32 SegmentCount;
                } Ribbon;

                struct
                {
                    Float3 Position;
                    float Radius;
                    int32 ParticleIndex;
                } VolumetricFog;
            } Particle;

            struct
            {
                GPUBuffer* SplineDeformation;
                Matrix LocalMatrix; // Geometry transformation applied before deformation.
                Float3 GeometrySize; // Object geometry size in the world (unscaled).
                float Segment;
                float ChunksPerSegment;
                float MeshMinZ;
                float MeshMaxZ;
            } Deformable;

            struct
            {
                byte Raw[96];
            } Custom;
        };

        /// <summary>
        /// Object world transformation matrix.
        /// </summary>
        Matrix World;

        /// <summary>
        /// Object location in the world used for draw calls sorting.
        /// </summary>
        Float3 ObjectPosition;

        /// <summary>
        /// Object bounding sphere radius that contains it whole (sphere at ObjectPosition).
        /// </summary>
        float ObjectRadius;

        /// <summary>
        /// The world matrix determinant sign (used for geometry that is two sided or has inverse scale - needs to flip normal vectors and change triangles culling).
        /// </summary>
        float WorldDeterminantSign;

        /// <summary>
        /// The random per-instance value (normalized to range 0-1).
        /// </summary>
        float PerInstanceRandom;

        /// <summary>
        /// The sorting key for the draw call calculate by RenderList.
        /// </summary>
        uint64 SortKey;

        /// <summary>
        /// Zero-init.
        /// </summary>
        FORCE_INLINE DrawCall()
        {
            Platform::MemoryClear(this, sizeof(DrawCall));
        }
    };

    /// <summary>
    /// Data container for meshes and skinned meshes rendering with minimal state caching.
    /// Used to update previous world transformation matrix for motion vectors pass and handle LOD transitions blending.
    /// </summary>
    struct GeometryDrawStateData
    {
        /// <summary>
        /// The previous frame world transformation matrix for the given geometry instance.
        /// </summary>
        Matrix PrevWorld = Matrix::Identity;

        /// <summary>
        /// The previous frame index. In sync with Engine::FrameCount used to detect new frames and rendering gaps to reset state.
        /// </summary>
        uint64 PrevFrame = 0;

        /// <summary>
        /// The previous frame model LOD index used. It's locked during LOD transition to cache the transition start LOD.
        /// </summary>
        char PrevLOD = -1;

        /// <summary>
        /// The LOD transition timer. Value 255 means the end of the transition (aka no transition), value 0 means transition started.
        /// Interpolated between 0-255 to smooth transition over several frames and reduce LOD changing artifacts.
        /// </summary>
        byte LODTransition = 255;
    };



#define GEOMETRY_DRAW_STATE_EVENT_BEGIN(drawState, worldMatrix) \
const auto frame = Engine::FrameCount; \
if (drawState.PrevFrame + 1 < frame && !renderContext.view.IsSingleFrame) \
{ \
drawState.PrevWorld = worldMatrix; \
}

#define GEOMETRY_DRAW_STATE_EVENT_END(drawState, worldMatrix) \
if (drawState.PrevFrame != frame && !renderContext.view.IsSingleFrame) \
{ \
drawState.PrevWorld = worldMatrix; \
drawState.PrevFrame = frame; \
}

}

template<>
struct TIsPODType<SE::DrawCall>
{
    enum { Value = true };
};

template<>
struct TIsPODType<SE::GeometryDrawStateData>
{
    enum { Value = true };
};
