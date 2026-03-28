#pragma once

#include "Runtime/Graphics/Textures/GPUTexture.h"
#include "GPUDeviceVulkan.h"
#include "ResourceOwnerVulkan.h"
#include "VulkanNative.h"

namespace SE
{
	class GPUDeviceVulkan;
	class GPUBufferVulkan;

	/// <summary>
	/// The texture view for Vulkan backend.
	/// </summary>
	class GPUTextureViewVulkan : public GPUTextureView, public DescriptorResourceVulkan
	{
	public:
		GPUTextureViewVulkan()
		{
		}

		GPUTextureViewVulkan(const GPUTextureViewVulkan& other)
			: GPUTextureViewVulkan()
		{
#if !BUILD_RELEASE
			ENGINE_UNREACHABLE_CODE(); // Not used
#endif
		}

		GPUTextureViewVulkan& operator=(const GPUTextureViewVulkan& other)
		{
#if !BUILD_RELEASE
			ENGINE_UNREACHABLE_CODE(); // Not used
#endif
			return *this;
		}

#if !BUILD_RELEASE
		~GPUTextureViewVulkan()
		{
			ENGINE_ASSERT(view == VK_NULL_HANDLE);
		}
#endif

	public:
		GPUDeviceVulkan* device = nullptr;
		ResourceOwnerVulkan* owner = nullptr;
		VkImage image = VK_NULL_HANDLE;
		VkImageView view = VK_NULL_HANDLE;
		VkImageView viewFramebuffer = VK_NULL_HANDLE;
		VkImageView viewSRV = VK_NULL_HANDLE;
		VkExtent3D extent = {};
		uint32 layers = 0;
		VkImageViewCreateInfo info = {};
		int32 subresourceIndex = 0;
		VkImageLayout layoutRTV = {};
		VkImageLayout layoutSRV = {};
#if VULKAN_USE_DEBUG_DATA
		PixelFormat format = PixelFormat::Undefined;
    	bool readOnlyDepth = 0;
#endif

	public:
		void Init(GPUDeviceVulkan* device, ResourceOwnerVulkan* owner, VkImage image, int32 totalMipLevels, PixelFormat format,
			MSAALevel msaa, VkExtent3D extent, VkImageViewType viewType, int32 mipLevels = 1,
			int32 firstMipIndex = 0, int32 arraySize = 1, int32 firstArraySlice = 0, bool readOnlyDepth = false);

		VkImageView GetFramebufferView();

		void Release();

	public:
		// [GPUResourceView]
		void* GetNativePtr() const override
		{
			return (void*)(DescriptorResourceVulkan*)this;
		}

		// [DescriptorResourceVulkan]
		void AsImage(GPUContextVulkan* context, VkImageView& imageView, VkImageLayout& layout) override;
		void AsStorageImage(GPUContextVulkan* context, VkImageView& imageView, VkImageLayout& layout) override;
	};

	/// <summary>
	/// Texture object for Vulkan backend.
	/// </summary>
	class GPUTextureVulkan : public GPUResourceVulkan<GPUTexture>, public ResourceOwnerVulkan, public DescriptorResourceVulkan
	{
	private:
		VkImage m_Image = VK_NULL_HANDLE;
		VmaAllocation m_Allocation = VK_NULL_HANDLE;
		GPUTextureViewVulkan m_HandleArray;
		GPUTextureViewVulkan m_HandleVolume;
		GPUTextureViewVulkan m_HandleUAV;
		GPUTextureViewVulkan m_HandleReadOnlyDepth;
		List<GPUTextureViewVulkan> m_HandlesPerSlice; // [slice]
		List<List<GPUTextureViewVulkan>> m_HandlesPerMip; // [slice][mip]

	public:
		/// <summary>
		/// Initializes a new instance of the <see cref="GPUTextureVulkan"/> class.
		/// </summary>
		/// <param name="device">The device.</param>
		/// <param name="name">The resource name.</param>
		GPUTextureVulkan(GPUDeviceVulkan* device, const StringView& name)
			: GPUResourceVulkan<GPUTexture>(device, name)
		{
		}

	public:
		/// <summary>
		/// Gets the Vulkan image handle.
		/// </summary>
		inline VkImage GetHandle() const
		{
			return m_Image;
		}

		/// <summary>
		/// The Vulkan staging buffer (used by the staging textures for memory transfers).
		/// </summary>
		GPUBufferVulkan* StagingBuffer = nullptr;

		/// <summary>
		/// The default aspect mask flags for the texture (all planes).
		/// </summary>
		VkImageAspectFlags DefaultAspectMask = VK_IMAGE_ASPECT_NONE;

	private:
		void initHandles();

	public:
		// [GPUTexture]
		GPUTextureView* View(int32 arrayOrDepthIndex) const override
		{
			return (GPUTextureView*)&m_HandlesPerSlice[arrayOrDepthIndex];
		}

		GPUTextureView* View(int32 arrayOrDepthIndex, int32 mipMapIndex) const override
		{
			return (GPUTextureView*)&m_HandlesPerMip[arrayOrDepthIndex][mipMapIndex];
		}

		GPUTextureView* ViewArray() const override
		{
			ENGINE_ASSERT(ArraySize() > 1);
			return (GPUTextureView*)&m_HandleArray;
		}

		GPUTextureView* ViewVolume() const override
		{
			ENGINE_ASSERT(IsVolume());
			return (GPUTextureView*)&m_HandleVolume;
		}

		GPUTextureView* ViewReadOnlyDepth() const override
		{
			ENGINE_ASSERT(m_Desc.Flags.IsFlag(GPUTextureFlags::ReadOnlyDepthView));
			return (GPUTextureView*)&m_HandleReadOnlyDepth;
		}

		void* GetNativePtr() const override
		{
			return (void*)m_Image;
		}

		bool GetData(int32 arrayOrDepthSliceIndex, int32 mipMapIndex, TextureMipData& data, uint32 mipRowPitch) override;

		// [ResourceOwnerVulkan]
		GPUResource* AsGPUResource() const override
		{
			return (GPUResource*)this;
		}

		// [DescriptorResourceVulkan]
		void AsStorageImage(GPUContextVulkan* context, VkImageView& imageView, VkImageLayout& layout) override;

	protected:
		// [GPUTexture]
		bool OnInit() override;
		void OnResidentMipsChanged() override;
		void OnReleaseGPU() override;
	};

} // SE

