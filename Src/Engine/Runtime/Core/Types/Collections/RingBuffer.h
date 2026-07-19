#pragma once

//#include "Engine/Platform/Platform.h"
#include "Runtime/Core/Memory/Memory.h"
#include "Runtime/Core/Memory/Allocation.h"
#include "Runtime/Core/Math/Math.h"

namespace SE
{
    /**
     * 可变容量的循环Buffer
     * @tparam T
     * @tparam AllocationType
     */
    template<typename T, typename AllocationType = HeapAllocation>
    class RingBuffer
    {
    public:
        typedef T ItemType;
        typedef typename AllocationType::template Data<T> AllocationData;

    private:
        int32 m_front = 0, m_back = 0, m_count = 0, m_capacity = 0;
        AllocationData m_allocation;

    public:
        ~RingBuffer()
        {
            Memory::DestructItems(Get() + Math::Min(m_front, m_back), m_count);
        }

        inline T* Get()
        {
            return m_allocation.Get();
        }

        inline int32 Count() const
        {
            return m_count;
        }

        inline int32 Capacity() const
        {
            return m_capacity;
        }

        void PushBack(const T& data)
        {
            if (m_capacity == 0 || m_capacity == m_count)
            {
                const int32 capacity = m_allocation.CalculateCapacityGrow(m_capacity, m_count + 1);
                AllocationData alloc;
                alloc.Allocate(capacity);
                const int32 frontCount = Math::Min(m_capacity - m_front, m_count);
                Memory::MoveItems(alloc.Get(), m_allocation.Get() + m_front, frontCount);
                Memory::DestructItems(m_allocation.Get() + m_front, frontCount);
                const int32 backCount = m_count - frontCount;
                Memory::MoveItems(alloc.Get() + frontCount, m_allocation.Get(), backCount);
                Memory::DestructItems(m_allocation.Get(), backCount);
                m_allocation.Swap(alloc);
                m_front = 0;
                m_back = m_count;
                m_capacity = capacity;
            }
            Memory::ConstructItems(m_allocation.Get() + m_back, &data, 1);
            m_back = (m_back + 1) % m_capacity;
            m_count++;
        }

        inline T& PeekFront()
        {
            return m_allocation.Get()[m_front];
        }

        inline const T& PeekFront() const
        {
            return m_allocation.Get()[m_front];
        }

        inline T& operator[](int32 index)
        {
            ENGINE_ASSERT(index >= 0 && index < m_count);
            return m_allocation.Get()[(m_front + index) % m_capacity];
        }

        inline const T& operator[](int32 index) const
        {
            ENGINE_ASSERT(index >= 0 && index < m_count);
            return m_allocation.Get()[(m_front + index) % m_capacity];
        }

        void PopFront()
        {
            Memory::DestructItems(m_allocation.Get() + m_front, 1);
            m_front = (m_front + 1) % m_capacity;
            m_count--;
        }

        void Clear()
        {
            Memory::DestructItems(Get() + Math::Min(m_front, m_back), m_count);
            m_front = m_back = m_count = 0;
        }
    };
}
