#pragma once

#include "Runtime/API.h"
#include "Runtime/Core/Types/Collections/List.h"

namespace SE
{
    class GPUBuffer;
    class GPUConstantBuffer;
    class GPUSampler;
    class GPUTexture;
    class GPUTextureView;
    class GPUResource;
    class ScriptingObject;
    class SLC2GPUShaderProgram;
    class ShaderProgramInstance;

    enum class ShaderRuntimeResourceType : byte
    {
        Texture,
        Buffer,
        Sampler,
    };

    // One dense logical resource slot. The tagged pointers are never exposed as void* and are
    // invalidated by both GPU release and scripting-object deletion notifications.
    class SE_API_RUNTIME ShaderBindingResource
    {
    public:
        explicit ShaderBindingResource(ShaderRuntimeResourceType type);
        ~ShaderBindingResource();

        ShaderRuntimeResourceType GetType() const;
        GPUTextureView*           GetTexture() const;
        GPUBuffer*                GetBuffer() const;
        GPUSampler*               GetSampler() const;
        bool                      IsValid() const;

        NON_COPYABLE(ShaderBindingResource)

    private:
        friend class ShaderProgramInstance;
        friend class ShaderBindingSnapshot;

        bool CloneFrom(const ShaderBindingResource& other);
        void SetTexture(GPUTextureView* view, GPUTexture* parent);
        void SetBuffer(GPUBuffer* buffer);
        void SetSampler(GPUSampler* sampler);
        void Clear();
        void OnResourceReleasing();
        void OnObjectDeleted(ScriptingObject* object);

        ShaderRuntimeResourceType m_Type;
        GPUTextureView*           m_Texture      = nullptr;
        GPUTexture*               m_TextureOwner = nullptr;
        GPUBuffer*                m_Buffer       = nullptr;
        GPUSampler*               m_Sampler      = nullptr;
    };

    enum class ShaderUniformBlockSource : byte
    {
        InternalData,
        ExternalConstantBuffer,
    };

    struct SE_API_RUNTIME ShaderBindingUniformData
    {
        uint32                   UniformBlockIndex = 0;
        ShaderUniformBlockSource Source            = ShaderUniformBlockSource::InternalData;
        List<byte>               Data;
        GPUConstantBuffer*       ExternalBuffer    = nullptr;
    };

	// 平台无关提交边界。后端只能读取此快照，不能回读或修改 Instance 的可变状态。
    // InternalData 会复制到快照；ExternalConstantBuffer 只冻结对象身份，其 Allocation 由后端提交时读取。
    class SE_API_RUNTIME ShaderBindingSnapshot
    {
    public:
        ShaderBindingSnapshot() = default;
        ShaderBindingSnapshot(ShaderBindingSnapshot&& other) noexcept;
        ShaderBindingSnapshot& operator=(ShaderBindingSnapshot&& other) noexcept;
        ~ShaderBindingSnapshot();

        NON_COPYABLE(ShaderBindingSnapshot)

        bool                                  IsFrozen() const;
        // Valid only for a frozen snapshot; the pointer must remain alive through backend descriptor submission.
        const SLC2GPUShaderProgram*           GetProgram() const;
        uint32                                GetResourceCount() const;
        const ShaderBindingResource*          GetResource(uint32 logicalResourceSlotIndex) const;
        const List<ShaderBindingUniformData>& GetUniformData() const;
        // Returns the frozen dense entry for a UniformBlockIndex, or nullptr when the index is absent.
        const ShaderBindingUniformData*       GetUniformData(uint32 uniformBlockIndex) const;

    private:
        friend class ShaderProgramInstance;

        bool AddResource(const ShaderBindingResource& resource);
        void AddUniformData(uint32 uniformBlockIndex, const List<byte>& data);
        void AddExternalUniformBuffer(uint32 uniformBlockIndex, GPUConstantBuffer* buffer);
        void Freeze(const SLC2GPUShaderProgram* program);

        bool                           _frozen   = false;
        const SLC2GPUShaderProgram*    m_Program = nullptr;
        List<ShaderBindingResource*>   _resources;
        List<ShaderBindingUniformData> _uniformData;
    };
} // namespace SE
