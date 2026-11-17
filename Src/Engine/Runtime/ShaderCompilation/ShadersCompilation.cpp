#include <Runtime/ShaderCompilation/ShadersCompilation.h>

#include <Runtime/Core/Platform/FileSystem.h>
#include <Runtime/Core/Platform/FileSystemWatcher.h>
#include <Runtime/Core/Profiler/Profiler.h>
#include <Runtime/Core/Systems.h>
#include <Runtime/Core/Thread/Threading.h>
#include <Runtime/Core/Types/Collections/Dictionary.h>
#include <Runtime/Core/Types/Strings/String.h>

#include <Runtime/EngineContext.h>
#include <Runtime/Resource/AssetContent.h>
#include <Runtime/Resource/Assets/Materials/Material.h>
#include <Runtime/Resource/Assets/Materials/Shader.h>
#include <Runtime/Resource/Importers/AssetsImportingSystem.h>
#include <Runtime/ShaderCompilation/Slang/SlangShaderCompiler.h>

namespace SE
{
	namespace ShadersCompilationImpl
	{
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

	ShaderCompileResult ShadersCompilation::CompileSlang(const ShaderCompileRequest& request)
	{
		PROFILE_CPU_NAMED("Shader.CompileSlang");

		SlangShaderCompiler compiler;
		ShaderCompileResult result = compiler.Compile(request);
		if (result.Success)
		{
			LOG_INFO("ShaderCompiler", "Slang shader compilation '{0}' succeed.", request.ShaderName);
			if (!result.CompileMessage.IsEmpty())
			{
				LOG_WARNING("ShaderCompiler", "{0}", result.CompileMessage);
			}
		}
		else
		{
			LOG_ERROR("ShaderCompiler", "Slang shader compilation '{0}' failed. {1}", request.ShaderName, result.CompileMessage);
		}
		return result;
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

	bool ShadersCompilationSystem::OnInit()
	{
#if SE_EDITOR

		List<String> shaderFileList;

		String nativeShaderDirectory = String::Format(SE_TEXT("{0}/Shaders"), EngineContext::StartupFolder);
		FileSystem::DirectoryGetFiles(shaderFileList, nativeShaderDirectory, SE_TEXT("*.shader"), DirectorySearchOption::All);
		FileSystem::DirectoryGetFiles(shaderFileList, nativeShaderDirectory, SE_TEXT("*.slang"), DirectorySearchOption::All);

		StringBuilder outPutPath;

		for (String& shaderFile : shaderFileList)
		{
			FileSystem::NormalizePath(shaderFile);
            String relativePath = FileSystem::ConvertAbsolutePathToRelative(nativeShaderDirectory, shaderFile);
            if (relativePath.Contains(SE_TEXT("MaterialTemplates")))
            {
                continue;
            }
            String parentPath   = FileSystem::GetParentDirectory(relativePath);
            String fileName     = FileSystem::GetFileNameWithoutExtension(relativePath);

			outPutPath.Clear();
			outPutPath.Append(EngineContext::ProjectCacheFolder);
			outPutPath.Append("/Shaders/");

            if (!parentPath.IsEmpty())
            {
                outPutPath.Append(parentPath);
                outPutPath.Append("/");
            }

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
		// Clear includes scanning
		ShaderIncludesMapLocker.Lock();
		ShaderIncludesMap.Clear();
		ShaderIncludesWatcher.ClearDelete();
		ShaderIncludesMapLocker.Unlock();
	}

}
