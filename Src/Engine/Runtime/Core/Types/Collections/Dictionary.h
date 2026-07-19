#pragma once

#include "Runtime/Core/Memory/Memory.h"
#include "Runtime/Core/Types/Hash.h"
#include "Runtime/Core/Types/Pair.h"
#include "Runtime/Core/Logging/Logging.h"
#include "Config.h"

#include <initializer_list>


namespace SE
{
    /// <summary>
    /// Template for unordered dictionary with mapped key with Value pairs.
    /// </summary>
    /// <typeparam name="KeyType">The type of the keys in the dictionary.</typeparam>
    /// <typeparam name="ValueType">The type of the values in the dictionary.</typeparam>
    /// <typeparam name="AllocationType">The type of memory allocator.</typeparam>
    template<typename KeyType, typename ValueType, typename AllocationType>
    class Dictionary
    {
        friend Dictionary;
    public:
        /// <summary>
        /// Describes single portion of space for the key and Value pair in a hash map.
        /// </summary>
        struct Bucket
        {
            friend Dictionary;

            enum State : byte
            {
                Empty = 0,
                Deleted = 1,
                Occupied = 2,
            };

            /// <summary>The key.</summary>
            KeyType Key;
            /// <summary>The Value.</summary>
            ValueType Value;

        private:
            State m_State;

            inline void Free()
            {
                if (m_State == Occupied)
                {
                    Memory::DestructItem(&Key);
                    Memory::DestructItem(&Value);
                }
                m_State = Empty;
            }

            inline void Delete()
            {
                m_State = Deleted;
                Memory::DestructItem(&Key);
                Memory::DestructItem(&Value);
            }

            template<typename KeyComparableType>
            inline void Occupy(const KeyComparableType &key)
            {
                Memory::ConstructItems(&this->Key, &key, 1);
                Memory::ConstructItem(&this->Value);
                m_State = Occupied;
            }

            template<typename KeyComparableType>
            inline void Occupy(const KeyComparableType &key, const ValueType &value)
            {
                Memory::ConstructItems(&this->Key, &key, 1);
                Memory::ConstructItems(&this->Value, &value, 1);
                m_State = Occupied;
            }

            template<typename KeyComparableType>
            inline void Occupy(const KeyComparableType &key, ValueType &&value)
            {
                Memory::ConstructItems(&this->Key, &key, 1);
                Memory::MoveItems(&this->Value, &value, 1);
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
                        Memory::MoveItems(&toBucket.Key, &fromBucket.Key, 1);
                        Memory::MoveItems(&toBucket.Value, &fromBucket.Value, 1);
                        toBucket.m_State = Bucket::Occupied;
                        Memory::DestructItem(&fromBucket.Key);
                        Memory::DestructItem(&fromBucket.Value);
                        fromBucket.m_State = Bucket::Empty;
                    }
                }
                from.Free();
            }
        }

    public:
        /// <summary>
        /// Initializes a new instance of the <see cref="Dictionary"/> class.
        /// </summary>
        Dictionary()
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Dictionary"/> class.
        /// </summary>
        /// <param name="capacity">The initial capacity.</param>
        Dictionary(int32 capacity)
        {
            SetCapacity(capacity);
        }

		Dictionary(const std::initializer_list<Pair<KeyType, ValueType>> &initList)
		{
			SetCapacity(initList.size());

			for (int i = 0; i < initList.size(); ++i)
			{
				auto pair = initList.begin() + i;
				Add(pair->First, pair->Second);
			}
		}

        /// <summary>
        /// Initializes a new instance of the <see cref="Dictionary"/> class.
        /// </summary>
        /// <param name="other">The other collection to move.</param>
        Dictionary(Dictionary &&other) noexcept
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
        /// Initializes a new instance of the <see cref="Dictionary"/> class.
        /// </summary>
        /// <param name="other">Other collection to copy</param>
        Dictionary(const Dictionary &other)
        {
            Clone(other);
        }

        /// <summary>
        /// Clones the data from the other collection.
        /// </summary>
        /// <param name="other">The other collection to copy.</param>
        /// <returns>The reference to this.</returns>
        Dictionary &operator=(const Dictionary &other)
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
        Dictionary &operator=(Dictionary &&other) noexcept
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
        /// Finalizes an instance of the <see cref="Dictionary"/> class.
        /// </summary>
        ~Dictionary()
        {
            Clear();
        }

    public:
        /// <summary>
        /// Gets the amount of the elements in the collection.
        /// </summary>
        inline int32 Count() const
        {
            return m_elementsCount;
        }

        /// <summary>
        /// Gets the amount of the elements that can be contained by the collection.
        /// </summary>
        inline int32 Capacity() const
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
        /// The Dictionary collection iterator.
        /// </summary>
        struct Iterator
        {
            friend Dictionary;
        private:
            Dictionary *_collection;
            int32 _index;

        public:
            Iterator(Dictionary *collection, const int32 index)
                    : _collection(collection), _index(index)
            {
            }

            Iterator(Dictionary const *collection, const int32 index)
                    : _collection(const_cast<Dictionary *>(collection)), _index(index)
            {
            }

            Iterator()
                    : _collection(nullptr), _index(-1)
            {
            }

            Iterator(const Iterator &i)
                    : _collection(i._collection), _index(i._index)
            {
            }

            Iterator(Iterator &&i)
                    : _collection(i._collection), _index(i._index)
            {
            }

        public:
            inline int32

            Index() const
            {
                return _index;
            }

            inline bool IsEnd() const
            {
                return _index == _collection->m_size;
            }

            inline bool IsNotEnd() const
            {
                return _index != _collection->m_size;
            }

            inline Bucket& operator*() const
            {
                return _collection->m_allocation.Get()[_index];
            }

            inline Bucket* operator->() const
            {
                return &_collection->m_allocation.Get()[_index];
            }

            inline explicit operator bool() const
            {
                return _index >= 0 && _index < _collection->m_size;
            }

            inline bool operator!() const
            {
                return !(bool) *this;
            }

            inline bool operator==(const Iterator &v) const
            {
                return _index == v._index && _collection == v._collection;
            }

            inline bool operator!=(const Iterator &v) const
            {
                return _index != v._index || _collection != v._collection;
            }

            Iterator &operator=(const Iterator &v)
            {
                _collection = v._collection;
                _index = v._index;
                return *this;
            }

            Iterator &operator++()
            {
                const int32 capacity = _collection->m_size;
                if (_index != capacity)
                {
                    const Bucket *data = _collection->m_allocation.Get();
                    do
                    {
                        _index++;
                    } while (_index != capacity && data[_index].IsNotOccupied());
                }
                return *this;
            }

            Iterator operator++(int) const
            {
                Iterator i = *this;
                ++i;
                return i;
            }

            Iterator &operator--()
            {
                if (_index > 0)
                {
                    const Bucket *data = _collection->m_allocation.Get();
                    do
                    {
                        _index--;
                    } while (_index > 0 && data[_index].IsNotOccupied());
                }
                return *this;
            }

            Iterator operator--(int) const
            {
                Iterator i = *this;
                --i;
                return i;
            }
        };

    public:
        /// <summary>
        /// Gets element by the key (will add default ValueType element if key not found).
        /// </summary>
        /// <param name="key">The key of the element.</param>
        /// <returns>The Value that is at given index.</returns>
        template<typename KeyComparableType>
        ValueType &At(const KeyComparableType &key)
        {
            // Check if need to rehash elements (prevent many deleted elements that use too much of capacity)
            if (m_deletedCount > m_size / DICTIONARY_DEFAULT_SLACK_SCALE)
                Compact();

            // Ensure to have enough memory for the next item (in case of new element insertion)
            EnsureCapacity((m_elementsCount + 1) * DICTIONARY_DEFAULT_SLACK_SCALE + m_deletedCount);

            // Find location of the item or place to insert it
            FindPositionResult pos;
            FindPosition(key, pos);

            // Check if that key has been already added
            if (pos.ObjectIndex != -1)
                return m_allocation.Get()[pos.ObjectIndex].Value;

            // Insert
            ENGINE_ASSERT(pos.FreeSlotIndex != -1);
            m_elementsCount++;
            Bucket &bucket = m_allocation.Get()[pos.FreeSlotIndex];
            bucket.Occupy(key);
            return bucket.Value;
        }

        /// <summary>
        /// Gets the element by the key.
        /// </summary>
        /// <param name="key">The ky of the element.</param>
        /// <returns>The Value that is at given index.</returns>
        template<typename KeyComparableType>
        const ValueType &At(const KeyComparableType &key) const
        {
            FindPositionResult pos;
            FindPosition(key, pos);
            ENGINE_ASSERT(pos.ObjectIndex != -1);
            return m_allocation.Get()[pos.ObjectIndex].Value;
        }

        /// <summary>
        /// Gets or sets the element by the key.
        /// </summary>
        /// <param name="key">The key of the element.</param>
        /// <returns>The Value that is at given index.</returns>
        template<typename KeyComparableType>
        inline ValueType& operator[](const KeyComparableType &key)
        {
            return At(key);
        }

        /// <summary>
        /// Gets or sets the element by the key.
        /// </summary>
        /// <param name="key">The ky of the element.</param>
        /// <returns>The Value that is at given index.</returns>
        template<typename KeyComparableType>
        inline const ValueType& operator[](const KeyComparableType &key) const
        {
            return At(key);
        }

        /// <summary>
        /// Tries to get element with given key.
        /// </summary>
        /// <param name="key">The key of the element.</param>
        /// <param name="result">The result Value.</param>
        /// <returns>True if element of given key has been found, otherwise false.</returns>
        template<typename KeyComparableType>
        bool TryGet(const KeyComparableType &key, ValueType &result) const
        {
            if (IsEmpty())
            {
                return false;
            }
            FindPositionResult pos;
            FindPosition(key, pos);
            if (pos.ObjectIndex == -1)
                return false;
            result = m_allocation.Get()[pos.ObjectIndex].Value;
            return true;
        }

        /// <summary>
        /// Tries to get pointer to the element with given key.
        /// </summary>
        /// <param name="key">The ky of the element.</param>
        /// <returns>Pointer to the element Value or null if cannot find it.</returns>
        template<typename KeyComparableType>
        ValueType *TryGet(const KeyComparableType &key) const
        {
            if (IsEmpty())
                return nullptr;
            FindPositionResult pos;
            FindPosition(key, pos);
            if (pos.ObjectIndex == -1)
                return nullptr;
            return (ValueType * ) & m_allocation.Get()[pos.ObjectIndex].Value;
        }

    public:
        /// <summary>
        /// Clears the collection but without changing its capacity (all inserted elements: keys and values will be removed).
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
        /// Clears the collection and delete Value objects.
        /// Note: collection must contain pointers to the objects that have public destructor and be allocated using New method.
        /// </summary>
        void ClearDelete()
        {
			if (TIsPointer<ValueType>::Value)
			{
				for (Iterator i = begin(); i.IsNotEnd(); ++i)
				{
					if (i->Value)
						Delete(i->Value);
				}
			}

            Clear();
        }

        /// <summary>
        /// Changes the capacity of the collection.
        /// </summary>
        /// <param name="capacity">The new capacity.</param>
        /// <param name="preserveContents">Enables preserving collection contents during resizing.</param>
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
                // Align capacity Value to the next power of two (http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2)
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
                        FindPosition(oldBucket.Key, pos);
                        ENGINE_ASSERT(pos.FreeSlotIndex != -1);
                        Bucket *bucket = &m_allocation.Get()[pos.FreeSlotIndex];
                        Memory::MoveItems(&bucket->Key, &oldBucket.Key, 1);
                        Memory::MoveItems(&bucket->Value, &oldBucket.Value, 1);
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
        void Swap(Dictionary &other)
        {
            if constexpr (AllocationType::HasSwap)
            {
                ::Swap(m_elementsCount, other.m_elementsCount);
				::Swap(m_deletedCount, other.m_deletedCount);
				::Swap(m_size, other.m_size);
                m_allocation.Swap(other.m_allocation);
            }
            else
            {
				::Swap(other, *this);
            }
        }

    public:
        /// <summary>
        /// Add pair element to the collection.
        /// </summary>
        /// <param name="key">The key.</param>
        /// <param name="Value">The Value.</param>
        /// <returns>Weak reference to the stored bucket.</returns>
        template<typename KeyComparableType>
        inline Bucket * Add(const KeyComparableType &key, const ValueType &Value)
        {
            Bucket * bucket = OnAdd(key);
            bucket->Occupy(key, Value);
            return bucket;
        }

        /// <summary>
        /// Add pair element to the collection.
        /// </summary>
        /// <param name="key">The key.</param>
        /// <param name="Value">The Value.</param>
        /// <returns>Weak reference to the stored bucket.</returns>
        template<typename KeyComparableType>
        inline Bucket *Add(const KeyComparableType &key, ValueType &&Value)
        {
            Bucket * bucket = OnAdd(key);
            bucket->Occupy(key, MoveTemp(Value));
            return bucket;
        }

        /// <summary>
        /// Add pair element to the collection.
        /// </summary>
        /// <param name="i">Iterator with key and Value.</param>
        void Add(const Iterator &i)
        {
            ENGINE_ASSERT(i._collection != this && i);
            const Bucket &bucket = *i;
            Add(bucket.Key, bucket.Value);
        }

        /// <summary>
        /// Removes element with a specified key.
        /// </summary>
        /// <param name="key">The element key to remove.</param>
        /// <returns>True if cannot remove item from the collection because cannot find it, otherwise false.</returns>
        template<typename KeyComparableType>
        bool Remove(const KeyComparableType &key)
        {
            if (IsEmpty())
                return false;
            FindPositionResult pos;
            FindPosition(key, pos);
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
        /// Removes element at specified iterator.
        /// </summary>
        /// <param name="i">The element iterator to remove.</param>
        /// <returns>True if cannot remove item from the collection because cannot find it, otherwise false.</returns>
        bool Remove(const Iterator &i)
        {
            ENGINE_ASSERT(i._collection == this);
            if (i)
            {
                ENGINE_ASSERT(m_allocation.Get()[i._index].IsOccupied());
                m_allocation.Get()[i._index].Delete();
                m_elementsCount--;
                m_deletedCount++;
                return true;
            }
            return false;
        }

        /// <summary>
        /// Removes elements with a specified Value
        /// </summary>
        /// <param name="Value">Element Value to remove</param>
        /// <returns>The amount of removed items. Zero if nothing changed.</returns>
        int32 RemoveValue(const ValueType &Value)
        {
            int32 result = 0;
            for (Iterator i = begin(); i.IsNotEnd(); ++i)
            {
                if (i->Value == Value)
                {
                    Remove(i);
                    result++;
                }
            }
            return result;
        }

    public:
        /// <summary>
        /// Finds the element with given key in the collection.
        /// </summary>
        /// <param name="key">The key to find.</param>
        /// <returns>The iterator for the found element or End if cannot find it.</returns>
        template<typename KeyComparableType>
        Iterator Find(const KeyComparableType &key) const
        {
            if (IsEmpty())
                return end();
            FindPositionResult pos;
            FindPosition(key, pos);
            return pos.ObjectIndex != -1 ? Iterator(this, pos.ObjectIndex) : end();
        }


		/// <summary>
        /// Checks if given key is in a collection.
        /// </summary>
        /// <param name="key">The key to find.</param>
        /// <returns>True if key has been found in a collection, otherwise false.</returns>
        template<typename KeyComparableType>
        bool ContainsKey(const KeyComparableType &key) const
        {
            if (IsEmpty())
                return false;
            FindPositionResult pos;
            FindPosition(key, pos);
            return pos.ObjectIndex != -1;
        }

        /// <summary>
        /// Checks if given Value is in a collection.
        /// </summary>
        /// <param name="Value">The Value to find.</param>
        /// <returns>True if Value has been found in a collection, otherwise false.</returns>
        bool ContainsValue(const ValueType &Value) const
        {
            if (HasItems())
            {
                const Bucket *data = m_allocation.Get();
                for (int32 i = 0; i < m_size; i++)
                {
                    if (data[i].IsOccupied() && data[i].Value == Value)
                        return true;
                }
            }
            return false;
        }

        /// <summary>
        /// Searches for the specified object and returns the zero-based index of the first occurrence within the entire dictionary.
        /// </summary>
        /// <param name="Value">The Value of the key to find.</param>
        /// <param name="key">The output key.</param>
        /// <returns>True if Value has been found, otherwise false.</returns>
        bool KeyOf(const ValueType &Value, KeyType *key) const
        {
            if (HasItems())
            {
                const Bucket *data = m_allocation.Get();
                for (int32 i = 0; i < m_size; i++)
                {
                    if (data[i].IsOccupied() && data[i].Value == Value)
                    {
                        if (key)
                            *key = data[i].Key;
                        return true;
                    }
                }
            }
            return false;
        }

    public:
        /// <summary>
        /// Clones other collection into this.
        /// </summary>
        /// <param name="other">The other collection to clone.</param>
        void Clone(const Dictionary &other)
        {
            // TODO: if both key and Value are POD types then use raw memory copy for buckets
            Clear();
            EnsureCapacity(other.Capacity(), false);
            for (Iterator i = other.begin(); i != other.end(); ++i)
                Add(i);
        }

        /// <summary>
        /// Gets the keys collection to the output array (will contain unique items).
        /// </summary>
        /// <param name="result">The result.</param>
        template<typename ArrayAllocation>
        void GetKeys(List<KeyType, ArrayAllocation> &result) const
        {
            for (Iterator i = begin(); i.IsNotEnd(); ++i)
                result.Add(i->Key);
        }

        /// <summary>
        /// Gets the values collection to the output array (may contain duplicates).
        /// </summary>
        /// <param name="result">The result.</param>
        template<typename ArrayAllocation>
        void GetValues(List<ValueType, ArrayAllocation> &result) const
        {
            for (Iterator i = begin(); i.IsNotEnd(); ++i)
                result.Add(i->Value);
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
        /// The result container of the dictionary item lookup searching.
        /// </summary>
        struct SE_API_RUNTIME FindPositionResult
        {
            int32 ObjectIndex;
            int32 FreeSlotIndex;
        };

        /// <summary>
        /// Returns a pair of positions: 1st where the object is, 2nd where
        /// it would go if you wanted to insert it. 1st is -1
        /// if object is not found; 2nd is -1 if it is.
        /// Note: because of deletions where-to-insert is not trivial: it's the
        /// first deleted bucket we see, as long as we don't find the key later
        /// </summary>
        /// <param name="key">The ky to find.</param>
        /// <param name="result">The pair of values: where the object is and where it would go if you wanted to insert it.</param>
        template<typename KeyComparableType>
        void FindPosition(const KeyComparableType &key, FindPositionResult &result) const
        {
            ENGINE_ASSERT(m_size);
            const int32 tableSizeMinusOne = m_size - 1;
            int32 bucketIndex = GetHash(key) & tableSizeMinusOne;
            int32 insertPos = -1;
            int32 checksCount = 0;
            const Bucket *data = m_allocation.Get();
            result.FreeSlotIndex = -1;
            while (checksCount < m_size)
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
                    // Occupied bucket by target key
                else if (bucket.Key == key)
                {
                    // Found key
                    result.ObjectIndex = bucketIndex;
                    return;
                }
                checksCount++;
                bucketIndex = (bucketIndex + DICTIONARY_PROB_FUNC(m_size, checksCount)) & tableSizeMinusOne;
            }
            result.ObjectIndex = -1;
            result.FreeSlotIndex = insertPos;
        }

        template<typename KeyComparableType>
        Bucket *OnAdd(const KeyComparableType &key)
        {
            // Check if need to rehash elements (prevent many deleted elements that use too much of capacity)
            if (m_deletedCount > m_size / DICTIONARY_DEFAULT_SLACK_SCALE)
                Compact();

            // Ensure to have enough memory for the next item (in case of new element insertion)
            EnsureCapacity((m_elementsCount + 1) * DICTIONARY_DEFAULT_SLACK_SCALE + m_deletedCount);

            // Find location of the item or place to insert it
            FindPositionResult pos;
            FindPosition(key, pos);

            // Ensure key is unknown
            ENGINE_ASSERT(pos.ObjectIndex == -1 && "That key has been already added to the dictionary.");

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
                Bucket * data = m_allocation.Get();
                for (int32 i = 0; i < m_size; i++)
                    data[i].m_State = Bucket::Empty;
            }
            else
            {
                // Rebuild entire table completely
                AllocationData oldAllocation;
                MoveToEmpty(oldAllocation, m_allocation, m_size);
                m_allocation.Allocate(m_size);
                Bucket * data = m_allocation.Get();
                for (int32 i = 0; i < m_size; i++)
                    data[i].m_State = Bucket::Empty;
                Bucket * oldData = oldAllocation.Get();
                FindPositionResult pos;
                for (int32 i = 0; i < m_size; i++)
                {
                    Bucket & oldBucket = oldData[i];
                    if (oldBucket.IsOccupied())
                    {
                        FindPosition(oldBucket.Key, pos);
                        ENGINE_ASSERT(pos.FreeSlotIndex != -1);
                        Bucket * bucket = &m_allocation.Get()[pos.FreeSlotIndex];
                        Memory::MoveItems(&bucket->Key, &oldBucket.Key, 1);
                        Memory::MoveItems(&bucket->Value, &oldBucket.Value, 1);
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
