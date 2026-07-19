
#include "PointLight.h"

#include "Runtime/Core/Serialization/Serialization.h"
#include "../Scene/Scene.h"
#include "Runtime/Render/RenderList.h"

namespace SE
{
    PointLight::PointLight(): LightWithShadow(), _radius(1000.0f)
    {
        CastVolumetricShadow = false;
        ShadowsDistance = 2000.0f;
        ShadowsFadeDistance = 100.0f;
        ShadowsDepthBias = 0.5f;
        _direction = Float3::Forward;
        m_Sphere = BoundingSphere(Float3::Zero, _radius);
        BoundingBox::FromSphere(m_Sphere, m_Box);
    }

    float PointLight::ComputeBrightness() const
    {
        float result = Brightness;

        /*if (IESTexture)
        {
            if (UseIESBrightness)
            {
                result = IESTexture->Brightness * IESBrightnessScale;
            }

            result *= IESTexture->TextureMultiplier;
        }*/

        //if (UseInverseSquaredFalloff)
        //	result *= 16.0f;

        return result;
    }

    float PointLight::GetScaledRadius() const
    {
        return _radius * m_Transform.Scale.MaxValue();
    }

    void PointLight::SetRadius(float value)
    {
        value = Math::Max(0.0f, value);
        if (Math::IsNearEqual(value, _radius))
        {
            return;
        }

        _radius = value;
        UpdateBounds();
    }

    void PointLight::UpdateBounds()
    {
        // Cache light direction
        Float3::Transform(Float3::Forward, m_Transform.Orientation, _direction);
        _direction.Normalize();

        // Cache bounding box
        m_Sphere = BoundingSphere(GetPosition(), GetScaledRadius());
        BoundingBox::FromSphere(m_Sphere, m_Box);

        if (_sceneRenderingKey != -1)
        {
            GetScene()->Rendering.UpdateRender(this, _sceneRenderingKey);
        }
    }

    void PointLight::OnTransformChanged()
    {
        // Base
        LightWithShadow::OnTransformChanged();

        UpdateBounds();
    }

    void PointLight::RenderDraw(RenderContext& renderContext)
    {
        float brightness = ComputeBrightness();
        AdjustBrightness(renderContext.view, brightness);
        const Float3 position = GetPosition() - renderContext.view.Origin;
        const float radius = GetScaledRadius();
        if (/*EnumHasAnyFlags(renderContext.view.flags, ViewFlags::PointLights) &&*/
            renderContext.view.Pass.IsFlag(DrawPass::GBuffer) &&
            brightness > Math::ZeroTolerance &&
            radius > Math::ZeroTolerance &&
            (ViewDistance < Math::ZeroTolerance || Float3::DistanceSquared(renderContext.view.Position, position) < ViewDistance * ViewDistance))
        {
            RendererPointLightData data;
            data.Position = position;
            data.MinRoughness = MinRoughness;
            data.ShadowsDistance = ShadowsDistance;
            data.Color = Color.ToFloat3() * (Color.a * brightness);
            data.ShadowsStrength = ShadowsStrength;
            data.Direction = _direction;
            data.ShadowsFadeDistance = ShadowsFadeDistance;
            data.ShadowsNormalOffsetScale = ShadowsNormalOffsetScale;
            data.ShadowsDepthBias = ShadowsDepthBias;
            data.ShadowsSharpness = ShadowsSharpness;
            data.VolumetricScatteringIntensity = VolumetricScatteringIntensity;
            data.CastVolumetricShadow = CastVolumetricShadow;
            data.RenderedVolumetricFog = 0;
            data.ShadowsMode = ShadowsMode;
            data.Radius = radius;
            data.FallOffExponent = FallOffExponent;
            data.UseInverseSquaredFalloff = UseInverseSquaredFalloff;
            data.SourceRadius = SourceRadius;
            data.SourceLength = SourceLength;
            data.ContactShadowsLength = ContactShadowsLength;
            data.IndirectLightingIntensity = IndirectLightingIntensity;
            data.IESTexture = nullptr;// IESTexture ? IESTexture->GetTexture() : nullptr;
            data.StaticFlags = GetStaticFlags();
            data.ID = GetInstanceID();
            renderContext.list->PointLights.Add(data);
        }
    }

#if SE_EDITOR

/*
#include "Engine/Debug/DebugDraw.h"

    void PointLight::OnDebugDraw()
    {
        if (SourceRadius > Math::ZeroTolerance || SourceLength > Math::ZeroTolerance)
        {
            // Draw source tube
            DEBUG_DRAW_WIRE_TUBE(GetPosition(), GetOrientation(), SourceRadius, SourceLength, Color::Orange, 0, true);
        }

        // Base
        LightWithShadow::OnDebugDraw();
    }

    void PointLight::OnDebugDrawSelected()
    {
        // Draw influence range
        DEBUG_DRAW_WIRE_SPHERE(_sphere, Color::Yellow, 0, true);

        // Base
        LightWithShadow::OnDebugDrawSelected();
    }

    void PointLight::DrawLightsDebug(RenderView& view)
    {
        const BoundingSphere sphere(_sphere.Center - view.Origin, _sphere.Radius);
        if (!view.CullingFrustum.Intersects(sphere) || !EnumHasAnyFlags(view.Flags, ViewFlags::PointLights))
            return;

        // Draw influence range
        DEBUG_DRAW_WIRE_SPHERE(_sphere, Color::Yellow, 0, true);
    }
    */

#endif

    void PointLight::OnLayerChanged()
    {
        if (_sceneRenderingKey != -1)
            GetScene()->Rendering.UpdateRender(this, _sceneRenderingKey);
    }

    void PointLight::Serialize(SerializeContext& context)
    {
        // Base
        LightWithShadow::Serialize(context);

        SERIALIZE_GET_OTHER_OBJ(PointLight, context.otherObj);

        SERIALIZE_MEMBER(Radius, _radius);
        // SERIALIZE(IESTexture);
        SERIALIZE(SourceRadius);
        SERIALIZE(SourceLength);
        SERIALIZE(FallOffExponent);
        SERIALIZE(UseInverseSquaredFalloff);
        // SERIALIZE(UseIESBrightness);
        // SERIALIZE(IESBrightnessScale);
    }

    void PointLight::Deserialize(DeserializeContext& context)
    {
        // Base
        LightWithShadow::Deserialize(context);

        DESERIALIZE_MEMBER(Radius, _radius);
        // DESERIALIZE(IESTexture);
        DESERIALIZE(SourceRadius);
        DESERIALIZE(SourceLength);
        DESERIALIZE(FallOffExponent);
        DESERIALIZE(UseInverseSquaredFalloff);
        // DESERIALIZE(UseIESBrightness);
        // DESERIALIZE(IESBrightnessScale);
    }

    bool PointLight::IntersectsItself(const Ray& ray, float& distance, Float3& normal)
    {
        return CollisionsHelper::RayIntersectsSphere(ray, m_Sphere, distance, normal);
    }
} // SE