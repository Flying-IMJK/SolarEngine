#pragma once

#include "VulkanTool.h"
#include "VulkanInclude.h"
#include "Core/Types/Strings/StringConverter.h"

namespace SE
{
	VkFormat VulkanTool::m_PixelFormatToVkFormat[] =
	{
		VK_FORMAT_UNDEFINED,

		VK_FORMAT_R32G32B32A32_SFLOAT,
		VK_FORMAT_R32G32B32A32_UINT,
		VK_FORMAT_R32G32B32A32_SINT,

		VK_FORMAT_R32G32B32_SFLOAT,
		VK_FORMAT_R32G32B32_UINT,
		VK_FORMAT_R32G32B32_SINT,

		VK_FORMAT_R16G16B16A16_SFLOAT,
		VK_FORMAT_R16G16B16A16_UNORM,
		VK_FORMAT_R16G16B16A16_UINT,
		VK_FORMAT_R16G16B16A16_SNORM,
		VK_FORMAT_R16G16B16A16_SINT,

		VK_FORMAT_R32G32_SFLOAT,
		VK_FORMAT_R32G32_UINT,
		VK_FORMAT_R32G32_SINT,

		VK_FORMAT_D32_SFLOAT_S8_UINT,

		VK_FORMAT_A2B10G10R10_UNORM_PACK32,
		VK_FORMAT_A2B10G10R10_UINT_PACK32,
		VK_FORMAT_B10G11R11_UFLOAT_PACK32,

		VK_FORMAT_R8G8B8A8_UNORM,
		VK_FORMAT_R8G8B8A8_SRGB,
		VK_FORMAT_R8G8B8A8_UINT,
		VK_FORMAT_R8G8B8A8_SNORM,
		VK_FORMAT_R8G8B8A8_SINT,
		VK_FORMAT_B8G8R8A8_UNORM,
		VK_FORMAT_B8G8R8A8_SRGB,
		VK_FORMAT_R16G16_SFLOAT,
		VK_FORMAT_R16G16_UNORM,
		VK_FORMAT_R16G16_UINT,
		VK_FORMAT_R16G16_SNORM,
		VK_FORMAT_R16G16_SINT,
		VK_FORMAT_D32_SFLOAT,
		VK_FORMAT_R32_SFLOAT,
		VK_FORMAT_R32_UINT,
		VK_FORMAT_R32_SINT,
		VK_FORMAT_D24_UNORM_S8_UINT,
		VK_FORMAT_E5B9G9R9_UFLOAT_PACK32,
		VK_FORMAT_R8G8_UNORM,
		VK_FORMAT_R8G8_UINT,
		VK_FORMAT_R8G8_SNORM,
		VK_FORMAT_R8G8_SINT,
		VK_FORMAT_R16_SFLOAT,
		VK_FORMAT_D16_UNORM,
		VK_FORMAT_R16_UNORM,
		VK_FORMAT_R16_UINT,
		VK_FORMAT_R16_SNORM,
		VK_FORMAT_R16_SINT,
		VK_FORMAT_R8_UNORM,
		VK_FORMAT_R8_UINT,
		VK_FORMAT_R8_SNORM,
		VK_FORMAT_R8_SINT,
		VK_FORMAT_BC1_RGBA_UNORM_BLOCK,
		VK_FORMAT_BC1_RGBA_SRGB_BLOCK,
		VK_FORMAT_BC2_UNORM_BLOCK,
		VK_FORMAT_BC2_SRGB_BLOCK,
		VK_FORMAT_BC3_UNORM_BLOCK,
		VK_FORMAT_BC3_SRGB_BLOCK,
		VK_FORMAT_BC4_UNORM_BLOCK,
		VK_FORMAT_BC4_SNORM_BLOCK,
		VK_FORMAT_BC5_UNORM_BLOCK,
		VK_FORMAT_BC5_SNORM_BLOCK,

		VK_FORMAT_BC6H_UFLOAT_BLOCK,
		VK_FORMAT_BC6H_SFLOAT_BLOCK,
		VK_FORMAT_BC7_UNORM_BLOCK,
		VK_FORMAT_BC7_SRGB_BLOCK
	};

	VkBlendFactor VulkanTool::m_BlendToVkBlendFactor[20] =
	{
		VK_BLEND_FACTOR_MAX_ENUM,
		VK_BLEND_FACTOR_ZERO, // Zero
		VK_BLEND_FACTOR_ONE, // One
		VK_BLEND_FACTOR_SRC_COLOR, // SrcColor
		VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR, // InvSrcColor
		VK_BLEND_FACTOR_SRC_ALPHA, // SrcAlpha
		VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, // InvSrcAlpha
		VK_BLEND_FACTOR_DST_ALPHA, // DestAlpha
		VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA, // InvDestAlpha
		VK_BLEND_FACTOR_DST_COLOR, // DestColor,
		VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR, // InvDestColor
		VK_BLEND_FACTOR_SRC_ALPHA_SATURATE, // SrcAlphaSat
		VK_BLEND_FACTOR_CONSTANT_ALPHA, // BlendFactor
		VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA, // BlendInvFactor
		VK_BLEND_FACTOR_SRC1_COLOR, // Src1Color
		VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR, // InvSrc1Color
		VK_BLEND_FACTOR_SRC1_ALPHA, // Src1Alpha
		VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA, // InvSrc1Alpha
	};

	VkBlendOp VulkanTool::m_OperationToVkBlendOp[6] =
	{
		VK_BLEND_OP_MAX_ENUM,
		VK_BLEND_OP_ADD, // Add
		VK_BLEND_OP_SUBTRACT, // Subtract
		VK_BLEND_OP_REVERSE_SUBTRACT, // RevSubtract
		VK_BLEND_OP_MIN, // Min
		VK_BLEND_OP_MAX, // Max
	};

	VkCompareOp VulkanTool::m_ComparisonFuncToVkCompareOp[9] =
	{
		VK_COMPARE_OP_MAX_ENUM,
		VK_COMPARE_OP_NEVER, // Never
		VK_COMPARE_OP_LESS, // Less
		VK_COMPARE_OP_EQUAL, // Equal
		VK_COMPARE_OP_LESS_OR_EQUAL, // LessEqual
		VK_COMPARE_OP_GREATER, // Grather
		VK_COMPARE_OP_NOT_EQUAL, // NotEqual
		VK_COMPARE_OP_GREATER_OR_EQUAL, // GratherEqual
		VK_COMPARE_OP_ALWAYS, // Always
	};


	String VulkanTool::GetVkErrorString(VkResult result)
	{
		StringBuilder sb(256);
		#define VKERR(x) case x: sb.Append(SE_TEXT(#x)); break
		// Switch error code
		switch (result)
		{
			VKERR(VK_SUCCESS);
			VKERR(VK_NOT_READY);
			VKERR(VK_TIMEOUT);
			VKERR(VK_EVENT_SET);
			VKERR(VK_EVENT_RESET);
			VKERR(VK_INCOMPLETE);
			VKERR(VK_ERROR_OUT_OF_HOST_MEMORY);
			VKERR(VK_ERROR_OUT_OF_DEVICE_MEMORY);
			VKERR(VK_ERROR_INITIALIZATION_FAILED);
			VKERR(VK_ERROR_DEVICE_LOST);
			VKERR(VK_ERROR_MEMORY_MAP_FAILED);
			VKERR(VK_ERROR_LAYER_NOT_PRESENT);
			VKERR(VK_ERROR_EXTENSION_NOT_PRESENT);
			VKERR(VK_ERROR_FEATURE_NOT_PRESENT);
			VKERR(VK_ERROR_INCOMPATIBLE_DRIVER);
			VKERR(VK_ERROR_TOO_MANY_OBJECTS);
			VKERR(VK_ERROR_FORMAT_NOT_SUPPORTED);
			VKERR(VK_ERROR_FRAGMENTED_POOL);
			VKERR(VK_ERROR_OUT_OF_POOL_MEMORY);
			VKERR(VK_ERROR_INVALID_EXTERNAL_HANDLE);
			VKERR(VK_ERROR_SURFACE_LOST_KHR);
			VKERR(VK_ERROR_NATIVE_WINDOW_IN_USE_KHR);
			VKERR(VK_SUBOPTIMAL_KHR);
			VKERR(VK_ERROR_OUT_OF_DATE_KHR);
			VKERR(VK_ERROR_INCOMPATIBLE_DISPLAY_KHR);
			VKERR(VK_ERROR_VALIDATION_FAILED_EXT);
			VKERR(VK_ERROR_INVALID_SHADER_NV);
#if VK_HEADER_VERSION >= 89
			VKERR(VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT);
#endif
			VKERR(VK_ERROR_FRAGMENTATION_EXT);
			VKERR(VK_ERROR_NOT_PERMITTED_EXT);
#if VK_HEADER_VERSION < 140
			VKERR(VK_RESULT_RANGE_SIZE);
#endif
		default:
			sb.AppendFormat(SE_TEXT("0x{0:x}"), static_cast<uint32>(result));
			break;
		}

		return sb.ToString();
	}

	void VulkanTool::ValidateVkResult(VkResult result, const char* file, uint32 line)
	{
		// Ensure result if invalid
		ENGINE_ASSERT(result != VK_SUCCESS);

		// Get error string
		const String& errorString = GetVkErrorString(result);

		// Send error
		Log::AddEntry(Log::Severity::Error, SE_TEXT("Graphic"),
			file, line, SE_TEXT("Vulkan error: {0}"), errorString);
	}

	void VulkanTool::LogVkResult(VkResult result, const char* file, uint32 line)
	{
		// Ensure result if invalid
		ENGINE_ASSERT(result != VK_SUCCESS);

		// Get error string
		const String& errorString = GetVkErrorString(result);

		Log::AddEntry(Log::Severity::Error, SE_TEXT("Graphic"),
			file, line, SE_TEXT("Vulkan error: {0}"), errorString);
	}

	void VulkanTool::LogVkResult(VkResult result)
	{
		LogVkResult(result, "", 0);
	}

	void VulkanTool::SetObjectName(VkDevice device, uint64 objectHandle, VkObjectType objectType, StringView name)
	{
#if VK_EXT_debug_utils
		StringAsUTF8 convert(name.Get(), name.Length());
		SetObjectName(device, objectHandle, objectType, convert.Get());
#endif
	}

	void VulkanTool::SetObjectName(VkDevice device, uint64 objectHandle, VkObjectType objectType, const char* name)
	{
#if VK_EXT_debug_utils
		// Check for valid function pointer (may not be present if not running in a debugging application)
		if (vkSetDebugUtilsObjectNameEXT != nullptr && name != nullptr && *name != 0)
		{
			VkDebugUtilsObjectNameInfoEXT objectNameInfo;
			ZeroStruct(objectNameInfo, VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT);
			objectNameInfo.objectType = objectType;
			objectNameInfo.objectHandle = objectHandle;
			objectNameInfo.pObjectName = name;
			const VkResult result = vkSetDebugUtilsObjectNameEXT(device, &objectNameInfo);
			LOG_VULKAN_RESULT(result);
		}
#endif
	}

	bool VulkanTool::HasExtension(const List<const char*>& extensions, const char* name)
	{
		for (int32 i = 0; i < extensions.Count(); i++)
		{
			const char* extension = extensions[i];
			if (extension && StringUtils::Compare(extension, name) == 0)
				return true;
		}
		return false;
	}

	VkBlendFactor VulkanTool::ConvertBlend(RHIBlendFactor value)
	{
		switch (value)
		{
		case RHIBlendFactor::ZERO:
			return VK_BLEND_FACTOR_ZERO;
		case RHIBlendFactor::ONE:
			return VK_BLEND_FACTOR_ONE;
		case RHIBlendFactor::SRC_COLOR:
			return VK_BLEND_FACTOR_SRC_COLOR;
		case RHIBlendFactor::INV_SRC_COLOR:
			return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
		case RHIBlendFactor::SRC_ALPHA:
			return VK_BLEND_FACTOR_SRC_ALPHA;
		case RHIBlendFactor::INV_SRC_ALPHA:
			return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		case RHIBlendFactor::DEST_ALPHA:
			return VK_BLEND_FACTOR_DST_ALPHA;
		case RHIBlendFactor::INV_DEST_ALPHA:
			return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
		case RHIBlendFactor::DEST_COLOR:
			return VK_BLEND_FACTOR_DST_COLOR;
		case RHIBlendFactor::INV_DEST_COLOR:
			return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
		case RHIBlendFactor::SRC_ALPHA_SAT:
			return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
		case RHIBlendFactor::BLEND_FACTOR:
			return VK_BLEND_FACTOR_CONSTANT_COLOR;
		case RHIBlendFactor::INV_BLEND_FACTOR:
			return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
		case RHIBlendFactor::SRC1_COLOR:
			return VK_BLEND_FACTOR_SRC1_COLOR;
		case RHIBlendFactor::INV_SRC1_COLOR:
			return VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR;
		case RHIBlendFactor::SRC1_ALPHA:
			return VK_BLEND_FACTOR_SRC1_ALPHA;
		case RHIBlendFactor::INV_SRC1_ALPHA:
			return VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA;
		default:
			return VK_BLEND_FACTOR_ZERO;
		}
	}

	VkBlendOp VulkanTool::ConvertBlendOp(RHIBlendOp value)
	{
		switch (value)
		{
		case RHIBlendOp::ADD:
			return VK_BLEND_OP_ADD;
		case RHIBlendOp::SUBTRACT:
			return VK_BLEND_OP_SUBTRACT;
		case RHIBlendOp::REVERSE_SUBTRACT:
			return VK_BLEND_OP_REVERSE_SUBTRACT;
		case RHIBlendOp::MIN:
			return VK_BLEND_OP_MIN;
		case RHIBlendOp::MAX:
			return VK_BLEND_OP_MAX;
		default:
			return VK_BLEND_OP_ADD;
		}
	}

	VkSamplerAddressMode VulkanTool::ConvertTextureAddressMode(RHIAddressMode value, const VkPhysicalDeviceVulkan12Features &features_1_2)
	{
		switch (value)
		{
		case RHIAddressMode::Repeat:
			return VK_SAMPLER_ADDRESS_MODE_REPEAT;
		case RHIAddressMode::MirroredRepeat:
			return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
		case RHIAddressMode::ClampToEdge:
			return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		case RHIAddressMode::ClampToBorder:
			return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		case RHIAddressMode::MirroreOnce:
			if (features_1_2.samplerMirrorClampToEdge == VK_TRUE)
			{
				return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
			}
			return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
		default:
			return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		}
	}

	VkBorderColor VulkanTool::ConvertSamplerBorderColor(RHIBorderColor value)
	{
		switch (value)
		{
		case RHIBorderColor::TransparentBlack:
			return VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
		case RHIBorderColor::OpaqueBlack:
			return VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
		case RHIBorderColor::OpaqueWhite:
			return VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		default:
			return VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
		}
	}

	VkStencilOp VulkanTool::ConvertStencilOp(RHIStencilOp value)
	{
		switch (value)
		{
		case RHIStencilOp::Keep:
			return VK_STENCIL_OP_KEEP;
		case RHIStencilOp::ZERO:
			return VK_STENCIL_OP_ZERO;
		case RHIStencilOp::Replace:
			return VK_STENCIL_OP_REPLACE;
		case RHIStencilOp::IncrementAndClamp:
			return VK_STENCIL_OP_INCREMENT_AND_CLAMP;
		case RHIStencilOp::DecrementAndClamp:
			return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
		case RHIStencilOp::Invert:
			return VK_STENCIL_OP_INVERT;
		case RHIStencilOp::IncrementAndWarp:
			return VK_STENCIL_OP_INCREMENT_AND_WRAP;
		case RHIStencilOp::DecrementAndWarp:
			return VK_STENCIL_OP_DECREMENT_AND_WRAP;
		default:
			return VK_STENCIL_OP_KEEP;
		}
	}

	VkImageLayout VulkanTool::ConvertImageLayout(RHIResourceStateFlag value)
	{
		switch (value)
		{
		case RHIResourceStateFlag::Undefined:
			return VK_IMAGE_LAYOUT_UNDEFINED;
		case RHIResourceStateFlag::Rendertarget:
			return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		case RHIResourceStateFlag::DepthStencil:
			return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		case RHIResourceStateFlag::DepthStencilReadonly:
			return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
		case RHIResourceStateFlag::ShaderResource:
		case RHIResourceStateFlag::ShaderResourceCompute:
			return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		case RHIResourceStateFlag::UnorderedAccess:
			return VK_IMAGE_LAYOUT_GENERAL;
		case RHIResourceStateFlag::CopySrc:
			return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		case RHIResourceStateFlag::CopyDst:
			return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		case RHIResourceStateFlag::ShadingRateSource:
			return VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR;
		case RHIResourceStateFlag::VideoDecodeSrc:
		case RHIResourceStateFlag::VideoDecodeDst:
			return VK_IMAGE_LAYOUT_VIDEO_DECODE_DPB_KHR;
		default:
			return VK_IMAGE_LAYOUT_UNDEFINED;
		}
	}


	VkImageAspectFlags VulkanTool::ConvertImageAspect(RHIImageAspect value)
	{
		switch (value)
		{
		default:
		case RHIImageAspect::Color:
			return VK_IMAGE_ASPECT_COLOR_BIT;
		case RHIImageAspect::Depth:
			return VK_IMAGE_ASPECT_DEPTH_BIT;
		case RHIImageAspect::Stencil:
			return VK_IMAGE_ASPECT_STENCIL_BIT;
		case RHIImageAspect::Luminance:
			return VK_IMAGE_ASPECT_PLANE_0_BIT;
		case RHIImageAspect::Chrominance:
			return VK_IMAGE_ASPECT_PLANE_1_BIT;
		}
	}

	VkPipelineStageFlags2 VulkanTool::ConvertPipelineStage(EnumFlags<RHIResourceStateFlag> value)
	{
		VkPipelineStageFlags2 flags = VK_PIPELINE_STAGE_2_NONE;

		if (value.IsFlag(RHIResourceStateFlag::ShaderResource) ||
			value.IsFlag(RHIResourceStateFlag::ShaderResourceCompute) ||
			value.IsFlag(RHIResourceStateFlag::UnorderedAccess) ||
			value.IsFlag(RHIResourceStateFlag::ConstantBuffer))
		{
			flags |= VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
		}
		if (value.IsFlag(RHIResourceStateFlag::CopySrc) ||
			value.IsFlag(RHIResourceStateFlag::CopyDst))
		{
			flags |= VK_PIPELINE_STAGE_2_TRANSFER_BIT;
		}
		if (value.IsFlag(RHIResourceStateFlag::Rendertarget))
		{
			flags |= VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
		}
		if (value.IsFlag(RHIResourceStateFlag::DepthStencil) ||
			value.IsFlag(RHIResourceStateFlag::DepthStencilReadonly))
		{
			flags |= VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
		}
		if (value.IsFlag(RHIResourceStateFlag::ShadingRateSource))
		{
			flags |= VK_PIPELINE_STAGE_2_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR;
		}
		if (value.IsFlag(RHIResourceStateFlag::VertexBuffer))
		{
			flags |= VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT;
		}
		if (value.IsFlag(RHIResourceStateFlag::IndexBuffer))
		{
			flags |= VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT;
		}
		if (value.IsFlag(RHIResourceStateFlag::IndirectArgument))
		{
			flags |= VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT;
		}
		if (value.IsFlag(RHIResourceStateFlag::RaytracingAccelerationStructure))
		{
			flags |= VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR | VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR;
		}
		if (value.IsFlag(RHIResourceStateFlag::Predication))
		{
			flags |= VK_PIPELINE_STAGE_2_CONDITIONAL_RENDERING_BIT_EXT;
		}
		if (value.IsFlag(RHIResourceStateFlag::VideoDecodeSrc) ||
			value.IsFlag(RHIResourceStateFlag::VideoDecodeDst))
		{
			flags |= VK_PIPELINE_STAGE_2_VIDEO_DECODE_BIT_KHR;
		}

		return flags;
	}

	VkAccessFlags2 VulkanTool::ParseResourceState(EnumFlags<RHIResourceStateFlag> value)
	{
		VkAccessFlags2 flags = 0;

		if (value.IsFlag(RHIResourceStateFlag::ShaderResource))
		{
			flags |= VK_ACCESS_2_SHADER_READ_BIT;
		}
		if (value.IsFlag(RHIResourceStateFlag::ShaderResourceCompute))
		{
			flags |= VK_ACCESS_2_SHADER_READ_BIT;
		}
		if (value.IsFlag(RHIResourceStateFlag::UnorderedAccess))
		{
			flags |= VK_ACCESS_2_SHADER_READ_BIT;
			flags |= VK_ACCESS_2_SHADER_WRITE_BIT;
		}
		if (value.IsFlag(RHIResourceStateFlag::CopySrc))
		{
			flags |= VK_ACCESS_2_TRANSFER_READ_BIT;
		}
		if (value.IsFlag(RHIResourceStateFlag::CopyDst))
		{
			flags |= VK_ACCESS_2_TRANSFER_WRITE_BIT;
		}
		if (value.IsFlag(RHIResourceStateFlag::Rendertarget))
		{
			flags |= VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT;
			flags |= VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
		}
		if (value.IsFlag(RHIResourceStateFlag::DepthStencil))
		{
			flags |= VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
			flags |= VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		}
		if (value.IsFlag(RHIResourceStateFlag::DepthStencilReadonly))
		{
			flags |= VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
		}
		if (value.IsFlag(RHIResourceStateFlag::VertexBuffer))
		{
			flags |= VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT;
		}
		if (value.IsFlag(RHIResourceStateFlag::IndexBuffer))
		{
			flags |= VK_ACCESS_2_INDEX_READ_BIT;
		}
		if (value.IsFlag(RHIResourceStateFlag::ConstantBuffer))
		{
			flags |= VK_ACCESS_2_UNIFORM_READ_BIT;
		}
		if (value.IsFlag(RHIResourceStateFlag::IndirectArgument))
		{
			flags |= VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT;
		}
		if (value.IsFlag(RHIResourceStateFlag::Predication))
		{
			flags |= VK_ACCESS_2_CONDITIONAL_RENDERING_READ_BIT_EXT;
		}
		if (value.IsFlag(RHIResourceStateFlag::ShadingRateSource))
		{
			flags |= VK_ACCESS_2_FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT_KHR;
		}
		if (value.IsFlag(RHIResourceStateFlag::VideoDecodeDst))
		{
			flags |= VK_ACCESS_2_VIDEO_DECODE_WRITE_BIT_KHR;
		}
		if (value.IsFlag(RHIResourceStateFlag::VideoDecodeSrc))
		{
			flags |= VK_ACCESS_2_VIDEO_DECODE_READ_BIT_KHR;
		}

		return flags;
	}

	VkComponentSwizzle VulkanTool::ConvertComponentSwizzle(ComponentSwizzle value)
	{
		switch (value)
		{
		default:
			return VK_COMPONENT_SWIZZLE_IDENTITY;
		case ComponentSwizzle::R:
			return VK_COMPONENT_SWIZZLE_R;
		case ComponentSwizzle::G:
			return VK_COMPONENT_SWIZZLE_G;
		case ComponentSwizzle::B:
			return VK_COMPONENT_SWIZZLE_B;
		case ComponentSwizzle::A:
			return VK_COMPONENT_SWIZZLE_A;
		case ComponentSwizzle::ZERO:
			return VK_COMPONENT_SWIZZLE_ZERO;
		case ComponentSwizzle::ONE:
			return VK_COMPONENT_SWIZZLE_ONE;
		}
	}
}