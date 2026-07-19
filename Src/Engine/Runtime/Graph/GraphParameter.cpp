
#include "GraphParameter.h"

#include "Runtime/Core/Types/Collections/DataContainer.h"

namespace SE
{

	BytesContainer GraphParameter::GetMetaData(int32 typeID) const
	{
		BytesContainer result;
		for (const auto& e : Meta.Entries)
		{
			if (e.TypeID == typeID)
			{
				result.Link(e.Data);
				break;
			}
		}
		return result;
	}
} // SE