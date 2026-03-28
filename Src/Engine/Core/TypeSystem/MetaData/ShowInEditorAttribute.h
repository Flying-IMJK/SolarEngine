#pragma once

#include "Core/TypeSystem/MetaData/TypeMetaAttribute.h"

namespace SE
{
	/// <summary>
	/// Marks a property or field to be shown in the editor.
	/// </summary>
	SE_META()
	class SE_API_CORE ShowInEditorAttribute : public TypeMetaAttribute
	{
	public:
		ShowInEditorAttribute() = default;
		
		bool Parse(const Json::Array& value) override 
		{ 
			return true; 
		}
	};
} // namespace SE
