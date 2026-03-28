#pragma once

#ifdef PLATFORM_WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#endif // _WIN32


#define VK_NO_PROTOTYPES
#include "vulkan/vulkan.h"
#include "vulkan/volk.h"
#include "vulkan/vulkan_mem_alloc.h"

// Vulkan API version to target
#ifndef VULKAN_API_VERSION
#define VULKAN_API_VERSION VK_API_VERSION_1_0
#endif

/// <summary>
/// Default amount of frames to wait until resource delete.
/// </summary>
#define VULKAN_RESOURCE_DELETE_SAFE_FRAMES_COUNT 20

#define VULKAN_RESET_QUERY_POOLS 0
#define VULKAN_HASH_POOLS_WITH_LAYOUT_TYPES 1
#define VULKAN_USE_DEBUG_DATA 1

#define VULKAN_BACK_BUFFERS_COUNT 2
#define VULKAN_BACK_BUFFERS_COUNT_MAX 4

#define VK_ENABLE_BARRIERS_BATCHING 0