#include "ShaderVar.h"

#include "../ShaderParameterBlock.h"
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
		if (!ShaderNameResolver::ResolveMember(*_reflection, _location, name, location))
		{
			return false;
		}
		ShaderParameterBlock* block = m_ParameterBlock;
		if (location.BlockId != _location.BlockId)
		{
			// 访问参数块成员时，反射位置可能进入 sub-block，需要切换到对应的可变状态对象。
			block = m_ParameterBlock->FindSubBlock(location.BlockId);
			if (block == nullptr)
			{
				LOG_ERROR("Graphics", "Shader parameter block state was not created for the reflected sub-block.");
				return false;
			}
		}
		result = ShaderVar(block, _reflection, location);
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
		if (!ShaderNameResolver::ResolveIndex(*_reflection, _location, index, location))
		{
			return false;
		}
		result = ShaderVar(m_ParameterBlock, _reflection, location);
		return true;
	}

	ShaderVar ShaderVar::operator[](const uint32 index) const
	{
		ShaderVar result;
		String error;
		GetElement(index, result);
		return result;
	}

	bool ShaderVar::SetTexture(void* value) const
	{
		if (!IsValid())
		{
			LOG_ERROR("Graphics", "Shader variable is not valid.");
			return false;
		}
		return m_ParameterBlock->SetResource(_location, ShaderRuntimeResourceType::Texture, value);
	}

	bool ShaderVar::SetBuffer(void* value) const
	{
		if (!IsValid())
		{
			LOG_ERROR("Graphics", "Shader variable is not valid.");
			return false;
		}
		return m_ParameterBlock->SetResource(_location, ShaderRuntimeResourceType::Buffer, value);
	}

	bool ShaderVar::SetSampler(void* value) const
	{
		if (!IsValid())
		{
			LOG_ERROR("Graphics", "Shader variable is not valid.");
			return false;
		}
		return m_ParameterBlock->SetResource(_location, ShaderRuntimeResourceType::Sampler, value);
	}

	bool ShaderVar::SetUniform(const void* data, const uint32 size) const
	{
		if (!IsValid())
		{
			LOG_ERROR("Graphics", "Shader variable is not valid.");
			return false;
		}
		return m_ParameterBlock->SetUniform(_location, data, size);
	}
}
