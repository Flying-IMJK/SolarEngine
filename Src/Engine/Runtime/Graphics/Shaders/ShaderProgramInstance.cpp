#include "ShaderProgramInstance.h"

#include <Runtime/Core/Math/Vector2.h>
#include <Runtime/Core/Math/Vector3.h>
#include <Runtime/Core/Math/Vector4.h>
#include <Runtime/Core/Memory/Memory.h>
#include <Runtime/Graphics/Base/GPUBuffer.h>
#include <Runtime/Graphics/GPUDevice.h>
#include <Runtime/Graphics/Shaders/GPUConstantBuffer.h>
#include <Runtime/Graphics/Shaders/Reflection/ShaderNameResolver.h>
#include <Runtime/Graphics/Shaders/Reflection/ShaderProgramReflection.h>
#include <Runtime/Graphics/Shaders/SLC2GPUShader.h>
#include <Runtime/Graphics/Shaders/SLC2GPUShaderProgram.h>
#include <Runtime/Graphics/Textures/GPUSampler.h>
#include <Runtime/Graphics/Textures/GPUTexture.h>

namespace SE
{
    namespace
    {
        bool RequiresWriteAccess(const ShaderIRTypeRecord& type)
        {
            // Slang ResourceAccess::Read is serialized as 1. Every other non-empty access mode
            // represents a writable/UAV-capable resource in the current reflection contract.
            return !type.Access.IsEmpty() && type.Access != SE_TEXT("1");
        }

        bool IsTextureResource(const GPUResource* resource)
        {
            if (resource == nullptr)
            {
                return false;
            }
            const GPUResourceType type = resource->GetResType();
            return type == GPUResourceType::Texture || type == GPUResourceType::CubeTexture ||
                type == GPUResourceType::VolumeTexture || type == GPUResourceType::RenderTarget;
        }

    }

    ShaderProgramInstance::ShaderProgramInstance() : ScriptingObject(SpawnParams(UID::New(), ShaderProgramInstance::TypeInitializer))
    {
    }

    ShaderProgramInstance::~ShaderProgramInstance()
    {
        ClearResources();
    }

    bool ShaderProgramInstance::Initialize(SLC2GPUShaderProgram* program)
    {
        m_Program = nullptr;
        ClearResources();
        m_UniformBlocks.Clear();
        if (program == nullptr)
        {
            LOG_ERROR("Graphics", "Shader program instance requires a selected program.");
            return false;
        }
        const ShaderProgramReflection& reflection = program->GetReflection();
        const uint32 resourceCount = reflection.GetLogicalResourceSlotCount();
        m_Resources.SetCapacity(static_cast<int32>(resourceCount), false);

        for (uint32 resourceIndex = 0; resourceIndex < resourceCount; resourceIndex++)
        {
            const ShaderLogicalResourceSlot* reflectedSlot = reflection.GetLogicalResourceSlot(resourceIndex);
            if (reflectedSlot == nullptr || reflectedSlot->TypeId == Max_uint32)
            {
                LOG_ERROR("Graphics", "Shader program contains an invalid logical resource slot.");
                ClearResources();
                return false;
            }
            m_Resources.Add(New<ShaderBindingResource>(reflectedSlot->Kind));
        }

        const uint32 uniformBlockCount = reflection.GetUniformBlockCount();
        m_UniformBlocks.Resize(static_cast<int32>(uniformBlockCount));
        for (uint32 uniformBlockIndex = 0; uniformBlockIndex < uniformBlockCount; uniformBlockIndex++)
        {
            const ShaderUniformBlockLayout* layout = reflection.GetUniformBlockLayout(uniformBlockIndex);
            if (layout == nullptr || layout->ByteSize == 0)
            {
                LOG_ERROR("Graphics", "Shader program contains an invalid uniform block layout.");
                m_UniformBlocks.Clear();
                ClearResources();
                return false;
            }
            // List<byte> 的新元素会清零，Instance 初始 shadow 因而具有确定内容。
            m_UniformBlocks[uniformBlockIndex].Data.Resize(static_cast<int32>(layout->ByteSize));
        }
        m_Program = program;
        return true;
    }

    ShaderVar ShaderProgramInstance::GetRootVar()
    {
        if (m_Program == nullptr)
        {
            return ShaderVar();
        }
        ShaderVarLocation             location;
        const ShaderReflectionIR&     layout = m_Program->GetReflection().GetLayout();
        const ShaderIRParameterBlock* root   = m_Program->GetReflection().GetBlock(layout.RootBlockId);
        if (root == nullptr)
        {
            LOG_ERROR("Graphics", "Shader program reflection root is not valid.");
            return ShaderVar();
        }
        location.BlockId = root->Id;
        location.TypeId  = root->ElementTypeId;
        uint32 uniformBlockIndex;
        if (m_Program->GetReflection().FindUniformBlockIndex(root->Id, uniformBlockIndex))
        {
            // 根变量代表参数作用域；只有存在 ordinary data 时它才同时代表可写的 Global ConstantBuffer。
            location.Kind              = ShaderParameterAddressKind::UniformBlock;
            location.UniformBlockIndex = static_cast<int32>(uniformBlockIndex);
            location.UniformByteOffset = 0;
            location.UniformByteSize   = root->UniformByteSize;
        }
        location.ResourceRangeIndex = -1;
        location.ResourceIndex      = -1;
        return ShaderVar(this, &m_Program->GetReflection(), location);
    }

    bool ShaderProgramInstance::FindVar(const String& path, ShaderVar& result)
    {
        if (m_Program == nullptr)
        {
            LOG_ERROR("Graphics", "Shader program instance is not initialized.");
            return false;
        }

        ShaderVarLocation location;

        String msg;
        if (!ShaderNameResolver::Resolve(m_Program->GetReflection(), path, location, msg))
        {
            return false;
        }

        result = ShaderVar(this, &m_Program->GetReflection(), location);
        return true;
    }

    bool ShaderProgramInstance::SetTexture(const String& path, GPUTextureView* value)
    {
        ShaderVar var;
        return FindVar(path, var) && var.SetTexture(value);
    }

    bool ShaderProgramInstance::SetBuffer(const String& path, GPUBuffer* value)
    {
        ShaderVar var;
        return FindVar(path, var) && var.SetBuffer(value);
    }

    bool ShaderProgramInstance::SetSampler(const String& path, GPUSampler* value)
    {
        ShaderVar var;
        return FindVar(path, var) && var.SetSampler(value);
    }

    bool ShaderProgramInstance::SetInt(const String& path, const int32 value)
    {
        return SetUniform(path, &value, sizeof(value));
    }

    bool ShaderProgramInstance::SetUInt(const String& path, const uint32 value)
    {
        return SetUniform(path, &value, sizeof(value));
    }

    bool ShaderProgramInstance::SetFloat(const String& path, const float value)
    {
        return SetUniform(path, &value, sizeof(value));
    }

    bool ShaderProgramInstance::SetFloat2(const String& path, Float2& value)
    {
        return SetUniform(path, &value, sizeof(value));
    }

    bool ShaderProgramInstance::SetFloat3(const String& path, Float3& value)
    {
        return SetUniform(path, &value, sizeof(value));
    }

    bool ShaderProgramInstance::SetFloat4(const String& path, Float4& value)
    {
        return SetUniform(path, &value, sizeof(value));
    }

    bool ShaderProgramInstance::SetBool(const String& path, const bool value)
    {
        const uint32 encodedValue = value ? 1u : 0u;
        return SetUniform(path, &encodedValue, sizeof(encodedValue));
    }

    bool ShaderProgramInstance::SetUniform(const String& path, const void* data, const uint32 size)
    {
        ShaderVar var;
        return FindVar(path, var) && var.SetUniform(data, size);
    }

    bool ShaderProgramInstance::SetConstantBuffer(const String& path, const void* data, const uint32 size)
    {
        if (m_Program == nullptr)
        {
            LOG_ERROR("Graphics", "Shader program instance is not initialized.");
            return false;
        }
        ShaderVarLocation location;
        String msg;
        if (!ShaderNameResolver::ResolveUniformBlock(m_Program->GetReflection(), path, location, msg))
        {
            LOG_ERROR("Graphics", "{0}", msg);
        }

        if (location.UniformBlockIndex < 0 || location.UniformBlockIndex >= m_UniformBlocks.Count() || data == nullptr)
        {
            return false;
        }

        ShaderUniformBlockState& state = m_UniformBlocks[location.UniformBlockIndex];
        if (size != static_cast<uint32>(state.Data.Count()))
        {
            LOG_ERROR("Graphics", "Constant buffer write size does not match the reflected uniform block size.");
            return false;
        }
        Platform::MemoryCopy(state.Data.Get(), data, size);
        state.Source         = ShaderUniformBlockSource::InternalData;
        state.ExternalBuffer = nullptr;
        return true;
    }

    bool ShaderProgramInstance::SetConstantBuffer(const String& path, GPUConstantBuffer* buffer)
    {
        if (m_Program == nullptr)
        {
            LOG_ERROR("Graphics", "Shader program instance is not initialized.");
            return false;
        }
        ShaderVarLocation location;
        String msg;
        if (!ShaderNameResolver::ResolveUniformBlock(m_Program->GetReflection(), path, location, msg))
        {
            LOG_ERROR("Graphics", "{0}", msg);
        }

        if (location.UniformBlockIndex < 0 || location.UniformBlockIndex >= m_UniformBlocks.Count() ||
            buffer == nullptr)
        {
            return false;
        }

        ShaderUniformBlockState& state = m_UniformBlocks[location.UniformBlockIndex];
        if (buffer->GetSize() < static_cast<uint32>(state.Data.Count()))
        {
            LOG_ERROR("Graphics", "External constant buffer is smaller than the reflected uniform block size.");
            return false;
        }

        // Validation is complete before switching the active source, so failure preserves the previous mode.
        state.Source         = ShaderUniformBlockSource::ExternalConstantBuffer;
        state.ExternalBuffer = buffer;
        return true;
    }

    bool ShaderProgramInstance::BuildSnapshot(ShaderBindingSnapshot& snapshot) const
    {
        if (m_Program == nullptr)
        {
            LOG_ERROR("Graphics", "Shader program instance is not initialized.");
            return false;
        }

        ShaderBindingSnapshot pending;
        const ShaderProgramReflection& reflection = m_Program->GetReflection();
        if (reflection.GetLogicalResourceSlotCount() != static_cast<uint32>(m_Resources.Count()))
        {
            LOG_ERROR("Graphics", "Shader instance resource state does not match its program reflection.");
            return false;
        }

        // 先完成全部验证与复制；失败时调用方原有 snapshot 保持不变，不暴露部分结果。
        GPUDevice* device = GPUDevice::instance;
        for (int32 resourceIndex = 0; resourceIndex < m_Resources.Count(); resourceIndex++)
        {
            const ShaderBindingResource* resource = m_Resources[resourceIndex];
            const ShaderLogicalResourceSlot* reflectedSlot = reflection.GetLogicalResourceSlot(static_cast<uint32>(resourceIndex));
            if (reflectedSlot == nullptr)
            {
                LOG_ERROR("Graphics", "Shader logical resource slot {0} has no valid reflection.", resourceIndex);
                return false;
            }

            const ShaderIRTypeRecord* type = reflection.GetType(reflectedSlot->TypeId);
            if (resource == nullptr || resource->GetType() != reflectedSlot->Kind || type == nullptr)
            {
                LOG_ERROR("Graphics", "Shader logical resource '{0}' (slot {1}) has an invalid type.",
                          reflectedSlot->Path, resourceIndex);
                return false;
            }

            // Bind the dummy resource without setting any resources.
            if (!resource->IsValid())
            {
                // Resolve only the snapshot copy so an unbound instance remains unbound for later submissions.
                ShaderBindingResource dummy(reflectedSlot->Kind);
                switch (reflectedSlot->Kind)
                {
                    case ShaderProgramBindingKind::Texture:
                        if (resource->GetTexture() == nullptr)
                        {
                            GPUTextureView* view = device->GetDummyTextureView(*type);
                            dummy.SetTexture(view, static_cast<GPUTexture*>(view->GetParent()));
                        }
                        break;
                    case ShaderProgramBindingKind::Buffer:
                        if (resource->GetBuffer() == nullptr)
                        {
                            dummy.SetBuffer(device->GetDummyBuffer());
                        }
                        break;
                    case ShaderProgramBindingKind::Sampler:
                        if (resource->GetSampler() == nullptr)
                        {
                            GPUSampler* sampler = device->GetDummySampler(type->Name == SE_TEXT("SamplerComparisonState"));
                            dummy.SetSampler(sampler);
                        }
                        break;
                    default:
                        return false;
                }

                resource = &dummy;
            }



            if (!resource->IsValid() || !pending.AddResource(*resource))
            {
                LOG_ERROR("Graphics", "Shader logical resource '{0}' (slot {1}) is invalid.", reflectedSlot->Path, resourceIndex);
                return false;
            }
        }

        if (reflection.GetUniformBlockCount() != static_cast<uint32>(m_UniformBlocks.Count()))
        {
            LOG_ERROR("Graphics", "Shader instance uniform block state does not match its program reflection.");
            return false;
        }

        for (int32 uniformBlockIndex = 0; uniformBlockIndex < m_UniformBlocks.Count(); uniformBlockIndex++)
        {
            const ShaderUniformBlockLayout* layout = reflection.GetUniformBlockLayout(static_cast<uint32>(uniformBlockIndex));
            const ShaderUniformBlockState& state = m_UniformBlocks[uniformBlockIndex];

            if (layout == nullptr || layout->ByteSize != static_cast<uint32>(state.Data.Count()))
            {
                LOG_ERROR("Graphics", "Shader instance uniform block size does not match its program reflection.");
                return false;
            }

            if (state.Source == ShaderUniformBlockSource::InternalData)
            {
                pending.AddUniformData(static_cast<uint32>(uniformBlockIndex), state.Data);
            }
            else
            {
                if (state.ExternalBuffer == nullptr || state.ExternalBuffer->GetSize() < layout->ByteSize)
                {
                    LOG_ERROR("Graphics", "External constant buffer does not satisfy its reflected uniform block.");
                    return false;
                }
                pending.AddExternalUniformBuffer(static_cast<uint32>(uniformBlockIndex), state.ExternalBuffer);
            }
        }

        pending.Freeze(m_Program);
        snapshot = MoveTemp(pending);
        return true;
    }

    SLC2GPUShaderProgram* ShaderProgramInstance::GetProgram() const { return m_Program; }


    ShaderBindingResource* ShaderProgramInstance::GetResourceSlot(const ShaderVarLocation& location,
                                                                  const ShaderProgramBindingKind expectedKind) const
    {
        if (m_Program == nullptr || location.Kind != ShaderParameterAddressKind::Resource ||
            location.LogicalResourceSlotIndex < 0 || location.LogicalResourceSlotIndex >= m_Resources.Count())
        {
            LOG_ERROR("Graphics", "Shader variable does not identify a logical resource slot.");
            return nullptr;
        }

        const ShaderLogicalResourceSlot* reflectedSlot = m_Program->GetReflection().GetLogicalResourceSlot(static_cast<uint32>(location.LogicalResourceSlotIndex));
        if (reflectedSlot == nullptr || reflectedSlot->Kind != expectedKind || reflectedSlot->TypeId != location.TypeId)
        {
            LOG_ERROR("Graphics", "Shader resource setter does not match the reflected logical slot.");
            return nullptr;
        }

        return m_Resources[location.LogicalResourceSlotIndex];
    }

    bool ShaderProgramInstance::SetTexture(const ShaderVarLocation& location, GPUTextureView* value)
    {
        ShaderBindingResource* slot = GetResourceSlot(location, ShaderProgramBindingKind::Texture);
        if (slot != nullptr && value == nullptr)
        {
            slot->SetTexture(nullptr, nullptr);
            return true;
        }
        if (slot == nullptr || value->GetParent() == nullptr ||
            !IsTextureResource(value->GetParent()))
        {
            LOG_ERROR("Graphics", "Texture binding requires a valid texture view and parent texture.");
            return false;
        }
        GPUTexture* parent = static_cast<GPUTexture*>(value->GetParent());
        const ShaderIRTypeRecord* type = m_Program->GetReflection().GetType(location.TypeId);
        if (type == nullptr || 
            !parent->IsAllocated() || 
            value->GetNativePtr() == nullptr ||
            (RequiresWriteAccess(*type) ? !parent->IsUnorderedAccess() : !parent->IsShaderResource()))
        {
            LOG_ERROR("Graphics", "Texture binding does not satisfy the reflected access capability.");
            return false;
        }

        // Validation is complete before the tracked slot replaces its previous binding.
        slot->SetTexture(value, parent);
        return true;
    }


    bool ShaderProgramInstance::SetBuffer(const ShaderVarLocation& location, GPUBuffer* value)
    {
        ShaderBindingResource* slot = GetResourceSlot(location, ShaderProgramBindingKind::Buffer);
        if (slot != nullptr && value == nullptr)
        {
            slot->SetBuffer(nullptr);
            return true;
        }
        const ShaderIRTypeRecord* type =
            slot != nullptr ? m_Program->GetReflection().GetType(location.TypeId) : nullptr;
        if (slot == nullptr || type == nullptr || value == nullptr || !value->IsAllocated() || value->View() == nullptr ||
            value->View()->GetNativePtr() == nullptr ||
            (RequiresWriteAccess(*type) ? !value->IsUnorderedAccess() : !value->IsShaderResource()))
        {
            LOG_ERROR("Graphics", "Buffer binding does not satisfy the reflected access capability.");
            return false;
        }

        const bool isStructured = EnumHasAnyFlags(value->GetFlags(), GPUBufferFlags::Structured);
        const bool isRaw        = EnumHasAnyFlags(value->GetFlags(), GPUBufferFlags::RawBuffer);
        const bool shapeMatches = (type->Shape == SE_TEXT("5") && !isStructured && !isRaw) ||
            (type->Shape == SE_TEXT("6") && isStructured && !isRaw) ||
            (type->Shape == SE_TEXT("7") && isRaw && !isStructured);
        if (!shapeMatches)
        {
            LOG_ERROR("Graphics", "Buffer binding does not match the reflected typed/raw/structured shape.");
            return false;
        }

		// Append/Consume is one logical structured-buffer binding whose backend view owns
		// an auxiliary counter. The caller still binds only the parent GPUBuffer.
		const bool requiresCounter = type->Access == SE_TEXT("4") || type->Access == SE_TEXT("5");
		if (requiresCounter && !EnumHasAnyFlags(value->GetFlags(), GPUBufferFlags::Append) &&
			!EnumHasAnyFlags(value->GetFlags(), GPUBufferFlags::Counter))
		{
			LOG_ERROR("Graphics", "Append/Consume buffer binding has no counter capability.");
			return false;
		}

        slot->SetBuffer(value);
        return true;
    }

    bool ShaderProgramInstance::SetSampler(const ShaderVarLocation& location, GPUSampler* value)
    {
        ShaderBindingResource* slot = GetResourceSlot(location, ShaderProgramBindingKind::Sampler);
        if (slot == nullptr || (value != nullptr && !value->IsInitialized()))
        {
            LOG_ERROR("Graphics", "Sampler binding requires an initialized sampler.");
            return false;
        }
        slot->SetSampler(value);
        return true;
    }

    bool ShaderProgramInstance::SetUniform(const ShaderVarLocation& location, const void* data, const uint32 size)
    {
        if (location.Kind != ShaderParameterAddressKind::UniformValue || location.UniformBlockIndex < 0 ||
            location.UniformBlockIndex >= m_UniformBlocks.Count() || location.UniformByteOffset < 0 ||
            data == nullptr)
        {
            LOG_ERROR("Graphics", "Shader variable does not identify writable ordinary data.");
            return false;
        }

        ShaderUniformBlockState& state = m_UniformBlocks[location.UniformBlockIndex];
        if (static_cast<uint64>(location.UniformByteOffset) + size > static_cast<uint64>(state.Data.Count()))
        {
            LOG_ERROR("Graphics", "Shader uniform write is outside the reflected uniform block.");
            return false;
        }

        if (state.Source == ShaderUniformBlockSource::ExternalConstantBuffer)
        {
            LOG_ERROR("Graphics", "Cannot write a field while its uniform block uses an external constant buffer.");
            return false;
        }

        // SetUniform 是原始字节写入；调用者选择何种 typed setter 以及写入多少字节属于接口使用契约。
        Platform::MemoryCopy(state.Data.Get() + location.UniformByteOffset, data, size);
        return true;
    }

    void ShaderProgramInstance::ClearResources()
    {
        m_Resources.ClearDelete();
    }
} // namespace SE
