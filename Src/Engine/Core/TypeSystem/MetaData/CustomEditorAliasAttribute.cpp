#include "CustomEditorAliasAttribute.h"

namespace SE
{
	bool CustomEditorAliasAttribute::Parse(const Json::Array& value)
	{
		if (value.Size() > 0 && value[0].IsString())
		{
			typeID = TypeID(value[0].GetText());
			return true;
		}
		return false;
	}
} // namespace SE
