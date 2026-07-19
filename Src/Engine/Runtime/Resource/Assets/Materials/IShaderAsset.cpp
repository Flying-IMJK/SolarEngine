

#include "IShaderAsset.h"

#include "ShaderCacheManager.h"
#include "Runtime/Core/Serialization/MemoryWriteStream.h"
#include "Runtime/Core/Platform/FileSystem.h"
#include "Runtime/Core/Serialization/MemoryReadStream.h"
#include "Runtime/Core/Encoding.h"
#include "Runtime/Graphics/GPUDevice.h"
#include "Runtime/ShaderCompilation/ShadersCompilation.h"
#include "Runtime/Graphics/Shaders/GPUShader.h"
#if SE_EDITOR
#include "Runtime/ShaderCompilation/ShadersCompilation.h"
#endif

namespace SE
{
    ShaderStorage::CachingMode ShaderStorage::CurrentCachingMode =
#if SE_EDITOR
        CachingMode::ProjectCache;
#else
        CachingMode::AssetInternal;
#endif
    const int32 ShaderStorage::MagicCode = -842185133;

    ShaderStorage::CachingMode ShaderStorage::GetCachingMode()
    {
#if !COMPILE_WITH_SHADER_CACHE_MANAGER
        if (CurrentCachingMode == CachingMode::ProjectCache)
            return CachingMode::AssetInternal;
#endif
        return CurrentCachingMode;
    }


    bool IShaderAsset::IsNullRenderer()
    {
        return false;// GPUDevice::instance->GetRendererType() == RendererType::Null;
    }

    int32 IShaderAsset::GetCacheChunkIndex()
    {
        return GetCacheChunkIndex(GPUDevice::instance->GetShaderProfile());
    }

    int32 IShaderAsset::GetCacheChunkIndex(ShaderProfile profile)
    {
        int32 result;
        switch (profile)
        {
        case ShaderProfile::DirectX_SM6:
            result = SHADER_FILE_CHUNK_INTERNAL_D3D_SM6_CACHE;
            break;
        case ShaderProfile::DirectX_SM5:
            result = SHADER_FILE_CHUNK_INTERNAL_D3D_SM5_CACHE;
            break;
        case ShaderProfile::DirectX_SM4:
            result = SHADER_FILE_CHUNK_INTERNAL_D3D_SM4_CACHE;
            break;
        case ShaderProfile::GLSL_410:
            result = SHADER_FILE_CHUNK_INTERNAL_GLSL_410_CACHE;
            break;
        case ShaderProfile::GLSL_440:
            result = SHADER_FILE_CHUNK_INTERNAL_GLSL_440_CACHE;
            break;
        case ShaderProfile::Vulkan_SM5:
            result = SHADER_FILE_CHUNK_INTERNAL_VULKAN_SM5_CACHE;
            break;
        default:
            result = SHADER_FILE_CHUNK_INTERNAL_GENERIC_CACHE;
            break;
        }
        return result;
    }


#if SE_EDITOR

    bool IsValidShaderCache(DataContainer<byte>& shaderCache, List<String>& includes)
    {
        if (shaderCache.Length() == 0)
            return false;
        MemoryReadStream stream(shaderCache.Get(), shaderCache.Length());

        // Read cache format version
        int32 version;
        stream.ReadInt32(&version);
        if (version != GPU_SHADER_CACHE_VERSION)
            return false;

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
            include  = ShadersCompilation::ResolveShaderPath(include);
            DateTime lastEditTime;
            stream.Read(lastEditTime);

            // Check if included file exists locally and has been modified since last compilation
            if (!(FileSystem::FileExists(include) && FileSystem::GetFileLastEditTime(include) > lastEditTime))
            {
                return false;
            }
        }

        return true;
    }

#endif

    bool IShaderAsset::LoadShaderCache(ShaderCacheResult& result)
    {
        // Prepare
        auto parent = GetShaderAsset();
        const ShaderProfile shaderProfile = GPUDevice::instance->GetShaderProfile();
        const int32 cacheChunkIndex = GetCacheChunkIndex(shaderProfile);
        const auto cachingMode = ShaderStorage::GetCachingMode();
        ShaderCacheManager::CachedEntryHandle cachedEntry;

#if SE_EDITOR
        // Try to get cached shader (based on current caching policy)
        bool hasCache = false;
        if (cachingMode == ShaderStorage::CachingMode::AssetInternal)
        {
            if (parent->HasChunkLoaded(cacheChunkIndex))
            {
                // Load cached shader data, create reading stream (memory free, super fast, super easy, no overhead at all)
                const auto cacheChunk = parent->GetChunk(cacheChunkIndex);
                result.Data.Link(cacheChunk->Data);
                if (IsValidShaderCache(result.Data, result.Includes))
                {
                    hasCache = true;
                }
                else
                {
                    result.Data.Release();
                }
            }
        }
        else if (cachingMode == ShaderStorage::CachingMode::ProjectCache)
        {
            // Try get cached shader cache
            const DateTime assetModificationDate = parent->storage ? FileSystem::GetFileLastEditTime(parent->storage->GetPath()) : DateTime::MinValue();
            if (ShaderCacheManager::TryGetEntry(shaderProfile, parent->GetID(), cachedEntry)
                && cachedEntry.GetModificationDate() > assetModificationDate
                && !ShaderCacheManager::GetCache(shaderProfile, cachedEntry, result.Data)
                && IsValidShaderCache(result.Data, result.Includes))
            {
                hasCache = true;
            }
            else
            {
                result.Data.Release();
            }
        }

        // Check if shader should be compiled
#if BUILD_DEBUG
        const bool forceRecompile = false;
#else
        const bool forceRecompile = false;
#endif
        if ((forceRecompile || GPU_FORCE_RECOMPILE_SHADERS || !hasCache)
            && parent->HasChunk(SHADER_FILE_CHUNK_SOURCE))
        {
            result.Data.Release();
            const String parentPath = parent->GetPath();
            const UID parentID = parent->GetID();
            LOG_INFO("Resource", "Compiling shader '{0}':{1}...", parentPath, parentID);

            // Load all chunks (except that with internal cache for current shader profile, it will be generated)
            AssetChunksFlag chunksToLoad = ALL_ASSET_CHUNKS;
            chunksToLoad &= ~GET_CHUNK_FLAG(cacheChunkIndex);
            if (!parent->LoadChunks(chunksToLoad))
            {
                LOG_WARNING("Resource", "Cannot load '{0}' data from chunk {1}.", parent->ToString(), chunksToLoad);
                return false;
            }

            // Remove current profile internal chunk (it could be loaded by the asset itself during precaching phrase)
            parent->ReleaseChunk(cacheChunkIndex);

            // Decrypt source code
            auto sourceChunk = parent->GetChunk(SHADER_FILE_CHUNK_SOURCE);
            auto source = sourceChunk->Get<char>();
            auto sourceLength = sourceChunk->Size();
            Encoding::DecryptBytes((byte*)source, sourceLength);
            source[sourceLength - 1] = 0;

            // Init shader cache output stream
            // TODO: use cached data per thread?
            MemoryWriteStream cacheStream(32 * 1024);

            // Compile shader source
            ShaderCompilationOptions options;
            options.TargetName = FileSystem::GetFileNameWithoutExtension(parentPath);
            options.TargetID = parentID;
            options.Source = source;
            options.SourceLength = sourceLength;
            options.Profile = shaderProfile;
            options.Output = &cacheStream;
            options.GenerateDebugData = true;
            options.NoOptimize = true;
            options.ForceStoreCompilationSource = true;
            options.debugInfo = true;

            auto& platformDefine = options.Macros.AddOne();
#if PLATFORM_WINDOWS
            platformDefine.Name = "PLATFORM_WINDOWS";
#elif PLATFORM_LINUX
            platformDefine.Name = "PLATFORM_LINUX";
#elif PLATFORM_MAC
            platformDefine.Name = "PLATFORM_MAC";
#else
            #error "Unknown platform."
#endif
            platformDefine.Definition = "1";
#if SE_EDITOR
            auto& editorDefine = options.Macros.AddOne();
            editorDefine.Name = "SE_EDITOR";
            editorDefine.Definition = "1";
#endif

            InitCompilationOptions(options);
            const bool success = ShadersCompilation::Compile(options);

            // Encrypt source code
            Encoding::EncryptBytes(reinterpret_cast<byte*>(source), sourceLength);

            if (!success)
            {
                LOG_ERROR("Resource", "Failed to compile shader '{0}'", parent->ToString());
                return false;
            }
            LOG_INFO("Resource", "Shader '{0}' compiled! Cache size: {1} bytes", parent->ToString(), cacheStream.GetPosition());

            // Save compilation result (based on current caching policy)
            if (cachingMode == ShaderStorage::CachingMode::AssetInternal)
            {
                // Save cache to the internal shader cache chunk
                auto cacheChunk = parent->GetOrCreateChunk(cacheChunkIndex);
                if (cacheChunk == nullptr)
                {
                    return false;
                }
                cacheChunk->Data.Copy(cacheStream.GetHandle(), cacheStream.GetPosition());

#if SE_EDITOR
                // Save chunks to the asset file
                if (!Save())
                {
                    LOG_WARNING("Resource", "Cannot save '{0}'.", parent->ToString());
                    return false;
                }

                // Save 会导致chunk 数据被释放需要重新加载
                List<StorageChunk*> chunks;
                parent->storage->GetChunks(chunks);
                for (int i = 0; i < chunks.Count(); i++)
                {
                    parent->storage->LoadAssetChunk(chunks[i]);
                }
#endif
            }
            else if (cachingMode == ShaderStorage::CachingMode::ProjectCache)
            {
                // Save results to cache
                if (ShaderCacheManager::SetCache(shaderProfile, cachedEntry, cacheStream))
                {
                    LOG_WARNING("Resource", "Cannot save shader cache.");
                    return false;
                }
            }
            else
            {
                // Use temporary generated data without caching (but read the includes from cache)
                result.Data.Copy(cacheStream.GetHandle(), cacheStream.GetLength());
                IsValidShaderCache(result.Data, result.Includes);
                return true;
            }
        }
        else if (hasCache && result.Data.IsValid())
        {
            // The cached version is valid
            return true;
        }

#endif

        // Check if has internal shader cache
        if (parent->HasChunkLoaded(cacheChunkIndex))
        {
            // Load cached shader data, create reading stream (memory free, super fast, super easy, no overhead at all)
            const auto cacheChunk = parent->GetChunk(cacheChunkIndex);
            result.Data.Link(cacheChunk->Data);
        }
        // Check if has cached shader
        else if (cachedEntry.IsValid() || ShaderCacheManager::TryGetEntry(shaderProfile, parent->GetID(), cachedEntry))
        {
            // Load results from cache
            if (ShaderCacheManager::GetCache(shaderProfile, cachedEntry, result.Data))
            {
                LOG_WARNING("Resource", "Cannot load shader cache.");
                return false;
            }
        }
        else
        {
            LOG_WARNING("Resource", "Missing shader cache '{0}'.", parent->ToString());
            return false;
        }

        ENGINE_ASSERT(result.Data.IsValid());

#if SE_EDITOR
        // Read includes from cache
        IsValidShaderCache(result.Data, result.Includes);
#endif

        return true;
    }

    void IShaderAsset::RegisterForShaderReloads(Asset* asset, const ShaderCacheResult& shaderCache)
    {
        for (auto& include : shaderCache.Includes)
        {
            ShadersCompilation::RegisterForShaderReloads(asset, include);
        }
    }

    void IShaderAsset::UnregisterForShaderReloads(Asset* asset)
    {
        ShadersCompilation::UnregisterForShaderReloads(asset);
    }

} // SE