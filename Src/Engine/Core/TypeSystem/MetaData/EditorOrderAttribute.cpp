#include "EditorOrderAttribute.h"

namespace SE
{
	bool EditorOrderAttribute::Parse(const Json::Array& value)
	{
		if (value.Size() > 0 && value[0].IsInt())
		{
			Order = value[0].GetInt();
			return true;
		}
		return false;
	}
} // namespace SE
