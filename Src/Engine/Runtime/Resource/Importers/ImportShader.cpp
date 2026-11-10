
#include <Runtime/Resource/Importers/ImportShader.h>

#include <Runtime/Core/Encoding.h>
#include <Runtime/Core/Platform/File.h>
#include <Runtime/Core/TypeSystem/Types.h>
#include <Runtime/Resource/Assets/Materials/Shader.h>
#include <Runtime/Resource/Assets/Materials/ShaderCacheManager.h>
#include <Runtime/Resource/Assets/Materials/ShaderStorage.h>

namespace SE
{

	CreateAssetResult ImportShader::Import(CreateAssetContext& context)
	{
		// Base
		IMPORT_SETUP(Shader, ShaderStorage::Header::Version);
		const int32 SourceCodeChunk = 15;
		context.SkipMetadata = true;

		// Read text (handles any Unicode convert into ANSI)
		StringAnsi sourceCodeText;
		if (!File::ReadAllText(context.InputPath, sourceCodeText))
			return CreateAssetResult::InvalidPath;

		// Load source code
		if (!context.AllocateChunk(SourceCodeChunk))
		{
			return CreateAssetResult::CannotAllocateChunk;
		}
		const auto sourceCodeSize = sourceCodeText.Length();
		if (sourceCodeSize < 10)
		{
			LOG_WARNING("Resource", "Empty shader source file.");
			return CreateAssetResult::Error;
		}

        // Encrypt the complete payload, including its terminator, as expected by IShaderAsset.
        const auto& chunk = context.Data.Header.Chunks[SourceCodeChunk];
        chunk->Data.Allocate(sourceCodeSize + 1);
        Platform::MemoryCopy(chunk->Get(), sourceCodeText.Get(), sourceCodeSize);
        chunk->Get()[sourceCodeSize] = 0;
        Encoding::EncryptBytes(chunk->Get(), sourceCodeSize + 1);

        // Set Custom Data with Header
        ShaderStorage::Header shaderHeader;
        Platform::MemoryClear(&shaderHeader, sizeof(shaderHeader));
		context.Data.CustomData.Copy(&shaderHeader);

#if COMPILE_WITH_SHADER_CACHE_MANAGER
		// Invalidate shader cache
		ShaderCacheManager::RemoveCache(context.Data.Header.ID);
#endif

		return CreateAssetResult::Ok;
	}

} // namespace SE
