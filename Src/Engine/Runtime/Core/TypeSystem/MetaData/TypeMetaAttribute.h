#pragma once

#include "Runtime/Core/Serialization/Json.h"
#include "Runtime/Core/TypeSystem/IType.h"

namespace SE
{
	class SE_API_RUNTIME TypeMetaAttribute
	{
	public:
		virtual ~TypeMetaAttribute() = default;

		virtual bool Parse(const Json::Array& value) = 0;
	};
} // SE