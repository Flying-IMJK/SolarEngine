#pragma once
#include "ImportFileEntry.h"
#include "Runtime/Resource/Importers/ImportModel.h"

namespace SE::Editor
{

	class ModelImportEntry : public ImportFileEntry
	{
		// private ModelImportSettings _settings = new();
	private:
		ImportModel::Options options;

	public:
		/// <summary>
		/// Initializes a new instance of the <see cref="ModelImportEntry"/> class.
		/// </summary>
		ModelImportEntry(ImportRequest& request);

		/// <inheritdoc />
		bool Import() override;
	};

} // SE

