
#include "GPUShaderVulkan.h"

#include "Runtime/Core/Types/Collections/DataContainer.h"

#include "VulkanTool.h"
#include "GPUShaderProgramVulkan.h"
#include "DescriptorSetVulkan.h"
#include "GPUDeviceVulkan.h"
#include "GPUPipelineStateVulkan.h"
#include "GPUContextVulkan.h"
#include <Runtime/Graphics/Shaders/SLC2GPUShaderProgram.h>

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

	/// /////////////////////////////////////////////////////////////////////////

	SLC2GPUShaderVulkan::SLC2GPUShaderVulkan(GPUDeviceVulkan* device, const StringView& name) :
        GPUResourceVulkan<SLC2GPUShader>(device, name)
    {}

    SLC2GPUShaderProgram* SLC2GPUShaderVulkan::CreateSLC2ShaderProgram(const SLC2ProgramRecord* program,
                                                                       const SLC2TargetRecord*  target,
                                                                       const SLC2VariantRecord* variant)
    {
        SLC2ShaderProgramVulkan* shaderProgram = New<SLC2ShaderProgramVulkan>(m_Device);
        if (!shaderProgram->Initialize(program, target, variant))
        {
            Delete(shaderProgram);
            return nullptr;
        }
        return shaderProgram;
    }


	namespace
    {
        uint32 GetLayoutDescriptorCount(const ShaderIRDescriptorRange& range)
        {
            return range.ArrayElementBase + (range.DescriptorCount - 1) * range.LogicalElementStride + 1;
        }

        bool HasPhysicalDescriptorOwner(const List<SLC2VulkanDescriptorBinding>& bindings,
                                        const uint32                             set,
                                        const uint32                             bindingIndex,
                                        const uint32                             arrayElement)
        {
            for (int32 index = 0; index < bindings.Count(); index++)
            {
                const SLC2VulkanDescriptorBinding& binding = bindings[index];
                if (binding.Set == set && binding.Binding == bindingIndex && binding.ArrayElement == arrayElement)
                {
                    return true;
                }
            }
            return false;
        }

        void SortSetBindings(DescriptorSetLayoutInfoVulkan::SetLayout& setLayout)
        {
            for (int32 index = 1; index < setLayout.LayoutBindings.Count(); index++)
            {
                VkDescriptorSetLayoutBinding value       = setLayout.LayoutBindings[index];
                int32                        insertIndex = index - 1;
                while (insertIndex >= 0 && setLayout.LayoutBindings[insertIndex].binding > value.binding)
                {
                    setLayout.LayoutBindings[insertIndex + 1] = setLayout.LayoutBindings[insertIndex];
                    insertIndex--;
                }
                setLayout.LayoutBindings[insertIndex + 1] = value;
            }
        }

        void FinalizeLayoutHash(DescriptorSetLayoutInfoVulkan& output)
        {
            output.hash = 0;
            for (int32 setIndex = 0; setIndex < output.setLayouts.Count(); setIndex++)
            {
                SortSetBindings(output.setLayouts[setIndex]);
                const List<VkDescriptorSetLayoutBinding>& bindings = output.setLayouts[setIndex].LayoutBindings;
                for (int32 bindingIndex = 0; bindingIndex < bindings.Count(); bindingIndex++)
                {
                    output.hash =
                        HashCombine(output.hash, &bindings[bindingIndex], sizeof(VkDescriptorSetLayoutBinding));
                }
            }
#if VULKAN_HASH_POOLS_WITH_LAYOUT_TYPES
            output.setLayoutsHash = GetHash(output.layoutTypes, sizeof(output.layoutTypes));
#endif
        }

        void ResolveDescriptorWriteIndices(const DescriptorSetLayoutInfoVulkan& layout,
                                           List<SLC2VulkanDescriptorBinding>&   bindings)
        {
            uint32 descriptorWriteIndex = 0;
            for (int32 setIndex = 0; setIndex < layout.setLayouts.Count(); setIndex++)
            {
                const List<VkDescriptorSetLayoutBinding>& layoutBindings = layout.setLayouts[setIndex].LayoutBindings;
                for (int32 layoutBindingIndex = 0; layoutBindingIndex < layoutBindings.Count(); layoutBindingIndex++)
                {
                    const VkDescriptorSetLayoutBinding& layoutBinding = layoutBindings[layoutBindingIndex];
                    for (int32 bindingIndex = 0; bindingIndex < bindings.Count(); bindingIndex++)
                    {
                        SLC2VulkanDescriptorBinding& binding = bindings[bindingIndex];
                        if (binding.Set == static_cast<uint32>(setIndex) && binding.Binding == layoutBinding.binding)
                        {
                            binding.DescriptorWriteIndex = descriptorWriteIndex;
                        }
                    }
                    descriptorWriteIndex++;
                }
            }
        }

        bool AddOrMergeLayoutBinding(const ShaderIRDescriptorRange& range, DescriptorSetLayoutInfoVulkan& output)
        {
            if (range.Set >= DescriptorSet::Max)
            {
                LOG_ERROR("Graphic", "Descriptor set exceeds current Vulkan descriptor set layout slots.");
                return false;
            }
            if (range.DescriptorCount == 0 || range.LogicalElementStride == 0)
            {
                LOG_ERROR("Graphic", "Descriptor range uses an invalid descriptor count or logical stride.");
                return false;
            }

            VkDescriptorType   descriptorType = VK_DESCRIPTOR_TYPE_MAX_ENUM;
            VkShaderStageFlags stageFlags     = 0;
            if (!VulkanTool::ToVkDescriptorType(range.DescriptorType, descriptorType) ||
                !VulkanTool::ToVkStageFlags(range.StageMask, stageFlags))
            {
                return false;
            }

            if (range.Set >= static_cast<uint32>(output.setLayouts.Count()))
            {
                output.setLayouts.Resize(range.Set + 1);
            }

            const uint32                              descriptorCount = GetLayoutDescriptorCount(range);
            DescriptorSetLayoutInfoVulkan::SetLayout& setLayout       = output.setLayouts[range.Set];
            for (int32 index = 0; index < setLayout.LayoutBindings.Count(); index++)
            {
                VkDescriptorSetLayoutBinding& binding = setLayout.LayoutBindings[index];
                if (binding.binding != range.Binding)
                {
                    continue;
                }
                if (binding.descriptorType != descriptorType || binding.descriptorCount != descriptorCount ||
                    binding.pImmutableSamplers != nullptr)
                {
                    LOG_ERROR("Graphic", "Descriptor ranges use incompatible Vulkan layout at the same set/binding.");
                    return false;
                }
                binding.stageFlags |= stageFlags;
                return true;
            }

            VkDescriptorSetLayoutBinding binding;
            Platform::MemoryClear(&binding, sizeof(binding));
            binding.binding            = range.Binding;
            binding.descriptorType     = descriptorType;
            binding.descriptorCount    = descriptorCount;
            binding.stageFlags         = stageFlags;
            binding.pImmutableSamplers = nullptr;
            setLayout.LayoutBindings.Add(binding);
            output.layoutTypes[descriptorType] += descriptorCount;
            return true;
        }

        bool AddDescriptorBindingMap(const ShaderIRDescriptorRange&     range,
                                     const uint32                       blockId,
                                     const uint32                       rangeIndex,
                                     const bool                         isUniformData,
                                     List<SLC2VulkanDescriptorBinding>& bindings)
        {
            if (range.DescriptorCount == 0 || range.LogicalElementStride == 0)
            {
                LOG_ERROR("Graphic", "Descriptor binding map uses an invalid descriptor count or logical stride.");
                return false;
            }
            VkDescriptorType descriptorType = VK_DESCRIPTOR_TYPE_MAX_ENUM;
            if (!VulkanTool::ToVkDescriptorType(range.DescriptorType, descriptorType))
            {
                return false;
            }
            for (uint32 resourceIndex = 0; resourceIndex < range.DescriptorCount; resourceIndex++)
            {
                const uint32 arrayElement = range.ArrayElementBase + resourceIndex * range.LogicalElementStride;
                if (HasPhysicalDescriptorOwner(bindings, range.Set, range.Binding, arrayElement))
                {
                    LOG_ERROR("Graphic", "Multiple SLC2 runtime resources map to the same Vulkan descriptor element.");
                    return false;
                }
                // 保留 block/range/resource 到 set/binding/arrayElement 的映射，后续写 descriptor 时不再回查反射树。
                SLC2VulkanDescriptorBinding binding;
                binding.BlockId        = blockId;
                binding.RangeIndex     = rangeIndex;
                binding.ResourceIndex  = resourceIndex;
                binding.IsUniformData  = isUniformData;
                binding.Set            = range.Set;
                binding.Binding        = range.Binding;
                binding.ArrayElement   = arrayElement;
                binding.DescriptorType = descriptorType;
                bindings.Add(binding);
            }
            return true;
        }

        bool BuildBlockLayout(const ShaderReflectionIR&          layout,
                              const uint32                       blockId,
                              DescriptorSetLayoutInfoVulkan&     output,
                              List<SLC2VulkanDescriptorBinding>& bindings)
        {
            if (blockId >= static_cast<uint32>(layout.ParameterBlocks.Count()))
            {
                LOG_ERROR("Graphic", "Parameter block id is invalid for Vulkan runtime layout.");
                return false;
            }

            const ShaderIRParameterBlock& block = layout.ParameterBlocks[blockId];
            if (block.HasDefaultUniformBuffer)
            {
                // 普通 uniform 通过编译端生成的 defaultUniformBuffer 进入 Vulkan descriptor layout。
                if (!AddOrMergeLayoutBinding(block.DefaultUniformBuffer, output) ||
                    !AddDescriptorBindingMap(block.DefaultUniformBuffer, block.Id, Max_uint32, true, bindings))
                {
                    return false;
                }
            }

            for (int32 rangeIndex = 0; rangeIndex < block.RangeBindings.Count(); rangeIndex++)
            {
                const ShaderIRRangeBinding& rangeBinding = block.RangeBindings[rangeIndex];
                if (rangeBinding.SubBlockId >= 0)
                {
                    if (!BuildBlockLayout(layout, static_cast<uint32>(rangeBinding.SubBlockId), output, bindings))
                    {
                        return false;
                    }
                    continue;
                }
                if (rangeBinding.Flavor != SE_TEXT("simple"))
                {
                    LOG_ERROR("Graphic", "Range binding is not a simple Vulkan descriptor mapping.");
                    return false;
                }
                for (int32 descriptorIndex = 0; descriptorIndex < rangeBinding.DescriptorRanges.Count();
                     descriptorIndex++)
                {
                    const ShaderIRDescriptorRange& descriptor = rangeBinding.DescriptorRanges[descriptorIndex];
                    if (!AddOrMergeLayoutBinding(descriptor, output) ||
                        !AddDescriptorBindingMap(descriptor, block.Id, rangeBinding.RangeIndex, false, bindings))
                    {
                        return false;
                    }
                }
            }
            return true;
        }

        bool BuildSLC2DescriptorSetLayoutInfo(const ShaderReflectionIR&          layout,
                                              DescriptorSetLayoutInfoVulkan&     output,
                                              List<SLC2VulkanDescriptorBinding>& bindings)
        {
            output.hash           = 0;
            output.setLayoutsHash = 0;
            Platform::MemoryClear(output.layoutTypes, sizeof(output.layoutTypes));
            output.setLayouts.Clear();
            bindings.Clear();

            if (!BuildBlockLayout(layout, layout.RootBlockId, output, bindings))
            {
                return false;
            }
            FinalizeLayoutHash(output);
            ResolveDescriptorWriteIndices(output, bindings);
            return true;
        }
    } // namespace


    SLC2ShaderProgramVulkan::SLC2ShaderProgramVulkan(GPUDeviceVulkan* device) : m_Device(device)
    {}

    SLC2ShaderProgramVulkan::~SLC2ShaderProgramVulkan()
    {
        Delete(m_ComputeState);
        m_ComputeState = nullptr;
        for (int32 index = 0; index < m_StageModules.Count(); index++)
        {
            if (m_StageModules[index].Module != VK_NULL_HANDLE)
            {
                m_Device->deferredDeletionQueue.EnqueueResource(DeferredDeletionQueueVulkan::Type::ShaderModule,
                                                                m_StageModules[index].Module);
                m_StageModules[index].Module = VK_NULL_HANDLE;
            }
        }
        m_PipelineLayout = nullptr;
        m_DescriptorBindings.Clear();
    }

    bool SLC2ShaderProgramVulkan::Initialize(const SLC2ProgramRecord* program,
                                             const SLC2TargetRecord*  target,
                                             const SLC2VariantRecord* variant)
    {
        if (!SLC2GPUShaderProgram::Initialize(program, target, variant))
        {
            return false;
        }

        DescriptorSetLayoutInfoVulkan descriptorSetLayoutInfo;
        if (!BuildSLC2DescriptorSetLayoutInfo(GetReflection().GetLayout(), descriptorSetLayoutInfo, m_DescriptorBindings))
        {
            return false;
        }
        m_PipelineLayout = m_Device->GetOrCreateLayout(descriptorSetLayoutInfo);
        if (m_PipelineLayout == nullptr)
        {
            LOG_ERROR("Graphic", "Failed to create Vulkan pipeline layout.");
            return false;
        }

        m_StageModules.SetCapacity(variant->Stages.Count(), false);
        for (int32 stageIndex = 0; stageIndex < variant->Stages.Count(); stageIndex++)
        {
            // 每个 Stage 只创建 shader module；pipeline 按具体用途延迟创建，避免加载 shader 时立即铺开全部状态。
            const SLC2StageRecord&   stage = variant->Stages[stageIndex];
            VkShaderModuleCreateInfo createInfo;
            VulkanTool::ZeroStruct(createInfo, VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO);
            createInfo.codeSize = static_cast<size_t>(stage.Code.Count());
            createInfo.pCode    = reinterpret_cast<const uint32_t*>(stage.Code.Get());

            SLC2VulkanStageModule module;
            module.Stage               = stage.Stage;
            module.EntryPointAnsi      = stage.EntryPoint.ToStringAnsi();
            module.OutputControlPoints = stage.OutputControlPoints;
            const VkResult result = vkCreateShaderModule(m_Device->device, &createInfo, nullptr, &module.Module);
            LOG_VULKAN_RESULT(result);
            if (result != VK_SUCCESS)
            {
                LOG_ERROR("Graphic", "Failed to create Vulkan shader module for stage {0}.", ToString(stage.Stage));
                return false;
            }
            m_StageModules.Add(MoveTemp(module));
        }
        return true;
    }

    PipelineLayoutVulkan* SLC2ShaderProgramVulkan::GetPipelineLayout() const { return m_PipelineLayout; }

    const DescriptorSetLayoutVulkan* SLC2ShaderProgramVulkan::GetDescriptorSetLayout() const
    {
        return m_PipelineLayout != nullptr ? &m_PipelineLayout->descriptorSetLayout : nullptr;
    }

    const List<SLC2VulkanDescriptorBinding>& SLC2ShaderProgramVulkan::GetDescriptorBindings() const
    {
        return m_DescriptorBindings;
    }

    VkShaderModule SLC2ShaderProgramVulkan::GetShaderModule(const ShaderStage stage) const
    {
        for (int32 index = 0; index < m_StageModules.Count(); index++)
        {
            if (m_StageModules[index].Stage == stage)
            {
                return m_StageModules[index].Module;
            }
        }
        return VK_NULL_HANDLE;
    }

    const char* SLC2ShaderProgramVulkan::GetEntryPoint(const ShaderStage stage) const
    {
        for (int32 index = 0; index < m_StageModules.Count(); index++)
        {
            if (m_StageModules[index].Stage == stage)
            {
                return m_StageModules[index].EntryPointAnsi.Get();
            }
        }
        return nullptr;
    }

    SLC2ComputePipelineStateVulkan* SLC2ShaderProgramVulkan::GetOrCreateComputeState()
    {
        if (m_ComputeState != nullptr)
        {
            return m_ComputeState;
        }

        const SLC2VariantRecord* variant = GetVariant();
        if (variant == nullptr)
        {
            LOG_ERROR("Graphic", "compute pipeline requires a selected variant.");
            return nullptr;
        }
        if (variant->Stages.Count() != 1 || variant->Stages[0].Stage != ShaderStage::Compute)
        {
            LOG_ERROR("Graphic", "compute pipeline requires a variant with exactly one compute stage.");
            return nullptr;
        }

        const VkShaderModule shaderModule = GetShaderModule(ShaderStage::Compute);
        const char*          entryPoint   = GetEntryPoint(ShaderStage::Compute);
        if (shaderModule == VK_NULL_HANDLE || entryPoint == nullptr)
        {
            LOG_ERROR("Graphic", "compute shader module is missing.");
            return nullptr;
        }
        if (m_PipelineLayout == nullptr)
        {
            LOG_ERROR("Graphic", "compute pipeline layout is missing.");
            return nullptr;
        }

        VkComputePipelineCreateInfo desc;
        VulkanTool::ZeroStruct(desc, VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO);
        desc.basePipelineIndex = -1;
        desc.layout            = m_PipelineLayout->handle;
        VulkanTool::ZeroStruct(desc.stage, VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO);
        desc.stage.stage  = VK_SHADER_STAGE_COMPUTE_BIT;
        desc.stage.module = shaderModule;
        desc.stage.pName  = entryPoint;

        VkPipeline     pipeline = VK_NULL_HANDLE;
        const VkResult result =
            vkCreateComputePipelines(m_Device->device, m_Device->pipelineCache, 1, &desc, nullptr, &pipeline);
        LOG_VULKAN_RESULT(result);
        if (result != VK_SUCCESS)
        {
            LOG_ERROR("Graphic", "Failed to create SLC2 Vulkan compute pipeline.");
            return nullptr;
        }

        m_ComputeState = New<SLC2ComputePipelineStateVulkan>(m_Device, pipeline, m_PipelineLayout);
        return m_ComputeState;
    }

    bool SLC2ShaderProgramVulkan::PrepareAndBindComputeDescriptors(GPUContextVulkan*            context,
                                                                   const ShaderBindingSnapshot& snapshot)
    {
        SLC2ComputePipelineStateVulkan* computeState = GetOrCreateComputeState();
        if (computeState == nullptr)
        {
            return false;
        }
        return computeState->PrepareAndBindDescriptors(context, snapshot, m_DescriptorBindings);
    }

    bool SLC2ShaderProgramVulkan::PrepareAndBindGraphicsDescriptors(GPUContextVulkan*                context,
                                                                    const ShaderBindingSnapshot&     snapshot,
                                                                    SLC2GraphicsPipelineStateVulkan* state)
    {
        if (state == nullptr)
        {
            LOG_ERROR("Graphic", "SLC2 graphics descriptor binding requires a graphics pipeline state.");
            return false;
        }
        return state->PrepareAndBindDescriptors(context, snapshot, m_DescriptorBindings);
    }

} // SE
