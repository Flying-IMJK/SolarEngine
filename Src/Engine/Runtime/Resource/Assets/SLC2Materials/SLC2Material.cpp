#include <Runtime/Resource/Assets/SLC2Materials/SLC2Material.h>

#include <Runtime/Core/Math/Matrix.h>
#include <Runtime/Core/Serialization/MemoryReadStream.h>
#include <Runtime/Core/Serialization/MemoryWriteStream.h>
#include <Runtime/Core/Thread/Threading.h>
#include <Runtime/Graphics/Shaders/SLC2GPUShaderProgram.h>
#include <Runtime/Graphics/Shaders/ShaderBindingSnapshot.h>
#include <Runtime/Graphics/Textures/GPUTexture.h>
#include <Runtime/Render/Assets/Texture/TextureBase.h>
#include <Runtime/Resource/AssetContent.h>
#include <Runtime/Resource/Assets/SLC2Materials/SLC2MaterialStorage.h>
#include <Runtime/Resource/Factories/BinaryAssetFactory.h>
#include <Runtime/Resource/Storage/StorageChunk.h>
#include <Runtime/Utilities/Variant.h>

namespace SE
{
    BINARY_ASSET_FACTORY(SLC2Material, true);

    namespace
    {
        bool TryGetAuthoredBindingKind(const MaterialParameterType type, ShaderProgramBindingKind& kind)
        {
            switch (type)
            {
                case MaterialParameterType::Bool:
                case MaterialParameterType::Integer:
                case MaterialParameterType::Float:
                case MaterialParameterType::Vector2:
                case MaterialParameterType::Vector3:
                case MaterialParameterType::Vector4:
                case MaterialParameterType::Color:
                case MaterialParameterType::Matrix:
                case MaterialParameterType::ChannelMask:
                    kind = ShaderProgramBindingKind::Uniform;
                    return true;
                case MaterialParameterType::Texture:
                case MaterialParameterType::CubeTexture:
                case MaterialParameterType::NormalMap:
                    kind = ShaderProgramBindingKind::Texture;
                    return true;
                default:
                    return false;
            }
        }


        GPUTextureView* GetTextureView(const MaterialParameter& parameter)
        {
            TextureBase* textureAsset = TypeTryCast<TextureBase>(parameter.GetAssetValue());
            GPUTexture*  texture      = textureAsset != nullptr ? textureAsset->GetTexture() : nullptr;
            if (texture == nullptr)
                return nullptr;
            return parameter.GetParameterType() == MaterialParameterType::CubeTexture ? texture->ViewArray() :
                                                                                        texture->View();
        }

        bool WriteParameter(const MaterialParameter&    parameter,
                            const ShaderProgramBinding& binding,
                            ShaderProgramInstance&      instance)
        {
            const Variant value = parameter.GetValue();
            switch (parameter.GetParameterType())
            {
                case MaterialParameterType::Bool: {
                    const uint32 uniformValue = static_cast<bool>(value) ? 1u : 0u;
                    return instance.SetUniform(binding.Path, &uniformValue, sizeof(uniformValue));
                }
                case MaterialParameterType::Integer: {
                    const int32 uniformValue = static_cast<int32>(value);
                    return instance.SetUniform(binding.Path, &uniformValue, sizeof(uniformValue));
                }
                case MaterialParameterType::Float: {
                    const float uniformValue = static_cast<float>(value);
                    return instance.SetUniform(binding.Path, &uniformValue, sizeof(uniformValue));
                }
                case MaterialParameterType::Vector2: {
                    const Float2 uniformValue = static_cast<Float2>(value);
                    return instance.SetUniform(binding.Path, &uniformValue, sizeof(uniformValue));
                }
                case MaterialParameterType::Vector3: {
                    const Float3 uniformValue = static_cast<Float3>(value);
                    return instance.SetUniform(binding.Path, &uniformValue, sizeof(uniformValue));
                }
                case MaterialParameterType::Vector4: {
                    const Float4 uniformValue = static_cast<Float4>(value);
                    return instance.SetUniform(binding.Path, &uniformValue, sizeof(uniformValue));
                }
                case MaterialParameterType::Color: {
                    const Color uniformValue = static_cast<Color>(value);
                    return instance.SetUniform(binding.Path, &uniformValue, sizeof(uniformValue));
                }
                case MaterialParameterType::Matrix: {
                    Matrix uniformValue;
                    Matrix::Transpose(static_cast<Matrix>(value), uniformValue);
                    return instance.SetUniform(binding.Path, &uniformValue, sizeof(uniformValue));
                }
                case MaterialParameterType::ChannelMask: {
                    const int32  channel = static_cast<int32>(value);
                    const Float4 uniformValue(channel == 0, channel == 1, channel == 2, channel == 3);
                    return instance.SetUniform(binding.Path, &uniformValue, sizeof(uniformValue));
                }
                case MaterialParameterType::Texture:
                case MaterialParameterType::CubeTexture:
                case MaterialParameterType::NormalMap: {
                    GPUTextureView* view = GetTextureView(parameter);
                    return view != nullptr && instance.SetTexture(binding.Path, view);
                }
                default:
                    return false;
            }
        }

        bool WriteBindings(ShaderProgramInstance&                    instance,
                           const MaterialParams&                     parameters,
                           const List<SLC2MaterialParameterBinding>& bindings,
                           const SLC2MaterialDrawContext&            context)
        {
            const ShaderProgramReflection& reflection = instance.GetProgram()->GetReflection();
            List<ShaderProgramBinding>     reflectedBindings;
            if (!reflection.EnumerateBindings(reflectedBindings))
                return false;

            int32 authoredBindingsWritten = 0;
            for (const ShaderProgramBinding& reflected : reflectedBindings)
            {
                const SLC2MaterialParameterBinding* authored = nullptr;
                for (const SLC2MaterialParameterBinding& binding : bindings)
                {
                    if (binding.NamePath == reflected.Path)
                    {
                        authored = &binding;
                        break;
                    }
                }

                bool written;
                if (authored != nullptr)
                {
                    const MaterialParameter* parameter = parameters.Get(authored->ParameterID);
                    written = authored->BindingKind == reflected.Kind && parameter != nullptr &&
                              WriteParameter(*parameter, reflected, instance);
                    ++authoredBindingsWritten;
                }
                else
                {
                    written = context.RuntimeBindings != nullptr &&
                              context.RuntimeBindings->WriteBinding(reflected, instance);
                }
                if (!written)
                {
                    LOG_ERROR("Resource", "SLC2 Material could not write exact binding '{0}'.", reflected.Path);
                    return false;
                }
            }

            if (authoredBindingsWritten != bindings.Count())
            {
                LOG_ERROR("Resource", "SLC2 Material has authored bindings absent from the selected Program variant.");
                return false;
            }
            ShaderBindingSnapshot snapshot;
            return instance.BuildSnapshot(snapshot);
        }
    } // namespace

    SLC2Material::SLC2Material(const SpawnParams& params, const AssetInfo* info) : BinaryAsset(params, info)
    {
    
    }

    bool SLC2Material::GetParameters(MaterialParams& output) const
    {
        int32  version;
        uint64 revision;
        return SnapshotParameters(output, version, revision);
    }

    bool SLC2Material::SnapshotParameters(MaterialParams& output, int32& version, uint64& revision) const
    {
        Threading::ScopeLock lock(const_cast<CriticalSection&>(Locker));
        const_cast<MaterialParams&>(m_Parameters).Clone(output);
        version  = m_Parameters.GetVersionHash();
        revision = m_Revision;
        return true;
    }

    bool SLC2Material::SetParameters(MaterialParams& parameters)
    {
        m_Parameters = MoveTemp(parameters);
        m_CachedPipelines.Clear();
        ++m_Revision;
        return true;
    }

    bool SLC2Material::SetParameterValue(const UID& parameterId, const Variant& value)
    {
        MaterialParameter* parameter = m_Parameters.Get(parameterId);

        if (parameter == nullptr || parameter->TrySetValue(value) != MaterialParameterSetResult::Success)
        {
            return false;
        }

        m_CachedPipelines.Clear();
        ++m_Revision;
        return true;
    }

    void SLC2Material::GetParameterBindings(List<SLC2MaterialParameterBinding>& output) const
    {
        output = m_Bindings;
    }

    bool SLC2Material::SetParameterBinding(const SLC2MaterialParameterBinding& binding)
    {
        Threading::ScopeLock               lock(Locker);
        List<SLC2MaterialParameterBinding> candidate = m_Bindings;
        bool                               replaced  = false;
        for (SLC2MaterialParameterBinding& existing : candidate)
        {
            if (existing.ParameterID == binding.ParameterID)
            {
                existing = binding;
                replaced = true;
                break;
            }
        }

        if (!replaced)
        {
            candidate.Add(binding);
        }
        if (!m_Shader)
        {
            return false; 
        }
        m_Bindings = MoveTemp(candidate);
        m_CachedPipelines.Clear();
        ++m_Revision;
        return true;
    }

    bool SLC2Material::RemoveParameterBinding(const UID& parameterId)
    {
        List<SLC2MaterialParameterBinding> candidate = m_Bindings;
        for (int32 index = 0; index < candidate.Count(); ++index)
        {
            if (candidate[index].ParameterID == parameterId)
            {
                candidate.RemoveAt(index);

                m_Bindings = MoveTemp(candidate);
                m_CachedPipelines.Clear();
                ++m_Revision;
                return true;
            }
        }
        return false;
    }

    void SLC2Material::GetPipelines(List<SLC2MaterialPipelineState>& output) const
    {
        output = m_Pipelines;
    }

    bool SLC2Material::SetPipeline(const SLC2MaterialPipelineState& pipeline)
    {
        Threading::ScopeLock            lock(Locker);
        List<SLC2MaterialPipelineState> candidate = m_Pipelines;
        bool                            replaced  = false;
        for (SLC2MaterialPipelineState& existing : candidate)
        {
            if (existing.ProgramId == pipeline.ProgramId)
            {
                existing = pipeline;
                replaced = true;
                break;
            }
        }

        if (!replaced)
        {
            candidate.Add(pipeline);
        }

        m_Pipelines = MoveTemp(candidate);
        m_CachedPipelines.Clear();
        ++m_Revision;
        return true;
    }

    bool SLC2Material::RemovePipeline(const String& programId)
    {
        Threading::ScopeLock            lock(Locker);
        List<SLC2MaterialPipelineState> candidate = m_Pipelines;
        for (int32 index = 0; index < candidate.Count(); ++index)
        {
            if (candidate[index].ProgramId == programId)
            {
                candidate.RemoveAt(index);
                if (!m_Shader)
                {
                    return false;
                }
                m_Pipelines = MoveTemp(candidate);
                m_CachedPipelines.Clear();
                ++m_Revision;
                return true;
            }
        }
        return false;
    }

    bool SLC2Material::SetGraphicShader(GraphicShader* pShader)
    {
        if (pShader == nullptr || !pShader->WaitForLoaded())
        {
            return false;
        }

        m_Shader = pShader;
        m_CachedPipelines.Clear();
        ++m_Revision;
        return true;
    }

    AssetRef<GraphicShader> SLC2Material::GetGraphicShader() const
    {
        return m_Shader;
    }

    bool SLC2Material::PrepareDraw(const String&                             passName,
                                   const SLC2MaterialGeometryMode            geometryMode,
                                   const ShaderCompileTarget&                target,
                                   const List<String>&                       defines,
                                   const SLC2MaterialDrawContext&            context,
                                   std::unique_ptr<SLC2MaterialDrawProgram>& output) const
    {
        MaterialParams parameters;
        int32          version;
        uint64         revision;
        if (!SnapshotParameters(parameters, version, revision))
        {
            return false;
        }
        
        return PrepareDrawWithParameters(
            passName, geometryMode, target, defines, parameters, revision, context, output);
    }

    bool SLC2Material::PrepareDrawWithParameters(const String&                             passName,
                                                 const SLC2MaterialGeometryMode            geometryMode,
                                                 const ShaderCompileTarget&                target,
                                                 const List<String>&                       defines,
                                                 const MaterialParams&                     parameters,
                                                 const uint64                              expectedRevision,
                                                 const SLC2MaterialDrawContext&            context,
                                                 std::unique_ptr<SLC2MaterialDrawProgram>& output) const
    {
        /*output.reset();
        AssetRef<GraphicShader>            shader;
        List<SLC2MaterialParameterBinding> bindings;
        List<SLC2MaterialPipelineState>    pipelines;
        uint64                             revision;
        {
            Threading::ScopeLock lock(const_cast<CriticalSection&>(Locker));
            if (m_Revision != expectedRevision)
                return false;
            shader    = m_Shader;
            bindings  = m_Bindings;
            pipelines = m_Pipelines;
            revision  = m_Revision;
        }
        if (!shader || !shader->WaitForLoaded() || !parameters.HasContentLoaded())
            return false;

        ShaderProgramSelection request;
        request.Target    = target;
        request.Defines   = defines;

        const SLC2MaterialPipelineState* pipelineDescription = nullptr;
        for (const SLC2MaterialPipelineState& pipeline : pipelines)
        {
            if (pipeline.ProgramId == request.ProgramId)
            {
                pipelineDescription = &pipeline;
                break;
            }
        }
        if (pipelineDescription == nullptr)
        {
            LOG_ERROR("Resource", "SLC2 Material has no Pipeline for ProgramId {0}.", request.ProgramId);
            return false;
        }

        UniquePtr<ShaderProgramInstance> selected;
        if (!shader->SelectProgram(request, selected) || !WriteBindings(*selected, parameters, bindings, context))
        {
            return false;
        }

        SharedPtr<SLC2MaterialPipeline> pipeline;
        {
            Threading::ScopeLock lock(const_cast<CriticalSection&>(Locker));
            if (m_Revision != revision || m_Shader.Get() != shader.Get())
                return false;
            if (!m_CachedPipelines.IsEmpty())
            {
                m_CachedPipelines.Clear();
            }
            for (const SharedPtr<SLC2MaterialPipeline>& cached : m_CachedPipelines)
            {
                if (cached->Program == selected->GetProgram() && cached->ProgramId == request.ProgramId)
                {
                    pipeline = cached;
                    break;
                }
            }
        }

        if (pipeline == nullptr)
        {
            SharedPtr<SLC2MaterialPipeline> candidate = MakeRef<SLC2MaterialPipeline>();
            candidate->ProgramId                      = request.ProgramId;
            candidate->Program                        = selected->GetProgram();
            candidate->State                          = SLC2GPUPipelineState::New();
            if (candidate->State == nullptr || !candidate->State->Init(pipelineDescription->State, candidate->Program))
                return false;

            Threading::ScopeLock lock(const_cast<CriticalSection&>(Locker));
            if (m_Revision != revision || m_Shader.Get() != shader.Get())
                return false;
            for (const SharedPtr<SLC2MaterialPipeline>& cached : m_CachedPipelines)
            {
                if (cached->Program == candidate->Program && cached->ProgramId == candidate->ProgramId)
                {
                    pipeline = cached;
                    break;
                }
            }
            if (pipeline == nullptr)
            {
                pipeline = MoveTemp(candidate);
                m_CachedPipelines.Add(pipeline);
            }
        }

        output.reset(new SLC2MaterialDrawProgram(MoveTemp(selected), MoveTemp(pipeline)));*/
        return true;
    }

    uint32 SLC2Material::GetSerializedVersion() const { return SLC2MaterialStorage::Version; }

    bool SLC2Material::OnInit(AssetInitData& initData)
    {
        return initData.SerializedVersion == SLC2MaterialStorage::Version;
    }

    AssetChunksFlag SLC2Material::GetChunksToPreload() const { return GET_CHUNK_FLAG(SLC2MaterialStorage::Chunk); }

    Asset::LoadResult SLC2Material::load()
    {
        StorageChunk*                      chunk = GetChunk(SLC2MaterialStorage::Chunk);
        UID                                shaderId;
        MaterialParams                     parameters;
        List<SLC2MaterialParameterBinding> bindings;
        List<SLC2MaterialPipelineState>    pipelines;
        if (chunk == nullptr || !chunk->IsValid() ||
            !SLC2MaterialStorage::DecodeMaterial(
                chunk->Get(), chunk->Size(), shaderId, parameters, bindings, pipelines))
            return LoadResult::InvalidData;

        AssetRef<GraphicShader> shader = AssetContent::LoadAsync<GraphicShader>(shaderId);
        if (!shader || !shader->WaitForLoaded())
        {
            return LoadResult::InvalidData;
        }
        Threading::ScopeLock lock(Locker);
        m_Shader     = MoveTemp(shader);
        m_Parameters = MoveTemp(parameters);
        m_Bindings   = MoveTemp(bindings);
        m_Pipelines  = MoveTemp(pipelines);
        m_CachedPipelines.Clear();
        ++m_Revision;
        return LoadResult::Ok;
    }

    void SLC2Material::Unload(const bool isReloading)
    {
        // Reload keeps the published state readable until a complete candidate replaces it.
        if (isReloading)
            return;
        Threading::ScopeLock lock(Locker);
        m_Shader = nullptr;
        m_Parameters.Dispose();
        m_Bindings.Clear();
        m_Pipelines.Clear();
        m_CachedPipelines.Clear();
        ++m_Revision;
    }

#if SE_EDITOR
    bool SLC2Material::SaveAssetData(const StringView&                         path,
                                     GraphicShader*                            pShader,
                                     MaterialParams&                           parameters,
                                     const List<SLC2MaterialParameterBinding>& bindings,
                                     const List<SLC2MaterialPipelineState>&    pipelines)
    {
        if (pShader == nullptr || !pShader->WaitForLoaded())
            return false;
        List<byte> bytes;
        SLC2MaterialStorage::EncodeMaterial(pShader->GetID(), parameters, bindings, pipelines, bytes);
        StorageChunk chunk;
        chunk.Data.Copy(bytes.Get(), bytes.Count());
        AssetInitData data;
        data.Header.TypeID                             = Typeof<SLC2Material>();
        data.Header.Chunks[SLC2MaterialStorage::Chunk] = &chunk;
        data.SerializedVersion                         = SLC2MaterialStorage::Version;
        return SaveToAsset(path, data);
    }

    void SLC2Material::GetReferences(List<UID>& output) const
    {
        BinaryAsset::GetReferences(output);
        AssetRef<GraphicShader> shader;
        MaterialParams          parameters;
        {
            Threading::ScopeLock lock(const_cast<CriticalSection&>(Locker));
            shader = m_Shader;
            const_cast<MaterialParams&>(m_Parameters).Clone(parameters);
        }
        if (shader)
            output.Add(shader.GetID());
        parameters.GetReferences(output);
    }
#endif
} // namespace SE
