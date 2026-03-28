#include "PostProcessSettings.h"

namespace SE
{
	void ToneMappingSettings::Serialize(SerializeContext& context)
	{

	}
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
	}

	bool PostProcessSettings::HasContentLoaded() const
	{
		return true;
	}

	void PostProcessSettings::Serialize(SerializeContext& context)
	{

	}

	void PostProcessSettings::Deserialize(DeserializeContext& context)
	{

	}

}
