#include "EditorDisplayAttribute.h"

namespace SE
{
	bool EditorDisplayAttribute::Parse(const Json::Array& value)
	{
		int size = value.Size();
		if (size > 0 && value[0].IsString())
		{
			Name = value[0].GetText();
		}
		if (size > 1 && value[1].IsString())
		{
			Group = value[1].GetText();
		}
		if (size > 2 && value[2].IsString())
		{
			Tooltip = value[2].GetText();
		}
		return true;
	}
} // namespace SE
