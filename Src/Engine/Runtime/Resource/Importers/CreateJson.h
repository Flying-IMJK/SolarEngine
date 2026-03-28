#pragma once
#include "Types.h"
#include "Core/Serialization/Json.h"

namespace SE
{
	/// <summary>
	/// Json resources factory. Ensure to keep data encoded in UTF-8.
	/// </summary>
	class CreateJson
	{
	public:
		static bool Create(const StringView& path, Json::StringBuffer& data, const TypeID& dataTypeID);
		static bool Create(const StringView& path, const StringAnsiView& data, const TypeID& dataTypeID);
		static CreateAssetResult ImportPo(CreateAssetContext& context);
	};
}
