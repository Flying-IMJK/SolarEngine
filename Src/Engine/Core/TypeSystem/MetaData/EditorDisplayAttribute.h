#pragma once

#include "Core/TypeSystem/MetaData/TypeMetaAttribute.h"

namespace SE
{
	/// <summary>
	/// Specifies display properties for a property in the editor.
	/// </summary>
	SE_META()
	class SE_API_CORE EditorDisplayAttribute : public TypeMetaAttribute
	{
	public:
		String Name;        // Display name
		String Group;       // Group name
		String Tooltip;     // Tooltip text

		static constexpr const char* InlineStyle = "inline";

		EditorDisplayAttribute() = default;
		
		bool Parse(const Json::Array& value) override;
	};
} // namespace SE
