#include "ShaderVar.h"

#include "../ShaderProgramInstance.h"
#include "ShaderNameResolver.h"

namespace SE
{
    bool ShaderVar::FindMember(const String& name, ShaderVar& result) const
    {
        if (!IsValid())
        {
            LOG_ERROR("Graphics", "Shader variable is not valid.");
            return false;
        }
        ShaderVarLocation location;
        String msg;
        if (!ShaderNameResolver::ResolveMember(*_reflection, _location, name, location, msg))
        {
            return false;
        }
        result = ShaderVar(m_Instance, _reflection, location);
        return true;
    }

    bool ShaderVar::GetElement(const uint32 index, ShaderVar& result) const
    {
        if (!IsValid())
        {
            LOG_ERROR("Graphics", "Shader variable is not valid.");
            return false;
        }
        ShaderVarLocation location;
        String msg;
        if (!ShaderNameResolver::ResolveIndex(*_reflection, _location, index, location, msg))
        {
            return false;
        }
        result = ShaderVar(m_Instance, _reflection, location);
        return true;
    }

    ShaderVar ShaderVar::operator[](const uint32 index) const
    {
        ShaderVar result;
        GetElement(index, result);
        return result;
    }

    bool ShaderVar::SetTexture(GPUTextureView* value) const
    {
        if (!IsValid())
        {
            LOG_ERROR("Graphics", "Shader variable is not valid.");
            return false;
        }
        return m_Instance->SetTexture(_location, value);
    }

    bool ShaderVar::SetBuffer(GPUBuffer* value) const
    {
        if (!IsValid())
        {
            LOG_ERROR("Graphics", "Shader variable is not valid.");
            return false;
        }
        return m_Instance->SetBuffer(_location, value);
    }

    bool ShaderVar::SetSampler(GPUSampler* value) const
    {
        if (!IsValid())
        {
            LOG_ERROR("Graphics", "Shader variable is not valid.");
            return false;
        }
        return m_Instance->SetSampler(_location, value);
    }

    bool ShaderVar::SetUniform(const void* data, const uint32 size) const
    {
        if (!IsValid())
        {
            LOG_ERROR("Graphics", "Shader variable is not valid.");
            return false;
        }
        return m_Instance->SetUniform(_location, data, size);
    }
} // namespace SE
