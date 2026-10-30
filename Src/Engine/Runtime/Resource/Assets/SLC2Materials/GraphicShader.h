#pragma once

#include <Runtime/Graphics/Shaders/SLC2GPUShader.h>
#include <Runtime/Graphics/Shaders/ShaderProgramInstance.h>
#include <Runtime/Resource/BinaryAsset.h>

namespace SE
{
    class GraphicShaderProgramSelection;
    struct SLC2MaterialParameterBinding;
    struct SLC2MaterialPipelineState;
    class SLC2Material;

    // Shared compiled output contract for handwritten and, later, Graph-authored shaders.
    SE_CLASS(Reflect, API(NoSpawn))
    class SE_API_RUNTIME GraphicShader final : public BinaryAsset
    {
        SE_DEFINE_CLASS(GraphicShader, BinaryAsset)
        ASSET_HEADER(GraphicShader);

    public:
        ~GraphicShader() override;

        // Creates an independently mutable Program instance on the GPU/render-resource thread. output is reset on
        // entry and failure; success transfers an owning selection that retains the Generation. Keep it alive while
        // any Pipeline or draw uses its Program.
        bool SelectProgram(const String& programName, const List<String>& selection, SLC2GPUShaderProgram*& output);

        bool SelectProgram(const String& programName, const List<String>& selection, ShaderProgramInstance* programIns);

#if SE_EDITOR
        bool Save();
#endif

        uint32 GetSerializedVersion() const override;

        SLC2GPUShader* GetGPUShader() { return m_GPUShader; }

    protected:
        bool            OnInit(AssetInitData& initData) override;
        LoadResult      load() override;
        void            Unload(bool isReloading) override;

    private:
        friend class SLC2Material;

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
            /// The list of files included by the shader source (used by the given cache on the runtime graphics
            /// platform shader profile). Paths are absolute and unique.
            /// </summary>
            List<String> Includes;
        };

        /// <summary>
        /// Gets internal shader cache chunk index.
        /// </summary>
        /// <param name="profile">Shader profile</param>
        /// <returns>Chunk index</returns>
        static int32 GetCacheChunkIndex(ShaderProfile profile);

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


        SLC2GPUShader* m_GPUShader;
    };
} // namespace SE
