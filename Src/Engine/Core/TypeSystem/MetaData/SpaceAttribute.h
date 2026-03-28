#pragma once

#include "Core/TypeSystem/MetaData/TypeMetaAttribute.h"

namespace SE
{
	/// <summary>
	/// Adds vertical space before a property in the editor.
	/// </summary>
	SE_META()
	class SE_API_CORE SpaceAttribute : public TypeMetaAttribute
	{
	public:
		float Height = 10.0f;

		SpaceAttribute() = default;
		
		bool Parse(const Json::Array& value) override;
	};
} // namespace SE
