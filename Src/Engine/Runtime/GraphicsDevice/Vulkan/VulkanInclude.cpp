#include "VulkanInclude.h"

#define VOLK_IMPLEMENTATION
#include <vulkan/volk.h>

#define VMA_IMPLEMENTATION 1
#define VMA_STATIC_VULKAN_FUNCTIONS 1
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0
#include <vulkan/vulkan_mem_alloc.h>