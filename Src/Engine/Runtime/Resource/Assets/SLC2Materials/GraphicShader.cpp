#include <Runtime/Resource/Assets/SLC2Materials/GraphicShader.h>

#include <Runtime/Core/Platform/File.h>
#include <Runtime/Core/Platform/FileSystem.h>
#include <Runtime/Core/Serialization/MemoryWriteStream.h>
#include <Runtime/Core/Thread/Threading.h>
#include <Runtime/Graphics/GPUDevice.h>
#include <Runtime/Graphics/Shaders/Reflection/ShaderProgramReflection.h>
#include <Runtime/Resource/Assets/Materials/ShaderCacheManager.h>
#include <Runtime/Resource/Assets/Materials/ShaderStorage.h>
#include <Runtime/Resource/Assets/SLC2Materials/SLC2MaterialBinding.h>
#include <Runtime/Resource/Assets/SLC2Materials/SLC2MaterialPipeline.h>
#include <Runtime/Resource/Factories/BinaryAssetFactory.h>
#include <Runtime/Resource/Storage/StorageChunk.h>
#include <Runtime/ShaderCompilation/ShadersCompilation.h>
#include <Runtime/ShaderCompilation/Slang/SLC2/SLC2Reader.h>
#include <Runtime/ShaderCompilation/Slang/SlangShaderCompiler.h>

namespace SE
{
    BINARY_ASSET_FACTORY(GraphicShader, true);

#if SE_EDITOR

    bool IsValidShaderCache2(DataContainer<byte>& shaderCache, List<String>& includes)
    {
        if (shaderCache.Length() == 0)
        {
            return false;
        }
/*        MemoryReadStream stream(shaderCache.Get(), shaderCache.Length());

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
            include = ShadersCompilation::ResolveShaderPath(include);
            DateTime lastEditTime;
            stream.Read(lastEditTime);

            // Check if included file exists locally and has been modified since last compilation
            if (!(FileSystem::FileExists(include) && FileSystem::GetFileLastEditTime(include) > lastEditTime))
            {
                return false;
            }
        }*/

        return true;
    }

#endif


    bool GraphicShader::SelectProgram(String&                           name,
                                      const ShaderProgramSelection&     selection,
                                      UniquePtr<ShaderProgramInstance>& output)
    {
        UniquePtr<ShaderProgramInstance> selected = CreateScope<ShaderProgramInstance>();

        if (!m_GPUShader->CreateProgramInstance(name, selection, *selected))
        {
            return false;
        }
        output = MoveTemp(selected);
        return true;
    }

    GraphicShader::GraphicShader(const SpawnParams& params, const AssetInfo* info) : BinaryAsset(params, info) 
    {
        m_GPUShader = GPUDevice::instance->CreateSLC2Shader(info->path);
    }

    GraphicShader::~GraphicShader() 
    { 
        Delete(m_GPUShader);
    };

    int32 GraphicShader::GetCacheChunkIndex(ShaderProfile profile) 
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

    bool GraphicShader::LoadShaderCache(ShaderCacheResult& result) 
    {
        // Prepare
        const ShaderProfile                   shaderProfile   = GPUDevice::instance->GetShaderProfile();
        const int32                           cacheChunkIndex = GetCacheChunkIndex(shaderProfile);
        const auto                            cachingMode     = ShaderStorage::GetCachingMode();
        ShaderCacheManager::CachedEntryHandle cachedEntry;

#if SE_EDITOR
        // Try to get cached shader (based on current caching policy)
        bool hasCache = false;
        if (cachingMode == ShaderStorage::CachingMode::AssetInternal)
        {
            if (HasChunkLoaded(cacheChunkIndex))
            {
                // Load cached shader data, create reading stream (memory free, super fast, super easy, no overhead at
                // all)
                const auto cacheChunk = GetChunk(cacheChunkIndex);
                result.Data.Link(cacheChunk->Data);
                if (IsValidShaderCache2(result.Data, result.Includes))
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
            const DateTime assetModificationDate = storage ? FileSystem::GetFileLastEditTime(storage->GetPath()) : DateTime::MinValue();

            if (ShaderCacheManager::TryGetEntry(shaderProfile, GetID(), cachedEntry) &&
                cachedEntry.GetModificationDate() > assetModificationDate &&
                !ShaderCacheManager::GetCache(shaderProfile, cachedEntry, result.Data) &&
                IsValidShaderCache2(result.Data, result.Includes))
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
        if ((forceRecompile || GPU_FORCE_RECOMPILE_SHADERS || !hasCache) && HasChunk(SHADER_FILE_CHUNK_SOURCE))
        {
            result.Data.Release();
            const String parentPath = GetPath();
            const UID    parentID   = GetID();
            LOG_INFO("Resource", "Compiling shader '{0}':{1}...", parentPath, parentID);

            // Load all chunks (except that with internal cache for current shader profile, it will be generated)
            AssetChunksFlag chunksToLoad = ALL_ASSET_CHUNKS;
            chunksToLoad &= ~GET_CHUNK_FLAG(cacheChunkIndex);
            if (!LoadChunks(chunksToLoad))
            {
                LOG_WARNING("Resource", "Cannot load '{0}' data from chunk {1}.", ToString(), chunksToLoad);
                return false;
            }

            // Remove current profile internal chunk (it could be loaded by the asset itself during precaching phrase)
            ReleaseChunk(cacheChunkIndex);


            // Decrypt source code
            auto sourceChunk  = GetChunk(SHADER_FILE_CHUNK_SOURCE);
            auto source       = sourceChunk->Get<char>();
            auto sourceLength = sourceChunk->Size();
            Encoding::DecryptBytes((byte*)source, sourceLength);
            source[sourceLength - 1] = 0;


            SlangShaderCompiler compiler;

            ShaderCompileRequest request;
            request.ShaderName = FileSystem::GetFileNameWithoutExtension(parentPath);
            request.SourcePath = parentPath;
            request.SourceCode = source;
            request.SourceCodeLength = sourceLength;
            request.Targets.Add({shaderProfile, FeatureLevel::SM5});

/*            #if PLATFORM_WINDOWS
            platformDefine.Name = "PLATFORM_WINDOWS";
#elif PLATFORM_LINUX
            platformDefine.Name = "PLATFORM_LINUX";
#elif PLATFORM_MAC
            platformDefine.Name = "PLATFORM_MAC";
#else
#error "Unknown platform."
#endif*/

            ShaderCompileResult compileResult = compiler.Compile(request);
            
            // Encrypt source code
            Encoding::EncryptBytes(reinterpret_cast<byte*>(source), sourceLength);

            if (!compileResult.Success)
            {
                LOG_ERROR("Resource", "Failed to compile shader '{0}' {1}", ToString(), compileResult.CompileMessage);
                return false;
            }


            // Save compilation result (based on current caching policy)
            if (cachingMode == ShaderStorage::CachingMode::AssetInternal)
            {
                // Save cache to the internal shader cache chunk
                auto cacheChunk = GetOrCreateChunk(cacheChunkIndex);
                if (cacheChunk == nullptr)
                {
                    return false;
                }
                cacheChunk->Data.Copy(compileResult.SLC2Data);
#if SE_EDITOR
                // Save chunks to the asset file
                if (!Save())
                {
                    LOG_WARNING("Resource", "Cannot save '{0}'.", ToString());
                    return false;
                }

                // Save 会导致chunk 数据被释放需要重新加载
                List<StorageChunk*> chunks;
                storage->GetChunks(chunks);
                for (int i = 0; i < chunks.Count(); i++)
                {
                    storage->LoadAssetChunk(chunks[i]);
                }
#endif
            }
            else if (cachingMode == ShaderStorage::CachingMode::ProjectCache)
            {
                MemoryWriteStream stream(static_cast<uint32>(compileResult.SLC2Data.Count()));
                stream.WriteBytes(compileResult.SLC2Data.Get(), static_cast<uint32>(compileResult.SLC2Data.Count()));

                // Save results to cache
                if (!ShaderCacheManager::SetCache(shaderProfile, cachedEntry, stream))
                {
                    LOG_WARNING("Resource", "Cannot save shader cache.");
                    return false;
                }
            }
            else
            {
                // Use temporary generated data without caching (but read the includes from cache)
                result.Data.Copy(compileResult.SLC2Data);
                IsValidShaderCache2(result.Data, result.Includes);
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
        if (HasChunkLoaded(cacheChunkIndex))
        {
            // Load cached shader data, create reading stream (memory free, super fast, super easy, no overhead at all)
            const auto cacheChunk = GetChunk(cacheChunkIndex);
            result.Data.Link(cacheChunk->Data);
        }
        // Check if has cached shader
        else if (cachedEntry.IsValid() || ShaderCacheManager::TryGetEntry(shaderProfile, GetID(), cachedEntry))
        {
            // Load results from cache
            if (!ShaderCacheManager::GetCache(shaderProfile, cachedEntry, result.Data))
            {
                LOG_WARNING("Resource", "Cannot load shader cache.");
                return false;
            }
        }
        else
        {
            LOG_WARNING("Resource", "Missing shader cache '{0}'.", ToString());
            return false;
        }

        ENGINE_ASSERT(result.Data.IsValid());
    }

    uint32 GraphicShader::GetSerializedVersion() const { return 1; }

    bool GraphicShader::OnInit(AssetInitData& initData)
    {
        // Validate version
        if (initData.SerializedVersion != ShaderStorage::Header::Version)
        {
            LOG_WARNING("Resource", "Invalid shader serialized version.");
            return false;
        }

    }

    Asset::LoadResult GraphicShader::load()
    {
        // Load shader cache (it may call compilation or gather cached data)
        ShaderCacheResult shaderCache;
        if (!LoadShaderCache(shaderCache))
        {
            LOG_ERROR("Resource", "Cannot load \'{0}\' shader cache.", ToString());
            return LoadResult::Failed;
        }

        // Create shader from cache
        if (!m_GPUShader->Load(shaderCache.Data.Get(), shaderCache.Data.Length()))
        {
            LOG_ERROR("Resource", "Cannot load shader \'{0}\'", ToString());
            return LoadResult::Failed;
        }

        RegisterForShaderReloads(this, shaderCache);

        return LoadResult::Ok;
    }

    void GraphicShader::Unload(const bool isReloading)
    {
        ShadersCompilation::UnregisterForShaderReloads(this);
        m_GPUShader->ReleaseGPU();
    }

#if SE_EDITOR
    bool GraphicShader::Save() 
    {
        AssetInitData data;
        data.SerializedVersion = ShaderStorage::Header::Version;
        //data.CustomData.Link(&_shaderHeader);
        metadata.Release();
        return SaveAsset(data);
    }
#endif

    void GraphicShader::RegisterForShaderReloads(Asset* asset, const ShaderCacheResult& shaderCache)
    {
        for (auto& include : shaderCache.Includes)
        {
            ShadersCompilation::RegisterForShaderReloads(asset, include);
        }
    }

    void GraphicShader::UnregisterForShaderReloads(Asset* asset)
    {
        ShadersCompilation::UnregisterForShaderReloads(asset);
    }

} // namespace SE
