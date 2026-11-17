#include <Runtime/Level/Actors/ExponentialHeightFog.h>

#include <Runtime/Core/Serialization/Serialization.h>
#include <Runtime/Level/Scene/Scene.h>
#include <Runtime/Render/RenderList.h>

namespace SE
{
    ExponentialHeightFog::ExponentialHeightFog(const SpawnParams& params) : RenderActor(params)
    {
        m_DrawNoCulling = 1;
        m_DrawCategory  = SceneRendering::PreRender;
    }

    void ExponentialHeightFog::RenderDraw(RenderContext& renderContext) { renderContext.list->AtmosphericFog = this; }

    void ExponentialHeightFog::ApplyAtmosphericFog(ExponentialHeightFogData& data)
    {
        data           = FogData;
        data.FogHeight = GetPosition().Y;
    }

    void ExponentialHeightFog::Serialize(SerializeContext& context)
    {
        Actor::Serialize(context);
        SERIALIZE_GET_OTHER_OBJ(ExponentialHeightFog, context.otherObj);
        context.stream.JKEY("FogData");
        context.stream.StartObject();
        SERIALIZE_MEMBER(FogDensity, FogData.FogDensity);
        SERIALIZE_MEMBER(FogHeightFalloff, FogData.FogHeightFalloff);
        SERIALIZE_MEMBER(SecondFogDensity, FogData.SecondFogDensity);
        SERIALIZE_MEMBER(SecondFogHeightFalloff, FogData.SecondFogHeightFalloff);
        SERIALIZE_MEMBER(SecondFogHeightOffset, FogData.SecondFogHeightOffset);
        SERIALIZE_MEMBER(FogInscatteringColor, FogData.FogInscatteringColor);
        SERIALIZE_MEMBER(FogMaxOpacity, FogData.FogMaxOpacity);
        SERIALIZE_MEMBER(StartDistance, FogData.StartDistance);
        SERIALIZE_MEMBER(FogCutoffDistance, FogData.FogCutoffDistance);
        SERIALIZE_MEMBER(DirectionalInscatteringColor, FogData.DirectionalInscatteringColor);
        SERIALIZE_MEMBER(DirectionalInscatteringExponent, FogData.DirectionalInscatteringExponent);
        SERIALIZE_MEMBER(DirectionalInscatteringStartDistance, FogData.DirectionalInscatteringStartDistance);
        SERIALIZE_MEMBER(SkyAtmosphereAmbientContributionColorScale, FogData.SkyAtmosphereAmbientContributionColorScale);
        context.stream.EndObject();
    }

    void ExponentialHeightFog::Deserialize(DeserializeContext& context)
    {
        Actor::Deserialize(context);
        const auto fog = SERIALIZE_FIND_MEMBER(context.stream, "FogData");
        if (fog != context.stream->MemberEnd() && fog->value.IsObject())
        {
            DeserializeContext::StreamScope scope(context, fog->value);
            DESERIALIZE_MEMBER(FogDensity, FogData.FogDensity);
            DESERIALIZE_MEMBER(FogHeightFalloff, FogData.FogHeightFalloff);
            DESERIALIZE_MEMBER(SecondFogDensity, FogData.SecondFogDensity);
            DESERIALIZE_MEMBER(SecondFogHeightFalloff, FogData.SecondFogHeightFalloff);
            DESERIALIZE_MEMBER(SecondFogHeightOffset, FogData.SecondFogHeightOffset);
            DESERIALIZE_MEMBER(FogInscatteringColor, FogData.FogInscatteringColor);
            DESERIALIZE_MEMBER(FogMaxOpacity, FogData.FogMaxOpacity);
            DESERIALIZE_MEMBER(StartDistance, FogData.StartDistance);
            DESERIALIZE_MEMBER(FogCutoffDistance, FogData.FogCutoffDistance);
            DESERIALIZE_MEMBER(DirectionalInscatteringColor, FogData.DirectionalInscatteringColor);
            DESERIALIZE_MEMBER(DirectionalInscatteringExponent, FogData.DirectionalInscatteringExponent);
            DESERIALIZE_MEMBER(DirectionalInscatteringStartDistance, FogData.DirectionalInscatteringStartDistance);
            DESERIALIZE_MEMBER(SkyAtmosphereAmbientContributionColorScale, FogData.SkyAtmosphereAmbientContributionColorScale);
        }
    }

    void ExponentialHeightFog::OnEnable()
    {
        GetScene()->Rendering.AddRender(this, m_SceneRenderingKey);
#if USE_EDITOR
        GetSceneRendering()->AddViewportIcon(this);
#endif
        Actor::OnEnable();
    }

    void ExponentialHeightFog::OnDisable()
    {
#if USE_EDITOR
        GetSceneRendering()->RemoveViewportIcon(this);
#endif
        GetScene()->Rendering.RemoveRender(this, m_SceneRenderingKey);
        Actor::OnDisable();
    }

    void ExponentialHeightFog::OnTransformChanged()
    {
        Actor::OnTransformChanged();
        m_Box    = BoundingBox(m_Transform.Translation);
        m_Sphere = BoundingSphere(m_Transform.Translation, 0.0f);
    }
} // namespace SE
