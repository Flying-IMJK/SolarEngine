#pragma once

#include "Runtime/Core/Thread/Threading.h"

#include "Runtime/Graphics/GPUSwapChain.h"
#include "Runtime/Graphics/Base/GPUEnums.h"
#include "Runtime/Graphics/Textures/GPUTexture.h"
#include "VulkanInclude.h"
#include "GPUDeviceVulkan.h"
#include "GPUTextureVulkan.h"


namespace SE
{
	class AllocationHandler;

	/// <summary>
	/// Represents a Vulkan swap chain back buffer wrapper object.
	/// </summary>
	class BackBufferVulkan : public ResourceOwnerVulkan
	{
	public:
		/// <summary>
		/// The device.
		/// </summary>
		GPUDeviceVulkan* Device;

		/// <summary>
		/// The image acquired semaphore handle.
		/// </summary>
		SemaphoreVulkan* ImageAcquiredSemaphore;

		/// <summary>
		/// The rendering done semaphore handle.
		/// </summary>
		SemaphoreVulkan* RenderingDoneSemaphore;

		/// <summary>
		/// The render target surface handle.
		/// </summary>
		GPUTextureViewVulkan Handle;

	public:
		void Setup(GPUSwapChainVulkan* window, VkImage backbuffer, PixelFormat format, VkExtent3D extent);
		void Release();

	public:
		// [ResourceOwnerVulkan]
		GPUResource* AsGPUResource() const override
		{
			return nullptr;
		}
	};


	class GPUSwapChainVulkan : public GPUResourceVulkan<GPUSwapChain>, public ResourceOwnerVulkan
	{
		friend class GPUContextVulkan;
		friend GPUDeviceVulkan;
	private:
		VkSurfaceKHR _surface;
		VkSwapchainKHR _swapChain;
		int32 _currentImageIndex;
		int32 _semaphoreIndex;
		int32 m_AcquiredImageIndex;
		List<BackBufferVulkan, FixedAllocation<4>> _backBuffers;
		SemaphoreVulkan* _acquiredSemaphore;

	public:
		GPUSwapChainVulkan(GPUDeviceVulkan* device, Window* window);

	public:
		/// <summary>
		/// Gets the Vulkan surface.
		/// </summary>
		/// <returns>The surface object.</returns>
		VkSurfaceKHR GetSurface() const
		{
			return _surface;
		}

		/// <summary>
		/// Gets the Vulkan surface swap chain.
		/// </summary>
		/// <returns>The swap chain object.</returns>
		VkSwapchainKHR GetSwapChain() const
		{
			return _swapChain;
		}
	public:
		enum class Status
		{
			Ok = 0,
			Outdated = -1,
			LostSurface = -2,
		};

		Status Present(QueueVulkan* presentQueue, SemaphoreVulkan* backBufferRenderingDoneSemaphore);

		static int32 DoAcquireImageIndex(GPUSwapChainVulkan* viewport, void* customData);
		static int32 DoPresent(GPUSwapChainVulkan* viewport, void* customData);
		int32 TryPresent(Function<int32(GPUSwapChainVulkan*, void*)> job, void* customData = nullptr, bool skipOnOutOfDate = false);
		int32 AcquireNextImage(SemaphoreVulkan*& outSemaphore);

	private:
		void ReleaseBackBuffer();
		bool CreateSwapChain(uint32 width, uint32 height);

	public:
		// [GPUSwapChain]
		bool IsFullscreen() override;
		void SetFullscreen(bool isFullscreen) override;
		GPUTextureView* GetBackBufferView() override;
		void Present(bool vsync) override;
		bool Resize(uint32 width, uint32 height) override;
		void CopyBackBuffer(GPUContext* context, GPUTexture* dst) override;

		// [ResourceOwnerVulkan]
		GPUResource* AsGPUResource() const override
		{
			return (GPUResource*)this;
		}

	protected:
		// [GPUResourceVulkan]
		void OnReleaseGPU() override;
	};
}
