#pragma once

#include "Runtime/Graphics/Shaders/SLC2GPUShader.h"
#include "Runtime/Graphics/Base/GPUPipelineState.h"
#include "DescriptorSetVulkan.h"

namespace SE
{
	class GPUDeviceVulkan;
	class GPUContextVulkan;
	class ShaderBindingSnapshot;
	struct ShaderBindingUniformData;
	struct ShaderBindingResource;
    class SLC2ComputePipelineStateVulkan;
	class SLC2GraphicsPipelineStateVulkan;

	// SLC2 反射 range 到 Vulkan descriptor 写入位置的映射。
	// 同一个结构同时覆盖显式资源和 defaultUniformBuffer 对应的 uniform 数据。
	struct SLC2VulkanDescriptorBinding
	{
		uint32 BlockId = 0;
		uint32 RangeIndex = 0;
		uint32 ResourceIndex = 0;
		bool IsUniformData = false;
		uint32 Set = 0;
		uint32 Binding = 0;
		uint32 ArrayElement = 0;
		VkDescriptorType DescriptorType = VK_DESCRIPTOR_TYPE_MAX_ENUM;
		uint32 DescriptorWriteIndex = 0;
	};

	struct SLC2VulkanStageModule
	{
		ShaderStage Stage = ShaderStage::Max;
		StringAnsi EntryPointAnsi;
		VkShaderModule Module = VK_NULL_HANDLE;
	};

	class SLC2ShaderProgramVulkan final : public SLC2ShaderProgram
	{
	public:
		explicit SLC2ShaderProgramVulkan(GPUDeviceVulkan* device);
		~SLC2ShaderProgramVulkan() override;

		bool Initialize(const SLC2ProgramRecord* program, const SLC2TargetRecord* target, const SLC2VariantRecord* variant) override;

		PipelineLayoutVulkan* GetPipelineLayout() const;
		const DescriptorSetLayoutVulkan* GetDescriptorSetLayout() const;
		const List<SLC2VulkanDescriptorBinding>& GetDescriptorBindings() const;
		VkShaderModule GetShaderModule(ShaderStage stage) const;
		const char* GetEntryPoint(ShaderStage stage) const;
		// 延迟创建 compute pipeline；entry point 名称直接来自 SLC2 stage 记录。
		SLC2ComputePipelineStateVulkan* GetOrCreateComputeState();
		// 延迟创建 graphics pipeline；固定渲染状态由调用侧传入，shader module/descriptor layout 来自当前 SLC2 variant。
		SLC2GraphicsPipelineStateVulkan* GetOrCreateGraphicsState(const GPUPipelineState::Description& desc);
		bool PrepareAndBindComputeDescriptors(GPUContextVulkan* context, const ShaderBindingSnapshot& snapshot);
		bool PrepareAndBindGraphicsDescriptors(GPUContextVulkan* context, const ShaderBindingSnapshot& snapshot, SLC2GraphicsPipelineStateVulkan* state);

	private:
		GPUDeviceVulkan* m_Device = nullptr;
		PipelineLayoutVulkan* m_PipelineLayout = nullptr;
		SLC2ComputePipelineStateVulkan* m_ComputeState = nullptr;
		List<SLC2GraphicsPipelineStateVulkan*> m_GraphicsStates;
		List<SLC2VulkanStageModule> m_StageModules;
		List<SLC2VulkanDescriptorBinding> m_DescriptorBindings;
	};
}
