#include "SLC2ShaderProgramVulkan.h"

#include "CmdBufferVulkan.h"
#include "GPUBufferVulkan.h"
#include "GPUContextVulkan.h"
#include "GPUDeviceVulkan.h"
#include "GPUSamplerVulkan.h"
#include "GPUShaderVulkan.h"
#include "GPUTextureVulkan.h"
#include "GPUPipelineStateVulkan.h"
#include "VulkanTool.h"

#include "Runtime/Graphics/Shaders/ShaderBindingSnapshot.h"
#include "Runtime/Core/Types/Hash.h"
#include "Runtime/Core/Types/Strings/String.h"

namespace SE
{
	namespace
	{
		uint32 GetLayoutDescriptorCount(const ShaderIRDescriptorRange& range)
		{
			return range.ArrayElementBase + (range.DescriptorCount - 1) * range.LogicalElementStride + 1;
		}

		bool HasPhysicalDescriptorOwner(const List<SLC2VulkanDescriptorBinding>& bindings, const uint32 set, const uint32 bindingIndex, const uint32 arrayElement)
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
				VkDescriptorSetLayoutBinding value = setLayout.LayoutBindings[index];
				int32 insertIndex = index - 1;
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
					output.hash = HashCombine(output.hash, &bindings[bindingIndex], sizeof(VkDescriptorSetLayoutBinding));
				}
			}
#if VULKAN_HASH_POOLS_WITH_LAYOUT_TYPES
			output.setLayoutsHash = GetHash(output.layoutTypes, sizeof(output.layoutTypes));
#endif
		}

		void ResolveDescriptorWriteIndices(const DescriptorSetLayoutInfoVulkan& layout, List<SLC2VulkanDescriptorBinding>& bindings)
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

			VkDescriptorType descriptorType = VK_DESCRIPTOR_TYPE_MAX_ENUM;
			VkShaderStageFlags stageFlags = 0;
            if (!VulkanTool::ToVkDescriptorType(range.DescriptorType, descriptorType) ||
                !VulkanTool::ToVkStageFlags(range.StageMask, stageFlags))
			{
				return false;
			}

			if (range.Set >= static_cast<uint32>(output.setLayouts.Count()))
			{
				output.setLayouts.Resize(range.Set + 1);
			}

			const uint32 descriptorCount = GetLayoutDescriptorCount(range);
			DescriptorSetLayoutInfoVulkan::SetLayout& setLayout = output.setLayouts[range.Set];
			for (int32 index = 0; index < setLayout.LayoutBindings.Count(); index++)
			{
				VkDescriptorSetLayoutBinding& binding = setLayout.LayoutBindings[index];
				if (binding.binding != range.Binding)
				{
					continue;
				}
				if (binding.descriptorType != descriptorType || binding.descriptorCount != descriptorCount || binding.pImmutableSamplers != nullptr)
				{
					LOG_ERROR("Graphic", "Descriptor ranges use incompatible Vulkan layout at the same set/binding.");
					return false;
				}
				binding.stageFlags |= stageFlags;
				return true;
			}

			VkDescriptorSetLayoutBinding binding;
			Platform::MemoryClear(&binding, sizeof(binding));
			binding.binding = range.Binding;
			binding.descriptorType = descriptorType;
			binding.descriptorCount = descriptorCount;
			binding.stageFlags = stageFlags;
			binding.pImmutableSamplers = nullptr;
			setLayout.LayoutBindings.Add(binding);
			output.layoutTypes[descriptorType] += descriptorCount;
			return true;
		}

		bool AddDescriptorBindingMap(
			const ShaderIRDescriptorRange& range,
			const uint32 blockId,
			const uint32 rangeIndex,
			const bool isUniformData,
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
				binding.BlockId = blockId;
				binding.RangeIndex = rangeIndex;
				binding.ResourceIndex = resourceIndex;
				binding.IsUniformData = isUniformData;
				binding.Set = range.Set;
				binding.Binding = range.Binding;
				binding.ArrayElement = arrayElement;
				binding.DescriptorType = descriptorType;
				bindings.Add(binding);
			}
			return true;
		}

		bool BuildBlockLayout(
			const ShaderReflectionIR& layout,
			const uint32 blockId,
			DescriptorSetLayoutInfoVulkan& output,
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
				for (int32 descriptorIndex = 0; descriptorIndex < rangeBinding.DescriptorRanges.Count(); descriptorIndex++)
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

		bool BuildSLC2DescriptorSetLayoutInfo(
			const ShaderReflectionIR& layout,
			DescriptorSetLayoutInfoVulkan& output,
			List<SLC2VulkanDescriptorBinding>& bindings)
		{
			output.hash = 0;
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
	}

	

	SLC2ShaderProgramVulkan::SLC2ShaderProgramVulkan(GPUDeviceVulkan* device)
		: m_Device(device)
	{
	}

	SLC2ShaderProgramVulkan::~SLC2ShaderProgramVulkan()
	{
		Delete(m_ComputeState);
		m_ComputeState = nullptr;
		for (int32 index = 0; index < m_GraphicsStates.Count(); index++)
		{
			Delete(m_GraphicsStates[index]);
		}
		m_GraphicsStates.Clear();
		for (int32 index = 0; index < m_StageModules.Count(); index++)
		{
			if (m_StageModules[index].Module != VK_NULL_HANDLE)
			{
				m_Device->deferredDeletionQueue.EnqueueResource(DeferredDeletionQueueVulkan::Type::ShaderModule, m_StageModules[index].Module);
				m_StageModules[index].Module = VK_NULL_HANDLE;
			}
		}
		m_PipelineLayout = nullptr;
		m_DescriptorBindings.Clear();
	}

	bool SLC2ShaderProgramVulkan::Initialize(const SLC2ProgramRecord* program, const SLC2TargetRecord* target, const SLC2VariantRecord* variant)
	{
		if (!SLC2ShaderProgram::Initialize(program, target, variant))
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
			const SLC2StageRecord& stage = variant->Stages[stageIndex];
			VkShaderModuleCreateInfo createInfo;
			VulkanTool::ZeroStruct(createInfo, VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO);
			createInfo.codeSize = static_cast<size_t>(stage.Code.Count());
			createInfo.pCode = reinterpret_cast<const uint32_t*>(stage.Code.Get());

			SLC2VulkanStageModule module;
			module.Stage = stage.Stage;
			module.EntryPointAnsi = stage.EntryPoint.ToStringAnsi();
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

	PipelineLayoutVulkan* SLC2ShaderProgramVulkan::GetPipelineLayout() const
	{
		return m_PipelineLayout;
	}

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
		const char* entryPoint = GetEntryPoint(ShaderStage::Compute);
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
		desc.layout = m_PipelineLayout->handle;
		VulkanTool::ZeroStruct(desc.stage, VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO);
		desc.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		desc.stage.module = shaderModule;
		desc.stage.pName = entryPoint;

		VkPipeline pipeline = VK_NULL_HANDLE;
		const VkResult result = vkCreateComputePipelines(m_Device->device, m_Device->pipelineCache, 1, &desc, nullptr, &pipeline);
		LOG_VULKAN_RESULT(result);
		if (result != VK_SUCCESS)
		{
			LOG_ERROR("Graphic", "Failed to create SLC2 Vulkan compute pipeline.");
			return nullptr;
		}

		m_ComputeState = New<SLC2ComputePipelineStateVulkan>(m_Device, pipeline, m_PipelineLayout);
		return m_ComputeState;
	}

	SLC2GraphicsPipelineStateVulkan* SLC2ShaderProgramVulkan::GetOrCreateGraphicsState(const GPUPipelineState::Description& desc)
	{
		for (int32 index = 0; index < m_GraphicsStates.Count(); index++)
		{
			SLC2GraphicsPipelineStateVulkan* state = m_GraphicsStates[index];
			if (state != nullptr && state->Matches(desc))
			{
				return state;
			}
		}

		SLC2GraphicsPipelineStateVulkan* state = New<SLC2GraphicsPipelineStateVulkan>(m_Device, this);
		if (!state->Initialize(desc))
		{
			Delete(state);
			return nullptr;
		}
		m_GraphicsStates.Add(state);
		return state;
	}

	bool SLC2ShaderProgramVulkan::PrepareAndBindComputeDescriptors(GPUContextVulkan* context, const ShaderBindingSnapshot& snapshot)
	{
		SLC2ComputePipelineStateVulkan* computeState = GetOrCreateComputeState();
		if (computeState == nullptr)
		{
			return false;
		}
		return computeState->PrepareAndBindDescriptors(context, snapshot, m_DescriptorBindings);
	}

	bool SLC2ShaderProgramVulkan::PrepareAndBindGraphicsDescriptors(GPUContextVulkan* context, const ShaderBindingSnapshot& snapshot, SLC2GraphicsPipelineStateVulkan* state)
	{
		if (state == nullptr)
		{
			LOG_ERROR("Graphic", "SLC2 graphics descriptor binding requires a graphics pipeline state.");
			return false;
		}
		return state->PrepareAndBindDescriptors(context, snapshot, m_DescriptorBindings);
	}
}
