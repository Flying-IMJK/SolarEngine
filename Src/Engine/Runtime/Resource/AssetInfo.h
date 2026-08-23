
#pragma once

#include "Runtime/Core/TypeSystem/IType.h"
#include "Runtime/Core/Types/UID.h"
#include "Runtime/API.h"

namespace SE
{
	/// <summary>
	/// 包含有关资源的简短信息
	/// </summary>
	SE_STRUCT(Reflect)
	struct SE_API_RUNTIME AssetInfo : public IType
	{
		SE_DEFINE_CLASS_DEFAULT(AssetInfo, IType);

		/// <summary>
		/// Unique ID.
		/// </summary>
 		SE_FIELD(Reflect)
		UID id = UID::Empty;

		/// <summary>
		/// The stored data typeID. Used to recognize asset type.
		/// </summary>
		SE_FIELD(Reflect)
		TypeID typeID = TypeID::Invalid;

		/// <summary>
		/// Cached path.
		/// </summary>
		SE_FIELD(Reflect)
		String path = String::Empty;

	public:
		/// <summary>
		/// Initializes a new instance of the <see cref="AssetInfo"/> struct.
		/// </summary>
		/// <param name="id">The identifier.</param>
		/// <param name="typeId">The typeid</param>
		/// <param name="path">The path.</param>
		AssetInfo(const UID& id, const TypeID typeId, const StringView& path) : id(id), typeID(typeId), path(path)
		{
		}

	public:
		/// <summary>
		/// Gets the string.
		/// </summary>
		/// <returns>The string.</returns>
		String ToString() const;
	};
}
