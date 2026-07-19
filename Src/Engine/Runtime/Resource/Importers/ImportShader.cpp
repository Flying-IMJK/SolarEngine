
#include "ImportShader.h"

#include "Runtime/Core/Platform/File.h"
#include "Runtime/Core/TypeSystem/Types.h"
#include "Runtime/Resource/Assets/Materials/Shader.h"
#include "Runtime/Resource/Assets/Materials/ShaderStorage.h"

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

		// Ensure the source code has an empty line at the end (expected by glslang)
		auto sourceCodeChunkSize = sourceCodeSize + 1;
		if (sourceCodeText[sourceCodeSize - 1] != '\n')
			sourceCodeChunkSize++;

		const auto& sourceCodeChunk = context.Data.Header.Chunks[SourceCodeChunk];
		sourceCodeChunk->Data.Allocate(sourceCodeChunkSize);
		const auto sourceCode = sourceCodeChunk->Get();
		Platform::MemoryCopy(sourceCode, sourceCodeText.Get(), sourceCodeSize);
		sourceCode[sourceCodeChunkSize - 2] = '\n';

		// Encrypt source code
		Encoding::EncryptBytes(sourceCode, sourceCodeChunkSize - 1);
		sourceCode[sourceCodeChunkSize - 1] = 0;

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

} // SE