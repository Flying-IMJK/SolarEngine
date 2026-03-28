#pragma once

#include "TypeInfo.h"

namespace SE
{
	class TypeMetaAttribute;

	class TypeMetaInfo : public TypeInfo
	{
	public:
		virtual TypeMetaAttribute* Create() const = 0;
	};


	template <typename T>
	class TTypeMetaInfo : public TypeMetaInfo
	{

	};

} // SE

