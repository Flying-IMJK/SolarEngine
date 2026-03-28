
#include "GPUPipelineStateVulkan.h"
#include "GPUShaderProgramVulkan.h"

#include "Core/Types/Strings/StringView.h"
#include "Core/Profiler/Profiler.h"

namespace SE
{
	static VkStencilOp ToVulkanStencilOp(const StencilOperation value)
	{
		switch (value)
		{
		case StencilOperation::Keep:
			return VK_STENCIL_OP_KEEP;
		case StencilOperation::Zero:
			return VK_STENCIL_OP_ZERO;
		case StencilOperation::Replace:
			return VK_STENCIL_OP_REPLACE;
		case StencilOperation::IncrementSaturated:
			return VK_STENCIL_OP_INCREMENT_AND_CLAMP;
		case StencilOperation::DecrementSaturated:
			return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
		case StencilOperation::Invert:
			return VK_STENCIL_OP_INVERT;
		case StencilOperation::Increment:
			return VK_STENCIL_OP_INCREMENT_AND_WRAP;
		case StencilOperation::Decrement:
			return VK_STENCIL_OP_DECREMENT_AND_WRAP;
		default:
			return VK_STENCIL_OP_KEEP;
		}
	}

	ComputePipelineStateVulkan::ComputePipelineStateVulkan(GPUDeviceVulkan* device, VkPipeline pipeline, PipelineLayoutVulkan* layout)
		: _device(device)
		, m_Handle(pipeline)
		, m_Layout(layout)
	{
	}

	ComputePipelineStateVulkan::~ComputePipelineStateVulkan()
	{
		dsWriteContainer.Release();
		if (currentTypedDescriptorPoolSet)
		{
			currentTypedDescriptorPoolSet->GetOwner()->refs--;
			currentTypedDescriptorPoolSet = nullptr;
		}
		descriptorSetsLayout = nullptr;
		descriptorSetHandles.Resize(0);
		DynamicOffsets.Resize(0);
		_device->deferredDeletionQueue.EnqueueResource(DeferredDeletionQueueVulkan::Type::Pipeline, m_Handle);
		m_Layout = nullptr;
	}


	GPUPipelineStateVulkan::GPUPipelineStateVulkan(GPUDeviceVulkan* device)  : GPUResourceVulkan<GPUPipelineState>(device, StringView::Empty)
		, m_Pipelines(16)
		, m_Layout(nullptr)
	{

	}
	PipelineLayoutVulkan* GPUPipelineStateVulkan::GetLayout()
	{
		if (m_Layout)
			return m_Layout;

		DescriptorSetLayoutInfoVulkan descriptorSetLayoutInfo;
#define INIT_SHADER_STAGE(set, bit) \
	if (descriptorInfoPerStage[DescriptorSet::set]) \
		descriptorSetLayoutInfo.AddBindingsForStage(bit, DescriptorSet::set, descriptorInfoPerStage[DescriptorSet::set])

		INIT_SHADER_STAGE(Vertex, VK_SHADER_STAGE_VERTEX_BIT);
		INIT_SHADER_STAGE(Hull, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT);
		INIT_SHADER_STAGE(Domain, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT);
		INIT_SHADER_STAGE(Geometry, VK_SHADER_STAGE_GEOMETRY_BIT);
		INIT_SHADER_STAGE(Pixel, VK_SHADER_STAGE_FRAGMENT_BIT);
#undef INIT_SHADER_STAGE

		m_Layout = m_Device->GetOrCreateLayout(descriptorSetLayoutInfo);
		ENGINE_ASSERT(m_Layout);
		descriptorSetsLayout = &m_Layout->descriptorSetLayout;
		descriptorSetHandles.AddZeroed(descriptorSetsLayout->handles.Count());

		return m_Layout;
	}


	bool GPUPipelineStateVulkan::IsValid() const
	{
		return m_MemoryUsage != 0;
	}

	void GPUPipelineStateVulkan::OnReleaseGPU()
	{
		dsWriteContainer.Release();
		if (currentTypedDescriptorPoolSet)
		{
			currentTypedDescriptorPoolSet->GetOwner()->refs--;
			currentTypedDescriptorPoolSet = nullptr;
		}
		descriptorSetsLayout = nullptr;
		descriptorSetHandles.Resize(0);
		dynamicOffsets.Resize(0);
		for (auto i = m_Pipelines.begin(); i.IsNotEnd(); ++i)
		{
			m_Device->deferredDeletionQueue.EnqueueResource(DeferredDeletionQueueVulkan::Type::Pipeline, i->Value);
		}
		m_Layout = nullptr;
		m_Pipelines.Clear();
	}

	bool GPUPipelineStateVulkan::Init(const GPUPipelineState::Description& desc)
	{
		ENGINE_ASSERT(!IsValid());

		// Create description
		VulkanTool::ZeroStruct(m_Desc, VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO);

		// Vertex Input
		m_Desc.pVertexInputState = (VkPipelineVertexInputStateCreateInfo*)desc.VS->GetInputLayout();

		// Stages
		usedStagesMask = 0;
		hasDescriptorsPerStageMask = 0;
		Platform::MemoryClear(shaderBindingsPerStage, sizeof(shaderBindingsPerStage));
		Platform::MemoryClear(descriptorInfoPerStage, sizeof(descriptorInfoPerStage));

#define INIT_SHADER_STAGE(type, set, bit) 																		\
		if(desc.type) 																							\
		{ 																										\
			int32 stageIndex = (int32)DescriptorSet::set; 														\
			usedStagesMask |= (1 << stageIndex); 																\
			auto bindings = &desc.type->GetBindings(); 															\
			if (bindings->usedCBsMask + bindings->usedSRsMask + bindings->usedUAsMask) 							\
				hasDescriptorsPerStageMask |= (1 << stageIndex); 												\
			shaderBindingsPerStage[stageIndex] = bindings; 														\
			descriptorInfoPerStage[stageIndex] = &((GPUShaderProgram##type##Vulkan*)desc.type)->descriptorInfo; \
			auto& stage = m_ShaderStages[m_Desc.stageCount++]; 													\
			VulkanTool::ZeroStruct(stage, VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO); 				\
			stage.stage = bit; 																					\
			stage.module = (VkShaderModule)desc.type->GetBufferHandle(); 										\
			stage.pName = desc.type->m_NativeName.Get(); 														\
		}

		INIT_SHADER_STAGE(VS, Vertex, VK_SHADER_STAGE_VERTEX_BIT);
		INIT_SHADER_STAGE(HS, Hull, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT);
    	INIT_SHADER_STAGE(DS, Domain, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT);
		INIT_SHADER_STAGE(GS, Geometry, VK_SHADER_STAGE_GEOMETRY_BIT);
		INIT_SHADER_STAGE(PS, Pixel, VK_SHADER_STAGE_FRAGMENT_BIT);
#undef INIT_SHADER_STAGE
		m_Desc.pStages = m_ShaderStages;

		// Input Assembly
		VulkanTool::ZeroStruct(m_DescInputAssembly, VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO);;
		switch (desc.PrimitiveTopology)
		{
		case PrimitiveTopologyType::Point:
			m_DescInputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
			break;
		case PrimitiveTopologyType::Line:
			m_DescInputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
			break;
		case PrimitiveTopologyType::Triangle:
			m_DescInputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			break;
		}

		if (desc.HS)
		{
			m_DescInputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
		}

		m_Desc.pInputAssemblyState = &m_DescInputAssembly;
		
		// Tessellation
		if (desc.HS)
		{
			VulkanTool::ZeroStruct(m_DescTessellation, VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO);
			m_DescTessellation.patchControlPoints = desc.HS->GetControlPointsCount();
			m_Desc.pTessellationState = &m_DescTessellation;
		}

		// Viewport
		VulkanTool::ZeroStruct(m_DescViewport, VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO);
		m_DescViewport.viewportCount = 1;
		m_DescViewport.scissorCount = 1;
		m_Desc.pViewportState = &m_DescViewport;

		// Dynamic
		VulkanTool::ZeroStruct(m_DescDynamic, VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO);
		m_DescDynamic.pDynamicStates = m_DynamicStates;
		m_DynamicStates[m_DescDynamic.dynamicStateCount++] = VK_DYNAMIC_STATE_VIEWPORT;
		m_DynamicStates[m_DescDynamic.dynamicStateCount++] = VK_DYNAMIC_STATE_SCISSOR;
		m_DynamicStates[m_DescDynamic.dynamicStateCount++] = VK_DYNAMIC_STATE_STENCIL_REFERENCE;
		static_assert(ARRAY_SIZE(m_DynamicStates) <= 3, "Invalid dynamic states array.");
		m_Desc.pDynamicState = &m_DescDynamic;

		// Multisample
		VulkanTool::ZeroStruct(m_DescMultisample, VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO);
		m_DescMultisample.minSampleShading = 0.0f;
		m_DescMultisample.alphaToCoverageEnable = desc.BlendMode.AlphaToCoverageEnable;
		m_DescMultisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		m_Desc.pMultisampleState = &m_DescMultisample;

		// Depth Stencil
		VulkanTool::ZeroStruct(m_DescDepthStencil, VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO);
		m_DescDepthStencil.depthTestEnable = desc.DepthEnable;
		m_DescDepthStencil.depthWriteEnable = desc.DepthWriteEnable;
		m_DescDepthStencil.depthCompareOp = VulkanTool::ToVulkanCompareOp(desc.DepthFunc);
		m_DescDepthStencil.stencilTestEnable = desc.StencilEnable;
		m_DescDepthStencil.front.compareMask = desc.StencilReadMask;
		m_DescDepthStencil.front.writeMask = desc.StencilWriteMask;
		m_DescDepthStencil.front.compareOp = VulkanTool::ToVulkanCompareOp(desc.StencilFunc);
		m_DescDepthStencil.front.failOp = ToVulkanStencilOp(desc.StencilFailOp);
		m_DescDepthStencil.front.depthFailOp = ToVulkanStencilOp(desc.StencilDepthFailOp);
		m_DescDepthStencil.front.passOp = ToVulkanStencilOp(desc.StencilPassOp);
		m_DescDepthStencil.front = m_DescDepthStencil.back;
		m_Desc.pDepthStencilState = &m_DescDepthStencil;
		depthReadEnable = desc.DepthEnable && desc.DepthFunc != ComparisonFunc::Always;
		depthWriteEnable = m_DescDepthStencil.depthWriteEnable;
		stencilReadEnable = desc.StencilEnable && desc.StencilReadMask != 0 && desc.StencilFunc != ComparisonFunc::Always;
		stencilWriteEnable = desc.StencilEnable && desc.StencilWriteMask != 0;

		// Rasterization
		VulkanTool::ZeroStruct(m_DescRasterization, VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO);
		m_DescRasterization.polygonMode = desc.Wireframe ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;
		switch (desc.CullMode)
		{
		case CullMode::Normal:
			m_DescRasterization.cullMode = VK_CULL_MODE_BACK_BIT;
			break;
		case CullMode::Inverted:
			m_DescRasterization.cullMode = VK_CULL_MODE_FRONT_BIT;
			break;
		case CullMode::TwoSided:
			m_DescRasterization.cullMode = VK_CULL_MODE_NONE;
			break;
		}
		m_DescRasterization.frontFace = VK_FRONT_FACE_CLOCKWISE;
		m_DescRasterization.depthClampEnable = !desc.DepthClipEnable && m_Device->GetGPULimits().HasDepthClip;
		m_DescRasterization.lineWidth = 1.0f;
		m_Desc.pRasterizationState = &m_DescRasterization;

		// Color Blend State
		blendEnable = desc.BlendMode.BlendEnable;
		VulkanTool::ZeroStruct(m_DescColorBlend, VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO);
		{
			auto& blend = m_DescColorBlendAttachments[0];
			blend.blendEnable = desc.BlendMode.BlendEnable;
			blend.srcColorBlendFactor = VulkanTool::ToVulkanBlendFactor(desc.BlendMode.SrcBlend);
			blend.dstColorBlendFactor = VulkanTool::ToVulkanBlendFactor(desc.BlendMode.DestBlend);
			blend.colorBlendOp = VulkanTool::ToVulkanBlendOp(desc.BlendMode.BlendOp);
			blend.srcAlphaBlendFactor = VulkanTool::ToVulkanBlendFactor(desc.BlendMode.SrcBlendAlpha);
			blend.dstAlphaBlendFactor = VulkanTool::ToVulkanBlendFactor(desc.BlendMode.DestBlendAlpha);
			blend.alphaBlendOp = VulkanTool::ToVulkanBlendOp(desc.BlendMode.BlendOpAlpha);
			blend.colorWriteMask = (VkColorComponentFlags)desc.BlendMode.RenderTargetWriteMask;
		}
		for (int32 i = 1; i < GPU_MAX_RT_BINDED; i++)
		{
			m_DescColorBlendAttachments[i] = m_DescColorBlendAttachments[i - 1];
		}
		m_DescColorBlend.pAttachments = m_DescColorBlendAttachments;
		m_DescColorBlend.blendConstants[0] = 0.0f;
		m_DescColorBlend.blendConstants[1] = 0.0f;
		m_DescColorBlend.blendConstants[2] = 0.0f;
		m_DescColorBlend.blendConstants[3] = 0.0f;
		m_Desc.pColorBlendState = &m_DescColorBlend;

		ENGINE_ASSERT(dsWriteContainer.descriptorWrites.IsEmpty());
		for (int32 stage = 0; stage < DescriptorSet::GraphicsStagesCount; stage++)
		{
			const auto descriptor = descriptorInfoPerStage[stage];
			if (descriptor == nullptr || descriptor->descriptorTypesCount == 0)
				continue;

			// TODO: merge into a single allocation for a whole PSO
			dsWriteContainer.descriptorWrites.AddZeroed(descriptor->descriptorTypesCount);
			dsWriteContainer.descriptorImageInfo.AddZeroed(descriptor->imageInfosCount);
			dsWriteContainer.descriptorBufferInfo.AddZeroed(descriptor->bufferInfosCount);
			dsWriteContainer.descriptorTexelBufferView.AddZeroed(descriptor->texelBufferViewsCount);

			ENGINE_ASSERT(descriptor->descriptorTypesCount < 255);
			dsWriteContainer.bindingToDynamicOffset.AddDefault(descriptor->descriptorTypesCount);
			dsWriteContainer.bindingToDynamicOffset.SetAll(255);
		}

		VkWriteDescriptorSet* currentDescriptorWrite = dsWriteContainer.descriptorWrites.Get();
		VkDescriptorImageInfo* currentImageInfo = dsWriteContainer.descriptorImageInfo.Get();
		VkDescriptorBufferInfo* currentBufferInfo = dsWriteContainer.descriptorBufferInfo.Get();
		VkBufferView* currentTexelBufferView = dsWriteContainer.descriptorTexelBufferView.Get();
		byte* currentBindingToDynamicOffsetMap = dsWriteContainer.bindingToDynamicOffset.Get();
		uint32 dynamicOffsetsStart[DescriptorSet::GraphicsStagesCount];
		uint32 dynamicOffsetsCount = 0;
		for (int32 stage = 0; stage < DescriptorSet::GraphicsStagesCount; stage++)
		{
			dynamicOffsetsStart[stage] = dynamicOffsetsCount;

			const auto descriptor = descriptorInfoPerStage[stage];
			if (descriptor == nullptr || descriptor->descriptorTypesCount == 0)
				continue;

			const uint32 numDynamicOffsets = dsWriter[stage].SetupDescriptorWrites(*descriptor, currentDescriptorWrite,
				currentImageInfo, currentBufferInfo, currentTexelBufferView, currentBindingToDynamicOffsetMap);
			dynamicOffsetsCount += numDynamicOffsets;

			currentDescriptorWrite += descriptor->descriptorTypesCount;
			currentImageInfo += descriptor->imageInfosCount;
			currentBufferInfo += descriptor->bufferInfosCount;
			currentTexelBufferView += descriptor->texelBufferViewsCount;
			currentBindingToDynamicOffsetMap += descriptor->descriptorTypesCount;
		}

		dynamicOffsets.AddZeroed(dynamicOffsetsCount);
		for (int32 stage = 0; stage < DescriptorSet::GraphicsStagesCount; stage++)
		{
			dsWriter[stage].dynamicOffsets = dynamicOffsetsStart[stage] + dynamicOffsets.Get();
		}

		// Set non-zero memory usage
		m_MemoryUsage = sizeof(VkGraphicsPipelineCreateInfo);

		return GPUPipelineState::Init(desc);
	}

	VkPipeline GPUPipelineStateVulkan::GetState(RenderPassVulkan* renderPass)
	{
		ENGINE_ASSERT(renderPass);

		// Try reuse cached version
		VkPipeline pipeline = VK_NULL_HANDLE;
		if (m_Pipelines.TryGet(renderPass, pipeline))
		{
//#if BUILD_DEBUG
			// Verify
			RenderPassVulkan* refKey = nullptr;
			m_Pipelines.KeyOf(pipeline, &refKey);
			ENGINE_ASSERT(refKey == renderPass);
//#endif
			return pipeline;
		}

		PROFILE_CPU_NAMED("Create Pipeline");

		// Update description to match the pipeline
		m_DescColorBlend.attachmentCount = renderPass->layout.RTsCount;
		m_DescMultisample.rasterizationSamples = (VkSampleCountFlagBits)renderPass->layout.MSAA;
		m_Desc.renderPass = renderPass->handle;

		// Check if has missing layout
		if (m_Desc.layout == VK_NULL_HANDLE)
		{
			m_Desc.layout = GetLayout()->handle;
		}

		// Create object
		const VkResult result = vkCreateGraphicsPipelines(m_Device->device, m_Device->pipelineCache, 1, &m_Desc, nullptr, &pipeline);
		LOG_VULKAN_RESULT(result);
		if (result != VK_SUCCESS)
		{
//#if BUILD_DEBUG
			StringView vsName = debugDesc.VS ? debugDesc.VS->GetName() : StringView::Empty;
			StringView psName = debugDesc.PS ? debugDesc.PS->GetName() : StringView::Empty;
			LOG_ERROR("Graphic", "GPUPipelineState vkCreateGraphicsPipelines failed for VS={0}, PS={1}", vsName, psName);
//#endif
			return VK_NULL_HANDLE;
		}

		// Cache it
		m_Pipelines.Add(renderPass, pipeline);

		return pipeline;
	}

}