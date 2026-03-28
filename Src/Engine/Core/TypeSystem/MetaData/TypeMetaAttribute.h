#pragma once

#include "Core/Serialization/Json.h"
#include "Core/TypeSystem/IType.h"

namespace SE
{
	class SE_API_CORE TypeMetaAttribute
	{
	public:
		virtual ~TypeMetaAttribute() = default;

		virtual bool Parse(const Json::Array& value) = 0;
	};
} // SE