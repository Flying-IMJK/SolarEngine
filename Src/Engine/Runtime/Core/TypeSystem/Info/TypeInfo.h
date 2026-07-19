#pragma once

#include "Runtime/Core/TypeSystem/TypeID.h"

namespace SE
{
	class SE_API_RUNTIME TypeInfo
	{
	public:
		inline StringView GetTypeName() const { return fullName; }

	public:
		TypeID id = TypeID::Invalid;
		int32 size = -1;
		int32 alignment = -1;

		String name = String::Empty;
		String fullName = String::Empty;
	};
}
