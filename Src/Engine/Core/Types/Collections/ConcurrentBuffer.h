#pragma once

#include "Core/Platform/Platform.h"
#include "Core/Platform/CriticalSection.h"
#include "Core/Memory/Allocation.h"
#include "Core/Math/Math.h"

namespace SE
{

	/// <summary>
	/// The concurrent data buffer allows to implement asynchronous data writing to the linear buffer by more than one worker thread at once.
	/// Supports only value types that don't require constructor/destructor invocation.
	/// </summary>
	template<typename T>
	class ConcurrentBuffer
	{
		friend ConcurrentBuffer;

	private:

		int64 m_Count;
		int64 m_Capacity;
		T* m_Data;
		CriticalSection m_ResizeLocker;

	public:

		/// <summary>
		/// Initializes a new instance of the <see cref="ConcurrentBuffer"/> class.
		/// </summary>
		ConcurrentBuffer()
			: m_Count(0)
			, m_Capacity(0)
			, m_Data(nullptr)
		{
		}

		/// <summary>
		/// Initializes a new instance of the <see cref="ConcurrentBuffer"/> class.
		/// </summary>
		/// <param name="capacity">The capacity.</param>
		ConcurrentBuffer(int32 capacity)
			: m_Count(0)
			, m_Capacity(capacity)
		{
			if (m_Capacity > 0)
				m_Data = (T*)PlatformAllocator::Allocate(m_Capacity * sizeof(T));
			else
				m_Data = nullptr;
		}

		/// <summary>
		/// Finalizes an instance of the <see cref="ConcurrentBuffer"/> class.
		/// </summary>
		~ConcurrentBuffer()
		{
			PlatformAllocator::Free(m_Data);
		}

	public:

		/// <summary>
		/// Gets the amount of the elements in the collection.
		/// </summary>
		/// <returns>The items count.</returns>
		inline int64 Count()
		{
			return Platform::AtomicRead(&m_Count);
		}

		/// <summary>
		/// Get amount of the elements that can be holed by collection without resizing.
		/// </summary>
		/// <returns>the items capacity.</returns>
		inline int64 Capacity() const
		{
			return m_Capacity;
		}

		/// <summary>
		/// Determines whether this collection isn't empty.
		/// </summary>
		/// <returns><c>true</c> if this collection has elements; otherwise, <c>false</c>.</returns>
		inline bool HasItems() const
		{
			return m_Count != 0;
		}

		/// <summary>
		/// Determines whether this collection is empty.
		/// </summary>
		/// <returns><c>true</c> if this collection is empty; otherwise, <c>false</c>.</returns>
		inline bool IsEmpty() const
		{
			return m_Count == 0;
		}

		/// <summary>
		/// Gets the pointer to the first element in the collection.
		/// </summary>
		/// <returns>The allocation start.</returns>
		inline T* Get()
		{
			return m_Data;
		}

		/// <summary>
		/// Gets the pointer to the first element in the collection.
		/// </summary>
		/// <returns>The allocation start.</returns>
		inline const T* Get() const
		{
			return m_Data;
		}

		/// <summary>
		/// Gets the last element.
		/// </summary>
		/// <returns>The last element reference.</returns>
		inline T& Last()
		{
			ASSERT(m_Count > 0);
			return m_Data[m_Count - 1];
		}

		/// <summary>
		/// Gets the last element.
		/// </summary>
		/// <returns>The last element reference.</returns>
		inline const T& Last() const
		{
			ASSERT(m_Count > 0);
			return m_Data[m_Count - 1];
		}

		/// <summary>
		/// Gets the first element.
		/// </summary>
		/// <returns>The first element reference.</returns>
		inline T& First()
		{
			ASSERT(m_Count > 0);
			return m_Data[0];
		}

		/// <summary>
		/// Gets the first element.
		/// </summary>
		/// <returns>The first element reference.</returns>
		inline const T& First() const
		{
			ASSERT(m_Count > 0);
			return m_Data[0];
		}

		/// <summary>
		/// Get or sets element by the index.
		/// </summary>
		/// <param name="index">The index.</param>
		/// <returns>The item reference.</returns>
		inline T& operator[](int64 index)
		{
			ASSERT(index >= 0 && index < m_Count);
			return m_Data[index];
		}

		/// <summary>
		/// Get or sets element by the index.
		/// </summary>
		/// <param name="index">The index.</param>
		/// <returns>The item reference (constant).</returns>
		inline const T& operator[](int64 index) const
		{
			ASSERT(index >= 0 && index < m_Count);
			return m_Data[index];
		}

	public:

		/// <summary>
		/// Clear the collection but without changing its capacity.
		/// </summary>
		inline void Clear()
		{
			Platform::AtomicExchange(&m_Count, 0);
		}

		/// <summary>
		/// Releases this buffer data.
		/// </summary>
		void Release()
		{
			m_ResizeLocker.Lock();
			PlatformAllocator::Free(m_Data);
			m_Data = nullptr;
			m_Capacity = 0;
			m_Count = 0;
			m_ResizeLocker.Unlock();
		}

		/// <summary>
		/// Sets the custom size of the collection. Only for the custom usage with dedicated data.
		/// </summary>
		/// <param name="size">The size.</param>
		void SetSize(int32 size)
		{
			ASSERT(size >= 0 && size <= m_Capacity);
			m_Count = size;
		}

		/// <summary>
		/// Adds the single item to the collection. Handles automatic buffer resizing. Thread-safe function that can be called from many threads at once.
		/// </summary>
		/// <param name="item">The item to add.</param>
		/// <returns>The index of the added item.</returns>
		inline int64 Add(const T& item)
		{
			return Add(&item, 1);
		}

		/// <summary>
		/// Adds the array of items to the collection. Handles automatic buffer resizing. Thread-safe function that can be called from many threads at once.
		/// </summary>
		/// <param name="items">The collection of items to add.</param>
		/// <param name="count">The items count.</param>
		/// <returns>The index of the added first item.</returns>
		int64 Add(const T* items, int32 count)
		{
			const int64 index = Platform::AtomicAdd(&m_Count, (int64)count);
			const int64 newCount = index + (int64)count;
			EnsureCapacity(newCount);
			Memory::CopyItems(m_Data + index, items, count);
			return index;
		}

		// Add collection of items to the collection
		// @param collection Array with the items to add
		inline void Add(ConcurrentBuffer<T>& collection)
		{
			Add(collection.Get(), collection.Count());
		}

		/// <summary>
		/// Adds the given amount of items to the collection.
		/// </summary>
		/// <param name="count">The items count.</param>
		/// <returns>The index of the added first item.</returns>
		int64 AddDefault(int32 count = 1)
		{
			const int64 index = Platform::AtomicAdd(&m_Count, (int64)count);
			const int64 newCount = index + (int64)count;
			EnsureCapacity(newCount);
			Memory::ConstructItems(m_Data + newCount, count);
			return index;
		}

		/// <summary>
		/// Adds the one item to the collection and returns the reference to it.
		/// </summary>
		/// <returns>The reference to the added item.</returns>
		inline T& AddOne()
		{
			const int64 index = Platform::AtomicAdd(&m_Count, 1);
			const int64 newCount = index + 1;
			EnsureCapacity(newCount);
			Memory::ConstructItems(m_Data + index, 1);
			return m_Data[index - 1];
		}

		/// <summary>
		/// Adds the new items to the end of the collection, possibly reallocating the whole collection to fit. The new items will be zeroed.
		/// </summary>
		/// Warning! AddZeroed() will create items without calling the constructor and this is not appropriate for item types that require a constructor to function properly.
		/// <remarks>
		/// </remarks>
		/// <param name="count">The number of new items to add.</param>
		/// <returns>The index of the added first item.</returns>
		int64 AddZeroed(int32 count = 1)
		{
			const int64 index = Platform::AtomicAdd(&m_Count, 1);
			const int64 newCount = index + 1;
			EnsureCapacity(newCount);
			Platform::MemoryClear(Get() + index, count * sizeof(T));
			return m_Data[index - 1];
		}

		/// <summary>
		/// Ensures that the buffer has the given the capacity (equal or more). Preserves the existing items by copy operation.
		/// </summary>
		/// <param name="minCapacity">The minimum capacity.</param>
		void EnsureCapacity(int64 minCapacity)
		{
			// Early out
			if (m_Capacity >= minCapacity)
				return;

			m_ResizeLocker.Lock();

			// Skip if the other thread performed resizing
			if (m_Capacity >= minCapacity)
			{
				m_ResizeLocker.Unlock();
				return;
			}

			// Compute the new capacity
			int64 newCapacity = m_Capacity == 0 ? 8 : Math::NextPowerOfTwo(m_Capacity) * 2;
			if (newCapacity < minCapacity)
			{
				newCapacity = minCapacity;
			}
			ASSERT(newCapacity > m_Capacity);

			// Allocate new data
			T* newData = nullptr;
			if (newCapacity > 0)
			{
				newData = (T*)PlatformAllocator::Allocate(newCapacity * sizeof(T));
			}

			// Check if has data
			if (m_Data)
			{
				// Check if preserve contents
				if (newData && m_Count > 0)
				{
					Platform::MemoryCopy(newData, m_Data, m_Capacity * sizeof(T));
				}

				// Delete old data
				PlatformAllocator::Free(m_Data);
			}

			// Set state
			m_Data = newData;
			m_Capacity = newCapacity;

			m_ResizeLocker.Unlock();
		}

		/// <summary>
		/// Swaps the contents of buffer with the other object without copy operation. Performs fast internal data exchange.
		/// </summary>
		/// <param name="other">The other buffer.</param>
		void Swap(ConcurrentBuffer& other)
		{
			const auto count = m_Count;
			const auto capacity = m_Capacity;
			const auto data = m_Data;

			m_Count = other.m_Count;
			m_Capacity = other.m_Capacity;
			m_Data = other.m_Data;

			other.m_Count = count;
			other.m_Capacity = capacity;
			other.m_Data = data;
		}

	public:

		/// <summary>
		/// Checks if the given element is in the collection.
		/// </summary>
		/// <param name="item">The item.</param>
		/// <returns><c>true</c> if collection contains the specified item; otherwise, <c>false</c>.</returns>
		bool Contains(const T& item) const
		{
			for (int32 i = 0; i < m_Count; i++)
			{
				if (m_Data[i] == item)
				{
					return true;
				}
			}

			return false;
		}

		/// <summary>
		/// Searches for the specified object and returns the zero-based index of the first occurrence within the entire collection.
		/// </summary>
		/// <param name="item">The item.</param>
		/// <returns>The zero-based index of the first occurrence of item within the entire collection, if found; otherwise, INVALID_INDEX.</returns>
		int32 IndexOf(const T& item) const
		{
			for (int32 i = 0; i < m_Count; i++)
			{
				if (m_Data[i] == item)
				{
					return i;
				}
			}

			return INVALID_INDEX;
		}
	};
}