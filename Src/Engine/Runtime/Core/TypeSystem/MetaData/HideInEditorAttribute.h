#pragma once

#include "Runtime/Core/TypeSystem/MetaData/TypeMetaAttribute.h"

namespace SE
{
	/// <summary>
	/// Marks a property or field to be hidden in the editor.
	/// </summary>
	SE_META()
	class SE_API_RUNTIME HideInEditorAttribute : public TypeMetaAttribute
	{
	public:
		HideInEditorAttribute() = default;
		
		bool Parse(const Json::Array& value) override 
		{ 
			return true; 
		}
	};
} // namespace SE
