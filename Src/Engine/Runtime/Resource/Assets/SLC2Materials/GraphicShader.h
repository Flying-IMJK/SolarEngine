#pragma once

#include "Runtime/Graphics/Base/GPUPipelineState.h"
#include "Runtime/Graphics/Shaders/SLC2GPUShader.h"
#include "Runtime/Graphics/Shaders/ShaderProgramInstance.h"
#include "Runtime/Resource/BinaryAsset.h"

namespace SE
{
    class GPUContext;
    struct RenderGeometry;
    struct GraphicShaderGeneration;

    enum class GraphicShaderGeometryMode : byte
    {
        Static,
        Instanced,
        Skinned,
    };

    struct SE_API_RUNTIME GraphicShaderPipeline
    {
        String ProgramId;
        SLC2GPUPipelineState::Description State;
    };

    // A selected draw owns its shader generation, Program, mutable parameters and pipeline lifetime.
    // Keep this object alive until its GPU submission is finished; never detach its raw pipeline/Program pointers.
    class SE_API_RUNTIME GraphicShaderDrawProgram final
    {
    public:
        ShaderProgramInstance& GetInstance() { return m_Instance; }
        SLC2GPUPipelineState* GetPipeline() const { return m_Pipeline; }

        // The caller sets render targets before preparing geometry, then calls GPUContext::Draw*(GetInstance(), ...).
        bool Prepare(GPUContext& context, const RenderGeometry& geometry) const;

    private:
        friend class GraphicShader;
        explicit GraphicShaderDrawProgram(SharedPtr<GraphicShaderGeneration> generation);

        SharedPtr<GraphicShaderGeneration> m_Generation;
        ShaderProgramInstance m_Instance;
        SLC2GPUPipelineState* m_Pipeline = nullptr;
    };

    // Shared compiled output contract for handwritten and, later, Graph-authored shaders.
    SE_CLASS(Reflect, API(NoSpawn))
    class SE_API_RUNTIME GraphicShader final : public BinaryAsset
    {
        SE_DEFINE_CLASS(GraphicShader, BinaryAsset)
        ASSET_HEADER(GraphicShader);

    public:
        ~GraphicShader() override;

        // Install is also the in-memory authoring entry for virtual assets. Failure retains the active generation.
        bool InstallCompiled(const List<byte>& artifact, const List<GraphicShaderPipeline>& pipelines);
        bool CreateDrawProgram(const String& passName, GraphicShaderGeometryMode geometryMode,
                               const ShaderCompileTarget& target, const List<String>& defines,
                               std::unique_ptr<GraphicShaderDrawProgram>& output);

#if SE_EDITOR
        // Same versioned storage contract is used by handwritten and generated shader producers.
        // Returns true on success (the underlying Storage::Create convention).
        static bool SaveCompiledAsset(const StringView& path, const List<byte>& artifact,
                                      const List<GraphicShaderPipeline>& pipelines);
#endif

        uint32 GetSerializedVersion() const override;

    protected:
        bool OnInit(AssetInitData& initData) override;
        AssetChunksFlag GetChunksToPreload() const override;
        LoadResult load() override;
        void Unload(bool isReloading) override;

    private:
        SharedPtr<GraphicShaderGeneration> m_Generation;
    };
} // namespace SE
