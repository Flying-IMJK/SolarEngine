#pragma once
#include "ImportFileEntry.h"

namespace SE::Editor
{
	/// <summary>
	/// Folder import entry.
	/// </summary>
	class FolderImportEntry : public ImportFileEntry
	{
	public:
		/// <summary>
		/// Flag used to skip showing import settings dialog to used. Can be used for importing assets from code by plugins.
		/// </summary>
		bool SkipSettingsDialog;

		/// <inheritdoc />
		FolderImportEntry(ImportRequest& request);

		/// <inheritdoc />
		bool Import() override;
	};

} // SE

