
#pragma once

#include "Core/Types/Strings/String.h"
#include "Core/Types/Strings/StringView.h"
#include "Core/Types/UID.h"
#include "Core/TypeSystem/TypeID.h"

#include "Runtime/API.h"

namespace SE
{
	struct AssetInfo;

	/// <summary>
	/// Helper class for loading/saving Json file resources
	/// </summary>
	class SE_API_RUNTIME JsonStorageProxy
	{
	public:
		/// <summary>
		/// Determines whether the specified extension can be a json resource file.
		/// </summary>
		/// <param name="extension">The path.</param>
		/// <returns>True if can be a json resource extension, otherwise false.</returns>
		static bool IsValidExtension(const StringView& extension);

		/// <summary>
		/// Find asset info by path
		/// </summary>
		/// <param name="path">Asset path</param>
		/// <param name="resultId">Asset ID</param>
		/// <param name="resultDataTypeID">Asset data TypeID</param>
		/// <returns>True if found any asset, otherwise false.</returns>
		static bool GetAssetInfo(const StringView& path, UID& resultId, TypeID& resultDataTypeID);

		/// <summary>
		/// Changes asset ID.
		/// </summary>
		/// <param name="path">Asset path</param>
		/// <param name="newId">Asset ID to set</param>
		/// <returns>True if found any asset, otherwise false.</returns>
		static bool ChangeId(const StringView& path, const UID& newId);
	};
}