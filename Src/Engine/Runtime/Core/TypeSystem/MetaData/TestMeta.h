#pragma once
#include "TypeMetaAttribute.h"

namespace SE
{
	SE_META()
	class SE_API_RUNTIME TestMeta final : public TypeMetaAttribute
	{
	public:
		bool Parse(const Json::Array& value) override
		{
			return true;
		}
	};

} // SE

