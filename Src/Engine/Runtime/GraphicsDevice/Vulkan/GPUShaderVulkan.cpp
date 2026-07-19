
#include "GPUShaderVulkan.h"

#include "Runtime/Core/Types/Collections/DataContainer.h"

#include "VulkanTool.h"
#include "GPUShaderProgramVulkan.h"
#include "DescriptorSetVulkan.h"
#include "GPUDeviceVulkan.h"
#include "GPUPipelineStateVulkan.h"
#include "GPUContextVulkan.h"

namespace SE
{
	#define VULKAN_UNIFORM_RING_BUFFER_SIZE (24 * 1024 * 1024)

	UniformBufferUploaderVulkan::UniformBufferUploaderVulkan(GPUDeviceVulkan* device)
		: GPUResourceVulkan(device, TEXT("Uniform Buffer Uploader"))
		, m_Size(VULKAN_UNIFORM_RING_BUFFER_SIZE)
		, m_Offset(0)
		, m_Mapped(nullptr)
		, m_FenceCmdBuffer(nullptr)
		, m_FenceCounter(0)
	{
		m_MinAlignment = (uint32)device->physicalDeviceLimits.minUniformBufferOffsetAlignment;

		// Setup buffer description
		VkBufferCreateInfo bufferInfo;
		VulkanTool::ZeroStruct(bufferInfo, VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO);
		bufferInfo.size = m_Size;
		bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

		// Create buffer
		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
		VkResult result = vmaCreateBuffer(m_Device->allocator, &bufferInfo, &allocInfo, &m_Buffer, &m_Allocation, nullptr);
		LOG_VULKAN_RESULT(result);
		m_MemoryUsage = bufferInfo.size;

		// Map buffer
		result = vmaMapMemory(m_Device->allocator, m_Allocation, (void**)&m_Mapped);
		LOG_VULKAN_RESULT(result);
	}

	UniformBufferUploaderVulkan::Allocation UniformBufferUploaderVulkan::Allocate(uint64 size, uint32 alignment, GPUContextVulkan* context)
	{
		alignment = Math::Max(m_MinAlignment, alignment);
		uint64 offset = Math::AlignUp<uint64>(m_Offset, alignment);

		// Check if wrap around ring buffer
		if (offset + size >= m_Size)
		{
			auto cmdBuffer = context->GetCmdBufferManager()->GetActiveCmdBuffer();
			if (m_FenceCmdBuffer && m_FenceCounter == cmdBuffer->GetFenceSignaledCounter())
			{
				LOG_ERROR("Graphic", "UniformBufferUploader Wrapped around the ring buffer! Need to wait on the GPU!");
				context->Flush();
				cmdBuffer = context->GetCmdBufferManager()->GetActiveCmdBuffer();
			}

			offset = 0;
			m_Offset = size;

			m_FenceCmdBuffer = cmdBuffer;
			m_FenceCounter = cmdBuffer->GetSubmittedFenceCounter();
		}
		else
		{
			// Move within the buffer
			m_Offset = offset + size;
		}

		Allocation result;
		result.Offset = offset;
		result.Size = size;
		result.Buffer = m_Buffer;
		result.CPUAddress = m_Mapped + offset;
		return result;
	}

	void UniformBufferUploaderVulkan::OnReleaseGPU()
	{
		if (m_Allocation != VK_NULL_HANDLE)
		{
			if (m_Mapped)
			{
				vmaUnmapMemory(m_Device->allocator, m_Allocation);
				m_Mapped = nullptr;
			}
			vmaDestroyBuffer(m_Device->allocator, m_Buffer, m_Allocation);
			m_Buffer = VK_NULL_HANDLE;
			m_Allocation = VK_NULL_HANDLE;
		}
	}


	GPUShaderVulkan::GPUShaderVulkan(GPUDeviceVulkan* device, const StringView& name) : GPUResourceVulkan<GPUShader>(device, name)
	{
	}

	GPUShaderProgram* GPUShaderVulkan::CreateGPUShaderProgram(ShaderStage type, const GPUShaderProgramInitializer& initializer,
		byte* cacheBytes, uint32 cacheSize, MemoryReadStream& stream)
	{
		// Extract the SPIR-V shader header from the cache
		SpirvShaderHeader* header = (SpirvShaderHeader*)cacheBytes;
		cacheBytes += sizeof(SpirvShaderHeader);
		cacheSize -= sizeof(SpirvShaderHeader);

		// Extract the SPIR-V bytecode
		BytesContainer spirv;
		ENGINE_ASSERT(header->type == SpirvShaderHeader::Types::Raw);
		spirv.Link(cacheBytes, cacheSize);

		// Create shader module from SPIR-V bytecode
		VkShaderModule shaderModule = VK_NULL_HANDLE;
		VkShaderModuleCreateInfo createInfo;
		VulkanTool::ZeroStruct(createInfo, VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO);
		createInfo.codeSize = (size_t)spirv.Length();
		createInfo.pCode = (const uint32_t*)spirv.Get();
#if VULKAN_USE_VALIDATION_CACHE
		VkShaderModuleValidationCacheCreateInfoEXT validationInfo;
		if (m_Device->ValidationCache != VK_NULL_HANDLE)
		{
			VulkanTool::ZeroStruct(validationInfo, VK_STRUCTURE_TYPE_SHADER_MODULE_VALIDATION_CACHE_CREATE_INFO_EXT);
			validationInfo.validationCache = m_Device->ValidationCache;
			createInfo.pNext = &validationInfo;
		}
#endif
		VALIDATE_VULKAN_RESULT(vkCreateShaderModule(m_Device->device, &createInfo, nullptr, &shaderModule));

#if GPU_ENABLE_RESOURCE_NAMING
		VK_SET_DEBUG_NAME(m_Device, shaderModule, VK_OBJECT_TYPE_SHADER_MODULE, initializer.Name);
#endif

		GPUShaderProgram* shader = nullptr;
		switch (type)
		{
		case ShaderStage::Vertex:
		{
			// Create object
			auto vsShader = New<GPUShaderProgramVSVulkan>(m_Device, initializer, header->descriptorInfo, shaderModule);
			shader = vsShader;
			VkPipelineVertexInputStateCreateInfo& inputState = vsShader->VertexInputState;
			VkVertexInputBindingDescription* vertexBindingDescriptions = vsShader->VertexBindingDescriptions;
			VkVertexInputAttributeDescription* vertexAttributeDescriptions = vsShader->VertexAttributeDescriptions;
			VulkanTool::ZeroStruct(inputState, VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO);
			for (int32 i = 0; i < VERTEX_SHADER_MAX_INPUT_ELEMENTS; i++)
			{
				vertexBindingDescriptions[i].binding = i;
				vertexBindingDescriptions[i].stride = 0;
				vertexBindingDescriptions[i].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
			}

			// Load Input Layout (it may be empty)
			byte inputLayoutSize = stream.ReadByte();
			ENGINE_ASSERT(inputLayoutSize <= VERTEX_SHADER_MAX_INPUT_ELEMENTS);
			uint32 attributesCount = inputLayoutSize;
			uint32 bindingsCount = 0;
			int32 offset = 0;
			for (int32 a = 0; a < inputLayoutSize; a++)
			{
				// Read description
				GPUShaderProgramVS::InputElement inputElement;
				stream.ReadBytes(&inputElement, sizeof(GPUShaderProgramVS::InputElement));

				const auto size = PixelFormatGetSizeInBytes((PixelFormat)inputElement.Format);
				if (inputElement.AlignedByteOffset != INPUT_LAYOUT_ELEMENT_ALIGN)
					offset = inputElement.AlignedByteOffset;

				auto& vertexBindingDescription = vertexBindingDescriptions[inputElement.InputSlot];
				vertexBindingDescription.binding = inputElement.InputSlot;
				vertexBindingDescription.stride = Math::Max(vertexBindingDescription.stride, (uint32_t)(offset + size));
				vertexBindingDescription.inputRate =
					inputElement.InputSlotClass == INPUT_LAYOUT_ELEMENT_PER_VERTEX_DATA ? VK_VERTEX_INPUT_RATE_VERTEX
																						: VK_VERTEX_INPUT_RATE_INSTANCE;
				ENGINE_ASSERT(inputElement.InstanceDataStepRate == 0 || inputElement.InstanceDataStepRate == 1);

				auto& vertexAttributeDescription = vertexAttributeDescriptions[a];
				vertexAttributeDescription.location = a;
				vertexAttributeDescription.binding = inputElement.InputSlot;
				vertexAttributeDescription.format = VulkanTool::ToVulkanFormat((PixelFormat)inputElement.Format);
				vertexAttributeDescription.offset = offset;

				bindingsCount = Math::Max(bindingsCount, (uint32)inputElement.InputSlot + 1);
				offset += size;
			}

			inputState.vertexBindingDescriptionCount = bindingsCount;
			inputState.pVertexBindingDescriptions = vertexBindingDescriptions;

			inputState.vertexAttributeDescriptionCount = attributesCount;
			inputState.pVertexAttributeDescriptions = vertexAttributeDescriptions;

			break;
		}
		case ShaderStage::Hull:
		{
			int32 controlPointsCount;
			stream.ReadInt32(&controlPointsCount);
			shader = New<GPUShaderProgramHSVulkan>(m_Device,
				initializer,
				header->descriptorInfo,
				shaderModule,
				controlPointsCount);
			break;
		}
		case ShaderStage::Domain:
		{
			shader = New<GPUShaderProgramDSVulkan>(m_Device, initializer, header->descriptorInfo, shaderModule);
			break;
		}
		case ShaderStage::Geometry:
		{
			shader = New<GPUShaderProgramGSVulkan>(m_Device, initializer, header->descriptorInfo, shaderModule);
			break;
		}

		case ShaderStage::Pixel:
		{
			shader = New<GPUShaderProgramPSVulkan>(m_Device, initializer, header->descriptorInfo, shaderModule);
			break;
		}
		case ShaderStage::Compute:
		{
			shader = New<GPUShaderProgramCSVulkan>(m_Device, initializer, header->descriptorInfo, shaderModule);
			break;
		}
		}
		return shader;
	}


	GPUShaderProgramCSVulkan::~GPUShaderProgramCSVulkan()
	{
		if (m_PipelineState)
		{
			Delete(m_PipelineState);
		}
	}

	ComputePipelineStateVulkan* GPUShaderProgramCSVulkan::GetOrCreateState()
	{
		if (m_PipelineState)
			return m_PipelineState;

		// Create pipeline layout
		DescriptorSetLayoutInfoVulkan descriptorSetLayoutInfo;
		descriptorSetLayoutInfo.AddBindingsForStage(VK_SHADER_STAGE_COMPUTE_BIT, DescriptorSet::Compute, &descriptorInfo);
		const auto layout = m_Device->GetOrCreateLayout(descriptorSetLayoutInfo);

		// Create pipeline description
		VkComputePipelineCreateInfo desc;
		VulkanTool::ZeroStruct(desc, VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO);
		desc.basePipelineIndex = -1;
		desc.layout = layout->handle;
		VulkanTool::ZeroStruct(desc.stage, VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO);
		auto& stage = desc.stage;
		VulkanTool::ZeroStruct(stage, VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO);
		stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		stage.module = (VkShaderModule)GetBufferHandle();
		stage.pName = GetName().ToStringAnsi().Get();

		// Create pipeline object
		VkPipeline pipeline;
		const VkResult result = vkCreateComputePipelines(m_Device->device, m_Device->pipelineCache, 1, &desc, nullptr, &pipeline);
		LOG_VULKAN_RESULT(result);
		if (result != VK_SUCCESS)
			return nullptr;

		// Setup the state
		m_PipelineState = New<ComputePipelineStateVulkan>(m_Device, pipeline, layout);
		m_PipelineState->descriptorInfo = &descriptorInfo;
		m_PipelineState->descriptorSetsLayout = &layout->descriptorSetLayout;
		m_PipelineState->descriptorSetHandles.AddZeroed(m_PipelineState->descriptorSetsLayout->handles.Count());
		uint32 dynamicOffsetsCount = 0;
		if (descriptorInfo.descriptorTypesCount != 0)
		{
			// TODO: merge into a single allocation
			m_PipelineState->dsWriteContainer.descriptorWrites.AddZeroed(descriptorInfo.descriptorTypesCount);
			m_PipelineState->dsWriteContainer.descriptorImageInfo.AddZeroed(descriptorInfo.imageInfosCount);
			m_PipelineState->dsWriteContainer.descriptorBufferInfo.AddZeroed(descriptorInfo.bufferInfosCount);
			m_PipelineState->dsWriteContainer.descriptorTexelBufferView.AddZeroed(descriptorInfo.texelBufferViewsCount);

			ENGINE_ASSERT(descriptorInfo.descriptorTypesCount < 255);
			m_PipelineState->dsWriteContainer.bindingToDynamicOffset.AddDefault(descriptorInfo.descriptorTypesCount);
			m_PipelineState->dsWriteContainer.bindingToDynamicOffset.SetAll(255);

			VkWriteDescriptorSet* currentDescriptorWrite = m_PipelineState->dsWriteContainer.descriptorWrites.Get();
			VkDescriptorImageInfo* currentImageInfo = m_PipelineState->dsWriteContainer.descriptorImageInfo.Get();
			VkDescriptorBufferInfo* currentBufferInfo = m_PipelineState->dsWriteContainer.descriptorBufferInfo.Get();
			VkBufferView* currentTexelBufferView = m_PipelineState->dsWriteContainer.descriptorTexelBufferView.Get();
			uint8* currentBindingToDynamicOffsetMap = m_PipelineState->dsWriteContainer.bindingToDynamicOffset.Get();

			dynamicOffsetsCount = m_PipelineState->dsWriter.SetupDescriptorWrites(descriptorInfo, currentDescriptorWrite, currentImageInfo, currentBufferInfo, currentTexelBufferView, currentBindingToDynamicOffsetMap);
		}

		m_PipelineState->DynamicOffsets.AddZeroed(dynamicOffsetsCount);
		m_PipelineState->dsWriter.dynamicOffsets = m_PipelineState->DynamicOffsets.Get();

		return m_PipelineState;
	}



	GPUConstantBufferVulkan::GPUConstantBufferVulkan(GPUDeviceVulkan* device, uint32 size) noexcept
		: GPUResourceVulkan(device, String::Empty)
	{
		m_Size = size;
	}
} // SE