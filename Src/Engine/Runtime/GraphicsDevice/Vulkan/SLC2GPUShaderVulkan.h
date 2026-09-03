#pragma once

#include "Runtime/Graphics/Shaders/SLC2GPUShader.h"
#include "VulkanNative.h"

namespace SE
{
	// Vulkan 版本的 SLC2GPUShader，只负责把平台无关的 SLC2ShaderProgram 创建为 Vulkan 后端实现。
	class SLC2GPUShaderVulkan final : public GPUResourceVulkan<SLC2GPUShader>
	{
	public:
		SLC2GPUShaderVulkan(GPUDeviceVulkan* device, const StringView& name);

	protected:
		SLC2ShaderProgram* CreateSLC2ShaderProgram(const SLC2ProgramRecord* program, const SLC2TargetRecord* target, const SLC2VariantRecord* variant) override;
	};
}
