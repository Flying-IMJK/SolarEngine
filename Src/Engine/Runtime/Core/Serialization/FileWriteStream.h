#pragma once

#include "Runtime/Core/Platform/Types.h"
#include "WriteStream.h"

namespace SE
{

	/// <summary>
	/// Implementation of the stream that has access to the file and is optimized for fast writing to it.
	/// </summary>
	/// <seealso cref="WriteStream" />
	class SE_API_RUNTIME FileWriteStream final : public WriteStream
	{
		NON_COPYABLE(FileWriteStream)
	private:

		File* m_File;
		uint32 m_VirtualPosInBuffer;
		byte m_Buffer[FILESTREAM_BUFFER_SIZE];

	public:

		/// <summary>
		/// Init
		/// </summary>
		/// <param name="file">File to write</param>
		FileWriteStream(File* file);

		/// <summary>
		/// Destructor
		/// </summary>
		~FileWriteStream();

	public:

		/// <summary>
		/// Gets the file handle.
		/// </summary>
		/// <returns>File</returns>
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
		/// Open file to read data from it
		/// </summary>
		/// <param name="path">Path to the file</param>
		/// <returns>Created file writer stream or null if cannot perform it</returns>
		static FileWriteStream* Open(const StringView& path);

	public:

		// [WriteStream]
		void Flush() final override;
		void Close() final override;
		uint32 GetLength() override;
		uint32 GetPosition() override;
		void SetPosition(uint32 seek) override;
		void WriteBytes(const void* data, uint32 bytes) override;
	};

}