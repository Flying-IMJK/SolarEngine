#pragma once

#include "Base/GPUBuffer.h"

namespace SE
{
	/// <summary>
	/// Dynamic GPU buffer that allows to update and use GPU data (index/vertex/other) during single frame (supports dynamic resizing)
	/// </summary>
	class SE_API_RUNTIME DynamicBuffer
	{
		NON_COPYABLE(DynamicBuffer)
	protected:
		GPUBuffer* m_Buffer;
		String m_Name;
		uint32 m_Stride;

	public:

		/// <summary>
		/// Init
		/// </summary>
		/// <param name="initialCapacity">Initial capacity of the buffer (in bytes)</param>
		/// <param name="stride">Stride in bytes</param>
		/// <param name="name">Buffer name</param>
		DynamicBuffer(uint32 initialCapacity, uint32 stride, const String& name);

		/// <summary>
		/// Destructor
		/// </summary>
		virtual ~DynamicBuffer();

	public:
		/// <summary>
		/// The data container (raw bytes storage).
		/// </summary>
		List<byte> data;

		/// <summary>
		/// Gets buffer (may be null since it's using 'late init' feature)
		/// </summary>
		inline GPUBuffer* GetBuffer() const
		{
			return m_Buffer;
		}

		/// <summary>
		/// Clear data (begin for writing)
		/// </summary>
		inline void Clear()
		{
			data.Clear();
		}

		/// <summary>
		/// Write bytes to the buffer
		/// </summary>
		/// <param name="data">Data to write</param>
		template<typename T>
		inline void Write(const T& value)
		{
			data.Add((byte*)&value, sizeof(T));
		}

		/// <summary>
		/// Write bytes to the buffer
		/// </summary>
		/// <param name="bytes">Pointer to data to write</param>
		/// <param name="size">Amount of data to write (in bytes)</param>
		inline void Write(const void* bytes, int32 size)
		{
			data.Add((byte*)bytes, size);
		}

		/// <summary>
		/// 通过调整缓冲区的大小来分配缓冲区中的字节以[获取新内存]，并返回指向已分配空间开头的指针。
		/// </summary>
		/// <param name="size">Amount of data to allocate (in bytes)</param>
		inline byte* WriteReserve(int32 size)
		{
			const int32 start = data.Count();
			data.AddUninitialized(size);
			return data.Get() + start;
		}

		/// <summary>
		/// 通过调整缓冲区的大小来分配缓冲区中的字节以[获取新内存]，并返回指向已分配空间开头的指针。
		/// </summary>
		/// <param name="count">Amount of items to allocate</param>
		template<typename T>
		inline T* WriteReserve(int32 count)
		{
			return (T*)WriteReserve(count * sizeof(T));
		}

		/// <summary>
		/// Unlock buffer and flush data with a buffer (it will be ready for an immediate draw).
		/// </summary>
		void Flush();

		/// <summary>
		/// Unlock buffer and flush data with a buffer (it will be ready for a during next frame draw).
		/// </summary>
		/// <param name="context">The GPU command list context to use for data uploading.</param>
		void Flush(class GPUContext* context);

		/// <summary>
		/// Disposes the buffer resource and clears the used memory.
		/// </summary>
		void Dispose();

	protected:
		virtual void InitDesc(GPUBufferDescription& desc, int32 numElements) = 0;
	};

	/// <summary>
	/// Dynamic vertex buffer that allows to render any vertices during single frame (supports dynamic resizing)
	/// </summary>
	class SE_API_RUNTIME DynamicVertexBuffer : public DynamicBuffer
	{
	public:
		/// <summary>
		/// Init
		/// </summary>
		/// <param name="initialCapacity">Initial capacity of the buffer (in bytes)</param>
		/// <param name="stride">Stride in bytes</param>
		/// <param name="name">Buffer name</param>
		DynamicVertexBuffer(uint32 initialCapacity, uint32 stride, const String& name = String::Empty)
			: DynamicBuffer(initialCapacity, stride, name)
		{
		}

	protected:
		// [DynamicBuffer]
		void InitDesc(GPUBufferDescription& desc, int32 numElements) override
		{
			desc = GPUBufferDescription::Vertex(m_Stride, numElements, GPUResourceUsage::Dynamic);
		}
	};

	/// <summary>
	/// Dynamic index buffer that allows to render any indices during single frame (supports dynamic resizing)
	/// </summary>
	class SE_API_RUNTIME DynamicIndexBuffer : public DynamicBuffer
	{
	public:
		/// <summary>
		/// Init
		/// </summary>
		/// <param name="initialCapacity">Initial capacity of the buffer (in bytes)</param>
		/// <param name="stride">Stride in bytes</param>
		/// <param name="name">Buffer name</param>
		DynamicIndexBuffer(uint32 initialCapacity, uint32 stride, const String& name = String::Empty)
			: DynamicBuffer(initialCapacity, stride, name)
		{
		}

	protected:
		// [DynamicBuffer]
		void InitDesc(GPUBufferDescription& desc, int32 numElements) override
		{
			desc = GPUBufferDescription::Index(m_Stride, numElements, GPUResourceUsage::Dynamic);
		}
	};

	/// <summary>
	/// Dynamic structured buffer that allows to upload data to the GPU from CPU (supports dynamic resizing).
	/// </summary>
	class SE_API_RUNTIME DynamicStructuredBuffer : public DynamicBuffer
	{
	private:
		bool m_IsUnorderedAccess;

	public:
		/// <summary>
		/// Init
		/// </summary>
		/// <param name="initialCapacity">Initial capacity of the buffer (in bytes).</param>
		/// <param name="stride">Stride in bytes.</param>
		/// <param name="isUnorderedAccess">True if unordered access usage.</param>
		/// <param name="name">Buffer name.</param>
		DynamicStructuredBuffer(uint32 initialCapacity, uint32 stride, bool isUnorderedAccess = false, const String& name = String::Empty)
			: DynamicBuffer(initialCapacity, stride, name), m_IsUnorderedAccess(isUnorderedAccess)
		{
		}

	protected:
		// [DynamicBuffer]
		void InitDesc(GPUBufferDescription& desc, int32 numElements) override;
	};

	/// <summary>
	/// Dynamic Typed buffer that allows to upload data to the GPU from CPU (supports dynamic resizing).
	/// </summary>
	class SE_API_RUNTIME DynamicTypedBuffer : public DynamicBuffer
	{
	private:
		PixelFormat _format;
		bool m_IsUnorderedAccess;

	public:
		/// <summary>
		/// Init
		/// </summary>
		/// <param name="initialCapacity">Initial capacity of the buffer (in bytes).</param>
		/// <param name="format">CharsFormat of the data.</param>
		/// <param name="isUnorderedAccess">True if unordered access usage.</param>
		/// <param name="name">Buffer name.</param>
		DynamicTypedBuffer(uint32 initialCapacity, PixelFormat format, bool isUnorderedAccess = false, const String& name = String::Empty);

	protected:
		// [DynamicBuffer]
		void InitDesc(GPUBufferDescription& desc, int32 numElements) override;
	};

} // SE
