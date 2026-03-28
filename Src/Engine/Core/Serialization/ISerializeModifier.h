#pragma once

#include "Core/Types/UID.h"
#include "Core/Types/Collections/Dictionary.h"

namespace SE
{
	/// <summary>
	/// Object serialization modification base class. Allows to extend the serialization process by custom effects like object ids mapping.
	/// </summary>
	class ISerializeModifier
	{
	public:
		/// <summary>
		/// Number of engine build when data was serialized. Useful to upgrade data from the older storage format.
		/// </summary>
		uint32 EngineBuild = 1;

		// Utility for scene deserialization to track currently mapped in Prefab Instance object IDs into IdsMapping.
		int32 CurrentInstance = -1;

		/// <summary>
		/// The object IDs mapping. Key is a serialized object id, value is mapped value to use.
		/// </summary>
		Dictionary<UID, UID> IdsMapping;
	};
}
