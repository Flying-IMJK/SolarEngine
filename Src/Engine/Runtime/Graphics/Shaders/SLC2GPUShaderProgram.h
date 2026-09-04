#pragma once

#include "Runtime/API.h"
#include <Runtime/ShaderCompilation/Slang/SLC2/SLC2Artifact.h>
#include "Reflection/ShaderProgramReflection.h"

namespace SE
{
    // 已选中 Program/Target/Variant 后生成的不可变 ShaderProgram 缓存对象。
    // 平台无关层只保存 SLC2 指针和反射；具体 GPU 后端在派生类中持有 shader module / pipeline layout 等对象。
    class SE_API_RUNTIME SLC2GPUShaderProgram
    {
    public:
        virtual ~SLC2GPUShaderProgram() {}

        virtual bool Initialize(const SLC2ProgramRecord* program, const SLC2TargetRecord* target, const SLC2VariantRecord* variant);

        const SLC2VariantRecord*       GetVariant() const;
        const ShaderProgramReflection& GetReflection() const;

    protected:
        const SLC2ProgramRecord* _program = nullptr;
        const SLC2TargetRecord*  _target  = nullptr;
        const SLC2VariantRecord* _variant = nullptr;
        ShaderProgramReflection  _reflection;
    };
}
