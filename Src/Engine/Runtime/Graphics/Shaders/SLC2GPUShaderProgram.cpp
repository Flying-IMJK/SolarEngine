#include "SLC2GPUShaderProgram.h"

namespace SE
{
    bool SLC2GPUShaderProgram::Initialize(const SLC2ProgramRecord* program,
                                       const SLC2TargetRecord*  target,
                                       const SLC2VariantRecord* variant)
    {
        _program = program;
        _target  = target;
        _variant = variant;
        if (variant == nullptr)
        {
            LOG_ERROR("Shader", "Shader program requires a selected variant.");
            return false;
        }
        return _reflection.Initialize(variant->Layout);
    }

    const SLC2VariantRecord* SLC2GPUShaderProgram::GetVariant() const { return _variant; }

    const ShaderProgramReflection& SLC2GPUShaderProgram::GetReflection() const { return _reflection; }
} // namespace SE