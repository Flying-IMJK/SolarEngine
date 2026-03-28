#pragma once

#include "Core/TypeSystem/MetaData/TypeMetaAttribute.h"

namespace SE
{
	/// <summary>
	/// Specifies the order in which properties appear in the editor.
	/// </summary>
	SE_META()
	class SE_API_CORE EditorOrderAttribute : public TypeMetaAttribute
	{
	public:
		int Order = 0;

		EditorOrderAttribute() = default;
		
		bool Parse(const Json::Array& value) override;
	};
} // namespace SE
