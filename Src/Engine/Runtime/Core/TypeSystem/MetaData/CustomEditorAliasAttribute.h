#pragma once

#include "Runtime/Core/TypeSystem/MetaData/TypeMetaAttribute.h"

namespace SE
{
	/// <summary>
	/// Specifies a custom editor type name alias for a property.
	/// </summary>
	SE_META()
	class SE_API_RUNTIME CustomEditorAliasAttribute : public TypeMetaAttribute
	{
	public:
		TypeID typeID;

		CustomEditorAliasAttribute() = default;
		
		bool Parse(const Json::Array& value) override;
	};
} // namespace SE
