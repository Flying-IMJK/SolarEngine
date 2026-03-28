
#include "ShadersCompilation.h"

#include "Core/Profiler/Profiler.h"
#include "Core/Types/DateTime.h"
#include "Core/Types/Delegate.h"
#include "Core/Platform/FileSystem.h"
#include "Core/Platform/File.h"
#include "Core/Platform/FileSystemWatcher.h"
#include "Core/Platform/Win32/Win32CriticalSection.h"
#include "Core/Serialization/MemoryReadStream.h"
#include "Core/Systems.h"
#include "Core/Serialization/MemoryWriteStream.h"
#include "Core/Thread/Threading.h"

#include "Runtime/EngineContext.h"
#include "Runtime/Graphics/Base/GPUUtils.h"
#include "Runtime/Graphics/Shaders/GPUShader.h"
#include "Runtime/Project/ProjectInfo.h"
#include "Runtime/Resource/AssetContent.h"
#include "Runtime/Resource/Assets/Materials/Material.h"
#include "Runtime/Resource/Assets/Materials/Shader.h"
#include "Runtime/Resource/Importers/AssetsImportingSystem.h"

#include "Parser/GraphicShaderParser.h"
#include "Vulkan/ShaderCompilerVulkan.h"



namespace SE
{
	namespace ShadersCompilationImpl
	{
		CriticalSection Locker;
		List<ShaderCompiler*> Compilers;
		List<ShaderCompiler*> ReadyCompilers;

/*#if SE_EDITOR
		const ProjectInfo* FindProjectByName(const ProjectInfo* project, HashSet<const ProjectInfo*>& projects, const StringView& projectName)
    {
        if (!project || projects.Contains(project))
            return nullptr;
        projects.Add(project);

        // Check the project name
        if (project->Name == projectName)
            return project;

        // Search referenced projects
        for (const auto& reference : project->References)
        {
            const ProjectInfo* result = FindProjectByName(reference.Project, projects, projectName);
            if (result)
                return result;
        }
        return nullptr;
    }

    	const ProjectInfo* FindProjectByPath(const ProjectInfo* project, HashSet<const ProjectInfo*>& projects, const StringView& projectPath)
    {
        if (!project || projects.Contains(project))
            return nullptr;
        projects.Add(project);

        // Search referenced projects (depth first to handle plugin projects first)
        for (const auto& reference : project->References)
        {
            const ProjectInfo* result = FindProjectByPath(reference.Project, projects, projectPath);
            if (result)
                return result;
        }

        // Check the project path
        if (projectPath.StartsWith(project->ProjectFolderPath))
            return project;

        return nullptr;
    }
#endif*/

		CriticalSection ShaderIncludesMapLocker;
		Dictionary<String, List<Asset*>> ShaderIncludesMap;
		Dictionary<String, FileSystemWatcher*> ShaderIncludesWatcher;

		void OnShaderIncludesWatcherEvent(FileWatcherEvent &event)
		{
			if (event.action == FileSystemAction::Delete)
				return;

			// Get list of assets using this shader file
			List<Asset*> toReload;
			{
				Threading::ScopeLock lock(ShaderIncludesMapLocker);

				auto file = ShaderIncludesMap.Find(event.path);
				if (file == ShaderIncludesMap.end())
					return;
				toReload = file->Value;
			}

			// Add any shaders that failed to load (eg. due to error in included header)
			for (Asset* asset : AssetContent::GetAssets())
			{
				if (asset->LastLoadFailed() && !toReload.Contains(asset))
				{
					if (TypeAs<Shader>(asset) || TypeAs<Material>(asset)/* || asset->Is<ParticleEmitter>()*/)
					{
						toReload.Add(asset);
					}
				}
			}

			LOG_INFO("ShaderCompiler", "Shader include \'{0}\' has been modified.", event.path);

			// 稍等片刻，以便正在编辑文件的编辑器（例如 Visual Studio）有足够的时间刷新整个文件更改
			Platform::Sleep(100);

			// Reload shaders using this include
			for (Asset* asset : toReload)
			{
				asset->Reload();
			}
		}
	}

	using namespace ShadersCompilationImpl;

	class ShadersCompilationSystem : public ISystem
	{
		ENGINE_SYSTEM(ShadersCompilationSystem)
	public:
		ShadersCompilationSystem()
			: ISystem(SE_TEXT("Shaders Compilation System"), -100)
		{
		}

		bool OnInit() override;
		void OnDispose() override;
	};

	ENGINE_SYSTEM_REGISTER(ShadersCompilationSystem)

	bool ShadersCompilation::Compile(ShaderCompilationOptions& options)
	{
		PROFILE_CPU_NAMED("Shader.Compile");

		// Validate input options
		if (options.TargetName.IsEmpty() || !options.TargetID.IsValid())
		{
			LOG_WARNING("ShaderCompiler", "Unknown target object.");
			return false;
		}
		if (options.Output == nullptr)
		{
			LOG_WARNING("ShaderCompiler", "Missing output.");
			return false;
		}
		if (options.Profile == ShaderProfile::Unknown)
		{
			LOG_WARNING("ShaderCompiler", "Unknown shader profile.");
			return false;
		}
		if (options.Source == nullptr || options.SourceLength < 1)
		{
			LOG_WARNING("ShaderCompiler", "Missing source code.");
			return false;
		}

		// Adjust input source length if it ends with null
		while (options.SourceLength > 2 && options.Source[options.SourceLength - 1] == 0)
			options.SourceLength--;

		const DateTime startTime = DateTime::NowUTC();
		const FeatureLevel featureLevel = GPUUtils::GetFeatureLevel(options.Profile);

		// Process shader source to collect metadata
		ShaderMeta meta;
		if (!ShaderParser::Parser::Process(options.TargetName, options.Source, options.SourceLength, options.Macros, featureLevel, &meta))
		{
			LOG_WARNING("ShaderCompiler", "Failed to parse source code.");
			return false;
		}
		const int32 shadersCount = meta.GetShadersCount();
		if (shadersCount == 0)
		{
			LOG_WARNING("ShaderCompiler", "Shader has no valid functions.");
		}

		// Perform actual compilation
		bool result;
		{
			ShaderCompilationContext context(&options, &meta);

			// Request shaders compiler
			auto compiler = RequestCompiler(options.Profile);
			if (compiler == nullptr)
			{
				LOG_ERROR("ShaderCompiler", "Shader compiler request failed.");
				return false;
			}
			ENGINE_ASSERT(compiler->GetProfile() == options.Profile);

			// Call compilation process
			result = compiler->Compile(&context);

			// Dismiss compiler
			FreeCompiler(compiler);

			/*#if GPU_USE_SHADERS_DEBUG_LAYER
						// Export debug data
					S	haderDebugDataExporter::Export(&context);
			#endif*/
		}

#if SE_EDITOR
		if (!result || options.ForceStoreCompilationSource)
		{
			// Output shader source to easily investigate errors (eg. for generated shaders like materials or particles)
			const String outputSourceFolder = EngineContext::ProjectCacheFolder / SE_TEXT("/Shaders/CompilationSource");
			const String outputSourcePath = outputSourceFolder / options.TargetName + SE_TEXT(".hlsl");
			if (!FileSystem::DirectoryExists(outputSourceFolder))
				FileSystem::CreateDirectory(outputSourceFolder);
			File::WriteAllBytes(outputSourcePath, (const byte*)options.Source, options.SourceLength);
		}
#endif


		if (!result)
		{
			LOG_ERROR("ShaderCompiler", "Shader compilation '{0}' failed (profile: {1})", options.TargetName, ToString(options.Profile));
		}
		else
		{
			// Success
			const DateTime endTime = DateTime::NowUTC();
			LOG_INFO("ShaderCompiler",
				"Shader compilation '{0}' succeed in {1} ms (profile: {2})",
				options.TargetName,
				Math::CeilToInt(static_cast<float>((endTime - startTime).GetTotalMilliseconds())),
				ToString(options.Profile));
		}

		return result;
	}

	ShaderCompiler* ShadersCompilation::CreateCompiler(ShaderProfile profile)
	{
		ShaderCompiler* result = nullptr;

		switch (profile)
		{
/*
		case ShaderProfile::DirectX_SM4:
		case ShaderProfile::DirectX_SM5:
			result = New<ShaderCompilerD3D>(profile);
			break;

		case ShaderProfile::DirectX_SM6:
			result = New<ShaderCompilerDX>(profile);
			break;
*/

		case ShaderProfile::Vulkan_SM5:
			result = New<ShaderCompilerVulkan>(profile);
			break;
/*		case ShaderProfile::PS4:
			result = New<ShaderCompilerPS4>();
			break;

		case ShaderProfile::PS5:
			result = New<ShaderCompilerPS5>();
			break;*/
		default:
			break;
		}
		ASSERT_LOW_LAYER(result == nullptr || result->GetProfile() == profile);

		return result;
	}

	ShaderCompiler* ShadersCompilation::RequestCompiler(ShaderProfile profile)
	{
		ShaderCompiler* compiler;
		Threading::ScopeLock lock(Locker);

		// Try to find ready compiler
		for (int32 i = 0; i < ReadyCompilers.Count(); i++)
		{
			compiler = ReadyCompilers[i];
			if (compiler->GetProfile() == profile)
			{
				// Use it
				ReadyCompilers.RemoveAt(i);
				return compiler;
			}
		}

		// Create new compiler for a target profile
		compiler = CreateCompiler(profile);
		if (compiler == nullptr)
		{
			LOG_ERROR("ShaderCompiler", "Cannot create Shader Compiler for profile {0}", ToString(profile));
			return nullptr;
		}

		// Register new compiler
		Compilers.Add(compiler);

		return compiler;
	}

	void ShadersCompilation::FreeCompiler(ShaderCompiler* compiler)
	{
		Threading::ScopeLock lock(Locker);
		ENGINE_ASSERT(compiler && ReadyCompilers.Contains(compiler) == false);

		// Check if service has been disposed (this compiler is not in the compilers list)
		if (Compilers.Contains(compiler) == false)
		{
			// Delete it manually
			Delete(compiler);
		}
		else
		{
			// Make compiler free again
			ReadyCompilers.Add(compiler);
		}
	}



	void ShadersCompilation::RegisterForShaderReloads(Asset* asset, const String& includedPath)
	{
		Threading::ScopeLock lock(ShaderIncludesMapLocker);

		// Add to collection
		const bool alreadyAdded = ShaderIncludesMap.ContainsKey(includedPath);
		auto& file = ShaderIncludesMap[includedPath];
		ASSERT_LOW_LAYER(!file.Contains(asset));
		file.Add(asset);

		if (!alreadyAdded)
		{
			// Create a directory watcher to track the included file changes
			const String directory = FileSystem::GetDirectoryName(includedPath);
			if (FileSystem::DirectoryExists(directory) && !ShaderIncludesWatcher.ContainsKey(directory))
			{
				auto watcher = New<FileSystemWatcher>(directory, false);
				watcher->OnEvent.Bind<OnShaderIncludesWatcherEvent>();
				ShaderIncludesWatcher.Add(directory, watcher);
			}
		}
	}

	void ShadersCompilation::UnregisterForShaderReloads(Asset* asset)
	{
		Threading::ScopeLock lock(ShaderIncludesMapLocker);

		// Remove asset reference
		for (auto& file : ShaderIncludesMap)
		{
			file.Value.Remove(asset);
		}
	}

	void ShadersCompilation::ExtractShaderIncludes(byte* shaderCache, int32 shaderCacheLength, List<String>& includes)
	{
		MemoryReadStream stream(shaderCache, shaderCacheLength);

		// Read cache format version
		int32 version;
		stream.ReadInt32(&version);
		if (version != GPU_SHADER_CACHE_VERSION)
		{
			return;
		}

		// Read the location of additional data that contains list of included source files
		int32 additionalDataStart;
		stream.ReadInt32(&additionalDataStart);
		stream.SetPosition(additionalDataStart);

		// Read all includes
		int32 includesCount;
		stream.ReadInt32(&includesCount);
		includes.Clear();
		for (int32 i = 0; i < includesCount; i++)
		{
			String& include = includes.AddOne();
			stream.ReadString(&include, 11);
			include = ShadersCompilation::ResolveShaderPath(include);
			DateTime lastEditTime;
			stream.Read(lastEditTime);
		}
	}

	String ShadersCompilation::ResolveShaderPath(StringView path)
	{
		// Skip to the last root start './' but preserve the leading one
		for (int32 i = path.Length() - 2; i >= 2; i--)
		{
			if (StringUtils::Compare(path.Get() + i, SE_TEXT("./"), 2) == 0)
			{
				path = path.Substring(i);
				break;
			}
		}

		// Find the included file path
		String result;
#if SE_EDITOR
		if (path.StartsWith(SE_TEXT("./")))
		{
			int32 projectNameEnd = -1;
			for (int32 i = 2; i < path.Length(); i++)
			{
				if (path[i] == '/')
				{
					projectNameEnd = i;
					break;
				}
			}
			if (projectNameEnd == -1)
				return String::Empty; // Invalid project path
			StringView projectName = path.Substring(2, projectNameEnd - 2);
			if (projectName.StartsWith(SE_TEXT("Shaders")))
			{
				// Hard-coded redirect to platform-specific includes
				result = EngineContext::StartupFolder / SE_TEXT("Shaders");
			}
			else
			{
/*				HashSet<const ProjectInfo*> projects;
				const ProjectInfo* project = FindProjectByName(Editor::Project, projects, StringView(projectName.Get(), projectNameEnd - 2));
				if (project)
					result = project->ProjectFolderPath / SE_TEXT("/Source/Shaders/");
				else
					return String::Empty;*/
			}
			result /= path.Substring(projectNameEnd + 1);
		}
#endif
		{
			// 绝对路径
			result = path;
		}

		return result;
	}

	String ShadersCompilation::CompactShaderPath(StringView path)
	{
#if SE_EDITOR
		// Try to use file path relative to the project shader sources folder
		/*HashSet<const ProjectInfo*> projects;
		const ProjectInfo* project = FindProjectByPath(Editor::Project, projects, path);
		if (project)
		{
			String projectSourcesPath = project->ProjectFolderPath / SE_TEXT("/Source/Shaders/");
			if (path.StartsWith(projectSourcesPath))
				return String::Format(SE_TEXT("./{}/{}"), project->Name, path.Substring(projectSourcesPath.Length()));
		}*/
#endif
		return String(path);
	}

#if SE_EDITOR

	namespace
	{
		List<FileSystemWatcher*> ShadersSourcesWatchers;

		// Tries to generate a stable and unique ID for the given shader name.
		// Used in order to keep the same shader IDs and reduce version control issues with binary diff on ID.
		UID GetShaderAssetId(const String& name)
		{
			UID result;
			result.A = name.Length() * 100;
			result.B = GetHash(name);
			result.C = name.HasChars() ? name[0] : 0;
			result.D = name.HasChars() ? name[name.Length() - 1] : 0;
			return result;
		}

		void OnWatcherShadersEvent(FileWatcherEvent& event)
		{
			if (event.action == FileSystemAction::Delete || !event.path.EndsWith(SE_TEXT(".shader")))
				return;

			LOG_INFO("ShaderCompiler", "Shader \'{0}\' has been modified.", event.path);

			// 稍等片刻，以便正在编辑文件的编辑器（例如 Visual Studio）有足够的时间刷新整个文件更改
			Platform::Sleep(100);

			// Perform hot reload
			const int32 srcSubDirStart = event.path.FindLast(SE_TEXT("/Source/Shaders"));
			if (srcSubDirStart == -1)
				return;
			String projectFolderPath = event.path.Substring(0, srcSubDirStart);
			FileSystem::NormalizePath(projectFolderPath);
			const String shadersAssetsPath = projectFolderPath / SE_TEXT("/Content/Shaders");
			const String shadersSourcePath = projectFolderPath / SE_TEXT("/Source/Shaders");
			const String localPath = FileSystem::ConvertAbsolutePathToRelative(shadersSourcePath, event.path);
			const String name = FileSystem::GetPathWithoutExtension(localPath);
			const String outputPath = shadersAssetsPath / name;// + ASSET_FILES_EXTENSION_WITH_DOT;
			UID id = GetShaderAssetId(name);
//			AssetsImportingManager::ImportIfEdited(path, outputPath, id);
		}

		void RegisterShaderWatchers(const ProjectInfo* project, HashSet<const ProjectInfo*>& projects)
		{
			if (projects.Contains(project))
				return;
			projects.Add(project);

			// Check if project uses shaders sources
			const String shadersSourcePath = project->ProjectFolderPath / SE_TEXT("/Source/Shaders");
			if (FileSystem::DirectoryExists(shadersSourcePath))
			{
				// Track engine shaders editing
				auto sourceWatcher = New<FileSystemWatcher>(shadersSourcePath, true);
				sourceWatcher->OnEvent.Bind<OnWatcherShadersEvent>();
				ShadersSourcesWatchers.Add(sourceWatcher);

				// Reimport modified or import added shaders
				List<String> files(64);
				const String shadersAssetsPath = project->ProjectFolderPath / SE_TEXT("/Content/Shaders");
				FileSystem::DirectoryGetFiles(files, shadersSourcePath, SE_TEXT("*.shader"), DirectorySearchOption::All);
				for (int32 i = 0; i < files.Count(); i++)
				{
					const String& path = files[i];
					const String localPath = FileSystem::ConvertAbsolutePathToRelative(shadersSourcePath, path);
					const String name = FileSystem::GetPathWithoutExtension(localPath);
					const String outputPath = shadersAssetsPath / name;// + ASSET_FILES_EXTENSION_WITH_DOT;
					UID id = GetShaderAssetId(name);
//					AssetsImportingManager::ImportIfEdited(path, outputPath, id);
				}
			}

			// Initialize referenced projects
			for (const auto& reference : project->References)
			{
				if (reference.Project)
					RegisterShaderWatchers(reference.Project, projects);
			}
		}
	}

#endif

	bool ShadersCompilationSystem::OnInit()
	{
#if SE_EDITOR

		// Initialize automatic shaders importing and reloading for all loaded projects (game, engine, plugins)
		HashSet<const ProjectInfo*> projects;
//		RegisterShaderWatchers(Editor::Project, projects);

		//
		List<String> shaderFileList;
		List<String> shaderDirectoryList;

		String nativeShaderDirectory = String::Format(SE_TEXT("{0}/Shaders"), EngineContext::StartupFolder);
		FileSystem::GetChildDirectories(shaderDirectoryList, nativeShaderDirectory);
		FileSystem::DirectoryGetFiles(shaderFileList, nativeShaderDirectory, SE_TEXT("*.shader"), DirectorySearchOption::TopOnly);

		for (String& shaderDirectory : shaderDirectoryList)
		{
			if (!shaderDirectory.Contains(SE_TEXT("Template")))
			{
				FileSystem::DirectoryGetFiles(shaderFileList, shaderDirectory, SE_TEXT("*.shader"), DirectorySearchOption::All);
			}
		}

		MemoryWriteStream writeStream;
		StringBuilder outPutPath;

		for (String& shaderFile : shaderFileList)
		{
			FileSystem::NormalizePath(shaderFile);
			String fileName = FileSystem::GetFileNameWithoutExtension(shaderFile);

			outPutPath.Clear();
			outPutPath.Append(EngineContext::ProjectCacheFolder);
			outPutPath.Append("/Shaders/");
			outPutPath.Append(fileName);
			outPutPath.Append(".");
			outPutPath.Append(ASSET_FILES_EXTENSION);

			AssetsImporting::ImportIfEdited(shaderFile, outPutPath.ToString());
		}


#endif

		return true;
	}

	void ShadersCompilationSystem::OnDispose()
	{
#if SE_EDITOR
		ShadersSourcesWatchers.ClearDelete();
#endif

		Locker.Lock();

		// Check if any compilation is running
		if (ReadyCompilers.Count() != Compilers.Count())
		{
			LOG_ERROR("ShaderCompiler", "Cannot dispose Shaders Compilation Service. One or more compilers are still in use.");
		}

		// Cleanup all compilers (delete only those which are not in use)
		ReadyCompilers.ClearDelete();
		Compilers.Clear();

		Locker.Unlock();

		// Cleanup shader includes
		ShaderCompiler::DisposeIncludedFilesCache();

		// Clear includes scanning
		ShaderIncludesMapLocker.Lock();
		ShaderIncludesMap.Clear();
		ShaderIncludesWatcher.ClearDelete();
		ShaderIncludesMapLocker.Unlock();
	}

	void ShaderCompilationContext::OnError(const char* message, const char* file, int line)
	{
		LOG_ERROR_LINE("ShaderCompiler", file, line, "Failed to compile '{0}'. {1}", Options->TargetName, StringAnsiView(message));
	}

	void ShaderCompilationContext::OnCollectDebugInfo(ShaderFunctionMeta& meta, int32 permutationIndex, const char* data, const int32 dataLength)
	{
#ifdef GPU_USE_SHADERS_DEBUG_LAYER
		// Cache data
		meta.Permutations[permutationIndex].DebugData.Set(data, dataLength);
#endif
	}

	ShaderCompilationContext::ShaderCompilationContext(const ShaderCompilationOptions* options, ShaderMeta* meta)
		: Options(options), Meta(meta), Output(options->Output)
	{
		// Convert target name to ANSI SE_TEXT (with limited length)
		const int32 ansiNameLen = Math::Min<int32>(ARRAY_SIZE(TargetNameAnsi) - 1, options->TargetName.Length());
		StringUtils::ConvertUTF162ANSI(*options->TargetName, TargetNameAnsi, ansiNameLen);
		TargetNameAnsi[ansiNameLen] = 0;

	}
}
