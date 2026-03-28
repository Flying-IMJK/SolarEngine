#pragma once
#include "Light.h"

namespace SE
{
    /// <summary>
    /// Directional light emits light from direction in space.
    /// </summary>
    // API_CLASS(Attributes="ActorContextMenu(\"New/Lights/Directional Light\"), ActorToolbox(\"Lights\")")
    class SE_API_RUNTIME DirectionalLight : public LightWithShadow
    {
        SE_CLASS(DirectionalLight, LightWithShadow);
    public:
        /// <summary>
        /// The partitioning mode for the shadow cascades.
        /// </summary>
        // API_FIELD(Attributes = "EditorOrder(64), DefaultValue(PartitionMode.Manual), EditorDisplay(\"Shadow\")")
        PartitionMode PartitionMode = PartitionMode::Manual;

        /// <summary>
        /// The number of cascades used for slicing the range of depth covered by the light during shadow rendering. Values are 1, 2 or 4 cascades; a typical scene uses 4 cascades.
        /// </summary>
        // API_FIELD(Attributes="EditorOrder(65), DefaultValue(4), Limit(1, 4), EditorDisplay(\"Shadow\")")
        int32 CascadeCount = 4;

        /// <summary>
        /// Percentage of the shadow distance used by the first cascade.
        /// </summary>
        // API_FIELD(Attributes = "EditorOrder(66), DefaultValue(0.05f), VisibleIf(nameof(ShowCascade1)), Limit(0, 1, 0.001f), EditorDisplay(\"Shadow\")")
        float Cascade1Spacing = 0.05f;

        /// <summary>
        /// Percentage of the shadow distance used by the second cascade.
        /// </summary>
        // API_FIELD(Attributes = "EditorOrder(67), DefaultValue(0.15f), VisibleIf(nameof(ShowCascade2)), Limit(0, 1, 0.001f), EditorDisplay(\"Shadow\")")
        float Cascade2Spacing = 0.15f;

        /// <summary>
        /// Percentage of the shadow distance used by the third cascade.
        /// </summary>
        // API_FIELD(Attributes = "EditorOrder(68), DefaultValue(0.50f), VisibleIf(nameof(ShowCascade3)), Limit(0, 1, 0.001f), EditorDisplay(\"Shadow\")")
        float Cascade3Spacing = 0.50f;

        /// <summary>
        /// Percentage of the shadow distance used by the fourth cascade.
        /// </summary>
        // API_FIELD(Attributes = "EditorOrder(69), DefaultValue(1.0f), VisibleIf(nameof(ShowCascade4)), Limit(0, 1, 0.001f), EditorDisplay(\"Shadow\")")
        float Cascade4Spacing = 1.0f;

    public:
        DirectionalLight();

        // [Light]
        void RenderDraw(RenderContext& renderContext) override;
        void Serialize(SerializeContext& context) override;
        void Deserialize(DeserializeContext& context) override;

        bool IntersectsItself(const Ray& ray, float& distance, Float3& normal) override;

    protected:
        // [Light]
        void OnTransformChanged() override;
    };
} // SE
