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
        ~ShaderProgramInstance();

        bool      Initialize(SLC2GPUShaderProgram* program);
        ShaderVar GetRootVar();
        bool      FindVar(const String& path, ShaderVar& result);
        bool      SetTexture(const String& path, GPUTextureView* value);
        bool      SetBuffer(const String& path, GPUBuffer* value);
        bool      SetSampler(const String& path, GPUSampler* value);
        // NamePath typed convenience writes use fixed host sizes; no Shader scalar type or field-size validation is performed.
        bool      SetInt(const String& path, int32 value);
        bool      SetUInt(const String& path, uint32 value);
        bool      SetFloat(const String& path, float value);
        bool      SetBool(const String& path, bool value);
        // Raw field writes require a valid UniformValue address and remain inside its Uniform Block; the caller owns ABI correctness.
        bool      SetUniform(const String& path, const void* data, uint32 size);
        // Full-block writes require an exact reflected ByteSize and leave the previous bytes unchanged on failure.
        bool      SetConstantBuffer(const String& path, const void* data, uint32 size);
        // Binds an already uploaded external constant buffer; the caller owns its update and lifetime contract.
        bool      SetConstantBuffer(const String& path, GPUConstantBuffer* buffer);
        // Freezes resources, internal bytes, or an external CB identity; the snapshot retains the Program pointer identity.
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

		// 路径解析只依赖不可变 reflection；资源和 ordinary data 都落到 Instance 的 dense state。
        bool Resolve(const String& path, ShaderVarLocation& location) const;
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
