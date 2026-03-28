#pragma once

#include "EditorModule.h"
#include "Editor/Resource/Import/ImportFileEntry.h"
#include "Core/Types/Strings/String.h"
#include "Core/Types/Collections/Dictionary.h"
#include "Core/Types/Delegate.h"
#include "Core/Platform/CriticalSection.h"
#include "Core/Thread/JobSystem.h"

namespace SE::Editor
{
	class ContentFolder;

	class AssetImportingModule : public EditorModule
	{
	public:
		class ImportingBatch : Threading::IJob
		{
			friend class AssetImportingModule;
		private:
			Threading::JobHandle m_JobHandle;

			int64 m_ImportBatchDone;
			int64 m_ImportBatchSize;
			List<ImportFileEntry*> m_ImportFileEntries;

		public:
			void Run(int32 index) override;

			int GetBatchDone() const;
			int GetBatchSize() const;
		};

    public:
		AssetImportingModule(EditorApp* app);

		int InitOrder() override;

		void OnInit() override;

		void OnUpdate() override;

		/// <summary>
		/// Imports the specified files.
		/// </summary>
		/// <param name="files">The files.</param>
		/// <param name="targetLocation">The target location.</param>
		/// <param name="skipSettingsDialog">True if skip any popup dialogs showing for import options adjusting. Can be used when importing files from code.</param>
		void Import(const String* files, int length, ContentFolder* targetLocation, bool skipSettingsDialog = false);

		/// <summary>
		/// Imports the specified file.
		/// </summary>
		/// <param name="file">The file.</param>
		/// <param name="targetLocation">The target location.</param>
		/// <param name="skipSettingsDialog">True if skip any popup dialogs showing for import options adjusting. Can be used when importing files from code.</param>
		/// <param name="settings">Import settings to override. Use null to skip this value.</param>
		void Import(StringView file, ContentFolder* targetLocation, bool skipSettingsDialog = false, void* settings = nullptr);

        /// <summary>
        /// Creates the entry.
        /// </summary>
        /// <param name="request">The import request.</param>
        /// <returns>Created file entry.</returns>
        ImportFileEntry* CreateEntry(ImportRequest& request);

    private:
        Dictionary<String, ImportFileEntryHandler> m_ImportEntryHandlerDict;
		List<ImportRequest*> m_Requests;
		CriticalSection m_CriticalSection;
		List<ImportingBatch*> m_ImportingBatches;

		void CreateBatch(List<ImportFileEntry*>& entries);


		void Import(StringView inputPath, ContentFolder* targetLocation, bool skipSettingsDialog, void* settings, bool skipDialog);

		/// <summary>
		/// Imports the specified file to the target destination.
		/// Actual importing is done later after gathering settings from the user via <see cref="ImportFilesDialog"/>.
		/// </summary>
		/// <param name="inputPath">The input path.</param>
		/// <param name="outputPath">The output path.</param>
		/// <param name="isInBuilt">True if use in-built importer (engine backend).</param>
		/// <param name="skipSettingsDialog">True if skip any popup dialogs showing for import options adjusting. Can be used when importing files from code.</param>
		/// <param name="settings">Import settings to override. Use null to skip this value.</param>
		void Import(StringView inputPath, StringView outputPath, bool isInBuilt, bool skipSettingsDialog = false, void* settings = nullptr);
	};
} // SE
