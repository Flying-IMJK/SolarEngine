
#include "AssetImportingModule.h"
#include "Runtime/Core/Platform/FileSystem.h"
#include "Runtime/Core/Thread/Threading.h"
#include "Runtime/Core/Platform/MessageBox.h"
#include "Runtime/Core/Thread/JobSystem.h"
#include "Editor/Resource/Import/ImportRequest.h"
#include "Editor/Resource/Import/FolderImportEntry.h"
#include "Editor/Resource/Import/ModelImportEntry.h"
#include "Editor/Resource/Import/TextureImportEntry.h"
#include "Editor/Resource/Items/Contentfolder.h"
#include "Editor/Resource/Tree/Contenttreenode.h"
#include "Runtime/Resource/Importers/AssetsImportingSystem.h"

namespace SE::Editor
{
    ImportFileEntry* ImportTexture(ImportRequest& request)
    {
        return New<TextureImportEntry>(request);
    };

    ImportFileEntry* ImportModel(ImportRequest& request)
    {
        return New<ModelImportEntry>(request);
    };

    int AssetImportingModule::InitOrder()
    {
        return -50;
    }

    void AssetImportingModule::OnInit()
    {
        // Textures
        m_ImportEntryHandlerDict[SE_TEXT("tga")] =  CreateFunc(ImportTexture);
        m_ImportEntryHandlerDict[SE_TEXT("png")] =  CreateFunc(ImportTexture);
        m_ImportEntryHandlerDict[SE_TEXT("bmp")] =  CreateFunc(ImportTexture);
        m_ImportEntryHandlerDict[SE_TEXT("gif")] =  CreateFunc(ImportTexture);
        m_ImportEntryHandlerDict[SE_TEXT("tiff")] = CreateFunc(ImportTexture);
        m_ImportEntryHandlerDict[SE_TEXT("tif")] =  CreateFunc(ImportTexture);
        m_ImportEntryHandlerDict[SE_TEXT("jpeg")] = CreateFunc(ImportTexture);
        m_ImportEntryHandlerDict[SE_TEXT("jpg")] =  CreateFunc(ImportTexture);
        m_ImportEntryHandlerDict[SE_TEXT("dds")] =  CreateFunc(ImportTexture);
        m_ImportEntryHandlerDict[SE_TEXT("hdr")] =  CreateFunc(ImportTexture);
        m_ImportEntryHandlerDict[SE_TEXT("raw")] =  CreateFunc(ImportTexture);
        m_ImportEntryHandlerDict[SE_TEXT("exr")] =  CreateFunc(ImportTexture);

        // Models
        m_ImportEntryHandlerDict[SE_TEXT("obj")] =  CreateFunc(ImportModel);
        m_ImportEntryHandlerDict[SE_TEXT("fbx")] =  CreateFunc(ImportModel);
        m_ImportEntryHandlerDict[SE_TEXT("x")] =    CreateFunc(ImportModel);
        m_ImportEntryHandlerDict[SE_TEXT("dae")] =  CreateFunc(ImportModel);
        m_ImportEntryHandlerDict[SE_TEXT("gltf")] = CreateFunc(ImportModel);
        m_ImportEntryHandlerDict[SE_TEXT("glb")] =  CreateFunc(ImportModel);
        //
        m_ImportEntryHandlerDict[SE_TEXT("blend")] = ImportModel;
        m_ImportEntryHandlerDict[SE_TEXT("bvh")] = ImportModel;
        m_ImportEntryHandlerDict[SE_TEXT("ase")] = ImportModel;
        m_ImportEntryHandlerDict[SE_TEXT("ply")] = ImportModel;
        m_ImportEntryHandlerDict[SE_TEXT("dxf")] = ImportModel;
        m_ImportEntryHandlerDict[SE_TEXT("ifc")] = ImportModel;
        m_ImportEntryHandlerDict[SE_TEXT("nff")] = ImportModel;
        m_ImportEntryHandlerDict[SE_TEXT("smd")] = ImportModel;
        m_ImportEntryHandlerDict[SE_TEXT("vta")] = ImportModel;
        m_ImportEntryHandlerDict[SE_TEXT("mdl")] = ImportModel;
        m_ImportEntryHandlerDict[SE_TEXT("md2")] = ImportModel;
        m_ImportEntryHandlerDict[SE_TEXT("md3")] = ImportModel;
        m_ImportEntryHandlerDict[SE_TEXT("md5mesh")] = ImportModel;
        m_ImportEntryHandlerDict[SE_TEXT("q3o")] = ImportModel;
        m_ImportEntryHandlerDict[SE_TEXT("q3s")] = ImportModel;
        m_ImportEntryHandlerDict[SE_TEXT("ac")] = ImportModel;
        m_ImportEntryHandlerDict[SE_TEXT("stl")] = ImportModel;
        m_ImportEntryHandlerDict[SE_TEXT("lwo")] = ImportModel;
        m_ImportEntryHandlerDict[SE_TEXT("lws")] = ImportModel;
        m_ImportEntryHandlerDict[SE_TEXT("lxo")] = ImportModel;

        // Audio
//        importEntryHandlerDict[SE_TEXT("wav")] = ImportAudio;
//        importEntryHandlerDict[SE_TEXT("mp3")] = ImportAudio;
//        importEntryHandlerDict[SE_TEXT("ogg")] = ImportAudio;


    }

    void AssetImportingModule::OnUpdate()
    {
        for (int i = m_ImportingBatches.Count() - 1; i >= 0; i--)
        {
            ImportingBatch* batch = m_ImportingBatches[i];
            if (Threading::JobSystem::IsCompleted(batch->m_JobHandle))
            {
                m_ImportingBatches.RemoveAt(i);
                Delete(batch);
            }
        }

        // Check if has no requests to process
        if (m_Requests.Count() == 0)
        {
            return;
        }

        {
            m_CriticalSection.Lock();
            // Get entries
            List<ImportFileEntry*> entries(m_Requests.Count());
            bool needSettingsDialog = false;
            for (int i = 0; i < m_Requests.Count(); i++)
            {
                ImportRequest* request = m_Requests[i];
                ImportFileEntry* entry = CreateEntry(*request);
                if (entry != nullptr)
                {
                    if (request->Settings.has_value() && entry->TryOverrideSetting(request->Settings))
                    {
                        // Use overridden settings
                    }
                    else if (!request->SkipSettingsDialog)
                    {
                        needSettingsDialog |= entry->HasSetting();
                    }

                    entries.Add(entry);
                }

                Delete(request);
            }
            m_Requests.Clear();
            m_CriticalSection.Unlock();

            // Check if need to show importing dialog or can just pass requests
            if (needSettingsDialog)
            {
                /*var dialog = new ImportFilesDialog(entries);
                dialog.Show(Editor.Windows.MainWindow);
                dialog.Focus();*/
            }
            else
            {
                CreateBatch(entries);
            }
        }
    }

    void AssetImportingModule::Import(StringView file, ContentFolder* targetLocation, bool skipSettingsDialog, void* settings)
    {
        bool skipDialog = skipSettingsDialog;
        Import(file, targetLocation, skipSettingsDialog, settings, skipDialog);
    }

    ImportFileEntry* AssetImportingModule::CreateEntry(ImportRequest& request)
    {
        // Get extension (without a dot)
        String extension = FileSystem::GetExtension(request.InputPath);
        if (extension.IsEmpty())
            return New<FolderImportEntry>(request);

        if (extension[0] == '.')
        {
            extension.Remove(0, 1);
        }
        extension = extension.ToLower();

        ImportFileEntryHandler handler;
        // Check if use overriden type
        if (m_ImportEntryHandlerDict.TryGet(extension, handler))
            return handler(request);

//        return request.IsInBuilt ? new AssetImportEntry(ref request) : new ImportFileEntry(ref request);
        return nullptr;
    }

    void AssetImportingModule::Import(const String* files, int length, ContentFolder* targetLocation, bool skipSettingsDialog)
    {
        if (targetLocation == nullptr || files == nullptr)
        {
            return;
        }

        bool skipDialog = skipSettingsDialog;
        for (int i = 0; i < length; i++)
        {
            Import(files[i], targetLocation, skipSettingsDialog, nullptr, skipDialog);
        }
    }

    void AssetImportingModule::CreateBatch(List<ImportFileEntry*>& entries)
    {
        ImportingBatch*& importingBatch = m_ImportingBatches.AddOne();

        importingBatch = New<ImportingBatch>();
        importingBatch->m_ImportFileEntries = entries;
        importingBatch->m_ImportBatchSize = entries.Count();
        importingBatch->m_ImportBatchDone = 0;
        importingBatch->m_JobHandle = Threading::JobSystem::Dispatch(importingBatch, 1);
    }

    void AssetImportingModule::Import(StringView inputPath, ContentFolder* targetLocation, bool skipSettingsDialog, void* settings, bool skipDialog)
    {
        ENGINE_ASSERT(targetLocation != nullptr)

        String extension = FileSystem::GetExtension(inputPath);
        // Check if given file extension is a binary asset (.flax files) and can be imported by the engine
        String outputExtension;
        const AssetImporter* importer = AssetsImporting::GetImporter(extension);
        bool isBuilt = importer != nullptr;
        if (isBuilt)
        {
            outputExtension = String::Format(SE_TEXT(".{0}"), importer->ResultExtension);

            if (!targetLocation->CanHaveAssets)
            {
                // Error
                LOG_WARNING("Asset", "Cannot import \'{0}\' to \'{1}\'. The target directory cannot have assets.", inputPath, targetLocation->Node->GetPath());
                if (!skipDialog)
                {
                    skipDialog = true;
                    MessageBox::Show(SE_TEXT("Target location cannot have assets. Use Content folder for your game assets."), SE_TEXT("Cannot import assets"), MessageBoxButtons::OK, MessageBoxIcon::Error);
                }
                return;
            }
        }
        else
        {
            // Preserve file extension (will copy file to the import location)
            outputExtension = extension;

            /*// Check if can place source files here
            if (!targetLocation.CanHaveScripts && (extension == ".cs" || extension == ".cpp" || extension == ".h"))
            {
                // Error
                LOG_WARNING("Asset", "Cannot import \'{0}\' to \'{1}\'. The target directory cannot have scripts.", inputPath, targetLocation->Node.Path));
                if (!skipDialog)
                {
                    skipDialog = true;
                    MessageBox::Show("Target location cannot have scripts. Use Source folder for your game source code.", "Cannot import assets", MessageBoxButtons::OK, MessageBoxIcon::Error);
                }
                return;
            }*/
        }

        StringView shortName = FileSystem::GetFileNameWithoutExtension(inputPath);
        String outputPath = FileSystem::CombinePaths(targetLocation->Path, String::Format(SE_TEXT("{0}{1}"), shortName, outputExtension));

        Import(inputPath, outputPath, isBuilt, skipSettingsDialog, settings);
    }

    void AssetImportingModule::Import(StringView inputPath, StringView outputPath, bool isInBuilt, bool skipSettingsDialog, void* settings)
    {
        m_CriticalSection.Lock();
        ImportRequest* request = New<ImportRequest>();
        request->InputPath = inputPath;
        request->OutputPath = outputPath;
        request->IsInBuilt = isInBuilt;
        request->SkipSettingsDialog = skipSettingsDialog;
        request->Settings = settings;

        m_Requests.Add(request);
        m_CriticalSection.Unlock();
    }

    void AssetImportingModule::ImportingBatch::Run(int32 index)
    {
        for (int i = 0; i < m_ImportBatchSize; i++)
        {
            ImportFileEntry* fileEntry = m_ImportFileEntries[i];

            fileEntry->Execute();
            Platform::AtomicAdd(&m_ImportBatchDone, 1);
        }
    }

    int AssetImportingModule::ImportingBatch::GetBatchDone() const
    {
        return Platform::AtomicRead(&m_ImportBatchDone);
    }

    int AssetImportingModule::ImportingBatch::GetBatchSize() const
    {
        return Platform::AtomicRead(&m_ImportBatchSize);
    }

    AssetImportingModule::AssetImportingModule(EditorApp* app) : EditorModule(app)
    {
    }
} // SE