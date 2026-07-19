
#include "Light.h"

#include "Runtime/Core/Serialization/Serialization.h"
#include "../Scene/Scene.h"
#include "Runtime/Render/SceneRendering.h"

namespace SE
{
	Light::Light()
	{
		m_DrawCategory = SceneRendering::PreRender;
	}

	void Light::AdjustBrightness(const RenderView& view, float& brightness) const
	{
		// Indirect light scale during baking
#if SE_EDITOR
		brightness *= /*IsRunningRadiancePass && view.IsOfflinePass ? IndirectLightingIntensity * GetScene()->Info.LightmapSettings.IndirectLightingIntensity : */1.0f;
#endif
	}


	void Light::OnEnable()
	{
		GetScene()->Rendering.AddRender(this, _sceneRenderingKey);
#if SE_EDITOR
		/*GetScene()->Rendering.AddViewportIcon(this);
		GetScene()->Rendering.AddLightsDebug<Light, &Light::DrawLightsDebug>(this);*/
#endif

		// Base
		RenderActor::OnEnable();
	}

	void Light::OnDisable()
	{
#if SE_EDITOR
		/*GetScene()->Rendering.RemoveViewportIcon(this);
		GetScene()->Rendering.RemoveLightsDebug<Light, &Light::DrawLightsDebug>(this);*/
#endif
		GetScene()->Rendering.RemoveRender(this, _sceneRenderingKey);

		// Base
		RenderActor::OnDisable();
	}

#if SE_EDITOR
	void Light::DrawLightsDebug(RenderView& view)
	{
	}
#endif

	void Light::Serialize(SerializeContext& context)
	{
		// Base
		RenderActor::Serialize(context);

		SERIALIZE_GET_OTHER_OBJ(Light, context.otherObj);

		SERIALIZE(Color);
		SERIALIZE(Brightness);
		SERIALIZE(ViewDistance);
		SERIALIZE(IndirectLightingIntensity);
		SERIALIZE(VolumetricScatteringIntensity);
		SERIALIZE(CastVolumetricShadow);
	}

	void Light::Deserialize(DeserializeContext& context)
	{
		// Base
		RenderActor::Deserialize(context);

		DESERIALIZE(Color);
		DESERIALIZE(Brightness);
		DESERIALIZE(ViewDistance);
		DESERIALIZE(IndirectLightingIntensity);
		DESERIALIZE(VolumetricScatteringIntensity);
		DESERIALIZE(CastVolumetricShadow);
	}

	void LightWithShadow::Serialize(SerializeContext& context)
	{
		// Base
		Light::Serialize(context);

		SERIALIZE_GET_OTHER_OBJ(LightWithShadow, context.otherObj);

		SERIALIZE(MinRoughness);
		SERIALIZE(ShadowsDistance);
		SERIALIZE(ShadowsFadeDistance);
		SERIALIZE(ShadowsSharpness);
		SERIALIZE(ShadowsMode);
		SERIALIZE(ShadowsStrength);
		SERIALIZE(ShadowsDepthBias);
		SERIALIZE(ShadowsNormalOffsetScale);
		SERIALIZE(ContactShadowsLength);
	}

	void LightWithShadow::Deserialize(DeserializeContext& context)
	{
		// Base
		Light::Deserialize(context);

		DESERIALIZE(MinRoughness);
		DESERIALIZE(ShadowsDistance);
		DESERIALIZE(ShadowsFadeDistance);
		DESERIALIZE(ShadowsSharpness);
		DESERIALIZE(ShadowsMode);
		DESERIALIZE(ShadowsStrength);
		DESERIALIZE(ShadowsDepthBias);
		DESERIALIZE(ShadowsNormalOffsetScale);
		DESERIALIZE(ContactShadowsLength);
	}

	LightWithShadow::LightWithShadow()
	{
	}
} // SE