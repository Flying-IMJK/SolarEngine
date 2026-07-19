#pragma once

#include "Runtime/Core/TypeSystem/MetaData/TypeMetaAttribute.h"

namespace SE
{
	/// <summary>
	/// Controls property visibility based on another property's value.
	/// </summary>
	SE_META()
	class SE_API_RUNTIME VisibleIfAttribute : public TypeMetaAttribute
	{
	public:
		String MemberName;  // Condition member name
		bool Invert = false; // Whether to invert the condition

		VisibleIfAttribute() = default;
		
		bool Parse(const Json::Array& value) override
		{
			/*if (value.size() > 0 && value[0].is_string())
			{
				MemberName = value[0].get<std::string>().c_str();
			}
			if (value.size() > 1 && value[1].is_boolean())
			{
				Invert = value[1].get<bool>();
			}*/
			return true;
		}
	};
} // namespace SE
