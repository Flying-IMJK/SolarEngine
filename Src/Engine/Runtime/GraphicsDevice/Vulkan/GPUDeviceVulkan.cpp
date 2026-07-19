#include "GPUDeviceVulkan.h"

#include "Runtime/Core/Types/Collections/HashSet.h"
#include "Runtime/Core/Profiler/Profiler.h"
#include "Runtime/Core/Types/Strings/StringConverter.h"

#include "Runtime/EngineContext.h"
#include "GPUSwapChainVulkan.h"
#include "GPUPipelineStateVulkan.h"
#include "GPUShaderVulkan.h"
#include "GPUBufferVulkan.h"
#include "GPUSamplerVulkan.h"
#include "GPUAdapterVulkan.h"
#include "QueueVulkan.h"
#include "GPUContextVulkan.h"
#include "VulkanPlatformBase.h"
#include "GPUTimerQueryVulkan.h"

#ifdef WIN32
#include "Win32/Win32VulkanPlatform.h"
#endif

namespace SE
{
	bool SupportsDebugUtilsExt = true;
	bool SupportsDebugCallbackExt = true;

	GPUDeviceVulkan::OptionalVulkanDeviceExtensions GPUDeviceVulkan::optionalDeviceExtensions;

	extern VulkanValidationLevel ValidationLevel;

	VKAPI_ATTR VkBool32 VKAPI_CALL DebugUtilsMessengerCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
		VkDebugUtilsMessageTypeFlagsEXT message_type,
		const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
		void *user_data)
	{
		if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
		{
			LOG_WARNING("Graphic", "Vulkan {0}", String(callback_data->pMessage));
		}
		else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
		{
			LOG_ERROR("Graphic", "Vulkan {0}", String(callback_data->pMessage));
		}
		else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT || message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
		{
			LOG_INFO("Graphic", "Vulkan {0}", String(callback_data->pMessage));
		}

		return VK_FALSE;
	}

#if VK_EXT_debug_utils
	VkDebugUtilsMessengerEXT Messenger = VK_NULL_HANDLE;
#endif
	VkDebugReportCallbackEXT MsgCallback = VK_NULL_HANDLE;

#if VK_EXT_debug_report
	static VKAPI_ATTR VkBool32 VKAPI_PTR DebugReportFunction(VkDebugReportFlagsEXT msgFlags, VkDebugReportObjectTypeEXT objType, uint64_t srcObject, size_t location, int32 msgCode, const char* layerPrefix, const char* msg, void* userData)
	{
		if (msgFlags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
		{
			if (!StringUtils::Compare(layerPrefix, "SC"))
			{
				if (msgCode == 3)
				{
					// Attachment N not written by fragment shader
					return VK_FALSE;
				}
				else if (msgCode == 5)
				{
					// SPIR-V module not valid: MemoryBarrier: Vulkan specification requires Memory Semantics to have one of the following bits set: Acquire, Release, AcquireRelease or SequentiallyConsistent
					return VK_FALSE;
				}
			}

			LOG_ERROR("Graphic", "Vulkan {0}:{1} {2}", StringAnsiView(layerPrefix), msgCode, StringAnsiView(msg));
		}
		else if (msgFlags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
		{
			if (!StringUtils::Compare(layerPrefix, "SC"))
			{
				if (msgCode == 2)
				{
					// Fragment shader writes to output location 0 with no matching attachment
					return VK_FALSE;
				}
			}

			LOG_WARNING("Graphic", "Vulkan {0}:{1} {2}", StringAnsiView(layerPrefix), msgCode, StringAnsiView(msg));
		}
		else if (msgFlags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
		{
			if (!StringUtils::Compare(layerPrefix, "SC"))
			{
				if (msgCode == 2)
				{
					// Vertex shader outputs unused interpolator
					return VK_FALSE;
				}
			}
			else if (!StringUtils::Compare(layerPrefix, "DS"))
			{
				if (msgCode == 15)
				{
					// DescriptorSet previously bound is incompatible with set newly bound as set #0 so set #1 and any subsequent sets were disturbed by newly bound pipelineLayout
					return VK_FALSE;
				}
			}

			LOG_WARNING("Graphic", "Vulkan {0}:{1} {2}", StringAnsiView(layerPrefix), msgCode, StringAnsiView(msg));
		}
		else if (msgFlags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT)
		{
			LOG_INFO("Graphic", "Vulkan {0}:{1} {2}", StringAnsiView(layerPrefix), msgCode, StringAnsiView(msg));
		}
		else if (msgFlags & VK_DEBUG_REPORT_DEBUG_BIT_EXT)
		{
			LOG_INFO("Graphic", "Vulkan {0}:{1} {2}", StringAnsiView(layerPrefix), msgCode, StringAnsiView(msg));
		}

		return VK_FALSE;
	}
	static VKAPI_ATTR VkBool32 VKAPI_PTR DebugUtilsCallback(VkDebugUtilsMessageSeverityFlagBitsEXT msgSeverity, VkDebugUtilsMessageTypeFlagsEXT msgType, const VkDebugUtilsMessengerCallbackDataEXT* callbackData, void* userData)
	{
		// Ignore some errors
		switch (msgType)
		{
		case 2:
			switch (callbackData->messageIdNumber)
			{
			case 0: // Attachment 4 not written by fragment shader; undefined values will be written to attachment
			case 2: // Fragment shader writes to output location 0 with no matching attachment
			case 3: // Attachment 2 not written by fragment shader
			case 5: // SPIR-V module not valid: MemoryBarrier: Vulkan specification requires Memory Semantics to have one of the following bits set: Acquire, Release, AcquireRelease or SequentiallyConsistent
			case -1666394502: // After query pool creation, each query must be reset before it is used. Queries must also be reset between uses.
			case 602160055: // Attachment 4 not written by fragment shader; undefined values will be written to attachment. TODO: investigate it for PS_GBuffer shader from Deferred material with USE_LIGHTMAP=1
			case 7060244: //  Image Operand Offset can only be used with OpImage*Gather operations
			case -1539028524: // SortedIndices is null so Vulkan backend sets it to default R32_SFLOAT format which is not good for UINT format of the buffer
			case -1810835948: // SortedIndices is null so Vulkan backend sets it to default R32_SFLOAT format which is not good for UINT format of the buffer
			case -1621360350: // VkFramebufferCreateInfo attachment #0 has a layer count (1) smaller than the corresponding framebuffer layer count (64). The Vulkan spec states: If flags does not include VK_FRAMEBUFFER_CREATE_IMAGELESS_BIT, each element of pAttachments that is used as an input, color, resolve, or depth/stencil attachment by renderPass must have been created with a VkImageViewCreateInfo::subresourceRange.layerCount greater than or equal to layers
				return VK_FALSE;
			}
			break;
		case 4:
			switch (callbackData->messageIdNumber)
			{
			case 0: // Vertex shader writes to output location 0.0 which is not consumed by fragment shader
			case 558591440: // preTransform doesn't match the currentTransform returned by vkGetPhysicalDeviceSurfaceCapabilitiesKHR, the presentation engine will transform the image content as part of the presentation operation. TODO: implement preTransform for Android to improve swapchain presentation performance
			case 101294395: // Vertex shader writes to output location 0.0 which is not consumed by fragment shader
				return VK_FALSE;
			}
			break;
		case 6:
			switch (callbackData->messageIdNumber)
			{
			case 2: // Vertex shader writes to output location 0.0 which is not consumed by fragment shader
				return VK_FALSE;
			}
			break;
		}

		enum class Severity
		{
			Error,
			Warning,
			Info
		};

		Severity severity = Severity::Info;
		if (msgSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
		{
			severity = Severity::Error;
		}
		else if (msgSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
		{
			severity = Severity::Warning;
		}
		else if (msgSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
		{
			severity = Severity::Info;
		}
		else if (msgSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
		{
			severity = Severity::Info;
		}

		const Char* type = SE_TEXT("");
		if (msgType & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)
		{
			if (msgType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
			{
				type = SE_TEXT("General/Validation");
			}
			else
			{
				type = SE_TEXT("General");
			}
		}
		else if (msgType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
		{
			if (msgType & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)
			{
				type = SE_TEXT("Perf/Validation");
			}
			else
			{
				type = SE_TEXT("Validation");
			}
		}
		else if (msgType & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)
		{
			type = SE_TEXT("Perf");
		}

		// Fix invalid characters in hex values (bug in Debug Layer)
		char* handleStart = (char*)StringUtils::FindIgnoreCase(callbackData->pMessage, "0x");
		while (handleStart != nullptr)
		{
			while (*handleStart != ' ' && *handleStart != 0)
			{
				*handleStart = Math::Clamp<char>(*handleStart, '0', 'z');
				handleStart++;
			}
			if (*handleStart == 0)
				break;
			handleStart = (char*)StringUtils::FindIgnoreCase(handleStart, "0x");
		}

		StringAnsiView objectName = StringAnsiView::Empty;
		if (callbackData->pObjects != nullptr && callbackData->objectCount > 0)
		{
			VkDebugUtilsObjectNameInfoEXT objectNameInfoExt = callbackData->pObjects[0];
			objectName = StringAnsiView(objectNameInfoExt.pObjectName);
		}

		String info;
		const String message(callbackData->pMessage);
		if (callbackData->pMessageIdName)
		{
			info = String::Format(SE_TEXT("{0} (Vulkan {1}:{2}) {3} {4}"), objectName, type,
				callbackData->messageIdNumber, StringAnsiView(callbackData->pMessageIdName), message);
		}
		else
		{
			info = String::Format(SE_TEXT("{0} (Vulkan {1}) {2} {3}"), objectName, type, callbackData->messageIdNumber, message);
		}

		switch (severity)
		{
		case Severity::Error:
			LOG_ERROR("Graphic", "{0}", info);
			break;
		case Severity::Warning:
			LOG_WARNING("Graphic", "{0}", info);
			break;
		case Severity::Info:
			LOG_INFO("Graphic", "{0}", info);
			break;
		}

		return VK_FALSE;
	}
#endif

	void SetupDebugLayerCallback()
	{
#if VK_EXT_debug_utils
		if (SupportsDebugUtilsExt)
		{
			if (vkCreateDebugUtilsMessengerEXT)
			{
				VkDebugUtilsMessengerCreateInfoEXT createInfo;
				VulkanTool::ZeroStruct(createInfo, VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT);
				createInfo.pfnUserCallback = DebugUtilsCallback;
				switch ((int32)ValidationLevel)
				{
				default:
					createInfo.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
				case 4:
					createInfo.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
				case 3:
					createInfo.messageType |= VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
				case 2:
					createInfo.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
					createInfo.messageType |= VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
				case 1:
					createInfo.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
					createInfo.messageType |= VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT;
					break;
				case 0:
					break;
				}
				const VkResult result = vkCreateDebugUtilsMessengerEXT(GPUDeviceVulkan::instance, &createInfo, nullptr, &Messenger);
				LOG_VULKAN_RESULT(result);
			}
		}
		else if (SupportsDebugCallbackExt)
#else
		if (SupportsDebugCallbackExt)
#endif
		{
#if VK_EXT_debug_report
			if (vkCreateDebugReportCallbackEXT)
			{
				VkDebugReportCallbackCreateInfoEXT createInfo;
				VulkanTool::ZeroStruct(createInfo, VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT);
				createInfo.pfnCallback = DebugReportFunction;
				switch ((int32)ValidationLevel)
				{
				default:
					createInfo.flags |= VK_DEBUG_REPORT_DEBUG_BIT_EXT;
				case 4:
					createInfo.flags |= VK_DEBUG_REPORT_INFORMATION_BIT_EXT;
				case 3:
					createInfo.flags |= VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
				case 2:
					createInfo.flags |= VK_DEBUG_REPORT_WARNING_BIT_EXT;
				case 1:
					createInfo.flags |= VK_DEBUG_REPORT_ERROR_BIT_EXT;
					break;
				case 0:
					break;
				}
				const VkResult result = vkCreateDebugReportCallbackEXT(GPUDeviceVulkan::instance, &createInfo, nullptr, &MsgCallback);
				LOG_VULKAN_RESULT(result);
			}
			else
			{
				LOG_WARNING("Graphic", "GPUDeviceVulkan GetProcAddr: Unable to find vkDbgCreateMsgCallback; debug reporting skipped!");
			}
#endif
		}
		else
		{
			LOG_WARNING("Graphic", "GPUDeviceVulkan Instance does not support 'VK_EXT_debug_report' extension; debug reporting skipped!");
		}
	}

	void RemoveDebugLayerCallback()
	{
#if VK_EXT_debug_utils
		if (Messenger != VK_NULL_HANDLE)
		{
			if (vkDestroyDebugUtilsMessengerEXT)
				vkDestroyDebugUtilsMessengerEXT(GPUDeviceVulkan::instance, Messenger, nullptr);
			Messenger = VK_NULL_HANDLE;
		}
		else if (MsgCallback != VK_NULL_HANDLE)
#else
		if (MsgCallback != VK_NULL_HANDLE)
#endif
		{
#if VK_EXT_debug_report
			if (vkDestroyDebugReportCallbackEXT)
				vkDestroyDebugReportCallbackEXT(GPUDeviceVulkan::instance, MsgCallback, nullptr);
#endif
			MsgCallback = VK_NULL_HANDLE;
		}
	}

	static int32 GetMaxSampleCount(VkSampleCountFlags counts)
	{
		if (counts & VK_SAMPLE_COUNT_64_BIT)
		{
			return VK_SAMPLE_COUNT_64_BIT;
		}
		if (counts & VK_SAMPLE_COUNT_32_BIT)
		{
			return VK_SAMPLE_COUNT_32_BIT;
		}
		if (counts & VK_SAMPLE_COUNT_16_BIT)
		{
			return VK_SAMPLE_COUNT_16_BIT;
		}
		if (counts & VK_SAMPLE_COUNT_8_BIT)
		{
			return VK_SAMPLE_COUNT_8_BIT;
		}
		if (counts & VK_SAMPLE_COUNT_4_BIT)
		{
			return VK_SAMPLE_COUNT_4_BIT;
		}
		if (counts & VK_SAMPLE_COUNT_2_BIT)
		{
			return VK_SAMPLE_COUNT_2_BIT;
		}
		return VK_SAMPLE_COUNT_1_BIT;
	}


	VkInstance GPUDeviceVulkan::instance;
	List<const char*> GPUDeviceVulkan::instanceExtensions;
	List<const char*> GPUDeviceVulkan::instanceLayers;

	GPUDevice* GPUDeviceVulkan::Create(GPUGlobalSettings settings)
	{
		VkResult result;

		result = volkInitialize();
		if (result != VK_SUCCESS)
		{
			Platform::Fatal(String::Format(SE_TEXT("GPUDeviceVulkan volkInitialize failed! ERROR: {0}"), VulkanTool::GetVkErrorString(result)));
			return nullptr;
		}

		// Engine registration
		const StringAsANSI<> appName(*EngineContext::ProductName);
		VkApplicationInfo appInfo;
		VulkanTool::ZeroStruct(appInfo, VK_STRUCTURE_TYPE_APPLICATION_INFO);
		appInfo.pApplicationName =  "";// appName.Get();
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "SE";
//		appInfo.engineVersion = VK_MAKE_VERSION(FLAXENGINE_VERSION_MAJOR, FLAXENGINE_VERSION_MINOR, FLAXENGINE_VERSION_BUILD);
		appInfo.apiVersion = VULKAN_API_VERSION;


		VkInstanceCreateInfo instInfo;
		VulkanTool::ZeroStruct(instInfo, VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO);
#if PLATFORM_APPLE_FAMILY
		instInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif
		instInfo.pApplicationInfo = &appInfo;
		GetInstanceLayersAndExtensions(settings.validationMode, instanceExtensions, instanceLayers, SupportsDebugUtilsExt);
		instInfo.enabledExtensionCount = instanceExtensions.Count();
		instInfo.ppEnabledExtensionNames = instInfo.enabledExtensionCount > 0 ? static_cast<const char* const*>(instanceExtensions.Get()) : nullptr;
		instInfo.enabledLayerCount = instanceLayers.Count();
		instInfo.ppEnabledLayerNames = instInfo.enabledLayerCount > 0 ? instanceLayers.Get() : nullptr;
		SupportsDebugCallbackExt = !SupportsDebugUtilsExt && VulkanTool::HasExtension(instanceExtensions, VK_EXT_DEBUG_REPORT_EXTENSION_NAME);


		// Create Vulkan instance
		result = vkCreateInstance(&instInfo, nullptr, &instance);
		if (result == VK_ERROR_INCOMPATIBLE_DRIVER)
		{
			// Missing driver
#if PLATFORM_APPLE_FAMILY
			Platform::Fatal("Graphic", SE_TEXT("Cannot find a compatible Metal driver."));
#else
			Platform::Fatal(SE_TEXT("GPUDeviceVulkan Cannot find a compatible Vulkan driver."));
#endif
			return nullptr;
		}
		if (result == VK_ERROR_EXTENSION_NOT_PRESENT)
		{
			// Extensions error
			uint32 propertyCount;
			vkEnumerateInstanceExtensionProperties(nullptr, &propertyCount, nullptr);
			List<VkExtensionProperties> properties;
			properties.Resize(propertyCount);
			vkEnumerateInstanceExtensionProperties(nullptr, &propertyCount, properties.Get());
			for (const char* extension : instanceExtensions)
			{
				bool found = false;
				for (uint32 propertyIndex = 0; propertyIndex < propertyCount; propertyIndex++)
				{
					if (!StringUtils::Compare(properties[propertyIndex].extensionName, extension))
					{
						found = true;
						break;
					}
				}
				if (!found)
				{
					Platform::Fatal(String::Format(SE_TEXT("GPUDeviceVulkan Missing required Vulkan extension: {0}"), StringAnsiView(extension)));
				}
			}
			LOG_INFO("Graphic", "Vulkan driver doesn't contain specified extensions:\n{0}\nPlease make sure your layers path is set appropriately.");
			return nullptr;
		}
		if (result != VK_SUCCESS)
		{
			// Driver error
			Platform::Fatal(String::Format(SE_TEXT("Vulkan create instance failed with error code: {0}. Do you have a compatible Vulkan driver installed?"), VulkanTool::GetVkErrorString(result)));
			return nullptr;
		}

		volkLoadInstance(instance);

		SetupDebugLayerCallback();


		// Enumerate all GPU devices and pick one
		int32 selectedAdapterIndex = -1;
		uint32 gpuCount = 0;
		VALIDATE_VULKAN_RESULT(vkEnumeratePhysicalDevices(instance, &gpuCount, nullptr));
		if (gpuCount <= 0)
		{
			Platform::Fatal(SE_TEXT("GPUDeviceVulkan No valid GPU found for Vulkan. Do you have a Vulkan-compatible GPU?"));
			return nullptr;
		}

		ENGINE_ASSERT(gpuCount >= 1);

		List<VkPhysicalDevice, InlinedAllocation<4>> gpus;
		gpus.Resize(gpuCount);
		VALIDATE_VULKAN_RESULT(vkEnumeratePhysicalDevices(instance, &gpuCount, gpus.Get()));
		List<GPUAdapterVulkan, InlinedAllocation<4>> adapters;
		adapters.Resize(gpuCount);
		for (uint32 gpuIndex = 0; gpuIndex < gpuCount; gpuIndex++)
		{
			GPUAdapterVulkan& adapter = adapters[gpuIndex];
			adapter.Gpu = gpus[gpuIndex];
			vkGetPhysicalDeviceProperties(adapter.Gpu, &adapter.GpuProps);
			adapter.Description = adapter.GpuProps.deviceName;

			const Char* type;
			switch (adapter.GpuProps.deviceType)
			{
			case VK_PHYSICAL_DEVICE_TYPE_OTHER:
				type = SE_TEXT("Other");
				break;
			case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
				type = SE_TEXT("Integrated GPU");
				break;
			case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
				type = SE_TEXT("Discrete GPU");
				// Select the first discrete GPU device
				if (selectedAdapterIndex == -1)
					selectedAdapterIndex = gpuIndex;
				break;
			case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
				type = SE_TEXT("Virtual GPU");
				break;
			case VK_PHYSICAL_DEVICE_TYPE_CPU:
				type = SE_TEXT("CPU");
				break;
			default:
				type = SE_TEXT("Unknown");
			}

			LOG_INFO("Graphic", "GPUDeviceVulkan Adapter {}: '{}', API {}.{}.{}, Driver {}.{}.{}", gpuIndex, adapter.Description, VK_VERSION_MAJOR(adapter.GpuProps.apiVersion), VK_VERSION_MINOR(adapter.GpuProps.apiVersion), VK_VERSION_PATCH(adapter.GpuProps.apiVersion), VK_VERSION_MAJOR(adapter.GpuProps.driverVersion), VK_VERSION_MINOR(adapter.GpuProps.driverVersion), VK_VERSION_PATCH(adapter.GpuProps.driverVersion));
			LOG_INFO("Graphic", "GPUDeviceVulkan	VendorId: 0x{:x}, Type: {}, Max Descriptor Sets Bound: {}, Timestamps: {}", adapter.GpuProps.vendorID, type, adapter.GpuProps.limits.maxBoundDescriptorSets, !!adapter.GpuProps.limits.timestampComputeAndGraphics);
		}

		// Select the adapter to use
		if (selectedAdapterIndex < 0)
			selectedAdapterIndex = 0;
		if (adapters.Count() == 0 || selectedAdapterIndex >= adapters.Count())
		{
			Platform::Fatal(SE_TEXT("GPUDeviceVulkan Failed to find valid Vulkan adapter!"));
			return nullptr;
		}

		// Create device
		auto device = New<GPUDeviceVulkan>(settings, New<GPUAdapterVulkan>(adapters[selectedAdapterIndex]));
		if (!device->Init())
		{
			Platform::Fatal(SE_TEXT("GPUDeviceVulkan Graphics Device init failed"));
			Delete(device);
			return nullptr;
		}

		return device;
	}


	GPUDeviceVulkan::GPUDeviceVulkan(GPUGlobalSettings settings, GPUAdapterVulkan* gpuAdapter) :
		GPUDevice(settings),
		gpuAdapter(gpuAdapter),
		m_RenderPasses(512),
		m_Framebuffers(512),
		m_Layouts(4096),
		deferredDeletionQueue(this),
		stagingManager(this),
		dummyResources(this)
	{

	}

	GPUDeviceVulkan::~GPUDeviceVulkan()
	{

	}

	bool GPUDeviceVulkan::Init()
	{
		m_TotalGraphicsMemory = 0;
		m_DeviceState = DeviceState::Created;
		auto gpu = gpuAdapter->Gpu;

		// Get queues properties
		uint32 queueCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queueCount, nullptr);
		ENGINE_ASSERT(queueCount >= 1);
		queueFamilyProps.AddDefault(queueCount);
		vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queueCount, queueFamilyProps.Get());

		// Query device features
		vkGetPhysicalDeviceFeatures(gpu, &physicalDeviceFeatures);

		// Get extensions and layers
		List<const char*> deviceExtensions;
		List<const char*> validationLayers;
		GetDeviceExtensionsAndLayers(gpu, deviceExtensions, validationLayers);
		ParseOptionalDeviceExtensions(deviceExtensions);

		// Setup device info
		VkDeviceCreateInfo deviceInfo;
		VulkanTool::ZeroStruct(deviceInfo, VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO);
		deviceInfo.enabledExtensionCount = deviceExtensions.Count();
		deviceInfo.ppEnabledExtensionNames = deviceExtensions.Get();
		deviceInfo.enabledLayerCount = validationLayers.Count();
		deviceInfo.ppEnabledLayerNames = deviceInfo.enabledLayerCount > 0 ? validationLayers.Get() : nullptr;

		// Setup queues info
		List<VkDeviceQueueCreateInfo> queueFamilyInfos;
		int32 graphicsQueueFamilyIndex = -1;
		int32 computeQueueFamilyIndex = -1;
		int32 transferQueueFamilyIndex = -1;
		LOG_INFO("Graphic", "GPUDevice Found {0} queue families:", queueFamilyProps.Count());
		uint32 numPriorities = 0;
		for (int32 familyIndex = 0; familyIndex < queueFamilyProps.Count(); familyIndex++)
		{
			const VkQueueFamilyProperties& curProps = queueFamilyProps[familyIndex];

			bool isValidQueue = false;
			if ((curProps.queueFlags & VK_QUEUE_GRAPHICS_BIT) == VK_QUEUE_GRAPHICS_BIT)
			{
				if (graphicsQueueFamilyIndex == -1)
				{
					graphicsQueueFamilyIndex = familyIndex;
					isValidQueue = true;
				}
				else
				{
					// TODO: Support for multi-queue and choose the best queue
				}
			}

			if ((curProps.queueFlags & VK_QUEUE_COMPUTE_BIT) == VK_QUEUE_COMPUTE_BIT)
			{
				if (computeQueueFamilyIndex == -1 && graphicsQueueFamilyIndex != familyIndex)
				{
					computeQueueFamilyIndex = familyIndex;
					isValidQueue = true;
				}
			}

			if ((curProps.queueFlags & VK_QUEUE_TRANSFER_BIT) == VK_QUEUE_TRANSFER_BIT)
			{
				// Favor a non-gfx transfer queue
				if (transferQueueFamilyIndex == -1 && (curProps.queueFlags & VK_QUEUE_GRAPHICS_BIT) != VK_QUEUE_GRAPHICS_BIT && (curProps.queueFlags & VK_QUEUE_COMPUTE_BIT) != VK_QUEUE_COMPUTE_BIT)
				{
					transferQueueFamilyIndex = familyIndex;
					isValidQueue = true;
				}
			}

			String queueTypeInfo;
			if ((curProps.queueFlags & VK_QUEUE_GRAPHICS_BIT) == VK_QUEUE_GRAPHICS_BIT)
				queueTypeInfo += SE_TEXT(" graphics");
			if ((curProps.queueFlags & VK_QUEUE_COMPUTE_BIT) == VK_QUEUE_COMPUTE_BIT)
				queueTypeInfo += SE_TEXT(" compute");
			if ((curProps.queueFlags & VK_QUEUE_TRANSFER_BIT) == VK_QUEUE_TRANSFER_BIT)
				queueTypeInfo += SE_TEXT(" transfer");
			if ((curProps.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) == VK_QUEUE_SPARSE_BINDING_BIT)
				queueTypeInfo += SE_TEXT(" sparse");

			if (!isValidQueue)
			{
				LOG_INFO("Graphic", "GPUDevice Skipping unnecessary queue family {0}: {1} queues{2}", familyIndex, curProps.queueCount, queueTypeInfo);
				continue;
			}

			const int32 queueIndex = queueFamilyInfos.Count();
			queueFamilyInfos.AddZeroed(1);
			VkDeviceQueueCreateInfo& curQueue = queueFamilyInfos[queueIndex];
			curQueue.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			curQueue.queueFamilyIndex = familyIndex;
			curQueue.queueCount = curProps.queueCount;
			numPriorities += curProps.queueCount;
			LOG_INFO("Graphic", "GPUDeviceVulkan - queue family {0}: {1} queues{2}", familyIndex, curProps.queueCount, queueTypeInfo);
		}

		List<float> queuePriorities;
		queuePriorities.AddDefault(numPriorities);
		float* currentPriority = queuePriorities.Get();
		for (int32 index = 0; index < queueFamilyInfos.Count(); index++)
		{
			VkDeviceQueueCreateInfo& queue = queueFamilyInfos[index];
			queue.pQueuePriorities = currentPriority;
			const VkQueueFamilyProperties& properties = queueFamilyProps[queue.queueFamilyIndex];
			for (int32 queueIndex = 0; queueIndex < (int32)properties.queueCount; queueIndex++)
				*currentPriority++ = 1.0f;
		}
		deviceInfo.queueCreateInfoCount = queueFamilyInfos.Count();
		deviceInfo.pQueueCreateInfos = queueFamilyInfos.Get();

		VkPhysicalDeviceFeatures enabledFeatures;
		VulkanPlatform::RestrictEnabledPhysicalDeviceFeatures(physicalDeviceFeatures, enabledFeatures);
		deviceInfo.pEnabledFeatures = &enabledFeatures;

		// Create the device
		VkResult result = vkCreateDevice(gpu, &deviceInfo, nullptr, &device);
		if (result != VK_SUCCESS)
		{
			Platform::Fatal(String::Format(SE_TEXT("Vulkan error: {0}"), VulkanTool::GetVkErrorString(result)));
		}
		// Optimize bindings
		volkLoadDevice(device);

		// Create queues
		if (graphicsQueueFamilyIndex == -1)
		{
			Platform::Fatal(SE_TEXT("GPUDevice Missing Vulkan graphics queue."));
			return false;
		}

		graphicsQueue = New<QueueVulkan>(this, graphicsQueueFamilyIndex);
		computeQueue = computeQueueFamilyIndex != -1 ? New<QueueVulkan>(this, computeQueueFamilyIndex) : graphicsQueue;
		transferQueue = transferQueueFamilyIndex != -1 ? New<QueueVulkan>(this, transferQueueFamilyIndex) : graphicsQueue;

		InitDeviceLimits(gpu);
		InitMemory(gpu);

		// Prepare stuff
		fenceManager.Init(this);
		uniformBufferUploader = New<UniformBufferUploaderVulkan>(this);
		descriptorPoolsManager = New<DescriptorPoolsManagerVulkan>(this);
		mainContext = New<GPUContextVulkan>(this, graphicsQueue);

/*		if (vkCreatePipelineCache)
		{
			List<uint8> data;
			String path;
			GetPipelineCachePath(path);
			if (FileSystem::FileExists(path))
			{
				LOG_INFO("Graphic", "GPUDeviceVulkan", "Trying to load Vulkan pipeline cache file {0}", path);
				File::ReadAllBytes(path, data);
			}
			VkPipelineCacheCreateInfo pipelineCacheCreateInfo;
			VulkanTool::ZeroStruct(pipelineCacheCreateInfo, VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO);
			pipelineCacheCreateInfo.initialDataSize = data.Count();
			pipelineCacheCreateInfo.pInitialData = data.Count() > 0 ? data.Get() : nullptr;
			const VkResult result = vkCreatePipelineCache(device, &pipelineCacheCreateInfo, nullptr, &pipelineCache);
			LOG_VULKAN_RESULT(result);
		}*/
#if VULKAN_USE_VALIDATION_CACHE
		if (optionalDeviceExtensions.HasEXTValidationCache && vkCreateValidationCacheEXT && vkDestroyValidationCacheEXT)
		{
			List<uint8> data;
			String path;
			GetValidationCachePath(path);
			if (FileSystem::FileExists(path))
			{
				LOG_INFO("Graphic", "GPUDeviceVulkan", "Trying to load Vulkan validation cache file {0}", path);
				File::ReadAllBytes(path, data);
				if (data.HasItems())
				{
					int32* dataPtr = (int32*)data.Get();
					if (*dataPtr > 0)
					{
						const int32 cacheSize = *dataPtr++;
						const int32 cacheVersion = *dataPtr++;
						const int32 cacheVersionExpected = VK_PIPELINE_CACHE_HEADER_VERSION_ONE;
						if (cacheVersion == cacheVersionExpected)
						{
							dataPtr += VK_UUID_SIZE / sizeof(int32);
						}
						else
						{
							LOG_WARNING("Graphic", "GPUDeviceVulkan", "Bad validation cache file, version: {0}, expected: {1}", cacheVersion, cacheVersionExpected);
							data.Clear();
						}
					}
					else
					{
						LOG_WARNING("Graphic", "GPUDeviceVulkan", "Bad validation cache file, header size: {0}", *dataPtr);
						data.Clear();
					}
				}
			}
			VkValidationCacheCreateInfoEXT validationCreateInfo;
			VulkanTool::ZeroStruct(validationCreateInfo, VK_STRUCTURE_TYPE_VALIDATION_CACHE_CREATE_INFO_EXT);
			validationCreateInfo.initialDataSize = data.Count();
			validationCreateInfo.pInitialData = data.Count() > 0 ? data.Get() : nullptr;
			const VkResult result = vkCreateValidationCacheEXT(device, &validationCreateInfo, nullptr, &ValidationCache);
			LOG_VULKAN_RESULT(result);
		}
#endif

		m_DeviceState = DeviceState::Ready;
		return GPUDevice::Init();
	}

	void GPUDeviceVulkan::InitDeviceLimits(VkPhysicalDevice_T* gpu)
	{
		physicalDeviceLimits = gpuAdapter->GpuProps.limits;
		MSAALevel maxMsaa = MSAALevel::None;
		if (physicalDeviceFeatures.sampleRateShading)
		{
			const int32
				framebufferColorSampleCounts = GetMaxSampleCount(physicalDeviceLimits.framebufferColorSampleCounts);
			const int32
				framebufferDepthSampleCounts = GetMaxSampleCount(physicalDeviceLimits.framebufferDepthSampleCounts);
			maxMsaa =
				(MSAALevel)Math::Clamp(Math::Min<int32>(framebufferColorSampleCounts, framebufferDepthSampleCounts),
					1,
					8);
		}

//			limits.HasCompute = GetShaderProfile() == ShaderProfile::Vulkan_SM5 && physicalDeviceLimits.maxComputeWorkGroupCount[0] >= GPU_MAX_CS_DISPATCH_THREAD_GROUPS && physicalDeviceLimits.maxComputeWorkGroupCount[1] >= GPU_MAX_CS_DISPATCH_THREAD_GROUPS;
		limits.HasTessellation = !!physicalDeviceFeatures.tessellationShader
			&& physicalDeviceLimits.maxBoundDescriptorSets > (uint32)DescriptorSet::Domain;
		limits.HasGeometryShaders = !!physicalDeviceFeatures.geometryShader
			&& physicalDeviceLimits.maxBoundDescriptorSets > (uint32)DescriptorSet::Geometry;

		limits.HasInstancing = true;
		limits.HasVolumeTextureRendering = true;
		limits.HasDrawIndirect = physicalDeviceLimits.maxDrawIndirectCount >= 1;
		limits.HasAppendConsumeBuffers = false; // TODO: add Append Consume buffers support for Vulkan
		limits.HasSeparateRenderTargetBlendState = true;
		limits.HasDepthClip = physicalDeviceFeatures.depthClamp;
		limits.HasDepthAsSRV = true;
		limits.HasReadOnlyDepth = true;
		limits.HasMultisampleDepthAsSRV = !!physicalDeviceFeatures.sampleRateShading;
		limits.HasTypedUAVLoad = true;
		limits.MaximumMipLevelsCount =
			Math::Min(static_cast<int32>(log2(physicalDeviceLimits.maxImageDimension2D)), GPU_MAX_TEXTURE_MIP_LEVELS);
		limits.MaximumTexture1DSize = physicalDeviceLimits.maxImageDimension1D;
		limits.MaximumTexture1DArraySize = physicalDeviceLimits.maxImageArrayLayers;
		limits.MaximumTexture2DSize = physicalDeviceLimits.maxImageDimension2D;
		limits.MaximumTexture2DArraySize = physicalDeviceLimits.maxImageArrayLayers;
		limits.MaximumTexture3DSize = physicalDeviceLimits.maxImageDimension3D;
		limits.MaximumTextureCubeSize = physicalDeviceLimits.maxImageDimensionCube;
		limits.MaximumSamplerAnisotropy = physicalDeviceLimits.maxSamplerAnisotropy;

		for (int32 i = 0; i < static_cast<int32>(PixelFormat::Max); i++)
		{
			const auto format = static_cast<PixelFormat>(i);
			const auto vkFormat = VulkanTool::ToVulkanFormat(format);

			MSAALevel msaa = MSAALevel::None;
			EnumFlags<FormatSupport> support = FormatSupport::None;

			if (vkFormat != VK_FORMAT_UNDEFINED)
			{
				VkFormatProperties properties;
				Platform::MemoryClear(&properties, sizeof(properties));
				vkGetPhysicalDeviceFormatProperties(gpu, vkFormat, &properties);

				// Query image format features support flags
#define CHECK_IMAGE_FORMAT(bit, feature) if (((properties.linearTilingFeatures & bit) == bit) || ((properties.optimalTilingFeatures & bit) == bit)) support.SetFlag(feature)
				if (properties.linearTilingFeatures != 0 || properties.optimalTilingFeatures != 0)
				{
					support.SetFlag(FormatSupport::Texture1D);
					support.SetFlag(FormatSupport::Texture2D);
					support.SetFlag(FormatSupport::Texture3D);
					support.SetFlag(FormatSupport::TextureCube);
				}
				CHECK_IMAGE_FORMAT(VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT, FormatSupport::ShaderLoad);
				//VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT,
				//VK_FORMAT_FEATURE_STORAGE_IMAGE_ATOMIC_BIT
				CHECK_IMAGE_FORMAT(VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT, FormatSupport::RenderTarget);
				CHECK_IMAGE_FORMAT(VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT, FormatSupport::Blendable);
				CHECK_IMAGE_FORMAT(VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT, FormatSupport::DepthStencil);
				//VK_FORMAT_FEATURE_BLIT_SRC_BIT
				//VK_FORMAT_FEATURE_BLIT_DST_BIT
				CHECK_IMAGE_FORMAT(VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT, FormatSupport::ShaderSampleComparison);
				CHECK_IMAGE_FORMAT(VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT, FormatSupport::ShaderSample);
#undef CHECK_IMAGE_FORMAT

				// Query buffer format features support flags
#define CHECK_BUFFER_FORMAT(bit, feature) if ((properties.bufferFeatures & bit) == bit) support.SetFlag(feature)
				if (properties.bufferFeatures != 0)
				{
					support.SetFlag(FormatSupport::Buffer);
				}
				//VK_FORMAT_FEATURE_UNIFORM_TEXEL_BUFFER_BIT
				//VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_BIT
				//VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_ATOMIC_BIT
				CHECK_BUFFER_FORMAT(VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT, FormatSupport::InputAssemblyVertexBuffer);
#undef CHECK_BUFFER_FORMAT

				// Unused bits
				//VK_FORMAT_FEATURE_TRANSFER_SRC_BIT
				//VK_FORMAT_FEATURE_TRANSFER_DST_BIT
				//VK_FORMAT_FEATURE_MIDPOINT_CHROMA_SAMPLES_BIT
				//VK_FORMAT_FEATURE_SAMPLED_IMAGE_YCBCR_CONVERSION_LINEAR_FILTER_BIT
				//VK_FORMAT_FEATURE_SAMPLED_IMAGE_YCBCR_CONVERSION_SEPARATE_RECONSTRUCTION_FILTER_BIT
				//VK_FORMAT_FEATURE_SAMPLED_IMAGE_YCBCR_CONVERSION_CHROMA_RECONSTRUCTION_EXPLICIT_BIT
				//VK_FORMAT_FEATURE_SAMPLED_IMAGE_YCBCR_CONVERSION_CHROMA_RECONSTRUCTION_EXPLICIT_FORCEABLE_BIT
				//VK_FORMAT_FEATURE_DISJOINT_BIT
				//VK_FORMAT_FEATURE_COSITED_CHROMA_SAMPLES_BIT
				//VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_CUBIC_BIT_IMG
				//VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_MINMAX_BIT_EXT

				// Multi-sampling support
				if (support.IsFlag(FormatSupport::Texture2D))
				{
					msaa = maxMsaa;
				}
			}

			FeaturesPerFormat[i] = FormatFeatures(format, msaa, support);
		}

	}

	void GPUDeviceVulkan::InitMemory(VkPhysicalDevice_T* gpu)
	{
		// Setup memory limit and print memory info
		VkPhysicalDeviceMemoryProperties memoryProperties;
		vkGetPhysicalDeviceMemoryProperties(gpu, &memoryProperties);
		LOG_INFO("Graphic", "GPUDeviceVulkan Max memory allocations: {0}", gpuAdapter->GpuProps.limits.maxMemoryAllocationCount);
		LOG_INFO("Graphic", "GPUDeviceVulkan Found {0} device memory heaps:", memoryProperties.memoryHeapCount);
		for (uint32 i = 0; i < memoryProperties.memoryHeapCount; i++)
		{
			const VkMemoryHeap& heap = memoryProperties.memoryHeaps[i];
			bool isGPUHeap = (heap.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) == VK_MEMORY_HEAP_DEVICE_LOCAL_BIT;
			LOG_INFO("Graphic", "GPUDeviceVulkan -  memory heap {0}: flags 0x{1:x}, size {2} MB (GPU: {3})", i, heap.flags, (uint32)(heap.size / 1024 / 1024), isGPUHeap);
			if (isGPUHeap)
			{
				m_TotalGraphicsMemory += heap.size;
			}
		}
		LOG_INFO("Graphic", "GPUDeviceVulkan Found {0} device memory types:", memoryProperties.memoryTypeCount);
		for (uint32 i = 0; i < memoryProperties.memoryTypeCount; i++)
		{
			const VkMemoryType& type = memoryProperties.memoryTypes[i];
			String flagsInfo;
			if ((type.propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
				flagsInfo += SE_TEXT("local, ");
			if ((type.propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
				flagsInfo += SE_TEXT("host visible, ");
			if ((type.propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
				flagsInfo += SE_TEXT("host coherent, ");
			if ((type.propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) == VK_MEMORY_PROPERTY_HOST_CACHED_BIT)
				flagsInfo += SE_TEXT("host cached, ");
			if ((type.propertyFlags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT) == VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT)
				flagsInfo += SE_TEXT("lazy, ");
			if (flagsInfo.HasChars())
				flagsInfo = SE_TEXT(", properties: ") + flagsInfo.Left(flagsInfo.Length() - 2);
			LOG_INFO("Graphic", "GPUDeviceVulkan -  memory type {0}: flags 0x{1:x}, heap {2}{3}", i, type.propertyFlags, type.heapIndex, flagsInfo);
		}

		// Initialize memory allocator
		VmaVulkanFunctions vulkanFunctions;
#define INIT_FUNC(name) vulkanFunctions.name = name
		INIT_FUNC(vkGetPhysicalDeviceProperties);
		INIT_FUNC(vkGetPhysicalDeviceMemoryProperties);
		INIT_FUNC(vkAllocateMemory);
		INIT_FUNC(vkFreeMemory);
		INIT_FUNC(vkMapMemory);
		INIT_FUNC(vkUnmapMemory);
		INIT_FUNC(vkFlushMappedMemoryRanges);
		INIT_FUNC(vkInvalidateMappedMemoryRanges);
		INIT_FUNC(vkBindBufferMemory);
		INIT_FUNC(vkBindImageMemory);
		INIT_FUNC(vkGetBufferMemoryRequirements);
		INIT_FUNC(vkGetImageMemoryRequirements);
		INIT_FUNC(vkCreateBuffer);
		INIT_FUNC(vkDestroyBuffer);
		INIT_FUNC(vkCreateImage);
		INIT_FUNC(vkDestroyImage);
		INIT_FUNC(vkCmdCopyBuffer);
#if VMA_DEDICATED_ALLOCATION
#if PLATFORM_SWITCH
		vulkanFunctions.vkGetBufferMemoryRequirements2KHR = vkGetBufferMemoryRequirements2;
        vulkanFunctions.vkGetImageMemoryRequirements2KHR = vkGetImageMemoryRequirements2;
#else
		INIT_FUNC(vkGetBufferMemoryRequirements2KHR);
		INIT_FUNC(vkGetImageMemoryRequirements2KHR);
#endif
#endif
#undef INIT_FUNC
		VmaAllocatorCreateInfo allocatorInfo = {};
		allocatorInfo.vulkanApiVersion = VULKAN_API_VERSION;
		allocatorInfo.physicalDevice = gpu;
		allocatorInfo.instance = instance;
		allocatorInfo.device = device;
		allocatorInfo.pVulkanFunctions = &vulkanFunctions;

		VkResult result = vmaCreateAllocator(&allocatorInfo, &allocator);
		if (result != VK_SUCCESS)
		{
			Platform::Fatal(String::Format(SE_TEXT("Vulkan error: {0}"), VulkanTool::GetVkErrorString(result)));
		}
	}

	void GPUDeviceVulkan::Dispose()
	{
		GPUDeviceLock lock(this);

		// Check if has been disposed already
		if (m_DeviceState == DeviceState::Disposed)
			return;

		// Set current state
		m_DeviceState = DeviceState::Disposing;

		// Wait for rendering end
		WaitForGPU();

		// Pre dispose
		PreDispose();

		// Clear stuff
		m_Framebuffers.ClearDelete();
		m_RenderPasses.ClearDelete();
		m_Layouts.ClearDelete();
//		HelperResources.Dispose();
		stagingManager.Dispose();
		TimestampQueryPools.ClearDelete();
		uniformBufferUploader->ReleaseGPU();
		Delete(uniformBufferUploader);
		Delete(descriptorPoolsManager);
		Delete(mainContext);

		if (transferQueue != graphicsQueue && computeQueue != transferQueue)
		{
			Delete(transferQueue);
		}
		if (computeQueue != graphicsQueue)
		{
			Delete(computeQueue);
		}
		Delete(graphicsQueue);

		uniformBufferUploader = nullptr;
		mainContext = nullptr;
		graphicsQueue = nullptr;
		computeQueue = nullptr;
		transferQueue = nullptr;

//		presentQueue = nullptr;
		fenceManager.Dispose();
		deferredDeletionQueue.ReleaseResources(true);
		vmaDestroyAllocator(allocator);
		allocator = VK_NULL_HANDLE;

/*		if (pipelineCache != VK_NULL_HANDLE)
		{
			if (SavePipelineCache())
			{
				LOG_WARNING("Graphic", "GPUDeviceVulkan", "Failed to save Vulkan pipeline cache");
			}
			vkDestroyPipelineCache(device, pipelineCache, nullptr);
			pipelineCache = VK_NULL_HANDLE;
		}*/
#if VULKAN_USE_VALIDATION_CACHE
		if (ValidationCache != VK_NULL_HANDLE)
    {
        if (SaveValidationCache())
            LOG(Warning, "Failed to save Vulkan validation cache");
        vkDestroyValidationCacheEXT(Device, ValidationCache, nullptr);
        ValidationCache = VK_NULL_HANDLE;
    }
#endif

		// Destroy device
		vkDestroyDevice(device, nullptr);
		device = VK_NULL_HANDLE;
		Delete(gpuAdapter);
		gpuAdapter = nullptr;

		// Shutdown Vulkan
#if VULKAN_USE_DEBUG_LAYER
		RemoveDebugLayerCallback();
#endif
		vkDestroyInstance(instance, nullptr);

		// Base
		GPUDevice::Dispose();

		// Set current state
		m_DeviceState = DeviceState::Disposed;
	}

	void GPUDeviceVulkan::WaitForGPU()
	{
		if (device != VK_NULL_HANDLE)
		{
			PROFILE_CPU();
			VALIDATE_VULKAN_RESULT(vkDeviceWaitIdle(device));
		}
	}

	void GPUDeviceVulkan::DrawBegin()
	{
		// Base
		GPUDevice::DrawBegin();

		// Flush resources
		deferredDeletionQueue.ReleaseResources();
		stagingManager.ProcessPendingFree();
		descriptorPoolsManager->GC();
	}

	GPUSwapChain* GPUDeviceVulkan::CreateSwapChain(Window* window)
	{
		return New<GPUSwapChainVulkan>(this, window);
	}

	GPUPipelineState* GPUDeviceVulkan::CreatePipelineState()
	{
		return New<GPUPipelineStateVulkan>(this);
	}

	GPUTexture* GPUDeviceVulkan::CreateTexture(const StringView& name)
	{
		return New<GPUTextureVulkan>(this, name);
	}

	GPUShader* GPUDeviceVulkan::CreateShader(const StringView& name)
	{
		return New<GPUShaderVulkan>(this, name);
	}

	GPUBuffer* GPUDeviceVulkan::CreateBuffer(const StringView& name)
	{
		return New<GPUBufferVulkan>(this, name);
	}

	GPUSampler* GPUDeviceVulkan::CreateSampler()
	{
		return New<GPUSamplerVulkan>(this);
	}

	GPUTimerQuery* GPUDeviceVulkan::CreateTimerQuery()
	{
		return New<GPUTimerQueryVulkan>(this);
	}

	GPUConstantBuffer* GPUDeviceVulkan::CreateConstantBuffer(uint32 size, const StringView& name)
	{
		return New<GPUConstantBufferVulkan>(this, size);
	}


	RenderPassVulkan* GPUDeviceVulkan::GetOrCreateRenderPass(RenderTargetLayoutVulkan& layout)
	{
		RenderPassVulkan* renderPass;
		if (m_RenderPasses.TryGet(layout, renderPass))
			return renderPass;

		PROFILE_CPU_NAMED("Create Render Pass");
		renderPass = New<RenderPassVulkan>(this, layout);
		m_RenderPasses.Add(layout, renderPass);
		return renderPass;
	}

	FramebufferVulkan* GPUDeviceVulkan::GetOrCreateFramebuffer(FramebufferVulkan::Desc& key, VkExtent2D& extent,
		uint32 layers)
	{
		FramebufferVulkan* framebuffer;
		if (m_Framebuffers.TryGet(key, framebuffer))
			return framebuffer;

		PROFILE_CPU_NAMED("Create Framebuffer");
		framebuffer = New<FramebufferVulkan>(this, key, extent, layers);
		m_Framebuffers.Add(key, framebuffer);
		return framebuffer;
	}

	PipelineLayoutVulkan* GPUDeviceVulkan::GetOrCreateLayout(DescriptorSetLayoutInfoVulkan& key)
	{
		PipelineLayoutVulkan* layout;
		if (m_Layouts.TryGet(key, layout))
			return layout;

		PROFILE_CPU_NAMED("Create Pipeline Layout");
		layout = New<PipelineLayoutVulkan>(this, key);
		m_Layouts.Add(key, layout);
		return layout;
	}

	void GPUDeviceVulkan::OnImageViewDestroy(VkImageView imageView)
	{
		for (auto i = m_Framebuffers.begin(); i.IsNotEnd(); ++i)
		{
			if (i->Value->HasReference(imageView))
			{
				Delete(i->Value);
				m_Framebuffers.Remove(i);
				--i;
			}
		}
	}

	void GPUDeviceVulkan::SetupPresentQueue(VkSurfaceKHR surface)
	{
		if (presentQueue)
			return;

		const auto supportsPresent = [surface](VkPhysicalDevice physicalDevice, QueueVulkan* queue)
		{
		  VkBool32 supportsPresent = VK_FALSE;
		  const uint32 queueFamilyIndex = queue->GetFamilyIndex();
		  VALIDATE_VULKAN_RESULT(vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, queueFamilyIndex, surface, &supportsPresent));
		  if (supportsPresent)
		  {
			  LOG_INFO("Graphic", "Vulkan Queue Family {0}: supports present", queueFamilyIndex);
		  }
		  return supportsPresent == VK_TRUE;
		};

		const auto gpu = gpuAdapter->Gpu;
		bool graphics = supportsPresent(gpu, graphicsQueue);
		if (!graphics)
		{
			LOG_ERROR("Graphic", "Vulkan Graphics Queue doesn't support present");
		}
	// TODO: test using Compute queue for present
	//bool compute = SupportsPresent(gpu, ComputeQueue);
		presentQueue = graphicsQueue;
	}

	PixelFormat GPUDeviceVulkan::GetClosestSupportedPixelFormat(PixelFormat format, EnumFlags<GPUTextureFlags> flags, bool optimalTiling)
	{
		// Collect features to use
		VkFormatFeatureFlags wantedFeatureFlags = 0;
		if (flags.IsFlag(GPUTextureFlags::ShaderResource))
			wantedFeatureFlags |= VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
		if (flags.IsFlag(GPUTextureFlags::RenderTarget))
			wantedFeatureFlags |= VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT;
		if (flags.IsFlag(GPUTextureFlags::DepthStencil))
			wantedFeatureFlags |= VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
		if (flags.IsFlag(GPUTextureFlags::UnorderedAccess))
			wantedFeatureFlags |= VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT;

		if (!IsVkFormatSupported(VulkanTool::ToVulkanFormat(format), wantedFeatureFlags, optimalTiling))
		{
			// Special case for depth-stencil formats
			if (flags.IsFlag(GPUTextureFlags::DepthStencil))
			{
				const bool hasStencil = PixelFormatIsStencilSupport(format);

				// Spec guarantees at least one depth-only, and one depth-stencil format to be supported
				if (hasStencil)
				{
					if (IsVkFormatSupported(VK_FORMAT_D32_SFLOAT_S8_UINT, wantedFeatureFlags, optimalTiling))
						format = PixelFormat::D32_Float;
					else
						format = PixelFormat::D24_UNorm_S8_UInt;
				}
				else
				{
					// The only format that could have failed is 32-bit depth, so we must use the alternative 16-bit. Spec guarantees it is always supported.
					format = PixelFormat::D16_UNorm;
				}
			}
			else
			{
				// Perform remapping to bigger format that might be supported (more likely)
				auto remap = format;
				switch (format)
				{
				case PixelFormat::R11G11B10_Float:
				case PixelFormat::R10G10B10A2_UNorm:
					remap = PixelFormat::R16G16B16A16_Float;
					break;
				case PixelFormat::R16_Float:
					remap = PixelFormat::R32_Float;
					break;
				case PixelFormat::R16G16_UNorm:
				case PixelFormat::R16G16_Float:
					remap = PixelFormat::R32G32_Float;
					break;
				case PixelFormat::R32G32B32A32_Float:
					// RGBA32 is essential
					return PixelFormat::Undefined;
				default:
					// Ultimate performance eater
					remap = PixelFormat::R32G32B32A32_Float;
					break;
				}
#if !BUILD_RELEASE
				if (format != remap)
				{
					LOG_WARNING("Graphic", "GPUDeviceVulkan Unsupported Vulkan format {0}. Remapping to {1}",
						PixelFormatGetString(format),
						PixelFormatGetString(remap));
					format = GetClosestSupportedPixelFormat(remap, flags, optimalTiling);
				}
#endif
			}
		}

		return format;
	}

	bool GPUDeviceVulkan::IsVkFormatSupported(VkFormat vkFormat, VkFormatFeatureFlags wantedFeatureFlags, bool optimalTiling) const
	{
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(gpuAdapter->Gpu, vkFormat, &props);
		const VkFormatFeatureFlags featureFlags = optimalTiling ? props.optimalTilingFeatures : props.linearTilingFeatures;
		if ((featureFlags & wantedFeatureFlags) != wantedFeatureFlags)
			return false;

		//VkImageFormatProperties imageProps;
		//vkGetPhysicalDeviceImageFormatProperties(Adapter->Gpu, vkFormat, , &imageProps);

		return true;
	}

	GPUContext* GPUDeviceVulkan::GetMainContext()
	{
		return mainContext;
	}


/*    bool CheckExtensionSupport(const char *checkExtension, const List<VkExtensionProperties> &available_extensions)
    {
        for (const auto &x : available_extensions)
        {
            if (StringUtils::Compare(x.extensionName, checkExtension) == 0)
            {
                return true;
            }
        }
        return false;
    }

    bool ValidateLayers(const List<const char *> &required, const List<VkLayerProperties> &available)
    {
        for (auto layer : required)
        {
            bool found = false;
            for (auto &available_layer : available)
            {
                if (StringUtils::Compare(available_layer.layerName, layer) == 0)
                {
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                return false;
            }
        }

        return true;
    }

    GPUDeviceVulkan::~GPUDeviceVulkan()
    {
        VkResult res = vkDeviceWaitIdle(m_Device);
        ENGINE_ASSERT(res == VK_SUCCESS);

        for (auto &queue : m_CommandQueues)
        {
            vkDestroySemaphore(m_Device, queue.semaphore, nullptr);
            queue.locker.reset();
        }

        for (uint32 fr = 0; fr < BUFFER_COUNT; ++fr)
        {
            for (int queue = 0; queue < RHIQueueType_Count; ++queue)
            {
                vkDestroyFence(m_Device, m_FrameFence[fr][queue], nullptr);
            }
        }

        m_CopyAllocator.Destroy();

        for (auto &x : m_PsoLayoutCache)
        {
            vkDestroyPipelineLayout(m_Device, x.Value.pipelineLayout, nullptr);
            vkDestroyDescriptorSetLayout(m_Device, x.Value.descriptorSetLayout, nullptr);
        }

        for (auto &commandlist : m_Commandlists)
        {
            for (int buffer = 0; buffer < BUFFER_COUNT; ++buffer)
            {
                for (int queue = 0; queue < RHIQueueType_Count; ++queue)
                {
                    vkDestroyCommandPool(m_Device, commandlist->commandPools[buffer][queue], nullptr);
                }
            }

            for (auto &worker : commandlist->pipelines_worker)
            {
                vkDestroyPipeline(m_Device, worker.second, nullptr);
            }

            for (auto &binder : commandlist->binder_pools)
            {
                binder.Destroy();
            }

            commandlist.release();
        }

        for (auto &pipeline : m_PipelinesGlobal)
        {
            vkDestroyPipeline(m_Device, pipeline.Value, nullptr);
        }

        vmaDestroyBuffer(m_Allocationhandler->allocator, m_NullBuffer, m_NullBufferAllocation);
        vkDestroyBufferView(m_Device, m_NullBufferView, nullptr);
        vmaDestroyImage(m_Allocationhandler->allocator, m_NullImage1D, m_NullImageAllocation1D);
        vmaDestroyImage(m_Allocationhandler->allocator, m_NullImage2D, m_NullImageAllocation2D);
        vmaDestroyImage(m_Allocationhandler->allocator, m_NullImage3D, m_NullImageAllocation3D);
        vkDestroyImageView(m_Device, m_NullImageView1D, nullptr);
        vkDestroyImageView(m_Device, m_NullImageView1DArray, nullptr);
        vkDestroyImageView(m_Device, m_NullImageView2D, nullptr);
        vkDestroyImageView(m_Device, m_NullImageView2DArray, nullptr);
        vkDestroyImageView(m_Device, m_NullImageViewCube, nullptr);
        vkDestroyImageView(m_Device, m_NullImageViewCubeArray, nullptr);
        vkDestroyImageView(m_Device, m_NullImageView3D, nullptr);
        vkDestroySampler(m_Device, m_NullSampler, nullptr);

        for (VkSampler sample : m_ImmutableSamplers)
        {
            vkDestroySampler(m_Device, sample, nullptr);
        }

        if (m_PipelineCache != VK_NULL_HANDLE)
        {
            // Get Count of pipeline cache
            uint64 Count{};
            res = vkGetPipelineCacheData(m_Device, m_PipelineCache, &Count, nullptr);
            ENGINE_ASSERT(res == VK_SUCCESS);

            // Get Get of pipeline cache
            List<uint8> Get(Count);
			Get.Resize(Count);
            res = vkGetPipelineCacheData(m_Device, m_PipelineCache, &Count, Get.Get());
            ENGINE_ASSERT(res == VK_SUCCESS);

            // // Write pipeline cache Get to a file in binary format
            // std::string cachePath = GetCachePath();
            // wi::helper::FileWrite(cachePath, Get.Get(), Count);

            // Destroy Vulkan pipeline cache
            vkDestroyPipelineCache(m_Device, m_PipelineCache, nullptr);
            m_PipelineCache = VK_NULL_HANDLE;
        }

        if (m_DebugMessenger != VK_NULL_HANDLE)
        {
            vkDestroyDebugUtilsMessengerEXT(m_Instance, m_DebugMessenger, nullptr);
        }
    }

    bool GPUDeviceVulkan::Init()
    {
        VkResult res = volkInitialize();
        ENGINE_ASSERT(res == VK_SUCCESS);
        if (res != VK_SUCCESS)
        {
            LOG_ERROR("RHI", nullptr, "volkInitialize failed! ERROR: {0}", StringUtils::ToString(res));
            return false;
        }

        // SetEnvironmentVariableA("VK_LAYER_PATH", ENGINE_STR(ELECTRON_VK_LAYER_PATH));
        // SetEnvironmentVariableA("DISABLE_LAYER_AMD_SWITCHABLE_GRAPHICS_1", "1");

        CreateInstance(m_GPUSetting.m_ValidationMode);
        CreateDevice(m_GPUSetting.m_GPUPreference);
        CreateCommandQueue();
        CreateAllocation();

        // Create frame resources:
        for (uint32 fr = 0; fr < BUFFER_COUNT; ++fr)
        {
            for (int queue = 0; queue < static_cast<uint32>(RHIQueueType::RHIQueueType_Count); ++queue)
            {
                if (m_CommandQueues[queue].queue == VK_NULL_HANDLE)
                    continue;

                VkFenceCreateInfo fenceInfo = {};
                fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
                // fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
                VkResult res = vkCreateFence(m_Device, &fenceInfo, nullptr, &m_FrameFence[fr][queue]);
                ENGINE_ASSERT(res == VK_SUCCESS);
                if (res != VK_SUCCESS)
                {
                    LOG_ERROR("RHI", nullptr, "vkCreateFence[FRAME] failed! ERROR: {0}", StringUtils::ToString(res));
                    return false;
                }
            }
        }

        CreateDefaultResources();

		instance = reinterpret_cast<GPUDevice*>(this);
        return true;
    }

    void GPUDeviceVulkan::CreateInstance(RHIValidationMode validationMode)
    {
        // Enumerate available layers and extensions:
        uint32 instanceLayerCount;
        VkResult res = vkEnumerateInstanceLayerProperties(&instanceLayerCount, nullptr);
        ENGINE_ASSERT(res == VK_SUCCESS);
        List<VkLayerProperties> availableInstanceLayers(instanceLayerCount);
		availableInstanceLayers.Resize(instanceLayerCount);
        res = vkEnumerateInstanceLayerProperties(&instanceLayerCount, availableInstanceLayers.Get());
        ENGINE_ASSERT(res == VK_SUCCESS);

        uint32 extensionCount = 0;
        res = vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
        ENGINE_ASSERT(res == VK_SUCCESS);
        List<VkExtensionProperties> availableInstanceExtensions(extensionCount);
		availableInstanceExtensions.Resize(extensionCount);
        res = vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableInstanceExtensions.Get());
        ENGINE_ASSERT(res == VK_SUCCESS);

        List<const char *> instanceLayers;
        List<const char *> instanceExtensions;

        for (auto &availableExtension : availableInstanceExtensions)
        {
            if (StringUtils::Compare(availableExtension.extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0)
            {
                debugUtils = true;
                instanceExtensions.Add(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            }
            else if (StringUtils::Compare(availableExtension.extensionName, VK_EXT_SWAPCHAIN_COLOR_SPACE_EXTENSION_NAME) == 0)
            {
                instanceExtensions.Add(VK_EXT_SWAPCHAIN_COLOR_SPACE_EXTENSION_NAME);
            }
        }

        instanceExtensions.Add(VK_KHR_SURFACE_EXTENSION_NAME);
        instanceExtensions.Add(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);

#if defined(VK_USE_PLATFORM_WIN32_KHR)
        instanceExtensions.Add(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#endif // _WIN32

        if (validationMode != RHIValidationMode::Disabled)
        {
            // Determine the optimal validation layers to enable that are necessary for useful debugging
            static const List<const char *> validationLayerPriorityList[] =
                {
                    // The preferred validation layer is "VK_LAYER_KHRONOS_validation"
                    {"VK_LAYER_KHRONOS_validation"},

                    // Otherwise we fallback to using the LunarG meta layer
                    {"VK_LAYER_LUNARG_standard_validation"},

                    // Otherwise we attempt to enable the individual layers that compose the LunarG meta layer since it doesn't exist
                    {
                        "VK_LAYER_GOOGLE_threading",
                        "VK_LAYER_LUNARG_parameter_validation",
                        "VK_LAYER_LUNARG_object_tracker",
                        "VK_LAYER_LUNARG_core_validation",
                        "VK_LAYER_GOOGLE_unique_objects",
                    },

                    // Otherwise as a last resort we fallback to attempting to enable the LunarG core layer
                    {"VK_LAYER_LUNARG_core_validation"}};

            for (auto &validationLayers : validationLayerPriorityList)
            {
                if (ValidateLayers(validationLayers, availableInstanceLayers))
                {
                    for (auto &x : validationLayers)
                    {
                        instanceLayers.Add(x);
                    }
                    break;
                }
            }
        }

        // app info
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "SGERenderer";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "SE.h";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_3;

        // instance info
        VkInstanceCreateInfo instance_create_info{};
        instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instance_create_info.pApplicationInfo = &appInfo;
        instance_create_info.enabledLayerCount = static_cast<uint32>(instanceLayers.Count());
        instance_create_info.ppEnabledLayerNames = instanceLayers.Get();
        instance_create_info.enabledExtensionCount = static_cast<uint32>(instanceExtensions.Count());
        instance_create_info.ppEnabledExtensionNames = instanceExtensions.Get();
        instance_create_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;

        VkDebugUtilsMessengerCreateInfoEXT debugUtilsCreateInfo = {VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};

        if (validationMode != RHIValidationMode::Disabled && debugUtils)
        {
            debugUtilsCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
            debugUtilsCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

            if (validationMode == RHIValidationMode::Verbose)
            {
                debugUtilsCreateInfo.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
                debugUtilsCreateInfo.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
            }

            debugUtilsCreateInfo.pfnUserCallback = DebugUtilsMessengerCallback;
            instance_create_info.pNext = &debugUtilsCreateInfo;
        }

        res = vkCreateInstance(&instance_create_info, nullptr, &m_Instance);
        ENGINE_ASSERT(res == VK_SUCCESS);
        if (res != VK_SUCCESS)
        {
            LOG_FATAL_ERROR("RHI", nullptr, "vkCreateInstance failed! ERROR: {0}", res);
            return;
        }

        volkLoadInstanceOnly(m_Instance);

        if (validationMode != RHIValidationMode::Disabled && debugUtils)
        {
            res = vkCreateDebugUtilsMessengerEXT(m_Instance, &debugUtilsCreateInfo, nullptr, &m_DebugMessenger);
			//_vkCmdBeginDebugUtilsLabelEXT =
            //    (PFN_vkCmdBeginDebugUtilsLabelEXT)vkGetInstanceProcAddr(m_Instance, "vkCmdBeginDebugUtilsLabelEXT");
            // _vkCmdEndDebugUtilsLabelEXT =
            //     (PFN_vkCmdEndDebugUtilsLabelEXT)vkGetInstanceProcAddr(m_Instance, "vkCmdEndDebugUtilsLabelEXT");
            ENGINE_ASSERT(res == VK_SUCCESS);
        }
    }

    void GPUDeviceVulkan::CreateDevice(RHIGPUPreference gpuPreference)
    {
        // PhysicalDevice
        uint32 physical_m_Device_count;
        VkResult res = vkEnumeratePhysicalDevices(m_Instance, &physical_m_Device_count, nullptr);

        if (physical_m_Device_count == 0)
        {
            ENGINE_ASSERT(res == VK_SUCCESS);
            // Platform::WindowMessageBox("Failed to find GPU with Vulkan support!", "Error!");
            // Platform::Exit();
        }

        // find one m_Device that matches our requirement
        // or find which is the best
        List<VkPhysicalDevice> physical_m_Devices(physical_m_Device_count);
		physical_m_Devices.Resize(physical_m_Device_count);
        res = vkEnumeratePhysicalDevices(m_Instance, &physical_m_Device_count, physical_m_Devices.Get());
        ENGINE_ASSERT(res == VK_SUCCESS);

        const List<const char *> required_m_DeviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        };
        List<const char *> enabled_m_DeviceExtensions;

        bool suitable = false;
        bool h264_decode_extension = false;

        auto CheckPhysicalDeviceAndFillProperties2 = [&](VkPhysicalDevice dev)
        {
            suitable = true;

            uint32 extensionCount;
            VkResult res = vkEnumerateDeviceExtensionProperties(dev, nullptr, &extensionCount, nullptr);
            ENGINE_ASSERT(res == VK_SUCCESS);

            List<VkExtensionProperties> available_m_DeviceExtensions(extensionCount);
		  	available_m_DeviceExtensions.Resize(extensionCount);
            res = vkEnumerateDeviceExtensionProperties(dev, nullptr, &extensionCount, available_m_DeviceExtensions.Get());
            ENGINE_ASSERT(res == VK_SUCCESS);


            for (auto &extensions : required_m_DeviceExtensions)
            {
                if (!CheckExtensionSupport(extensions, available_m_DeviceExtensions))
                {
                    suitable = false;
                }
            }

            if (!suitable)
            {
                return;
            }

            h264_decode_extension = false;

            features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
            features_1_1.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
            features_1_2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
            features_1_3.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
            features2.pNext = &features_1_1;
            features_1_1.pNext = &features_1_2;
            features_1_2.pNext = &features_1_3;
            void **features_chain = &features_1_3.pNext;
            acceleration_structure_features = {};
            raytracing_features = {};
            raytracing_query_features = {};
            fragment_shading_rate_features = {};
            mesh_shader_features = {};
            conditional_rendering_features = {};
            depth_clip_enable_features = {};

            properties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
            properties_1_1.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES;
            properties_1_2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES;
            properties_1_3.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_PROPERTIES;
            properties2.pNext = &properties_1_1;
            properties_1_1.pNext = &properties_1_2;
            properties_1_2.pNext = &properties_1_3;
            void **properties_chain = &properties_1_3.pNext;
            sampler_minmax_properties = {};
            acceleration_structure_properties = {};
            raytracing_properties = {};
            fragment_shading_rate_properties = {};
            mesh_shader_properties = {};

            sampler_minmax_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SAMPLER_FILTER_MINMAX_PROPERTIES;
            *properties_chain = &sampler_minmax_properties;
            properties_chain = &sampler_minmax_properties.pNext;

            depth_stencil_resolve_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_STENCIL_RESOLVE_PROPERTIES;
            *properties_chain = &depth_stencil_resolve_properties;
            properties_chain = &depth_stencil_resolve_properties.pNext;

            enabled_m_DeviceExtensions = required_m_DeviceExtensions;

            if (CheckExtensionSupport(VK_EXT_DEPTH_CLIP_ENABLE_EXTENSION_NAME, available_m_DeviceExtensions))
            {
                enabled_m_DeviceExtensions.Add(VK_EXT_DEPTH_CLIP_ENABLE_EXTENSION_NAME);
                depth_clip_enable_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_CLIP_ENABLE_FEATURES_EXT;
                *features_chain = &depth_clip_enable_features;
                features_chain = &depth_clip_enable_features.pNext;
            }
            if (CheckExtensionSupport(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME, available_m_DeviceExtensions))
            {
                enabled_m_DeviceExtensions.Add(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
                ENGINE_ASSERT(CheckExtensionSupport(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME, available_m_DeviceExtensions));
                enabled_m_DeviceExtensions.Add(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
                acceleration_structure_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
                *features_chain = &acceleration_structure_features;
                features_chain = &acceleration_structure_features.pNext;
                acceleration_structure_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR;
                *properties_chain = &acceleration_structure_properties;
                properties_chain = &acceleration_structure_properties.pNext;

                if (CheckExtensionSupport(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME, available_m_DeviceExtensions))
                {
                    enabled_m_DeviceExtensions.Add(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
                    enabled_m_DeviceExtensions.Add(VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME);
                    raytracing_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
                    *features_chain = &raytracing_features;
                    features_chain = &raytracing_features.pNext;
                    raytracing_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
                    *properties_chain = &raytracing_properties;
                    properties_chain = &raytracing_properties.pNext;
                }

                if (CheckExtensionSupport(VK_KHR_RAY_QUERY_EXTENSION_NAME, available_m_DeviceExtensions))
                {
                    enabled_m_DeviceExtensions.Add(VK_KHR_RAY_QUERY_EXTENSION_NAME);
                    raytracing_query_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;
                    *features_chain = &raytracing_query_features;
                    features_chain = &raytracing_query_features.pNext;
                }
            }

            if (CheckExtensionSupport(VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME, available_m_DeviceExtensions))
            {
                enabled_m_DeviceExtensions.Add(VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME);
                fragment_shading_rate_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_FEATURES_KHR;
                *features_chain = &fragment_shading_rate_features;
                features_chain = &fragment_shading_rate_features.pNext;
                fragment_shading_rate_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_PROPERTIES_KHR;
                *properties_chain = &fragment_shading_rate_properties;
                properties_chain = &fragment_shading_rate_properties.pNext;
            }

            if (CheckExtensionSupport(VK_EXT_MESH_SHADER_EXTENSION_NAME, available_m_DeviceExtensions))
            {
                enabled_m_DeviceExtensions.Add(VK_EXT_MESH_SHADER_EXTENSION_NAME);
                mesh_shader_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT;
                *features_chain = &mesh_shader_features;
                features_chain = &mesh_shader_features.pNext;
                mesh_shader_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_PROPERTIES_EXT;
                *properties_chain = &mesh_shader_properties;
                properties_chain = &mesh_shader_properties.pNext;
            }

            if (CheckExtensionSupport(VK_EXT_CONDITIONAL_RENDERING_EXTENSION_NAME, available_m_DeviceExtensions))
            {
                enabled_m_DeviceExtensions.Add(VK_EXT_CONDITIONAL_RENDERING_EXTENSION_NAME);
                conditional_rendering_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CONDITIONAL_RENDERING_FEATURES_EXT;
                *features_chain = &conditional_rendering_features;
                features_chain = &conditional_rendering_features.pNext;
            }

            if (CheckExtensionSupport(VK_KHR_VIDEO_QUEUE_EXTENSION_NAME, available_m_DeviceExtensions) &&
                CheckExtensionSupport(VK_KHR_VIDEO_DECODE_QUEUE_EXTENSION_NAME, available_m_DeviceExtensions) &&
                CheckExtensionSupport(VK_KHR_VIDEO_DECODE_H264_EXTENSION_NAME, available_m_DeviceExtensions))
            {
                enabled_m_DeviceExtensions.Add(VK_KHR_VIDEO_QUEUE_EXTENSION_NAME);
                enabled_m_DeviceExtensions.Add(VK_KHR_VIDEO_DECODE_QUEUE_EXTENSION_NAME);
                enabled_m_DeviceExtensions.Add(VK_KHR_VIDEO_DECODE_H264_EXTENSION_NAME);
                h264_decode_extension = true;
            }

            *properties_chain = nullptr;
            *features_chain = nullptr;
            vkGetPhysicalDeviceProperties2(dev, &properties2);
        };

        bool properties2_matches_physical_m_Device = false;

        for (const auto &dev : physical_m_Devices)
        {
            properties2_matches_physical_m_Device = false;
            CheckPhysicalDeviceAndFillProperties2(dev);

            if (!suitable)
            {
                continue;
            }

            bool priority = properties2.properties.deviceType == VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
            if (gpuPreference == RHIGPUPreference::Integrated)
            {
                priority = properties2.properties.deviceType == VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU;
            }
            if (priority || m_PhysicalDevice == VK_NULL_HANDLE)
            {
                m_PhysicalDevice = dev;
                properties2_matches_physical_m_Device = true;
                if (priority)
                {
					// 如果这是优先的GPU类型，不要再看了
                    break;
                }
            }
        }

        if (m_PhysicalDevice == VK_NULL_HANDLE)
        {
            ENGINE_ASSERT(0);
            // Platform::WindowMessageBox("Failed to find a suitable GPU!", "Error!");
            // Platform::Exit();
        }

        ENGINE_ASSERT(properties2.properties.limits.timestampComputeAndGraphics == VK_TRUE);

        vkGetPhysicalDeviceFeatures2(m_PhysicalDevice, &features2);

        ENGINE_ASSERT(features2.features.imageCubeArray == VK_TRUE);
        ENGINE_ASSERT(features2.features.independentBlend == VK_TRUE);
        ENGINE_ASSERT(features2.features.geometryShader == VK_TRUE);
        ENGINE_ASSERT(features2.features.samplerAnisotropy == VK_TRUE);
        ENGINE_ASSERT(features2.features.shaderClipDistance == VK_TRUE);
        ENGINE_ASSERT(features2.features.textureCompressionBC == VK_TRUE);
        ENGINE_ASSERT(features2.features.occlusionQueryPrecise == VK_TRUE);
        ENGINE_ASSERT(features_1_2.descriptorIndexing == VK_TRUE);
        ENGINE_ASSERT(features_1_3.dynamicRendering == VK_TRUE);

        vendorId = properties2.properties.vendorID;
        deviceId = properties2.properties.deviceID;
        adapterName = properties2.properties.deviceName;

        driverDescription = properties_1_2.driverName;
        if (properties_1_2.driverInfo[0] != '\0')
        {
            driverDescription += ": ";
			driverDescription += properties_1_2.driverInfo;
        }

        switch (properties2.properties.deviceType)
        {
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
            adapterType = RHIAdapterType::IntegratedGpu;
            break;
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
            adapterType = RHIAdapterType::DiscreteGpu;
            break;
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
            adapterType = RHIAdapterType::VirtualGpu;
            break;
        case VK_PHYSICAL_DEVICE_TYPE_CPU:
            adapterType = RHIAdapterType::Cpu;
            break;
        default:
            adapterType = RHIAdapterType::Other;
            break;
        }

        CheckFeatures();

        // Find queue families:
        uint32 queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties2(m_PhysicalDevice, &queueFamilyCount, nullptr);

		m_QueueFamilies.Resize(queueFamilyCount);
		for (uint32 i = 0; i < queueFamilyCount; ++i)
		{
			VkQueueFamilyProperties2& properties21 = m_QueueFamilies[i];
			properties21.sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2;
			properties21.pNext = nullptr;
		}

        vkGetPhysicalDeviceQueueFamilyProperties2(m_PhysicalDevice, &queueFamilyCount, m_QueueFamilies.Get());

        // Query base queue families:
        for (uint32 i = 0; i < queueFamilyCount; ++i)
        {
            auto &queueFamily = m_QueueFamilies[i];

            if (m_GraphicsFamily == VK_QUEUE_FAMILY_IGNORED &&
                queueFamily.queueFamilyProperties.queueCount > 0 &&
                queueFamily.queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                m_GraphicsFamily = i;
                if (queueFamily.queueFamilyProperties.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT)
                {
                    m_CommandQueues[static_cast<uint32>(RHIQueueType::RHIQueueType_Graphics)].sparse_binding_supported = true;
                }
            }

            if (m_CopyFamily == VK_QUEUE_FAMILY_IGNORED &&
                queueFamily.queueFamilyProperties.queueCount > 0 &&
                queueFamily.queueFamilyProperties.queueFlags & VK_QUEUE_TRANSFER_BIT)
            {
                m_CopyFamily = i;
            }

            if (m_ComputeFamily == VK_QUEUE_FAMILY_IGNORED &&
                queueFamily.queueFamilyProperties.queueCount > 0 &&
                queueFamily.queueFamilyProperties.queueFlags & VK_QUEUE_COMPUTE_BIT)
            {
                m_ComputeFamily = i;
            }
        }

        // Now try to find dedicated compute and transfer queues:
        for (uint32 i = 0; i < queueFamilyCount; ++i)
        {
            auto &queueFamily = m_QueueFamilies[i];

            if (queueFamily.queueFamilyProperties.queueCount > 0 &&
                queueFamily.queueFamilyProperties.queueFlags & VK_QUEUE_TRANSFER_BIT &&
                !(queueFamily.queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
                !(queueFamily.queueFamilyProperties.queueFlags & VK_QUEUE_COMPUTE_BIT))
            {
                m_CopyFamily = i;

                if (queueFamily.queueFamilyProperties.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT)
                {
                    m_CommandQueues[static_cast<uint32>(RHIQueueType::RHIQueueType_Copy)].sparse_binding_supported = true;
                }
            }

            if (queueFamily.queueFamilyProperties.queueCount > 0 &&
                queueFamily.queueFamilyProperties.queueFlags & VK_QUEUE_COMPUTE_BIT &&
                !(queueFamily.queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT))
            {
                m_ComputeFamily = i;

                if (queueFamily.queueFamilyProperties.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT)
                {
                    m_CommandQueues[static_cast<uint32>(RHIQueueType::RHIQueueType_Compute)].sparse_binding_supported = true;
                }
            }
        }

        // Now try to find dedicated transfer queue with only transfer and sparse flags:
        //	(This is a workaround for a driver bug with sparse updating from transfer queue)
        for (uint32 i = 0; i < queueFamilyCount; ++i)
        {
            auto &queueFamily = m_QueueFamilies[i];

            if (queueFamily.queueFamilyProperties.queueCount > 0 && queueFamily.queueFamilyProperties.queueFlags == (VK_QUEUE_TRANSFER_BIT | VK_QUEUE_SPARSE_BINDING_BIT))
            {
                m_CopyFamily = i;
                m_CommandQueues[static_cast<uint32>(RHIQueueType::RHIQueueType_Copy)].sparse_binding_supported = true;
            }
        }

        List<VkDeviceQueueCreateInfo> queueCreateInfos;
        HashSet<uint32> uniqueQueueFamilies = {m_GraphicsFamily, m_CopyFamily, m_ComputeFamily};

        float queuePriority = 1.0f;
		for(uint32 queueFamily : uniqueQueueFamilies)
		{
			VkDeviceQueueCreateInfo queueCreateInfo = {};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.Add(queueCreateInfo);
			m_Families.Add(queueFamily);
		}

        VkDeviceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.queueCreateInfoCount = (uint32)queueCreateInfos.Count();
        createInfo.pQueueCreateInfos = queueCreateInfos.Get();
        createInfo.pEnabledFeatures = nullptr;
        createInfo.pNext = &features2;
        createInfo.enabledExtensionCount = static_cast<uint32>(enabled_m_DeviceExtensions.Count());
        createInfo.ppEnabledExtensionNames = enabled_m_DeviceExtensions.Get();

        res = vkCreateDevice(m_PhysicalDevice, &createInfo, nullptr, &m_Device);
        ENGINE_ASSERT(res == VK_SUCCESS);
        if (res != VK_SUCCESS)
        {
            LOG_FATAL_ERROR("RHI", nullptr, "vkCreateDevice failed! ERROR: {0}", res);
            return;
        }

        volkLoadDevice(m_Device);

        // failed to find suitable physical m_Device
        ENGINE_ASSERT(m_PhysicalDevice != VK_NULL_HANDLE);
    }

    void GPUDeviceVulkan::CheckFeatures()
    {
        // 是否支持曲面细分
        if (features2.features.tessellationShader == VK_TRUE)
        {
            capabilities.SetFlag(RHIGraphicsDeviceCapabilityFlag::Tessellation);
        }

        // 指定是否支持所有的“存储镜像扩展格式
        if (features2.features.shaderStorageImageExtendedFormats == VK_TRUE)
        {
            capabilities.SetFlag(RHIGraphicsDeviceCapabilityFlag::Uav_Load_Format_Views);
        }

        if (features_1_2.shaderOutputLayer == VK_TRUE && features_1_2.shaderOutputViewportIndex)
        {
            capabilities.SetFlag(RHIGraphicsDeviceCapabilityFlag::RenderTarget_Add_Viewport_Arrayindex_Without_GS);
        }

        // 是否支持光线追踪
        if (raytracing_features.rayTracingPipeline == VK_TRUE &&
            raytracing_query_features.rayQuery == VK_TRUE &&
            acceleration_structure_features.accelerationStructure == VK_TRUE &&
            features_1_2.bufferDeviceAddress == VK_TRUE)
        {
            capabilities.SetFlag(RHIGraphicsDeviceCapabilityFlag::RayTracting);
            m_Shader_Identifier_Size = raytracing_properties.shaderGroupHandleSize;
        }

        // 是否支持MeshSHader
        if (mesh_shader_features.meshShader == VK_TRUE && mesh_shader_features.taskShader == VK_TRUE)
        {
            capabilities.SetFlag(RHIGraphicsDeviceCapabilityFlag::MeshShader);
        }
        //
        if (fragment_shading_rate_features.pipelineFragmentShadingRate == VK_TRUE)
        {
            capabilities.SetFlag(RHIGraphicsDeviceCapabilityFlag::Variable_Rate_Shading);
        }

        if (fragment_shading_rate_features.attachmentFragmentShadingRate == VK_TRUE)
        {
            capabilities.SetFlag(RHIGraphicsDeviceCapabilityFlag::Variable_Rate_Shading_Attachment);
            m_Variable_Rate_Shading_Tile_size = std::min(fragment_shading_rate_properties.maxFragmentShadingRateAttachmentTexelSize.width, fragment_shading_rate_properties.maxFragmentShadingRateAttachmentTexelSize.height);
        }

        VkFormatProperties formatProperties = {};
        vkGetPhysicalDeviceFormatProperties(m_PhysicalDevice, VulkanNative::ConvertFormat(RHIFormat::R11G11B10_Float), &formatProperties);
        if (formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT)
        {
            capabilities.SetFlag(RHIGraphicsDeviceCapabilityFlag::Uav_Load_Format_R11G11B10_Float);
        }

        if (conditional_rendering_features.conditionalRendering == VK_TRUE)
        {
            capabilities.SetFlag(RHIGraphicsDeviceCapabilityFlag::Predication);
        }

        if (features_1_2.samplerFilterMinmax == VK_TRUE)
        {
            *//*
            指示实现是否支持支持最小/最大过滤的最小所需格式集，
            如 filterMinmaxSingleComponentFormats 属性最低要求所定义。
            如果未启用此功能，则 VkSamplerReductionModeCreateInfo 必须仅使用 VK_SAMPLER_REDUCTION_MODE_WEIGHTED_AVERAGE 。
            *//*
            capabilities.SetFlag(RHIGraphicsDeviceCapabilityFlag::Sampler_Minmax);
        }

        if (features2.features.depthBounds == VK_TRUE)
        {
            *//*
            指定是否支持深度边界测试。如果未启用此功能，则 VkPipelineDepthStencilStateCreateInfo 结构的 depthBoundsTestEnable 成员必须设置为 VK_FALSE 。
            当 depthBoundsTestEnable 设置为 VK_FALSE 时，VkPipelineDepthStencilStateCreateInfo 结构的 minDepthBounds 和 maxDepthBounds 成员将被忽略。
            *//*
            capabilities.SetFlag(RHIGraphicsDeviceCapabilityFlag::Depth_Bounds_Test);
        }

        if (features2.features.sparseBinding == VK_TRUE && features2.features.sparseResidencyAliased == VK_TRUE)
        {
            if (properties2.properties.sparseProperties.residencyNonResidentStrict == VK_TRUE)
            {
                capabilities.SetFlag(RHIGraphicsDeviceCapabilityFlag::Sparse_Null_Mapping);
            }
            if (features2.features.sparseResidencyBuffer == VK_TRUE)
            {
                capabilities.SetFlag(RHIGraphicsDeviceCapabilityFlag::Sparse_Buffer);
            }
            if (features2.features.sparseResidencyImage2D == VK_TRUE)
            {
                capabilities.SetFlag(RHIGraphicsDeviceCapabilityFlag::Sparse_Texture2D);
            }
            if (features2.features.sparseResidencyImage3D == VK_TRUE)
            {
                capabilities.SetFlag(RHIGraphicsDeviceCapabilityFlag::Sparse_Texture3D);
            }

            capabilities.SetFlag(RHIGraphicsDeviceCapabilityFlag::Generic_Sparse_Tile_Pool);
        }

        if ((depth_stencil_resolve_properties.supportedDepthResolveModes & VK_RESOLVE_MODE_MIN_BIT) &&
            (depth_stencil_resolve_properties.supportedDepthResolveModes & VK_RESOLVE_MODE_MAX_BIT))
        {
            capabilities.SetFlag(RHIGraphicsDeviceCapabilityFlag::Depth_Resolve_Min_Max);
        }

        if ((depth_stencil_resolve_properties.supportedStencilResolveModes & VK_RESOLVE_MODE_MIN_BIT) &&
            (depth_stencil_resolve_properties.supportedStencilResolveModes & VK_RESOLVE_MODE_MAX_BIT))
        {
            capabilities.SetFlag(RHIGraphicsDeviceCapabilityFlag::Stencil_Resolve_Min_Max);
        }

        memory_properties_2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2;
        vkGetPhysicalDeviceMemoryProperties2(m_PhysicalDevice, &memory_properties_2);

        if (memory_properties_2.memoryProperties.memoryHeapCount == 1 &&
            memory_properties_2.memoryProperties.memoryHeaps[0].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
        {
            // https://registry.khronos.org/vulkan/specs/1.0-extensions/html/vkspec.html#memory-m_Device
            //	"In a unified memory architecture (UMA) system there is often only a single memory heap which is
            //	considered to be equally “local” to the host and to the m_Device, and such an implementation must advertise the heap as m_Device-local."
            capabilities.SetFlag(RHIGraphicsDeviceCapabilityFlag::Cache_Coherent_UMA);
        }
    }

    void GPUDeviceVulkan::CreateCommandQueue()
    {
        vkGetDeviceQueue(m_Device, m_GraphicsFamily, 0, &m_GraphicsQueue);
        vkGetDeviceQueue(m_Device, m_ComputeFamily, 0, &m_ComputeQueue);
        vkGetDeviceQueue(m_Device, m_CopyFamily, 0, &m_CopyQueue);

        VulkanNative::CommandQueue_Vulkan& graphicQueue = m_CommandQueues[static_cast<uint32>(RHIQueueType::RHIQueueType_Graphics)];
        VulkanNative::CommandQueue_Vulkan& conputeQueue = m_CommandQueues[static_cast<uint32>(RHIQueueType::RHIQueueType_Compute)];
        VulkanNative::CommandQueue_Vulkan& copyQueue = m_CommandQueues[static_cast<uint32>(RHIQueueType::RHIQueueType_Copy)];

        graphicQueue.queue = m_GraphicsQueue;
        graphicQueue.locker = CreateRef<std::mutex>();
        conputeQueue.queue = m_ComputeQueue;
        conputeQueue.locker = CreateRef<std::mutex>();
        copyQueue.queue = m_CopyQueue;
        copyQueue.locker = CreateRef<std::mutex>();

        VkSemaphoreTypeCreateInfo timelineCreateInfo = {};
        timelineCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
        timelineCreateInfo.pNext = nullptr;
        timelineCreateInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
        timelineCreateInfo.initialValue = 0;

        VkSemaphoreCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        createInfo.pNext = &timelineCreateInfo;
        createInfo.flags = 0;

        VkResult res = vkCreateSemaphore(m_Device, &createInfo, nullptr, &graphicQueue.semaphore);
        ENGINE_ASSERT(res == VK_SUCCESS);
        if (res != VK_SUCCESS)
        {
            LOG_FATAL_ERROR("RHI", nullptr, "vkCreateSemaphore[GRAPHICS] failed! ERROR: {0}", res);
            return;
        }
        res = vkCreateSemaphore(m_Device, &createInfo, nullptr, &conputeQueue.semaphore);
        ENGINE_ASSERT(res == VK_SUCCESS);
        if (res != VK_SUCCESS)
        {
            LOG_FATAL_ERROR("RHI", nullptr, "vkCreateSemaphore[COMPUTE] failed! ERROR: {0}", res);
            return;
        }
        res = vkCreateSemaphore(m_Device, &createInfo, nullptr, &copyQueue.semaphore);
        ENGINE_ASSERT(res == VK_SUCCESS);
        if (res != VK_SUCCESS)
        {
            LOG_FATAL_ERROR("RHI", nullptr, "vkCreateSemaphore[COPY] failed! ERROR: {0}", res);
            return;
        }
    }

    void GPUDeviceVulkan::CreateAllocation()
    {
        m_Allocationhandler = CreateRef<VulkanNative::AllocationHandler>();
        m_Allocationhandler->device = m_Device;
        m_Allocationhandler->instance = m_Instance;

        // Initialize Vulkan Memory Allocator helper:
        VmaVulkanFunctions vma_vulkan_func{};
        vma_vulkan_func.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
        vma_vulkan_func.vkGetDeviceProcAddr = vkGetDeviceProcAddr;
        vma_vulkan_func.vkGetPhysicalDeviceProperties = vkGetPhysicalDeviceProperties;
        vma_vulkan_func.vkGetPhysicalDeviceMemoryProperties = vkGetPhysicalDeviceMemoryProperties;
        vma_vulkan_func.vkAllocateMemory = vkAllocateMemory;
        vma_vulkan_func.vkFreeMemory = vkFreeMemory;
        vma_vulkan_func.vkMapMemory = vkMapMemory;
        vma_vulkan_func.vkUnmapMemory = vkUnmapMemory;
        vma_vulkan_func.vkFlushMappedMemoryRanges = vkFlushMappedMemoryRanges;
        vma_vulkan_func.vkInvalidateMappedMemoryRanges = vkInvalidateMappedMemoryRanges;
        vma_vulkan_func.vkBindBufferMemory = vkBindBufferMemory;
        vma_vulkan_func.vkBindImageMemory = vkBindImageMemory;
        vma_vulkan_func.vkGetBufferMemoryRequirements = vkGetBufferMemoryRequirements;
        vma_vulkan_func.vkGetImageMemoryRequirements = vkGetImageMemoryRequirements;
        vma_vulkan_func.vkCreateBuffer = vkCreateBuffer;
        vma_vulkan_func.vkDestroyBuffer = vkDestroyBuffer;
        vma_vulkan_func.vkCreateImage = vkCreateImage;
        vma_vulkan_func.vkDestroyImage = vkDestroyImage;
        vma_vulkan_func.vkCmdCopyBuffer = vkCmdCopyBuffer;

        VmaAllocatorCreateInfo allocatorInfo = {};
        allocatorInfo.physicalDevice = m_PhysicalDevice;
        allocatorInfo.device = m_Device;
        allocatorInfo.instance = m_Instance;

        // Core in 1.1
        allocatorInfo.flags =
            VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT |
            VMA_ALLOCATOR_CREATE_KHR_BIND_MEMORY2_BIT;
        vma_vulkan_func.vkGetBufferMemoryRequirements2KHR = vkGetBufferMemoryRequirements2;
        vma_vulkan_func.vkGetImageMemoryRequirements2KHR = vkGetImageMemoryRequirements2;

        if (features_1_2.bufferDeviceAddress)
        {
            allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
            vma_vulkan_func.vkBindBufferMemory2KHR = vkBindBufferMemory2;
            vma_vulkan_func.vkBindImageMemory2KHR = vkBindImageMemory2;
        }
        allocatorInfo.pVulkanFunctions = &vma_vulkan_func;

        VkResult res = vmaCreateAllocator(&allocatorInfo, &m_Allocationhandler->allocator);
        ENGINE_ASSERT(res == VK_SUCCESS);
        if (res != VK_SUCCESS)
        {
            LOG_FATAL_ERROR("RHI", nullptr, "vmaCreateAllocator failed! ERROR: {0}", res);
            return;
        }

        m_CopyAllocator.Init(this);
    }

    void GPUDeviceVulkan::CreateDefaultResources()
    {
        // Create default null descriptors:
        {
            VkBufferCreateInfo bufferInfo = {};
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.size = 4;
            bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
            bufferInfo.flags = 0;

            VmaAllocationCreateInfo allocInfo = {};
            allocInfo.preferredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

            VkResult res = vmaCreateBuffer(m_Allocationhandler->allocator, &bufferInfo, &allocInfo, &m_NullBuffer, &m_NullBufferAllocation, nullptr);
            ENGINE_ASSERT(res == VK_SUCCESS);

            VkBufferViewCreateInfo viewInfo = {};
            viewInfo.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
            viewInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
            viewInfo.range = VK_WHOLE_SIZE;
            viewInfo.buffer = m_NullBuffer;
            res = vkCreateBufferView(m_Device, &viewInfo, nullptr, &m_NullBufferView);
            ENGINE_ASSERT(res == VK_SUCCESS);
        }
        {
            VkImageCreateInfo imageInfo = {};
            imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageInfo.extent.width = 1;
            imageInfo.extent.height = 1;
            imageInfo.extent.depth = 1;
            imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
            imageInfo.arrayLayers = 1;
            imageInfo.mipLevels = 1;
            imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
            imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
            imageInfo.flags = 0;

            VmaAllocationCreateInfo allocInfo = {};
            allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

            imageInfo.imageType = VK_IMAGE_TYPE_1D;
            VkResult res = vmaCreateImage(m_Allocationhandler->allocator, &imageInfo, &allocInfo, &m_NullImage1D, &m_NullImageAllocation1D, nullptr);
            ENGINE_ASSERT(res == VK_SUCCESS);

            imageInfo.imageType = VK_IMAGE_TYPE_2D;
            imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
            imageInfo.arrayLayers = 6;
            res = vmaCreateImage(m_Allocationhandler->allocator, &imageInfo, &allocInfo, &m_NullImage2D, &m_NullImageAllocation2D, nullptr);
            ENGINE_ASSERT(res == VK_SUCCESS);

            imageInfo.imageType = VK_IMAGE_TYPE_3D;
            imageInfo.flags = 0;
            imageInfo.arrayLayers = 1;
            res = vmaCreateImage(m_Allocationhandler->allocator, &imageInfo, &allocInfo, &m_NullImage3D, &m_NullImageAllocation3D, nullptr);
            ENGINE_ASSERT(res == VK_SUCCESS);

            // Transitions:
            {
                VulkanNative::CopyCommand cmd = m_CopyAllocator.Allocate(0);
                VkImageMemoryBarrier2 barrier = {};
                barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
                barrier.oldLayout = imageInfo.initialLayout;
                barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
                barrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
                barrier.srcAccessMask = 0;
                barrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
                barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
                barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                barrier.subresourceRange.baseArrayLayer = 0;
                barrier.subresourceRange.baseMipLevel = 0;
                barrier.subresourceRange.levelCount = 1;
                barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.image = m_NullImage1D;
                barrier.subresourceRange.layerCount = 1;

                VkDependencyInfo dependencyInfo = {};
                dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
                dependencyInfo.imageMemoryBarrierCount = 1;
                dependencyInfo.pImageMemoryBarriers = &barrier;

                vkCmdPipelineBarrier2(cmd.transitionCommandBuffer, &dependencyInfo);

                barrier.image = m_NullImage2D;
                barrier.subresourceRange.layerCount = 6;
                vkCmdPipelineBarrier2(cmd.transitionCommandBuffer, &dependencyInfo);

                barrier.image = m_NullImage3D;
                barrier.subresourceRange.layerCount = 1;
                vkCmdPipelineBarrier2(cmd.transitionCommandBuffer, &dependencyInfo);

                m_CopyAllocator.Submit(cmd);
            }

            VkImageViewCreateInfo viewInfo = {};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = 1;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;

            viewInfo.image = m_NullImage1D;
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_1D;
            res = vkCreateImageView(m_Device, &viewInfo, nullptr, &m_NullImageView1D);
            ENGINE_ASSERT(res == VK_SUCCESS);

            viewInfo.image = m_NullImage1D;
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_1D_ARRAY;
            res = vkCreateImageView(m_Device, &viewInfo, nullptr, &m_NullImageView1DArray);
            ENGINE_ASSERT(res == VK_SUCCESS);

            viewInfo.image = m_NullImage2D;
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            res = vkCreateImageView(m_Device, &viewInfo, nullptr, &m_NullImageView2D);
            ENGINE_ASSERT(res == VK_SUCCESS);

            viewInfo.image = m_NullImage2D;
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
            res = vkCreateImageView(m_Device, &viewInfo, nullptr, &m_NullImageView2DArray);
            ENGINE_ASSERT(res == VK_SUCCESS);

            viewInfo.image = m_NullImage2D;
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
            viewInfo.subresourceRange.layerCount = 6;
            res = vkCreateImageView(m_Device, &viewInfo, nullptr, &m_NullImageViewCube);
            ENGINE_ASSERT(res == VK_SUCCESS);

            viewInfo.image = m_NullImage2D;
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
            viewInfo.subresourceRange.layerCount = 6;
            res = vkCreateImageView(m_Device, &viewInfo, nullptr, &m_NullImageViewCubeArray);
            ENGINE_ASSERT(res == VK_SUCCESS);

            viewInfo.image = m_NullImage3D;
            viewInfo.subresourceRange.layerCount = 1;
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_3D;
            res = vkCreateImageView(m_Device, &viewInfo, nullptr, &m_NullImageView3D);
            ENGINE_ASSERT(res == VK_SUCCESS);
        }

        {
            VkSamplerCreateInfo createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

            VkResult res = vkCreateSampler(m_Device, &createInfo, nullptr, &m_NullSampler);
            ENGINE_ASSERT(res == VK_SUCCESS);
        }

        timestampFrequency = uint64(1.0 / double(properties2.properties.limits.timestampPeriod) * 1000 * 1000 * 1000);

        // Dynamic PSO states:
        m_PsoSynamicStates.Add(VK_DYNAMIC_STATE_VIEWPORT);
        m_PsoSynamicStates.Add(VK_DYNAMIC_STATE_SCISSOR);
        m_PsoSynamicStates.Add(VK_DYNAMIC_STATE_STENCIL_REFERENCE);
        m_PsoSynamicStates.Add(VK_DYNAMIC_STATE_BLEND_CONSTANTS);
        if (CheckCapability(RHIGraphicsDeviceCapabilityFlag::Depth_Bounds_Test))
        {
            m_PsoSynamicStates.Add(VK_DYNAMIC_STATE_DEPTH_BOUNDS);
        }
        if (CheckCapability(RHIGraphicsDeviceCapabilityFlag::Variable_Rate_Shading))
        {
            m_PsoSynamicStates.Add(VK_DYNAMIC_STATE_FRAGMENT_SHADING_RATE_KHR);
        }
        m_PsoSynamicStates.Add(VK_DYNAMIC_STATE_VERTEX_INPUT_BINDING_STRIDE);

        m_DynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        m_DynamicStateInfo.pDynamicStates = m_PsoSynamicStates.Get();
        m_DynamicStateInfo.dynamicStateCount = m_PsoSynamicStates.Count();

        m_DynamicStateInfo_MeshShader.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        m_DynamicStateInfo_MeshShader.pDynamicStates = m_PsoSynamicStates.Get();
        m_DynamicStateInfo_MeshShader.dynamicStateCount = m_PsoSynamicStates.Count() - 1; // don't include VK_DYNAMIC_STATE_VERTEX_INPUT_BINDING_STRIDE for mesh shader

        // Note: limiting descriptors by constant amount is needed, because the bindless sets are bound to multiple slots to match DX12 layout
        //	And binding to multiple slot adds up towards limits, so the limits will be quickly reached for some descriptor types
        //	But not all descriptor types have this problem, like storage buffers that are not bound for multiple slots usually
        //	Ideally, this shouldn't be the case, because Vulkan could have it's own layout in shaders
        const uint32 limit_bindless_descriptors = 100000u;

        if (features_1_2.descriptorBindingSampledImageUpdateAfterBind == VK_TRUE)
        {
            m_Allocationhandler->bindlessSampledImages.Init(m_Device, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, std::min(limit_bindless_descriptors, properties_1_2.maxDescriptorSetUpdateAfterBindSampledImages / 4));
        }
        if (features_1_2.descriptorBindingUniformTexelBufferUpdateAfterBind == VK_TRUE)
        {
            m_Allocationhandler->bindlessUniformTexelBuffers.Init(m_Device, VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, std::min(limit_bindless_descriptors, properties_1_2.maxDescriptorSetUpdateAfterBindSampledImages / 4));
        }
        if (features_1_2.descriptorBindingStorageBufferUpdateAfterBind == VK_TRUE)
        {
            m_Allocationhandler->bindlessStorageBuffers.Init(m_Device, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, properties_1_2.maxDescriptorSetUpdateAfterBindStorageBuffers / 4);
        }
        if (features_1_2.descriptorBindingStorageImageUpdateAfterBind == VK_TRUE)
        {
            m_Allocationhandler->bindlessStorageImages.Init(m_Device, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, std::min(limit_bindless_descriptors, properties_1_2.maxDescriptorSetUpdateAfterBindStorageImages / 4));
        }
        if (features_1_2.descriptorBindingStorageTexelBufferUpdateAfterBind == VK_TRUE)
        {
            m_Allocationhandler->bindlessStorageTexelBuffers.Init(m_Device, VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, std::min(limit_bindless_descriptors, properties_1_2.maxDescriptorSetUpdateAfterBindStorageImages / 4));
        }
        if (features_1_2.descriptorBindingSampledImageUpdateAfterBind == VK_TRUE)
        {
            m_Allocationhandler->bindlessSamplers.Init(m_Device, VK_DESCRIPTOR_TYPE_SAMPLER, 256);
        }

        if (CheckCapability(RHIGraphicsDeviceCapabilityFlag::RayTracting))
        {
            m_Allocationhandler->bindlessAccelerationStructures.Init(m_Device, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 32);
        }

        // Pipeline Cache
        {
            // Try to read pipeline cache file if exists.
            List<uint8> pipelineData;

            *//*//*
            std::string cachePath = GetCachePath();
            if (!wi::helper::FileRead(cachePath, pipelineData))
            {
                pipelineData.clear();
            }

            // Verify cache validation.
            if (!pipelineData.empty())
            {
                uint32 headerLength = 0;
                uint32 cacheHeaderVersion = 0;
                uint32 vendorID = 0;
                uint32 m_DeviceID = 0;
                uint8_t pipelineCacheUUID[VK_UUID_SIZE] = {};

                std::memcpy(&headerLength, (uint8_t *)pipelineData.Get() + 0, 4);
                std::memcpy(&cacheHeaderVersion, (uint8_t *)pipelineData.Get() + 4, 4);
                std::memcpy(&vendorID, (uint8_t *)pipelineData.Get() + 8, 4);
                std::memcpy(&m_DeviceID, (uint8_t *)pipelineData.Get() + 12, 4);
                std::memcpy(pipelineCacheUUID, (uint8_t *)pipelineData.Get() + 16, VK_UUID_SIZE);

                bool badCache = false;

                if (headerLength <= 0)
                {
                    badCache = true;
                }

                if (cacheHeaderVersion != VK_PIPELINE_CACHE_HEADER_VERSION_ONE)
                {
                    badCache = true;
                }

                if (vendorID != properties2.properties.vendorID)
                {
                    badCache = true;
                }

                if (m_DeviceID != properties2.properties.m_DeviceID)
                {
                    badCache = true;
                }

                if (memcmp(pipelineCacheUUID, properties2.properties.pipelineCacheUUID, sizeof(pipelineCacheUUID)) != 0)
                {
                    badCache = true;
                }

                if (badCache)
                {
                    // Don't submit initial cache Get if any version info is incorrect
                    pipelineData.clear();
                }
            }
            *//*
            VkPipelineCacheCreateInfo createInfo{VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO};
            createInfo.initialDataSize = pipelineData.Count();
            createInfo.pInitialData = pipelineData.Get();

            // Create Vulkan pipeline cache
            VkResult res = vkCreatePipelineCache(m_Device, &createInfo, nullptr, &m_PipelineCache);
            ENGINE_ASSERT(res == VK_SUCCESS);
        }

        // Static samplers:
        {
            VkSamplerCreateInfo createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            createInfo.pNext = nullptr;
            createInfo.flags = 0;
            createInfo.compareEnable = false;
            createInfo.compareOp = VK_COMPARE_OP_NEVER;
            createInfo.minLod = 0;
            createInfo.maxLod = FLT_MAX;
            createInfo.mipLodBias = 0;
            createInfo.anisotropyEnable = false;
            createInfo.maxAnisotropy = 0;

            // sampler_linear_clamp:
            createInfo.minFilter = VK_FILTER_LINEAR;
            createInfo.magFilter = VK_FILTER_LINEAR;
            createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            VkResult res = vkCreateSampler(m_Device, &createInfo, nullptr, &m_ImmutableSamplers.AddOne());
            ENGINE_ASSERT(res == VK_SUCCESS);

            // sampler_linear_wrap:
            createInfo.minFilter = VK_FILTER_LINEAR;
            createInfo.magFilter = VK_FILTER_LINEAR;
            createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            res = vkCreateSampler(m_Device, &createInfo, nullptr, &m_ImmutableSamplers.AddOne());
            ENGINE_ASSERT(res == VK_SUCCESS);

            // sampler_linear_mirror:
            createInfo.minFilter = VK_FILTER_LINEAR;
            createInfo.magFilter = VK_FILTER_LINEAR;
            createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
            createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
            createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
            res = vkCreateSampler(m_Device, &createInfo, nullptr, &m_ImmutableSamplers.AddOne());
            ENGINE_ASSERT(res == VK_SUCCESS);

            // sampler_point_clamp:
            createInfo.minFilter = VK_FILTER_NEAREST;
            createInfo.magFilter = VK_FILTER_NEAREST;
            createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
            createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            res = vkCreateSampler(m_Device, &createInfo, nullptr, &m_ImmutableSamplers.AddOne());
            ENGINE_ASSERT(res == VK_SUCCESS);

            // sampler_point_wrap:
            createInfo.minFilter = VK_FILTER_NEAREST;
            createInfo.magFilter = VK_FILTER_NEAREST;
            createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
            createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            res = vkCreateSampler(m_Device, &createInfo, nullptr, &m_ImmutableSamplers.AddOne());
            ENGINE_ASSERT(res == VK_SUCCESS);

            // sampler_point_mirror:
            createInfo.minFilter = VK_FILTER_NEAREST;
            createInfo.magFilter = VK_FILTER_NEAREST;
            createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
            createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
            createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
            createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
            res = vkCreateSampler(m_Device, &createInfo, nullptr, &m_ImmutableSamplers.AddOne());
            ENGINE_ASSERT(res == VK_SUCCESS);

            // sampler_aniso_clamp:
            createInfo.minFilter = VK_FILTER_LINEAR;
            createInfo.magFilter = VK_FILTER_LINEAR;
            createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            createInfo.anisotropyEnable = true;
            createInfo.maxAnisotropy = 16;
            res = vkCreateSampler(m_Device, &createInfo, nullptr, &m_ImmutableSamplers.AddOne());
            ENGINE_ASSERT(res == VK_SUCCESS);

            // sampler_aniso_wrap:
            createInfo.minFilter = VK_FILTER_LINEAR;
            createInfo.magFilter = VK_FILTER_LINEAR;
            createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            createInfo.anisotropyEnable = true;
            createInfo.maxAnisotropy = 16;
            res = vkCreateSampler(m_Device, &createInfo, nullptr, &m_ImmutableSamplers.AddOne());
            ENGINE_ASSERT(res == VK_SUCCESS);

            // sampler_aniso_mirror:
            createInfo.minFilter = VK_FILTER_LINEAR;
            createInfo.magFilter = VK_FILTER_LINEAR;
            createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
            createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
            createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
            createInfo.anisotropyEnable = true;
            createInfo.maxAnisotropy = 16;
            res = vkCreateSampler(m_Device, &createInfo, nullptr, &m_ImmutableSamplers.AddOne());
            ENGINE_ASSERT(res == VK_SUCCESS);

            // sampler_cmp_depth:
            createInfo.minFilter = VK_FILTER_LINEAR;
            createInfo.magFilter = VK_FILTER_LINEAR;
            createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
            createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            createInfo.anisotropyEnable = false;
            createInfo.maxAnisotropy = 0;
            createInfo.compareEnable = true;
            createInfo.compareOp = VK_COMPARE_OP_GREATER_OR_EQUAL;
            createInfo.minLod = 0;
            createInfo.maxLod = 0;
            res = vkCreateSampler(m_Device, &createInfo, nullptr, &m_ImmutableSamplers.AddOne());
            ENGINE_ASSERT(res == VK_SUCCESS);
        }
    }

    void GPUDeviceVulkan::EventBegin(const char *name, RHICommandList cmd)
    {
        if (!debugUtils)
        {
            return;
        }

        VulkanNative::CommandList_Vulkan &commandlist = GetCommandList(cmd);

        VkDebugUtilsLabelEXT label = {VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT};
        label.pLabelName = name;
        label.color[0] = 0.0f;
        label.color[1] = 0.0f;
        label.color[2] = 0.0f;
        label.color[3] = 1.0f;
        vkCmdBeginDebugUtilsLabelEXT(comman dlist.GetCommandBuffer(), &label);
    }

    void GPUDeviceVulkan::EventEnd(RHICommandList cmd)
    {
        if (!debugUtils)
        {
            return;
        }

        VulkanNative::CommandList_Vulkan &commandlist = GetCommandList(cmd);

        vkCmdEndDebugUtilsLabelEXT(commandlist.GetCommandBuffer());
    }

    void GPUDeviceVulkan::SetMarker(const char *name, RHICommandList cmd)
    {
        if (!debugUtils)
        {
            return;
        }

        VulkanNative::CommandList_Vulkan &commandlist = GetCommandList(cmd);

        VkDebugUtilsLabelEXT label{VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT};
        label.pLabelName = name;
        label.color[0] = 0.0f;
        label.color[1] = 0.0f;
        label.color[2] = 0.0f;
        label.color[3] = 1.0f;
        vkCmdInsertDebugUtilsLabelEXT(commandlist.GetCommandBuffer(), &label);
    }*/


} // namespace SE