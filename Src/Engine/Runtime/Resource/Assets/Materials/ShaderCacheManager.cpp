
#include "ShaderCacheManager.h"

#include "Core/Systems.h"
#include "Core/Platform/File.h"
#include "Core/Platform/FileSystem.h"
#include "Runtime/Graphics/Shaders/GPUShader.h"
#include "Runtime/EngineContext.h"
#include "Runtime/Render/Assets/Material/MaterialShader.h"

namespace SE
{
    const Char* ShaderProfileCacheDirNames[] =
{
        // @formatter:off
        SE_TEXT(""), // Unknown
        SE_TEXT("DX_SM4"), // DirectX_SM4
        SE_TEXT("DX_SM5"), // DirectX_SM5
        SE_TEXT("GLSL_410"), // GLSL_410
        SE_TEXT("GLSL_440"), // GLSL_440
        SE_TEXT("VK_SM5"), // Vulkan_SM5
        SE_TEXT("PS4"), // PS4
        SE_TEXT("DX_SM6"), // DirectX_SM6
        SE_TEXT("PS5"), // PS5
        // @formatter:on
    };

    static_assert(ARRAY_SIZE(ShaderProfileCacheDirNames) == (int32)ShaderProfile::MAX, "Invalid shaders cache dirs");

    class ShaderProfileDatabase
    {
    public:
        ShaderProfile Profile;
        String Folder;

    public:
        void Init(ShaderProfile profile, const String& cacheRoot)
        {
            Profile = profile;
            Folder = cacheRoot / ShaderProfileCacheDirNames[static_cast<int32>(profile)];
            if (!FileSystem::DirectoryExists(Folder))
            {
                if (FileSystem::CreateDirectory(Folder))
                {
                    LOG_WARNING("Resource", "Cannot create cache directory for ShaderProfileDatabase: {0} (path: \'{1}\')", ToString(Profile), Folder);
                }
            }
        }

        bool TryGetEntry(const UID& id, ShaderCacheManager::CachedEntryHandle& cachedEntry) const
        {
            ENGINE_ASSERT(id.IsValid());

            cachedEntry.ID = id;
            cachedEntry.Path = Folder / id.ToString(UID::FormatType::D);
            return cachedEntry.Exists();
        }

        bool GetCache(const ShaderCacheManager::CachedEntryHandle& cachedEntry, BytesContainer& outputShaderCache) const
        {
            ENGINE_ASSERT(cachedEntry.IsValid());
            return File::ReadAllBytes(cachedEntry.Path, outputShaderCache);
        }

        static bool SetCache(const ShaderCacheManager::CachedEntryHandle& cachedEntry, MemoryWriteStream& inputShaderCache)
        {
            ENGINE_ASSERT(cachedEntry.IsValid() && inputShaderCache.GetHandle() != nullptr);
            return inputShaderCache.SaveToFile(cachedEntry.Path);
        }

        void RemoveCache(const UID& id) const
        {
            ENGINE_ASSERT(id.IsValid());
            ShaderCacheManager::CachedEntryHandle cachedEntry;
            cachedEntry.ID = id;
            cachedEntry.Path = Folder / id.ToString(UID::FormatType::D);
            FileSystem::DeleteFile(cachedEntry.Path);
        }
    };

    class ShaderCacheSystem : public ISystem
    {
        ENGINE_SYSTEM(ShaderCacheSystem)
    public:
        ShaderCacheSystem() : ISystem(SE_TEXT("Shader Cache Manager"), -200)
        {
        }

        bool OnInit() override;
    };

    ENGINE_SYSTEM_REGISTER(ShaderCacheSystem)

    ShaderProfileDatabase Databases[(int32)ShaderProfile::MAX - 1];

    static int32 shaderProfile2Index(const ShaderProfile profile)
    {
        return static_cast<int32>(profile) - 1;
    }

    static ShaderProfile index2ShaderProfile(const int32 index)
    {
        return static_cast<ShaderProfile>(index + 1);
    }

    bool ShaderCacheManager::CachedEntryHandle::IsValid() const
    {
        return ID.IsValid();
    }

    bool ShaderCacheManager::CachedEntryHandle::Exists() const
    {
        return FileSystem::FileExists(Path);
    }

    DateTime ShaderCacheManager::CachedEntryHandle::GetModificationDate() const
    {
        return FileSystem::GetFileLastEditTime(Path);
    }

    bool ShaderCacheManager::TryGetEntry(const ShaderProfile profile, const UID& id, CachedEntryHandle& cachedEntry)
    {
        ENGINE_ASSERT(profile != ShaderProfile::Unknown);
        return Databases[shaderProfile2Index(profile)].TryGetEntry(id, cachedEntry);
    }

    bool ShaderCacheManager::GetCache(const ShaderProfile profile, const CachedEntryHandle& cachedEntry, BytesContainer& outputShaderCache)
    {
        ENGINE_ASSERT(profile != ShaderProfile::Unknown);
        return Databases[shaderProfile2Index(profile)].GetCache(cachedEntry, outputShaderCache);
    }

    bool ShaderCacheManager::SetCache(const ShaderProfile profile, const CachedEntryHandle& cachedEntry, MemoryWriteStream& inputShaderCache)
    {
        ENGINE_ASSERT(profile != ShaderProfile::Unknown);
        return Databases[shaderProfile2Index(profile)].SetCache(cachedEntry, inputShaderCache);
    }

    void ShaderCacheManager::RemoveCache(const ShaderProfile profile, const UID& id)
    {
        ENGINE_ASSERT(profile != ShaderProfile::Unknown);
        Databases[shaderProfile2Index(profile)].RemoveCache(id);
    }

    void ShaderCacheManager::RemoveCache(const UID& id)
    {
        for (int32 i = 0; i < ARRAY_SIZE(Databases); i++)
            Databases[i].RemoveCache(id);
    }

    void ShaderCacheManager::CopyCache(const UID& dstId, const UID& srcId)
    {
        ENGINE_ASSERT(dstId.IsValid() && srcId.IsValid());

        String dstFilename = dstId.ToString(UID::FormatType::D);
        String srcFilename = srcId.ToString(UID::FormatType::D);
        for (int32 i = 0; i < ARRAY_SIZE(Databases); i++)
        {
            const String& folder = Databases[i].Folder;
            String dstPath = folder / dstFilename;
            String srcPath = folder / srcFilename;

            if (FileSystem::FileExists(srcPath))
                FileSystem::CopyFile(dstPath, srcPath);
            else if (FileSystem::FileExists(dstPath))
                FileSystem::DeleteFile(dstPath);
        }
    }

    bool ShaderCacheSystem::OnInit()
    {
#if SE_EDITOR
        const String rootDir = EngineContext::ProjectCacheFolder / SE_TEXT("Shaders/Cache");
#else
        const String rootDir = EngineContext::ProductLocalFolder / SE_TEXT("Shaders/Cache");
#endif

        // Validate the database cache version (need to recompile all shaders on shader cache format change)
        struct CacheVersion
        {
            int32 EngineVersion = -1;
            int32 ShaderCacheVersion = -1;
            int32 MaterialGraphVersion = -1;
            int32 ParticleGraphVersion = -1;
        };
        CacheVersion cacheVersion;
        const String cacheVerFile = rootDir / SE_TEXT("CacheVersion");
        if (FileSystem::FileExists(cacheVerFile))
        {
            if (File::ReadAllBytes(cacheVerFile, (byte*)&cacheVersion, sizeof(cacheVersion)))
            {
                cacheVersion = CacheVersion();
                LOG_WARNING("Resource", "Failed to read the shaders cache database version file.");
            }
        }
        if (/*cacheVersion.EngineVersion != ENGINE_V FLAXENGINE_VERSION_BUILD
            || */cacheVersion.ShaderCacheVersion != GPU_SHADER_CACHE_VERSION
            || cacheVersion.MaterialGraphVersion != MATERIAL_GRAPH_VERSION
            /*|| cacheVersion.ParticleGraphVersion != PARTICLE_GPU_GRAPH_VERSION*/
        )
        {
            LOG_WARNING("Resource", "Shaders cache database is invalid. Performing reset.");

            if (FileSystem::DirectoryExists(rootDir) && FileSystem::DeleteDirectory(rootDir))
            {
                LOG_WARNING("Resource", "Failed to reset the shaders cache database.");
            }
            if (FileSystem::CreateDirectory(rootDir))
            {
                LOG_ERROR("Resource", "Failed to createe the shaders cache database directory.");
            }

            // cacheVersion.EngineVersion = FLAXENGINE_VERSION_BUILD;
            cacheVersion.ShaderCacheVersion = GPU_SHADER_CACHE_VERSION;
            cacheVersion.MaterialGraphVersion = MATERIAL_GRAPH_VERSION;
            // cacheVersion.ParticleGraphVersion = PARTICLE_GPU_GRAPH_VERSION;
            if (File::WriteAllBytes(cacheVerFile, (byte*)&cacheVersion, sizeof(cacheVersion)))
            {
                LOG_ERROR("Resource", "Failed to create the shaders cache database version file.");
            }
        }

        // Init shader formats databases
        for (int32 i = 0; i < ARRAY_SIZE(Databases); i++)
        {
            Databases[i].Init(index2ShaderProfile(i), rootDir);
        }

        return true;
    }
} // SE