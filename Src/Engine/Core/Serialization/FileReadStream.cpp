#include "FileReadStream.h"
#include "Core/Logging/Logging.h"
#include "Core/Platform/File.h"
#include "Core/Types/Strings/StringView.h"

namespace SE
{

	FileReadStream* FileReadStream::Open(const StringView& path)
	{
		const auto file = File::Open(path, FileMode::OpenExisting, FileAccess::Read, FileShare::Read);
		if (file == nullptr)
		{
			LOG_WARNING("Serialization", "Cannot open file '{0}'", path);
			return nullptr;
		}
		return New<FileReadStream>(file);
	}

	void FileReadStream::Unlink()
	{
		m_File = nullptr;
	}

	FileReadStream::FileReadStream(File* file)
		: m_File(file), m_VirtualPosInBuffer(0), m_BufferSize(0)
	{
		ASSERT_LOW_LAYER(m_File);
	}

	FileReadStream::~FileReadStream()
	{
		// Ensure to be file closed and deleted
		if (m_File)
			Delete(m_File);
	}

	void FileReadStream::Flush()
	{
		// Check if need to flush
		/*if (_virtualPosInBuffer > 0)
		{
			// Update buffer
			uint32 bytesRead;
			_hasError |= _file->Read(_buffer, FILESTREAM_BUFFER_SIZE, &bytesRead) != 0;
			_virtualPosInBuffer = 0;
		}*/
	}

	void FileReadStream::Close()
	{
		if (m_File)
		{
			m_File->Close();
		}
	}

	uint32 FileReadStream::GetLength()
	{
		return m_File->GetSize();
	}

	uint32 FileReadStream::GetPosition()
	{
		return m_File->GetPosition() - m_BufferSize + m_VirtualPosInBuffer;
	}

	void FileReadStream::SetPosition(uint32 seek)
	{
		// Seek
		m_File->SetPosition(seek);

		// Update buffer
		_hasError |= m_File->Read(m_Buffer, FILESTREAM_BUFFER_SIZE, &m_BufferSize) == 0;
		m_VirtualPosInBuffer = 0;
	}

	void FileReadStream::ReadBytes(void* data, uint32 bytes)
	{
		// Skip if nothing to read
		if (bytes == 0)
		{
			return;
		}

		// Ensure to flush the buffer if it's empty
		if (m_BufferSize == 0)
		{
//			CHECK(_virtualPosInBuffer == 0);
			_hasError |= m_File->Read(m_Buffer, FILESTREAM_BUFFER_SIZE, &m_BufferSize) == 0;
		}

		// Check if buffer has enough data for this read
		const uint32 bufferBytesLeft = static_cast<uint32>(m_BufferSize - m_VirtualPosInBuffer);
		if (bytes <= bufferBytesLeft)
		{
			Platform::MemoryCopy(data, m_Buffer + m_VirtualPosInBuffer, bytes);
			m_VirtualPosInBuffer += bytes;
		}
		else
		{
			// Flush already buffered bytes and read more it the buffer (reduce amount of flushes)
			if (m_VirtualPosInBuffer > 0)
			{
				Platform::MemoryCopy(data, m_Buffer + m_VirtualPosInBuffer, bufferBytesLeft);
				data = (byte*)data + bufferBytesLeft;
				bytes -= bufferBytesLeft;
				m_VirtualPosInBuffer = 0;
				_hasError |= m_File->Read(m_Buffer, FILESTREAM_BUFFER_SIZE, &m_BufferSize) == 0;
			}

			// Read as much as can using whole buffer
			while (bytes >= FILESTREAM_BUFFER_SIZE)
			{
				Platform::MemoryCopy(data, m_Buffer, FILESTREAM_BUFFER_SIZE);
				data = (byte*)data + FILESTREAM_BUFFER_SIZE;
				bytes -= FILESTREAM_BUFFER_SIZE;
				_hasError |= m_File->Read(m_Buffer, FILESTREAM_BUFFER_SIZE, &m_BufferSize) == 0;
			}

			// Read the rest of the buffer but without flushing its data
			if (bytes > 0)
			{
				Platform::MemoryCopy(data, m_Buffer, bytes);
				m_VirtualPosInBuffer = bytes;
			}
		}
	}
}