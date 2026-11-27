
#include <Runtime/Resource/Importers/ImportShader.h>

#include <Runtime/Core/Encoding.h>
#include <Runtime/Core/Platform/File.h>
#include <Runtime/Core/Serialization/JsonWriters.hpp>
#include <Runtime/Core/TypeSystem/Types.h>
#include <Runtime/Resource/Assets/Materials/Shader.h>
#include <Runtime/Resource/Assets/Materials/ShaderCacheManager.h>
#include <Runtime/Resource/Assets/Materials/ShaderStorage.h>
#include <Runtime/Resource/Importers/AssetsImportingSystem.h>

namespace SE
{

    CreateAssetResult ImportShader::Import(CreateAssetContext& context)
    {
        // Base
        IMPORT_SETUP(Shader, ShaderStorage::Header::Version);

        // Read text (handles any Unicode convert into ANSI)
        StringAnsi sourceCodeText;
        if (!File::ReadAllText(context.InputPath, sourceCodeText))
        {
            return CreateAssetResult::InvalidPath;
        }

        // Load source code
        if (!context.AllocateChunk(SHADER_FILE_CHUNK_SOURCE))
        {
            return CreateAssetResult::CannotAllocateChunk;
        }
        const auto sourceCodeSize = sourceCodeText.Length();
        if (sourceCodeSize < 10)
        {
            LOG_WARNING("Resource", "Empty shader source file.");
            return CreateAssetResult::Error;
        }

        Json::StringBuffer sourceDescription;
        CompactJsonWriter  writer(sourceDescription);
        writer.StartObject();
        writer.JKEY("inlineCode");
        writer.Bool(false);
        writer.JKEY("source");
        writer.JsonWriter::String(AssetsImporting::GetImportPath(context.InputPath));
        writer.EndObject();

        // Store only the source reference; compilation reads the current file contents.
        const auto& chunk = context.Data.Header.Chunks[SHADER_FILE_CHUNK_SOURCE];
        chunk->Data.Copy(reinterpret_cast<const byte*>(sourceDescription.GetString()), static_cast<int32>(sourceDescription.GetSize()));
        Encoding::EncryptBytes(chunk->Get(), chunk->Size());

        // Set Custom Data with Header
        ShaderStorage::Header shaderHeader;
        Platform::MemoryClear(&shaderHeader, sizeof(shaderHeader));
        context.Data.CustomData.Copy(&shaderHeader);

#if SE_EDITOR || COMPILE_WITH_SHADER_CACHE_MANAGER
        // Invalidate shader cache
        ShaderCacheManager::RemoveCache(context.Data.Header.ID);
#endif

        return CreateAssetResult::Ok;
    }

} // namespace SE
