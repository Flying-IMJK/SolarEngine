
#include "Runtime/Core/Platform/Platform.h"
#include "Runtime/Core/Platform/File.h"
#include "MemoryWriteStream.h"
#include "Runtime/Core/Memory/Memory.h"
#include "Runtime/Core/Logging/Logging.h"
namespace SE
{

	MemoryWriteStream::MemoryWriteStream()
		: m_Buffer(nullptr)
		, m_Position(nullptr)
		, m_Capacity(0)
	{
	}

	MemoryWriteStream::MemoryWriteStream(uint32 capacity)
		: m_Capacity(capacity)
	{
		if (capacity > 0)
		{
			m_Buffer = (byte*)PlatformAllocator::Allocate(capacity);
			if (m_Buffer == nullptr)
			{
				LOG_ERROR("Serialize", "MemoryWriteStream out of memory");
			}
		}
		else
		{
			m_Buffer = nullptr;
		}
		m_Position = m_Buffer;
	}

	MemoryWriteStream::~MemoryWriteStream()
	{
		PlatformAllocator::Free(m_Buffer);
	}

	void* MemoryWriteStream::Move(uint32 bytes)
	{
		const uint32 position = GetPosition();

		// Check if there is need to update a buffer size
		if (m_Capacity - position < bytes)
		{
			// Perform reallocation
			uint32 newCapacity = m_Capacity != 0 ? m_Capacity * 2 : 256;
			while (newCapacity < position + bytes)
			{
				newCapacity *= 2;
			}
			byte* newBuf = (byte*)PlatformAllocator::Allocate(newCapacity);
			if (newBuf == nullptr)
			{
				LOG_ERROR("Serialize", "MemoryWriteStream out of memory");
			}
			Platform::MemoryCopy(newBuf, m_Buffer, m_Capacity);
			PlatformAllocator::Free(m_Buffer);

			// Update state
			m_Buffer = newBuf;
			m_Capacity = newCapacity;
			m_Position = m_Buffer + position;
		}

		// Skip bytes
		m_Position += bytes;

		// Return pointer to begin
		return m_Buffer + position;
	}

	void MemoryWriteStream::Reset(uint32 capacity)
	{
		// Check if resize
		if (capacity > m_Capacity)
		{
			PlatformAllocator::Free(m_Buffer);
			m_Buffer = (byte*)PlatformAllocator::Allocate(capacity);
			if (m_Buffer == nullptr)
			{
				LOG_ERROR("Serialize", "MemoryWriteStream out of memory");
			}
			m_Capacity = capacity;
		}

		// Reset pointer
		m_Position = m_Buffer;
	}

	bool MemoryWriteStream::SaveToFile(const StringView& path) const
	{
		// Open file for writing
		auto file = File::Open(path, FileMode::CreateAlways, FileAccess::Write, FileShare::Read);
		if (file == nullptr)
		{
			return false;
		}

		// Write data
		uint32 bytesWritten;
		file->Write(GetHandle(), GetPosition(), &bytesWritten);

		Delete(file);
		return true;
	}

	void MemoryWriteStream::Flush()
	{
		// Nothing to do
	}

	void MemoryWriteStream::Close()
	{
		PlatformAllocator::Free(m_Buffer);
		m_Buffer = nullptr;
		m_Position = nullptr;
		m_Capacity = 0;
	}

	uint32 MemoryWriteStream::GetLength()
	{
		return m_Capacity;
	}

	uint32 MemoryWriteStream::GetPosition()
	{
		return static_cast<uint32>(m_Position - m_Buffer);
	}

	void MemoryWriteStream::SetPosition(uint32 seek)
	{
		m_Position = m_Buffer + seek;
	}

	void MemoryWriteStream::WriteBytes(const void* data, uint32 bytes)
	{
		// Calculate current position
		const uint32 position = GetPosition();

		// Check if there is need to update a buffer size
		if (m_Capacity - position < bytes)
		{
			// Perform reallocation
			uint32 newCapacity = m_Capacity != 0 ? m_Capacity * 2 : 256;
			while (newCapacity < position + bytes)
				newCapacity *= 2;
			byte* newBuf = (byte*)PlatformAllocator::Allocate(newCapacity);
			if (newBuf == nullptr)
			{
				LOG_ERROR("Serialize", "MemoryWriteStream out of memory");
			}
			Platform::MemoryCopy(newBuf, m_Buffer, m_Capacity);
			PlatformAllocator::Free(m_Buffer);

			// Update state
			m_Buffer = newBuf;
			m_Capacity = newCapacity;
			m_Position = m_Buffer + position;
		}

		// Copy data
		Platform::MemoryCopy(m_Position, data, bytes);
		m_Position += bytes;
	}

}