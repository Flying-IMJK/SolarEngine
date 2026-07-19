#include "GPUDeviceVulkan.h"
#include "Runtime/Core/Types/Collections/Sorting.h"
#include "VulkanPlatformBase.h"
#include "Win32/Win32VulkanPlatform.h"

namespace SE
{
	// TODO: expose it as a command line or engine parameter to end-user
	#define VULKAN_USE_KHRONOS_STANDARD_VALIDATION 1 // uses VK_LAYER_KHRONOS_validation
	#define VULKAN_USE_LUNARG_STANDARD_VALIDATION 1 // uses VK_LAYER_LUNARG_standard_validation

#if GPU_ENABLE_DIAGNOSTICS
	VulkanValidationLevel ValidationLevel = VulkanValidationLevel::ErrorsAndWarningsPerf;
#else
	VulkanValidationLevel ValidationLevel = VulkanValidationLevel::All;
#endif

	static const char* GInstanceExtensions[] =
	{
#if PLATFORM_APPLE_FAMILY && defined(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME)
		VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
		VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
#endif
#if VULKAN_USE_VALIDATION_CACHE
		VK_EXT_VALIDATION_CACHE_EXTENSION_NAME,
#endif
#if defined(VK_KHR_display) && 0
		VK_KHR_DISPLAY_EXTENSION_NAME,
#endif
		nullptr
	};

	static const char* GDeviceExtensions[] =
	{
#if PLATFORM_APPLE_FAMILY && defined(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME)
		VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME,
#endif
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
#if VK_KHR_maintenance1
		VK_KHR_MAINTENANCE1_EXTENSION_NAME,
#endif
#if VULKAN_USE_VALIDATION_CACHE
		VK_EXT_VALIDATION_CACHE_EXTENSION_NAME,
#endif
#if VK_KHR_sampler_mirror_clamp_to_edge
		VK_KHR_SAMPLER_MIRROR_CLAMP_TO_EDGE_EXTENSION_NAME,
#endif
		nullptr
	};

	static const char* GValidationLayers[] =
	{
		"VK_LAYER_GOOGLE_threading",
		"VK_LAYER_LUNARG_parameter_validation",
		"VK_LAYER_LUNARG_object_tracker",
		"VK_LAYER_LUNARG_core_validation",
		nullptr
	};

	struct LayerExtension
	{
		VkLayerProperties Layer;
		List<VkExtensionProperties> Extensions;

		LayerExtension()
		{
			Platform::MemoryClear(&Layer, sizeof(Layer));
		}

		void GetExtensions(List<String>& result)
		{
			for (auto& e : Extensions)
			{
				result.AddUnique(String(e.extensionName));
			}
		}

		void GetExtensions(List<const char*>& result)
		{
			for (auto& e : Extensions)
			{
				result.AddUnique(e.extensionName);
			}
		}
	};

	static void EnumerateInstanceExtensionProperties(const char* layerName, LayerExtension& outLayer)
	{
		VkResult result;
		do
		{
			uint32 count = 0;
			result = vkEnumerateInstanceExtensionProperties(layerName, &count, nullptr);
			ENGINE_ASSERT(result >= VK_SUCCESS);

			if (count > 0)
			{
				outLayer.Extensions.Clear();
				outLayer.Extensions.AddDefault(count);
				result = vkEnumerateInstanceExtensionProperties(layerName, &count, outLayer.Extensions.Get());
				ENGINE_ASSERT(result >= VK_SUCCESS);
			}
		} while (result == VK_INCOMPLETE);
	}

	static void EnumerateDeviceExtensionProperties(VkPhysicalDevice device, const char* layerName, LayerExtension& outLayer)
	{
		VkResult result;
		do
		{
			uint32 count = 0;
			result = vkEnumerateDeviceExtensionProperties(device, layerName, &count, nullptr);
			ENGINE_ASSERT(result >= VK_SUCCESS);

			if (count > 0)
			{
				outLayer.Extensions.Clear();
				outLayer.Extensions.AddDefault(count);
				result = vkEnumerateDeviceExtensionProperties(device, layerName, &count, outLayer.Extensions.Get());
				ENGINE_ASSERT(result >= VK_SUCCESS);
			}
		} while (result == VK_INCOMPLETE);
	}

	static void TrimDuplicates(List<const char*>& array)
	{
		for (int32 i = array.Count() - 1; i >= 0; i--)
		{
			bool found = false;
			for (int32 j = i - 1; j >= 0; j--)
			{
				if (!StringUtils::Compare(array[i], array[j]))
				{
					found = true;
					break;
				}
			}
			if (found)
			{
				array.RemoveAt(i);
			}
		}
	}

	static int FindLayerIndex(const List<LayerExtension>& list, const char* layerName)
	{
		for (int32 i = 1; i < list.Count(); i++)
		{
			if (!StringUtils::Compare(list[i].Layer.layerName, layerName))
			{
				return i;
			}
		}
		return INVALID_INDEX;
	}

	static bool ContainsLayer(const List<LayerExtension>& list, const char* layerName)
	{
		return FindLayerIndex(list, layerName) != INVALID_INDEX;
	}

	static bool FindLayerExtension(const List<LayerExtension>& list, const char* extensionName, const char*& foundLayer)
	{
		for (int32 extIndex = 0; extIndex < list.Count(); extIndex++)
		{
			for (int32 i = 0; i < list[extIndex].Extensions.Count(); i++)
			{
				if (!StringUtils::Compare(list[extIndex].Extensions[i].extensionName, extensionName))
				{
					foundLayer = list[extIndex].Layer.layerName;
					return true;
				}
			}
		}
		return false;
	}

	static bool FindLayerExtension(const List<LayerExtension>& list, const char* extensionName)
	{
		const char* dummy = nullptr;
		return FindLayerExtension(list, extensionName, dummy);
	}

	static bool ListContains(const List<const char*>& list, const char* name)
	{
		for (const char* element : list)
		{
			if (!StringUtils::Compare(element, name))
				return true;
		}
		return false;
	}


	void GPUDeviceVulkan::GetInstanceLayersAndExtensions(RHIValidationMode validationMode,
		List<const char*>& outInstanceExtensions,
		List<const char*>& outInstanceLayers,
		bool& outDebugUtils)
	{
		VkResult result;
		outDebugUtils = false;

		List<LayerExtension> globalLayerExtensions;
		globalLayerExtensions.AddDefault(1);
		EnumerateInstanceExtensionProperties(nullptr, globalLayerExtensions[0]);

		List<String> foundUniqueExtensions;
		List<String> foundUniqueLayers;
		for (int32 i = 0; i < globalLayerExtensions[0].Extensions.Count(); i++)
		{
			foundUniqueExtensions.AddUnique(String(globalLayerExtensions[0].Extensions[i].extensionName));
		}

		{
			List<VkLayerProperties> globalLayerProperties;
			do
			{
				uint32 instanceLayerCount = 0;
				result = vkEnumerateInstanceLayerProperties(&instanceLayerCount, nullptr);
				ENGINE_ASSERT(result >= VK_SUCCESS);

				if (instanceLayerCount > 0)
				{
					globalLayerProperties.AddZeroed(instanceLayerCount);
					result = vkEnumerateInstanceLayerProperties(&instanceLayerCount,
						&globalLayerProperties[globalLayerProperties.Count() - instanceLayerCount]);
					ENGINE_ASSERT(result >= VK_SUCCESS);
				}
			} while (result == VK_INCOMPLETE);

			globalLayerExtensions.AddDefault(globalLayerProperties.Count());
			for (int32 i = 0; i < globalLayerProperties.Count(); i++)
			{
				LayerExtension* layer = &globalLayerExtensions[i + 1];
				auto& prop = globalLayerProperties[i];
				layer->Layer = prop;
				EnumerateInstanceExtensionProperties(prop.layerName, *layer);
				layer->GetExtensions(foundUniqueExtensions);
				foundUniqueLayers.AddUnique(String(prop.layerName));
			}
		}

		if (foundUniqueLayers.HasItems())
		{
//			LOG_ERROR("Graphic", "GPUDeviceVulkan Found instance layers:");
			Sorting::QuickSort(foundUniqueLayers);
/*			for (const String& name : foundUniqueLayers)
			{
				LOG_INFO("Graphic", " - {0}", name);
			}*/
		}

		if (foundUniqueExtensions.HasItems())
		{
//			LOG_ERROR("Graphic", "GPUDeviceVulkan Found instance extensions:");
			Sorting::QuickSort(foundUniqueExtensions);
/*			for (const String& name : foundUniqueExtensions)
			{
				LOG_INFO("Graphic", " - {0}", name);
			}*/
		}

		// TODO: expose as a command line parameter or sth
		const bool useVkTrace = false;
		bool vkTrace = false;
		if (useVkTrace)
		{
			const char* VkTraceName = "VK_LAYER_LUNARG_vktrace";
			if (ContainsLayer(globalLayerExtensions, VkTraceName))
			{
				outInstanceLayers.Add(VkTraceName);
				vkTrace = true;
			}
		}

#if VULKAN_ENABLE_API_DUMP
		if (!vkTrace)
		{
			const char* VkApiDumpName = "VK_LAYER_LUNARG_api_dump";
			if (FindLayerInList(globalLayerExtensions, VkApiDumpName))
			{
				outInstanceLayers.Add(VkApiDumpName);
			}
			else
			{
				LOG(Warning, "Unable to find Vulkan instance layer {0}", String(VkApiDumpName));
			}
		}
#endif

		if (!vkTrace && validationMode != RHIValidationMode::Disabled)
		{
			bool hasKhronosStandardValidationLayer = false, hasLunargStandardValidationLayer = false;
#if VULKAN_USE_KHRONOS_STANDARD_VALIDATION
			const char* vkLayerKhronosValidation = "VK_LAYER_KHRONOS_validation";
			hasKhronosStandardValidationLayer = ContainsLayer(globalLayerExtensions, vkLayerKhronosValidation);
			if (hasKhronosStandardValidationLayer)
			{
				outInstanceLayers.Add(vkLayerKhronosValidation);
			}
			else
			{
				LOG_WARNING("Graphic",
					"GPUDeviceVulkan Unable to find Vulkan instance validation layer {0}",
					SE_TEXT("VK_LAYER_KHRONOS_validation"));
			}
#endif
#if VULKAN_USE_LUNARG_STANDARD_VALIDATION
			if (!hasKhronosStandardValidationLayer)
			{
				const char* vkLayerLunargStandardValidation = "VK_LAYER_LUNARG_standard_validation";
				hasLunargStandardValidationLayer =
					ContainsLayer(globalLayerExtensions, vkLayerLunargStandardValidation);
				if (hasLunargStandardValidationLayer)
				{
					outInstanceLayers.Add(vkLayerLunargStandardValidation);
				}
				else
				{
					LOG_WARNING("Graphic",
						"GPUDeviceVulkan Unable to find Vulkan instance validation layer {0}",
						SE_TEXT("VK_LAYER_LUNARG_standard_validation"));
				}
			}
#endif
			if (!hasKhronosStandardValidationLayer && !hasLunargStandardValidationLayer)
			{
				for (uint32 i = 0; GValidationLayers[i] != nullptr; i++)
				{
					const char* validationLayer = GValidationLayers[i];
					const bool validationFound = ContainsLayer(globalLayerExtensions, validationLayer);
					if (validationFound)
					{
						outInstanceLayers.Add(validationLayer);
					}
					else
					{
						LOG_WARNING("Graphic",
							"GPUDeviceVulkan Unable to find Vulkan instance validation layer {0}",
							StringAnsi(validationLayer));
					}
				}
			}
		}

#if VK_EXT_debug_utils
		if (!vkTrace && validationMode != RHIValidationMode::Disabled)
		{
			const char* foundDebugUtilsLayer = nullptr;
			outDebugUtils =
				FindLayerExtension(globalLayerExtensions, VK_EXT_DEBUG_UTILS_EXTENSION_NAME, foundDebugUtilsLayer);
			if (outDebugUtils && *foundDebugUtilsLayer)
			{
				outInstanceLayers.Add(foundDebugUtilsLayer);
			}
		}
#endif

		List<const char*> platformExtensions;
		VulkanPlatform::GetInstanceExtensions(platformExtensions, outInstanceLayers);

		for (const char* extension : platformExtensions)
		{
			if (FindLayerExtension(globalLayerExtensions, extension))
			{
				outInstanceExtensions.Add(extension);
			}
		}

		for (int32 i = 0; GInstanceExtensions[i] != nullptr; i++)
		{
			if (FindLayerExtension(globalLayerExtensions, GInstanceExtensions[i]))
			{
				outInstanceExtensions.Add(GInstanceExtensions[i]);
			}
		}

#if VK_EXT_debug_utils
		if (!vkTrace && FindLayerExtension(globalLayerExtensions, VK_EXT_DEBUG_UTILS_EXTENSION_NAME))
		{
			outInstanceExtensions.Add(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}
#endif
		if (!vkTrace && validationMode == RHIValidationMode::Disabled)
		{
			if (FindLayerExtension(globalLayerExtensions, VK_EXT_DEBUG_REPORT_EXTENSION_NAME))
			{
				outInstanceExtensions.Add(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
			}
		}

		TrimDuplicates(outInstanceLayers);
		if (outInstanceLayers.HasItems())
		{
			/*LOG_ERROR("Graphic", "GPUDeviceVulkan Using instance layers:");
			for (const char* layer : outInstanceLayers)
			{
				LOG_INFO("Graphic", "GPUDeviceVulkan - {0}", StringAnsiView(layer));
			}*/
		}
		else
		{
			LOG_ERROR("Graphic", "GPUDeviceVulkan Not using instance layers");
		}

		TrimDuplicates(outInstanceExtensions);
		if (outInstanceExtensions.HasItems())
		{
/*			LOG_ERROR("Graphic", "GPUDeviceVulkan Using instance extensions:");
			for (const char* extension : outInstanceExtensions)
			{
				LOG_INFO("Graphic", "GPUDeviceVulkan - {0}", StringAnsiView(extension));
			}*/
		}
		else
		{
			LOG_INFO("Graphic", "GPUDeviceVulkan Not using instance extensions");
		}
	}

	void GPUDeviceVulkan::GetDeviceExtensionsAndLayers(VkPhysicalDevice gpu, List<const char*>& outDeviceExtensions, List<const char*>& outDeviceLayers)
	{
		List<LayerExtension> deviceLayerExtensions;
		deviceLayerExtensions.AddDefault(1);
		{
			uint32 count = 0;
			List<VkLayerProperties> properties;
			VALIDATE_VULKAN_RESULT(vkEnumerateDeviceLayerProperties(gpu, &count, nullptr));
			properties.AddZeroed(count);
			VALIDATE_VULKAN_RESULT(vkEnumerateDeviceLayerProperties(gpu, &count, properties.Get()));
			ENGINE_ASSERT(count == properties.Count());
			for (const VkLayerProperties& property : properties)
			{
				deviceLayerExtensions.AddDefault(1);
				deviceLayerExtensions.Last().Layer = property;
			}
		}

		List<String> foundUniqueLayers;
		List<String> foundUniqueExtensions;

		for (int32 i = 0; i < deviceLayerExtensions.Count(); i++)
		{
			if (i == 0)
			{
				EnumerateDeviceExtensionProperties(gpu, nullptr, deviceLayerExtensions[i]);
			}
			else
			{
				foundUniqueLayers.AddUnique(String(deviceLayerExtensions[i].Layer.layerName));
				EnumerateDeviceExtensionProperties(gpu, deviceLayerExtensions[i].Layer.layerName, deviceLayerExtensions[i]);
			}

			deviceLayerExtensions[i].GetExtensions(foundUniqueExtensions);
		}

		if (foundUniqueLayers.HasItems())
		{
//			LOG_ERROR("Graphic", "GPUDeviceVulkan Found device layers:");
			Sorting::QuickSort(foundUniqueLayers);
/*			for (const String& name : foundUniqueLayers)
			{
				LOG_INFO("Graphic", " - {0}", name);
			}*/
		}

		if (foundUniqueExtensions.HasItems())
		{
//			LOG_ERROR("Graphic", "GPUDeviceVulkan Found device extensions:");
			Sorting::QuickSort(foundUniqueExtensions);
/*			for (const String& name : foundUniqueExtensions)
			{
				LOG_INFO("Graphic", " - {0}", name);
			}*/
		}

		// Add device layers for debugging

		bool hasKhronosStandardValidationLayer = false, hasLunargStandardValidationLayer = false;
#if VULKAN_USE_KHRONOS_STANDARD_VALIDATION
		const char* vkLayerKhronosValidation = "VK_LAYER_KHRONOS_validation";
		hasKhronosStandardValidationLayer = ContainsLayer(deviceLayerExtensions, vkLayerKhronosValidation);
		if (hasKhronosStandardValidationLayer)
		{
			outDeviceLayers.Add(vkLayerKhronosValidation);
		}
#endif
#if VULKAN_USE_LUNARG_STANDARD_VALIDATION
		if (!hasKhronosStandardValidationLayer)
		{
			const char* vkLayerLunargStandardValidation = "VK_LAYER_LUNARG_standard_validation";
			hasLunargStandardValidationLayer = ContainsLayer(deviceLayerExtensions, vkLayerLunargStandardValidation);
			if (hasLunargStandardValidationLayer)
			{
				outDeviceLayers.Add(vkLayerLunargStandardValidation);
			}
		}
#endif
		if (!hasKhronosStandardValidationLayer && !hasLunargStandardValidationLayer)
		{
			for (uint32 i = 0; GValidationLayers[i] != nullptr; i++)
			{
				const char* validationLayer = GValidationLayers[i];
				if (ContainsLayer(deviceLayerExtensions, validationLayer))
				{
					outDeviceLayers.Add(validationLayer);
				}
			}
		}


		// Find all extensions
		List<const char*> availableExtensions;
		{
			for (int32 i = 0; i < deviceLayerExtensions[0].Extensions.Count(); i++)
			{
				availableExtensions.Add(deviceLayerExtensions[0].Extensions[i].extensionName);
			}

			for (int32 layerIndex = 0; layerIndex < outDeviceLayers.Count(); layerIndex++)
			{
				int32 findLayerIndex;
				for (findLayerIndex = 1; findLayerIndex < deviceLayerExtensions.Count(); findLayerIndex++)
				{
					if (!StringUtils::Compare(deviceLayerExtensions[findLayerIndex].Layer.layerName, outDeviceLayers[layerIndex]))
					{
						break;
					}
				}

				if (findLayerIndex < deviceLayerExtensions.Count())
				{
					deviceLayerExtensions[findLayerIndex].GetExtensions(availableExtensions);
				}
			}
		}
		TrimDuplicates(availableExtensions);

		// Pick extensions to use
		List<const char*> platformExtensions;
		VulkanPlatform::GetDeviceExtensions(platformExtensions, outDeviceLayers);
		for (const char* extension : platformExtensions)
		{
			if (ListContains(availableExtensions, extension))
			{
				outDeviceExtensions.Add(extension);
				break;
			}
		}
		for (uint32 i = 0; i < ARRAY_SIZE(GDeviceExtensions) && GDeviceExtensions[i] != nullptr; i++)
		{
			if (ListContains(availableExtensions, GDeviceExtensions[i]))
			{
				outDeviceExtensions.Add(GDeviceExtensions[i]);
			}
		}

		if (outDeviceExtensions.HasItems())
		{
/*			LOG_ERROR("Graphic", "GPUDeviceVulkan Using device extensions:");
			for (const char* extension : outDeviceExtensions)
			{
				LOG_INFO("Graphic", "GPUDeviceVulkan - {0}", StringAnsiView(extension));
			}*/
		}

		if (outDeviceLayers.HasItems())
		{
/*			LOG_ERROR("Graphic", "GPUDeviceVulkan Using device layers:");
			for (const char* layer : outDeviceLayers)
			{
				LOG_INFO("Graphic", "GPUDeviceVulkan - {0}", StringAnsiView(layer));
			}*/
		}
	}


	void GPUDeviceVulkan::ParseOptionalDeviceExtensions(const List<const char*>& deviceExtensions)
	{
		Platform::MemoryClear(&optionalDeviceExtensions, sizeof(optionalDeviceExtensions));
#if VK_KHR_maintenance1
		optionalDeviceExtensions.HasKHRMaintenance1 = VulkanTool::HasExtension(deviceExtensions, VK_KHR_MAINTENANCE1_EXTENSION_NAME);
#endif
#if VK_KHR_maintenance2
		optionalDeviceExtensions.HasKHRMaintenance2 = VulkanTool::HasExtension(deviceExtensions, VK_KHR_MAINTENANCE2_EXTENSION_NAME);
#endif
		optionalDeviceExtensions.HasMirrorClampToEdge = VulkanTool::HasExtension(deviceExtensions, VK_KHR_SAMPLER_MIRROR_CLAMP_TO_EDGE_EXTENSION_NAME);
#if VULKAN_USE_VALIDATION_CACHE
		OptionalDeviceExtensions.HasEXTValidationCache = RenderToolsVulkan::HasExtension(deviceExtensions, VK_EXT_VALIDATION_CACHE_EXTENSION_NAME);
#endif
	}


}
