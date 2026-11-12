#include "GPUShaderProgram.h"

namespace SE
{
    bool GPUShaderProgram::Initialize(const ShaderProgramRecord* program,
                                       const ShaderTargetRecord*  target,
                                       const ShaderVariantRecord* variant)
    {
        m_Program = program;
        m_Target  = target;
        m_Variant = variant;
        if (variant == nullptr)
        {
            LOG_ERROR("Shader", "Shader program requires a selected variant.");
            return false;
        }
        return m_Reflection.Initialize(variant->Layout);
    }

    const String& GPUShaderProgram::GetProgramId() const
    {
        return m_Program != nullptr ? m_Program->ProgramId : String::Empty;
    }

    const ShaderVariantRecord* GPUShaderProgram::GetVariant() const { return m_Variant; }

    const ShaderProgramReflection& GPUShaderProgram::GetReflection() const { return m_Reflection; }
} // namespace SE
