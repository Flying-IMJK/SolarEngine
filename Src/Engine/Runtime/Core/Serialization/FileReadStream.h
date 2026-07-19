#pragma once

#include "Runtime/Core/Platform/Types.h"
#include "ReadStream.h"

namespace SE
{

	/// <summary>
	/// Implementation of the stream that has access to the file and is optimized for fast reading from it
	/// </summary>
	/// <seealso cref="ReadStream" />
	class SE_API_RUNTIME FileReadStream : public ReadStream
	{
		NON_COPYABLE(FileReadStream)
	private:
		File* m_File;
		uint32 m_VirtualPosInBuffer; // Current position in the buffer (index)
		uint32 m_BufferSize; // Amount of loaded bytes from the file to the buffer
		byte m_Buffer[FILESTREAM_BUFFER_SIZE];

	public:

		/// <summary>
		/// Init
		/// </summary>
		/// <param name="file">File to read</param>
		FileReadStream(File* file);

		/// <summary>
		/// Destructor
		/// </summary>
		~FileReadStream() override;

	public:
		/// <summary>
		/// Gets the file handle.
		/// </summary>
		inline const File* GetFile() const
		{
			return m_File;
		}

		/// <summary>
		/// Unlink file object passed via constructor
		/// </summary>
		void Unlink();

	public:
		/// <summary>
		/// Open file to write data to it
		/// </summary>
		/// <param name="path">Path to the file</param>
		/// <returns>Created file reader stream or null if cannot perform that action</returns>
		static FileReadStream* Open(const StringView& path);

	public:
		// [ReadStream]
		void Flush() final override;
		void Close() final override;
		uint32 GetLength() override;
		uint32 GetPosition() override;
		void SetPosition(uint32 seek) override;
		void ReadBytes(void* data, uint32 bytes) override;
	};

}