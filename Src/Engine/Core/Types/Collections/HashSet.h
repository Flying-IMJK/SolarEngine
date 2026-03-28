#pragma once

#include "Core/Memory/Memory.h"
#include "Core/Memory/Allocation.h"
#include "../Hash.h"
#include "Config.h"

#include <initializer_list>


namespace SE
{

    /// <summary>
    /// Template for unordered set of values (without duplicates with O(1) lookup access).
    /// </summary>
    /// <typeparam name="T">The type of elements in the set.</typeparam>
    /// <typeparam name="AllocationType">The type of memory allocator.</typeparam>
    template<typename T, typename AllocationType = HeapAllocation>
    class HashSet
    {
        friend HashSet;
    public:
        /// <summary>
        /// Describes single portion of space for the item in a hash map.
        /// </summary>
        struct Bucket
        {
            friend HashSet;

            enum State : byte
            {
                Empty,
                Deleted,
                Occupied,
            };

            /// <summary>The item.</summary>
            T Item;

        private:
            State m_State;

            inline void Free()
            {
                if (m_State == Occupied)
                    Memory::DestructItem(&Item);
                m_State = Empty;
            }

            inline void Delete()
            {
                m_State = Deleted;
                Memory::DestructItem(&Item);
            }

            template<typename ItemType>
            inline void Occupy(const ItemType &item)
            {
                Memory::ConstructItems(&Item, &item, 1);
                m_State = Occupied;
            }

            template<typename ItemType>
            inline void Occupy(ItemType &item)
            {
                Memory::MoveItems(&Item, &item, 1);
                m_State = Occupied;
            }

            inline bool IsEmpty() const
            {
                return m_State == Empty;
            }

            inline bool IsDeleted() const
            {
                return m_State == Deleted;
            }

            inline bool IsOccupied() const
            {
                return m_State == Occupied;
            }

            inline bool IsNotOccupied() const
            {
                return m_State != Occupied;
            }
        };

        typedef typename AllocationType::template Data<Bucket> AllocationData;

    private:
        int32 m_elementsCount = 0;
        int32 m_deletedCount = 0;
        int32 m_size = 0;
        AllocationData m_allocation;

        inline static void MoveToEmpty(AllocationData &to, AllocationData &from, int32 fromSize)
        {
            if constexpr (AllocationType::HasSwap)
            to.Swap(from);
            else
            {
                to.Allocate(fromSize);
                Bucket *toData = to.Get();
                Bucket *fromData = from.Get();
                for (int32 i = 0; i < fromSize; i++)
                {
                    Bucket &fromBucket = fromData[i];
                    if (fromBucket.IsOccupied())
                    {
                        Bucket &toBucket = toData[i];
                        Memory::MoveItems(&toBucket.Item, &fromBucket.Item, 1);
                        toBucket.m_State = Bucket::Occupied;
                        Memory::DestructItem(&fromBucket.Item);
                        fromBucket.m_State = Bucket::Empty;
                    }
                }
                from.Free();
            }
        }

    public:
        /// <summary>
        /// Initializes a new instance of the <see cref="HashSet"/> class.
        /// </summary>
        HashSet()
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="HashSet"/> class.
        /// </summary>
        /// <param name="capacity">The initial capacity.</param>
        HashSet(int32 capacity)
        {
            SetCapacity(capacity);
        }

		HashSet(const std::initializer_list<T> &initList)
		{
			SetCapacity(initList.size());

			for (int i = 0; i < initList.size(); ++i)
			{
				Add(*(initList.begin() + i));
			}
		}

        /// <summary>
        /// Initializes a new instance of the <see cref="HashSet"/> class.
        /// </summary>
        /// <param name="other">The other collection to move.</param>
        HashSet(HashSet &&other) noexcept
        {
            m_elementsCount = other.m_elementsCount;
            m_deletedCount = other.m_deletedCount;
            m_size = other.m_size;
            other.m_elementsCount = 0;
            other.m_deletedCount = 0;
            other.m_size = 0;
            MoveToEmpty(m_allocation, other.m_allocation, m_size);
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="HashSet"/> class.
        /// </summary>
        /// <param name="other">Other collection to copy</param>
        HashSet(const HashSet &other)
        {
            Clone(other);
        }

        /// <summary>
        /// Clones the data from the other collection.
        /// </summary>
        /// <param name="other">The other collection to copy.</param>
        /// <returns>The reference to this.</returns>
        HashSet &operator=(const HashSet &other)
        {
            if (this != &other)
                Clone(other);
            return *this;
        }

        /// <summary>
        /// Moves the data from the other collection.
        /// </summary>
        /// <param name="other">The other collection to move.</param>
        /// <returns>The reference to this.</returns>
        HashSet &operator=(HashSet &&other) noexcept
        {
            if (this != &other)
            {
                Clear();
                m_allocation.Free();
                m_elementsCount = other.m_elementsCount;
                m_deletedCount = other.m_deletedCount;
                m_size = other.m_size;
                other.m_elementsCount = 0;
                other.m_deletedCount = 0;
                other.m_size = 0;
                MoveToEmpty(m_allocation, other.m_allocation, m_size);
            }
            return *this;
        }

        /// <summary>
        /// Finalizes an instance of the <see cref="HashSet"/> class.
        /// </summary>
        ~HashSet()
        {
            Clear();
        }

    public:
        /// <summary>
        /// Gets the amount of the elements in the collection.
        /// </summary>
        inline int32

        Count() const
        {
            return m_elementsCount;
        }

        /// <summary>
        /// Gets the amount of the elements that can be contained by the collection.
        /// </summary>
        inline int32

        Capacity() const
        {
            return m_size;
        }

        /// <summary>
        /// Returns true if collection is empty.
        /// </summary>
        inline bool IsEmpty() const
        {
            return m_elementsCount == 0;
        }

        /// <summary>
        /// Returns true if collection has one or more elements.
        /// </summary>
        inline bool HasItems() const
        {
            return m_elementsCount != 0;
        }

    public:
        /// <summary>
        /// The hash set collection iterator.
        /// </summary>
        struct Iterator
        {
            friend HashSet;
        private:
            HashSet *m_collection;
            int32 m_index;

        public:
            Iterator(HashSet *collection, const int32 index)
                    : m_collection(collection), m_index(index)
            {
            }

            Iterator(HashSet const *collection, const int32 index)
                    : m_collection(const_cast<HashSet *>(collection)), m_index(index)
            {
            }

            Iterator()
                    : m_collection(nullptr), m_index(-1)
            {
            }

            Iterator(const Iterator &i)
                    : m_collection(i.m_collection), m_index(i.m_index)
            {
            }

            Iterator(Iterator &&i)
                    : m_collection(i.m_collection), m_index(i.m_index)
            {
            }

        public:
            inline int32

            Index() const
            {
                return m_index;
            }

            inline bool IsEnd() const
            {
                return m_index == m_collection->m_size;
            }

            inline bool IsNotEnd() const
            {
                return m_index != m_collection->m_size;
            }

            inline T& operator*() const
            {
                return m_collection->m_allocation.Get()[m_index].Item;
            }

            inline Bucket* operator->() const
            {
                return &m_collection->m_allocation.Get()[m_index];
            }

            inline explicit operator bool() const
            {
                return m_index >= 0 && m_index < m_collection->m_size;
            }

            inline bool operator!() const
            {
                return !(bool) *this;
            }

            inline bool operator==(const Iterator &v) const
            {
                return m_index == v.m_index && m_collection == v.m_collection;
            }

            inline bool operator!=(const Iterator &v) const
            {
                return m_index != v.m_index || m_collection != v.m_collection;
            }

            Iterator &operator=(const Iterator &v)
            {
                m_collection = v.m_collection;
                m_index = v.m_index;
                return *this;
            }

            Iterator &operator++()
            {
                const int32 capacity = m_collection->m_size;
                if (m_index != capacity)
                {
                    const Bucket *data = m_collection->m_allocation.Get();
                    do
                    {
                        m_index++;
                    } while (m_index != capacity && data[m_index].IsNotOccupied());
                }
                return *this;
            }

            Iterator operator++(int)
            {
                Iterator i = *this;
                ++i;
                return i;
            }

            Iterator &operator--()
            {
                if (m_index > 0)
                {
                    const Bucket *data = m_collection->m_allocation.Get();
                    do
                    {
                        m_index--;
                    } while (m_index > 0 && data[m_index].IsNotOccupied());
                }
                return *this;
            }

            Iterator operator--(int)
            {
                Iterator i = *this;
                --i;
                return i;
            }
        };

    public:
        /// <summary>
        /// Removes all elements from the collection.
        /// </summary>
        void Clear()
        {
            if (m_elementsCount + m_deletedCount != 0)
            {
                Bucket *data = m_allocation.Get();
                for (int32 i = 0; i < m_size; i++)
                    data[i].Free();
                m_elementsCount = m_deletedCount = 0;
            }
        }

        /// <summary>
        /// Clears the collection and delete value objects.
        /// Note: collection must contain pointers to the objects that have public destructor and be allocated using New method.
        /// </summary>
        void ClearDelete()
        {
			if (TIsPointer<T>::Value)
			{
				for (Iterator i = begin(); i.IsNotEnd(); ++i)
				{
					if (i->Item)
						Delete(i->Item);
				}
			}
            Clear();
        }

        /// <summary>
        /// Changes capacity of the collection
        /// </summary>
        /// <param name="capacity">New capacity</param>
        /// <param name="preserveContents">Enable/disable preserving collection contents during resizing</param>
        void SetCapacity(int32 capacity, bool preserveContents = true)
        {
            if (capacity == Capacity())
                return;
            ENGINE_ASSERT(capacity >= 0);
            AllocationData oldAllocation;
            MoveToEmpty(oldAllocation, m_allocation, m_size);
            const int32 oldSize = m_size;
            const int32 oldElementsCount = m_elementsCount;
            m_deletedCount = m_elementsCount = 0;
            if (capacity != 0 && (capacity & (capacity - 1)) != 0)
            {
                // Align capacity value to the next power of two (http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2)
                capacity--;
                capacity |= capacity >> 1;
                capacity |= capacity >> 2;
                capacity |= capacity >> 4;
                capacity |= capacity >> 8;
                capacity |= capacity >> 16;
                capacity++;
            }
            if (capacity)
            {
                m_allocation.Allocate(capacity);
                Bucket *data = m_allocation.Get();
                for (int32 i = 0; i < capacity; i++)
                    data[i].m_State = Bucket::Empty;
            }
            m_size = capacity;
            Bucket *oldData = oldAllocation.Get();
            if (oldElementsCount != 0 && capacity != 0 && preserveContents)
            {
                FindPositionResult pos;
                for (int32 i = 0; i < oldSize; i++)
                {
                    Bucket &oldBucket = oldData[i];
                    if (oldBucket.IsOccupied())
                    {
                        FindPosition(oldBucket.Item, pos);
                        ENGINE_ASSERT(pos.FreeSlotIndex != -1);
                        Bucket *bucket = &m_allocation.Get()[pos.FreeSlotIndex];
                        Memory::MoveItems(&bucket->Item, &oldBucket.Item, 1);
                        bucket->m_State = Bucket::Occupied;
                        m_elementsCount++;
                    }
                }
            }
            if (oldElementsCount != 0)
            {
                for (int32 i = 0; i < oldSize; i++)
                    oldData[i].Free();
            }
        }

        /// <summary>
        /// Ensures that collection has given capacity.
        /// </summary>
        /// <param name="minCapacity">The minimum required capacity.</param>
        /// <param name="preserveContents">True if preserve collection data when changing its size, otherwise collection after resize will be empty.</param>
        void EnsureCapacity(int32 minCapacity, bool preserveContents = true)
        {
            if (m_size >= minCapacity)
                return;
            int32 capacity = m_allocation.CalculateCapacityGrow(m_size, minCapacity);
            if (capacity < DICTIONARY_DEFAULT_CAPACITY)
                capacity = DICTIONARY_DEFAULT_CAPACITY;
            SetCapacity(capacity, preserveContents);
        }

        /// <summary>
        /// Swaps the contents of collection with the other object without copy operation. Performs fast internal data exchange.
        /// </summary>
        /// <param name="other">The other collection.</param>
        void Swap(HashSet &other)
        {
            if constexpr(AllocationType::HasSwap)
            {
                ::Swap(m_elementsCount, other.m_elementsCount);
                ::Swap(m_deletedCount, other.m_deletedCount);
                ::Swap(m_size, other.m_size);
                m_allocation.Swap(other.m_allocation);
            }
            else
            {
                HashSet tmp = MoveTemp(other);
                other = *this;
                *this = MoveTemp(tmp);
            }
        }

    public:
        /// <summary>
        /// Add element to the collection.
        /// </summary>
        /// <param name="item">The element to add to the set.</param>
        /// <returns>True if element has been added to the collection, otherwise false if the element is already present.</returns>
        template<typename ItemType>
        bool Add(const ItemType &item)
        {
            Bucket *bucket = OnAdd(item);
            if (bucket)
                bucket->Occupy(item);
            return bucket != nullptr;
        }

        /// <summary>
        /// Add element to the collection.
        /// </summary>
        /// <param name="item">The element to add to the set.</param>
        /// <returns>True if element has been added to the collection, otherwise false if the element is already present.</returns>
        bool Add(T &&item)
        {
            Bucket *bucket = OnAdd(item);
            if (bucket)
                bucket->Occupy(MoveTemp(item));
            return bucket != nullptr;
        }

        /// <summary>
        /// Add element at iterator to the collection
        /// </summary>
        /// <param name="i">Iterator with item to add</param>
        void Add(const Iterator &i)
        {
            ENGINE_ASSERT(i.m_collection != this && i);
            const Bucket *bucket = i.operator->();
            Add(bucket->Item);
        }

        /// <summary>
        /// Removes the specified element from the collection.
        /// </summary>
        /// <param name="item">The element to remove.</param>
        /// <returns>True if cannot remove item from the collection because cannot find it, otherwise false.</returns>
        template<typename ItemType>
        bool Remove(const ItemType &item)
        {
            if (IsEmpty())
                return false;
            FindPositionResult pos;
            FindPosition(item, pos);
            if (pos.ObjectIndex != -1)
            {
                m_allocation.Get()[pos.ObjectIndex].Delete();
                m_elementsCount--;
                m_deletedCount++;
                return true;
            }
            return false;
        }

        /// <summary>
        /// Removes an element at specified iterator position.
        /// </summary>
        /// <param name="i">The element iterator to remove.</param>
        /// <returns>True if cannot remove item from the collection because cannot find it, otherwise false.</returns>
        bool Remove(const Iterator &i)
        {
            ENGINE_ASSERT(i.m_collection == this);
            if (i)
            {
                ENGINE_ASSERT(m_allocation.Get()[i.m_index].IsOccupied());
                m_allocation.Get()[i.m_index].Delete();
                m_elementsCount--;
                m_deletedCount++;
                return true;
            }
            return false;
        }

    public:
        /// <summary>
        /// Find element with given item in the collection
        /// </summary>
        /// <param name="item">Item to find</param>
        /// <returns>Iterator for the found element or End if cannot find it</returns>
        template<typename ItemType>
        Iterator Find(const ItemType &item) const
        {
            if (IsEmpty())
                return end();
            FindPositionResult pos;
            FindPosition(item, pos);
            return pos.ObjectIndex != -1 ? Iterator(this, pos.ObjectIndex) : end();
        }

        /// <summary>
        /// Determines whether a collection contains the specified element.
        /// </summary>
        /// <param name="item">The item to locate.</param>
        /// <returns>True if value has been found in a collection, otherwise false</returns>
        template<typename ItemType>
        bool Contains(const ItemType &item) const
        {
            if (IsEmpty())
                return false;
            FindPositionResult pos;
            FindPosition(item, pos);
            return pos.ObjectIndex != -1;
        }

    public:
        /// <summary>
        /// Clones other collection into this
        /// </summary>
        /// <param name="other">Other collection to clone</param>
        void Clone(const HashSet &other)
        {
            Clear();
            SetCapacity(other.Capacity(), false);
            for (Iterator i = other.begin(); i != other.end(); ++i)
                Add(i);
            ENGINE_ASSERT(Count() == other.Count());
            ENGINE_ASSERT(Capacity() == other.Capacity());
        }

    public:
        Iterator begin()
        {
            Iterator iterator(this, -1);
            ++iterator;
            return iterator;
        }

        inline Iterator end()
        {
            return Iterator(this, m_size);
        }

        const Iterator begin() const
        {
            Iterator iterator(this, -1);
            ++iterator;
            return iterator;
        }

        inline const Iterator end() const
        {
            return Iterator(this, m_size);
        }

    private:
        /// <summary>
        /// The result container of the set item lookup searching.
        /// </summary>
        struct FindPositionResult
        {
            int32 ObjectIndex;
            int32 FreeSlotIndex;
        };

        /// <summary>
        /// Returns a pair of positions: 1st where the object is, 2nd where
        /// it would go if you wanted to insert it. 1st is -1
        /// if object is not found; 2nd is -1 if it is.
        /// Note: because of deletions where-to-insert is not trivial: it's the
        /// first deleted bucket we see, as long as we don't find the item later
        /// </summary>
        /// <param name="item">The item to find</param>
        /// <param name="result">Pair of values: where the object is and where it would go if you wanted to insert it</param>
        template<typename ItemType>
        void FindPosition(const ItemType &item, FindPositionResult &result) const
        {
            ENGINE_ASSERT(m_size);
            const int32 tableSizeMinusOne = m_size - 1;
            int32 bucketIndex = GetHash(item) & tableSizeMinusOne;
            int32 insertPos = -1;
            int32 numChecks = 0;
            const Bucket *data = m_allocation.Get();
            result.FreeSlotIndex = -1;
            while (numChecks < m_size)
            {
                // Empty bucket
                const Bucket &bucket = data[bucketIndex];
                if (bucket.IsEmpty())
                {
                    // Found place to insert
                    result.ObjectIndex = -1;
                    result.FreeSlotIndex = insertPos == -1 ? bucketIndex : insertPos;
                    return;
                }
                // Deleted bucket
                if (bucket.IsDeleted())
                {
                    // Keep searching but mark to insert
                    if (insertPos == -1)
                        insertPos = bucketIndex;
                }
                    // Occupied bucket by target item
                else if (bucket.Item == item)
                {
                    // Found item
                    result.ObjectIndex = bucketIndex;
                    return;
                }

                numChecks++;
                bucketIndex = (bucketIndex + DICTIONARY_PROB_FUNC(m_size, numChecks)) & tableSizeMinusOne;
            }
            result.ObjectIndex = -1;
            result.FreeSlotIndex = insertPos;
        }

        template<typename ItemType>
        Bucket *OnAdd(const ItemType &key)
        {
            // Check if need to rehash elements (prevent many deleted elements that use too much of capacity)
            if (m_deletedCount > m_size / DICTIONARY_DEFAULT_SLACK_SCALE)
                Compact();

            // Ensure to have enough memory for the next item (in case of new element insertion)
            EnsureCapacity((m_elementsCount + 1) * DICTIONARY_DEFAULT_SLACK_SCALE + m_deletedCount);

            // Find location of the item or place to insert it
            FindPositionResult pos;
            FindPosition(key, pos);

            // Check if object has been already added
            if (pos.ObjectIndex != -1)
                return nullptr;

            // Insert
            ENGINE_ASSERT(pos.FreeSlotIndex != -1);
            m_elementsCount++;
            return &m_allocation.Get()[pos.FreeSlotIndex];
        }

        void Compact()
        {
            if (m_elementsCount == 0)
            {
                // Fast path if it's empty
                Bucket *data = m_allocation.Get();
                for (int32 i = 0; i < m_size; i++)
                    data[i].m_State = Bucket::Empty;
            }
            else
            {
                // Rebuild entire table completely
                AllocationData oldAllocation;
                MoveToEmpty(oldAllocation, m_allocation, m_size);
                m_allocation.Allocate(m_size);
                Bucket *data = m_allocation.Get();
                for (int32 i = 0; i < m_size; i++)
                    data[i].m_State = Bucket::Empty;
                Bucket *oldData = oldAllocation.Get();
                FindPositionResult pos;
                for (int32 i = 0; i < m_size; i++)
                {
                    Bucket &oldBucket = oldData[i];
                    if (oldBucket.IsOccupied())
                    {
                        FindPosition(oldBucket.Item, pos);
                        ENGINE_ASSERT(pos.FreeSlotIndex != -1);
                        Bucket *bucket = &m_allocation.Get()[pos.FreeSlotIndex];
                        Memory::MoveItems(&bucket->Item, &oldBucket.Item, 1);
                        bucket->m_State = Bucket::Occupied;
                    }
                }
                for (int32 i = 0; i < m_size; i++)
                    oldData[i].Free();
            }
            m_deletedCount = 0;
        }
    };
}