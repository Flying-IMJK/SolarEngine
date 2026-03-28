#pragma once

#include "Types.h"
#include "Runtime/Utilities/Model/ModelUtils.h"

namespace SE
{

	/// <summary>
	/// Importing models utility
	/// </summary>
	class ImportModel
	{
	public:
		typedef ModelUtils::Options Options;

	public:
		/// <summary>
		/// Tries the get model import options from the target location asset.
		/// </summary>
		/// <param name="path">The asset path.</param>
		/// <param name="options">The options.</param>
		/// <returns>True if success, otherwise false.</returns>
		static bool TryGetImportOptions(const StringView& path, Options& options);

		/// <summary>
		/// Imports the model file.
		/// </summary>
		/// <param name="context">The importing context.</param>
		/// <returns>Result.</returns>
		static CreateAssetResult Import(CreateAssetContext& context);

		/// <summary>
		/// Creates the model asset from the ModelData storage (input argument should be pointer to ModelData).
		/// </summary>
		/// <param name="context">The importing context.</param>
		/// <returns>Result.</returns>
		static CreateAssetResult Create(CreateAssetContext& context);

	private:
		static CreateAssetResult CreateModel(CreateAssetContext& context, ModelData& data, const Options* options = nullptr);
		static CreateAssetResult CreateSkinnedModel(CreateAssetContext& context, ModelData& data, const Options* options = nullptr);
		static CreateAssetResult CreateAnimation(CreateAssetContext& context, ModelData& data, const Options* options = nullptr);
		static CreateAssetResult CreatePrefab(CreateAssetContext& context, ModelData& data, const Options& options, const List<struct PrefabObject>& prefabObjects);
	};

} // SE
