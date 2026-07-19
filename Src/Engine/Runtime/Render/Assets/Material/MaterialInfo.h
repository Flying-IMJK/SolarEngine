#pragma once

#include "Runtime/Core/Types/BitFlags.h"
#include "Runtime/Core/Types/Variable.h"

#include "Runtime/API.h"
#include "Runtime/Graphics/Base/GPUEnums.h"

namespace SE
{
    /// <summary>
    /// Material domain type. Material domain defines the target usage of the material shader.
    /// </summary>
    enum class MaterialDomain : byte
    {
        /// <summary>
        /// The surface material. Can be used to render the scene geometry including models and skinned models.
        /// </summary>
        Surface = 0,

        /// <summary>
        /// The post process material. Can be used to perform custom post-processing of the rendered frame.
        /// </summary>
        PostProcess = 1,

        /// <summary>
        /// The deferred decal material. Can be used to apply custom overlay or surface modifications to the object surfaces in the world.
        /// </summary>
        Decal = 2,

        /// <summary>
        /// The GUI shader. Can be used to draw custom control interface elements or to add custom effects to the GUI.
        /// </summary>
        GUI = 3,

        /// <summary>
        /// The terrain shader. Can be used only with landscape chunks geometry that use optimized vertex data and support multi-layered blending.
        /// </summary>
        Terrain = 4,

        /// <summary>
        /// The particle shader. Can be used only with particles geometry (sprites, trails and ribbons). Supports reading particle data on a GPU.
        /// </summary>
        Particle = 5,

        /// <summary>
        /// The deformable shader. Can be used only with objects that can be deformed (spline models).
        /// </summary>
        Deformable = 6,

        /// <summary>
        /// The particle shader used for volumetric effects rendering such as Volumetric Fog.
        /// </summary>
        VolumeParticle = 7,

        MAX
    };

    /// <summary>
    /// Material blending modes.
    /// </summary>
    enum class MaterialBlendMode : byte
    {
        /// <summary>
        /// The opaque material. Used during GBuffer pass rendering.
        /// </summary>
        Opaque = 0,

        /// <summary>
        /// The transparent material. Used during Forward pass rendering.
        /// </summary>
        Transparent = 1,

        /// <summary>
        /// The additive blend. Material color is used to add to color of the objects behind the surface. Used during Forward pass rendering.
        /// </summary>
        Additive = 2,

        /// <summary>
        /// The multiply blend. Material color is used to multiply color of the objects behind the surface. Used during Forward pass rendering.
        /// </summary>
        Multiply = 3,
    };

    /// <summary>
    /// Material shading modes. Defines how material inputs and properties are combined to result the final surface color.
    /// </summary>
    enum class MaterialShadingModel : byte
    {
        /// <summary>
        /// The unlit material. The emissive channel is used as an output color. Can perform custom lighting operations or just glow. Won't be affected by the lighting pipeline.
        /// </summary>
        Unlit = 0,

        /// <summary>
        /// The default lit material. The most common choice for the material surfaces.
        /// </summary>
        Lit = 1,

        /// <summary>
        /// The subsurface material. Intended for materials like wax or skin that need light scattering to transport simulation through the object.
        /// </summary>
        Subsurface = 2,

        /// <summary>
        /// The foliage material. Intended for foliage materials like leaves and grass that need light scattering to transport simulation through the thin object.
        /// </summary>
        Foliage = 3,
    };

    /// <summary>
    /// Decal material blending modes.
    /// </summary>
    enum class MaterialDecalBlendingMode : byte
    {
        /// <summary>
        /// Decal will be fully blended with the material surface.
        /// </summary>
        Translucent = 0,

        /// <summary>
        /// Decal color will be blended with the material surface color (using multiplication).
        /// </summary>
        Stain = 1,

        /// <summary>
        /// Decal will blend the normal vector only.
        /// </summary>
        Normal = 2,

        /// <summary>
        /// Decal will apply the emissive light only.
        /// </summary>
        Emissive = 3,
    };

    /// <summary>
    /// Transparent material lighting modes.
    /// </summary>
    enum class MaterialTransparentLightingMode : byte
    {
        /// <summary>
        /// Default directional lighting evaluated per-pixel at the material surface. Use it for semi-transparent surfaces - with both diffuse and specular lighting components active.
        /// </summary>
        Surface = 0,

        /// <summary>
        /// Non-directional lighting evaluated per-pixel at material surface. Use it for volumetric objects such as smoke, rain or dust - only the diffuse lighting term is active (no specular highlights).
        /// </summary>
        SurfaceNonDirectional = 1,
    };

    /// <summary>
    /// Material features flags.
    /// </summary>
    enum class MaterialFeatures : uint32
    {
        /// <summary>
        /// No flags.
        /// </summary>
        None = 0,

        /// <summary>
        /// The wireframe material.
        /// </summary>
        Wireframe = 1 << 1,

        /// <summary>
        /// The depth test is disabled (material ignores depth).
        /// </summary>
        DisableDepthTest = 1 << 2,

        /// <summary>
        /// Disable depth buffer write (won't modify depth buffer value during rendering).
        /// </summary>
        DisableDepthWrite = 1 << 3,

        /// <summary>
        /// The flag used to indicate that material input normal vector is defined in the world space rather than tangent space.
        /// </summary>
        InputWorldSpaceNormal = 1 << 4,

        /// <summary>
        /// The flag used to indicate that material uses dithered model LOD transition for smoother LODs switching.
        /// </summary>
        DitheredLODTransition = 1 << 5,

        /// <summary>
        /// The flag used to disable fog. The Forward Pass materials option.
        /// </summary>
        DisableFog = 1 << 6,

        /// <summary>
        /// The flag used to disable reflections. The Forward Pass materials option.
        /// </summary>
        DisableReflections = 1 << 7,

        /// <summary>
        /// The flag used to disable distortion effect (light refraction). The Forward Pass materials option.
        /// </summary>
        DisableDistortion = 1 << 8,

        /// <summary>
        /// The flag used to enable refraction offset based on the difference between the per-pixel normal and the per-vertex normal. Useful for large water-like surfaces.
        /// </summary>
        PixelNormalOffsetRefraction = 1 << 9,

        /// <summary>
        /// The flag used to enable high-quality reflections based on the screen space raytracing. Useful for large water-like surfaces. The Forward Pass materials option.
        /// </summary>
        ScreenSpaceReflections = 1 << 10,

        /// <summary>
        /// The flag used to enable sampling Global Illumination in material (eg. light probes or volumetric lightmap). The Forward Pass materials option.
        /// </summary>
        GlobalIllumination = 1 << 11,
    };

    /// <summary>
    /// Material features usage flags. Detected by the material generator to help graphics pipeline optimize rendering of material shaders.
    /// </summary>
    enum class MaterialUsage : uint32
    {
        /// <summary>
        /// No flags.
        /// </summary>
        None = 0,

        /// <summary>
        /// Material is using mask to discard some pixels. Masked materials are using full vertex buffer during shadow maps and depth pass rendering (need UVs).
        /// </summary>
        UseMask = 1 << 0,

        /// <summary>
        /// The material is using emissive light.
        /// </summary>
        UseEmissive = 1 << 1,

        /// <summary>
        /// The material is using world position offset (it may be animated inside a shader).
        /// </summary>
        UsePositionOffset = 1 << 2,

        /// <summary>
        /// The material is using vertex colors. The render will try to feed the pipeline with a proper buffer so material can gather valid data.
        /// </summary>
        UseVertexColor = 1 << 3,

        /// <summary>
        /// The material is using per-pixel normal mapping.
        /// </summary>
        UseNormal = 1 << 4,

        /// <summary>
        /// The material is using position displacement (in domain shader).
        /// </summary>
        UseDisplacement = 1 << 5,

        /// <summary>
        /// The flag used to indicate that material uses refraction feature.
        /// </summary>
        UseRefraction = 1 << 6,
    };


    /// <summary>
    /// Material input scene textures. Special inputs from the graphics pipeline.
    /// </summary>
    enum class MaterialSceneTextures
    {
        /// <summary>
        /// The scene color.
        /// </summary>
        SceneColor = 0,

        /// <summary>
        /// The scene depth.
        /// </summary>
        SceneDepth = 1,

        /// <summary>
        /// The material diffuse color.
        /// </summary>
        DiffuseColor = 2,

        /// <summary>
        /// The material specular color.
        /// </summary>
        SpecularColor = 3,

        /// <summary>
        /// The material world space normal.
        /// </summary>
        WorldNormal = 4,

        /// <summary>
        /// The ambient occlusion.
        /// </summary>
        AmbientOcclusion = 5,

        /// <summary>
        /// The material metalness value.
        /// </summary>
        Metalness = 6,

        /// <summary>
        /// The material roughness value.
        /// </summary>
        Roughness = 7,

        /// <summary>
        /// The material specular value.
        /// </summary>
        Specular = 8,

        /// <summary>
        /// The material color.
        /// </summary>
        BaseColor = 9,

        /// <summary>
        /// The material shading mode.
        /// </summary>
        ShadingModel = 10,

        /// <summary>
        /// The scene world-space position (relative to the render view origin).
        /// </summary>
        WorldPosition = 11,
    };


    /// <summary>
    /// Structure with basic information about the material surface. It describes how material is reacting on light and which graphical features of it requires to render.
    /// </summary>
    struct SE_API_RUNTIME MaterialInfo
    {
        /// <summary>
        /// The material shader domain.
        /// </summary>
        MaterialDomain Domain;

        /// <summary>
        /// The blending mode for rendering.
        /// </summary>
        MaterialBlendMode BlendMode;

        /// <summary>
        /// The shading mode for lighting.
        /// </summary>
        MaterialShadingModel ShadingModel;

        /// <summary>
        /// The usage flags.
        /// </summary>
        EnumFlags<MaterialUsage> UsageFlags;

        /// <summary>
        /// The features usage flags.
        /// </summary>
        EnumFlags<MaterialFeatures> FeaturesFlags;

        /// <summary>
        /// The decal material blending mode.
        /// </summary>
        MaterialDecalBlendingMode DecalBlendingMode;

        /// <summary>
        /// The transparent material lighting mode.
        /// </summary>
        MaterialTransparentLightingMode TransparentLightingMode;

        /// <summary>
        /// The post fx material rendering location.
        /// </summary>
        // MaterialPostFxLocation PostFxLocation;

        /// <summary>
        /// The primitives culling mode.
        /// </summary>
        CullMode CullMode;

        /// <summary>
        /// The mask threshold.
        /// </summary>
        float MaskThreshold;

        /// <summary>
        /// The opacity threshold.
        /// </summary>
        float OpacityThreshold;

        /// <summary>
        /// The tessellation mode.
        /// </summary>
        TessellationMethod TessellationMode;

        /// <summary>
        /// The maximum tessellation factor (used only if material uses tessellation).
        /// </summary>
        int32 MaxTessellationFactor;

        MaterialInfo(): Domain(), BlendMode(), ShadingModel(), DecalBlendingMode(), TransparentLightingMode(), CullMode(), MaskThreshold(0), OpacityThreshold(0),
                        TessellationMode(),
                        MaxTessellationFactor(0)
        {
        }

        // MaterialInfo(const MaterialInfo& other);
        bool operator==(const MaterialInfo& other) const;
    };
}
