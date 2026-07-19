#pragma once

#include "Runtime/Core/TypeSystem/MetaData/TypeMetaAttribute.h"

namespace SE
{
	/// <summary>
	/// Specifies a custom editor type for a property.
	/// </summary>
	SE_META()
	class SE_API_RUNTIME CustomEditorAttribute : public TypeMetaAttribute
	{
	public:
		TypeID EditorType = TypeID::Invalid;

		CustomEditorAttribute() = default;
		
		bool Parse(const Json::Array& value) override;
	};
} // namespace SE
