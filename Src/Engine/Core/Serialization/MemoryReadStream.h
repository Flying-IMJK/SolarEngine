#pragma once

#include "ReadStream.h"
#include "Core/Logging/Logging.h"

namespace SE
{
	/// <summary>
	/// Super fast advanced data reading from raw bytes without any overhead at all
	/// </summary>
	class SE_API_CORE MemoryReadStream : public ReadStream
	{
	private:

		const byte* m_Buffer;
		const byte* m_Position;
		uint32 m_Length;

	public:

		/// <summary>
		/// Init (empty, cannot access before Init())
		/// </summary>
		MemoryReadStream();

		/// <summary>
		/// Init
		/// </summary>
		/// <param name="bytes">Bytes with data to read from it (no memory cloned, using input buffer)</param>
		/// <param name="length">Amount of bytes</param>
		MemoryReadStream(const byte* bytes, uint32 length);

		/// <summary>
		/// Init
		/// </summary>
		/// <param name="data">List with data to read from</param>
		template<typename T, typename AllocationType = HeapAllocation>
		MemoryReadStream(const List<T, AllocationType>& data)
			: MemoryReadStream(data.Get(), data.Count() * sizeof(T))
		{
		}

	public:

		/// <summary>
		/// Init stream to the custom buffer location
		/// </summary>
		/// <param name="bytes">Bytes with data to read from it (no memory cloned, using input buffer)</param>
		/// <param name="length">Amount of bytes</param>
		void Init(const byte* bytes, uint32 length);

		/// <summary>
		/// Init stream to the custom buffer location
		/// </summary>
		/// <param name="data">List with data to read from</param>
		template<typename T, typename AllocationType = HeapAllocation>
		inline void Init(const List<T, AllocationType>& data)
		{
			Init(data.Get(), data.Count() * sizeof(T));
		}

		/// <summary>
		/// Gets the current handle to position in buffer.
		/// </summary>
		/// <returns>The position of the buffer in memory.</returns>
		const byte* GetPositionHandle() const
		{
			return m_Position;
		}

	public:
		/// <summary>
		/// Skips the data from the target buffer without reading from it. Moves the read pointer in the buffer forward.
		/// </summary>
		/// <param name="bytes">The amount of bytes to read.</param>
		/// <returns>The pointer to the data in memory.</returns>
		void* Move(uint32 bytes)
		{
			ENGINE_ASSERT(GetLength() - GetPosition() >= bytes);
			const auto result = (void*)m_Position;
			m_Position += bytes;
			return result;
		}

		/// <summary>
		/// Skips the data from the target buffer without reading from it. Moves the read pointer in the buffer forward.
		/// </summary>
		/// <returns>The pointer to the data in memory.</returns>
		template<typename T>
		inline T* Move()
		{
			return static_cast<T*>(Move(sizeof(T)));
		}

		/// <summary>
		/// Skips the data from the target buffer without reading from it. Moves the read pointer in the buffer forward.
		/// </summary>
		/// <param name="count">The amount of items to read.</param>
		/// <returns>The pointer to the data in memory.</returns>
		template<typename T>
		inline T* Move(uint32 count)
		{
			return static_cast<T*>(Move(sizeof(T) * count));
		}

	public:

		// [ReadStream]
		void Flush() override;
		void Close() override;
		uint32 GetLength() override;
		uint32 GetPosition() override;
		void SetPosition(uint32 seek) override;
		void ReadBytes(void* data, uint32 bytes) override;
	};
}