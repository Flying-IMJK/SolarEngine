#include "MemoryReadStream.h"

namespace SE
{

	MemoryReadStream::MemoryReadStream()
		: m_Buffer(nullptr)
		, m_Position(nullptr)
		, m_Length(0)
	{
	}

	MemoryReadStream::MemoryReadStream(const byte* bytes, uint32 length)
		: m_Buffer(bytes)
		, m_Position(bytes)
		, m_Length(length)
	{
	}

	void MemoryReadStream::Init(const byte* bytes, uint32 length)
	{
		m_Buffer = m_Position = bytes;
		m_Length = length;
	}

	void MemoryReadStream::Flush()
	{
	}

	void MemoryReadStream::Close()
	{
		m_Position = m_Buffer = nullptr;
		m_Length = 0;
	}

	uint32 MemoryReadStream::GetLength()
	{
		return m_Length;
	}

	uint32 MemoryReadStream::GetPosition()
	{
		return static_cast<uint32>(m_Position - m_Buffer);
	}

	void MemoryReadStream::SetPosition(uint32 seek)
	{
		ENGINE_ASSERT(m_Length > 0);
		m_Position = m_Buffer + seek;
	}

	void MemoryReadStream::ReadBytes(void* data, uint32 bytes)
	{
		if (bytes > 0)
		{
			ENGINE_ASSERT(data && GetLength() - GetPosition() >= bytes);
			Platform::MemoryCopy(data, m_Position, bytes);
			m_Position += bytes;
		}
	}
}