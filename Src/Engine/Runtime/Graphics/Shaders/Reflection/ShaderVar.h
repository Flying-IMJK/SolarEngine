#pragma once

#include "Runtime/API.h"
#include "Runtime/Core/Types/Strings/String.h"

namespace SE
{
    class GPUBuffer;
    class GPUSampler;
    class GPUTextureView;
    class ShaderProgramInstance;
    class ShaderProgramReflection;

    enum class ShaderParameterAddressKind : byte
    {
        Invalid,
        UniformValue,
        UniformBlock,
        Resource,
    };

    // NamePath 在反射树中的临时定位结果。预计算索引用于 Instance 热路径；
    // Uniform offsets and logical slots are valid only for the immutable Reflection that produced them.
    struct SE_API_RUNTIME ShaderVarLocation
    {
        ShaderParameterAddressKind Kind                     = ShaderParameterAddressKind::Invalid;
        uint32                     BlockId                  = 0;
        int32                      UniformBlockIndex        = -1;
        int32                      UniformByteOffset        = -1;
        uint32                     UniformByteSize          = 0;
        int32                      LogicalResourceSlotIndex = -1;
        int32                      ResourceRangeIndex       = -1;
        int32                      ResourceIndex            = -1;
        uint32                     TypeId                   = 0;
    };

    // 面向上层的临时 shader 参数访问对象，不缓存跨 Setter 调用的 NamePath 地址。
    // 它不拥有数据；资源和 ordinary data 写入都转发到所属 Instance 的 dense state。
    class SE_API_RUNTIME ShaderVar
    {
    public:
        ShaderVar() = default;
        ShaderVar(ShaderProgramInstance*         instance,
                  const ShaderProgramReflection* reflection,
                  const ShaderVarLocation&       location) :
            m_Instance(instance), _reflection(reflection), _location(location)
        {}

        bool      FindMember(const String& name, ShaderVar& result) const;
        bool      GetElement(uint32 index, ShaderVar& result) const;
        ShaderVar operator[](uint32 index) const;
        bool      SetTexture(GPUTextureView* value) const;
        bool      SetBuffer(GPUBuffer* value) const;
        bool      SetSampler(GPUSampler* value) const;
        bool      SetUniform(const void* data, uint32 size) const;

        const ShaderVarLocation& GetLocation() const { return _location; }

        bool IsValid() const { return m_Instance != nullptr && _reflection != nullptr; }

    private:
        ShaderProgramInstance*         m_Instance       = nullptr;
        const ShaderProgramReflection* _reflection      = nullptr;
        ShaderVarLocation              _location;
    };
} // namespace SE
