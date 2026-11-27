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
#include <Runtime/Resource/Storage/AssetStorages.h>
#include <Runtime/ShaderCompilation/Slang/SlangShaderCompiler.h>

namespace SE
{
	namespace ShadersCompilationImpl
	{
		CriticalSection ShaderIncludesMapLocker;
        Dictionary<String, List<UID>> ShaderIncludesMap;
        Dictionary<String, FileSystemWatcher*> ShaderIncludesWatcher;
        struct PendingReload
        {
            List<String> Paths;
            double LastChange;
        };
        Dictionary<UID, PendingReload> PendingReloads;
        bool Stopping = false;

        String WatchedPath(String path)
        {
            FileSystem::NormalizePath(path);
#if PLATFORM_WINDOWS
            path.ToLower();
#endif
            return path;
        }

        void OnShaderIncludesWatcherEvent(FileWatcherEvent& event)
        {
            if (event.action != FileSystemAction::Modify && event.action != FileSystemAction::Create &&
                event.action != FileSystemAction::Rename)
                return;

            const String path = WatchedPath(event.path);
            Threading::ScopeLock lock(ShaderIncludesMapLocker);
            const auto* assets = ShaderIncludesMap.TryGet(path);
            if (!assets)
                return;
            const double now = Platform::GetTimeSeconds();
            for (const UID& id : *assets)
            {
                auto& pending = PendingReloads[id];
                pending.Paths.AddUnique(path);
                pending.LastChange = now;
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
        void OnUpdate() override;
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
        const String path = WatchedPath(includedPath);
        Threading::ScopeLock lock(ShaderIncludesMapLocker);
        if (Stopping)
            return;

        ShaderIncludesMap[path].AddUnique(asset->GetID());
        String directory = FileSystem::GetParentDirectory(path);
#if PLATFORM_WINDOWS
        if (directory.Length() == 2 && directory[1] == ':')
            directory += SE_TEXT("/");
#endif
        if (!ShaderIncludesWatcher.ContainsKey(directory) && FileSystem::DirectoryExists(directory))
        {
            auto watcher = New<FileSystemWatcher>(directory, false);
            watcher->OnEvent.Bind<OnShaderIncludesWatcherEvent>();
            ShaderIncludesWatcher.Add(directory, watcher);
        }
    }

    void ShadersCompilation::UnregisterForShaderReloads(Asset* asset)
    {
        Threading::ScopeLock lock(ShaderIncludesMapLocker);
        for (auto it = ShaderIncludesMap.begin(); it != ShaderIncludesMap.end(); ++it)
        {
            if (it->Value.Remove(asset->GetID()) && it->Value.IsEmpty())
                ShaderIncludesMap.Remove(it);
        }
    }

    void ShadersCompilationSystem::OnUpdate()
    {
        const double settleBefore = Platform::GetTimeSeconds() - 0.1;
        List<UID> ready;
        {
            Threading::ScopeLock lock(ShaderIncludesMapLocker);
            for (const auto& change : PendingReloads)
            {
                if (change.Value.LastChange <= settleBefore)
                {
                    ready.Add(change.Key);
                }
            }
        }

        List<UID> reloaded;
        for (const UID& id : ready)
        {
            Asset* asset = AssetContent::GetAsset(id);
            if (!asset->IsLoaded())
            {
                continue;
            }

            // Re-check the paths: a load may have replaced the include set since the notification.
            bool affected = false;
            {
                Threading::ScopeLock lock(ShaderIncludesMapLocker);
                const auto* pending = PendingReloads.TryGet(id);
                if (!pending || pending->LastChange > settleBefore)
                {
                    continue;
                }
                for (const String& path : pending->Paths)
                {
                    const auto* assets = ShaderIncludesMap.TryGet(path);
                    if (assets && assets->Contains(id))
                    {
                        affected = true;
                        break;
                    }
                }
                PendingReloads.Remove(id);
            }
            if (!affected || !asset)
            {
                continue;
            }
            reloaded.Add(id);
            asset->Reload();
        }

        if (reloaded.IsEmpty())
        {
            return;
        }

        // Only query registered materials when a live asset has failed to load.
        const ContentStats stats = AssetContent::GetStats();
        if (stats.LoadedAssetsCount + stats.LoadingAssetsCount == stats.AssetsCount)
        {
            return;
        }

        for (const UID& id : AssetContent::GetAllAssetsByType(Typeof<Material>()))
        {
            Asset* asset = AssetContent::GetAsset(id);
            if (asset && asset->LastLoadFailed() && !reloaded.Contains(id))
            {
                asset->Reload();
            }
        }
    }

	bool ShadersCompilationSystem::OnInit()
	{
        Stopping = false;
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

            const String assetPath = outPutPath.ToString();
            bool         reimport  = false;
            if (FileSystem::FileExists(assetPath))
            {
                // Source timestamps cannot detect a shared asset-header format change.
                const StorageReference storage = AssetStorages::GetStorage(assetPath);
                AssetInitData          data;
                if (!storage || storage->GetEntriesCount() != 1 || !storage->LoadAssetHeader(0, data))
                {
                    LOG_ERROR("Shader", "Cannot read Shader asset header for '{0}'.", assetPath);
                    return false;
                }
                reimport = data.SerializedVersion != ShaderStorage::Header::Version || data.CustomData.Length() != sizeof(ShaderStorage::Header);
            }

            if (reimport)
            {
                LOG_INFO("Shader", "Reimporting '{0}' for shared Shader header version {1}.", assetPath, ShaderStorage::Header::Version);
                if (!AssetsImporting::Import(shaderFile, assetPath, nullptr))
                {
                    LOG_ERROR("Shader", "Cannot reimport Shader '{0}'.", shaderFile);
                    return false;
                }
            }
            else
            {
                // ImportIfEdited also returns false when the source is already up to date.
                AssetsImporting::ImportIfEdited(shaderFile, assetPath);
            }
        }


#endif

		return true;
	}

	void ShadersCompilationSystem::OnDispose()
	{
        Dictionary<String, FileSystemWatcher*> watchers;
        {
            Threading::ScopeLock lock(ShaderIncludesMapLocker);
            Stopping = true;
            PendingReloads.Clear();
            ShaderIncludesMap.Clear();
            watchers.Swap(ShaderIncludesWatcher);
        }
        watchers.ClearDelete();
	}

}
