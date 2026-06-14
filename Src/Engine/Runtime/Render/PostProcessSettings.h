#pragma once


#include "Core/Math/Vector4.h"
#include "Core/Serialization/ISerializable.h"
#include "Core/TypeSystem/IType.h"

#include "Runtime/API.h"
#include "Runtime/Render/Assets/Texture/Texture.h"
#include "Runtime/Resource/AssetRef.h"

namespace SE
{
    /// </summary>
    SE_ENUM(Reflect)
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
    struct SE_API_RUNTIME ToneMappingSettings : IType, ISerializable
    {
        SE_DEFINE_CLASS_DEFAULT(ToneMappingSettings, IType);
        // typedef ToneMappingSettingsOverride Override;

        /// <summary>
        /// The flags for overriden properties.
        /// </summary>
        // SE_PROPERTY(API, Attributes="HideInEditor")
        // ToneMappingSettingsOverride OverrideFlags = Override::None;

        /// <summary>
        /// Adjusts the white balance in relation to the temperature of the light in the scene. When the light temperature and this one match the light will appear white. When a value is used that is higher than the light in the scene it will yield a "warm" or yellow color, and, conversely, if the value is lower, it would yield a "cool" or blue color.
        /// </summary>
        // SE_PROPERTY(API, Attributes="Limit(1500, 15000), EditorOrder(0), PostProcessSetting((int)ToneMappingSettingsOverride.WhiteTemperature)")
        float WhiteTemperature = 6500.0f;

        /// <summary>
        /// Adjusts the white balance temperature tint for the scene by adjusting the cyan and magenta color ranges. Ideally, this setting should be used once you've adjusted the white balance temperature to get accurate colors. Under some light temperatures, the colors may appear to be more yellow or blue. This can be used to balance the resulting color to look more natural.
        /// </summary>
        // SE_PROPERTY(API, Attributes="Limit(-1, 1, 0.001f), EditorOrder(1), PostProcessSetting((int)ToneMappingSettingsOverride.WhiteTint)")
        float WhiteTint = 0.0f;

        /// <summary>
        /// The tone mapping mode to use for the color grading process.
        /// </summary>
        // SE_PROPERTY(API, Attributes="EditorOrder(2), PostProcessSetting((int)ToneMappingSettingsOverride.Mode)")
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
    struct SE_API_RUNTIME ColorGradingSettings : IType, ISerializable
    {
        // API_AUTO_SERIALIZATION();
        SE_DEFINE_CLASS_DEFAULT(ColorGradingSettings, IType);
        // typedef ColorGradingSettingsOverride Override;


        /// <summary>
        /// The flags for overriden properties.
        /// </summary>
        // SE_PROPERTY(API, Attributes="HideInEditor")
        // ColorGradingSettingsOverride OverrideFlags = Override::None;

        // Global

        /// <summary>
        /// Gets or sets the color saturation (applies globally to the whole image). Default is 1.
        /// </summary>
        // SE_PROPERTY(API, Attributes="DefaultValue(typeof(Float4), \"1,1,1,1\"), CustomEditorAlias(\"FlaxEditor.CustomEditors.Editors.ColorTrackball\"), EditorOrder(0), PostProcessSetting((int)ColorGradingSettingsOverride.ColorSaturation), Limit(0, 2, 0.01f), EditorDisplay(\"Global\", \"Saturation\")")
        Float4 ColorSaturation = Float4::One;

        /// <summary>
        /// Gets or sets the color contrast (applies globally to the whole image). Default is 1.
        /// </summary>
        // SE_PROPERTY(API, Attributes="DefaultValue(typeof(Float4), \"1,1,1,1\"), CustomEditorAlias(\"FlaxEditor.CustomEditors.Editors.ColorTrackball\"), EditorOrder(1), PostProcessSetting((int)ColorGradingSettingsOverride.ColorContrast), Limit(0, 2, 0.01f), EditorDisplay(\"Global\", \"Contrast\")")
        Float4 ColorContrast = Float4::One;

        /// <summary>
        /// Gets or sets the color gamma (applies globally to the whole image). Default is 1.
        /// </summary>
        // SE_PROPERTY(API, Attributes="DefaultValue(typeof(Float4), \"1,1,1,1\"), CustomEditorAlias(\"FlaxEditor.CustomEditors.Editors.ColorTrackball\"), EditorOrder(2), PostProcessSetting((int)ColorGradingSettingsOverride.ColorGamma), Limit(0, 2, 0.01f), EditorDisplay(\"Global\", \"Gamma\")")
        Float4 ColorGamma = Float4::One;

        /// <summary>
        /// Gets or sets the color gain (applies globally to the whole image). Default is 1.
        /// </summary>
        // SE_PROPERTY(API, Attributes="DefaultValue(typeof(Float4), \"1,1,1,1\"), CustomEditorAlias(\"FlaxEditor.CustomEditors.Editors.ColorTrackball\"), EditorOrder(3), PostProcessSetting((int)ColorGradingSettingsOverride.ColorGain), Limit(0, 2, 0.01f), EditorDisplay(\"Global\", \"Gain\")")
        Float4 ColorGain = Float4::One;

        /// <summary>
        /// Gets or sets the color offset (applies globally to the whole image). Default is 0.
        /// </summary>
        // SE_PROPERTY(API, Attributes="DefaultValue(typeof(Float4), \"0,0,0,0\"), CustomEditorAlias(\"FlaxEditor.CustomEditors.Editors.ColorTrackball\"), EditorOrder(4), PostProcessSetting((int)ColorGradingSettingsOverride.ColorOffset), Limit(-1, 1, 0.001f), EditorDisplay(\"Global\", \"Offset\")")
        Float4 ColorOffset = Float4::Zero;

        // Shadows

        /// <summary>
        /// Gets or sets the color saturation (applies to shadows only). Default is 1.
        /// </summary>
        // SE_PROPERTY(API, Attributes="DefaultValue(typeof(Float4), \"1,1,1,1\"), CustomEditorAlias(\"FlaxEditor.CustomEditors.Editors.ColorTrackball\"), EditorOrder(5), PostProcessSetting((int)ColorGradingSettingsOverride.ColorSaturationShadows), Limit(0, 2, 0.01f), EditorDisplay(\"Shadows\", \"Shadows Saturation\")")
        Float4 ColorSaturationShadows = Float4::One;

        /// <summary>
        /// Gets or sets the color contrast (applies to shadows only). Default is 1.
        /// </summary>
        // SE_PROPERTY(API, Attributes="DefaultValue(typeof(Float4), \"1,1,1,1\"), CustomEditorAlias(\"FlaxEditor.CustomEditors.Editors.ColorTrackball\"), EditorOrder(6), PostProcessSetting((int)ColorGradingSettingsOverride.ColorContrastShadows), Limit(0, 2, 0.01f), EditorDisplay(\"Shadows\", \"Shadows Contrast\")")
        Float4 ColorContrastShadows = Float4::One;

        /// <summary>
        /// Gets or sets the color gamma (applies to shadows only). Default is 1.
        /// </summary>
        // SE_PROPERTY(API, Attributes="DefaultValue(typeof(Float4), \"1,1,1,1\"), CustomEditorAlias(\"FlaxEditor.CustomEditors.Editors.ColorTrackball\"), EditorOrder(7), PostProcessSetting((int)ColorGradingSettingsOverride.ColorGammaShadows), Limit(0, 2, 0.01f), EditorDisplay(\"Shadows\", \"Shadows Gamma\")")
        Float4 ColorGammaShadows = Float4::One;

        /// <summary>
        /// Gets or sets the color gain (applies to shadows only). Default is 1.
        /// </summary>
        // SE_PROPERTY(API, Attributes="DefaultValue(typeof(Float4), \"1,1,1,1\"), CustomEditorAlias(\"FlaxEditor.CustomEditors.Editors.ColorTrackball\"), EditorOrder(8), PostProcessSetting((int)ColorGradingSettingsOverride.ColorGainShadows), Limit(0, 2, 0.01f), EditorDisplay(\"Shadows\", \"Shadows Gain\")")
        Float4 ColorGainShadows = Float4::One;

        /// <summary>
        /// Gets or sets the color offset (applies to shadows only). Default is 0.
        /// </summary>
        // SE_PROPERTY(API, Attributes="DefaultValue(typeof(Float4), \"0,0,0,0\"), CustomEditorAlias(\"FlaxEditor.CustomEditors.Editors.ColorTrackball\"), EditorOrder(9), PostProcessSetting((int)ColorGradingSettingsOverride.ColorOffsetShadows), Limit(-1, 1, 0.001f), EditorDisplay(\"Shadows\", \"Shadows Offset\")")
        Float4 ColorOffsetShadows = Float4::Zero;

        // Midtones

        /// <summary>
        /// Gets or sets the color saturation (applies to midtones only). Default is 1.
        /// </summary>
        // SE_PROPERTY(API, Attributes="DefaultValue(typeof(Float4), \"1,1,1,1\"), CustomEditorAlias(\"FlaxEditor.CustomEditors.Editors.ColorTrackball\"), EditorOrder(10), PostProcessSetting((int)ColorGradingSettingsOverride.ColorSaturationMidtones), Limit(0, 2, 0.01f), EditorDisplay(\"Midtones\", \"Midtones Saturation\")")
        Float4 ColorSaturationMidtones = Float4::One;

        /// <summary>
        /// Gets or sets the color contrast (applies to midtones only). Default is 1.
        /// </summary>
        // SE_PROPERTY(API, Attributes="DefaultValue(typeof(Float4), \"1,1,1,1\"), CustomEditorAlias(\"FlaxEditor.CustomEditors.Editors.ColorTrackball\"), EditorOrder(11), PostProcessSetting((int)ColorGradingSettingsOverride.ColorContrastMidtones), Limit(0, 2, 0.01f), EditorDisplay(\"Midtones\", \"Midtones Contrast\")")
        Float4 ColorContrastMidtones = Float4::One;

        /// <summary>
        /// Gets or sets the color gamma (applies to midtones only). Default is 1.
        /// </summary>
        // SE_PROPERTY(API, Attributes="DefaultValue(typeof(Float4), \"1,1,1,1\"), CustomEditorAlias(\"FlaxEditor.CustomEditors.Editors.ColorTrackball\"), EditorOrder(12), PostProcessSetting((int)ColorGradingSettingsOverride.ColorGammaMidtones), Limit(0, 2, 0.01f), EditorDisplay(\"Midtones\", \"Midtones Gamma\")")
        Float4 ColorGammaMidtones = Float4::One;

        /// <summary>
        /// Gets or sets the color gain (applies to midtones only). Default is 1.
        /// </summary>
        // SE_PROPERTY(API, Attributes="DefaultValue(typeof(Float4), \"1,1,1,1\"), CustomEditorAlias(\"FlaxEditor.CustomEditors.Editors.ColorTrackball\"), EditorOrder(13), PostProcessSetting((int)ColorGradingSettingsOverride.ColorGainMidtones), Limit(0, 2, 0.01f), EditorDisplay(\"Midtones\", \"Midtones Gain\")")
        Float4 ColorGainMidtones = Float4::One;

        /// <summary>
        /// Gets or sets the color offset (applies to midtones only). Default is 0.
        /// </summary>
        // SE_PROPERTY(API, Attributes="DefaultValue(typeof(Float4), \"0,0,0,0\"), CustomEditorAlias(\"FlaxEditor.CustomEditors.Editors.ColorTrackball\"), EditorOrder(14), PostProcessSetting((int)ColorGradingSettingsOverride.ColorOffsetMidtones), Limit(-1, 1, 0.001f), EditorDisplay(\"Midtones\", \"Midtones Offset\")")
        Float4 ColorOffsetMidtones = Float4::Zero;

        // Highlights

        /// <summary>
        /// Gets or sets the color saturation (applies to highlights only). Default is 1.
        /// </summary>
        // SE_PROPERTY(API, Attributes="DefaultValue(typeof(Float4), \"1,1,1,1\"), CustomEditorAlias(\"FlaxEditor.CustomEditors.Editors.ColorTrackball\"), EditorOrder(15), PostProcessSetting((int)ColorGradingSettingsOverride.ColorSaturationHighlights), Limit(0, 2, 0.01f), EditorDisplay(\"Highlights\", \"Highlights Saturation\")")
        Float4 ColorSaturationHighlights = Float4::One;

        /// <summary>
        /// Gets or sets the color contrast (applies to highlights only). Default is 1.
        /// </summary>
        // SE_PROPERTY(API, Attributes="DefaultValue(typeof(Float4), \"1,1,1,1\"), CustomEditorAlias(\"FlaxEditor.CustomEditors.Editors.ColorTrackball\"), EditorOrder(16), PostProcessSetting((int)ColorGradingSettingsOverride.ColorContrastHighlights), Limit(0, 2, 0.01f), EditorDisplay(\"Highlights\", \"Highlights Contrast\")")
        Float4 ColorContrastHighlights = Float4::One;

        /// <summary>
        /// Gets or sets the color gamma (applies to highlights only). Default is 1.
        /// </summary>
        // SE_PROPERTY(API, Attributes="DefaultValue(typeof(Float4), \"1,1,1,1\"), CustomEditorAlias(\"FlaxEditor.CustomEditors.Editors.ColorTrackball\"), EditorOrder(17), PostProcessSetting((int)ColorGradingSettingsOverride.ColorGammaHighlights), Limit(0, 2, 0.01f), EditorDisplay(\"Highlights\", \"Highlights Gamma\")")
        Float4 ColorGammaHighlights = Float4::One;

        /// <summary>
        /// Gets or sets the color gain (applies to highlights only). Default is 1.
        /// </summary>
        // SE_PROPERTY(API, Attributes="DefaultValue(typeof(Float4), \"1,1,1,1\"), CustomEditorAlias(\"FlaxEditor.CustomEditors.Editors.ColorTrackball\"), EditorOrder(18), PostProcessSetting((int)ColorGradingSettingsOverride.ColorGainHighlights), Limit(0, 2, 0.01f), EditorDisplay(\"Highlights\", \"Highlights Gain\")")
        Float4 ColorGainHighlights = Float4::One;

        /// <summary>
        /// Gets or sets the color offset (applies to highlights only). Default is 0.
        /// </summary>
        // SE_PROPERTY(API, Attributes="DefaultValue(typeof(Float4), \"0,0,0,0\"), CustomEditorAlias(\"FlaxEditor.CustomEditors.Editors.ColorTrackball\"), EditorOrder(19), PostProcessSetting((int)ColorGradingSettingsOverride.ColorOffsetHighlights), Limit(-1, 1, 0.001f), EditorDisplay(\"Highlights\", \"Highlights Offset\")")
        Float4 ColorOffsetHighlights = Float4::Zero;

        //

        /// <summary>
        /// The shadows maximum value. Default is 0.09.
        /// </summary>
        // SE_PROPERTY(API, Attributes="Limit(-1, 1, 0.01f), EditorOrder(20), PostProcessSetting((int)ColorGradingSettingsOverride.ShadowsMax)")
        float ShadowsMax = 0.09f;

        /// <summary>
        /// The highlights minimum value. Default is 0.5.
        /// </summary>
        // SE_PROPERTY(API, Attributes="Limit(-1, 1, 0.01f), EditorOrder(21), PostProcessSetting((int)ColorGradingSettingsOverride.HighlightsMin)")
        float HighlightsMin = 0.5f;

        //

        /// <summary>
        /// The Lookup Table (LUT) used to perform color correction.
        /// </summary>
        // SE_PROPERTY(API, Attributes="DefaultValue(null), EditorOrder(22), PostProcessSetting((int)ColorGradingSettingsOverride.LutTexture)")
        SoftAssetRef<Texture> LutTexture;

        /// <summary>
        /// The LUT blending weight (normalized to range 0-1). Default is 1.0.
        /// </summary>
        // SE_PROPERTY(API, Attributes="Limit(0, 1, 0.01f), EditorOrder(23), PostProcessSetting((int)ColorGradingSettingsOverride.LutWeight)")
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

    // <summary>
    /// Contains settings for rendering advanced visual effects and post effects.
    /// </summary>
    struct SE_API_RUNTIME PostProcessSettings : IType, ISerializable
    {
        SE_DEFINE_CLASS_DEFAULT(PostProcessSettings, IType);


        /// <summary>
        /// The ambient occlusion effect settings.
        /// </summary>
        // SE_PROPERTY(API, Attributes="EditorDisplay(\"Ambient Occlusion\"), EditorOrder(100), JsonProperty(\"AO\")")
        // AmbientOcclusionSettings AmbientOcclusion;

        /// <summary>
        /// The global illumination effect settings.
        /// </summary>
        // SE_PROPERTY(API, Attributes="EditorDisplay(\"Global Illumination\"), EditorOrder(150), JsonProperty(\"GI\")")
        // GlobalIlluminationSettings GlobalIllumination;

        /// <summary>
        /// The bloom effect settings.
        /// </summary>
        // SE_PROPERTY(API, Attributes="EditorDisplay(\"Bloom\"), EditorOrder(200)")
        // BloomSettings Bloom;

        /// <summary>
        /// The tone mapping effect settings.
        /// </summary>
        // SE_PROPERTY(API, Attributes="EditorDisplay(\"Tone Mapping\"), EditorOrder(300)")
        ToneMappingSettings ToneMapping;

        /// <summary>
        /// The color grading effect settings.
        /// </summary>
        // SE_PROPERTY(API, Attributes="EditorDisplay(\"Color Grading\"), EditorOrder(400)")
        ColorGradingSettings ColorGrading;

        /// <summary>
        /// The eye adaptation effect settings.
        /// </summary>
        // SE_PROPERTY(API, Attributes="EditorDisplay(\"Eye Adaptation\"), EditorOrder(500)")
        // EyeAdaptationSettings EyeAdaptation;

        /// <summary>
        /// The camera artifacts effect settings.
        /// </summary>
        // SE_PROPERTY(API, Attributes="EditorDisplay(\"Camera Artifacts\"), EditorOrder(600)")
        // CameraArtifactsSettings CameraArtifacts;

        /// <summary>
        /// The lens flares effect settings.
        /// </summary>
        // SE_PROPERTY(API, Attributes="EditorDisplay(\"Lens Flares\"), EditorOrder(700)")
        // LensFlaresSettings LensFlares;

        /// <summary>
        /// The depth of field effect settings.
        /// </summary>
        // SE_PROPERTY(API, Attributes="EditorDisplay(\"Depth Of Field\"), EditorOrder(800)")
        // DepthOfFieldSettings DepthOfField;

        /// <summary>
        /// The motion blur effect settings.
        /// </summary>
        // SE_PROPERTY(API, Attributes="EditorDisplay(\"Motion Blur\"), EditorOrder(900)")
        // MotionBlurSettings MotionBlur;

        /// <summary>
        /// The screen space reflections effect settings.
        /// </summary>
        // SE_PROPERTY(API, Attributes="EditorDisplay(\"Screen Space Reflections\"), EditorOrder(1000), JsonProperty(\"SSR\")")
        // ScreenSpaceReflectionsSettings ScreenSpaceReflections;

        /// <summary>
        /// The antialiasing effect settings.
        /// </summary>
        // SE_PROPERTY(API, Attributes="EditorDisplay(\"Anti Aliasing\"), EditorOrder(1100), JsonProperty(\"AA\")")
        // AntiAliasingSettings AntiAliasing;

        /// <summary>
        /// The PostFx materials rendering settings.
        /// </summary>
        // SE_PROPERTY(API, Attributes="EditorDisplay(\"PostFx Materials\"), NoAnimate, EditorOrder(1200)")
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
