#pragma once

#include "Core/Types/Variable.h"
#include "Core/Types/BitFlags.h"
#include "VulkanInclude.h"

#include "Runtime/Graphics/Base/GPUEnums.h"
#include "Runtime/Graphics/Textures/GPUSamplerDescription.h"

namespace SE
{
	// Vulkan results validation
	#define VALIDATE_VULKAN_RESULT(x) { VkResult result = x; if (result != VK_SUCCESS) VulkanTool::ValidateVkResult(result, __FILE__, __LINE__); }
	#define LOG_VULKAN_RESULT(result) if (result != VK_SUCCESS) VulkanTool::LogVkResult(result, __FILE__, __LINE__)
	#define LOG_VULKAN_RESULT_WITH_RETURN(result) if (result != VK_SUCCESS) { VulkanTool::LogVkResult(result, __FILE__, __LINE__); return true; }
	#define VK_SET_DEBUG_NAME(deviceValue, handle, type, name) VulkanTool::SetObjectName(deviceValue->device, (uint64)handle, type, name)


	class VulkanTool
    {
	private:
		static VkFormat m_PixelFormatToVkFormat[static_cast<int32>(PixelFormat::Max)];
		static VkBlendFactor m_BlendToVkBlendFactor[static_cast<int32>(BlendingMode::Blend::END)];
		static VkBlendOp m_OperationToVkBlendOp[static_cast<int32>(BlendingMode::Operation::END)];
		static VkCompareOp m_ComparisonFuncToVkCompareOp[static_cast<int32>(ComparisonFunc::END)];

	public:
		static VkFormat ToVulkanFormat(PixelFormat value)
		{
			return m_PixelFormatToVkFormat[(int32)value];
		}

		static String GetVkErrorString(VkResult result);

		static void ValidateVkResult(VkResult result, const char* file, uint32 line);
		static void LogVkResult(VkResult result, const char* file, uint32 line);
		static void LogVkResult(VkResult result);
		static void SetObjectName(VkDevice device, uint64 objectHandle, VkObjectType objectType, StringView name);
		static void SetObjectName(VkDevice device, uint64 objectHandle, VkObjectType objectType, const char* name);

		static inline VkPipelineStageFlags GetBufferBarrierFlags(VkAccessFlags accessFlags)
		{
			VkPipelineStageFlags stageFlags = (VkPipelineStageFlags)0;
			switch (accessFlags)
			{
			case 0:
				stageFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
				break;
			case VK_ACCESS_INDIRECT_COMMAND_READ_BIT:
				stageFlags = VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
				break;
			case VK_ACCESS_TRANSFER_WRITE_BIT:
				stageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;
				break;
			case VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT:
				stageFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
				break;
			case VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT:
				stageFlags = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
				break;
			case VK_ACCESS_TRANSFER_READ_BIT:
				stageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;
				break;
			case VK_ACCESS_SHADER_READ_BIT:
			case VK_ACCESS_SHADER_WRITE_BIT:
				stageFlags = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
				break;
			case VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT:
			case VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT:
				stageFlags = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
				break;
			default:
				ENGINE_UNREACHABLE_CODE();
				break;
			}
			return stageFlags;
		}

		static inline VkPipelineStageFlags GetImageBarrierFlags(VkImageLayout layout, VkAccessFlags& accessFlags)
		{
			VkPipelineStageFlags stageFlags;
			switch (layout)
			{
			case VK_IMAGE_LAYOUT_UNDEFINED:
				accessFlags = 0;
				stageFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
				break;
			case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
				accessFlags = VK_ACCESS_TRANSFER_WRITE_BIT;
				stageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;
				break;
			case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
				accessFlags = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				stageFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
				break;
			case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL:
			case VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL:
				accessFlags = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				stageFlags = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
				break;
			case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
				accessFlags = VK_ACCESS_TRANSFER_READ_BIT;
				stageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;
				break;
			case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
				accessFlags = 0;
				stageFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
				break;
			case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
				accessFlags = VK_ACCESS_SHADER_READ_BIT;
				stageFlags = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
				break;
			case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
			case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL:
			case VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL:
				accessFlags = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
				stageFlags = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
				break;
			case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL:
			case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL:
				accessFlags = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				stageFlags = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
				break;
			case VK_IMAGE_LAYOUT_GENERAL:
				accessFlags = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
				stageFlags = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
				break;
			default:
				ENGINE_UNREACHABLE_CODE();
				break;
			}
			return stageFlags;
		}

		static VkSamplerMipmapMode ToVulkanMipFilterMode(GPUSamplerFilter filter)
		{
			VkSamplerMipmapMode result;
			switch (filter)
			{
			case GPUSamplerFilter::Point:
				result = VK_SAMPLER_MIPMAP_MODE_NEAREST;
				break;
			case GPUSamplerFilter::Bilinear:
				result = VK_SAMPLER_MIPMAP_MODE_NEAREST;
				break;
			case GPUSamplerFilter::Trilinear:
				result = VK_SAMPLER_MIPMAP_MODE_LINEAR;
				break;
			case GPUSamplerFilter::Anisotropic:
				result = VK_SAMPLER_MIPMAP_MODE_LINEAR;
				break;
			default:
				ENGINE_UNREACHABLE_CODE();
				break;
			}
			return result;
		}

		static VkFilter ToVulkanMagFilterMode(GPUSamplerFilter filter)
		{
			VkFilter result;
			switch (filter)
			{
			case GPUSamplerFilter::Point:
				result = VK_FILTER_NEAREST;
				break;
			case GPUSamplerFilter::Bilinear:
				result = VK_FILTER_LINEAR;
				break;
			case GPUSamplerFilter::Trilinear:
				result = VK_FILTER_LINEAR;
				break;
			case GPUSamplerFilter::Anisotropic:
				result = VK_FILTER_LINEAR;
				break;
			default:
				ENGINE_UNREACHABLE_CODE();
				break;
			}
			return result;
		}

		static VkFilter ToVulkanMinFilterMode(GPUSamplerFilter filter)
		{
			VkFilter result;
			switch (filter)
			{
			case GPUSamplerFilter::Point:
				result = VK_FILTER_NEAREST;
				break;
			case GPUSamplerFilter::Bilinear:
				result = VK_FILTER_LINEAR;
				break;
			case GPUSamplerFilter::Trilinear:
				result = VK_FILTER_LINEAR;
				break;
			case GPUSamplerFilter::Anisotropic:
				result = VK_FILTER_LINEAR;
				break;
			default:
				ENGINE_UNREACHABLE_CODE();
				break;
			}
			return result;
		}

		static VkSamplerAddressMode ToVulkanWrapMode(GPUSamplerAddressMode addressMode, const bool supportsMirrorClampToEdge)
		{
			VkSamplerAddressMode result;
			switch (addressMode)
			{
			case GPUSamplerAddressMode::Wrap:
				result = VK_SAMPLER_ADDRESS_MODE_REPEAT;
				break;
			case GPUSamplerAddressMode::Clamp:
				result = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
				break;
			case GPUSamplerAddressMode::Mirror:
				result = supportsMirrorClampToEdge ? VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE : VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
				break;
			case GPUSamplerAddressMode::Border:
				result = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
				break;
			default:
				ENGINE_UNREACHABLE_CODE();
				break;
			}
			return result;
		}

		static VkCompareOp ToVulkanSamplerCompareFunction(GPUSamplerCompareFunction samplerComparisonFunction)
		{
			VkCompareOp result;
			switch (samplerComparisonFunction)
			{
			case GPUSamplerCompareFunction::Less:
				result = VK_COMPARE_OP_LESS;
				break;
			case GPUSamplerCompareFunction::Never:
				result = VK_COMPARE_OP_NEVER;
				break;
			default:
				ENGINE_UNREACHABLE_CODE();
				break;
			}
			return result;
		}

		static bool HasExtension(const List<const char*, HeapAllocation>& extensions, const char* name);


		/// <summary>
		/// Converts Flax blend mode to the Vulkan blend factor.
		/// </summary>
		/// <param name="value">The Flax blend mode.</param>
		/// <returns>The Vulkan blend factor.</returns>
		static VkBlendFactor ToVulkanBlendFactor(const BlendingMode::Blend value)
		{
			return m_BlendToVkBlendFactor[(int32)value];
		}

		/// <summary>
		/// Converts Flax blend operation to the Vulkan blend operation.
		/// </summary>
		/// <param name="value">The Flax blend operation.</param>
		/// <returns>The Vulkan blend operation.</returns>
		static VkBlendOp ToVulkanBlendOp(const BlendingMode::Operation value)
		{
			return m_OperationToVkBlendOp[(int32)value];
		}

		/// <summary>
		/// Converts Flax comparison function to the Vulkan comparison operation.
		/// </summary>
		/// <param name="value">The Flax comparison function.</param>
		/// <returns>The Vulkan comparison operation.</returns>
		static VkCompareOp ToVulkanCompareOp(const ComparisonFunc value)
		{
			return m_ComparisonFuncToVkCompareOp[(int32)value];
		}

		static VkBlendFactor ConvertBlend(RHIBlendFactor value);

		static VkBlendOp ConvertBlendOp(RHIBlendOp value);

		static VkSamplerAddressMode ConvertTextureAddressMode(RHIAddressMode value, const VkPhysicalDeviceVulkan12Features &features_1_2);

		static VkBorderColor ConvertSamplerBorderColor(RHIBorderColor value);

		static VkStencilOp ConvertStencilOp(RHIStencilOp value);

		static VkImageLayout ConvertImageLayout(RHIResourceStateFlag value);

		static VkImageAspectFlags ConvertImageAspect(RHIImageAspect value);

		static VkPipelineStageFlags2 ConvertPipelineStage(EnumFlags<RHIResourceStateFlag> value);

		static VkAccessFlags2 ParseResourceState(EnumFlags<RHIResourceStateFlag> value);

		static VkComponentSwizzle ConvertComponentSwizzle(ComponentSwizzle value);


		template<class T>
		static inline void ZeroStruct(T& data, VkStructureType type)
		{
			static_assert(!TIsPointer<T>::Value, "Don't use a pointer.");
			static_assert(offsetof(T, sType) == 0, "Assumes type is the first member in the Vulkan type.");
			data.sType = type;
			Platform::MemoryClear((uint8*)&data + sizeof(VkStructureType), sizeof(T) - sizeof(VkStructureType));
		};
    };
}