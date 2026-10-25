#include "SLC2GPUShaderProgram.h"

namespace SE
{
    bool SLC2GPUShaderProgram::Initialize(const SLC2ProgramRecord* program,
                                       const SLC2TargetRecord*  target,
                                       const SLC2VariantRecord* variant)
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

    const String& SLC2GPUShaderProgram::GetProgramId() const
    {
        return m_Program != nullptr ? m_Program->ProgramId : String::Empty;
    }

    const SLC2VariantRecord* SLC2GPUShaderProgram::GetVariant() const { return m_Variant; }

    const ShaderProgramReflection& SLC2GPUShaderProgram::GetReflection() const { return m_Reflection; }
} // namespace SE
