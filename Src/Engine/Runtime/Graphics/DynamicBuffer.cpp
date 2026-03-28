
#include "DynamicBuffer.h"
#include "GPUDevice.h"
#include "GPUContext.h"

namespace SE
{
	DynamicBuffer::DynamicBuffer(uint32 initialCapacity, uint32 stride, const String& name)
		: m_Buffer(nullptr)
		, m_Name(name)
		, m_Stride(stride)
		, data(initialCapacity)
	{
	}

	DynamicBuffer::~DynamicBuffer()
	{
		m_Buffer->ReleaseGPU();
		m_Buffer = nullptr;
	}

	void DynamicBuffer::Flush()
	{
		// Check if has sth to flush
		const uint32 size = data.Count();
		if (size > 0)
		{
			// Check if has no buffer
			if (m_Buffer == nullptr)
				m_Buffer = GPUDevice::instance->CreateBuffer(m_Name);

			// Check if need to resize buffer
			if (m_Buffer->GetSize() < size)
			{
				const uint32 numElements = Math::AlignUp<uint32>(static_cast<uint32>((size / m_Stride) * 1.3f), 32);
				GPUBufferDescription desc;
				InitDesc(desc, numElements);
				if (!m_Buffer->Init(desc))
				{
					LOG_FATAL("Graphic", "DynamicBuffer Cannot setup dynamic buffer '{0}'! Size: {1}", m_Name, size/*Utilities::BytesToText(size)*/);
					return;
				}
			}

			// Upload data to the buffer
			if (GPUDevice::instance->IsRendering())
			{
//				RenderContext::GPULocker.Lock();
				GPUDevice::instance->GetMainContext()->UpdateBuffer(m_Buffer, data.Get(), size);
//				RenderContext::GPULocker.Unlock();
			}
			else
			{
				m_Buffer->SetData(data.Get(), size);
			}
		}
	}

	void DynamicBuffer::Flush(GPUContext* context)
	{
		// Check if has sth to flush
		const uint32 size = data.Count();
		if (size > 0)
		{
			// Check if has no buffer
			if (m_Buffer == nullptr)
				m_Buffer = GPUDevice::instance->CreateBuffer(m_Name);

			// Check if need to resize buffer
			if (m_Buffer->GetSize() < size)
			{
				const uint32 numElements = Math::AlignUp<uint32>(static_cast<uint32>((size / m_Stride) * 1.3f), 32);
				GPUBufferDescription desc;
				InitDesc(desc, numElements);
				if (!m_Buffer->Init(desc))
				{
					LOG_FATAL("Graphic", "DynamicBuffer Cannot setup dynamic buffer '{0}'! Size: {1}", m_Name, size/*Utilities::BytesToText(size)*/);
					return;
				}
			}

			// Upload data to the buffer
			context->UpdateBuffer(m_Buffer, data.Get(), size);
		}
	}

	void DynamicBuffer::Dispose()
	{
		m_Buffer->ReleaseGPU();
		m_Buffer = nullptr;
		data.Resize(0);
	}

	void DynamicStructuredBuffer::InitDesc(GPUBufferDescription& desc, int32 numElements)
	{
		desc = GPUBufferDescription::Structured(numElements, m_Stride, m_IsUnorderedAccess);
		desc.Usage = GPUResourceUsage::Dynamic;
	}

	DynamicTypedBuffer::DynamicTypedBuffer(uint32 initialCapacity, PixelFormat format, bool isUnorderedAccess, const String& name)
		: DynamicBuffer(initialCapacity, PixelFormatGetSizeInBits(format), name)
		, _format(format)
		, m_IsUnorderedAccess(isUnorderedAccess)
	{
	}

	void DynamicTypedBuffer::InitDesc(GPUBufferDescription& desc, int32 numElements)
	{
		EnumFlags<GPUBufferFlags> bufferFlags = GPUBufferFlags::ShaderResource;
		if (m_IsUnorderedAccess)
		{
			bufferFlags.SetFlag(GPUBufferFlags::UnorderedAccess);
		}
		desc = GPUBufferDescription::Buffer(numElements * m_Stride, bufferFlags, _format, nullptr, m_Stride, GPUResourceUsage::Dynamic);
	}


} // SE