#include "ShaderProgramInstance.h"

#include "SLC2GPUShader.h"
#include "Reflection/ShaderNameResolver.h"
#include "Reflection/ShaderProgramReflection.h"

namespace SE
{
	bool ShaderProgramInstance::Initialize(SLC2ShaderProgram* program)
	{
		if (program == nullptr)
		{
			LOG_ERROR("Graphics", "Shader program instance requires a selected program.");
			return false;
		}
		const ShaderProgramReflection& reflection = program->GetReflection();
		// Instance 的可变存储按反射出的 root parameter block 建立，Program 本身仍保持不可变。
		if (!m_RootParameters.Initialize(reflection, reflection.GetLayout().RootBlockId))
		{
			return false;
		}
		m_Program = program;
		return true;
	}

	ShaderVar ShaderProgramInstance::GetRootVar()
	{
		if (m_Program == nullptr)
		{
			return ShaderVar();
		}
		ShaderVarLocation location;
		const ShaderReflectionIR& layout = m_Program->GetReflection().GetLayout();
		const ShaderIRParameterBlock* root = m_Program->GetReflection().GetBlock(layout.RootBlockId);
		location.BlockId = root->Id;
		location.TypeId = root->ElementTypeId;
		location.UniformByteOffset = 0;
		location.ResourceRangeIndex = -1;
		location.ResourceIndex = -1;
		return ShaderVar(&m_RootParameters, &m_Program->GetReflection(), location);
	}

	bool ShaderProgramInstance::FindVar(const String& path, ShaderVar& result)
	{
		ShaderVarLocation location;
		if (!Resolve(path, location))
		{
			return false;
		}
		ShaderParameterBlock* block = m_RootParameters.FindBlock(location.BlockId);
		if (block == nullptr)
		{
            LOG_ERROR("Graphics", "Shader parameter block state was not created for the reflected location.");
			return false;
		}
		result = ShaderVar(block, &m_Program->GetReflection(), location);
		return true;
	}

	bool ShaderProgramInstance::SetTexture(const String& path, void* value)
	{
		ShaderVar var;
		return FindVar(path, var) && var.SetTexture(value);
	}

	bool ShaderProgramInstance::SetBuffer(const String& path, void* value)
	{
		ShaderVar var;
		return FindVar(path, var) && var.SetBuffer(value);
	}

	bool ShaderProgramInstance::SetSampler(const String& path, void* value)
	{
		ShaderVar var;
		return FindVar(path, var) && var.SetSampler(value);
	}

	bool ShaderProgramInstance::SetUniform(const String& path, const void* data, const uint32 size)
	{
		ShaderVar var;
		return FindVar(path, var) && var.SetUniform(data, size);
	}

	bool ShaderProgramInstance::ValidateAllBindings(ShaderBindingSnapshot& snapshot) const
	{
		snapshot.Clear();
		// 先校验所有显式资源都已绑定，再把普通 uniform 数据一起提交成平台无关快照。
		if (!m_RootParameters.AppendResources(snapshot))
		{
			return false;
		}
		m_RootParameters.AppendUniformData(snapshot);
		snapshot.Freeze();
		return true;
	}

	SLC2ShaderProgram* ShaderProgramInstance::GetProgram() const
	{
		return m_Program;
	}

	bool ShaderProgramInstance::Resolve(const String& path, ShaderVarLocation& location) const
	{
		if (m_Program == nullptr)
		{
            LOG_ERROR("Graphics", "Shader program instance is not initialized.");
			return false;
		}
		return ShaderNameResolver::Resolve(m_Program->GetReflection(), path, location);
	}
}
