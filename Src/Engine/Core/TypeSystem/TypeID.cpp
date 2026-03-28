

#include "TypeID.h"

#include "CoreTypes.h"

namespace SE
{
	TypeID TypeID::Invalid = TypeID(String::Empty);

	StableID StableID::Invalid = StableID::Generate<void>();

	bool TypeID::IsCoreType() const
	{
		return CoreTypeRegistry::IsCoreType(*this);
	}
}
