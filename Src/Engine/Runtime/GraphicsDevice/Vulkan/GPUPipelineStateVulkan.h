#pragma once

//#include "Runtime/Graphics/GPUConfig.h"
#include "Runtime/Graphics/Base/GPUPipelineState.h"
#include "GPUDeviceVulkan.h"
#include "VulkanTypes.h"
#include "DescriptorSetVulkan.h"
#include "CmdBufferVulkan.h"

namespace SE
{
	class PipelineLayoutVulkan;

	/// <summary>
	/// Vulkan Compute管线状态对象
	/// </summary>
	class ComputePipelineStateVulkan
	{
	private:
		GPUDeviceVulkan* _device;
		VkPipeline m_Handle;
		PipelineLayoutVulkan* m_Layout;

	public:
		ComputePipelineStateVulkan(GPUDeviceVulkan* device, VkPipeline pipeline, PipelineLayoutVulkan* layout);
		~ComputePipelineStateVulkan();

	public:
		/// <summary>
		/// The cached shader descriptor infos for compute shader.
		/// </summary>
		const SpirvShaderDescriptorInfo* descriptorInfo;

		DescriptorSetWriteContainerVulkan dsWriteContainer;
		DescriptorSetWriterVulkan dsWriter;

		const DescriptorSetLayoutVulkan* descriptorSetsLayout = nullptr;
		TypedDescriptorPoolSetVulkan* currentTypedDescriptorPoolSet = nullptr;
		List<VkDescriptorSet> descriptorSetHandles;

		inline bool AcquirePoolSet(CmdBufferVulkan* cmdBuffer)
		{
			// Pipeline state has no current descriptor pools set or set owner is not current - acquire a new pool set
			DescriptorPoolSetContainerVulkan* cmdBufferPoolSet = cmdBuffer->GetDescriptorPoolSet();
			if (currentTypedDescriptorPoolSet == nullptr || currentTypedDescriptorPoolSet->GetOwner() != cmdBufferPoolSet)
			{
				if (currentTypedDescriptorPoolSet)
					currentTypedDescriptorPoolSet->GetOwner()->refs--;
				currentTypedDescriptorPoolSet = cmdBufferPoolSet->AcquireTypedPoolSet(*descriptorSetsLayout);
				currentTypedDescriptorPoolSet->GetOwner()->refs++;
				return true;
			}
			return false;
		}

		inline bool AllocateDescriptorSets()
		{
			return currentTypedDescriptorPoolSet->AllocateDescriptorSets(*descriptorSetsLayout, descriptorSetHandles.Get());
		}

		List<uint32> DynamicOffsets;

	public:
		void Bind(CmdBufferVulkan* cmdBuffer)
		{
			vkCmdBindDescriptorSets(
				cmdBuffer->GetHandle(),
				VK_PIPELINE_BIND_POINT_COMPUTE,
				GetLayout()->handle,
				0,
				descriptorSetHandles.Count(),
				descriptorSetHandles.Get(),
				DynamicOffsets.Count(),
				DynamicOffsets.Get());
		}

	public:
		VkPipeline GetHandle() const
		{
			return m_Handle;
		}

		PipelineLayoutVulkan* GetLayout() const
		{
			return m_Layout;
		}
	};

	/// <summary>
	/// Vulkan Graphics管线状态对象
	/// </summary>
	class GPUPipelineStateVulkan : public GPUResourceVulkan<GPUPipelineState>
	{
	private:
		Dictionary<RenderPassVulkan*, VkPipeline> m_Pipelines;
		VkGraphicsPipelineCreateInfo m_Desc;
		VkPipelineShaderStageCreateInfo m_ShaderStages[(int)ShaderStage::Max];
		VkPipelineInputAssemblyStateCreateInfo m_DescInputAssembly;
		VkPipelineTessellationStateCreateInfo m_DescTessellation;
		VkPipelineViewportStateCreateInfo m_DescViewport;
		VkPipelineDynamicStateCreateInfo m_DescDynamic;
		VkDynamicState m_DynamicStates[3];
		VkPipelineMultisampleStateCreateInfo m_DescMultisample;
		VkPipelineDepthStencilStateCreateInfo m_DescDepthStencil;
		VkPipelineRasterizationStateCreateInfo m_DescRasterization;
		VkPipelineColorBlendStateCreateInfo m_DescColorBlend;
		VkPipelineColorBlendAttachmentState m_DescColorBlendAttachments[GPU_MAX_RT_BINDED];
		PipelineLayoutVulkan* m_Layout;

	public:
		/// <summary>
		/// Initializes a new instance of the <see cref="GPUPipelineStateVulkan"/> class.
		/// </summary>
		/// <param name="device">The graphics device.</param>
		GPUPipelineStateVulkan(GPUDeviceVulkan* device);

	public:
		/// <summary>
		/// The bitmask of stages that exist in this pipeline.
		/// </summary>
		uint32 usedStagesMask;

		uint32 blendEnable : 1;
		uint32 depthReadEnable : 1;
		uint32 depthWriteEnable : 1;
		uint32 stencilReadEnable : 1;
		uint32 stencilWriteEnable : 1;

		/// <summary>
		/// The bitmask of stages that have descriptors.
		/// </summary>
		uint32 hasDescriptorsPerStageMask;

		/// <summary>
		/// The cached shader bindings per stage.
		/// </summary>
		const ShaderBindings* shaderBindingsPerStage[DescriptorSet::GraphicsStagesCount];

		/// <summary>
		/// The cached shader descriptor infos per stage.
		/// </summary>
		const SpirvShaderDescriptorInfo* descriptorInfoPerStage[DescriptorSet::GraphicsStagesCount];

		const VkPipelineVertexInputStateCreateInfo* GetVertexInputState() const
		{
			return m_Desc.pVertexInputState;
		}

		DescriptorSetWriteContainerVulkan dsWriteContainer;
		DescriptorSetWriterVulkan dsWriter[DescriptorSet::GraphicsStagesCount];

		const DescriptorSetLayoutVulkan* descriptorSetsLayout = nullptr;
		TypedDescriptorPoolSetVulkan* currentTypedDescriptorPoolSet = nullptr;
		List<VkDescriptorSet> descriptorSetHandles;

		List<uint32> dynamicOffsets;

	public:
		inline bool AcquirePoolSet(CmdBufferVulkan* cmdBuffer)
		{
			// Lazy init
			if (!descriptorSetsLayout)
			{
				GetLayout();
			}

			// Pipeline state has no current descriptor pools set or set owner is not current - acquire a new pool set
			DescriptorPoolSetContainerVulkan* cmdBufferPoolSet = cmdBuffer->GetDescriptorPoolSet();
			if (currentTypedDescriptorPoolSet == nullptr || currentTypedDescriptorPoolSet->GetOwner() != cmdBufferPoolSet)
			{
				if (currentTypedDescriptorPoolSet)
				{
					currentTypedDescriptorPoolSet->GetOwner()->refs--;
				}
				currentTypedDescriptorPoolSet = cmdBufferPoolSet->AcquireTypedPoolSet(*descriptorSetsLayout);
				currentTypedDescriptorPoolSet->GetOwner()->refs++;
				return true;
			}
			return false;
		}

		/// <summary>
		/// Gets the Vulkan pipeline layout for this pipeline state.
		/// </summary>
		/// <returns>The layout.</returns>
		PipelineLayoutVulkan* GetLayout();

		/// <summary>
		/// Gets the Vulkan graphics pipeline object for the given rendering state. Uses depth buffer and render targets formats and multi-sample levels to setup a proper PSO. Uses caching.
		/// </summary>
		/// <param name="renderPass">The render pass.</param>
		/// <returns>Vulkan graphics pipeline object.</returns>
		VkPipeline GetState(RenderPassVulkan* renderPass);

	public:
		// [GPUPipelineState]
		bool IsValid() const final override;
		bool Init(const Description& desc) final override;

	protected:
		// [GPUResourceVulkan]
		void OnReleaseGPU() override;
	};
}
