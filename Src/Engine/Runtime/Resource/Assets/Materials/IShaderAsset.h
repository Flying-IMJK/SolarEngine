#pragma once
#include "ShaderStorage.h"
#include "Runtime/Resource/BinaryAsset.h"

namespace SE
{
    /// <summary>
    /// Base class for assets that can contain shader.
    /// </summary>
    class SE_API_RUNTIME IShaderAsset
    {
    public:
	    static constexpr uint32 ShadersSerializedVersion = ShaderStorage::Header::Version;

        static bool IsNullRenderer();

        /// <summary>
        /// Gets internal shader cache chunk index (for current GPU device shader profile).
        /// </summary>
        static int32 GetCacheChunkIndex();

        /// <summary>
        /// Gets internal shader cache chunk index.
        /// </summary>
        /// <param name="profile">Shader profile</param>
        /// <returns>Chunk index</returns>
        static int32 GetCacheChunkIndex(ShaderProfile profile);

#if SE_EDITOR

        /// <summary>
        /// Prepare shader compilation options
        /// </summary>
        /// <param name="options">Options</param>
        virtual void InitCompilationOptions(struct ShaderCompilationOptions& options)
        {
        }

#endif

    protected:
        /// <summary>
        /// Gets the parent asset.
        /// </summary>
        virtual BinaryAsset* GetShaderAsset() const = 0;


#if SE_EDITOR
        /// <summary>
        /// Saves this shader asset to the storage container.
        /// </summary>
        /// <returns>True if failed, otherwise false.</returns>
        virtual bool Save() = 0;
#endif

        /// <summary>
        /// Shader cache loading result data container.
        /// </summary>
        struct ShaderCacheResult
        {
            /// <summary>
            /// The shader cache data. Allocated or linked (if gathered from asset chunk).
            /// </summary>
            DataContainer<byte> Data;

            /// <summary>
            /// The list of files included by the shader source (used by the given cache on the runtime graphics platform shader profile). Paths are absolute and unique.
            /// </summary>
            List<String> Includes;
        };

        /// <summary>
        /// Loads shader cache (it may call compilation or gather precached data).
        /// </summary>
        /// <param name="result">The output data.</param>
        /// <returns>True if cannot load data, otherwise false.</returns>
        bool LoadShaderCache(ShaderCacheResult& result);

        /// <summary>
        /// Registers shader asset for the automated reloads on source includes changes.
        /// </summary>
        /// <param name="asset">The asset.</param>
        /// <param name="shaderCache">The loaded shader cache.</param>
        void RegisterForShaderReloads(Asset* asset, const ShaderCacheResult& shaderCache);

        /// <summary>
        /// Unregisters shader asset from the automated reloads on source includes changes.
        /// </summary>
        /// <param name="asset">The asset.</param>
        void UnregisterForShaderReloads(Asset* asset);
    };
} // SE

