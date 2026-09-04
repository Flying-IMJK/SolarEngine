
#include "GPUPipelineStateVulkan.h"
#include "GPUShaderProgramVulkan.h"
#include "GPUContextVulkan.h"
#include "GPUSamplerVulkan.h"
#include "GPUBufferVulkan.h"
#include "GPUShaderVulkan.h"

#include "Runtime/Core/Types/Strings/String.h"
#include "Runtime/Core/Types/Strings/StringView.h"
#include "Runtime/Core/Profiler/Profiler.h"

#include "Runtime/Graphics/Shaders/ShaderBindingSnapshot.h"
#include "Runtime/Core/Types/Hash.h"


namespace SE
{
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
		m_DescDepthStencil.depthCompareOp = VulkanTool::ConvertCompareOp(desc.DepthFunc);
		m_DescDepthStencil.stencilTestEnable = desc.StencilEnable;
		m_DescDepthStencil.front.compareMask = desc.StencilReadMask;
		m_DescDepthStencil.front.writeMask = desc.StencilWriteMask;
		m_DescDepthStencil.front.compareOp = VulkanTool::ConvertCompareOp(desc.StencilFunc);
        m_DescDepthStencil.front.failOp      = VulkanTool::ConvertStencilOp(desc.StencilFailOp);
		m_DescDepthStencil.front.depthFailOp = VulkanTool::ConvertStencilOp(desc.StencilDepthFailOp);
		m_DescDepthStencil.front.passOp = VulkanTool::ConvertStencilOp(desc.StencilPassOp);
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



	SLC2PipelineStateVulkanBase::SLC2PipelineStateVulkanBase(GPUDeviceVulkan* device) :
        GPUResourceVulkan<SLC2GPUPipelineState>(device, StringView::Empty)
    {
    }

    SLC2PipelineStateVulkanBase::~SLC2PipelineStateVulkanBase()
    {
        ReleaseDescriptorState();
        m_Layout = nullptr;
    }

    PipelineLayoutVulkan* SLC2PipelineStateVulkanBase::GetLayout() const { return m_Layout; }

    const DescriptorSetLayoutVulkan* SLC2PipelineStateVulkanBase::GetDescriptorSetLayout() const
    {
        return m_Layout != nullptr ? &m_Layout->descriptorSetLayout : nullptr;
    }

    bool SLC2PipelineStateVulkanBase::PrepareAndBindDescriptors(GPUContextVulkan*                        context,
                                                                const ShaderBindingSnapshot&             snapshot,
                                                                const List<SLC2VulkanDescriptorBinding>& bindings,
                                                                VkPipelineBindPoint                      bindPoint)
    {
        if (!snapshot.IsFrozen())
        {
            LOG_ERROR("Graphic", "Descriptor binding requires a frozen binding snapshot.");
            return false;
        }

        if (m_Layout == nullptr)
        {
            LOG_ERROR("Graphic", "Descriptor binding requires a pipeline layout.");
            return false;
        }

        if (!SetupDescriptorWrites())
        {
            return false;
        }

        const DescriptorSetLayoutVulkan& descriptorSetLayout = m_Layout->descriptorSetLayout;
        if (descriptorSetLayout.handles.IsEmpty())
        {
            if (bindings.HasItems())
            {
                LOG_ERROR("Graphic",
                          "SLC2 Vulkan descriptor binding map is not empty but the layout has no descriptor sets.");
                return false;
            }
            if (snapshot.GetResources().HasItems() || snapshot.GetUniformData().HasItems())
            {
                LOG_ERROR("Graphic",
                          "Descriptor snapshot contains bindings but the Vulkan layout has no descriptor sets.");
                return false;
            }
            return true;
        }

        if (!ValidateSnapshotCompleteness(snapshot, bindings))
        {
            return false;
        }

        const auto                        cmdBuffer        = context->GetCmdBufferManager()->GetCmdBuffer();
        DescriptorPoolSetContainerVulkan* cmdBufferPoolSet = cmdBuffer->GetDescriptorPoolSet();
        if (m_CurrentTypedDescriptorPoolSet == nullptr ||
            m_CurrentTypedDescriptorPoolSet->GetOwner() != cmdBufferPoolSet)
        {
            // descriptor set 从当前命令缓冲的 pool 获取；换 pool 时释放旧 owner 引用。
            if (m_CurrentTypedDescriptorPoolSet != nullptr)
            {
                m_CurrentTypedDescriptorPoolSet->GetOwner()->refs--;
            }
            m_CurrentTypedDescriptorPoolSet = cmdBufferPoolSet->AcquireTypedPoolSet(descriptorSetLayout);
            m_CurrentTypedDescriptorPoolSet->GetOwner()->refs++;
        }

        m_DescriptorSetHandles.Resize(0);
        m_DescriptorSetHandles.AddZeroed(descriptorSetLayout.handles.Count());
        if (!m_CurrentTypedDescriptorPoolSet->AllocateDescriptorSets(descriptorSetLayout, m_DescriptorSetHandles.Get()))
        {
            LOG_ERROR("Graphic", "Failed to allocate SLC2 Vulkan descriptor sets.");
            return false;
        }

        for (int32 writeIndex = 0; writeIndex < m_DescriptorWriteContainer.descriptorWrites.Count(); writeIndex++)
        {
            if (m_DescriptorWriteSetIndices[writeIndex] >= static_cast<uint32>(m_DescriptorSetHandles.Count()))
            {
                LOG_ERROR("Graphic", "SLC2 Vulkan descriptor write references an invalid descriptor set.");
                return false;
            }
            VkWriteDescriptorSet& descriptorWrite = m_DescriptorWriteContainer.descriptorWrites[writeIndex];
            descriptorWrite.dstSet                = m_DescriptorSetHandles[m_DescriptorWriteSetIndices[writeIndex]];
        }

        if (!WriteSnapshot(context, snapshot, bindings))
        {
            return false;
        }

        vkUpdateDescriptorSets(m_Device->device,
                               m_DescriptorWriteContainer.descriptorWrites.Count(),
                               m_DescriptorWriteContainer.descriptorWrites.Get(),
                               0,
                               nullptr);
        vkCmdBindDescriptorSets(cmdBuffer->GetHandle(),
                                bindPoint,
                                m_Layout->handle,
                                0,
                                m_DescriptorSetHandles.Count(),
                                m_DescriptorSetHandles.Get(),
                                m_DynamicOffsets.Count(),
                                m_DynamicOffsets.Get());
        return true;
    }


    bool SLC2PipelineStateVulkanBase::IsValid() const
    {
        return m_MemoryUsage != 0;
    }

	SLC2ComputePipelineStateVulkan::SLC2ComputePipelineStateVulkan(GPUDeviceVulkan*      device,
                                                                   VkPipeline            pipeline,
                                                                   PipelineLayoutVulkan* layout) :
        SLC2PipelineStateVulkanBase(device), m_Pipeline(pipeline)
    {}

    SLC2ComputePipelineStateVulkan::~SLC2ComputePipelineStateVulkan()
    {
        if (m_Pipeline != VK_NULL_HANDLE)
        {
            m_Device->deferredDeletionQueue.EnqueueResource(DeferredDeletionQueueVulkan::Type::Pipeline, m_Pipeline);
            m_Pipeline = VK_NULL_HANDLE;
        }
    }

    bool SLC2ComputePipelineStateVulkan::Init(const Description& desc, const SLC2GPUShaderProgram* program)
    {
        return false;
    }

    VkPipeline SLC2ComputePipelineStateVulkan::GetHandle() const { return m_Pipeline; }

    PipelineLayoutVulkan* SLC2ComputePipelineStateVulkan::GetLayout() const { return SLC2PipelineStateVulkanBase::GetLayout(); }

    const DescriptorSetLayoutVulkan* SLC2ComputePipelineStateVulkan::GetDescriptorSetLayout() const
    {
        return SLC2PipelineStateVulkanBase::GetDescriptorSetLayout();
    }

    bool SLC2ComputePipelineStateVulkan::PrepareAndBindDescriptors(GPUContextVulkan*                        context,
                                                                   const ShaderBindingSnapshot&             snapshot,
                                                                   const List<SLC2VulkanDescriptorBinding>& bindings)
    {
        return SLC2PipelineStateVulkanBase::PrepareAndBindDescriptors(context, snapshot, bindings, VK_PIPELINE_BIND_POINT_COMPUTE);
    }

    void SLC2ComputePipelineStateVulkan::OnReleaseGPU() {}

    bool SLC2PipelineStateVulkanBase::SetupDescriptorWrites()
    {
        if (m_DescriptorWritesReady)
        {
            return true;
        }

        m_DescriptorWriteContainer.Release();
        m_DescriptorWriteSetIndices.Clear();
        m_DynamicOffsets.Clear();

        const DescriptorSetLayoutVulkan& descriptorSetLayout  = m_Layout->descriptorSetLayout;
        int32                            imageInfoCount       = 0;
        int32                            bufferInfoCount      = 0;
        int32                            texelBufferViewCount = 0;
        int32                            writeCount           = 0;
        int32                            dynamicOffsetCount   = 0;
        // 写入容器只按 layout 形状初始化一次，具体资源数据在 WriteSnapshot 中按快照填充。
        for (int32 setIndex = 0; setIndex < descriptorSetLayout.setLayouts.Count(); setIndex++)
        {
            const List<VkDescriptorSetLayoutBinding>& layoutBindings =
                descriptorSetLayout.setLayouts[setIndex].LayoutBindings;
            writeCount += layoutBindings.Count();
            for (int32 bindingIndex = 0; bindingIndex < layoutBindings.Count(); bindingIndex++)
            {
                const VkDescriptorSetLayoutBinding& layoutBinding = layoutBindings[bindingIndex];
                switch (layoutBinding.descriptorType)
                {
                    case VK_DESCRIPTOR_TYPE_SAMPLER:
                    case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
                    case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
                    case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
                        imageInfoCount += layoutBinding.descriptorCount;
                        break;
                    case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
                    case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
                    case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
                    case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
                        bufferInfoCount += layoutBinding.descriptorCount;
                        if (layoutBinding.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC ||
                            layoutBinding.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC)
                        {
                            dynamicOffsetCount++;
                        }
                        break;
                    case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
                    case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
                        texelBufferViewCount += layoutBinding.descriptorCount;
                        break;
                    default:
                       LOG_ERROR("Graphic", "SLC2 Vulkan descriptor type is not supported.");
                        return false;
                }
            }
        }

        m_DescriptorWriteContainer.descriptorWrites.AddZeroed(writeCount);
        m_DescriptorWriteContainer.descriptorImageInfo.AddZeroed(imageInfoCount);
        m_DescriptorWriteContainer.descriptorBufferInfo.AddZeroed(bufferInfoCount);
        m_DescriptorWriteContainer.descriptorTexelBufferView.AddZeroed(texelBufferViewCount);
        m_DescriptorWriteContainer.bindingToDynamicOffset.AddDefault(writeCount);
        m_DescriptorWriteContainer.bindingToDynamicOffset.SetAll(255);
        m_DescriptorWriteSetIndices.AddDefault(writeCount);
        m_DynamicOffsets.AddZeroed(dynamicOffsetCount);

        VkDescriptorImageInfo*  currentImageInfo       = m_DescriptorWriteContainer.descriptorImageInfo.Get();
        VkDescriptorBufferInfo* currentBufferInfo      = m_DescriptorWriteContainer.descriptorBufferInfo.Get();
        VkBufferView*           currentTexelBufferView = m_DescriptorWriteContainer.descriptorTexelBufferView.Get();
        int32                   writeIndex             = 0;
        int32                   dynamicOffsetIndex     = 0;
        for (int32 setIndex = 0; setIndex < descriptorSetLayout.setLayouts.Count(); setIndex++)
        {
            const List<VkDescriptorSetLayoutBinding>& layoutBindings =
                descriptorSetLayout.setLayouts[setIndex].LayoutBindings;
            for (int32 bindingIndex = 0; bindingIndex < layoutBindings.Count(); bindingIndex++)
            {
                const VkDescriptorSetLayoutBinding& layoutBinding = layoutBindings[bindingIndex];
                VkWriteDescriptorSet& descriptorWrite = m_DescriptorWriteContainer.descriptorWrites[writeIndex];
                VulkanTool::ZeroStruct(descriptorWrite, VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET);
                descriptorWrite.dstBinding              = layoutBinding.binding;
                descriptorWrite.dstArrayElement         = 0;
                descriptorWrite.descriptorCount         = layoutBinding.descriptorCount;
                descriptorWrite.descriptorType          = layoutBinding.descriptorType;
                m_DescriptorWriteSetIndices[writeIndex] = setIndex;

                switch (layoutBinding.descriptorType)
                {
                    case VK_DESCRIPTOR_TYPE_SAMPLER:
                    case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
                    case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
                    case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
                        descriptorWrite.pImageInfo = currentImageInfo;
                        currentImageInfo += layoutBinding.descriptorCount;
                        break;
                    case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
                    case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
                    case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
                    case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
                        descriptorWrite.pBufferInfo = currentBufferInfo;
                        currentBufferInfo += layoutBinding.descriptorCount;
                        if (layoutBinding.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC ||
                            layoutBinding.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC)
                        {
                            m_DescriptorWriteContainer.bindingToDynamicOffset[writeIndex] =
                                static_cast<byte>(dynamicOffsetIndex);
                            dynamicOffsetIndex++;
                        }
                        break;
                    case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
                    case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
                        descriptorWrite.pTexelBufferView = currentTexelBufferView;
                        currentTexelBufferView += layoutBinding.descriptorCount;
                        break;
                    default:
                        ENGINE_UNREACHABLE_CODE();
                        break;
                }
                writeIndex++;
            }
        }

        m_DescriptorWritesReady = true;
        return true;
    }

    bool SLC2PipelineStateVulkanBase::WriteSnapshot(GPUContextVulkan*                        context,
                                                       const ShaderBindingSnapshot&             snapshot,
                                                       const List<SLC2VulkanDescriptorBinding>& bindings)
    {
        // defaultUniformBuffer 通过完整性检查后再写入，显式资源随后按 BindingMap 补齐到对应 descriptor write。
        if (!WriteUniformData(context, snapshot, bindings))
        {
            return false;
        }

        const List<ShaderBindingResource>& resources = snapshot.GetResources();
        for (int32 resourceIndex = 0; resourceIndex < resources.Count(); resourceIndex++)
        {
            const ShaderBindingResource& resource = resources[resourceIndex];
            bool hasBinding = false;
            for (int32 bindingIndex = 0; bindingIndex < bindings.Count(); bindingIndex++)
            {
                const SLC2VulkanDescriptorBinding& binding = bindings[bindingIndex];
                if (binding.IsUniformData || binding.BlockId != resource.BlockId ||
                    binding.RangeIndex != resource.RangeIndex || binding.ResourceIndex != resource.ResourceIndex)
                {
                    continue;
                }
                if (!WriteResourceBinding(context, resource, binding))
                {
                    return false;
                }
                hasBinding = true;
            }
            if (!hasBinding)
            {
                LOG_ERROR("Graphic", "Runtime resource has no Vulkan descriptor binding.");
                return false;
            }
        }
        return true;
    }

    bool SLC2PipelineStateVulkanBase::ValidateSnapshotCompleteness(const ShaderBindingSnapshot&             snapshot,
                                                                      const List<SLC2VulkanDescriptorBinding>& bindings) const
    {
        const List<ShaderBindingResource>& resources = snapshot.GetResources();
        for (int32 bindingIndex = 0; bindingIndex < bindings.Count(); bindingIndex++)
        {
            const SLC2VulkanDescriptorBinding& binding = bindings[bindingIndex];
            if (binding.DescriptorWriteIndex >= static_cast<uint32>(m_DescriptorWriteContainer.descriptorWrites.Count()))
            {
                LOG_ERROR("Graphic", "SLC2 Vulkan descriptor binding references an invalid write index.");
                return false;
            }
            if (binding.IsUniformData)
            {
                if (binding.DescriptorType != VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC)
                {
                    LOG_ERROR("Graphic", "Default uniform buffer must map to a Vulkan dynamic uniform buffer descriptor.");
                    return false;
                }
                const ShaderBindingUniformData* uniformData = FindUniformData(snapshot, binding.BlockId);
                if (uniformData == nullptr || uniformData->Data.IsEmpty())
                {
                    LOG_ERROR("Graphic", "Vulkan descriptor layout has default uniform data that is missing from the binding snapshot.");
                    return false;
                }
                continue;
            }

            bool hasSnapshotResource = false;
            for (int32 resourceIndex = 0; resourceIndex < resources.Count(); resourceIndex++)
            {
                const ShaderBindingResource& resource = resources[resourceIndex];
                if (binding.BlockId == resource.BlockId && binding.RangeIndex == resource.RangeIndex &&
                    binding.ResourceIndex == resource.ResourceIndex)
                {
                    if (resource.Value == nullptr)
                    {
                        LOG_ERROR("Graphic", "Vulkan descriptor layout has a null runtime resource.");
                        return false;
                    }
                    if (!IsDescriptorCompatibleWithResource(binding.DescriptorType, resource.Type))
                    {
                        LOG_ERROR("Graphic", "Runtime resource type does not match the Vulkan descriptor type.");
                        return false;
                    }
                    hasSnapshotResource = true;
                    break;
                }
            }

            if (!hasSnapshotResource)
            {
                LOG_ERROR("Graphic", "Vulkan descriptor layout has an ordinary resource that is missing from the "
                                "binding snapshot.");
                return false;
            }
        }

        const List<ShaderBindingUniformData>& uniformDataList = snapshot.GetUniformData();
        for (int32 uniformIndex = 0; uniformIndex < uniformDataList.Count(); uniformIndex++)
        {
            const ShaderBindingUniformData& uniformData = uniformDataList[uniformIndex];
            for (int32 otherIndex = uniformIndex + 1; otherIndex < uniformDataList.Count(); otherIndex++)
            {
                if (uniformData.BlockId == uniformDataList[otherIndex].BlockId)
                {
                    LOG_ERROR("Graphic", "Binding snapshot contains duplicate default uniform data.");
                    return false;
                }
            }

            bool hasBinding = false;
            for (int32 bindingIndex = 0; bindingIndex < bindings.Count(); bindingIndex++)
            {
                const SLC2VulkanDescriptorBinding& binding = bindings[bindingIndex];
                if (binding.IsUniformData && binding.BlockId == uniformData.BlockId)
                {
                    hasBinding = true;
                    break;
                }
            }
            if (!hasBinding)
            {
                LOG_ERROR("Graphic", "Binding snapshot has default uniform data that is not used by the Vulkan descriptor layout.");
                return false;
            }
        }

        for (int32 resourceIndex = 0; resourceIndex < resources.Count(); resourceIndex++)
        {
            const ShaderBindingResource& resource = resources[resourceIndex];
            for (int32 otherIndex = resourceIndex + 1; otherIndex < resources.Count(); otherIndex++)
            {
                const ShaderBindingResource& other = resources[otherIndex];
                if (resource.BlockId == other.BlockId && resource.RangeIndex == other.RangeIndex &&
                    resource.ResourceIndex == other.ResourceIndex)
                {
                    LOG_ERROR("Graphic", "Binding snapshot contains duplicate runtime resources.");
                    return false;
                }
            }

            bool hasBinding = false;
            for (int32 bindingIndex = 0; bindingIndex < bindings.Count(); bindingIndex++)
            {
                const SLC2VulkanDescriptorBinding& binding = bindings[bindingIndex];
                if (!binding.IsUniformData && binding.BlockId == resource.BlockId &&
                    binding.RangeIndex == resource.RangeIndex && binding.ResourceIndex == resource.ResourceIndex)
                {
                    if (!IsDescriptorCompatibleWithResource(binding.DescriptorType, resource.Type))
                    {
                        LOG_ERROR("Graphic", "Runtime resource type does not match the Vulkan descriptor type.");
                        return false;
                    }
                    hasBinding = true;
                }
            }
            if (!hasBinding)
            {
                LOG_ERROR("Graphic", "Binding snapshot has a runtime resource that is not used by the Vulkan descriptor layout.");
                return false;
            }
        }
        return true;
    }

    bool SLC2PipelineStateVulkanBase::WriteResourceBinding(GPUContextVulkan*                   context,
                                                              const ShaderBindingResource&        resource,
                                                              const SLC2VulkanDescriptorBinding& binding)
    {
        VkWriteDescriptorSet& descriptorWrite =
            m_DescriptorWriteContainer.descriptorWrites[binding.DescriptorWriteIndex];
        if (binding.ArrayElement >= descriptorWrite.descriptorCount)
        {
            LOG_ERROR("Graphic", "Vulkan descriptor array element is outside the descriptor write range.");
            return false;
        }

        switch (binding.DescriptorType)
        {
            case VK_DESCRIPTOR_TYPE_SAMPLER: {
                const GPUSamplerVulkan* sampler = static_cast<const GPUSamplerVulkan*>(resource.Value);
                if (sampler->Sampler == VK_NULL_HANDLE)
                {
                    LOG_ERROR("Graphic", "Runtime sampler binding has no Vulkan sampler handle.");
                    return false;
                }
                VkDescriptorImageInfo*  imageInfo = const_cast<VkDescriptorImageInfo*>(descriptorWrite.pImageInfo + binding.ArrayElement);
                imageInfo->sampler = sampler->Sampler;
                break;
            }
            case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE: {
                GPUResourceView*          view   = static_cast<GPUResourceView*>(resource.Value);
                DescriptorResourceVulkan* handle = static_cast<DescriptorResourceVulkan*>(view->GetNativePtr());
                if (handle == nullptr)
                {
                    LOG_ERROR("Graphic", "Runtime texture binding has no Vulkan descriptor resource.");
                    return false;
                }
                VkImageView               imageView;
                VkImageLayout             layout;
                handle->AsImage(context, imageView, layout);
                VkDescriptorImageInfo* imageInfo = const_cast<VkDescriptorImageInfo*>(descriptorWrite.pImageInfo + binding.ArrayElement);
                imageInfo->imageView   = imageView;
                imageInfo->imageLayout = layout;
                break;
            }
            case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE: {
                GPUResourceView*          view   = static_cast<GPUResourceView*>(resource.Value);
                DescriptorResourceVulkan* handle = static_cast<DescriptorResourceVulkan*>(view->GetNativePtr());
                if (handle == nullptr)
                {
                    LOG_ERROR("Graphic", "Runtime storage image binding has no Vulkan descriptor resource.");
                    return false;
                }
                VkImageView               imageView;
                VkImageLayout             layout;
                handle->AsStorageImage(context, imageView, layout);
                VkDescriptorImageInfo* imageInfo =
                    const_cast<VkDescriptorImageInfo*>(descriptorWrite.pImageInfo + binding.ArrayElement);
                imageInfo->imageView   = imageView;
                imageInfo->imageLayout = layout;
                break;
            }
            case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER: {
                GPUBuffer*                buffer = static_cast<GPUBuffer*>(resource.Value);
                if (buffer->View() == nullptr)
                {
                    LOG_ERROR("Graphic", "Runtime storage buffer binding has no buffer view.");
                    return false;
                }
                DescriptorResourceVulkan* handle =
                        static_cast<DescriptorResourceVulkan*>(buffer->View()->GetNativePtr());
                if (handle == nullptr)
                {
                    LOG_ERROR("Graphic", "Runtime storage buffer binding has no Vulkan descriptor resource.");
                    return false;
                }
                VkBuffer     vkBuffer;
                VkDeviceSize offset;
                VkDeviceSize range;
                handle->AsStorageBuffer(context, vkBuffer, offset, range);
                VkDescriptorBufferInfo* bufferInfo =
                    const_cast<VkDescriptorBufferInfo*>(descriptorWrite.pBufferInfo + binding.ArrayElement);
                bufferInfo->buffer = vkBuffer;
                bufferInfo->offset = offset;
                bufferInfo->range  = range;
                break;
            }
            case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER: {
                GPUBuffer*           buffer = static_cast<GPUBuffer*>(resource.Value);
                GPUBufferViewVulkan* handle = static_cast<GPUBufferViewVulkan*>(buffer->View());
                if (handle == nullptr)
                {
                    LOG_ERROR("Graphic", "Runtime texel buffer binding has no Vulkan buffer view.");
                    return false;
                }
                VkBufferView         bufferView;
                handle->AsUniformTexelBuffer(context, bufferView);
                VkBufferView* texelBufferView =
                    const_cast<VkBufferView*>(descriptorWrite.pTexelBufferView + binding.ArrayElement);
                *texelBufferView = bufferView;
                break;
            }
            case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER: {
                GPUBuffer*           buffer = static_cast<GPUBuffer*>(resource.Value);
                GPUBufferViewVulkan* handle = static_cast<GPUBufferViewVulkan*>(buffer->View());
                if (handle == nullptr)
                {
                    LOG_ERROR("Graphic", "Runtime storage texel buffer binding has no Vulkan buffer view.");
                    return false;
                }
                VkBufferView         bufferView;
                handle->AsStorageTexelBuffer(context, bufferView);
                VkBufferView* texelBufferView =
                    const_cast<VkBufferView*>(descriptorWrite.pTexelBufferView + binding.ArrayElement);
                *texelBufferView = bufferView;
                break;
            }
            default:
                LOG_ERROR("Graphic", "Runtime descriptor type is not supported by ordinary resource binding.");
                return false;
        }
        return true;
    }

    bool SLC2PipelineStateVulkanBase::IsDescriptorCompatibleWithResource(const VkDescriptorType descriptorType,
                                                                            const ShaderRuntimeResourceType resourceType) const
    {
        switch (descriptorType)
        {
            case VK_DESCRIPTOR_TYPE_SAMPLER:
                return resourceType == ShaderRuntimeResourceType::Sampler;
            case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
            case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
                return resourceType == ShaderRuntimeResourceType::Texture;
            case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
            case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
            case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
                return resourceType == ShaderRuntimeResourceType::Buffer;
            default:
                return false;
        }
    }

    bool SLC2PipelineStateVulkanBase::WriteUniformData(GPUContextVulkan*                        context,
                                                          const ShaderBindingSnapshot&             snapshot,
                                                          const List<SLC2VulkanDescriptorBinding>& bindings)
    {
        const List<ShaderBindingUniformData>& uniformDataList = snapshot.GetUniformData();
        for (int32 bindingIndex = 0; bindingIndex < bindings.Count(); bindingIndex++)
        {
            const SLC2VulkanDescriptorBinding& binding = bindings[bindingIndex];
            if (!binding.IsUniformData)
            {
                continue;
            }
            if (binding.DescriptorType != VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC)
            {
                LOG_ERROR("Graphic", "Default uniform buffer must map to a Vulkan dynamic uniform buffer descriptor.");
                return false;
            }

            // uniform 数据以 blockId 匹配 defaultUniformBuffer，保持普通 uniform 裁剪后的布局与运行时一致。
            const ShaderBindingUniformData* uniformData = FindUniformData(snapshot, binding.BlockId);
            if (uniformData == nullptr)
            {
                LOG_ERROR("Graphic", " Vulkan descriptor layout has default uniform data that is missing from the binding snapshot.");
                return false;
            }
            if (uniformData->Data.IsEmpty())
            {
                LOG_ERROR("Graphic", "Default uniform buffer data is empty.");
                return false;
            }

            VkWriteDescriptorSet& descriptorWrite =
                m_DescriptorWriteContainer.descriptorWrites[binding.DescriptorWriteIndex];
            if (binding.ArrayElement >= descriptorWrite.descriptorCount)
            {
                LOG_ERROR("Graphic", "Vulkan default uniform buffer array element is outside the descriptor write range.");
                return false;
            }

            const UniformBufferUploaderVulkan::Allocation allocation =
                m_Device->uniformBufferUploader->Allocate(uniformData->Data.Count(), 0, context);
            Platform::MemoryClear(allocation.CPUAddress, allocation.Size);
            Platform::MemoryCopy(allocation.CPUAddress, uniformData->Data.Get(), uniformData->Data.Count());

            VkDescriptorBufferInfo* bufferInfo =
                const_cast<VkDescriptorBufferInfo*>(descriptorWrite.pBufferInfo + binding.ArrayElement);
            bufferInfo->buffer = allocation.Buffer;
            bufferInfo->offset = 0;
            bufferInfo->range  = allocation.Size;

            const byte dynamicOffsetIndex =
                m_DescriptorWriteContainer.bindingToDynamicOffset[binding.DescriptorWriteIndex];
            if (dynamicOffsetIndex == 255)
            {
                LOG_ERROR("Graphic", "Default uniform buffer descriptor has no dynamic offset slot.");
                return false;
            }
            m_DynamicOffsets[dynamicOffsetIndex] = static_cast<uint32>(allocation.Offset);
        }

        for (int32 uniformIndex = 0; uniformIndex < uniformDataList.Count(); uniformIndex++)
        {
            const ShaderBindingUniformData& uniformData = uniformDataList[uniformIndex];
            bool                            hasBinding  = false;
            for (int32 bindingIndex = 0; bindingIndex < bindings.Count(); bindingIndex++)
            {
                const SLC2VulkanDescriptorBinding& binding = bindings[bindingIndex];
                if (binding.IsUniformData && binding.BlockId == uniformData.BlockId)
                {
                    hasBinding = true;
                    break;
                }
            }
            if (!hasBinding)
            {
                LOG_ERROR("Graphic", "Binding snapshot has default uniform data that is not used by the Vulkan descriptor layout.");
                return false;
            }
        }
        return true;
    }

    const ShaderBindingUniformData* SLC2PipelineStateVulkanBase::FindUniformData(const ShaderBindingSnapshot& snapshot, const uint32 blockId) const
    {
        const List<ShaderBindingUniformData>& uniformDataList = snapshot.GetUniformData();
        for (int32 index = 0; index < uniformDataList.Count(); index++)
        {
            if (uniformDataList[index].BlockId == blockId)
            {
                return &uniformDataList[index];
            }
        }
        return nullptr;
    }

    void SLC2PipelineStateVulkanBase::ReleaseDescriptorState()
    {
        if (m_CurrentTypedDescriptorPoolSet != nullptr)
        {
            m_CurrentTypedDescriptorPoolSet->GetOwner()->refs--;
            m_CurrentTypedDescriptorPoolSet = nullptr;
        }
        m_DescriptorWriteContainer.Release();
        m_DescriptorSetHandles.Clear();
        m_DescriptorWriteSetIndices.Clear();
        m_DynamicOffsets.Clear();
        m_DescriptorWritesReady = false;
    }

    SLC2GraphicsPipelineStateVulkan::SLC2GraphicsPipelineStateVulkan(GPUDeviceVulkan* device) :
        SLC2PipelineStateVulkanBase(device),
        m_Pipelines(16)
    {
        blendEnable = 0;
        depthReadEnable = 0;
        depthWriteEnable = 0;
        stencilReadEnable = 0;
        stencilWriteEnable = 0;
        Platform::MemoryClear(m_ShaderStagesMark, sizeof(m_ShaderStagesMark));
    }

    SLC2GraphicsPipelineStateVulkan::~SLC2GraphicsPipelineStateVulkan()
    {
        ReleasePipelines();
        m_Program = nullptr;
    }

    bool SLC2GraphicsPipelineStateVulkan::Init(const Description& desc, const SLC2GPUShaderProgram* program)
    {
        SLC2GPUShaderProgram* programConst = const_cast<SLC2GPUShaderProgram*>(program);
        m_Program                          = static_cast<SLC2ShaderProgramVulkan*>(programConst);
        if (m_Program == nullptr)
        {
            LOG_ERROR("Graphic", "SLC2 graphics pipeline requires a shader program.");
            return false;
        }

        m_Layout = static_cast<PipelineLayoutVulkan*>(m_Program->GetPipelineLayout());

        if (m_Layout == nullptr)
        {
            LOG_ERROR("Graphic", "SLC2 graphics pipeline requires a pipeline layout.");
            return false;
        }

        const SLC2VariantRecord* variant = m_Program->GetVariant();
        if (variant == nullptr)
        {
            LOG_ERROR("Graphic", "SLC2 graphics pipeline requires a selected variant.");
            return false;
        }

        m_DescKey = desc;
        VulkanTool::ZeroStruct(m_Desc, VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO);

        for (int32 stageIndex = 0; stageIndex < variant->Stages.Count(); stageIndex++)
        {
            ShaderStage stage = variant->Stages[stageIndex].Stage;
            if (stage == ShaderStage::Compute)
            {
                LOG_ERROR("Graphic", "SLC2 graphics pipeline cannot be created from a compute variant.");
                return false;
            }

            VkShaderStageFlagBits stageFlag;
            if (!VulkanTool::ToVulkanShaderStage(stage, stageFlag))
            {
                return false;
            }

            if (!AddShaderStage(stage, stageFlag))
            {
                return false;
            }
        }

        if (!m_ShaderStagesMark[(int)ShaderStage::Vertex])
        {
            LOG_ERROR("Graphic", "SLC2 graphics pipeline requires a vertex stage.");
            return false;
        }

        m_Desc.pStages = m_ShaderStages;

/*        if (desc.VS != nullptr)
        {
            // P6.0 先复用旧 VS 输入布局描述；无 VS 时走空 vertex input，用于 fullscreen/triangle smoke。
            m_DescVertexInput = *static_cast<const VkPipelineVertexInputStateCreateInfo*>(desc.VS->GetInputLayout());
        }
        else*/
        {
            VulkanTool::ZeroStruct(m_DescVertexInput, VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO);
        }
        m_Desc.pVertexInputState = &m_DescVertexInput;

        VulkanTool::ZeroStruct(m_DescInputAssembly, VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO);
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
        default:
            m_DescInputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            break;
        }

        if (m_ShaderStagesMark[(int)ShaderStage::Domain])
        {
            m_DescInputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
        }
        m_Desc.pInputAssemblyState = &m_DescInputAssembly;

        if (m_ShaderStagesMark[(int)ShaderStage::Hull])
        {
            int32 outputControlPoints = 0;
            for (int32 stageIndex = 0; stageIndex < variant->Stages.Count(); stageIndex++)
            {
                SLC2StageRecord stageRecord = variant->Stages[stageIndex];
                if (stageRecord.Stage == ShaderStage::Hull)
                {
                    outputControlPoints = stageRecord.OutputControlPoints;
                }
            }

            if (outputControlPoints <= 0 || outputControlPoints > 32)
            {
                LOG_ERROR("Graphic", "SLC2 graphics pipeline requires valid hull shader output control points.");
                return false;
            }
            VulkanTool::ZeroStruct(m_DescTessellation, VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO);
            m_DescTessellation.patchControlPoints = static_cast<uint32>(outputControlPoints);
            m_Desc.pTessellationState = &m_DescTessellation;
        }

        VulkanTool::ZeroStruct(m_DescViewport, VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO);
        m_DescViewport.viewportCount = 1;
        m_DescViewport.scissorCount = 1;
        m_Desc.pViewportState = &m_DescViewport;

        VulkanTool::ZeroStruct(m_DescDynamic, VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO);
        m_DescDynamic.pDynamicStates = m_DynamicStates;
        m_DynamicStates[m_DescDynamic.dynamicStateCount++] = VK_DYNAMIC_STATE_VIEWPORT;
        m_DynamicStates[m_DescDynamic.dynamicStateCount++] = VK_DYNAMIC_STATE_SCISSOR;
        m_DynamicStates[m_DescDynamic.dynamicStateCount++] = VK_DYNAMIC_STATE_STENCIL_REFERENCE;
        static_assert(ARRAY_SIZE(m_DynamicStates) <= 3, "Invalid dynamic states array.");
        m_Desc.pDynamicState = &m_DescDynamic;

        VulkanTool::ZeroStruct(m_DescMultisample, VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO);
        m_DescMultisample.minSampleShading = 0.0f;
        m_DescMultisample.alphaToCoverageEnable = desc.BlendMode.AlphaToCoverageEnable;
        m_DescMultisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        m_Desc.pMultisampleState = &m_DescMultisample;

        VulkanTool::ZeroStruct(m_DescDepthStencil, VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO);
        m_DescDepthStencil.depthTestEnable = desc.DepthEnable;
        m_DescDepthStencil.depthWriteEnable = desc.DepthWriteEnable;
        m_DescDepthStencil.depthCompareOp = VulkanTool::ConvertCompareOp(desc.DepthFunc);
        m_DescDepthStencil.stencilTestEnable = desc.StencilEnable;
        m_DescDepthStencil.front.compareMask = desc.StencilReadMask;
        m_DescDepthStencil.front.writeMask = desc.StencilWriteMask;
        m_DescDepthStencil.front.compareOp = VulkanTool::ConvertCompareOp(desc.StencilFunc);
        m_DescDepthStencil.front.failOp      = VulkanTool::ConvertStencilOp(desc.StencilFailOp);
        m_DescDepthStencil.front.depthFailOp = VulkanTool::ConvertStencilOp(desc.StencilDepthFailOp);
        m_DescDepthStencil.front.passOp      = VulkanTool::ConvertStencilOp(desc.StencilPassOp);
        m_DescDepthStencil.back = m_DescDepthStencil.front;
        m_Desc.pDepthStencilState = &m_DescDepthStencil;
        depthReadEnable = desc.DepthEnable && desc.DepthFunc != ComparisonFunc::Always;
        depthWriteEnable = m_DescDepthStencil.depthWriteEnable;
        stencilReadEnable = desc.StencilEnable && desc.StencilReadMask != 0 && desc.StencilFunc != ComparisonFunc::Always;
        stencilWriteEnable = desc.StencilEnable && desc.StencilWriteMask != 0;

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
        default:
            m_DescRasterization.cullMode = VK_CULL_MODE_BACK_BIT;
            break;
        }
        m_DescRasterization.frontFace = VK_FRONT_FACE_CLOCKWISE;
        m_DescRasterization.depthClampEnable = !desc.DepthClipEnable && m_Device->GetGPULimits().HasDepthClip;
        m_DescRasterization.lineWidth = 1.0f;
        m_Desc.pRasterizationState = &m_DescRasterization;

        blendEnable = desc.BlendMode.BlendEnable;
        VulkanTool::ZeroStruct(m_DescColorBlend, VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO);
        VkPipelineColorBlendAttachmentState& blend = m_DescColorBlendAttachments[0];
        blend.blendEnable = desc.BlendMode.BlendEnable;
        blend.srcColorBlendFactor = VulkanTool::ToVulkanBlendFactor(desc.BlendMode.SrcBlend);
        blend.dstColorBlendFactor = VulkanTool::ToVulkanBlendFactor(desc.BlendMode.DestBlend);
        blend.colorBlendOp = VulkanTool::ToVulkanBlendOp(desc.BlendMode.BlendOp);
        blend.srcAlphaBlendFactor = VulkanTool::ToVulkanBlendFactor(desc.BlendMode.SrcBlendAlpha);
        blend.dstAlphaBlendFactor = VulkanTool::ToVulkanBlendFactor(desc.BlendMode.DestBlendAlpha);
        blend.alphaBlendOp = VulkanTool::ToVulkanBlendOp(desc.BlendMode.BlendOpAlpha);
        blend.colorWriteMask = static_cast<VkColorComponentFlags>(desc.BlendMode.RenderTargetWriteMask);
        for (int32 index = 1; index < GPU_MAX_RT_BINDED; index++)
        {
            m_DescColorBlendAttachments[index] = m_DescColorBlendAttachments[index - 1];
        }
        m_DescColorBlend.pAttachments = m_DescColorBlendAttachments;
        m_DescColorBlend.blendConstants[0] = 0.0f;
        m_DescColorBlend.blendConstants[1] = 0.0f;
        m_DescColorBlend.blendConstants[2] = 0.0f;
        m_DescColorBlend.blendConstants[3] = 0.0f;
        m_Desc.pColorBlendState = &m_DescColorBlend;
        m_Desc.layout = m_Layout->handle;
        return true;
    }

    bool SLC2GraphicsPipelineStateVulkan::Matches(const Description& desc) const
    {
        return m_DescKey.DepthEnable == desc.DepthEnable &&
               m_DescKey.DepthWriteEnable == desc.DepthWriteEnable &&
               m_DescKey.DepthClipEnable == desc.DepthClipEnable &&
               m_DescKey.DepthFunc == desc.DepthFunc &&
               m_DescKey.StencilEnable == desc.StencilEnable &&
               m_DescKey.StencilReadMask == desc.StencilReadMask &&
               m_DescKey.StencilWriteMask == desc.StencilWriteMask &&
               m_DescKey.StencilFunc == desc.StencilFunc &&
               m_DescKey.StencilFailOp == desc.StencilFailOp &&
               m_DescKey.StencilDepthFailOp == desc.StencilDepthFailOp &&
               m_DescKey.StencilPassOp == desc.StencilPassOp &&
               m_DescKey.PrimitiveTopology == desc.PrimitiveTopology &&
               m_DescKey.Wireframe == desc.Wireframe &&
               m_DescKey.CullMode == desc.CullMode &&
               m_DescKey.BlendMode == desc.BlendMode;
    }

    VkPipeline SLC2GraphicsPipelineStateVulkan::GetState(RenderPassVulkan* renderPass)
    {
        ENGINE_ASSERT(renderPass);

        VkPipeline pipeline = VK_NULL_HANDLE;
        if (m_Pipelines.TryGet(renderPass, pipeline))
        {
            return pipeline;
        }

        PROFILE_CPU_NAMED("Create SLC2 Graphics Pipeline");
        m_DescColorBlend.attachmentCount = renderPass->layout.RTsCount;
        m_DescMultisample.rasterizationSamples = static_cast<VkSampleCountFlagBits>(renderPass->layout.MSAA);
        m_Desc.renderPass = renderPass->handle;
        m_Desc.layout = m_Layout->handle;

        const VkResult result = vkCreateGraphicsPipelines(m_Device->device, m_Device->pipelineCache, 1, &m_Desc, nullptr, &pipeline);
        LOG_VULKAN_RESULT(result);
        if (result != VK_SUCCESS)
        {
            LOG_ERROR("Graphic", "SLC2 graphics pipeline creation failed.");
            return VK_NULL_HANDLE;
        }

        m_Pipelines.Add(renderPass, pipeline);
        return pipeline;
    }

    PipelineLayoutVulkan* SLC2GraphicsPipelineStateVulkan::GetLayout() const
    {
        return SLC2PipelineStateVulkanBase::GetLayout();
    }

    const VkPipelineVertexInputStateCreateInfo* SLC2GraphicsPipelineStateVulkan::GetVertexInputState() const
    {
        return &m_DescVertexInput;
    }

    bool SLC2GraphicsPipelineStateVulkan::PrepareAndBindDescriptors(GPUContextVulkan*                        context,
                                                                    const ShaderBindingSnapshot&             snapshot,
                                                                    const List<SLC2VulkanDescriptorBinding>& bindings)
    {
        return SLC2PipelineStateVulkanBase::PrepareAndBindDescriptors(context, snapshot, bindings, VK_PIPELINE_BIND_POINT_GRAPHICS);
    }

    void SLC2GraphicsPipelineStateVulkan::OnReleaseGPU() {}

    void SLC2GraphicsPipelineStateVulkan::ReleasePipelines()
    {
        for (auto iterator = m_Pipelines.begin(); iterator.IsNotEnd(); ++iterator)
        {
            m_Device->deferredDeletionQueue.EnqueueResource(DeferredDeletionQueueVulkan::Type::Pipeline, iterator->Value);
        }
        m_Pipelines.Clear();
    }

    bool SLC2GraphicsPipelineStateVulkan::AddShaderStage(const ShaderStage stage,
                                                         const VkShaderStageFlagBits stageFlag)
    {
        if (m_Desc.stageCount >= ARRAY_SIZE(m_ShaderStages))
        {
            LOG_ERROR("Graphic", "SLC2 graphics pipeline has too many shader stages.");
            return false;
        }

        VkShaderModule shaderModule = m_Program->GetShaderModule(stage);
        const char* entryPoint = m_Program->GetEntryPoint(stage);
        if (shaderModule == VK_NULL_HANDLE || entryPoint == nullptr)
        {
            LOG_ERROR("Graphic", "SLC2 graphics shader module or entry point is missing.");
            return false;
        }

        m_ShaderStagesMark[(int)stage] = true;

        VkPipelineShaderStageCreateInfo& stageDesc = m_ShaderStages[m_Desc.stageCount++];
        VulkanTool::ZeroStruct(stageDesc, VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO);
        stageDesc.stage = stageFlag;
        stageDesc.module = shaderModule;
        stageDesc.pName = entryPoint;
        return true;
    }

}
