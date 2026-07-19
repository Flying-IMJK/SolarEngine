#pragma once

#include <initializer_list>
#include "Runtime/Core/Memory/Allocation.h"

namespace SE
{
    /// <summary>
    /// Template for dynamic array with variable capacity.
    /// </summary>
    /// <typeparam name="T">The type of elements in the array.</typeparam>
    /// <typeparam name="AllocationType">The type of memory allocator.</typeparam>
    template<typename T, typename AllocationType>
    class List
    {
        friend List;
    public:
        typedef T ItemType;
        typedef typename AllocationType::template Data<T> AllocationData;

    private:
        int32 m_count;
        int32 m_capacity;
        AllocationData m_allocation;

        inline static void MoveToEmpty(AllocationData &to, AllocationData &from, int32 fromCount, int32 fromCapacity)
        {
            if constexpr (AllocationType::HasSwap)
            {
                to.Swap(from);
            }
            else
            {
                to.Allocate(fromCapacity);
                Memory::MoveItems(to.Get(), from.Get(), fromCount);
                Memory::DestructItems(from.Get(), fromCount);
                from.Free();
            }
        }

    public:
        /// <summary>
        /// Initializes a new instance of the <see cref="Array"/> class.
        /// </summary>
        List() : m_count(0), m_capacity(0)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Array"/> class.
        /// </summary>
        /// <param name="capacity">The initial capacity.</param>
        explicit List(int32 capacity): m_count(0), m_capacity(capacity)
        {
            if (capacity > 0)
            {
                m_allocation.Allocate(capacity);
				Memory::ConstructItems(m_allocation.Get(), m_count);
            }
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Array"/> class.
        /// </summary>
        /// <param name="initList">The initial values defined in the array.</param>
        List(std::initializer_list<T> initList)
        {
            m_count = m_capacity = (int32) initList.size();
            if (m_count > 0)
            {
                m_allocation.Allocate(m_count);
                Memory::ConstructItems(m_allocation.Get(), initList.begin(), m_count);
            }
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Array"/> class.
        /// </summary>
        /// <param name="data">The initial data.</param>
        /// <param name="length">The amount of items.</param>
        List(const T *data, int32 length)
        {
            ENGINE_ASSERT(length >= 0);
            m_count = m_capacity = length;
            if (length > 0)
            {
                m_allocation.Allocate(length);
                Memory::ConstructItems(m_allocation.Get(), data, length);
            }
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Array"/> class.
        /// </summary>
        /// <param name="other">The other collection to copy.</param>
        List(const List &other)
        {
            m_count = m_capacity = other.m_count;
            if (m_capacity > 0)
            {
                m_allocation.Allocate(m_capacity);
                Memory::ConstructItems(m_allocation.Get(), other.Get(), other.m_count);
            }
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Array"/> class.
        /// </summary>
        /// <param name="other">The other collection to copy.</param>
        /// <param name="extraSize">The additionally amount of items to add to the add.</param>
        List(const List &other, int32 extraSize)
        {
            ENGINE_ASSERT(extraSize >= 0);
            m_count = m_capacity = other.m_count + extraSize;
            if (m_capacity > 0)
            {
                m_allocation.Allocate(m_capacity);
                Memory::ConstructItems(m_allocation.Get(), other.Get(), other.m_count);
                Memory::ConstructItems(m_allocation.Get() + other.m_count, extraSize);
            }
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Array"/> class.
        /// </summary>
        /// <param name="other">The other collection to copy.</param>
        template<typename OtherT = T, typename OtherAllocationType = AllocationType>
        explicit List(const List<OtherT, OtherAllocationType> &other) noexcept
        {
            m_capacity = other.Capacity();
            m_count = other.Count();
            if (m_capacity > 0)
            {
                m_allocation.Allocate(m_capacity);
                Memory::ConstructItems(m_allocation.Get(), other.Get(), m_count);
            }
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Array"/> class.
        /// </summary>
        /// <param name="other">The other collection to move.</param>
        List(List &&other) noexcept
        {
            m_count = other.m_count;
            m_capacity = other.m_capacity;
            other.m_count = 0;
            other.m_capacity = 0;
            MoveToEmpty(m_allocation, other.m_allocation, m_count, m_capacity);
        }

        /// <summary>
        /// The assignment operator that deletes the current collection of items and the copies items from the initializer list.
        /// </summary>
        /// <param name="initList">The other collection to copy.</param>
        /// <returns>The reference to this.</returns>
        List &operator=(std::initializer_list<T> initList) noexcept
        {
            Clear();
            if (initList.size() > 0)
            {
                EnsureCapacity((int32) initList.size());
                m_count = (int32) initList.size();
                Memory::ConstructItems(m_allocation.Get(), initList.begin(), m_count);
            }
            return *this;
        }

        /// <summary>
        /// The assignment operator that deletes the current collection of items and the copies items from the other array.
        /// </summary>
        /// <param name="other">The other collection to copy.</param>
        /// <returns>The reference to this.</returns>
        List &operator=(const List &other) noexcept
        {
            if (this != &other)
            {
                Memory::DestructItems(m_allocation.Get(), m_count);
                if (m_capacity < other.Count())
                {
                    m_allocation.Free();
                    m_capacity = other.Count();
                    m_allocation.Allocate(m_capacity);
                }
                m_count = other.Count();
                Memory::ConstructItems(m_allocation.Get(), other.Get(), m_count);
            }
            return *this;
        }

        /// <summary>
        /// The move assignment operator that deletes the current collection of items and the moves items from the other array.
        /// </summary>
        /// <param name="other">The other collection to move.</param>
        /// <returns>The reference to this.</returns>
        List &operator=(List &&other) noexcept
        {
            if (this != &other)
            {
                Memory::DestructItems(m_allocation.Get(), m_count);
                m_allocation.Free();
                m_count = other.m_count;
                m_capacity = other.m_capacity;
                other.m_count = 0;
                other.m_capacity = 0;
                MoveToEmpty(m_allocation, other.m_allocation, m_count, m_capacity);
            }
            return *this;
        }

        /// <summary>
        /// Finalizes an instance of the <see cref="Array"/> class.
        /// </summary>
        ~List()
        {
            Memory::DestructItems(m_allocation.Get(), m_count);
        }

    public:
        /// <summary>
        /// Gets the amount of the items in the collection.
        /// </summary>
        inline int32 Count() const
        {
            return m_count;
        }

        /// <summary>
        /// Gets the amount of the items that can be contained by collection without resizing.
        /// </summary>
        inline int32 Capacity() const
        {
            return m_capacity;
        }

        /// <summary>
        /// Returns true if collection isn't empty.
        /// </summary>
        inline bool HasItems() const
        {
            return m_count != 0;
        }

        /// <summary>
        /// Returns true if collection is empty.
        /// </summary>
        inline bool IsEmpty() const
        {
            return m_count == 0;
        }

        /// <summary>
        /// Determines if given index is valid.
        /// </summary>
        /// <param name="index">The index.</param>
        /// <returns><c>true</c> if is valid a index; otherwise, <c>false</c>.</returns>
        bool IsValidIndex(int32 index) const
        {
            return index < m_count && index >= 0;
        }

        /// <summary>
        /// Gets the pointer to the first item in the collection (linear allocation).
        /// </summary>
        inline T* Get()
        {
            return m_allocation.Get();
        }

        /// <summary>
        /// Gets the pointer to the first item in the collection (linear allocation).
        /// </summary>
        inline const T* Get() const
        {
            return m_allocation.Get();
        }

        /// <summary>
        /// Gets item at the given index.
        /// </summary>
        /// <returns>The reference to the item.</returns>
        inline T &At(int32 index)
        {
            ENGINE_ASSERT(index >= 0 && index < m_count);
            return m_allocation.Get()[index];
        }

        /// <summary>
        /// Gets item at the given index.
        /// </summary>
        /// <returns>The reference to the item.</returns>
        inline const T &At(int32 index) const
        {
            ENGINE_ASSERT(index >= 0 && index < m_count);
            return m_allocation.Get()[index];
        }

        /// <summary>
        /// Gets or sets the item at the given index.
        /// </summary>
        /// <returns>The reference to the item.</returns>
        inline T& operator[](int32 index)
        {
            ENGINE_ASSERT(index >= 0 && index < m_count);
            return m_allocation.Get()[index];
        }

        /// <summary>
        /// Gets the item at the given index.
        /// </summary>
        /// <returns>The reference to the item.</returns>
        inline const T& operator[](int32 index) const
        {
            ENGINE_ASSERT(index >= 0 && index < m_count);
            return m_allocation.Get()[index];
        }

        /// <summary>
        /// Gets the last item.
        /// </summary>
        inline T& Last()
        {
            ENGINE_ASSERT(m_count > 0);
            return m_allocation.Get()[m_count - 1];
        }

        /// <summary>
        /// Gets the last item.
        /// </summary>
        inline const T& Last() const
        {
            ENGINE_ASSERT(m_count > 0);
            return m_allocation.Get()[m_count - 1];
        }

        /// <summary>
        /// Gets the first item.
        /// </summary>
        inline T& First()
        {
            ENGINE_ASSERT(m_count > 0);
            return m_allocation.Get()[0];
        }

        /// <summary>
        /// Gets the first item.
        /// </summary>
        inline const T& First() const
        {
            ENGINE_ASSERT(m_count > 0);
            return m_allocation.Get()[0];
        }
    public:
        /// <summary>
        /// Clear the collection without changing its capacity.
        /// </summary>
        void Clear()
        {
            Memory::DestructItems(m_allocation.Get(), m_count);
            m_count = 0;
        }

        /// <summary>
        /// Clears the collection without changing its capacity. Deletes all not null items.
        /// Note: collection must contain pointers to the objects that have public destructor and be allocated using New method.
        /// </summary>
        void ClearDelete()
        {
            if constexpr (TIsPointer<T>::Value)
            {
                T *data = Get();
                for (int32 i = 0; i < m_count; i++)
                {
                    if (data[i])
                    {
                        Delete(data[i]);
                    }
                }
                Clear();
            }
        }

        /// <summary>
        /// Changes the capacity of the collection.
        /// </summary>
        /// <param name="capacity">The new capacity.</param>
        /// <param name="preserveContents">True if preserve collection data when changing its size, otherwise collection after resize will be empty.</param>
        void SetCapacity(const int32 capacity, bool preserveContents = true)
        {
            if (capacity == m_capacity)
            {
                return;
            }
            ENGINE_ASSERT(capacity >= 0);
            const int32 count = preserveContents ? (m_count < capacity ? m_count : capacity) : 0;
            m_allocation.Relocate(capacity, m_count, count);
            m_capacity = capacity;
            m_count = count;
        }

        /// <summary>
        /// 将集合的大小调整为指定的大小。如果大小等于或小于当前容量，则不会执行额外的内存重新分配。
        /// </summary>
        /// <param name="size">The new collection size.</param>
        /// <param name="preserveContents">True if preserve collection data when changing its size, otherwise collection after resize might not contain the previous data.</param>
        void Resize(int32 size, bool preserveContents = true)
        {
            if (m_count > size)
            {
                Memory::DestructItems(m_allocation.Get() + size, m_count - size);
            }
            else
            {
                EnsureCapacity(size, preserveContents);
                Memory::ConstructItems(m_allocation.Get() + m_count, size - m_count);
            }
            m_count = size;
        }

        /// <summary>
        /// 确保集合具有给定的容量（或更多）。
        /// </summary>
        /// <param name="minCapacity">The minimum capacity.</param>
        /// <param name="preserveContents">True if preserve collection data when changing its size, otherwise collection after resize will be empty.</param>
        void EnsureCapacity(int32 minCapacity, bool preserveContents = true)
        {
            if (m_capacity < minCapacity)
            {
                const int32 capacity = m_allocation.CalculateCapacityGrow(m_capacity, minCapacity);
                SetCapacity(capacity, preserveContents);
            }
        }

        /// <summary>
        /// Sets all items to the given value
        /// </summary>
        /// <param name="value">The value to assign to all the collection items.</param>
        void SetAll(const T &value)
        {
            T *data = m_allocation.Get();
            for (int32 i = 0; i < m_count; i++)
            {
                data[i] = value;
            }
        }

        /// <summary>
        /// Sets the collection data.
        /// </summary>
        /// <param name="data">The data.</param>
        /// <param name="count">The amount of items.</param>
        void Set(const T *data, int32 count)
        {
            EnsureCapacity(count, false);
            Memory::DestructItems(m_allocation.Get(), m_count);
            m_count = count;
            Memory::ConstructItems(m_allocation.Get(), data, m_count);
        }

        /// <summary>
        /// Adds the specified item to the collection.
        /// </summary>
        /// <param name="item">The item to add.</param>
        void Add(const T &item)
        {
            EnsureCapacity(m_count + 1);
            Memory::ConstructItems(m_allocation.Get() + m_count, &item, 1);
            m_count++;
        }

        /// <summary>
        /// Adds the specified item to the collection.
        /// </summary>
        /// <param name="item">The item to add.</param>
        void Add(T &&item)
        {
            EnsureCapacity(m_count + 1);
            Memory::MoveItems(m_allocation.Get() + m_count, &item, 1);
            m_count++;
        }

        /// <summary>
        /// Adds the specified item to the collection.
        /// </summary>
        /// <param name="items">The items to add.</param>
        /// <param name="count">The items count.</param>
        void Add(const T *items, int32 count)
        {
            EnsureCapacity(m_count + count);
            Memory::ConstructItems(m_allocation.Get() + m_count, items, count);
            m_count += count;
        }

        /// <summary>
        /// Adds the other collection to the collection.
        /// </summary>
        /// <param name="other">The other collection to add.</param>
        template<typename OtherT, typename OtherAllocationType = AllocationType>
        inline void Add(const List<OtherT, OtherAllocationType> &other)
        {
            Add(other.Get(), other.Count());
        }

        /// <summary>
        /// Adds the unique item to the collection if it doesn't exist.
        /// </summary>
        /// <param name="item">The item to add.</param>
        inline void AddUnique(const T &item)
        {
            if (!Contains(item))
            {
                Add(item);
            }
        }

        /// <summary>
        /// Adds the given amount of items to the collection.
        /// </summary>
        /// <param name="count">The items count.</param>
        inline void AddDefault(int32 count = 1)
        {
            EnsureCapacity(m_count + count);
            Memory::ConstructItems(m_allocation.Get() + m_count, count);
            m_count += count;
        }

        /// <summary>
        /// Adds the given amount of uninitialized items to the collection without calling the constructor.
        /// </summary>
        /// <param name="count">The items count.</param>
        inline void AddUninitialized(int32 count = 1)
        {
            EnsureCapacity(m_count + count);
            m_count += count;
        }

        /// <summary>
        /// Adds the one item to the collection and returns the reference to it.
        /// </summary>
        /// <returns>The reference to the added item.</returns>
        inline T& AddOne()
        {
            EnsureCapacity(m_count + 1);
            Memory::ConstructItems(m_allocation.Get() + m_count, 1);
            m_count++;
            return m_allocation.Get()[m_count - 1];
        }

        /// <summary>
        /// Adds the new items to the end of the collection, possibly reallocating the whole collection to fit. The new items will be zeroed.
        /// </summary>
        /// <remarks>
        /// Warning! AddZeroed() will create items without calling the constructor and this is not appropriate for item types that require a constructor to function properly.
        /// </remarks>
        /// <param name="count">The number of new items to add.</param>
        void AddZeroed(int32 count = 1)
        {
            EnsureCapacity(m_count + count);
            Platform::MemoryClear(m_allocation.Get() + m_count, count * sizeof(T));
            m_count += count;
        }

        /// <summary>
        /// Insert the given item at specified index with keeping items order.
        /// </summary>
        /// <param name="index">The zero-based index at which item should be inserted.</param>
        /// <param name="item">The item to insert.</param>
        void Insert(int32 index, const T &item)
        {
            ENGINE_ASSERT(index >= 0 && index <= m_count);
            EnsureCapacity(m_count + 1);
            T *data = m_allocation.Get();
            Memory::ConstructItems(data + m_count, 1);
            for (int32 i = m_count - 1; i >= index; i--)
            {
                data[i + 1] = data[i];
            }
            m_count++;
            data[index] = item;
        }

        /// <summary>
        /// Insert the given item at specified index with keeping items order.
        /// </summary>
        /// <param name="index">The zero-based index at which item should be inserted.</param>
        void Insert(int32 index)
        {
            ENGINE_ASSERT(index >= 0 && index <= m_count);
            EnsureCapacity(m_count + 1);
            T *data = m_allocation.Get();
            Memory::ConstructItems(data + m_count, 1);
            for (int32 i = m_count - 1; i >= index; i--)
            {
                data[i + 1] = data[i];
            }
            m_count++;
        }

        /// <summary>
        /// Determines whether the collection contains the specified item.
        /// </summary>
        /// <param name="item">The item to check.</param>
        /// <returns>True if item has been found in the collection, otherwise false.</returns>
        template<typename TComparableType>
        bool Contains(const TComparableType &item) const
        {
            const T *data = m_allocation.Get();
            for (int32 i = 0; i < m_count; i++)
            {
                if (data[i] == item)
                {
                    return true;
                }
            }
            return false;
        }

        /// <summary>
        /// Removes the first occurrence of a specific object from the collection and keeps order of the items in the collection.
        /// </summary>
        /// <param name="item">The item to remove.</param>
        /// <returns>True if cannot remove item from the collection because cannot find it, otherwise false.</returns>
        bool RemoveKeepOrder(const T &item)
        {
            const int32 index = Find(item);
            if (index == -1)
            {
                return true;
            }
            RemoveAtKeepOrder(index);
            return false;
        }

        /// <summary>
        /// Removes all occurrence of a specific object from the collection and keeps order of the items in the collection.
        /// </summary>
        /// <param name="item">The item to remove.</param>
        void RemoveAllKeepOrder(const T &item)
        {
            for (int32 i = Count() - 1; i >= 0; i--)
            {
                if (m_allocation.Get()[i] == item)
                {
                    RemoveAtKeepOrder(i);
                    if (IsEmpty())
                    {
                        break;
                    }
                }
            }
        }

        /// <summary>
        /// Removes the item at the specified index of the collection and keeps order of the items in the collection.
        /// </summary>
        /// <param name="index">The zero-based index of the item to remove.</param>
        void RemoveAtKeepOrder(const int32 index)
        {
            ENGINE_ASSERT(index < m_count && index >= 0);
            m_count--;
            T *data = m_allocation.Get();
            if (index < m_count)
            {
                T *dst = data + index;
                T *src = data + (index + 1);
                const int32 count = m_count - index;
                for (int32 i = 0; i < count; i++)
                {
                    dst[i] = MoveTemp(src[i]);
                }
            }
            Memory::DestructItems(data + m_count, 1);
        }

        /// <summary>
        /// Removes the first occurrence of a specific object from the collection.
        /// </summary>
        /// <param name="item">The item to remove.</param>
        /// <returns>True if cannot remove item from the collection because cannot find it, otherwise false.</returns>
        bool Remove(const T &item)
        {
            const int32 index = Find(item);
            if (index == -1)
            {
                return false;
            }
            RemoveAt(index);
            return true;
        }

        /// <summary>
        /// Removes all occurrence of a specific object from the collection.
        /// </summary>
        /// <param name="item">The item to remove.</param>
        void RemoveAll(const T &item)
        {
            for (int32 i = Count() - 1; i >= 0; i--)
            {
                if (m_allocation.Get()[i] == item)
                {
                    RemoveAt(i);
                    if (IsEmpty())
                    {
                        break;
                    }
                }
            }
        }

        /// <summary>
        /// Removes the item at the specified index of the collection.
        /// </summary>
        /// <param name="index">The zero-based index of the item to remove.</param>
        void RemoveAt(const int32 index)
        {
            ENGINE_ASSERT(index < m_count && index >= 0);
            m_count--;
            T *data = m_allocation.Get();
            if (m_count)
            {
                data[index] = data[m_count];
            }
            Memory::DestructItems(data + m_count, 1);
        }

        /// <summary>
        /// Removes the last items from the collection.
        /// </summary>
        void RemoveLast()
        {
            ENGINE_ASSERT(m_count > 0);
            m_count--;
            Memory::DestructItems(m_allocation.Get() + m_count, 1);
        }

        /// <summary>
        /// Swaps the contents of collection with the other object without copy operation. Performs fast internal data exchange.
        /// </summary>
        /// <param name="other">The other collection.</param>
        void Swap(List &other)
        {
            if constexpr (AllocationType::HasSwap)
            {
                m_allocation.Swap(other.m_allocation);
                ::Swap(m_count, other.m_count);
                ::Swap(m_capacity, other.m_capacity);
            }
            else
            {
                ::Swap(other, *this);
            }
        }

        /// <summary>
        /// Reverses the order of the added items in the collection.
        /// </summary>
        void Reverse()
        {
            T *data = m_allocation.Get();
            const int32 count = m_count / 2;
            for (int32 i = 0; i < count; i++)
            {
                ::Swap(data[i], data[m_count - i - 1]);
            }
        }

    public:
        /// <summary>
        /// Performs push on stack operation (stack grows at the end of the collection).
        /// </summary>
        /// <param name="item">The item to append.</param>
        inline void Push(const T &item)
        {
            Add(item);
        }

        /// <summary>
        /// Performs pop from stack operation (stack grows at the end of the collection).
        /// </summary>
        T Pop()
        {
            T item(Last());
            RemoveLast();
            return item;
        }

        /// <summary>
        /// Peeks items which is at the top of the stack (stack grows at the end of the collection).
        /// </summary>
        inline T &Peek()
        {
            ENGINE_ASSERT(m_count > 0);
            return m_allocation.Get()[m_count - 1];
        }

        /// <summary>
        /// Peeks items which is at the top of the stack (stack grows at the end of the collection).
        /// </summary>
        inline const T &Peek() const
        {
            ENGINE_ASSERT(m_count > 0);
            return m_allocation.Get()[m_count - 1];
        }

    public:
        /// <summary>
        /// Performs enqueue to queue operation (queue head is in the beginning of queue).
        /// </summary>
        /// <param name="item">The item to append.</param>
        void Enqueue(const T &item)
        {
            Add(item);
        }

        /// <summary>
        /// Performs dequeue from queue operation (queue head is in the beginning of queue).
        /// </summary>
        /// <returns>The item.</returns>
        T Dequeue()
        {
            ENGINE_ASSERT(HasItems());
            T item(First());
            RemoveAtKeepOrder(0);
            return item;
        }

    public:
        /// <summary>
        /// Searches for the given item within the entire collection.
        /// </summary>
        /// <param name="item">The item to look for.</param>
        /// <param name="index">The found item index, -1 if missing.</param>
        /// <returns>True if found, otherwise false.</returns>
        template<typename ComparableType>
        inline bool Find(const ComparableType &item, int32 &index) const
        {
            index = Find(item);
            return index != -1;
        }

        /// <summary>
        /// Searches for the specified object and returns the zero-based index of the first occurrence within the entire collection.
        /// </summary>
        /// <param name="item">The item to find.</param>
        /// <returns>The zero-based index of the first occurrence of itm within the entire collection, if found; otherwise, -1.</returns>
        template<typename ComparableType>
        int32 Find(const ComparableType &item) const
        {
            if (m_count > 0)
            {
                const T *start = m_allocation.Get();
                for (const T *data = start, *dataEnd = data + m_count; data != dataEnd; ++data)
                {
                    if (*data == item)
                    {
                        return static_cast<int32>(data - start);
                    }
                }
            }
            return -1;
        }

        /// <summary>
        /// Searches for the given item within the entire collection starting from the end.
        /// </summary>
        /// <param name="item">The item to look for.</param>
        /// <param name="index">The found item index, -1 if missing.</param>
        /// <returns>True if found, otherwise false.</returns>
        template<typename ComparableType>
        inline bool FindLast(const ComparableType &item, int &index) const
        {
            index = FindLast(item);
            return index != INVALID_INDEX;
        }

		template<typename ComparableType>
		inline bool FindFirst(const ComparableType &item, int &index) const
		{
			index = FindFirst(item);
			return index != INVALID_INDEX;
		}

        /// <summary>
        /// Searches for the specified object and returns the zero-based index of the first occurrence within the entire collection  starting from the end.
        /// </summary>
        /// <param name="item">The item to find.</param>
        /// <returns>The zero-based index of the first occurrence of itm within the entire collection, if found; otherwise, -1.</returns>
        template<typename ComparableType>
        int32 FindLast(const ComparableType &item) const
        {
            if (m_count > 0)
            {
                const T *end = m_allocation.Get() + m_count;
                for (const T *data = end, *dataStart = data - m_count; data != dataStart;)
                {
                    --data;
                    if (*data == item)
                    {
                        return static_cast<int32>(data - dataStart);
                    }
                }
            }
            return INVALID_INDEX;
        }

		template<typename ComparableType>
		int32 FindFirst(const ComparableType &item) const
		{
			if (m_count > 0)
			{
				const T *start = m_allocation.Get();
				for (const T *data = start, *dataStart = start + m_count; data != dataStart;)
				{
					++data;
					if (*data == item)
					{
						return static_cast<int32>(data - dataStart);
					}
				}
			}
			return INVALID_INDEX;
		}

    public:
        template<typename OtherT = T, typename OtherAllocationType = AllocationType>
        bool operator==(const List<OtherT, OtherAllocationType> &other) const
        {
            if (m_count == other.Count())
            {
                const T *data = m_allocation.Get();
                const T *otherData = other.Get();
                for (int32 i = 0; i < m_count; i++)
                {
                    if (data[i] != otherData[i])
                    {
                        return false;
                    }
                }
                return true;
            }
            return false;
        }

        template<typename OtherT = T, typename OtherAllocationType = AllocationType>
        bool operator!=(const List<OtherT, OtherAllocationType> &other) const
        {
            return !operator==(other);
        }

    public:
        /// <summary>
        /// The collection iterator.
        /// </summary>
        struct Iterator
        {
            friend List;
        private:
            List *_array;
            int32 _index;

            Iterator(List *array, const int32 index)
                    : _array(array), _index(index)
            {
            }

            Iterator(List const *array, const int32 index)
                    : _array(const_cast<List *>(array)), _index(index)
            {
            }

        public:
            Iterator()
                    : _array(nullptr), _index(-1)
            {
            }

            Iterator(const Iterator &i)
                    : _array(i._array), _index(i._index)
            {
            }

			Iterator(Iterator &i)
				: _array(i._array), _index(i._index)
			{
			}

            Iterator(Iterator &&i)
                    : _array(i._array), _index(i._index)
            {
            }

        public:
            inline List* GetArray() const
            {
                return _array;
            }

            inline int32 GetIndex() const
            {
                return _index;
            }

            inline bool IsEnd() const
            {
                return _index == _array->m_count;
            }

            inline bool IsNotEnd() const
            {
                return _index != _array->m_count;
            }

            inline T& operator*() const
            {
                return _array->Get()[_index];
            }

            inline T* operator->() const
            {
                return &_array->Get()[_index];
            }

            inline bool operator==(const Iterator &v) const
            {
                return _array == v._array && _index == v._index;
            }

            inline bool operator!=(const Iterator &v) const
            {
                return _array != v._array || _index != v._index;
            }

			inline int operator-(const Iterator &v) const
			{
				return _index - v._index;
			}

            Iterator &operator=(const Iterator &v)
            {
                _array = v._array;
                _index = v._index;
                return *this;
            }

            Iterator &operator++()
            {
                if (_index != _array->m_count)
                {
                    _index++;
                }
                return *this;
            }

            Iterator operator++(int)
            {
                Iterator temp = *this;
                if (_index != _array->m_count)
                {
                    _index++;
                }
                return temp;
            }

            Iterator &operator--()
            {
                if (_index > 0)
                {
                    _index--;
                }
                return *this;
            }

            Iterator operator--(int)
            {
                Iterator temp = *this;
                if (_index > 0)
                {
                    _index--;
                }
                return temp;
            }
        };

    public:
		/// <summary>
		/// Gets iterator for beginning of the collection.
		/// </summary>
		inline Iterator begin()
		{
			return Iterator(this, 0);
		}

		/// <summary>
		/// Gets iterator for ending of the collection.
		/// </summary>
		inline Iterator end()
		{
			return Iterator(this, m_count);
		}


        /// <summary>
        /// Gets iterator for beginning of the collection.
        /// </summary>
        inline Iterator begin() const
        {
            return Iterator(this, 0);
        }

        /// <summary>
        /// Gets iterator for ending of the collection.
        /// </summary>
        inline Iterator end() const
        {
            return Iterator(this, m_count);
        }
    };
}

template<typename T, typename AllocationType>
inline void *operator new(size_t size, SE::List<T, AllocationType> &array)
{
    ENGINE_ASSERT(size == sizeof(T));
    const int32 index = array.Count();
    array.AddUninitialized(1);
    return &array[index];
}

template<typename T, typename AllocationType>
inline void *operator new(size_t size, SE::List<T, AllocationType> &array, int32 index)
{
    ENGINE_ASSERT(size == sizeof(T));
    array.Insert(index);
    return &array[index];
}