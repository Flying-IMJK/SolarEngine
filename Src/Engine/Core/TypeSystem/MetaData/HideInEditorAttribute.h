#pragma once

#include "Core/TypeSystem/MetaData/TypeMetaAttribute.h"

namespace SE
{
	/// <summary>
	/// Marks a property or field to be hidden in the editor.
	/// </summary>
	SE_META()
	class SE_API_CORE HideInEditorAttribute : public TypeMetaAttribute
	{
	public:
		HideInEditorAttribute() = default;
		
		bool Parse(const Json::Array& value) override 
		{ 
			return true; 
		}
	};
} // namespace SE
