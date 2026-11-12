#pragma once

#include <Runtime/Resource/Assets/Materials/ShaderStorage.h>
#include <Runtime/Resource/BinaryAsset.h>
#include <Runtime/ShaderCompilation/ShaderCompileTypes.h>

namespace SE
{
    /// Common source compilation, ShaderArtifact storage, and reload dependencies for shader-bearing assets.
    class SE_API_RUNTIME IShaderAsset
    {
    public:
        static constexpr uint32 ShadersSerializedVersion = ShaderStorage::Header::Version;
        static bool IsNullRenderer();
        static int32            GetCacheChunkIndex();
        static int32            GetCacheChunkIndex(ShaderProfile profile);

#if SE_EDITOR
        // Append compile-time definitions to preamble and configure the new Slang request. The base owns
        // source storage and attaches the final SourceCode pointer after this hook returns.
        virtual void InitCompilationRequest(ShaderCompileRequest& request, StringAnsi& preamble);
        virtual void GetAdditionalSourceFiles(List<String>& files) const { files.Clear(); }
#endif

    protected:
        virtual BinaryAsset* GetShaderAsset() const = 0;
#if SE_EDITOR
        // True on success. Saving can release the parent's chunk memory.
        virtual bool Save() = 0;
#endif
        struct ShaderCacheResult
        {
            // Owns raw ShaderArtifact bytes independently of asset chunks, including across Save().
            BytesContainer Data;
            List<String> Includes;
        };

        bool LoadShaderCache(ShaderCacheResult& result);
        void RegisterForShaderReloads(Asset* pAsset, const ShaderCacheResult& cache);
        void UnregisterForShaderReloads(Asset* pAsset);
    };
} // namespace SE
