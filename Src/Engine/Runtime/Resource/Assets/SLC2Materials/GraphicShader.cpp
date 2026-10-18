#include "GraphicShader.h"

#include "GraphicShaderStorage.h"
#include "Runtime/Core/Memory/Memory.h"
#include "Runtime/Core/Thread/Threading.h"
#include "Runtime/Graphics/GPUContext.h"
#include "Runtime/Graphics/GPUDevice.h"
#include "Runtime/Graphics/Base/RenderGeometry.h"
#include "Runtime/Graphics/Shaders/Reflection/ShaderProgramReflection.h"
#include "Runtime/Resource/Factories/BinaryAssetFactory.h"
#include "Runtime/Resource/Storage/StorageChunk.h"
#include "Runtime/ShaderCompilation/Slang/SLC2/SLC2Reader.h"

namespace SE
{
    BINARY_ASSET_FACTORY(GraphicShader, true);

    struct GraphicShaderGeneration
    {
        struct CachedPipeline
        {
            const SLC2GPUShaderProgram* Program = nullptr;
            SLC2GPUPipelineState* State = nullptr;
        };

        SLC2GPUShader* Shader = nullptr;
        List<GraphicShaderPipeline> Descriptions;
        List<CachedPipeline> Pipelines;

        ~GraphicShaderGeneration()
        {
            // Pipeline objects still refer to the selected Program's modules/layout.
            for (const CachedPipeline& pipeline : Pipelines)
                Delete(pipeline.State);
            Delete(Shader);
        }
    };

    namespace
    {
        bool ValidatePipelineState(const SLC2GPUPipelineState::Description& state)
        {
            const auto comparisonValid = [](ComparisonFunc value)
            {
                return value >= ComparisonFunc::Never && value <= ComparisonFunc::Always;
            };
            const auto stencilValid = [](StencilOperation value)
            {
                return value >= StencilOperation::Keep && value < StencilOperation::MAX;
            };
            const auto factorValid = [](BlendingMode::Blend value)
            {
                const uint32 factor = static_cast<uint32>(value);
                return (factor >= 1 && factor <= 11) || (factor >= 14 && factor <= 19);
            };
            const auto operationValid = [](BlendingMode::Operation value)
            {
                return value >= BlendingMode::Operation::Add && value <= BlendingMode::Operation::Max;
            };
            const auto& blend = state.BlendMode;
            return comparisonValid(state.DepthFunc) && comparisonValid(state.StencilFunc) &&
                stencilValid(state.StencilFailOp) && stencilValid(state.StencilDepthFailOp) &&
                stencilValid(state.StencilPassOp) &&
                state.PrimitiveTopology >= PrimitiveTopologyType::Point &&
                state.PrimitiveTopology <= PrimitiveTopologyType::Triangle &&
                state.CullMode >= CullMode::Normal && state.CullMode <= CullMode::TwoSided &&
                factorValid(blend.SrcBlend) && factorValid(blend.DestBlend) &&
                factorValid(blend.SrcBlendAlpha) && factorValid(blend.DestBlendAlpha) &&
                operationValid(blend.BlendOp) && operationValid(blend.BlendOpAlpha) &&
                static_cast<uint32>(blend.RenderTargetWriteMask) <= 15;
        }

        bool ValidateCompiled(const List<byte>& artifactData, const List<GraphicShaderPipeline>& pipelines)
        {
            SLC2Artifact artifact;
            String error;
            if (!SLC2Reader::Read(artifactData, artifact, error) || artifact.Programs.IsEmpty() ||
                artifact.Programs.Count() != pipelines.Count())
            {
                LOG_ERROR("Resource", "GraphicShader requires valid SLC2 data and one pipeline description per Program: {0}", error);
                return false;
            }

            // Validate the entire generation before publishing; only a requested target/variant creates GPU objects.
            for (const GraphicShaderPipeline& pipeline : pipelines)
            {
                const SLC2ProgramRecord* program = nullptr;
                for (const SLC2ProgramRecord& record : artifact.Programs)
                {
                    if (record.ProgramId == pipeline.ProgramId)
                    {
                        program = &record;
                        break;
                    }
                }
                if (program == nullptr || pipeline.ProgramId.IsEmpty() || !ValidatePipelineState(pipeline.State))
                {
                    LOG_ERROR("Resource", "GraphicShader pipeline names an invalid or non-graphics Program.");
                    return false;
                }
                for (const GraphicShaderPipeline& other : pipelines)
                {
                    if (&other != &pipeline && other.ProgramId == pipeline.ProgramId)
                    {
                        LOG_ERROR("Resource", "GraphicShader has duplicate ProgramId {0}.", pipeline.ProgramId);
                        return false;
                    }
                }
                for (const SLC2TargetRecord& target : program->Targets)
                {
                    for (const SLC2VariantRecord& variant : target.Variants)
                    {
                        ShaderProgramReflection reflection;
                        if (!reflection.Initialize(variant.Layout))
                        {
                            LOG_ERROR("Resource", "GraphicShader Program {0} has invalid reflection.", pipeline.ProgramId);
                            return false;
                        }
                        bool vertex = false, pixel = false;
                        for (const SLC2StageRecord& stage : variant.Stages)
                        {
                            vertex |= stage.Stage == ShaderStage::Vertex;
                            pixel |= stage.Stage == ShaderStage::Pixel;
                        }
                        if (!vertex || !pixel)
                        {
                            LOG_ERROR("Resource", "GraphicShader Program {0} is not drawable graphics.", pipeline.ProgramId);
                            return false;
                        }
                    }
                }
            }
            return true;
        }
    }

    GraphicShaderDrawProgram::GraphicShaderDrawProgram(SharedPtr<GraphicShaderGeneration> generation)
        : m_Generation(MoveTemp(generation))
    {
    }

    bool GraphicShaderDrawProgram::Prepare(GPUContext& context, const RenderGeometry& geometry) const
    {
        if (m_Pipeline == nullptr || !m_Pipeline->IsValid())
            return false;
        context.SetSLC2State(m_Pipeline);
        return context.BindRenderGeometry(geometry);
    }

    GraphicShader::GraphicShader(const SpawnParams& params, const AssetInfo* info) : BinaryAsset(params, info) {}
    GraphicShader::~GraphicShader() = default;

    bool GraphicShader::InstallCompiled(const List<byte>& artifact, const List<GraphicShaderPipeline>& pipelines)
    {
        if (!ValidateCompiled(artifact, pipelines) || GPUDevice::instance == nullptr)
            return false;

        auto candidate = std::make_shared<GraphicShaderGeneration>();
        candidate->Shader = GPUDevice::instance->CreateSLC2Shader(SE_TEXT("GraphicShader"));
        if (candidate->Shader == nullptr || !candidate->Shader->Load(artifact))
            return false;
        candidate->Descriptions = pipelines;

        // Never call Load on the active GPUShader: its cached Programs and reflection pointers belong to the old artifact.
        Threading::ScopeLock lock(Locker);
        m_Generation = MoveTemp(candidate);
        return true;
    }

    bool GraphicShader::CreateDrawProgram(const String& passName, const GraphicShaderGeometryMode geometryMode,
                                           const ShaderCompileTarget& target, const List<String>& defines,
                                           std::unique_ptr<GraphicShaderDrawProgram>& output)
    {
        // Failed selection must not leave a previous draw's Program available as an implicit fallback.
        output.reset();
        String programId = passName;
        switch (geometryMode)
        {
        case GraphicShaderGeometryMode::Static: break;
        case GraphicShaderGeometryMode::Instanced: programId += SE_TEXT("Instanced"); break;
        case GraphicShaderGeometryMode::Skinned: programId += SE_TEXT("Skinned"); break;
        default: return false;
        }

        Threading::ScopeLock lock(Locker);
        auto generation = m_Generation;
        if (generation == nullptr)
            return false;

        const GraphicShaderPipeline* description = nullptr;
        for (const GraphicShaderPipeline& pipeline : generation->Descriptions)
        {
            if (pipeline.ProgramId == programId)
            {
                description = &pipeline;
                break;
            }
        }
        if (description == nullptr)
        {
            LOG_ERROR("Resource", "GraphicShader has no selected ProgramId {0}.", programId);
            return false;
        }

        ShaderProgramSelection selection;
        selection.ProgramId = programId;
        selection.Target = target;
        selection.Defines = defines;
        auto chosen = std::unique_ptr<GraphicShaderDrawProgram>(new GraphicShaderDrawProgram(generation));
        if (!generation->Shader->CreateProgramInstance(selection, chosen->m_Instance))
            return false;

        const SLC2GPUShaderProgram* program = chosen->m_Instance.GetProgram();
        for (const GraphicShaderGeneration::CachedPipeline& pipeline : generation->Pipelines)
        {
            if (pipeline.Program == program)
            {
                chosen->m_Pipeline = pipeline.State;
                output = MoveTemp(chosen);
                return true;
            }
        }

        SLC2GPUPipelineState* pipeline = SLC2GPUPipelineState::New();
        if (pipeline == nullptr || !pipeline->Init(description->State, program))
        {
            Delete(pipeline);
            return false;
        }
        GraphicShaderGeneration::CachedPipeline cached;
        cached.Program = program;
        cached.State = pipeline;
        generation->Pipelines.Add(cached);
        chosen->m_Pipeline = pipeline;
        output = MoveTemp(chosen);
        return true;
    }

    uint32 GraphicShader::GetSerializedVersion() const { return GraphicShaderStorage::Version; }

    bool GraphicShader::OnInit(AssetInitData& initData)
    {
        return initData.SerializedVersion == GraphicShaderStorage::Version;
    }

    AssetChunksFlag GraphicShader::GetChunksToPreload() const
    {
        return GET_CHUNK_FLAG(GraphicShaderStorage::Chunk);
    }

    Asset::LoadResult GraphicShader::load()
    {
        auto chunk = GetChunk(GraphicShaderStorage::Chunk);
        if (chunk == nullptr || !chunk->IsValid())
            return LoadResult::MissingDataChunk;

        List<byte> artifact;
        List<GraphicShaderPipeline> pipelines;
        if (!GraphicShaderStorage::Decode(chunk->Get(), chunk->Size(), artifact, pipelines) ||
            !InstallCompiled(artifact, pipelines))
            return LoadResult::InvalidData;
        return LoadResult::Ok;
    }

    void GraphicShader::Unload(const bool isReloading)
    {
        // Asset::Reload unloads before loading the candidate: retain the active generation until installation succeeds.
        if (!isReloading)
            m_Generation.reset();
    }

#if SE_EDITOR
    bool GraphicShader::SaveCompiledAsset(const StringView& path, const List<byte>& artifact,
                                          const List<GraphicShaderPipeline>& pipelines)
    {
        if (!ValidateCompiled(artifact, pipelines))
            return false;

        List<byte> bytes;
        GraphicShaderStorage::Encode(artifact, pipelines, bytes);
        StorageChunk chunk;
        chunk.Data.Copy(bytes.Get(), bytes.Count());
        AssetInitData data;
        data.Header.TypeID = Typeof<GraphicShader>();
        data.Header.Chunks[GraphicShaderStorage::Chunk] = &chunk;
        data.SerializedVersion = GraphicShaderStorage::Version;
        return SaveToAsset(path, data);
    }
#endif
} // namespace SE
