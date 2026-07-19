#pragma once

#include "BitFlags.h"
#include "Runtime/Core/TypeSystem/Types.h"
#include "Runtime/Core/TypeSystem/Info/TypeEnumInfo.h"

namespace SE
{
	class TBFlagsUtility
	{
	public:
		template <typename T>
		static String ToString(EnumFlags<T> flag)
		{
			StringBuilder str;
			TypeEnumInfo const* enumInfo = Types::GetEnumInfo<T>();

			int i = 0;
/*			for (EnumInfo::ConstantInfo constantInfo : enumInfo->constants)
			{
				if (flag.IsFlagSet(constantInfo.value))
				{
					str.Append(enumInfo->id.ToString());
					str.Append(":");
					str.Append(constantInfo.id.ToString());

					if(i < enumInfo->constants.Count() - 1)
					{
						str.Append(" | ");
					}
				}
			}*/
			return str.ToString();
		}
	};
}
