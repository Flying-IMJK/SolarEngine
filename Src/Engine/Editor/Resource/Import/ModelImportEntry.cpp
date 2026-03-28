
#include "ModelImportEntry.h"

#include "Runtime/Resource/Importers/AssetsImportingSystem.h"
#include "Editor/Modules/AssetImportingModule.h"

namespace SE::Editor
{
	ModelImportEntry::ModelImportEntry(ImportRequest& request) : ImportFileEntry(request)
	{
		// Try to restore target asset model import options (useful for fast reimport)
		// Editor.TryRestoreImportOptions(_settings.Settings, ResultUrl);
	}

	bool ModelImportEntry::Import()
	{
		return AssetsImporting::Import(SourceUrl, ResultUrl, &options);
	}
} // SE