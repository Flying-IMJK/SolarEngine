#pragma once

#include "Core/TypeSystem/MetaData/TypeMetaAttribute.h"

namespace SE
{
	/// <summary>
	/// Adds a header text before a property in the editor.
	/// </summary>
	SE_META()
	class SE_API_CORE HeaderAttribute : public TypeMetaAttribute
	{
	public:
		String Text;
		float FontSize = 0.0f;
		uint32 Color = 0;

		HeaderAttribute() = default;
		
		bool Parse(const Json::Array& value) override;
	};
} // namespace SE
