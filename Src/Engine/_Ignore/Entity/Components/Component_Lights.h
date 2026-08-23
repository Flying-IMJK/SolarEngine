#pragma once

#include "Runtime/API.h"
#include "Runtime/Entity/EntitySpatialComponent.h"
#include "Core/Math/Color.h"

//-------------------------------------------------------------------------

namespace SE
{
    class SE_API_RUNTIME LightComponent : public SpatialEntityComponent
    {
//        ENGINE_ENTITY_COMPONENT(LightComponent);
        friend class RenderDebugView;
        SE_CLASS(LightComponent, SpatialEntityComponent)
    public:

        inline float GetLightIntensity() const { return m_intensity; }
        inline Color32 const& GetLightColor() const { return m_color; }
        inline VectorSIMD const& GetLightPosition() const { return GetWorldTransform().GetTranslation(); }
        inline bool GetShadowed() const { return m_shadowed; }

    private:

        SE_FIELD()
		Color32                m_color = Colors::White;
        SE_FIELD()
		float                m_intensity = 1.0f;
        SE_FIELD()
		bool                 m_shadowed = false;
    };

    //-------------------------------------------------------------------------

    class SE_API_RUNTIME DirectionalLightComponent : public LightComponent
    {
		SE_CLASS(DirectionalLightComponent, LightComponent);
//        ENGINE_ENTITY_COMPONENT( DirectionalLightComponent );
        friend class RenderDebugView;

    public:

        inline VectorSIMD GetLightDirection() const { return GetWorldTransform().GetForwardVector(); }

    };

    //-------------------------------------------------------------------------

    class SE_API_RUNTIME PointLightComponent : public LightComponent
    {
//        ENGINE_ENTITY_COMPONENT( PointLightComponent );
        friend class RenderDebugView;
        SE_CLASS(PointLightComponent, LightComponent);
    public:

        inline float GetLightRadius() const { return m_radius; }

    private:

        SE_PROPERTY()
		float                m_radius = 1.0f;
    };

    //-------------------------------------------------------------------------

    class SE_API_RUNTIME SpotLightComponent : public LightComponent
    {
		SE_CLASS(SpotLightComponent, LightComponent)
//		ENGINE_ENTITY_COMPONENT( SpotLightComponent );
        friend class RenderDebugView;

    public:

        inline float GetLightRadius() const { return m_radius; }
        inline Degrees GetLightInnerUmbraAngle() const { return m_innerUmbraAngle; }
        inline Degrees GetLightOuterUmbraAngle() const { return m_outerUmbraAngle; }
        inline VectorSIMD GetLightDirection() const { return GetWorldTransform().GetForwardVector(); }

    private:

        SE_PROPERTY()
		Degrees              m_innerUmbraAngle = 0.0f;
		SE_PROPERTY()
		Degrees              m_outerUmbraAngle = 45.0f;
		SE_PROPERTY()
		float                m_radius = 1.0f;

        // TODO: Fall-off parameters
    };
}