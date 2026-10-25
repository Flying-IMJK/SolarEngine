#include "ShaderBindingSnapshot.h"

#include "Runtime/Core/Memory/Memory.h"
#include "Runtime/Graphics/Base/GPUBuffer.h"
#include "Runtime/Graphics/Textures/GPUSampler.h"
#include "Runtime/Graphics/Textures/GPUTexture.h"

namespace SE
{
    ShaderBindingResource::ShaderBindingResource(const ShaderRuntimeResourceType type) : m_Type(type)
    {}

    ShaderBindingResource::~ShaderBindingResource()
    {
        Clear();
    }

    ShaderRuntimeResourceType ShaderBindingResource::GetType() const { return m_Type; }

    GPUTextureView* ShaderBindingResource::GetTexture() const { return m_Texture; }

    GPUBuffer* ShaderBindingResource::GetBuffer() const { return m_Buffer; }

    GPUSampler* ShaderBindingResource::GetSampler() const { return m_Sampler; }

    bool ShaderBindingResource::IsValid() const
    {
        switch (m_Type)
        {
        case ShaderRuntimeResourceType::Texture:
            return m_Texture != nullptr && m_TextureOwner != nullptr && m_TextureOwner->IsAllocated() &&
                m_Texture->GetParent() == m_TextureOwner && m_Texture->GetNativePtr() != nullptr;
        case ShaderRuntimeResourceType::Buffer:
            return m_Buffer != nullptr && m_Buffer->IsAllocated() && m_Buffer->View() != nullptr &&
                m_Buffer->View()->GetNativePtr() != nullptr;
        case ShaderRuntimeResourceType::Sampler:
            return m_Sampler != nullptr && m_Sampler->IsInitialized();
        default:
            return false;
        }
    }

    bool ShaderBindingResource::CloneFrom(const ShaderBindingResource& other)
    {
        if (m_Type != other.m_Type || !other.IsValid())
        {
            return false;
        }
        switch (m_Type)
        {
        case ShaderRuntimeResourceType::Texture:
            SetTexture(other.m_Texture, other.m_TextureOwner);
            break;
        case ShaderRuntimeResourceType::Buffer:
            SetBuffer(other.m_Buffer);
            break;
        case ShaderRuntimeResourceType::Sampler:
            SetSampler(other.m_Sampler);
            break;
        default:
            return false;
        }
        return true;
    }

    void ShaderBindingResource::SetTexture(GPUTextureView* view, GPUTexture* parent)
    {
        Clear();
        m_Texture      = view;
        m_TextureOwner = parent;
        m_Texture->Deleted.Bind<ShaderBindingResource, &ShaderBindingResource::OnObjectDeleted>(this);
        m_TextureOwner->Deleted.Bind<ShaderBindingResource, &ShaderBindingResource::OnObjectDeleted>(this);
        m_TextureOwner->Releasing.Bind<ShaderBindingResource, &ShaderBindingResource::OnResourceReleasing>(this);
    }


    void ShaderBindingResource::SetBuffer(GPUBuffer* buffer)
    {
        Clear();
        m_Buffer = buffer;
        m_Buffer->Deleted.Bind<ShaderBindingResource, &ShaderBindingResource::OnObjectDeleted>(this);
        m_Buffer->Releasing.Bind<ShaderBindingResource, &ShaderBindingResource::OnResourceReleasing>(this);
    }

    void ShaderBindingResource::SetSampler(GPUSampler* sampler)
    {
        Clear();
        m_Sampler = sampler;
        m_Sampler->Deleted.Bind<ShaderBindingResource, &ShaderBindingResource::OnObjectDeleted>(this);
        m_Sampler->Releasing.Bind<ShaderBindingResource, &ShaderBindingResource::OnResourceReleasing>(this);
    }

    void ShaderBindingResource::Clear()
    {
        if (m_Texture != nullptr)
        {
            m_Texture->Deleted.Unbind<ShaderBindingResource, &ShaderBindingResource::OnObjectDeleted>(this);
        }
        if (m_TextureOwner != nullptr)
        {
            m_TextureOwner->Deleted.Unbind<ShaderBindingResource, &ShaderBindingResource::OnObjectDeleted>(this);
            m_TextureOwner->Releasing.Unbind<ShaderBindingResource, &ShaderBindingResource::OnResourceReleasing>(this);
        }
        if (m_Buffer != nullptr)
        {
            m_Buffer->Deleted.Unbind<ShaderBindingResource, &ShaderBindingResource::OnObjectDeleted>(this);
            m_Buffer->Releasing.Unbind<ShaderBindingResource, &ShaderBindingResource::OnResourceReleasing>(this);
        }
        if (m_Sampler != nullptr)
        {
            m_Sampler->Deleted.Unbind<ShaderBindingResource, &ShaderBindingResource::OnObjectDeleted>(this);
            m_Sampler->Releasing.Unbind<ShaderBindingResource, &ShaderBindingResource::OnResourceReleasing>(this);
        }
        m_Texture      = nullptr;
        m_TextureOwner = nullptr;
        m_Buffer       = nullptr;
        m_Sampler      = nullptr;
    }

    void ShaderBindingResource::OnResourceReleasing()
    {
        Clear();
    }

    void ShaderBindingResource::OnObjectDeleted(ScriptingObject* object)
    {
        (void)object;
        Clear();
    }

    ShaderBindingSnapshot::ShaderBindingSnapshot(ShaderBindingSnapshot&& other) noexcept
    {
        *this = MoveTemp(other);
    }

    ShaderBindingSnapshot& ShaderBindingSnapshot::operator=(ShaderBindingSnapshot&& other) noexcept
    {
        if (this != &other)
        {
            _resources.ClearDelete();
            _resources  = MoveTemp(other._resources);
            _uniformData = MoveTemp(other._uniformData);
            _frozen      = other._frozen;
            m_Program    = other.m_Program;
            other._frozen   = false;
            other.m_Program = nullptr;
        }
        return *this;
    }

    ShaderBindingSnapshot::~ShaderBindingSnapshot()
    {
        _resources.ClearDelete();
    }

    bool ShaderBindingSnapshot::IsFrozen() const { return _frozen; }

    const SLC2GPUShaderProgram* ShaderBindingSnapshot::GetProgram() const { return m_Program; }

    uint32 ShaderBindingSnapshot::GetResourceCount() const { return static_cast<uint32>(_resources.Count()); }

    const ShaderBindingResource* ShaderBindingSnapshot::GetResource(const uint32 logicalResourceSlotIndex) const
    {
        return logicalResourceSlotIndex < static_cast<uint32>(_resources.Count())
            ? _resources[logicalResourceSlotIndex]
            : nullptr;
    }

    const List<ShaderBindingUniformData>& ShaderBindingSnapshot::GetUniformData() const { return _uniformData; }

    const ShaderBindingUniformData* ShaderBindingSnapshot::GetUniformData(const uint32 uniformBlockIndex) const
    {
        if (uniformBlockIndex >= static_cast<uint32>(_uniformData.Count()) ||
            _uniformData[uniformBlockIndex].UniformBlockIndex != uniformBlockIndex)
        {
            return nullptr;
        }
        return &_uniformData[uniformBlockIndex];
    }

    bool ShaderBindingSnapshot::AddResource(const ShaderBindingResource& resource)
    {
        ShaderBindingResource* copy = New<ShaderBindingResource>(resource.GetType());
        if (!copy->CloneFrom(resource))
        {
            Delete(copy);
            return false;
        }
        _resources.Add(copy);
        return true;
    }

    void ShaderBindingSnapshot::AddUniformData(const uint32 uniformBlockIndex, const List<byte>& data)
    {
        ShaderBindingUniformData uniformData;
        uniformData.UniformBlockIndex = uniformBlockIndex;
        uniformData.Source            = ShaderUniformBlockSource::InternalData;
        uniformData.Data.Resize(data.Count());
        if (data.HasItems())
        {
			// 快照保存 uniform 数据副本，避免后端写 descriptor 时依赖仍可修改的 Instance shadow。
            Platform::MemoryCopy(uniformData.Data.Get(), data.Get(), data.Count());
        }
        _uniformData.Add(MoveTemp(uniformData));
    }

    void ShaderBindingSnapshot::AddExternalUniformBuffer(const uint32 uniformBlockIndex, GPUConstantBuffer* buffer)
    {
        ShaderBindingUniformData uniformData;
        uniformData.UniformBlockIndex = uniformBlockIndex;
        uniformData.Source            = ShaderUniformBlockSource::ExternalConstantBuffer;
        uniformData.ExternalBuffer    = buffer;
        _uniformData.Add(MoveTemp(uniformData));
    }

    void ShaderBindingSnapshot::Freeze(const SLC2GPUShaderProgram* program)
    {
        m_Program = program;
        _frozen   = true;
    }
} // namespace SE
