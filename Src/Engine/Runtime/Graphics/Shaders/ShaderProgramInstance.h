#pragma once

#include "Runtime/API.h"
#include "Reflection/ShaderVar.h"
#include "ShaderBindingSnapshot.h"
#include <Runtime/Core/Scripting/ScriptingObject.h>

namespace SE
{
    class GPUBuffer;
    class GPUConstantBuffer;
    class GPUSampler;
    class GPUTextureView;
    class SLC2GPUShaderProgram;
    class ShaderProgramReflection;
    enum class ShaderProgramBindingKind : byte;

    // 一次 Shader 使用的可变参数状态。
    // 它引用不可变 Program，并分别持有 dense logical resources 与 Uniform Block tagged state。
    SE_CLASS(API(NoSpawn))
    class SE_API_RUNTIME ShaderProgramInstance : public ScriptingObject
    {
        SCRIPTING_TYPE_NO_SPAWN(ShaderProgramInstance)
    public:
        ShaderProgramInstance();
        ~ShaderProgramInstance() override;

        bool      Initialize(SLC2GPUShaderProgram* program);
        ShaderVar GetRootVar();
        bool      FindVar(const String& path, ShaderVar& result);
        // Passing nullptr clears a resource binding; BuildSnapshot supplies the device-owned dummy resource.
        bool      SetTexture(const String& path, GPUTextureView* value);
        bool      SetBuffer(const String& path, GPUBuffer* value);
        bool      SetSampler(const String& path, GPUSampler* value);
        // NamePath typed convenience writes use fixed host sizes; no Shader scalar type or field-size validation is performed.
        bool      SetInt(const String& path, int32 value);
        bool      SetUInt(const String& path, uint32 value);
        bool      SetFloat(const String& path, float value);
        bool      SetFloat2(const String& path, Float2& value);
        bool      SetFloat3(const String& path, Float3& value);
        bool      SetFloat4(const String& path, Float4& value);
        bool      SetBool(const String& path, bool value);
        // Raw field writes require a valid UniformValue address and remain inside its Uniform Block; the caller owns ABI correctness.
        bool      SetUniform(const String& path, const void* data, uint32 size);
        // Full-block writes require an exact reflected ByteSize and leave the previous bytes unchanged on failure.
        bool      SetConstantBuffer(const String& path, const void* data, uint32 size);
        // Binds an already uploaded external constant buffer; the caller owns its update and lifetime contract.
        bool      SetConstantBuffer(const String& path, GPUConstantBuffer* buffer);
        // Completes empty resource slots with device-owned dummies before freezing resources and uniform data.
        // The snapshot retains the Program pointer identity and any external CB identity.
        bool BuildSnapshot(ShaderBindingSnapshot& snapshot) const;

        SLC2GPUShaderProgram* GetProgram() const;

    private:
        friend class ShaderVar;

        struct ShaderUniformBlockState
        {
            ShaderUniformBlockSource Source         = ShaderUniformBlockSource::InternalData;
            List<byte>               Data;
            GPUConstantBuffer*       ExternalBuffer = nullptr;
        };

        bool SetTexture(const ShaderVarLocation& location, GPUTextureView* value);
        bool SetBuffer(const ShaderVarLocation& location, GPUBuffer* value);
        bool SetSampler(const ShaderVarLocation& location, GPUSampler* value);
        bool SetUniform(const ShaderVarLocation& location, const void* data, uint32 size);
        ShaderBindingResource* GetResourceSlot(const ShaderVarLocation& location,
                                               ShaderProgramBindingKind expectedKind) const;
        void ClearResources();

        SLC2GPUShaderProgram*         m_Program = nullptr;
        List<ShaderBindingResource*>  m_Resources;
        List<ShaderUniformBlockState> m_UniformBlocks;
    };
} // namespace SE
