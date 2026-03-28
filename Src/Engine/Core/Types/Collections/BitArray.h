// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Core/Platform/Platform.h"
#include "Core/Memory/Allocation.h"
#include "Core/Math/Math.h"

namespace SE
{
	/// <summary>
	/// Template for dynamic array with variable capacity that stores the bit values.
	/// </summary>
	template<typename AllocationType = HeapAllocation>
	class BitArray
	{
		friend BitArray;
	public:
		typedef uint64 ItemType;
		typedef typename AllocationType::template Data<ItemType> AllocationData;

	private:
		int32 m_count;
		int32 m_capacity;
		AllocationData m_allocation;

		inline static int32 ToItemCount(int32 size)
		{
			return Math::DivideAndRoundUp<int32>(size, sizeof(ItemType));
		}

		inline static int32 ToItemCapacity(int32 size)
		{
			return Math::Max<int32>(Math::DivideAndRoundUp<int32>(size, sizeof(ItemType)), 1);
		}

	public:
		/// <summary>
		/// Initializes a new instance of the <see cref="BitArray"/> class.
		/// </summary>
		inline BitArray()
			: m_count(0)
			, m_capacity(0)
		{
		}

		/// <summary>
		/// Initializes a new instance of the <see cref="BitArray"/> class.
		/// </summary>
		/// <param name="capacity">The initial capacity.</param>
		BitArray(int32 capacity)
			: m_count(0)
			, m_capacity(capacity)
		{
			if (capacity > 0)
				m_allocation.Allocate(ToItemCapacity(capacity));
		}

		/// <summary>
		/// Initializes a new instance of the <see cref="BitArray"/> class.
		/// </summary>
		/// <param name="other">The other collection to copy.</param>
		BitArray(const BitArray& other) noexcept
		{
			m_count = m_capacity = other.Count();
			if (m_capacity > 0)
			{
				const int32 itemsCapacity = ToItemCapacity(m_capacity);
				m_allocation.Allocate(itemsCapacity);
				Platform::MemoryCopy(Get(), other.Get(), itemsCapacity * sizeof(ItemType));
			}
		}

		/// <summary>
		/// Initializes a new instance of the <see cref="BitArray"/> class.
		/// </summary>
		/// <param name="other">The other collection to copy.</param>
		template<typename OtherAllocationType = AllocationType>
		explicit BitArray(const BitArray<OtherAllocationType>& other) noexcept
		{
			m_count = m_capacity = other.Count();
			if (m_capacity > 0)
			{
				const int32 itemsCapacity = ToItemCapacity(m_capacity);
				m_allocation.Allocate(itemsCapacity);
				Platform::MemoryCopy(Get(), other.Get(), itemsCapacity * sizeof(ItemType));
			}
		}

		/// <summary>
		/// Initializes a new instance of the <see cref="BitArray"/> class.
		/// </summary>
		/// <param name="other">The other collection to move.</param>
		inline BitArray(BitArray&& other) noexcept
		{
			m_count = other.m_count;
			m_capacity = other.m_capacity;
			other.m_count = 0;
			other.m_capacity = 0;
			m_allocation.Swap(other.m_allocation);
		}

		/// <summary>
		/// The assignment operator that deletes the current collection of items and the copies items from the other array.
		/// </summary>
		/// <param name="other">The other collection to copy.</param>
		/// <returns>The reference to this.</returns>
		BitArray& operator=(const BitArray& other) noexcept
		{
			if (this != &other)
			{
				if (m_capacity < other.m_count)
				{
					m_allocation.Free();
					m_capacity = other.m_count;
					const int32 itemsCapacity = ToItemCapacity(m_capacity);
					m_allocation.Allocate(itemsCapacity);
					Platform::MemoryCopy(Get(), other.Get(), itemsCapacity * sizeof(ItemType));
				}
				m_count = other.m_count;
			}
			return *this;
		}

		/// <summary>
		/// The move assignment operator that deletes the current collection of items and the moves items from the other array.
		/// </summary>
		/// <param name="other">The other collection to move.</param>
		/// <returns>The reference to this.</returns>
		BitArray& operator=(BitArray&& other) noexcept
		{
			if (this != &other)
			{
				m_allocation.Free();
				m_count = other.m_count;
				m_capacity = other.m_capacity;
				other.m_count = 0;
				other.m_capacity = 0;
				m_allocation.Swap(other.m_allocation);
			}
			return *this;
		}

		/// <summary>
		/// Finalizes an instance of the <see cref="BitArray"/> class.
		/// </summary>
		~BitArray()
		{
		}

	public:
		/// <summary>
		/// Gets the pointer to the bits storage data (linear allocation).
		/// </summary>
		inline ItemType* Get()
		{
			return m_allocation.Get();
		}

		/// <summary>
		/// Gets the pointer to the bits storage data (linear allocation).
		/// </summary>
		inline const ItemType* Get() const
		{
			return m_allocation.Get();
		}

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
		/// Gets the item at the given index.
		/// </summary>
		/// <param name="index">The index of the item.</param>
		/// <returns>The value of the item.</returns>
		inline bool operator[](int32 index) const
		{
			return Get(index);
		}

		/// <summary>
		/// Gets the item at the given index.
		/// </summary>
		/// <param name="index">The index of the item.</param>
		/// <returns>The value of the item.</returns>
		bool Get(int32 index) const
		{
			ENGINE_ASSERT(index >= 0 && index < m_count);
			const ItemType offset = index / sizeof(ItemType);
			const ItemType bitMask = (ItemType)(int32)(1 << (index & ((int32)sizeof(ItemType) - 1)));
			const ItemType item = ((ItemType*)m_allocation.Get())[offset];
			return (item & bitMask) != 0;
		}

		/// <summary>
		/// Sets the item at the given index.
		/// </summary>
		/// <param name="index">The index of the item.</param>
		/// <param name="value">The value to set.</param>
		void Set(int32 index, bool value)
		{
			ENGINE_ASSERT(index >= 0 && index < m_count);
			const ItemType offset = index / sizeof(ItemType);
			const ItemType bitMask = (ItemType)(int32)(1 << (index & ((int32)sizeof(ItemType) - 1)));
			ItemType& item = ((ItemType*)m_allocation.Get())[offset];
			if (value)
				item |= bitMask;
			else
				item &= ~bitMask;
		}

	public:
		/// <summary>
		/// Clear the collection without changing its capacity.
		/// </summary>
		inline void Clear()
		{
			m_count = 0;
		}

		/// <summary>
		/// Changes the capacity of the collection.
		/// </summary>
		/// <param name="capacity">The new capacity.</param>
		/// <param name="preserveContents">True if preserve collection data when changing its size, otherwise collection after resize will be empty.</param>
		void SetCapacity(const int32 capacity, bool preserveContents = true)
		{
			if (capacity == m_capacity)
				return;
			ENGINE_ASSERT(capacity >= 0);
			const int32 count = preserveContents ? (m_count < capacity ? m_count : capacity) : 0;
			m_allocation.Relocate(ToItemCapacity(capacity), ToItemCount(m_count), ToItemCount(count));
			m_capacity = capacity;
			m_count = count;
		}

		/// <summary>
		/// Resizes the collection to the specified size. If the size is equal or less to the current capacity no additional memory reallocation in performed.
		/// </summary>
		/// <param name="size">The new collection size.</param>
		/// <param name="preserveContents">True if preserve collection data when changing its size, otherwise collection after resize might not contain the previous data.</param>
		void Resize(int32 size, bool preserveContents = true)
		{
			if (m_count <= size)
				EnsureCapacity(size, preserveContents);
			m_count = size;
		}

		/// <summary>
		/// Ensures the collection has given capacity (or more).
		/// </summary>
		/// <param name="minCapacity">The minimum capacity.</param>
		/// <param name="preserveContents">True if preserve collection data when changing its size, otherwise collection after resize will be empty.</param>
		void EnsureCapacity(int32 minCapacity, bool preserveContents = true)
		{
			if (m_capacity < minCapacity)
			{
				const int32 capacity = m_allocation.CalculateCapacityGrow(ToItemCapacity(m_capacity), minCapacity);
				SetCapacity(capacity, preserveContents);
			}
		}

		/// <summary>
		/// Sets all items to the given value
		/// </summary>
		/// <param name="value">The value to assign to all the collection items.</param>
		void SetAll(const bool value)
		{
			if (m_count != 0)
				Platform::MemorySet(m_allocation.Get(), ToItemCount(m_count) * sizeof(ItemType), value ? Max_uint32 : 0);
		}

		/// <summary>
		/// Adds the specified item to the collection.
		/// </summary>
		/// <param name="item">The item to add.</param>
		void Add(const bool item)
		{
			EnsureCapacity(m_count + 1);
			m_count++;
			Set(m_count - 1, item);
		}

		/// <summary>
		/// Adds the specified item to the collection.
		/// </summary>
		/// <param name="items">The items to add.</param>
		/// <param name="count">The items count.</param>
		void Add(const bool* items, int32 count)
		{
			EnsureCapacity(m_count + count);
			for (int32 i = 0; i < count; i++)
				Add(items[i]);
		}

		/// <summary>
		/// Adds the other collection to the collection.
		/// </summary>
		/// <param name="other">The other collection to add.</param>
		void Add(const BitArray& other)
		{
			EnsureCapacity(m_count, other.Count());
			for (int32 i = 0; i < other.Count(); i++)
				Add(other[i]);
		}

		/// <summary>
		/// Swaps the contents of collection with the other object without copy operation. Performs fast internal data exchange.
		/// </summary>
		/// <param name="other">The other collection.</param>
		void Swap(BitArray& other)
		{
			::Swap(m_count, other.m_count);
			::Swap(m_capacity, other.m_capacity);
			m_allocation.Swap(other.m_allocation);
		}

	public:
		template<typename OtherAllocationType = AllocationType>
		bool operator==(const BitArray<OtherAllocationType>& other) const
		{
			if (m_count == other.Count())
			{
				for (int32 i = 0; i < m_count; i++)
				{
					if (Get(i) != other.Get(i))
						return false;
				}
				return true;
			}
			return false;
		}

		template<typename OtherAllocationType = AllocationType>
		bool operator!=(const BitArray<OtherAllocationType>& other) const
		{
			return !operator==(other);
		}
	};
}
