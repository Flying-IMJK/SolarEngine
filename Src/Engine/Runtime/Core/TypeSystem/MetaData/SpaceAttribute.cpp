#include "SpaceAttribute.h"

namespace SE
{
	bool SpaceAttribute::Parse(const Json::Array& value)
	{
		if (value.Size() > 0 && value[0].IsNumber())
		{
			Height = value[0].GetFloat();
			return true;
		}
		return false;
	}
} // namespace SE
