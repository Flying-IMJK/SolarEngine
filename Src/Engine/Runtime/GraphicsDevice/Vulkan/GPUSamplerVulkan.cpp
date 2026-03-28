
#include "GPUSamplerVulkan.h"
#include "VulkanTool.h"

namespace SE
{
	bool GPUSamplerVulkan::OnInit()
	{
		VkSamplerCreateInfo createInfo;
		VulkanTool::ZeroStruct(createInfo, VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO);
		createInfo.magFilter = VulkanTool::ToVulkanMagFilterMode(m_Desc.Filter);
		createInfo.minFilter = VulkanTool::ToVulkanMinFilterMode(m_Desc.Filter);
		createInfo.mipmapMode = VulkanTool::ToVulkanMipFilterMode(m_Desc.Filter);
		const bool supportsMirrorClampToEdge = false;// GPUDeviceVulkan::OptionalDeviceExtensions.HasMirrorClampToEdge;
		createInfo.addressModeU = VulkanTool::ToVulkanWrapMode(m_Desc.AddressU, supportsMirrorClampToEdge);
		createInfo.addressModeV = VulkanTool::ToVulkanWrapMode(m_Desc.AddressV, supportsMirrorClampToEdge);
		createInfo.addressModeW = VulkanTool::ToVulkanWrapMode(m_Desc.AddressW, supportsMirrorClampToEdge);
		createInfo.mipLodBias = m_Desc.MipBias;
		createInfo.anisotropyEnable = m_Desc.Filter == GPUSamplerFilter::Anisotropic ? VK_TRUE : VK_FALSE;
		createInfo.maxAnisotropy = (float)m_Desc.MaxAnisotropy;
		createInfo.compareEnable = m_Desc.ComparisonFunction != GPUSamplerCompareFunction::Never ? VK_TRUE : VK_FALSE;
		createInfo.compareOp = VulkanTool::ToVulkanSamplerCompareFunction(m_Desc.ComparisonFunction);
		createInfo.minLod = m_Desc.MinMipLevel;
		createInfo.maxLod = m_Desc.MaxMipLevel;
		switch (m_Desc.BorderColor)
		{
		case GPUSamplerBorderColor::TransparentBlack:
			createInfo.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
			break;
		case GPUSamplerBorderColor::OpaqueBlack:
			createInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
			break;
		case GPUSamplerBorderColor::OpaqueWhite:
			createInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
			break;
		default:
			return false;
		}

		VALIDATE_VULKAN_RESULT(vkCreateSampler(m_Device->device, &createInfo, nullptr, &Sampler));
		return true;
	}
	void GPUSamplerVulkan::OnReleaseGPU()
	{
		if (Sampler != VK_NULL_HANDLE)
		{

			Sampler = VK_NULL_HANDLE;
		}

		GPUSampler::OnReleaseGPU();
	}
}