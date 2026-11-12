#pragma once

#include "Runtime/API.h"
#include <Runtime/ShaderCompilation/Slang/Artifact/ShaderArtifact.h>
#include "Reflection/ShaderProgramReflection.h"

namespace SE
{
    // 已选中 Program/Target/Variant 后生成的不可变 ShaderProgram 缓存对象。
    // 平台无关层只保存 ShaderArtifact 指针和反射；具体 GPU 后端在派生类中持有 shader module / pipeline layout 等对象。
    class SE_API_RUNTIME GPUShaderProgram
    {
    public:
        virtual ~GPUShaderProgram() {}

        virtual bool Initialize(const ShaderProgramRecord* program, const ShaderTargetRecord* target, const ShaderVariantRecord* variant);

        const String&                  GetProgramId() const;
        const ShaderVariantRecord*       GetVariant() const;
        const ShaderProgramReflection& GetReflection() const;

    protected:
        const ShaderProgramRecord* m_Program = nullptr;
        const ShaderTargetRecord*  m_Target  = nullptr;
        const ShaderVariantRecord* m_Variant = nullptr;
        ShaderProgramReflection  m_Reflection;
    };
}
