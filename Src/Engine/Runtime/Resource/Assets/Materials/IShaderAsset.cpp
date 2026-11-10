#include <Runtime/Resource/Assets/Materials/IShaderAsset.h>

#include <Runtime/Core/Encoding.h>
#include <Runtime/Core/Platform/File.h>
#include <Runtime/Core/Platform/FileSystem.h>
#include <Runtime/Core/Serialization/MemoryReadStream.h>
#include <Runtime/Core/Serialization/MemoryWriteStream.h>
#include <Runtime/Graphics/GPUDevice.h>
#include <Runtime/Graphics/GlobalSettings_GPU.h>
#include <Runtime/Resource/Assets/Materials/ShaderCacheManager.h>
#include <Runtime/Resource/Storage/StorageChunk.h>
#include <Runtime/ShaderCompilation/ShadersCompilation.h>
#include <Runtime/ShaderCompilation/Slang/SLC2/SLC2Reader.h>
#if SE_EDITOR
#include <Runtime/ShaderCompilation/Slang/SlangShaderCompiler.h>
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
/*#if !COMPILE_WITH_SHADER_CACHE_MANAGER
        if (CurrentCachingMode == CachingMode::ProjectCache)
            return CachingMode::AssetInternal;
#endif*/
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
    namespace
    {
        uint32 Fingerprint(const BytesContainer& data)
        {
            uint32 hash = 2166136261u;
            for (int32 i = 0; i < data.Length(); ++i)
                hash = (hash ^ data.Get()[i]) * 16777619u;
            return hash;
        }

        // The payload stays a raw SLC2 artifact. Dependency metadata is a separate record, tied to the exact
        // profile and payload fingerprint so an interrupted cache write cannot publish a mismatched pair.
        bool ReadDependencies(const BytesContainer& metadata,
                              const BytesContainer& shader,
                              ShaderProfile         profile,
                              List<String>&         includes)
        {
            if (metadata.Length() < 4 * sizeof(uint32))
                return false;
            MemoryReadStream stream(metadata.Get(), metadata.Length());
            uint32           version, storedProfile, hash, count;
            stream.Read(version);
            stream.Read(storedProfile);
            stream.Read(hash);
            stream.Read(count);
            if (version != 1 || storedProfile != static_cast<uint32>(profile) || hash != Fingerprint(shader) ||
                count > (stream.GetLength() - stream.GetPosition()) / (sizeof(uint32) + sizeof(DateTime)))
                return false;
            includes.Clear();
            for (uint32 i = 0; i < count; ++i)
            {
                if (stream.GetLength() - stream.GetPosition() < sizeof(uint32))
                    return false;
                uint32 length;
                stream.Read(length);
                if (length > (stream.GetLength() - stream.GetPosition()) / sizeof(Char) ||
                    stream.GetLength() - stream.GetPosition() - length * sizeof(Char) < sizeof(DateTime))
                    return false;
                String path(reinterpret_cast<const Char*>(stream.GetPositionHandle()), length);
                stream.SetPosition(stream.GetPosition() + length * sizeof(Char));
                DateTime timestamp;
                stream.Read(timestamp);
                if (!FileSystem::FileExists(path) || FileSystem::GetFileLastEditTime(path) != timestamp)
                    return false;
                includes.Add(path);
            }
            return stream.GetPosition() == stream.GetLength();
        }

        void WriteDependencies(MemoryWriteStream&    writer,
                               const BytesContainer& shader,
                               ShaderProfile         profile,
                               const List<String>&   includes)
        {
            writer.Write(static_cast<uint32>(1));
            writer.Write(static_cast<uint32>(profile));
            writer.Write(Fingerprint(shader));
            writer.Write(static_cast<uint32>(includes.Count()));
            for (const String& path : includes)
            {
                writer.Write(static_cast<uint32>(path.Length()));
                writer.WriteBytes(reinterpret_cast<const byte*>(path.Get()), path.Length() * sizeof(Char));
                writer.Write(FileSystem::GetFileLastEditTime(path));
            }
        }
    } // namespace

    void IShaderAsset::InitCompilationRequest(ShaderCompileRequest& request, StringAnsi& preamble)
    {
#if PLATFORM_WINDOWS
        preamble += "#define PLATFORM_WINDOWS 1\n";
#elif PLATFORM_LINUX
        preamble += "#define PLATFORM_LINUX 1\n";
#elif PLATFORM_MAC
        preamble += "#define PLATFORM_MAC 1\n";
#endif
        preamble += "#define SE_EDITOR 1\n";
    }
#endif

    bool IShaderAsset::LoadShaderCache(ShaderCacheResult& result)
    {
        result.Data.Release();
        result.Includes.Clear();
        BinaryAsset* parent = GetShaderAsset();
        if (!parent || !GPUDevice::instance)
        {
            return false;
        }

        const ShaderProfile                   profile    = GPUDevice::instance->GetShaderProfile();
        const int32                           chunkIndex = GetCacheChunkIndex(profile);
        const auto                            mode       = ShaderStorage::GetCachingMode();
        ShaderCacheManager::CachedEntryHandle entry;
        BytesContainer                        cache;
        BytesContainer                        dependencies;
        bool                                  hasCache = false;
        if (mode == ShaderStorage::CachingMode::AssetInternal && parent->HasChunk(chunkIndex))
        {
            if (!parent->HasChunkLoaded(chunkIndex) && !parent->LoadChunks(GET_CHUNK_FLAG(chunkIndex)))
            {
                return false;
            }
            const StorageChunk* chunk = parent->GetChunk(chunkIndex);
            if (chunk && chunk->IsValid())
            {
                cache.Copy(chunk->Data);
                hasCache = true;
            }
#if SE_EDITOR
            if (parent->HasChunk(SHADER_FILE_CHUNK_DEPENDENCIES) &&
                parent->LoadChunks(GET_CHUNK_FLAG(SHADER_FILE_CHUNK_DEPENDENCIES)))
            {
                const StorageChunk* metadata = parent->GetChunk(SHADER_FILE_CHUNK_DEPENDENCIES);
                if (metadata && metadata->IsValid())
                {
                    dependencies.Copy(metadata->Data);
                }
            }
#endif
        }
        else if (mode == ShaderStorage::CachingMode::ProjectCache)
        {
            const bool     exists = ShaderCacheManager::TryGetEntry(profile, parent->GetID(), entry);

            DateTime assetTime;
            if (parent->storage)
            {
                assetTime = FileSystem::GetFileLastEditTime(parent->storage->GetPath());
            }
            else
            {
                assetTime = DateTime::MinValue();
            }

            hasCache = exists && entry.GetModificationDate() >= assetTime && ShaderCacheManager::GetCache(profile, entry, cache);

#if SE_EDITOR
            if (hasCache)
            {
                File::ReadAllBytes(entry.Path + SE_TEXT(".deps"), dependencies);
            }
#endif
        }

        String error;
        bool   valid = hasCache && SLC2Reader::ReadAndValidate(cache.Get(), cache.Length(), error);
#if SE_EDITOR
        valid = valid && !GPU_FORCE_RECOMPILE_SHADERS && ReadDependencies(dependencies, cache, profile, result.Includes);
#endif
        if (valid)
        {
            result.Data.Copy(cache);
            return true;
        }
        result.Includes.Clear();

#if SE_EDITOR
        if (!parent->HasChunk(SHADER_FILE_CHUNK_SOURCE) || !parent->LoadChunks(GET_CHUNK_FLAG(SHADER_FILE_CHUNK_SOURCE)))
        {
            return false;
        }
        const StorageChunk* sourceChunk = parent->GetChunk(SHADER_FILE_CHUNK_SOURCE);
        if (!sourceChunk || sourceChunk->Size() == 0)
        {
            return false;
        }

        BytesContainer decrypted;
        decrypted.Copy(sourceChunk->Data);
        Encoding::DecryptBytes(decrypted.Get(), decrypted.Length());
        decrypted.Get()[decrypted.Length() - 1] = 0;

        ShaderCompileRequest request;
        request.ShaderName = FileSystem::GetFileNameWithoutExtension(parent->GetPath());
        request.SourcePath = parent->GetPath();
        request.Targets.Add(profile);
        StringAnsi source;
        InitCompilationRequest(request, source);
        source += StringAnsi(reinterpret_cast<const char*>(decrypted.Get()), decrypted.Length() - 1);
        request.SourceCode       = source.Get();
        request.SourceCodeLength = source.Length();
        SlangShaderCompiler compiler;
        ShaderCompileResult compiled = compiler.Compile(request);
        if (!compiled.Success)
        {
            LOG_ERROR("Resource", "Cannot compile SLC2 shader '{0}': {1}", parent->ToString(), compiled.CompileMessage);
            return false;
        }
        result.Data.Copy(compiled.SLC2Data);
        List<String> additional;
        GetAdditionalSourceFiles(additional);
        additional.Add(compiled.SourceDependencies);
        for (const String& path : additional)
        {
            if (!FileSystem::AreFilePathsEqual(path, parent->GetPath()) && !result.Includes.Contains(path))
            {
                result.Includes.Add(path);
            }
        }
        MemoryWriteStream metadata;
        WriteDependencies(metadata, result.Data, profile, result.Includes);
        if (mode == ShaderStorage::CachingMode::AssetInternal)
        {
            StorageChunk* target          = parent->GetOrCreateChunk(chunkIndex);
            StorageChunk* dependencyChunk = parent->GetOrCreateChunk(SHADER_FILE_CHUNK_DEPENDENCIES);
            if (!target || !dependencyChunk)
                return false;
            target->Data.Copy(result.Data);
            dependencyChunk->Data.Copy(metadata.GetHandle(), metadata.GetLength());
            if (!Save())
                return false;
        }
        else if (mode == ShaderStorage::CachingMode::ProjectCache)
        {
            MemoryWriteStream writer;
            writer.WriteBytes(result.Data.Get(), result.Data.Length());
            if (!ShaderCacheManager::SetCache(profile, entry, writer) ||
                !metadata.SaveToFile(entry.Path + SE_TEXT(".deps")))
                LOG_WARNING("Resource", "Cannot save SLC2 shader cache '{0}'.", parent->ToString());
        }
        return true;
#else
        LOG_ERROR("Resource", "Missing or invalid precompiled SLC2 shader '{0}': {1}", parent->ToString(), error);
        return false;
#endif
    }

    void IShaderAsset::RegisterForShaderReloads(Asset* pAsset, const ShaderCacheResult& cache)
    {
        for (const String& path : cache.Includes)
            ShadersCompilation::RegisterForShaderReloads(pAsset, path);
    }

    void IShaderAsset::UnregisterForShaderReloads(Asset* pAsset)
    {
        ShadersCompilation::UnregisterForShaderReloads(pAsset);
    }
} // namespace SE
