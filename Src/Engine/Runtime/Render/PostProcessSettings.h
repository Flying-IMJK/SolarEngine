#pragma once


#include "Runtime/Core/Math/Vector4.h"
#include "Runtime/Core/Serialization/ISerializable.h"
#include "Runtime/Core/TypeSystem/IType.h"

#include "Runtime/API.h"
#include "Runtime/Render/Assets/Texture/Texture.h"
#include "Runtime/Resource/AssetRef.h"

namespace SE
{
    /// </summary>
    SE_ENUM(API())
    enum class ToneMappingMode
    {
        /// <summary>
        /// Disabled tone mapping effect.
        /// </summary>
        None = 0,

        /// <summary>
        /// The neutral tonemapper.
        /// </summary>
        Neutral = 1,

        /// <summary>
        /// The ACES Filmic reference tonemapper (approximation).
        /// </summary>
        ACES = 2,
    };

    /// <summary>
    /// Contains settings for Tone Mapping effect rendering.
    /// </summary>
    SE_STRUCT(API())
    struct SE_API_RUNTIME ToneMappingSettings : ISerializable
    {
        SCRIPTING_TYPE_MIN(ToneMappingSettings);

        /// <summary>
        /// Adjusts the white balance in relation to the temperature of the light in the scene. When the light temperature and this one match the light will appear white. When a value is used that is higher than the light in the scene it will yield a "warm" or yellow color, and, conversely, if the value is lower, it would yield a "cool" or blue color.
        /// </summary>
        SE_FIELD(API(Attributes = "Limit(1500, 15000), EditorOrder(0)"))
        float WhiteTemperature = 6500.0f;

        /// <summary>
        /// Adjusts the white balance temperature tint for the scene by adjusting the cyan and magenta color ranges. Ideally, this setting should be used once you've adjusted the white balance temperature to get accurate colors. Under some light temperatures, the colors may appear to be more yellow or blue. This can be used to balance the resulting color to look more natural.
        /// </summary>
        SE_FIELD(API(Attributes = "Limit(-1, 1, 0.001f), EditorOrder(1)"))
        float WhiteTint = 0.0f;

        /// <summary>
        /// The tone mapping mode to use for the color grading process.
        /// </summary>
        SE_FIELD(API(Attributes = "EditorOrder(2)"))
        ToneMappingMode Mode = ToneMappingMode::ACES;

    public:
        /// <summary>
        /// Blends the settings using given weight.
        /// </summary>
        /// <param name="other">The other settings.</param>
        /// <param name="weight">The blend weight.</param>
        void BlendWith(ToneMappingSettings& other, float weight);

    public:
        // [ISerializable]
        void Serialize(SerializeContext& context) override;
        void Deserialize(DeserializeContext& context) override;
    };


    /// <summary>
    /// Contains settings for Color Grading effect rendering.
    /// </summary>
    SE_STRUCT(API())
    struct SE_API_RUNTIME ColorGradingSettings : ISerializable
    {
        SCRIPTING_TYPE_MIN(ColorGradingSettings);
        // Global

        /// <summary>
        /// Gets or sets the color saturation (applies globally to the whole image). Default is 1.
        /// </summary>
        SE_FIELD(API(Attributes = "DefaultValue(typeof(Float4), \"1,1,1,1\"), EditorOrder(0), Limit(0, 2, 0.01f), EditorDisplay(\"Global\", \"Saturation\")"))
        Float4 ColorSaturation = Float4::One;

        /// <summary>
        /// Gets or sets the color contrast (applies globally to the whole image). Default is 1.
        /// </summary>
        SE_FIELD(API(Attributes = "DefaultValue(typeof(Float4), \"1,1,1,1\"), EditorOrder(1), Limit(0, 2, 0.01f), EditorDisplay(\"Global\", \"Contrast\")"))
        Float4 ColorContrast = Float4::One;

        /// <summary>
        /// Gets or sets the color gamma (applies globally to the whole image). Default is 1.
        /// </summary>
        SE_FIELD(API(Attributes = "DefaultValue(typeof(Float4), \"1,1,1,1\"), EditorOrder(2), Limit(0, 2, 0.01f), EditorDisplay(\"Global\", \"Gamma\")"))
        Float4 ColorGamma = Float4::One;

        /// <summary>
        /// Gets or sets the color gain (applies globally to the whole image). Default is 1.
        /// </summary>
        SE_FIELD(API(Attributes = "DefaultValue(typeof(Float4), \"1,1,1,1\"), EditorOrder(3), Limit(0, 2, 0.01f), EditorDisplay(\"Global\", \"Gain\")"))
        Float4 ColorGain = Float4::One;

        /// <summary>
        /// Gets or sets the color offset (applies globally to the whole image). Default is 0.
        /// </summary>
        SE_FIELD(API(Attributes = "DefaultValue(typeof(Float4), \"0,0,0,0\"), EditorOrder(4), Limit(-1, 1, 0.001f), EditorDisplay(\"Global\", \"Offset\")"))
        Float4 ColorOffset = Float4::Zero;

        // Shadows

        /// <summary>
        /// Gets or sets the color saturation (applies to shadows only). Default is 1.
        /// </summary>
        SE_FIELD(API(Attributes = "DefaultValue(typeof(Float4), \"1,1,1,1\"), EditorOrder(5), Limit(0, 2, 0.01f), EditorDisplay(\"Shadows\", \"Shadows Saturation\")"))
        Float4 ColorSaturationShadows = Float4::One;

        /// <summary>
        /// Gets or sets the color contrast (applies to shadows only). Default is 1.
        /// </summary>
        SE_FIELD(API(Attributes = "DefaultValue(typeof(Float4), \"1,1,1,1\"), EditorOrder(6), Limit(0, 2, 0.01f), EditorDisplay(\"Shadows\", \"Shadows Contrast\")"))
        Float4 ColorContrastShadows = Float4::One;

        /// <summary>
        /// Gets or sets the color gamma (applies to shadows only). Default is 1.
        /// </summary>
        SE_FIELD(API(Attributes = "DefaultValue(typeof(Float4), \"1,1,1,1\"), EditorOrder(7), Limit(0, 2, 0.01f), EditorDisplay(\"Shadows\", \"Shadows Gamma\")"))
        Float4 ColorGammaShadows = Float4::One;

        /// <summary>
        /// Gets or sets the color gain (applies to shadows only). Default is 1.
        /// </summary>
        SE_FIELD(API(Attributes = "DefaultValue(typeof(Float4), \"1,1,1,1\"), EditorOrder(8), Limit(0, 2, 0.01f), EditorDisplay(\"Shadows\", \"Shadows Gain\")"))
        Float4 ColorGainShadows = Float4::One;

        /// <summary>
        /// Gets or sets the color offset (applies to shadows only). Default is 0.
        /// </summary>
        SE_FIELD(API(Attributes = "DefaultValue(typeof(Float4), \"0,0,0,0\"), EditorOrder(9), Limit(-1, 1, 0.001f), EditorDisplay(\"Shadows\", \"Shadows Offset\")"))
        Float4 ColorOffsetShadows = Float4::Zero;

        // Midtones

        /// <summary>
        /// Gets or sets the color saturation (applies to midtones only). Default is 1.
        /// </summary>
        SE_FIELD(API(Attributes = "DefaultValue(typeof(Float4), \"1,1,1,1\"), EditorOrder(10), Limit(0, 2, 0.01f), EditorDisplay(\"Midtones\", \"Midtones Saturation\")"))
        Float4 ColorSaturationMidtones = Float4::One;

        /// <summary>
        /// Gets or sets the color contrast (applies to midtones only). Default is 1.
        /// </summary>
        SE_FIELD(API(Attributes = "DefaultValue(typeof(Float4), \"1,1,1,1\"), EditorOrder(11), Limit(0, 2, 0.01f), EditorDisplay(\"Midtones\", \"Midtones Contrast\")"))
        Float4 ColorContrastMidtones = Float4::One;

        /// <summary>
        /// Gets or sets the color gamma (applies to midtones only). Default is 1.
        /// </summary>
        SE_FIELD(API(Attributes = "DefaultValue(typeof(Float4), \"1,1,1,1\"), EditorOrder(12), Limit(0, 2, 0.01f), EditorDisplay(\"Midtones\", \"Midtones Gamma\")"))
        Float4 ColorGammaMidtones = Float4::One;

        /// <summary>
        /// Gets or sets the color gain (applies to midtones only). Default is 1.
        /// </summary>
        SE_FIELD(API(Attributes = "DefaultValue(typeof(Float4), \"1,1,1,1\"), EditorOrder(13), Limit(0, 2, 0.01f), EditorDisplay(\"Midtones\", \"Midtones Gain\")"))
        Float4 ColorGainMidtones = Float4::One;

        /// <summary>
        /// Gets or sets the color offset (applies to midtones only). Default is 0.
        /// </summary>
        SE_FIELD(API(Attributes = "DefaultValue(typeof(Float4), \"0,0,0,0\"), EditorOrder(14), Limit(-1, 1, 0.001f), EditorDisplay(\"Midtones\", \"Midtones Offset\")"))
        Float4 ColorOffsetMidtones = Float4::Zero;

        // Highlights

        /// <summary>
        /// Gets or sets the color saturation (applies to highlights only). Default is 1.
        /// </summary>
        SE_FIELD(API(Attributes = "DefaultValue(typeof(Float4), \"1,1,1,1\"), EditorOrder(15), Limit(0, 2, 0.01f), EditorDisplay(\"Highlights\", \"Highlights Saturation\")"))
        Float4 ColorSaturationHighlights = Float4::One;

        /// <summary>
        /// Gets or sets the color contrast (applies to highlights only). Default is 1.
        /// </summary>
        SE_FIELD(API(Attributes = "DefaultValue(typeof(Float4), \"1,1,1,1\"), EditorOrder(16), Limit(0, 2, 0.01f), EditorDisplay(\"Highlights\", \"Highlights Contrast\")"))
        Float4 ColorContrastHighlights = Float4::One;

        /// <summary>
        /// Gets or sets the color gamma (applies to highlights only). Default is 1.
        /// </summary>
        SE_FIELD(API(Attributes = "DefaultValue(typeof(Float4), \"1,1,1,1\"), EditorOrder(17), Limit(0, 2, 0.01f), EditorDisplay(\"Highlights\", \"Highlights Gamma\")"))
        Float4 ColorGammaHighlights = Float4::One;

        /// <summary>
        /// Gets or sets the color gain (applies to highlights only). Default is 1.
        /// </summary>
        SE_FIELD(API(Attributes = "DefaultValue(typeof(Float4), \"1,1,1,1\"), EditorOrder(18), Limit(0, 2, 0.01f), EditorDisplay(\"Highlights\", \"Highlights Gain\")"))
        Float4 ColorGainHighlights = Float4::One;

        /// <summary>
        /// Gets or sets the color offset (applies to highlights only). Default is 0.
        /// </summary>
        SE_FIELD(API(Attributes = "DefaultValue(typeof(Float4), \"0,0,0,0\"), EditorOrder(19), Limit(-1, 1, 0.001f), EditorDisplay(\"Highlights\", \"Highlights Offset\")"))
        Float4 ColorOffsetHighlights = Float4::Zero;

        //

        /// <summary>
        /// The shadows maximum value. Default is 0.09.
        /// </summary>
        SE_FIELD(API(Attributes = "Limit(-1, 1, 0.01f), EditorOrder(20)"))
        float ShadowsMax = 0.09f;

        /// <summary>
        /// The highlights minimum value. Default is 0.5.
        /// </summary>
        SE_FIELD(API(Attributes = "Limit(-1, 1, 0.01f), EditorOrder(21)"))
        float HighlightsMin = 0.5f;

        //

        /// <summary>
        /// The Lookup Table (LUT) used to perform color correction.
        /// </summary>
        SE_FIELD(API(Attributes = "EditorOrder(22)"))
        SoftAssetRef<Texture> LutTexture;

        /// <summary>
        /// The LUT blending weight (normalized to range 0-1). Default is 1.0.
        /// </summary>
        SE_FIELD(API(Attributes = "Limit(0, 1, 0.01f), EditorOrder(23)"))
        float LutWeight = 1.0f;

    public:
        /// <summary>
        /// Blends the settings using given weight.
        /// </summary>
        /// <param name="other">The other settings.</param>
        /// <param name="weight">The blend weight.</param>
        void BlendWith(ColorGradingSettings& other, float weight);

    public:
        // [ISerializable]
        void Serialize(SerializeContext& context) override;
        void Deserialize(DeserializeContext& context) override;
    };

    /// Automatic photometric exposure. EV limits include exposure compensation.
    SE_STRUCT(API())
    struct SE_API_RUNTIME AutoExposureSettings
    {
        SCRIPTING_TYPE_MIN(AutoExposureSettings);

        /// Positive stops brighten the image before tone mapping.
        SE_FIELD(API(Attributes = "EditorDisplay(\"Auto Exposure\"), Limit(-16, 16, 0.1f)"))
        float ExposureCompensation = 0.0f;

        SE_FIELD(API(Attributes = "EditorDisplay(\"Auto Exposure\"), Limit(-16, 32, 0.1f)"))
        float MinEV100 = -10.0f;

        SE_FIELD(API(Attributes = "EditorDisplay(\"Auto Exposure\"), Limit(-16, 32, 0.1f)"))
        float MaxEV100 = 20.0f;

        /// Fractions of valid pixels retained for logarithmic luminance metering.
        SE_FIELD(API(Attributes = "EditorDisplay(\"Auto Exposure\"), Limit(0, 1, 0.01f)"))
        float LowPercentile = 0.5f;

        SE_FIELD(API(Attributes = "EditorDisplay(\"Auto Exposure\"), Limit(0, 1, 0.01f)"))
        float HighPercentile = 0.95f;

        /// Inverse-second adaptation rates toward a brighter or darker environment.
        SE_FIELD(API(Attributes = "EditorDisplay(\"Auto Exposure\"), Limit(0.01f, 100, 0.1f)"))
        float SpeedUp = 3.0f;

        SE_FIELD(API(Attributes = "EditorDisplay(\"Auto Exposure\"), Limit(0.01f, 100, 0.1f)"))
        float SpeedDown = 1.0f;
    };

    // <summary>
    /// Contains settings for rendering advanced visual effects and post effects.
    /// </summary>
    SE_STRUCT(API)
    struct SE_API_RUNTIME PostProcessSettings : ISerializable
    {
        SCRIPTING_TYPE_MIN(PostProcessSettings)
        /// <summary>
        /// The ambient occlusion effect settings.
        /// </summary>
        // SE_PROPERTY(API(Attributes="EditorDisplay(\"Ambient Occlusion\"), EditorOrder(100), JsonProperty(\"AO\")"))
        // AmbientOcclusionSettings AmbientOcclusion;

        /// <summary>
        /// The global illumination effect settings.
        /// </summary>
        // SE_PROPERTY(API(Attributes="EditorDisplay(\"Global Illumination\"), EditorOrder(150), JsonProperty(\"GI\")"))
        // GlobalIlluminationSettings GlobalIllumination;

        /// <summary>
        /// The bloom effect settings.
        /// </summary>
        // SE_PROPERTY(API(Attributes="EditorDisplay(\"Bloom\"), EditorOrder(200)"))
        // BloomSettings Bloom;

        /// <summary>
        /// The tone mapping effect settings.
        /// </summary>
        SE_FIELD(API(Attributes="EditorDisplay(\"Tone Mapping\"), EditorOrder(300)"))
        ToneMappingSettings ToneMapping;

        /// <summary>
        /// The color grading effect settings.
        /// </summary>
        SE_FIELD(API(Attributes="EditorDisplay(\"Color Grading\"), EditorOrder(400)"))
        ColorGradingSettings ColorGrading;

        /// Automatic exposure applied after scene HDR compositing.
        SE_FIELD(API(Attributes="EditorDisplay(\"Auto Exposure\"), EditorOrder(500)"))
        AutoExposureSettings AutoExposure;

        /// <summary>
        /// The camera artifacts effect settings.
        /// </summary>
        // SE_PROPERTY(API(Attributes="EditorDisplay(\"Camera Artifacts\"), EditorOrder(600)"))
        // CameraArtifactsSettings CameraArtifacts;

        /// <summary>
        /// The lens flares effect settings.
        /// </summary>
        // SE_PROPERTY(API(Attributes="EditorDisplay(\"Lens Flares\"), EditorOrder(700)"))
        // LensFlaresSettings LensFlares;

        /// <summary>
        /// The depth of field effect settings.
        /// </summary>
        // SE_PROPERTY(API(Attributes="EditorDisplay(\"Depth Of Field\"), EditorOrder(800)"))
        // DepthOfFieldSettings DepthOfField;

        /// <summary>
        /// The motion blur effect settings.
        /// </summary>
        // SE_PROPERTY(API(Attributes="EditorDisplay(\"Motion Blur\"), EditorOrder(900)"))
        // MotionBlurSettings MotionBlur;

        /// <summary>
        /// The screen space reflections effect settings.
        /// </summary>
        // SE_PROPERTY(API(Attributes="EditorDisplay(\"Screen Space Reflections\"), EditorOrder(1000), JsonProperty(\"SSR\")"))
        // ScreenSpaceReflectionsSettings ScreenSpaceReflections;

        /// <summary>
        /// The antialiasing effect settings.
        /// </summary>
        // SE_PROPERTY(API(Attributes="EditorDisplay(\"Anti Aliasing\"), EditorOrder(1100), JsonProperty(\"AA\")"))
        // AntiAliasingSettings AntiAliasing;

        /// <summary>
        /// The PostFx materials rendering settings.
        /// </summary>
        // SE_PROPERTY(API(Attributes="EditorDisplay(\"PostFx Materials\"), NoAnimate, EditorOrder(1200)"))
        // PostFxMaterialsSettings PostFxMaterials;

    public:
        /// <summary>
        /// Blends the settings using given weight.
        /// </summary>
        /// <param name="other">The other settings.</param>
        /// <param name="weight">The blend weight.</param>
        void BlendWith(PostProcessSettings& other, float weight);

        /// <summary>
        /// Returns true if object has loaded content (all postFx materials and textures are loaded).
        /// </summary>
        /// <returns>True if has content loaded.</returns>
        bool HasContentLoaded() const;

    public:
        // [ISerializable]
        void Serialize(SerializeContext& context) override;
        void Deserialize(DeserializeContext& context) override;
    };
}
