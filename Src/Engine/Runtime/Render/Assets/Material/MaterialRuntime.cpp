#include "MaterialRuntime.h"

#include "Runtime/Core/Math/Matrix.h"
#include "Runtime/Core/Memory/Memory.h"
#include "Runtime/Utilities/Variant.h"
#include "Runtime/Graphics/GPUDevice.h"
#include "Runtime/Graphics/Shaders/ShaderBindingSnapshot.h"
#include "Runtime/Graphics/Shaders/ShaderProgramInstance.h"
#include "Runtime/Graphics/Shaders/SLC2GPUShaderProgram.h"
#include "Runtime/Graphics/Textures/GPUTexture.h"
#include "Runtime/Render/Assets/Texture/TextureBase.h"
#include "Runtime/ShaderCompilation/Slang/SLC2/SLC2Reader.h"

namespace SE
{
    struct MaterialRuntimeBindingWrite
    {
        ShaderProgramBinding Binding;
        UID ParameterID = UID::Empty;
        MaterialRuntimeBindingSource Source = MaterialRuntimeBindingSource::PerView;
        bool UsesMaterialParameter = false;
    };

    class MaterialRuntimeProgram
    {
    public:
        ~MaterialRuntimeProgram()
        {
            Delete(Pipeline);
        }

        MaterialProgramContract Contract;
        // This is the active binding set for one exact Program/Target/Variant.
        // Prepare only executes these precomputed writes and never searches the
        // global material binding table.
        List<MaterialRuntimeBindingWrite> Bindings;
        SLC2GPUShaderProgram* Program = nullptr;
        SLC2GPUPipelineState* Pipeline = nullptr;
    };

    class MaterialProgramInstanceCacheEntry
    {
    public:
        // 成员逆序销毁，先销毁 Instance，再释放其依赖的 Generation。
        SharedPtr<MaterialRuntimeGeneration> Generation;
        const MaterialRuntimeProgram* Program = nullptr;
        ShaderProgramInstance Instance;
    };

    namespace
    {
        uint64 NextMaterialRuntimeGenerationID = 0;

        bool IsMaterialPath(const String& path)
        {
            return path.StartsWith(SE_TEXT("Material."), StringSearchCase::CaseSensitive);
        }

        bool IsPerViewPath(const String& path)
        {
            return path.StartsWith(SE_TEXT("PerView."), StringSearchCase::CaseSensitive);
        }

        bool IsPerDrawPath(const String& path)
        {
            return path.StartsWith(SE_TEXT("PerDraw."), StringSearchCase::CaseSensitive);
        }

        bool IsDirectUniformParameter(const MaterialParameterType type)
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
                return true;
            default:
                return false;
            }
        }

        bool IsDirectTextureParameter(const MaterialParameterType type)
        {
            switch (type)
            {
            case MaterialParameterType::Texture:
            case MaterialParameterType::CubeTexture:
            case MaterialParameterType::NormalMap:
            case MaterialParameterType::GPUTexture:
            case MaterialParameterType::GPUTextureArray:
            case MaterialParameterType::GPUTextureVolume:
            case MaterialParameterType::GPUTextureCube:
                return true;
            default:
                return false;
            }
        }

        bool TryGetProviderSource(const MaterialParameterType type, MaterialRuntimeBindingSource& source)
        {
            switch (type)
            {
            case MaterialParameterType::SceneTexture:
                source = MaterialRuntimeBindingSource::SceneTexture;
                return true;
            case MaterialParameterType::TextureGroupSampler:
                source = MaterialRuntimeBindingSource::TextureGroupSampler;
                return true;
            case MaterialParameterType::GameplayGlobal:
                source = MaterialRuntimeBindingSource::GameplayGlobal;
                return true;
            case MaterialParameterType::GlobalSDF:
                source = MaterialRuntimeBindingSource::GlobalSDF;
                return true;
            default:
                return false;
            }
        }

        bool ValidateDirectParameterBinding(const MaterialParameter& parameter, const ShaderProgramBinding& binding, const IMaterialRuntimeBindingProvider* provider, String& error)
        {
            const MaterialParameterType type = parameter.GetParameterType();
            if (IsDirectUniformParameter(type))
            {
                // Material selects the ordinary-data setter, but the caller owns
                // scalar/vector interpretation and ABI correctness.
                if (binding.Kind != ShaderProgramBindingKind::Uniform)
                {
                    error = String::Format(SE_TEXT("Material parameter '{0}' does not match reflected uniform '{1}'."), parameter.GetName(), binding.Path);
                    return false;
                }
                return true;
            }
            if (IsDirectTextureParameter(type))
            {
                if (binding.Kind != ShaderProgramBindingKind::Texture)
                {
                    error = String::Format(SE_TEXT("Material texture parameter '{0}' does not match reflected resource '{1}'."), parameter.GetName(), binding.Path);
                    return false;
                }
                if (provider == nullptr || !provider->SupportsBinding(MaterialRuntimeBindingSource::DefaultResource, type, binding))
                {
                    error = String::Format(SE_TEXT("Material texture binding '{0}' has no default-resource policy."), binding.Path);
                    return false;
                }
                return true;
            }

            MaterialRuntimeBindingSource source;
            if (!TryGetProviderSource(type, source) || provider == nullptr || !provider->SupportsBinding(source, type, binding))
            {
                error = String::Format(SE_TEXT("Material parameter type {0} has no runtime binding provider for '{1}'."), static_cast<int32>(type), binding.Path);
                return false;
            }
            return true;
        }

        GPUTextureView* GetTextureView(const MaterialParameter& parameter)
        {
            GPUTexture* texture = parameter.GetGPUTextureValue();
            if (texture == nullptr && parameter.GetAssetValue() != nullptr)
            {
                const TextureBase* textureAsset = TypeTryCast<TextureBase>(parameter.GetAssetValue());
                texture = textureAsset != nullptr ? textureAsset->GetTexture() : nullptr;
            }
            if (texture == nullptr)
            {
                return nullptr;
            }

            switch (parameter.GetParameterType())
            {
            case MaterialParameterType::CubeTexture:
            case MaterialParameterType::GPUTextureArray:
            case MaterialParameterType::GPUTextureCube:
                return texture->ViewArray();
            case MaterialParameterType::GPUTextureVolume:
                return texture->ViewVolume();
            default:
                return texture->View();
            }
        }

        bool WriteDirectParameter(const MaterialParameter& parameter, const ShaderProgramBinding& binding, const MaterialDrawContext& context, const IMaterialRuntimeBindingProvider* provider, ShaderProgramInstance& instance)
        {
            const Variant value = parameter.GetValue();
            switch (parameter.GetParameterType())
            {
            case MaterialParameterType::Bool:
            {
                const int32 uniformValue = static_cast<bool>(value) ? 1 : 0;
                return instance.SetUniform(binding.Path, &uniformValue, sizeof(uniformValue));
            }
            case MaterialParameterType::Integer:
            {
                const int32 uniformValue = static_cast<int32>(value);
                return instance.SetUniform(binding.Path, &uniformValue, sizeof(uniformValue));
            }
            case MaterialParameterType::Float:
            {
                const float uniformValue = static_cast<float>(value);
                return instance.SetUniform(binding.Path, &uniformValue, sizeof(uniformValue));
            }
            case MaterialParameterType::Vector2:
            {
                const Float2 uniformValue = static_cast<Float2>(value);
                return instance.SetUniform(binding.Path, &uniformValue, sizeof(uniformValue));
            }
            case MaterialParameterType::Vector3:
            {
                const Float3 uniformValue = static_cast<Float3>(value);
                return instance.SetUniform(binding.Path, &uniformValue, sizeof(uniformValue));
            }
            case MaterialParameterType::Vector4:
            {
                const Float4 uniformValue = static_cast<Float4>(value);
                return instance.SetUniform(binding.Path, &uniformValue, sizeof(uniformValue));
            }
            case MaterialParameterType::Color:
            {
                const Color uniformValue = static_cast<Color>(value);
                return instance.SetUniform(binding.Path, &uniformValue, sizeof(uniformValue));
            }
            case MaterialParameterType::Matrix:
            {
                Matrix uniformValue;
                Matrix::Transpose(static_cast<Matrix>(value), uniformValue);
                return instance.SetUniform(binding.Path, &uniformValue, sizeof(uniformValue));
            }
            case MaterialParameterType::ChannelMask:
            {
                const int32 channel = static_cast<int32>(value);
                const Float4 uniformValue(channel == 0, channel == 1, channel == 2, channel == 3);
                return instance.SetUniform(binding.Path, &uniformValue, sizeof(uniformValue));
            }
            default:
                break;
            }

            if (IsDirectTextureParameter(parameter.GetParameterType()))
            {
                GPUTextureView* view = GetTextureView(parameter);
                if (view != nullptr)
                {
                    return instance.SetTexture(binding.Path, view);
                }
                return provider != nullptr && provider->WriteBinding(MaterialRuntimeBindingSource::DefaultResource, &parameter, binding, context, instance);
            }

            MaterialRuntimeBindingSource source;
            return TryGetProviderSource(parameter.GetParameterType(), source) && provider != nullptr && provider->WriteBinding(source, &parameter, binding, context, instance);
        }

        const MaterialParameter* ResolveMaterialParameter(const List<const MaterialParams*>& parameterChain, const UID& parameterID)
        {
            for (int32 chainIndex = 0; chainIndex < parameterChain.Count(); chainIndex++)
            {
                const MaterialParams* parameters = parameterChain[chainIndex];
                const MaterialParameter* parameter = parameters != nullptr ? parameters->Get(parameterID) : nullptr;
                if (parameter == nullptr)
                {
                    return nullptr;
                }

                // The root value is the final fallback even when its Override bit is false.
                if (parameter->IsOverride() || chainIndex == parameterChain.Count() - 1)
                {
                    return parameter;
                }
            }
            return nullptr;
        }
    }

    MaterialProgramInstanceCache::~MaterialProgramInstanceCache()
    {
        Clear();
    }

    void MaterialProgramInstanceCache::Clear()
    {
        m_Entries.ClearDelete();
    }

    MaterialRuntimeGeneration::~MaterialRuntimeGeneration()
    {
        m_Programs.ClearDelete();
        Delete(m_Shader);
        m_Shader = nullptr;
    }

    SharedPtr<MaterialRuntimeGeneration> MaterialRuntimeGeneration::Create(MaterialSLC2RuntimeData&& data,
                                                                           const MaterialInfo&       materialInfo,
                                                                           const MaterialParams&     parameters,
                                                                           String&                   error)
    {
        error.Clear();
        if (materialInfo.Domain != MaterialDomain::Surface)
        {
            error = SE_TEXT("SLC2 material runtime currently supports Surface materials only.");
            return nullptr;
        }
        if (data.SLC2Data.IsEmpty() || data.Programs.IsEmpty())
        {
            error = SE_TEXT("Material SLC2 runtime data has no artifact or Program contracts.");
            return nullptr;
        }
        SharedPtr<MaterialRuntimeGeneration> candidate = MakeRef<MaterialRuntimeGeneration>();
        candidate->m_Self = candidate;
        SLC2Artifact artifact;
        if (!SLC2Reader::Read(data.SLC2Data, artifact, error))
        {
            return nullptr;
        }
        candidate->m_BindingProvider = data.BindingProvider;
        candidate->m_Bindings = MoveTemp(data.Bindings);

        for (int32 bindingIndex = 0; bindingIndex < candidate->m_Bindings.Count(); bindingIndex++)
        {
            const MaterialParameterBinding& binding = candidate->m_Bindings[bindingIndex];
            if (!IsMaterialPath(binding.ShaderPath) || parameters.Get(binding.ParameterID) == nullptr)
            {
                error = String::Format(SE_TEXT("Material binding '{0}' has an invalid path or ParameterID."), binding.ShaderPath);
                return nullptr;
            }
            for (int32 previous = 0; previous < bindingIndex; previous++)
            {
                if (candidate->m_Bindings[previous].ShaderPath == binding.ShaderPath)
                {
                    error = String::Format(SE_TEXT("Material binding path '{0}' is duplicated."), binding.ShaderPath);
                    return nullptr;
                }
            }
        }

        // 第一阶段只解析选择和 Reflection，所有契约通过后才创建 Program/Pipeline GPU 对象。
        for (int32 contractIndex = 0; contractIndex < data.Programs.Count(); contractIndex++)
        {
            const MaterialProgramContract& contract = data.Programs[contractIndex];
            if ((contract.Pass != DrawPass::GBuffer && contract.Pass != DrawPass::Forward) ||
                (contract.GeometryMode != MaterialGeometryMode::Static && contract.GeometryMode != MaterialGeometryMode::Instanced))
            {
                error = SE_TEXT("Material Program contract contains an unsupported pass or geometry mode.");
                return nullptr;
            }
            for (int32 previous = 0; previous < contractIndex; previous++)
            {
                if (data.Programs[previous].Pass == contract.Pass && data.Programs[previous].GeometryMode == contract.GeometryMode)
                {
                    error = SE_TEXT("Material Program contracts contain an ambiguous pass/geometry match.");
                    return nullptr;
                }
            }

            const SLC2ProgramRecord* programRecord = nullptr;
            const SLC2TargetRecord* targetRecord = nullptr;
            const SLC2VariantRecord* variantRecord = nullptr;
            if (!SLC2GPUShader::ResolveProgramSelection(artifact, contract.Selection, programRecord, targetRecord, variantRecord))
            {
                error = String::Format(SE_TEXT("Material Program contract cannot resolve Program '{0}'."), contract.Selection.ProgramId);
                return nullptr;
            }

            ShaderProgramReflection reflection;
            List<ShaderProgramBinding> reflectedBindings;
            MaterialRuntimeProgram* runtimeProgram = New<MaterialRuntimeProgram>();
            runtimeProgram->Contract = contract;
            if (!reflection.Initialize(variantRecord->Layout) || !reflection.EnumerateBindings(reflectedBindings))
            {
                Delete(runtimeProgram);
                error = String::Format(SE_TEXT("Material Program '{0}' has invalid Reflection."), contract.Selection.ProgramId);
                return nullptr;
            }

            for (int32 reflectedIndex = 0; reflectedIndex < reflectedBindings.Count(); reflectedIndex++)
            {
                const ShaderProgramBinding& reflected = reflectedBindings[reflectedIndex];
                MaterialRuntimeBindingWrite write;
                write.Binding = reflected;
                if (IsMaterialPath(reflected.Path))
                {
                    if (reflected.IsArrayElement)
                    {
                        Delete(runtimeProgram);
                        error = String::Format(SE_TEXT("Material parameter arrays are not supported: {0}"), reflected.Path);
                        return nullptr;
                    }

                    int32 matchIndex = -1;
                    for (int32 bindingIndex = 0; bindingIndex < candidate->m_Bindings.Count(); bindingIndex++)
                    {
                        if (candidate->m_Bindings[bindingIndex].ShaderPath == reflected.Path)
                        {
                            matchIndex = bindingIndex;
                            break;
                        }
                    }
                    if (matchIndex < 0 || candidate->m_Bindings[matchIndex].BindingKind != reflected.Kind)
                    {
                        Delete(runtimeProgram);
                        error = String::Format(SE_TEXT("Reflected Material binding '{0}' has no exact global binding."), reflected.Path);
                        return nullptr;
                    }
                    const MaterialParameter* parameter = parameters.Get(candidate->m_Bindings[matchIndex].ParameterID);
                    if (!ValidateDirectParameterBinding(*parameter, reflected, candidate->m_BindingProvider, error))
                    {
                        Delete(runtimeProgram);
                        return nullptr;
                    }
                    write.UsesMaterialParameter = true;
                    write.ParameterID = candidate->m_Bindings[matchIndex].ParameterID;
                }
                else if (IsPerViewPath(reflected.Path) || IsPerDrawPath(reflected.Path))
                {
                    const MaterialRuntimeBindingSource source = IsPerViewPath(reflected.Path) ? MaterialRuntimeBindingSource::PerView : MaterialRuntimeBindingSource::PerDraw;
                    if (candidate->m_BindingProvider == nullptr || !candidate->m_BindingProvider->SupportsBinding(source, MaterialParameterType::Invalid, reflected))
                    {
                        Delete(runtimeProgram);
                        error = String::Format(SE_TEXT("System binding '{0}' has no provider."), reflected.Path);
                        return nullptr;
                    }
                    write.Source = source;
                }
                else
                {
                    Delete(runtimeProgram);
                    error = String::Format(SE_TEXT("Unknown Material runtime binding namespace: {0}"), reflected.Path);
                    return nullptr;
                }
                runtimeProgram->Bindings.Add(MoveTemp(write));
            }
            candidate->m_Programs.Add(runtimeProgram);
        }

        // 当前阶段的 Surface 契约必须一次性覆盖两种 Pass 与两种 GeometryMode，避免运行时才发现半套 Generation。
        if (candidate->FindProgram(DrawPass::GBuffer, MaterialGeometryMode::Static) == nullptr ||
            candidate->FindProgram(DrawPass::GBuffer, MaterialGeometryMode::Instanced) == nullptr ||
            candidate->FindProgram(DrawPass::Forward, MaterialGeometryMode::Static) == nullptr ||
            candidate->FindProgram(DrawPass::Forward, MaterialGeometryMode::Instanced) == nullptr)
        {
            error = SE_TEXT("Material Program contracts do not cover all supported pass/geometry combinations.");
            return nullptr;
        }

        if (GPUDevice::instance == nullptr)
        {
            error = SE_TEXT("GPU device is not available for Material SLC2 runtime installation.");
            return nullptr;
        }
        candidate->m_Shader = GPUDevice::instance->CreateSLC2Shader(SE_TEXT("MaterialRuntimeGeneration"));
        if (candidate->m_Shader == nullptr || !candidate->m_Shader->Load(data.SLC2Data))
        {
            error = SE_TEXT("Failed to load the validated Material SLC2 artifact.");
            return nullptr;
        }

        // 第二阶段创建共享后端 Program 与 Pipeline；失败仍只销毁候选 Generation。
        for (int32 programIndex = 0; programIndex < candidate->m_Programs.Count(); programIndex++)
        {
            MaterialRuntimeProgram* runtimeProgram = candidate->m_Programs[programIndex];
            ShaderProgramInstance temporaryInstance;
            if (!candidate->m_Shader->CreateProgramInstance(runtimeProgram->Contract.Selection, temporaryInstance))
            {
                error = String::Format(SE_TEXT("Failed to create Material Program '{0}'."), runtimeProgram->Contract.Selection.ProgramId);
                return nullptr;
            }
            runtimeProgram->Program = temporaryInstance.GetProgram();
            runtimeProgram->Pipeline = SLC2GPUPipelineState::New();
            if (runtimeProgram->Pipeline == nullptr || !runtimeProgram->Pipeline->Init(runtimeProgram->Contract.PipelineDescription, runtimeProgram->Program))
            {
                error = String::Format(SE_TEXT("Failed to create Material Pipeline for Program '{0}'."), runtimeProgram->Contract.Selection.ProgramId);
                return nullptr;
            }
        }

        candidate->m_ID = ++NextMaterialRuntimeGenerationID;
        return candidate;
    }

    uint64 MaterialRuntimeGeneration::GetID() const
    {
        return m_ID;
    }

    const MaterialRuntimeProgram* MaterialRuntimeGeneration::FindProgram(const DrawPass pass, const MaterialGeometryMode geometryMode) const
    {
        const MaterialRuntimeProgram* result = nullptr;
        for (int32 index = 0; index < m_Programs.Count(); index++)
        {
            if (m_Programs[index]->Contract.Pass == pass && m_Programs[index]->Contract.GeometryMode == geometryMode)
            {
                if (result != nullptr)
                {
                    return nullptr;
                }
                result = m_Programs[index];
            }
        }
        return result;
    }

    bool MaterialRuntimeGeneration::Prepare(const MaterialDrawContext& context, const List<const MaterialParams*>& parameterChain, MaterialProgramInstanceCache& cache, MaterialSLC2Binding& output)
    {
        output.Clear();
        if (parameterChain.IsEmpty())
        {
            LOG_ERROR("Material", "SLC2 Material preparation requires a non-empty parameter override chain.");
            return false;
        }
        const MaterialRuntimeProgram* runtimeProgram = FindProgram(context.Pass, context.GeometryMode);
        if (runtimeProgram == nullptr)
        {
            LOG_ERROR("Material", "No unique SLC2 Program contract matches pass {0} and geometry mode {1}.", static_cast<int32>(context.Pass), static_cast<int32>(context.GeometryMode));
            return false;
        }

        if (cache.m_Entries.HasItems() && cache.m_Entries[0]->Generation->GetID() != m_ID)
        {
            cache.Clear();
        }
        MaterialProgramInstanceCacheEntry* entry = nullptr;
        for (int32 index = 0; index < cache.m_Entries.Count(); index++)
        {
            if (cache.m_Entries[index]->Program == runtimeProgram)
            {
                entry = cache.m_Entries[index];
                break;
            }
        }
        if (entry == nullptr)
        {
            SharedPtr<MaterialRuntimeGeneration> generation = m_Self.lock();
            if (generation == nullptr)
            {
                LOG_ERROR("Material", "Material RuntimeGeneration is no longer owned by its active snapshot.");
                return false;
            }
            entry = New<MaterialProgramInstanceCacheEntry>();
            // CacheEntry 持有创建它的 Generation，保证旧 ProgramInstance 退出前 Shader 数据仍然有效。
            entry->Generation = MoveTemp(generation);
            entry->Program = runtimeProgram;
            if (!m_Shader->CreateProgramInstance(runtimeProgram->Contract.Selection, entry->Instance))
            {
                Delete(entry);
                LOG_ERROR("Material", "Failed to create ProgramInstance for Program '{0}'.", runtimeProgram->Contract.Selection.ProgramId);
                return false;
            }
            cache.m_Entries.Add(entry);
        }

        for (int32 reflectedIndex = 0; reflectedIndex < runtimeProgram->Bindings.Count(); reflectedIndex++)
        {
            const MaterialRuntimeBindingWrite& write = runtimeProgram->Bindings[reflectedIndex];
            const ShaderProgramBinding& reflected = write.Binding;
            bool written = false;
            if (write.UsesMaterialParameter)
            {
                const MaterialParameter* parameter = ResolveMaterialParameter(parameterChain, write.ParameterID);
                written = parameter != nullptr && WriteDirectParameter(*parameter, reflected, context, m_BindingProvider, entry->Instance);
            }
            else
            {
                written = m_BindingProvider != nullptr && m_BindingProvider->WriteBinding(write.Source, nullptr, reflected, context, entry->Instance);
            }
            if (!written)
            {
                LOG_ERROR("Material", "Failed to write SLC2 Material binding '{0}' for Program '{1}'.", reflected.Path, runtimeProgram->Contract.Selection.ProgramId);
                return false;
            }
        }

        ShaderBindingSnapshot snapshot;
        if (!entry->Instance.BuildSnapshot(snapshot))
        {
            LOG_ERROR("Material", "SLC2 Material Program '{0}' has incomplete resource bindings.", runtimeProgram->Contract.Selection.ProgramId);
            return false;
        }

        output.ProgramInstance = &entry->Instance;
        output.Pipeline = runtimeProgram->Pipeline;
        output.GenerationID = m_ID;
        return true;
    }
}
