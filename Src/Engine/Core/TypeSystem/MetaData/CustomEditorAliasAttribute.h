#pragma once

#include "Core/TypeSystem/MetaData/TypeMetaAttribute.h"

namespace SE
{
	/// <summary>
	/// Specifies a custom editor type name alias for a property.
	/// </summary>
	SE_META()
	class SE_API_CORE CustomEditorAliasAttribute : public TypeMetaAttribute
	{
	public:
		TypeID typeID;

		CustomEditorAliasAttribute() = default;
		
		bool Parse(const Json::Array& value) override;
	};
} // namespace SE
