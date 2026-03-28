#include "CustomEditorAttribute.h"

namespace SE
{
	bool CustomEditorAttribute::Parse(const Json::Array& value)
	{
		if (value.Size() > 0 && value[0].IsString())
		{
			String typeName = value[0].GetText();
			EditorType = TypeID(typeName);;
			return EditorType != TypeID::Invalid;
		}
		return false;
	}
} // namespace SE
