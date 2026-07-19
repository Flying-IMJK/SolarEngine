#include "FileWriteStream.h"
#include "Runtime/Core/Logging/Logging.h"
#include "Runtime/Core/Types/Strings/StringView.h"
#include "Runtime/Core/Platform/File.h"


namespace SE
{

	FileWriteStream* FileWriteStream::Open(const StringView& path)
	{
		auto file = File::Open(path, FileMode::CreateAlways, FileAccess::Write, FileShare::Read);
		if (file == nullptr)
		{
			LOG_WARNING("Serialization", "Cannot open file '{0}'", path);
			return nullptr;
		}
		return New<FileWriteStream>(file);
	}

	void FileWriteStream::Unlink()
	{
		m_File = nullptr;
	}

	FileWriteStream::FileWriteStream(File* file)
		: m_File(file)
		, m_VirtualPosInBuffer(0)
	{
		ASSERT_LOW_LAYER(m_File);
	}

	FileWriteStream::~FileWriteStream()
	{
		// Ensure to be file flushed, closed and deleted
		if (m_File)
		{
			Flush();
			Delete(m_File);
		}
	}

	void FileWriteStream::Flush()
	{
		// Check if need to flush
		if (m_VirtualPosInBuffer > 0)
		{
			// Update buffer
			uint32 bytesWritten;
			_hasError |= m_File->Write(m_Buffer, m_VirtualPosInBuffer, &bytesWritten) == 0;
			m_VirtualPosInBuffer = 0;
		}
	}

	void FileWriteStream::Close()
	{
		if (m_File)
		{
			Flush();
			m_File->Close();
		}
	}

	uint32 FileWriteStream::GetLength()
	{
		Flush();
		return m_File->GetSize();
	}

	uint32 FileWriteStream::GetPosition()
	{
		Flush();
		return m_File->GetPosition();
	}

	void FileWriteStream::SetPosition(uint32 seek)
	{
		Flush();
		m_File->SetPosition(seek);
	}

	void FileWriteStream::WriteBytes(const void* data, uint32 bytes)
	{
		const uint32 bufferBytesLeft = FILESTREAM_BUFFER_SIZE - m_VirtualPosInBuffer;
		if (bytes <= bufferBytesLeft)
		{
			Platform::MemoryCopy(m_Buffer + m_VirtualPosInBuffer, data, bytes);
			m_VirtualPosInBuffer += bytes;
		}
		else
		{
			uint32 bytesWritten;

			// Flush already written bytes and write more it the buffer (reduce amount of flushes)
			if (m_VirtualPosInBuffer > 0)
			{
				Platform::MemoryCopy(m_Buffer + m_VirtualPosInBuffer, data, bufferBytesLeft);
				data = (byte*)data + bufferBytesLeft;
				bytes -= bufferBytesLeft;
				m_VirtualPosInBuffer = 0;
				_hasError |= m_File->Write(m_Buffer, FILESTREAM_BUFFER_SIZE, &bytesWritten) == 0;
			}

			// Write as much as can using whole buffer
			while (bytes >= FILESTREAM_BUFFER_SIZE)
			{
				Platform::MemoryCopy(m_Buffer, data, FILESTREAM_BUFFER_SIZE);
				data = (byte*)data + FILESTREAM_BUFFER_SIZE;
				bytes -= FILESTREAM_BUFFER_SIZE;
				_hasError |= m_File->Write(m_Buffer, FILESTREAM_BUFFER_SIZE, &bytesWritten) == 0;
			}

			// Write the rest of the buffer but without flushing its data
			if (bytes > 0)
			{
				Platform::MemoryCopy(m_Buffer, data, bytes);
				m_VirtualPosInBuffer = bytes;
			}
		}
	}

}