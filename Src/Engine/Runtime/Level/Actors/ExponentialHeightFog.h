#pragma once

#include <Runtime/Level/Actor.h>
#include <Runtime/Render/RenderDrawCall.h>

namespace SE
{
    SE_CLASS(Reflect, API())
    class SE_API_RUNTIME ExponentialHeightFog : public RenderActor, public IAtmosphericFogRenderer
    {
        SCRIPTING_TYPE(ExponentialHeightFog);
        SE_DEFINE_CLASS(ExponentialHeightFog, RenderActor);

    public:
        SE_FIELD(API(Attributes = "EditorDisplay(\"Fog\", EditorDisplayAttribute.InlineStyle)"))
        ExponentialHeightFogData FogData;

        void RenderDraw(RenderContext& renderContext) override;
        void ApplyAtmosphericFog(ExponentialHeightFogData& data) override;
        void Serialize(SerializeContext& context) override;
        void Deserialize(DeserializeContext& context) override;

    protected:
        void OnEnable() override;
        void OnDisable() override;
        void OnTransformChanged() override;

    private:
        int32 m_SceneRenderingKey = -1;
    };
} // namespace SE
