
#include "GPUBufferVulkan.h"
#include "GPUContextVulkan.h"
#include "Runtime/Graphics/Async/Tasks/GPUUploadBufferTask.h"

namespace SE
{
	void GPUBufferViewVulkan::Init(GPUDeviceVulkan* device,
		GPUBufferVulkan* owner,
		VkBuffer buffer,
		VkDeviceSize size,
		VkBufferUsageFlags usage,
		PixelFormat format)
	{
		ENGINE_ASSERT(View == VK_NULL_HANDLE);

		Device = device;
		Owner = owner;
		Buffer = buffer;
		Size = size;

		GPUBufferDescription description = owner->GetDescription();
		if ((description.IsShaderResource() && !description.Flags.IsFlag(GPUBufferFlags::Structured)) ||
			(usage & VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT) == VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT)
		{
			VkBufferViewCreateInfo viewInfo;
			VulkanTool::ZeroStruct(viewInfo, VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO);
			viewInfo.buffer = Buffer;
			viewInfo.format = VulkanTool::ToVulkanFormat(format);
			viewInfo.offset = 0;
			viewInfo.range = Size;
			if (viewInfo.format == VK_FORMAT_UNDEFINED)
			{
				// Skip for structured buffers that use custom structure type and have unknown format
				return;
			}
			VALIDATE_VULKAN_RESULT(vkCreateBufferView(device->device, &viewInfo, nullptr, &View));
		}
	}

	void GPUBufferViewVulkan::Release()
	{

	}

	void GPUBufferViewVulkan::AsUniformTexelBuffer(GPUContextVulkan* context, VkBufferView& bufferView)
	{
		ASSERT_LOW_LAYER(View != VK_NULL_HANDLE);
		bufferView = View;
		context->AddBufferBarrier(Owner, VK_ACCESS_SHADER_READ_BIT);
	}

	void GPUBufferViewVulkan::AsStorageBuffer(GPUContextVulkan* context, VkBuffer& buffer, VkDeviceSize& offset, VkDeviceSize& range)
	{
		ASSERT_LOW_LAYER(Buffer);
		buffer = Buffer;
		offset = 0;
		range = Size;
		context->AddBufferBarrier(Owner, VK_ACCESS_SHADER_READ_BIT);
	}

	void GPUBufferViewVulkan::AsStorageTexelBuffer(GPUContextVulkan* context, VkBufferView& bufferView)
	{
		ASSERT_LOW_LAYER(View != VK_NULL_HANDLE);
		bufferView = View;
		context->AddBufferBarrier(Owner, VK_ACCESS_SHADER_READ_BIT);
	}


	GPUBufferView* GPUBufferVulkan::View() const
	{
		return (GPUBufferView*)&_view;
	}

	void GPUBufferVulkan::Unmap()
	{
		vmaUnmapMemory(m_Device->allocator, m_Allocation);
	}

	void* GPUBufferVulkan::Map(GPUResourceMapMode mode)
	{
		void* mapped = nullptr;
		const VkResult result = vmaMapMemory(m_Device->allocator, m_Allocation, (void**)&mapped);
		LOG_VULKAN_RESULT(result);
		return mapped;
	}

	bool GPUBufferVulkan::OnInit()
	{
		const bool useSRV = IsShaderResource();
		const bool useUAV = IsUnorderedAccess();

		// Setup buffer description
		VkBufferCreateInfo bufferInfo;
		VulkanTool::ZeroStruct(bufferInfo, VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO);
		bufferInfo.size = m_Desc.Size;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		if (useSRV && !m_Desc.Flags.IsFlag(GPUBufferFlags::Structured))
			bufferInfo.usage |= VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;
		if (useUAV || m_Desc.Flags.IsFlag(GPUBufferFlags::RawBuffer) || m_Desc.Flags.IsFlag(GPUBufferFlags::Structured))
			bufferInfo.usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
		if (useUAV && useSRV)
			bufferInfo.usage |= VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;
		if (m_Desc.Flags.IsFlag(GPUBufferFlags::Argument))
			bufferInfo.usage |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
		if (m_Desc.Flags.IsFlag(GPUBufferFlags::Argument) && useUAV)
			bufferInfo.usage |= VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT; // For some reason, glslang marks indirect uav buffers (UpdateProbesInitArgs, IndirectArgsBuffer) as Storage Texel Buffers
		if (m_Desc.Flags.IsFlag(GPUBufferFlags::VertexBuffer))
			bufferInfo.usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		if (m_Desc.Flags.IsFlag(GPUBufferFlags::IndexBuffer))
			bufferInfo.usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
		if (IsStaging() || m_Desc.Flags.IsFlag(GPUBufferFlags::UnorderedAccess))
			bufferInfo.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

		// Create buffer
		VmaAllocationCreateInfo allocInfo = {};
		switch (m_Desc.Usage)
		{
		case GPUResourceUsage::Dynamic:
			allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
			break;
		case GPUResourceUsage::StagingUpload:
			allocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
			break;
		case GPUResourceUsage::StagingReadback:
			allocInfo.usage = VMA_MEMORY_USAGE_GPU_TO_CPU;
			break;
		default:
			allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
		}
		const VkResult result = vmaCreateBuffer(m_Device->allocator, &bufferInfo,
			&allocInfo, &m_Buffer, &m_Allocation, nullptr);
		LOG_VULKAN_RESULT_WITH_RETURN(result);
#if GPU_ENABLE_RESOURCE_NAMING
		VK_SET_DEBUG_NAME(m_Device, m_Buffer, VK_OBJECT_TYPE_BUFFER, GetName());
#endif
		m_MemoryUsage = m_Desc.Size;
		Access = 0;

		// Check if set initial data
		if (m_Desc.InitData)
		{
			if (IsDynamic() || IsStaging())
			{
				// Faster path using Map/Unmap sequence
				SetData(m_Desc.InitData, m_Desc.Size);
			}
			else if (m_Device->IsRendering() && Threading::IsMainThread())
			{
				// Upload resource data now
				m_Device->GetMainContext()->UpdateBuffer(this, m_Desc.InitData, m_Desc.Size);
			}
			else
			{
				// Create async resource copy task
				auto copyTask = New<GPUUploadBufferTask>(this, 0, Span<byte>((const byte*)m_Desc.InitData, m_Desc.Size), true);
				ENGINE_ASSERT(copyTask->HasReference(this));
				copyTask->Start();
			}
		}

		// Check if need to use a counter
		if (m_Desc.Flags.IsFlag(GPUBufferFlags::Counter) && m_Desc.Flags.IsFlag(GPUBufferFlags::Append))
		{
#if GPU_ENABLE_RESOURCE_NAMING
			String name = String(GetName()) + SE_TEXT(".Counter");
			Counter = New<GPUBufferVulkan>(m_Device, name);
#else
			Counter = ::New<GPUBufferVulkan>(_device, StringView::Empty);
#endif
			if (Counter->Init(GPUBufferDescription::Raw(4, GPUBufferFlags::UnorderedAccess)))
			{
				LOG_ERROR("Graphic", "Cannot create counter buffer.");
				return false;
			}
		}
			// Check if need to bind buffer to the shaders
		else if (useSRV || useUAV)
		{
			// Create buffer view
			_view.Init(m_Device, this, m_Buffer, m_Desc.Size, bufferInfo.usage, m_Desc.Format);
		}

		return true;
	}

	void GPUBufferVulkan::OnReleaseGPU()
	{
		_view.Release();
		if (Counter)
		{
			Counter->OnReleaseGPU();
			Counter = nullptr;
		}

		if (m_Allocation != VK_NULL_HANDLE)
		{
			m_Device->deferredDeletionQueue.EnqueueResource(DeferredDeletionQueueVulkan::Buffer, m_Buffer, m_Allocation);
			m_Buffer = VK_NULL_HANDLE;
			m_Allocation = VK_NULL_HANDLE;
		}

		// Base
		GPUBuffer::OnReleaseGPU();
	}

}
