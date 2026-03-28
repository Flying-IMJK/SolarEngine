

#include "GPUTextureVulkan.h"
#include "GPUContextVulkan.h"
#include "GPUBufferVulkan.h"
#include "Runtime/Graphics/Base/GPUUtils.h"

namespace SE
{
	void GPUTextureViewVulkan::Init(GPUDeviceVulkan* device, ResourceOwnerVulkan* owner, VkImage image, int32 totalMipLevels, PixelFormat format, MSAALevel msaa, VkExtent3D extent, VkImageViewType viewType, int32 mipLevels, int32 firstMipIndex, int32 arraySize, int32 firstArraySlice, bool readOnlyDepth)
	{
		ENGINE_ASSERT(view == VK_NULL_HANDLE);

		GPUTextureView::Init(owner->AsGPUResource(), format, msaa);

		this->device = device;
		this->owner = owner;
		this->image = image;
		this->extent.width = Math::Max<uint32>(1, extent.width >> firstMipIndex);
		this->extent.height = Math::Max<uint32>(1, extent.height >> firstMipIndex);
		this->extent.depth = Math::Max<uint32>(1, extent.depth >> firstMipIndex);
		this->layers = arraySize;
#if VULKAN_USE_DEBUG_DATA
		this->format = format;
		this->readOnlyDepth = readOnlyDepth;
#endif

		VulkanTool::ZeroStruct(info, VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO);
		info.image = image;
		info.viewType = viewType;
		info.format = VulkanTool::ToVulkanFormat(format);
		info.components =
			{
				VK_COMPONENT_SWIZZLE_R,
				VK_COMPONENT_SWIZZLE_G,
				VK_COMPONENT_SWIZZLE_B,
				VK_COMPONENT_SWIZZLE_A
			};

		auto& range = info.subresourceRange;
		range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		range.baseMipLevel = firstMipIndex;
		range.levelCount = mipLevels;
		range.baseArrayLayer = firstArraySlice;
		range.layerCount = arraySize;

		if (mipLevels != 1 || arraySize != 1)
		{
			subresourceIndex = -1;
		}
		else
		{
			subresourceIndex = GPUUtils::CalcSubresourceIndex(firstMipIndex, firstArraySlice, totalMipLevels);
		}

		if (PixelFormatIsDepthSupport(format) || PixelFormatIsStencilSupport(format))
		{
			range.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
#if 0
			// TODO: enable extension and use separateDepthStencilLayouts from Vulkan 1.2
			if (PixelFormatExtensions::HasStencil(format))
			{
				range.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
				LayoutRTV = readOnlyDepth ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
				LayoutSRV = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
			}
			else
			{
				LayoutRTV = readOnlyDepth ? VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
				LayoutSRV = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL;
			}
#else

			if (PixelFormatIsStencilSupport(format))
				range.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
			layoutRTV = readOnlyDepth ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			layoutSRV = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
#endif
		}
		else
		{
			layoutRTV = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			layoutSRV = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		}

		VALIDATE_VULKAN_RESULT(vkCreateImageView(device->device, &info, nullptr, &view));
	}

	VkImageView GPUTextureViewVulkan::GetFramebufferView()
	{
		if (viewFramebuffer)
			return viewFramebuffer;

		if (info.viewType == VK_IMAGE_VIEW_TYPE_3D)
		{
			// Special case:
			// Render Target Handle to a 3D Volume texture.
			// Use it as Texture2D Array with layers.
			VkImageViewCreateInfo createInfo = info;
			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
			createInfo.subresourceRange.layerCount = extent.depth;
			layers = extent.depth;
			VALIDATE_VULKAN_RESULT(vkCreateImageView(device->device, &createInfo, nullptr, &viewFramebuffer));
		}
		else if (info.subresourceRange.levelCount != 1)
		{
			// Special case:
			// Render Target Handle can be created for full texture including its mip maps but framebuffer image view can use only a single surface
			// Use an additional view for that case with modified level count to 1.
			VkImageViewCreateInfo createInfo = info;
			createInfo.subresourceRange.levelCount = 1;
			VALIDATE_VULKAN_RESULT(vkCreateImageView(device->device, &createInfo, nullptr, &viewFramebuffer));
		}
		else
		{
			viewFramebuffer = view;
		}

		return viewFramebuffer;
	}

	void GPUTextureViewVulkan::Release()
	{
		if (view != VK_NULL_HANDLE)
		{
			if (viewFramebuffer != view && viewFramebuffer != VK_NULL_HANDLE)
			{
				device->OnImageViewDestroy(viewFramebuffer);
				device->deferredDeletionQueue.EnqueueResource(DeferredDeletionQueueVulkan::Type::ImageView, viewFramebuffer);
			}
			viewFramebuffer = VK_NULL_HANDLE;
			if (viewSRV != view && viewSRV != VK_NULL_HANDLE)
			{
				device->OnImageViewDestroy(viewSRV);
				device->deferredDeletionQueue.EnqueueResource(DeferredDeletionQueueVulkan::Type::ImageView, viewSRV);
			}
			viewSRV = VK_NULL_HANDLE;

			device->OnImageViewDestroy(view);
			device->deferredDeletionQueue.EnqueueResource(DeferredDeletionQueueVulkan::Type::ImageView, view);
			view = VK_NULL_HANDLE;

			device = nullptr;
//			Owner = nullptr;
			image = VK_NULL_HANDLE;
		}
	}

	void GPUTextureViewVulkan::AsImage(GPUContextVulkan* context, VkImageView& imageView, VkImageLayout& layout)
	{
		imageView = view;
		layout = layoutSRV;
		const VkImageAspectFlags aspectMask = info.subresourceRange.aspectMask;
		if (aspectMask == (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT))
		{
			// Transition depth-only when binding depth buffer with stencil
			if (viewSRV == VK_NULL_HANDLE)
			{
				VkImageViewCreateInfo createInfo = info;
				createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
				VALIDATE_VULKAN_RESULT(vkCreateImageView(device->device, &createInfo, nullptr, &viewSRV));
			}
			imageView = viewSRV;
		}
		context->AddImageBarrier(this, layoutSRV);
		info.subresourceRange.aspectMask = aspectMask;
	}

	void GPUTextureViewVulkan::AsStorageImage(GPUContextVulkan* context, VkImageView& imageView, VkImageLayout& layout)
	{
		imageView = view;
		layout = VK_IMAGE_LAYOUT_GENERAL;
		context->AddImageBarrier(this, VK_IMAGE_LAYOUT_GENERAL);
	}



	bool GPUTextureVulkan::GetData(int32 arrayOrDepthSliceIndex, int32 mipMapIndex, TextureMipData& data, uint32 mipRowPitch)
	{
		if (!IsStaging())
		{
			LOG_WARNING("Graphic", "GPUTexture::GetData is valid only for staging resources.");
			return true;
		}
		GPUDeviceLock lock(m_Device);

		// Internally it's a buffer, so adapt resource index and offset
		const uint32 subresource = mipMapIndex + arrayOrDepthSliceIndex * MipLevels();
		// TODO: rowAlign/sliceAlign on Vulkan texture ???
		int32 offsetInBytes = ComputeBufferOffset(subresource, 1, 1);
		int32 lengthInBytes = ComputeSubresourceSize(subresource, 1, 1);
		int32 rowPitch = ComputeRowPitch(mipMapIndex, 1);
		int32 depthPicth = ComputeSlicePitch(mipMapIndex, 1);

		// Map the staging resource mip map for reading
		auto allocation = StagingBuffer->GetAllocation();
		void* mapped;
		const VkResult result = vmaMapMemory(m_Device->allocator, allocation, (void**)&mapped);
		LOG_VULKAN_RESULT_WITH_RETURN(result);

		// Shift mapped buffer to the beginning of the mip data start
		mapped = (void*)((byte*)mapped + offsetInBytes);

		// Check if target row pitch is the same
		if (mipRowPitch == rowPitch || mipRowPitch == 0)
		{
			// Init mip info
			data.Lines = depthPicth / rowPitch;
			data.DepthPitch = depthPicth;
			data.RowPitch = rowPitch;

			// Copy data
			data.Data.Copy((byte*)mapped, depthPicth);
		}
		else
		{
			// Init mip info
			data.Lines = depthPicth / rowPitch;
			data.DepthPitch = mipRowPitch * data.Lines;
			data.RowPitch = mipRowPitch;

			// Copy data
			data.Data.Allocate(data.DepthPitch);
			for (uint32 i = 0; i < data.Lines; i++)
			{
				Platform::MemoryCopy(data.Data.Get() + data.RowPitch * i, ((byte*)mapped) + rowPitch * i, data.RowPitch);
			}
		}

		// Unmap resource
		vmaUnmapMemory(m_Device->allocator, allocation);

		return false;
	}

	void GPUTextureVulkan::AsStorageImage(GPUContextVulkan* context, VkImageView& imageView, VkImageLayout& layout)
	{
//		TODO
//		ENGINE_ASSERT(_handleUAV.Owner == this);
		imageView = m_HandleUAV.view;
		layout = VK_IMAGE_LAYOUT_GENERAL;
		context->AddImageBarrier(this, VK_IMAGE_LAYOUT_GENERAL);
	}

	bool GPUTextureVulkan::OnInit()
	{
		// Check if texture should have optimal CPU read/write access
		if (IsStaging())
		{
			// TODO: rowAlign/sliceAlign on Vulkan texture ???
			const int32 totalSize = ComputeBufferTotalSize(1, 1);
			StagingBuffer = (GPUBufferVulkan*)m_Device->CreateBuffer(TEXT("Texture.StagingBuffer"));
			if (!StagingBuffer->Init(GPUBufferDescription::Buffer(totalSize, GPUBufferFlags::None, PixelFormat::Undefined, nullptr, 0, m_Desc.Usage)))
			{
				Delete(StagingBuffer);
				return false;
			}
			m_MemoryUsage = 1;
			return true;
		}

		const bool useSRV = IsShaderResource();
		const bool useDSV = IsDepthStencil();
		const bool useRTV = IsRenderTarget();
		const bool useUAV = IsUnorderedAccess();

		const bool optimalTiling = true;
		PixelFormat format = m_Desc.Format;
		if (useDSV)
			format = FindDepthStencilFormat(format);
		m_Desc.Format = m_Device->GetClosestSupportedPixelFormat(format, m_Desc.Flags, optimalTiling);
		if (m_Desc.Format == PixelFormat::Undefined)
		{
			LOG_ERROR("Graphic", "GPUTextureUnsupported texture format {0}.", PixelFormatGetString(format));
			return false;
		}

		// Setup texture description
		VkImageCreateInfo imageInfo;
		VulkanTool::ZeroStruct(imageInfo, VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO);
		imageInfo.imageType = IsVolume() ? VK_IMAGE_TYPE_3D : VK_IMAGE_TYPE_2D;
		imageInfo.format = VulkanTool::ToVulkanFormat(Format());
		imageInfo.mipLevels = MipLevels();
		imageInfo.arrayLayers = ArraySize();
		imageInfo.extent.width = Width();
		imageInfo.extent.height = Height();
		imageInfo.extent.depth = Depth();
		imageInfo.flags = IsCubeMap() ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0;
		if (IsSRGB())
			imageInfo.flags |= VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;
#if VK_KHR_maintenance1
		if (m_Device->optionalDeviceExtensions.HasKHRMaintenance1 && imageInfo.imageType == VK_IMAGE_TYPE_3D)
			imageInfo.flags |= VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT_KHR;
#endif
		imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		if (useSRV)
			imageInfo.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
		if (useDSV)
			imageInfo.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		if (useRTV)
			imageInfo.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		if (useUAV)
			imageInfo.usage |= VK_IMAGE_USAGE_STORAGE_BIT;
#if PLATFORM_MAC || PLATFORM_IOS
		// MoltenVK: VK_ERROR_FEATURE_NOT_PRESENT: vkCreateImageView(): 2D views on 3D images can only be used as color attachments.
    if (IsVolume() && _desc.HasPerSliceViews())
        imageInfo.usage &= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
#endif
		imageInfo.tiling = optimalTiling ? VK_IMAGE_TILING_OPTIMAL : VK_IMAGE_TILING_LINEAR;
		imageInfo.samples = (VkSampleCountFlagBits)MultiSampleLevel();
		// TODO: set initialLayout to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL for IsRegularTexture() ???

		// Create texture
		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
		const VkResult result = vmaCreateImage(m_Device->allocator, &imageInfo, &allocInfo, &m_Image, &m_Allocation, nullptr);
		LOG_VULKAN_RESULT_WITH_RETURN(result);
#if GPU_ENABLE_RESOURCE_NAMING
		VK_SET_DEBUG_NAME(m_Device, m_Image, VK_OBJECT_TYPE_IMAGE, "" /*GetName()*/);
#endif
		//LOG(Warning, "VULKAN TEXTURE -> 0x{0:x} - {1}", (int)_image, GetName());

		// Set state
		InitResource(VK_IMAGE_LAYOUT_UNDEFINED, m_Desc.MipLevels, m_Desc.ArraySize, true);
		m_MemoryUsage = CalculateMemoryUsage();
		if (PixelFormatIsStencilSupport(format) || PixelFormatIsDepthSupport(format))
		{
			DefaultAspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			if (PixelFormatIsStencilSupport(format))
				DefaultAspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
		else
		{
			DefaultAspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		}

		// Initialize handles to the resource
		if (IsRegularTexture())
		{
			// 'Regular' texture is using only one handle (texture/cubemap)
 			m_HandlesPerSlice.Resize(1, false);
		}
		else
		{
			// Create all handles
			initHandles();
		}

		return true;
	}

	void GPUTextureVulkan::initHandles()
	{
		// Cache properties
		const int32 arraySize = ArraySize();
		const int32 mipLevels = MipLevels();
		const bool isArray = arraySize > 1;
		const bool isCubeMap = IsCubeMap();
		const bool isVolume = IsVolume();
		const auto format = Format();
		const auto msaa = MultiSampleLevel();
		VkExtent3D extent;
		extent.width = Width();
		extent.height = Height();
		extent.depth = Depth();

		// Create resource views
		if (isVolume)
		{
			// Create handle for whole 3d texture
			m_HandleVolume.Init(m_Device, this, m_Image, mipLevels, format, msaa, extent, VK_IMAGE_VIEW_TYPE_3D, mipLevels, 0);

			// Init per slice views
			m_HandlesPerSlice.Resize(Depth(), false);
			if (m_Desc.HasPerSliceViews())
			{
				for (int32 sliceIndex = 0; sliceIndex < Depth(); sliceIndex++)
				{
					m_HandlesPerSlice[sliceIndex].Init(m_Device, this, m_Image, mipLevels, format, msaa, extent, VK_IMAGE_VIEW_TYPE_2D, mipLevels, 0, 1, sliceIndex);
				}
			}
		}
		else if (isArray)
		{
			// Resize handles
			m_HandlesPerSlice.Resize(ArraySize(), false);

			// Create per array slice handles
			for (int32 arrayIndex = 0; arrayIndex < arraySize; arrayIndex++)
			{
				m_HandlesPerSlice[arrayIndex].Init(m_Device, this, m_Image, mipLevels, format, msaa, extent, VK_IMAGE_VIEW_TYPE_2D, mipLevels, 0, 1, arrayIndex);
			}

			// Create whole array handle
			if (isCubeMap)
			{
				m_HandleArray.Init(m_Device, this, m_Image, mipLevels, format, msaa, extent, VK_IMAGE_VIEW_TYPE_CUBE, mipLevels, 0, arraySize, 0);
			}
			else
			{
				m_HandleArray.Init(m_Device, this, m_Image, mipLevels, format, msaa, extent, VK_IMAGE_VIEW_TYPE_2D_ARRAY, mipLevels, 0, arraySize, 0);
			}
		}
		else
		{
			// Create single handle for the whole texture
			m_HandlesPerSlice.Resize(1, false);
			if (isCubeMap)
			{
				m_HandlesPerSlice[0].Init(m_Device, this, m_Image, mipLevels, format, msaa, extent, VK_IMAGE_VIEW_TYPE_CUBE, mipLevels, 0, arraySize);
			}
			else
			{
				m_HandlesPerSlice[0].Init(m_Device, this, m_Image, mipLevels, format, msaa, extent, VK_IMAGE_VIEW_TYPE_2D, mipLevels);
			}
		}

		// Init per mip map handles
		if (HasPerMipViews())
		{
			// Init handles
			m_HandlesPerMip.Resize(arraySize, false);
			for (int32 arrayIndex = 0; arrayIndex < arraySize; arrayIndex++)
			{
				auto& slice = m_HandlesPerMip[arrayIndex];
				slice.Resize(mipLevels, false);

				for (int32 mipIndex = 0; mipIndex < mipLevels; mipIndex++)
				{
					slice[mipIndex].Init(m_Device, this, m_Image, mipLevels, format, msaa, extent, VK_IMAGE_VIEW_TYPE_2D, 1, mipIndex, 1, arrayIndex);
				}
			}
		}

		// UAV texture
		if (IsUnorderedAccess())
		{
			if (isVolume)
			{
				m_HandleUAV.Init(m_Device, this, m_Image, mipLevels, format, msaa, extent, VK_IMAGE_VIEW_TYPE_3D, 1, 0, 1, 0);
			}
			else if (isArray)
			{
				m_HandleUAV.Init(m_Device, this, m_Image, mipLevels, format, msaa, extent, VK_IMAGE_VIEW_TYPE_2D_ARRAY, 1, 0, arraySize, 0);
			}
			else
			{
				m_HandleUAV.Init(m_Device, this, m_Image, mipLevels, format, msaa, extent, VK_IMAGE_VIEW_TYPE_2D, 1, 0, 1, 0);
			}
		}

		// Read-only depth-stencil
		if (m_Desc.Flags.IsFlag(GPUTextureFlags::ReadOnlyDepthView))
		{
			m_HandleReadOnlyDepth.Init(m_Device, this, m_Image, mipLevels, format, msaa, extent, VK_IMAGE_VIEW_TYPE_2D, mipLevels, 0, 1, 0, true);
		}
	}

	void GPUTextureVulkan::OnResidentMipsChanged()
	{
		// Update view
		VkExtent3D extent;
		extent.width = Width();
		extent.height = Height();
		extent.depth = Depth();
		const int32 firstMipIndex = MipLevels() - ResidentMipLevels();
		const int32 mipLevels = ResidentMipLevels();
		const VkImageViewType viewType = IsVolume() ? VK_IMAGE_VIEW_TYPE_3D : (IsCubeMap() ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D);
		GPUTextureViewVulkan& view = IsVolume() ? m_HandleVolume : m_HandlesPerSlice[0];
		view.Release();
		view.Init(m_Device, this, m_Image, mipLevels, Format(), MultiSampleLevel(), extent, viewType, mipLevels, firstMipIndex, ArraySize());
	}

	void GPUTextureVulkan::OnReleaseGPU()
	{
		m_HandleArray.Release();
		m_HandleVolume.Release();
		m_HandleUAV.Release();
		m_HandleReadOnlyDepth.Release();
		for (int32 i = 0; i < m_HandlesPerMip.Count(); i++)
		{
			for (int32 j = 0; j < m_HandlesPerMip[i].Count(); j++)
			{
				m_HandlesPerMip[i][j].Release();
			}
		}
		for (int32 i = 0; i < m_HandlesPerSlice.Count(); i++)
		{
			m_HandlesPerSlice[i].Release();
		}
		m_HandlesPerMip.Resize(0, false);
		m_HandlesPerSlice.Resize(0, false);
		if (m_Image != VK_NULL_HANDLE)
		{
			m_Device->deferredDeletionQueue.EnqueueResource(DeferredDeletionQueueVulkan::Type::Image, m_Image, m_Allocation);
			m_Image = VK_NULL_HANDLE;
			m_Allocation = VK_NULL_HANDLE;
		}
		if (StagingBuffer != nullptr)
		{
			StagingBuffer->ReleaseGPU();
			Delete(StagingBuffer);
			StagingBuffer = nullptr;
		}

		State.Release();

		// Base
		GPUTexture::OnReleaseGPU();
	}

} // SE