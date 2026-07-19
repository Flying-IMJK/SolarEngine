#pragma once

#include "Runtime/Core/TypeSystem/MetaData/TypeMetaAttribute.h"

namespace SE
{
	/// <summary>
	/// Marks a property or field as read-only in the editor.
	/// </summary>
	SE_META()
	class SE_API_RUNTIME ReadOnlyAttribute : public TypeMetaAttribute
	{
	public:
		ReadOnlyAttribute() = default;
		
		bool Parse(const Json::Array& value) override 
		{ 
			return true; 
		}
	};
} // namespace SE
