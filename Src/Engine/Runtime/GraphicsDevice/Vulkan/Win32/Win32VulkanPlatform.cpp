#include "Win32VulkanPlatform.h"

#if PLATFORM_WIN32

#include "../VulkanTool.h"
#include "Runtime/Graphics/GPUDevice.h"

namespace SE
{

	void Win32VulkanPlatform::GetInstanceExtensions(List<const char*>& extensions, List<const char*>& layers)
	{
		extensions.Add(VK_KHR_SURFACE_EXTENSION_NAME);
		extensions.Add(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
	}

	void Win32VulkanPlatform::CreateSurface(void* windowHandle, VkInstance instance, VkSurfaceKHR* surface)
	{
		VkWin32SurfaceCreateInfoKHR surfaceCreateInfo;
		VulkanTool::ZeroStruct(surfaceCreateInfo, VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR);
		surfaceCreateInfo.hinstance = GetModuleHandle(nullptr);
		surfaceCreateInfo.hwnd = static_cast<HWND>(windowHandle);
		VALIDATE_VULKAN_RESULT(vkCreateWin32SurfaceKHR(instance, &surfaceCreateInfo, nullptr, surface));
	}

}

#endif
