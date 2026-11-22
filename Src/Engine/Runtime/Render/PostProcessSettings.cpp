#include <Runtime/Render/PostProcessSettings.h>

#include <Runtime/Core/Serialization/Serialization.h>

namespace SE
{
    void ToneMappingSettings::Serialize(SerializeContext& context) {}
    void ToneMappingSettings::Deserialize(DeserializeContext& context)
	{

	}

	void ColorGradingSettings::BlendWith(ColorGradingSettings& other, float weight)
	{
	}

	void ColorGradingSettings::Serialize(SerializeContext& context)
	{

	}

	void ColorGradingSettings::Deserialize(DeserializeContext& context)
	{

	}

	void PostProcessSettings::BlendWith(PostProcessSettings& other, float weight)
	{
        weight                            = Math::Saturate(weight);
        AutoExposure.ExposureCompensation = Math::Lerp(AutoExposure.ExposureCompensation, other.AutoExposure.ExposureCompensation, weight);
        AutoExposure.MinEV100             = Math::Lerp(AutoExposure.MinEV100, other.AutoExposure.MinEV100, weight);
        AutoExposure.MaxEV100             = Math::Lerp(AutoExposure.MaxEV100, other.AutoExposure.MaxEV100, weight);
        AutoExposure.LowPercentile        = Math::Lerp(AutoExposure.LowPercentile, other.AutoExposure.LowPercentile, weight);
        AutoExposure.HighPercentile       = Math::Lerp(AutoExposure.HighPercentile, other.AutoExposure.HighPercentile, weight);
        AutoExposure.SpeedUp              = Math::Lerp(AutoExposure.SpeedUp, other.AutoExposure.SpeedUp, weight);
        AutoExposure.SpeedDown            = Math::Lerp(AutoExposure.SpeedDown, other.AutoExposure.SpeedDown, weight);
    }

    bool PostProcessSettings::HasContentLoaded() const
	{
		return true;
	}

	void PostProcessSettings::Serialize(SerializeContext& context)
	{
        SERIALIZE_GET_OTHER_OBJ(PostProcessSettings, context.otherObj);
        context.stream.JKEY("AutoExposure");
        context.stream.StartObject();
        SERIALIZE_MEMBER(ExposureCompensation, AutoExposure.ExposureCompensation);
        SERIALIZE_MEMBER(MinEV100, AutoExposure.MinEV100);
        SERIALIZE_MEMBER(MaxEV100, AutoExposure.MaxEV100);
        SERIALIZE_MEMBER(LowPercentile, AutoExposure.LowPercentile);
        SERIALIZE_MEMBER(HighPercentile, AutoExposure.HighPercentile);
        SERIALIZE_MEMBER(SpeedUp, AutoExposure.SpeedUp);
        SERIALIZE_MEMBER(SpeedDown, AutoExposure.SpeedDown);
        context.stream.EndObject();
    }

    void PostProcessSettings::Deserialize(DeserializeContext& context)
	{
        const auto exposure = SERIALIZE_FIND_MEMBER(context.stream, "AutoExposure");
        if (exposure != context.stream->MemberEnd() && exposure->value.IsObject())
        {
            DeserializeContext::StreamScope scope(context, exposure->value);
            DESERIALIZE_MEMBER(ExposureCompensation, AutoExposure.ExposureCompensation);
            DESERIALIZE_MEMBER(MinEV100, AutoExposure.MinEV100);
            DESERIALIZE_MEMBER(MaxEV100, AutoExposure.MaxEV100);
            DESERIALIZE_MEMBER(LowPercentile, AutoExposure.LowPercentile);
            DESERIALIZE_MEMBER(HighPercentile, AutoExposure.HighPercentile);
            DESERIALIZE_MEMBER(SpeedUp, AutoExposure.SpeedUp);
            DESERIALIZE_MEMBER(SpeedDown, AutoExposure.SpeedDown);
        }
    }
}
