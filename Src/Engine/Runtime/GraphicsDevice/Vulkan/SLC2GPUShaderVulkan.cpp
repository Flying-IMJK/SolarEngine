#include "SLC2GPUShaderVulkan.h"

#include "SLC2ShaderProgramVulkan.h"

namespace SE
{
	SLC2GPUShaderVulkan::SLC2GPUShaderVulkan(GPUDeviceVulkan* device, const StringView& name)
		: GPUResourceVulkan<SLC2GPUShader>(device, name)
	{
	}

	SLC2ShaderProgram* SLC2GPUShaderVulkan::CreateSLC2ShaderProgram(const SLC2ProgramRecord* program, const SLC2TargetRecord* target, const SLC2VariantRecord* variant)
	{
		SLC2ShaderProgramVulkan* shaderProgram = New<SLC2ShaderProgramVulkan>(m_Device);
		if (!shaderProgram->Initialize(program, target, variant))
		{
			Delete(shaderProgram);
			return nullptr;
		}
		return shaderProgram;
	}
}
