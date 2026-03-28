#pragma once

#include "Core/Types/Collections/Dictionary.h"
#include "Core/Thread/Threading.h"
#include "Runtime/Graphics/GPUDevice.h"
#include "Runtime/Graphics/Base/GPUResource.h"
#include "Runtime/Graphics/Textures/GPUTextureDescription.h"
#include "VulkanNative.h"
#include "DescriptorSetVulkan.h"
#include "VulkanTool.h"


namespace SE
{
	class GPUSwapChainVulkan;
	class AllocationHandler;
	class CopyAllocator;
	class QueueVulkan;
	class GPUContextVulkan;
	class GPUAdapterVulkan;
	class UniformBufferUploaderVulkan;
	class DescriptorSetLayoutInfoVulkan;
	class PipelineLayoutVulkan;
	class DescriptorPoolsManagerVulkan;

	class GPUDeviceVulkan final : public GPUDevice
	{
		friend GPUContextVulkan;
		friend GPUSwapChainVulkan;
		friend FenceManagerVulkan;
	public:
		static GPUDevice* Create(GPUGlobalSettings settings);

		virtual bool Init() override;
		void InitDeviceLimits(VkPhysicalDevice_T* gpu);
		void InitMemory(VkPhysicalDevice_T* gpu);


		void Dispose() override;

	public:

		GPUDeviceVulkan(GPUGlobalSettings settings, GPUAdapterVulkan* gpuAdapter);
		virtual ~GPUDeviceVulkan();

		void WaitForGPU() override;

		void DrawBegin() override;

		GPUSwapChain* CreateSwapChain(Window* window) override;

		GPUPipelineState* CreatePipelineState() override;

		GPUTexture* CreateTexture(const StringView& name) override;

		GPUShader* CreateShader(const StringView& name) override;

//		virtual GPUTimerQuery* CreateTimerQuery() = 0;

		GPUBuffer* CreateBuffer(const StringView& name) override;

		GPUSampler* CreateSampler() override;

		GPUTimerQuery* CreateTimerQuery() override;

		GPUConstantBuffer* CreateConstantBuffer(uint32 size, const StringView& name) override;

		GPUContext* GetMainContext() override;

		RenderPassVulkan* GetOrCreateRenderPass(RenderTargetLayoutVulkan& layout);
		FramebufferVulkan* GetOrCreateFramebuffer(FramebufferVulkan::Desc& key, VkExtent2D& extent, uint32 layers);
		PipelineLayoutVulkan* GetOrCreateLayout(DescriptorSetLayoutInfoVulkan& key);

		void OnImageViewDestroy(VkImageView imageView);

		/// <summary>
		/// Setups the present queue to be ready for the given window surface.
		/// </summary>
		/// <param name="surface">The surface.</param>
		void SetupPresentQueue(VkSurfaceKHR surface);

		/// <summary>
		/// Finds the closest pixel format that a specific Vulkan device supports.
		/// </summary>
		/// <param name="format">The input format.</param>
		/// <param name="flags">The texture usage flags.</param>
		/// <param name="optimalTiling">If set to <c>true</c> the optimal tiling should be used, otherwise use linear tiling.</param>
		/// <returns>The output format.</returns>
		PixelFormat GetClosestSupportedPixelFormat(PixelFormat format, EnumFlags<GPUTextureFlags> flags, bool optimalTiling);
	private:
		bool IsVkFormatSupported(VkFormat vkFormat, VkFormatFeatureFlags wantedFeatureFlags, bool optimalTiling) const;

	public:
		VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;

		struct OptionalVulkanDeviceExtensions
		{
			uint32 HasKHRMaintenance1 : 1;
			uint32 HasKHRMaintenance2 : 1;
			uint32 HasMirrorClampToEdge : 1;
			uint32 HasEXTValidationCache : 1;
		};

		static void GetInstanceLayersAndExtensions(RHIValidationMode validationMode, List<const char*>& outInstanceExtensions, List<const char*>& outInstanceLayers, bool& outDebugUtils);
		static void GetDeviceExtensionsAndLayers(VkPhysicalDevice gpu, List<const char*>& outDeviceExtensions, List<const char*>& outDeviceLayers);
		static OptionalVulkanDeviceExtensions optionalDeviceExtensions;
		void ParseOptionalDeviceExtensions(const List<const char*>& deviceExtensions);

		inline BufferedQueryPoolVulkan* FindAvailableQueryPool(List<BufferedQueryPoolVulkan*>& pools, VkQueryType queryType)
		{
			// Try to use pool with available space inside
			for (int32 i = 0; i < pools.Count(); i++)
			{
				auto pool = pools.Get()[i];
				if (pool->HasRoom())
					return pool;
			}

			// Create new pool
			enum
			{
				NUM_OCCLUSION_QUERIES_PER_POOL = 4096,
				NUM_TIMESTAMP_QUERIES_PER_POOL = 1024,
			};
			const auto pool = New<BufferedQueryPoolVulkan>(this, queryType == VK_QUERY_TYPE_OCCLUSION ? NUM_OCCLUSION_QUERIES_PER_POOL : NUM_TIMESTAMP_QUERIES_PER_POOL, queryType);
			pools.Add(pool);
			return pool;
		}

		inline BufferedQueryPoolVulkan* FindAvailableTimestampQueryPool()
		{
			return FindAvailableQueryPool(TimestampQueryPools, VK_QUERY_TYPE_TIMESTAMP);
		}


		static VkInstance instance;

		/// <summary>
		/// The Vulkan instance extensions.
		/// </summary>
		static List<const char*> instanceExtensions;

		/// <summary>
		/// The Vulkan instance layers.
		/// </summary>
		static List<const char*> instanceLayers;


		VkDevice device = VK_NULL_HANDLE;
		VkPipelineCache pipelineCache = VK_NULL_HANDLE;
		VmaAllocator allocator = VK_NULL_HANDLE;

		DeferredDeletionQueueVulkan deferredDeletionQueue;
		/// <summary>
		/// The uniform buffers uploader.
		/// </summary>
		UniformBufferUploaderVulkan* uniformBufferUploader = nullptr;
		/// <summary>
		/// The descriptor pools manager.
		/// </summary>
		DescriptorPoolsManagerVulkan* descriptorPoolsManager = nullptr;

		StagingManagerVulkan stagingManager;

		DummyResourcesVulkan dummyResources;

		FenceManagerVulkan fenceManager;

		GPUAdapterVulkan* gpuAdapter;
		/// <summary>
		/// The Vulkan device queues family properties.
		/// </summary>
		List<VkQueueFamilyProperties> queueFamilyProps;

		/// <summary>
		/// The physical device limits.
		/// </summary>
		VkPhysicalDeviceLimits physicalDeviceLimits;

		/// <summary>
		/// The physical device enabled features.
		/// </summary>
		VkPhysicalDeviceFeatures physicalDeviceFeatures;

		List<BufferedQueryPoolVulkan*> TimestampQueryPools;

		/// <summary>
		/// The main Vulkan commands context.
		/// </summary>
		GPUContextVulkan* mainContext = nullptr;

		/// <summary>
		/// The graphics queue.
		/// </summary>
		QueueVulkan* graphicsQueue = nullptr;

		/// <summary>
		/// The compute queue.
		/// </summary>
		QueueVulkan* computeQueue = nullptr;

		/// <summary>
		/// The transfer queue.
		/// </summary>
		QueueVulkan* transferQueue = nullptr;

		/// <summary>
		/// The present queue.
		/// </summary>
		QueueVulkan* presentQueue = nullptr;

	private:
		Dictionary<RenderTargetLayoutVulkan, RenderPassVulkan*> m_RenderPasses;
		Dictionary<FramebufferVulkan::Desc, FramebufferVulkan*> m_Framebuffers;
		Dictionary<DescriptorSetLayoutInfoVulkan, PipelineLayoutVulkan*> m_Layouts;
		CriticalSection m_FenceLock;

	};

} // namespace SE
