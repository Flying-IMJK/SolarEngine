#pragma once

#include "Core/Math/Color.h"
#include "Runtime/Level/Actor.h"
#include "Runtime/Render/RenderView.h"
#include "Runtime/Render/SceneRendering.h"

namespace SE
{
    /// <summary>
    /// Base class for all light types.
    /// </summary>
    SE_CLASS(Reflect)
    class SE_API_RUNTIME Light : public RenderActor
    {
        SE_DEFINE_CLASS(Light, RenderActor);

    protected:
        int32 _sceneRenderingKey = -1;

    public:
        /// <summary>
        /// Color of the light
        /// </summary>
        // SE_PROPERTY(API, Attributes="EditorOrder(20), EditorDisplay(\"Light\")")
        Color Color = Colors::White;

        /// <summary>
        /// Brightness of the light
        /// </summary>
        // SE_PROPERTY(API, Attributes="EditorOrder(30), EditorDisplay(\"Light\"), Limit(0.0f, 100000000.0f, 0.1f)")
        float Brightness = 3.14f;

        /// <summary>
        /// Controls light visibility range. The distance at which the light becomes completely faded. Use a value of 0 to always draw light.
        /// </summary>
        // SE_PROPERTY(API, Attributes="EditorOrder(35), Limit(0, float.MaxValue, 10.0f), EditorDisplay(\"Light\")")
        float ViewDistance = 0.0f;

        /// <summary>
        /// Controls how much this light will contribute indirect lighting. When set to 0, there is no GI from the light. The default value is 1.
        /// </summary>
        // SE_PROPERTY(API, Attributes="EditorOrder(40), DefaultValue(1.0f), Limit(0, 100, 0.1f), EditorDisplay(\"Light\", \"Indirect Lighting Intensity\")")
        float IndirectLightingIntensity = 1.0f;

        /// <summary>
        /// Controls how much this light will contribute to the Volumetric Fog. When set to 0, there is no contribution.
        /// </summary>
        // SE_PROPERTY(API, Attributes="EditorOrder(110),DefaultValue(1.0f),  Limit(0, 100, 0.01f), EditorDisplay(\"Volumetric Fog\", \"Scattering Intensity\")")
        float VolumetricScatteringIntensity = 1.0f;

        /// <summary>
        /// Toggles whether or not to cast a volumetric shadow for lights contributing to Volumetric Fog. Also shadows casting by this light should be enabled in order to make it cast volumetric fog shadow.
        /// </summary>
        // SE_PROPERTY(API, Attributes="EditorOrder(120), EditorDisplay(\"Volumetric Fog\", \"Cast Shadow\")")
        bool CastVolumetricShadow = true;

    protected:
        // Adjust the light brightness used during rendering (called by light types inside SetupLightData callback)
        void AdjustBrightness(const RenderView& view, float& brightness) const;

    public:
        // [RenderActor]
        void OnEnable() override;
        void OnDisable() override;

        Light();

#if SE_EDITOR
        /*BoundingBox GetEditorBox() const override
        {
            const Float3 size(50);
            return BoundingBox(_transform.Translation - size, _transform.Translation + size);
        }*/

        virtual void DrawLightsDebug(RenderView& view);
#endif

        void Serialize(SerializeContext& context) override;
        void Deserialize(DeserializeContext& context) override;
    };

    /// <summary>
    /// Base class for all light types that can cast dynamic or static shadow. Contains more shared properties for point/spot/directional lights.
    /// </summary>
    SE_CLASS(Reflect)
    class SE_API_RUNTIME LightWithShadow : public Light
    {
        SE_DEFINE_CLASS(LightWithShadow, Light);
    public:
        /// <summary>
        /// The minimum roughness value used to clamp material surface roughness during shading pixel.
        /// </summary>
        // SE_PROPERTY(API, Attributes="EditorOrder(40), EditorDisplay(\"Light\", \"Minimum Roughness\"), Limit(0.0f, 1.0f, 0.01f)")
        float MinRoughness = 0.04f;

        /// <summary>
        /// Shadows casting distance from view.
        /// </summary>
        // SE_PROPERTY(API, Attributes="EditorOrder(80), EditorDisplay(\"Shadow\", \"Distance\"), Limit(0, 1000000)")
        float ShadowsDistance = 5000.0f;

        /// <summary>
        /// Shadows fade off distance.
        /// </summary>
        // SE_PROPERTY(API, Attributes="EditorOrder(90), EditorDisplay(\"Shadow\", \"Fade Distance\"), Limit(0.0f, 10000.0f, 0.1f)")
        float ShadowsFadeDistance = 500.0f;

        /// <summary>
        /// TheShadows edges sharpness.
        /// </summary>
        // SE_PROPERTY(API, Attributes="EditorOrder(70), EditorDisplay(\"Shadow\", \"Sharpness\"), Limit(1.0f, 10.0f, 0.001f)")
        float ShadowsSharpness = 1.0f;

        /// <summary>
        /// Dynamic shadows blending strength. Default is 1 for fully opaque shadows, value 0 disables shadows.
        /// </summary>
        // SE_PROPERTY(API, Attributes="EditorOrder(75), EditorDisplay(\"Shadow\", \"Strength\"), Limit(0.0f, 1.0f, 0.001f)")
        float ShadowsStrength = 1.0f;

        /// <summary>
        /// The depth bias used for shadow map comparison.
        /// </summary>
        // SE_PROPERTY(API, Attributes="EditorOrder(95), EditorDisplay(\"Shadow\", \"Depth Bias\"), Limit(0.0f, 10.0f, 0.0001f)")
        float ShadowsDepthBias = 0.005f;

        /// <summary>
        /// A factor specifying the offset to add to the calculated shadow map depth with respect to the surface normal.
        /// </summary>
        // SE_PROPERTY(API, Attributes="EditorOrder(96), EditorDisplay(\"Shadow\", \"Normal Offset Scale\"), Limit(0.0f, 100.0f, 0.1f)")
        float ShadowsNormalOffsetScale = 10.0f;

        /// <summary>
        /// The length of the rays for contact shadows computed via the screen-space tracing. Set this to values higher than 0 to enable screen-space shadows rendering for this light. This improves the shadowing details. Actual ray distance is based on the pixel distance from the camera.
        /// </summary>
        // SE_PROPERTY(API, Attributes="EditorOrder(99), EditorDisplay(\"Shadow\"), Limit(0.0f, 0.1f, 0.001f)")
        float ContactShadowsLength = 0.0f;

        /// <summary>
        /// Describes how a visual element casts shadows.
        /// </summary>
        // SE_PROPERTY(API, Attributes="EditorOrder(60), EditorDisplay(\"Shadow\", \"Mode\")")
        ShadowsCastingMode ShadowsMode = ShadowsCastingMode::All;

        // [Light]
        void Serialize(SerializeContext& context) override;
        void Deserialize(DeserializeContext& context) override;

        LightWithShadow();
    };

} // SE

