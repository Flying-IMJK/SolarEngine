#pragma once

#include "Core/Logging/Logging.h"
#include "Core/Memory/Memory.h"
#include "Core/Platform/CriticalSection.h"
#include "Core/Platform/Platform.h"
#include "Core/Types/Variable.h"

namespace SE
{
    class HeapAllocation;
    /// <summary>
    /// Template for dynamic array with variable capacity that support concurrent elements appending (atomic add).
    /// </summary>
    /// <typeparam name="T">The type of elements in the array.</typeparam>
    /// <typeparam name="AllocationType">The type of memory allocator.</typeparam>
    template<typename T, typename AllocationType = HeapAllocation>
    class RenderListBuffer
    {
        friend RenderListBuffer;
    public:
        typedef T ItemType;
        typedef typename AllocationType::template Data<T> AllocationData;

    private:
        volatile int64 m_Count;
        volatile int64 m_Capacity;
        volatile int64 m_ThreadsAdding = 0;
        volatile int64 m_ThreadsResizing = 0;
        AllocationData m_Allocation;
        CriticalSection m_Locker;

    public:
        /// <summary>
        /// Initializes a new instance of the <see cref="RenderListBuffer"/> class.
        /// </summary>
        FORCE_INLINE RenderListBuffer()
            : m_Count(0)
            , m_Capacity(0)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="RenderListBuffer"/> class.
        /// </summary>
        /// <param name="capacity">The initial capacity.</param>
        RenderListBuffer(int32 capacity)
            : m_Count(0)
            , m_Capacity(capacity)
        {
            if (capacity > 0)
                m_Allocation.Allocate(capacity);
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="RenderListBuffer"/> class.
        /// </summary>
        /// <param name="data">The initial data.</param>
        /// <param name="length">The amount of items.</param>
        RenderListBuffer(const T* data, int32 length)
        {
            ENGINE_ASSERT(length >= 0);
            m_Count = m_Capacity = length;
            if (length > 0)
            {
                m_Allocation.Allocate(length);
                Memory::ConstructItems(m_Allocation.Get(), data, length);
            }
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="RenderListBuffer"/> class.
        /// </summary>
        /// <param name="other">The other collection to copy.</param>
        RenderListBuffer(const RenderListBuffer& other)
        {
            m_Count = m_Capacity = other.m_Count;
            if (m_Capacity > 0)
            {
                m_Allocation.Allocate(m_Capacity);
                Memory::ConstructItems(m_Allocation.Get(), other.Get(), (int32)other.m_Count);
            }
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="RenderListBuffer"/> class.
        /// </summary>
        /// <param name="other">The other collection to move.</param>
        RenderListBuffer(RenderListBuffer&& other) noexcept
        {
            m_Count = other.m_Count;
            m_Capacity = other.m_Capacity;
            other.m_Count = 0;
            other.m_Capacity = 0;
            m_Allocation.Swap(other.m_Allocation);
        }

        /// <summary>
        /// The assignment operator that deletes the current collection of items and the copies items from the other array.
        /// </summary>
        /// <param name="other">The other collection to copy.</param>
        /// <returns>The reference to this.</returns>
        RenderListBuffer& operator=(const RenderListBuffer& other) noexcept
        {
            if (this != &other)
            {
                Memory::DestructItems(m_Allocation.Get(), (int32)m_Count);
                if (m_Capacity < other.Count())
                {
                    m_Allocation.Free();
                    m_Capacity = other.Count();
                    m_Allocation.Allocate(m_Capacity);
                }
                m_Count = other.Count();
                Memory::ConstructItems(m_Allocation.Get(), other.Get(), (int32)m_Count);
            }
            return *this;
        }

        /// <summary>
        /// The move assignment operator that deletes the current collection of items and the moves items from the other array.
        /// </summary>
        /// <param name="other">The other collection to move.</param>
        /// <returns>The reference to this.</returns>
        RenderListBuffer& operator=(RenderListBuffer&& other) noexcept
        {
            if (this != &other)
            {
                Memory::DestructItems(m_Allocation.Get(), (int32)m_Count);
                m_Allocation.Free();
                m_Count = other.m_Count;
                m_Capacity = other.m_Capacity;
                other.m_Count = 0;
                other.m_Capacity = 0;
                m_Allocation.Swap(other.m_Allocation);
            }
            return *this;
        }

        /// <summary>
        /// Finalizes an instance of the <see cref="RenderListBuffer"/> class.
        /// </summary>
        ~RenderListBuffer()
        {
            Memory::DestructItems(m_Allocation.Get(), (int32)m_Count);
        }

    public:
        /// <summary>
        /// Gets the amount of the items in the collection.
        /// </summary>
        FORCE_INLINE int32 Count() const
        {
            return (int32)Platform::AtomicRead((volatile int64*)&m_Count);
        }

        /// <summary>
        /// Gets the amount of the items that can be contained by collection without resizing.
        /// </summary>
        FORCE_INLINE int32 Capacity() const
        {
            return (int32)Platform::AtomicRead((volatile int64*)&m_Capacity);
        }

        /// <summary>
        /// Gets the critical section locking the collection during resizing.
        /// </summary>
        FORCE_INLINE const CriticalSection& Locker() const
        {
            return m_Locker;
        }

        /// <summary>
        /// Gets the pointer to the first item in the collection (linear allocation).
        /// </summary>
        FORCE_INLINE T* Get()
        {
            return m_Allocation.Get();
        }

        /// <summary>
        /// Gets the pointer to the first item in the collection (linear allocation).
        /// </summary>
        FORCE_INLINE const T* Get() const
        {
            return m_Allocation.Get();
        }

        /// <summary>
        /// Gets or sets the item at the given index.
        /// </summary>
        /// <returns>The reference to the item.</returns>
        FORCE_INLINE T& operator[](int32 index)
        {
            ENGINE_ASSERT(index >= 0 && index < Count());
            return m_Allocation.Get()[index];
        }

        /// <summary>
        /// Gets the item at the given index.
        /// </summary>
        /// <returns>The reference to the item.</returns>
        FORCE_INLINE const T& operator[](int32 index) const
        {
            ENGINE_ASSERT(index >= 0 && index < Count());
            return m_Allocation.Get()[index];
        }

    public:
        FORCE_INLINE T* begin()
        {
            return &m_Allocation.Get()[0];
        }

        FORCE_INLINE T* end()
        {
            return &m_Allocation.Get()[Count()];
        }

        FORCE_INLINE const T* begin() const
        {
            return &m_Allocation.Get()[0];
        }

        FORCE_INLINE const T* end() const
        {
            return &m_Allocation.Get()[Count()];
        }

    public:
        /// <summary>
        /// Clear the collection without changing its capacity.
        /// </summary>
        void Clear()
        {
            m_Locker.Lock();
            Memory::DestructItems(m_Allocation.Get(), (int32)m_Count);
            m_Count = 0;
            m_Locker.Unlock();
        }

        /// <summary>
        /// Changes the capacity of the collection.
        /// </summary>
        /// <param name="capacity">The new capacity.</param>
        /// <param name="preserveContents">True if preserve collection data when changing its size, otherwise collection after resize will be empty.</param>
        void SetCapacity(const int32 capacity, bool preserveContents = true)
        {
            if (capacity == Capacity())
                return;
            m_Locker.Lock();
            ENGINE_ASSERT(capacity >= 0);
            const int32 count = preserveContents ? ((int32)m_Count < capacity ? (int32)m_Count : capacity) : 0;
            m_Allocation.Relocate(capacity, (int32)m_Count, count);
            Platform::AtomicStore(&m_Capacity, capacity);
            Platform::AtomicStore(&m_Count, count);
            m_Locker.Unlock();
        }

        /// <summary>
        /// Resizes the collection to the specified size. If the size is equal or less to the current capacity no additional memory reallocation in performed.
        /// </summary>
        /// <param name="size">The new collection size.</param>
        /// <param name="preserveContents">True if preserve collection data when changing its size, otherwise collection after resize might not contain the previous data.</param>
        void Resize(int32 size, bool preserveContents = true)
        {
            m_Locker.Lock();
            if (m_Count > size)
            {
                Memory::DestructItems(m_Allocation.Get() + size, (int32)m_Count - size);
            }
            else
            {
                EnsureCapacity(size, preserveContents);
                Memory::ConstructItems(m_Allocation.Get() + m_Count, size - (int32)m_Count);
            }
            m_Count = size;
            m_Locker.Unlock();
        }

        /// <summary>
        /// Ensures the collection has given capacity (or more).
        /// </summary>
        /// <param name="minCapacity">The minimum capacity.</param>
        void EnsureCapacity(int32 minCapacity)
        {
            m_Locker.Lock();
            int32 capacity = (int32)Platform::AtomicRead(&m_Capacity);
            if (capacity < minCapacity)
            {
                capacity = m_Allocation.CalculateCapacityGrow(capacity, minCapacity);
                const int32 count = (int32)m_Count;
                m_Allocation.Relocate(capacity, count, count);
                Platform::AtomicStore(&m_Capacity, capacity);
            }
            m_Locker.Unlock();
        }

        /// <summary>
        /// Adds the specified item to the collection.
        /// </summary>
        /// <param name="item">The item to add.</param>
        /// <returns>Index of the added element.</returns>
        FORCE_INLINE int32 Add(const T& item)
        {
            const int32 index = AddOne();
            Memory::ConstructItems(m_Allocation.Get() + index, &item, 1);
            Platform::AtomicDecrement(&m_ThreadsAdding);
            return index;
        }

        /// <summary>
        /// Adds the specified item to the collection.
        /// </summary>
        /// <param name="item">The item to add.</param>
        /// <returns>Index of the added element.</returns>
        FORCE_INLINE int32 Add(T&& item)
        {
            const int32 index = AddOne();
            Memory::MoveItems(m_Allocation.Get() + index, &item, 1);
            Platform::AtomicDecrement(&m_ThreadsAdding);
            return index;
        }

    private:
        int32 AddOne()
        {
            Platform::AtomicIncrement(&m_ThreadsAdding);
            int32 count = (int32)Platform::AtomicRead(&m_Count);
            int32 capacity = (int32)Platform::AtomicRead(&m_Capacity);
            const int32 minCapacity = GetMinCapacity(count);
            if (minCapacity > capacity || Platform::AtomicRead(&m_ThreadsResizing)) // Resize if not enough space or someone else is already doing it (don't add mid-resizing)
            {
                // Move from adding to resizing
                Platform::AtomicIncrement(&m_ThreadsResizing);
                Platform::AtomicDecrement(&m_ThreadsAdding);

                // Wait for all threads to stop adding items before resizing can happen
                RETRY:
                    while (Platform::AtomicRead(&m_ThreadsAdding))
                        Platform::Sleep(0);

                // Thread-safe resizing
                m_Locker.Lock();
                capacity = (int32)Platform::AtomicRead(&m_Capacity);
                if (capacity < minCapacity)
                {
                    if (Platform::AtomicRead(&m_ThreadsAdding))
                    {
                        // Other thread entered during mutex locking so give them a chance to do safe resizing
                        m_Locker.Unlock();
                        goto RETRY;
                    }
                    capacity = m_Allocation.CalculateCapacityGrow(capacity, minCapacity);
                    count = (int32)Platform::AtomicRead(&m_Count);
                    m_Allocation.Relocate(capacity, count, count);
                    Platform::AtomicStore(&m_Capacity, capacity);
                }

                // Move from resizing to adding
                Platform::AtomicIncrement(&m_ThreadsAdding);
                Platform::AtomicDecrement(&m_ThreadsResizing);

                // Let other thread enter resizing-area
                m_Locker.Unlock();
            }
            return (int32)Platform::AtomicIncrement(&m_Count) - 1;
        }

        FORCE_INLINE static int32 GetMinCapacity(int32 count)
        {
            // Ensure there is a slack for others threads to reduce resize counts in highly multi-threaded environment
            constexpr int32 slack = PLATFORM_THREADS_LIMIT * 8;
            int32 capacity = count + slack;
            {
                // Round up to the next power of 2 and multiply by 2
                capacity--;
                capacity |= capacity >> 1;
                capacity |= capacity >> 2;
                capacity |= capacity >> 4;
                capacity |= capacity >> 8;
                capacity |= capacity >> 16;
                capacity = (capacity + 1) * 2;
            }
            return capacity;
        }
    };
}
