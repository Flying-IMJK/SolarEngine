
#include "DirectionalLight.h"

#include "Runtime/Core/Serialization/Serialization.h"
#include "Runtime/Render/RenderList.h"

namespace SE
{
    DirectionalLight::DirectionalLight()
    {
        m_DrawNoCulling = 1;
        Brightness = 8.0f;
    }

    void DirectionalLight::RenderDraw(RenderContext& renderContext)
    {
        float brightness = Brightness;
        AdjustBrightness(renderContext.view, brightness);
        const Float3 position = GetPosition() - renderContext.view.Origin;
        if (Brightness > Math::ZeroTolerance
            /*&& EnumHasAnyFlags(renderContext.view.flags, ViewFlags::DirectionalLights)*/
            && renderContext.view.Pass.IsFlag(DrawPass::GBuffer)
            && (ViewDistance < Math::ZeroTolerance || Float3::DistanceSquared(renderContext.view.Position, position) < ViewDistance * ViewDistance))
        {
            RendererDirectionalLightData data;
            data.Position = position;
            // data.MinRoughness = MinRoughness;
            data.ShadowsDistance = ShadowsDistance;
            data.Color = Color.ToFloat3() * (Color.a * brightness);
            data.ShadowsStrength = ShadowsStrength;
            data.Direction = GetForward();
            data.ShadowsFadeDistance = ShadowsFadeDistance;
            data.ShadowsNormalOffsetScale = ShadowsNormalOffsetScale;
            data.ShadowsDepthBias = ShadowsDepthBias;
            data.ShadowsSharpness = ShadowsSharpness;
            data.VolumetricScatteringIntensity = VolumetricScatteringIntensity;
            data.IndirectLightingIntensity = IndirectLightingIntensity;
            data.CastVolumetricShadow = CastVolumetricShadow;
            data.RenderedVolumetricFog = 0;
            data.ShadowsMode = ShadowsMode;
            data.CascadeCount = CascadeCount;
            data.Cascade1Spacing = Cascade1Spacing;
            data.Cascade2Spacing = Cascade2Spacing;
            data.Cascade3Spacing = Cascade3Spacing;
            data.Cascade4Spacing = Cascade4Spacing;

            data.PartitionMode = PartitionMode;
            // data.ContactShadowsLength = ContactShadowsLength;
            data.StaticFlags = GetStaticFlags();
            data.ID = GetInstanceID();
            renderContext.list->DirectionalLights.Add(data);
        }
    }

    void DirectionalLight::Serialize(SerializeContext& context)
    {
        // Base
        Light::Serialize(context);

        SERIALIZE_GET_OTHER_OBJ(DirectionalLight, context.otherObj);

        SERIALIZE(CascadeCount);
        SERIALIZE(Cascade1Spacing);
        SERIALIZE(Cascade2Spacing);
        SERIALIZE(Cascade3Spacing);
        SERIALIZE(Cascade4Spacing);

        SERIALIZE(PartitionMode);
    }

    void DirectionalLight::Deserialize(DeserializeContext& context)
    {
        // Base
        Light::Deserialize(context);

        DESERIALIZE(CascadeCount);
        DESERIALIZE(Cascade1Spacing);
        DESERIALIZE(Cascade2Spacing);
        DESERIALIZE(Cascade3Spacing);
        DESERIALIZE(Cascade4Spacing);

        DESERIALIZE(PartitionMode);
    }

    bool DirectionalLight::IntersectsItself(const Ray& ray, float& distance, Float3& normal)
    {
        return false;
    }

    void DirectionalLight::OnTransformChanged()
    {
        // Base
        Light::OnTransformChanged();

        m_Box = BoundingBox(m_Transform.Translation);
        m_Sphere = BoundingSphere(m_Transform.Translation, 0.0f);
    }
} // SE