#include "HeaderAttribute.h"

namespace SE
{
	bool HeaderAttribute::Parse(const Json::Array& value)
	{
		int size = value.Size();
		if (size > 0 && value[0].IsString())
		{
			Text = value[0].GetText();
		}
		if (size > 1 && value[1].IsNumber())
		{
			FontSize = value[1].GetFloat();
		}
		if (size > 2 && value[2].IsNumber())
		{
			Color = value[2].GetUint();
		}
		return !Text.IsEmpty();
	}
} // namespace SE
