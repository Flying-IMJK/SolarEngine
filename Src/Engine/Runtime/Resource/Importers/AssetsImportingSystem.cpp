
#include "AssetsImportingSystem.h"

#include "CreateMaterial.h"
#include "ImportFont.h"
#include "ImportModel.h"
#include "ImportTexture.h"
#include "ImportShader.h"

#include "Core/Serialization/JsonWriters.hpp"
#include "Core/Platform/FileSystem.h"
#include "Core/Thread/Threading.h"
#include "Core/Systems.h"
#include "Core/Profiler/Profiler.h"
#include "Core/TypeSystem/Types.h"
#include "Runtime/Resource/Storage/AssetStorages.h"
#include "Runtime/Resource/AssetContent.h"
#include "Runtime/Resource/AssetsCache.h"
#include "Runtime/Resource/AssetRef.h"
#include "Runtime/Resource/Asset.h"
#include "Runtime/EngineContext.h"




namespace SE
{
	// Tags used to detect asset creation mode
	const String AssetsImporting::CreateTextureTag(SE_TEXT("Texture"));
	const String AssetsImporting::CreateTextureAsTextureDataTag(SE_TEXT("TextureAsTextureData"));
	const String AssetsImporting::CreateTextureAsInitDataTag(SE_TEXT("TextureAsInitData"));
	const String AssetsImporting::CreateMaterialTag(SE_TEXT("Material"));
	const String AssetsImporting::CreateMaterialInstanceTag(SE_TEXT("MaterialInstance"));
	const String AssetsImporting::CreateCubeTextureTag(SE_TEXT("CubeTexture"));
	const String AssetsImporting::CreateModelTag(SE_TEXT("Model"));
	const String AssetsImporting::CreateRawDataTag(SE_TEXT("RawData"));
	const String AssetsImporting::CreateCollisionDataTag(SE_TEXT("CollisionData"));
	const String AssetsImporting::CreateAnimationGraphTag(SE_TEXT("AnimationGraph"));
	const String AssetsImporting::CreateSkeletonMaskTag(SE_TEXT("SkeletonMask"));
	const String AssetsImporting::CreateParticleEmitterTag(SE_TEXT("ParticleEmitter"));
	const String AssetsImporting::CreateParticleSystemTag(SE_TEXT("ParticleSystem"));
	const String AssetsImporting::CreateSceneAnimationTag(SE_TEXT("SceneAnimation"));
	const String AssetsImporting::CreateMaterialFunctionTag(SE_TEXT("MaterialFunction"));
	const String AssetsImporting::CreateParticleEmitterFunctionTag(SE_TEXT("ParticleEmitterFunction"));
	const String AssetsImporting::CreateAnimationGraphFunctionTag(SE_TEXT("AnimationGraphFunction"));
	const String AssetsImporting::CreateAnimationTag(SE_TEXT("Animation"));
	const String AssetsImporting::CreateBehaviorTreeTag(SE_TEXT("BehaviorTree"));
	const String AssetsImporting::CreateVisualScriptTag(SE_TEXT("VisualScript"));

	struct AssetsImportingData
	{
		/// <summary>
		/// The asset importers.
		/// </summary>
		List<AssetImporter> Importers;

		/// <summary>
		/// The asset creators.
		/// </summary>
		List<AssetCreator> Creators;
	} *systemData;


	class SE_API_RUNTIME AssetsImportingSystem : public ISystem
	{
	ENGINE_SYSTEM(AssetsImportingSystem)
	public:
		AssetsImportingSystem() : ISystem(SE_TEXT("AssetsImportingSystem"), -400)
		{
			systemData = New<AssetsImportingData>();
		}

		~AssetsImportingSystem()
		{
			Delete(systemData);
		}

		bool OnInit() override
		{
			// Initialize with in-build importers
			AssetImporter InBuildImporters[] =
			{
					// Textures and Cube Textures
					{ SE_TEXT("tga"), ASSET_FILES_EXTENSION, ImportTexture::Import },
					{ SE_TEXT("dds"), ASSET_FILES_EXTENSION, ImportTexture::Import },
					{ SE_TEXT("png"), ASSET_FILES_EXTENSION, ImportTexture::Import },
					{ SE_TEXT("bmp"), ASSET_FILES_EXTENSION, ImportTexture::Import },
					{ SE_TEXT("tif"), ASSET_FILES_EXTENSION, ImportTexture::Import },
					{ SE_TEXT("jpeg"), ASSET_FILES_EXTENSION, ImportTexture::Import },
					{ SE_TEXT("jpg"), ASSET_FILES_EXTENSION, ImportTexture::Import },
					{ SE_TEXT("hdr"), ASSET_FILES_EXTENSION, ImportTexture::Import },
					{ SE_TEXT("raw"), ASSET_FILES_EXTENSION, ImportTexture::Import },
					{ SE_TEXT("exr"), ASSET_FILES_EXTENSION, ImportTexture::Import },

/*				// IES Profiles
				{ SE_TEXT("ies"), ASSET_FILES_EXTENSION, ImportTexture::ImportIES },
				*/
				// Shaders
				{ SE_TEXT("shader"), ASSET_FILES_EXTENSION, ImportShader::Import },
/*
				// Audio
				{ SE_TEXT("wav"), ASSET_FILES_EXTENSION, ImportAudio::ImportWav },
				{ SE_TEXT("mp3"), ASSET_FILES_EXTENSION, ImportAudio::ImportMp3 },
#if COMPILE_WITH_OGG_VORBIS
				{ SE_TEXT("ogg"), ASSET_FILES_EXTENSION, ImportAudio::ImportOgg },
#endif
				*/
				// Fonts
				{ SE_TEXT("ttf"), ASSET_FILES_EXTENSION, ImportFont::Import },
				{ SE_TEXT("otf"), ASSET_FILES_EXTENSION, ImportFont::Import },

				// Models
				{ SE_TEXT("obj"), ASSET_FILES_EXTENSION, ImportModel::Import },
				{ SE_TEXT("fbx"), ASSET_FILES_EXTENSION, ImportModel::Import },
				{ SE_TEXT("gltf"), ASSET_FILES_EXTENSION, ImportModel::Import },

				/*// gettext PO files
				{ SE_TEXT("po"), SE_TEXT("json"), CreateJson::ImportPo },

				// Models (untested formats - may fail :/)
				{ SE_TEXT("blend"), ASSET_FILES_EXTENSION, ImportModel::Import },
				{ SE_TEXT("bvh"), ASSET_FILES_EXTENSION, ImportModel::Import },
				{ SE_TEXT("ase"), ASSET_FILES_EXTENSION, ImportModel::Import },
				{ SE_TEXT("ply"), ASSET_FILES_EXTENSION, ImportModel::Import },
				{ SE_TEXT("dxf"), ASSET_FILES_EXTENSION, ImportModel::Import },
				{ SE_TEXT("ifc"), ASSET_FILES_EXTENSION, ImportModel::Import },
				{ SE_TEXT("nff"), ASSET_FILES_EXTENSION, ImportModel::Import },
				{ SE_TEXT("smd"), ASSET_FILES_EXTENSION, ImportModel::Import },
				{ SE_TEXT("vta"), ASSET_FILES_EXTENSION, ImportModel::Import },
				{ SE_TEXT("mdl"), ASSET_FILES_EXTENSION, ImportModel::Import },
				{ SE_TEXT("md2"), ASSET_FILES_EXTENSION, ImportModel::Import },
				{ SE_TEXT("md3"), ASSET_FILES_EXTENSION, ImportModel::Import },
				{ SE_TEXT("md5mesh"), ASSET_FILES_EXTENSION, ImportModel::Import },
				{ SE_TEXT("q3o"), ASSET_FILES_EXTENSION, ImportModel::Import },
				{ SE_TEXT("q3s"), ASSET_FILES_EXTENSION, ImportModel::Import },
				{ SE_TEXT("ac"), ASSET_FILES_EXTENSION, ImportModel::Import },
				{ SE_TEXT("stl"), ASSET_FILES_EXTENSION, ImportModel::Import },
				{ SE_TEXT("lwo"), ASSET_FILES_EXTENSION, ImportModel::Import },
				{ SE_TEXT("lws"), ASSET_FILES_EXTENSION, ImportModel::Import },
				{ SE_TEXT("lxo"), ASSET_FILES_EXTENSION, ImportModel::Import },*/
			};

			systemData->Importers.Add(InBuildImporters, ARRAY_SIZE(InBuildImporters));

			// Initialize with in-build creators
			AssetCreator InBuildCreators[] =
				{
					// Textures
					{ AssetsImporting::CreateTextureTag, ImportTexture::Import },
					{ AssetsImporting::CreateTextureAsTextureDataTag, ImportTexture::ImportAsTextureData },
					{ AssetsImporting::CreateTextureAsInitDataTag, ImportTexture::ImportAsInitData },
					{ AssetsImporting::CreateCubeTextureTag, ImportTexture::ImportCube },

					// Materials
					{ AssetsImporting::CreateMaterialTag, CreateMaterial::Create },
					// { AssetsImporting::CreateMaterialInstanceTag, CreateMaterialInstance::Create },
				/*
					// Models
					{ AssetsImportingSystem::CreateModelTag, ImportModel::Create },

					// Other
					{ AssetsImportingSystem::CreateRawDataTag, CreateRawData::Create },
					{ AssetsImportingSystem::CreateCollisionDataTag, CreateCollisionData::Create },
					{ AssetsImportingSystem::CreateAnimationGraphTag, CreateAnimationGraph::Create },
					{ AssetsImportingSystem::CreateSkeletonMaskTag, CreateSkeletonMask::Create },
					{ AssetsImportingSystem::CreateParticleEmitterTag, CreateParticleEmitter::Create },
					{ AssetsImportingSystem::CreateParticleSystemTag, CreateParticleSystem::Create },
					{ AssetsImportingSystem::CreateSceneAnimationTag, CreateSceneAnimation::Create },
					{ AssetsImportingSystem::CreateMaterialFunctionTag, CreateMaterialFunction::Create },
					{ AssetsImportingSystem::CreateParticleEmitterFunctionTag, CreateParticleEmitterFunction::Create },
					{ AssetsImportingSystem::CreateAnimationGraphFunctionTag, CreateAnimationGraphFunction::Create },
					{ AssetsImportingSystem::CreateAnimationTag, CreateAnimation::Create },
					{ AssetsImportingSystem::CreateBehaviorTreeTag, CreateBehaviorTree::Create },
					{ AssetsImportingSystem::CreateVisualScriptTag, CreateVisualScript::Create },*/
				};

			systemData->Creators.Add(InBuildCreators, ARRAY_SIZE(InBuildCreators));

			return true;
		}

		void OnDispose() override
		{
			// Cleanup
			systemData->Importers.Clear();
			systemData->Creators.Clear();
		}
	};

	ENGINE_SYSTEM_REGISTER(AssetsImportingSystem);

	bool AssetsImporting::UseImportPathRelative = false;

	CreateAssetContext::CreateAssetContext(const StringView& inputPath, const StringView& outputPath, const UID& id, void* arg)
	{
		InputPath = inputPath;
		TargetAssetPath = outputPath;
		CustomArg = arg;
		Data.Header.ID = id;
		SkipMetadata = false;

		// TODO: we should use ASNI only chars path (Assimp can use only that kind)
		OutputPath = AssetContent::CreateTemporaryAssetPath();
	}

	CreateAssetResult CreateAssetContext::Run(const CreateAssetFunction& callback)
	{
		ENGINE_ASSERT(callback.IsBinded());

		// Call action
		auto result = callback(*this);
		if (result != CreateAssetResult::Ok)
			return result;

		// Skip for non-flax assets (eg. json resource or custom asset type)
		if (!TargetAssetPath.EndsWith(ASSET_FILES_EXTENSION))
			return CreateAssetResult::Ok;

		// Validate assigned TypeID
		if (Data.Header.TypeID == TypeID::Invalid)
		{
			LOG_WARNING("Resource", "Assigned asset TypeName is invalid.");
			return CreateAssetResult::InvalidTypeID;
		}

		// Add import metadata to the file (if it's empty)
		if (!SkipMetadata && Data.Metadata.IsInvalid())
		{
			Json::StringBuffer buffer;
			CompactJsonWriter writer(buffer);
			writer.StartObject();
			{
				AddMeta(writer);
			}
			writer.EndObject();
			Data.Metadata.Copy((const byte*)buffer.GetString(), (uint32)buffer.GetSize());
		}

		// Save file
		result = Storage::Create(OutputPath, Data) ? CreateAssetResult::Ok : CreateAssetResult::CannotSaveFile;
		if (result == CreateAssetResult::Ok)
		{
			_applyChangesResult = CreateAssetResult::Abort;
			INVOKE_ON_MAIN_THREAD(CreateAssetContext, CreateAssetContext::ApplyChanges, this);
			result = _applyChangesResult;
		}
		FileSystem::DeleteFile(OutputPath);

		return result;
	}

	bool CreateAssetContext::AllocateChunk(int32 index)
	{
		// Check index
		if (index < 0 || index >= ASSET_FILE_DATA_CHUNKS)
		{
			LOG_WARNING("Resource", "Invalid asset chunk index {0}.", index);
			return false;
		}

		// Check if chunk has been already allocated
		if (Data.Header.Chunks[index] != nullptr)
		{
			LOG_WARNING("Resource", "Asset chunk {0} has been already allocated.", index);
			return false;
		}

		// Create new chunk
		const auto chunk = New<StorageChunk>();
		Data.Header.Chunks[index] = chunk;

		if (chunk == nullptr)
		{
			OUT_OF_MEMORY;
		}

		return true;
	}

	void CreateAssetContext::AddMeta(JsonWriter& writer) const
	{
		writer.JKEY("ImportPath");
		writer.String(AssetsImporting::GetImportPath(InputPath));
		writer.JKEY("ImportUsername");
		writer.String(Platform::GetUserName());
	}

	void CreateAssetContext::ApplyChanges()
	{
		// Get access
		auto storage = AssetStorages::TryGetStorage(TargetAssetPath);
		if (storage && storage->IsLoaded())
		{
			if (!storage->CloseFileHandles())
			{
				LOG_ERROR("Resource", "Cannot close file access for '{}'", TargetAssetPath);
				_applyChangesResult = CreateAssetResult::CannotSaveFile;
				return;
			}
		}

		// Move file
		if (!FileSystem::MoveFile(TargetAssetPath, OutputPath, true))
		{
			LOG_WARNING("Resource", "Cannot move imported file {0} to the destination path {1}.", OutputPath, TargetAssetPath);
			_applyChangesResult = CreateAssetResult::CannotSaveFile;
			return;
		}

		// Reload (any asset using it will receive OnStorageReloaded event and handle it)
		if (storage)
		{
			storage->Reload();
		}

		_applyChangesResult = CreateAssetResult::Ok;
	}

	
	const AssetImporter* AssetsImporting::GetImporter(const String& extension)
	{
		for (int32 i = 0; i < systemData->Importers.Count(); i++)
		{
			AssetImporter* importer = &systemData->Importers[i];
			if (importer->FileExtension.Compare(extension, StringSearchCase::IgnoreCase) == 0)
			{
				return importer;
			}
		}
		return nullptr;
	}

	const AssetCreator* AssetsImporting::GetCreator(const String& tag)
	{
		for (int32 i = 0; i < systemData->Creators.Count(); i++)
		{
			if (systemData->Creators[i].Tag == tag)
				return &systemData->Creators[i];
		}
		return nullptr;
	}

	bool AssetsImporting::Create(const CreateAssetFunction& importFunc, const StringView& outputPath, UID& assetId, void* arg)
	{
		return Create(importFunc, StringView::Empty, outputPath, assetId, arg);
	}

	bool AssetsImporting::Create(const String& tag, const StringView& outputPath, UID& assetId, void* arg)
	{
		const auto creator = GetCreator(tag);
		if (creator == nullptr)
		{
			LOG_WARNING("Resource", "Cannot find asset creator object for tag \'{0}\'.", tag);
			return false;
		}
		return Create(creator->Callback, outputPath, assetId, arg);
	}

	bool AssetsImporting::Import(const StringView& inputPath, const StringView& outputPath, UID& assetId, void* arg)
	{
		LOG_INFO("Resource", "Importing file '{0}' to '{1}'...", inputPath, outputPath);

		// Check if input file exists
		if (!FileSystem::FileExists(inputPath))
		{
			LOG_ERROR("Resource", "Missing file '{0}'", inputPath);
			return false;
		}

		// Get file extension and try to find import function for it
		const String extension = FileSystem::GetExtension(inputPath).ToLower();

		// Special case for raw assets
		if (StringView(ASSET_FILES_EXTENSION).Compare(StringView(extension), StringSearchCase::IgnoreCase) == 0)
		{
			// Simply copy file (content layer will resolve duplicated IDs, etc.)
			return FileSystem::CopyFile(outputPath, inputPath);
		}

		// Find valid importer for that file
		const auto importer = GetImporter(extension);
		if (importer == nullptr)
		{
			LOG_ERROR("Resource", "Cannot import file \'{0}\'. Unknown file type.", inputPath);
			return false;
		}

		return Create(importer->Callback, inputPath, outputPath, assetId, arg);
	}

	bool AssetsImporting::ImportIfEdited(const StringView& inputPath, const StringView& outputPath, UID& assetId, void* arg)
	{
		// Check if asset not exists
		if (!FileSystem::FileExists(outputPath))
		{
			return Import(inputPath, outputPath, assetId, arg);
		}

		// Check if need to reimport file (it could be checked via asset header but this way is faster and works in general)
		const DateTime sourceEdited = FileSystem::GetFileLastEditTime(inputPath);
		const DateTime assetEdited = FileSystem::GetFileLastEditTime(outputPath);
		if (sourceEdited > assetEdited)
		{
			return Import(inputPath, outputPath, assetId, arg);
		}

		// No import
		if (!assetId.IsValid())
		{
			AssetInfo assetInfo;
			AssetContent::GetAssetInfo(outputPath, assetInfo);
			assetId = assetInfo.id;
		}
		return false;
	}

	String AssetsImporting::GetImportPath(const String& path)
	{
		if (UseImportPathRelative && !FileSystem::IsRelative(path)
#if PLATFORM_WINDOWS
			// Import path from other drive should be stored as absolute on Windows to prevent issues
			&& path.Length() > 2 && EngineContext::ProjectFolder.Length() > 2 && path[0] == EngineContext::ProjectFolder[0]
#endif
			)
		{
			return FileSystem::ConvertAbsolutePathToRelative(EngineContext::ProjectFolder, path);
		}
		return path;
	}

	bool AssetsImporting::Create(const Function<CreateAssetResult(CreateAssetContext&)>& callback, const StringView& inputPath,
		const StringView& outputPath, UID& assetId, void* arg)
	{
		PROFILE_CPU();
		ZoneText(*outputPath, outputPath.Length());
		const auto startTime = Platform::GetTimeSeconds();

		// Pick ID if not specified
		if (!assetId.IsValid())
			assetId = UID::New();

		// Check if asset at target path is loaded
		AssetRef<Asset> asset = AssetContent::GetAsset(outputPath);
		if (asset)
		{
			// Use the same ID
			assetId = asset->GetID();
		}
		else
		{
			// Check if asset already exists and isn't empty
			if (FileSystem::FileExists(outputPath) && FileSystem::GetFileSize(outputPath) > 0)
			{
				// Load storage container
				const auto storage = AssetStorages::GetStorage(outputPath);
				if (storage)
				{
					// Try to load old asset header and then use old ID
					List<Storage::Entry> e;
					storage->GetEntries(e);
					if (e.Count() == 1)
					{
						// Override asset id (use the old value)
						assetId = e[0].ID;
						LOG_INFO("Resource", "Asset already exists. Using old ID: {0}", assetId);
					}
					else
					{
						LOG_WARNING("Resource", "File {0} is a package.", outputPath);
					}
				}
				else
				{
					LOG_WARNING("Resource", "Cannot open storage container at {0}", outputPath);
				}
			}
			else
			{
				// Ensure that path exists
				const String outputDirectory = FileSystem::GetParentDirectory(outputPath);
				if (!FileSystem::CreateDirectory(outputDirectory))
				{
					LOG_WARNING("Resource", "Cannot create directory '{0}'", outputDirectory);
				}
			}
		}

		// Import file
		CreateAssetContext context(inputPath, outputPath, assetId, arg);
		const auto result = context.Run(callback);

		// Clear reference
		asset = nullptr;

		// Switch result
		if (result == CreateAssetResult::Ok)
		{
			// Register asset
			AssetContent::GetRegistry()->RegisterAsset(context.Data.Header, context.TargetAssetPath);

			// Done
			const auto endTime = Platform::GetTimeSeconds();
			LOG_INFO("Resource", "Asset '{0}' imported in {2}s! {1}", outputPath, context.Data.Header.ToString(), endTime - startTime);
		}
		else if (result == CreateAssetResult::Abort)
		{
			// Do nothing
			return false;
		}
		else if (result != CreateAssetResult::Skip)
		{
			LOG_ERROR("Resource", "Cannot import file '{0}'! Result: {1}", inputPath, Types::GetEnumString<CreateAssetResult>(result));
			return false;
		}

		return true;
	}
	
} // SE