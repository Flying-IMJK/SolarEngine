#pragma once
#include "Runtime/Graphics/Base/GPUResource.h"
#include "Runtime/Graphics/Base/GPUBuffer.h"

#include "GPUDeviceVulkan.h"


namespace SE
{
	class GPUBufferVulkan;

	/// <summary>
	/// The buffer view for Vulkan backend.
	/// </summary>
	class GPUBufferViewVulkan : public GPUBufferView, DescriptorResourceVulkan
	{
	public:
		GPUBufferViewVulkan()
		{
		}

	public:
		GPUDeviceVulkan* Device = nullptr;
		GPUBufferVulkan* Owner = nullptr;
		VkBuffer Buffer = VK_NULL_HANDLE;
		VkBufferView View = VK_NULL_HANDLE;
		VkDeviceSize Size = 0;

	public:
		void Init(GPUDeviceVulkan* device, GPUBufferVulkan* owner, VkBuffer buffer, VkDeviceSize size, VkBufferUsageFlags usage, PixelFormat format);
		void Release();

	public:
		// [GPUResourceView]
		void* GetNativePtr() const override
		{
			return (void*)this;
		}

		// [DescriptorOwnerResourceVulkan]
		void AsUniformTexelBuffer(GPUContextVulkan* context, VkBufferView& bufferView) override;
		void AsStorageBuffer(GPUContextVulkan* context, VkBuffer& buffer, VkDeviceSize& offset, VkDeviceSize& range) override;
		void AsStorageTexelBuffer(GPUContextVulkan* context, VkBufferView& bufferView) override;
	};

	class GPUBufferVulkan : public GPUResourceVulkan<GPUBuffer>
	{
	private:
		VkBuffer m_Buffer = VK_NULL_HANDLE;
		VmaAllocation m_Allocation = VK_NULL_HANDLE;
		GPUBufferViewVulkan _view;
	public:
		GPUBufferVulkan(GPUDeviceVulkan* device, const StringView& name)
			: GPUResourceVulkan<GPUBuffer>(device, name)
		{
		}

		/// <summary>
		/// Gets the Vulkan buffer handle.
		/// </summary>
		inline VkBuffer GetHandle() const
		{
			return m_Buffer;
		}

		/// <summary>
		/// Gets the Vulkan memory allocation handle.
		/// </summary>
		inline VmaAllocation GetAllocation() const
		{
			return m_Allocation;
		}

		/// <summary>
		/// The current buffer access flags.
		/// </summary>
		VkAccessFlags Access;

		/// <summary>
		/// The counter buffer attached to the Append/Counter buffers.
		/// </summary>
		GPUBufferVulkan* Counter = nullptr;


	public:
		void* Map(GPUResourceMapMode mode) override;
		void Unmap() override;
		GPUBufferView* View() const override;

		// [GPUBuffer]
		bool OnInit() override;
		void OnReleaseGPU() override;
	};

}
