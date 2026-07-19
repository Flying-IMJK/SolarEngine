#pragma once

#include "Runtime/Core/Memory/Memory.h"
#include "Runtime/API.h"

namespace SE
{
    class RendererAllocation
    {
    public:
        static SE_API_RUNTIME void* Allocate(uintptr size);
        static SE_API_RUNTIME void Free(void* ptr, uintptr size);

        enum { HasSwap = true };

        template<typename T>
        class Data
        {
            T* m_Data = nullptr;
            uintptr m_Size = 0;

        public:
            FORCE_INLINE Data()
            {
            }

            FORCE_INLINE ~Data()
            {
                if (m_Data)
                    RendererAllocation::Free(m_Data, m_Size);
            }

            FORCE_INLINE T* Get()
            {
                return m_Data;
            }

            FORCE_INLINE const T* Get() const
            {
                return m_Data;
            }

            FORCE_INLINE int32 CalculateCapacityGrow(int32 capacity, int32 minCapacity) const
            {
                capacity = capacity ? capacity * 2 : 64;
                if (capacity < minCapacity)
                    capacity = minCapacity;
                return capacity;
            }

            FORCE_INLINE void Allocate(uint64 capacity)
            {
                m_Size = capacity * sizeof(T);
                m_Data = (T*)RendererAllocation::Allocate(m_Size);
            }

            FORCE_INLINE void Relocate(uint64 capacity, int32 oldCount, int32 newCount)
            {
                T* newData = capacity != 0 ? (T*)RendererAllocation::Allocate(capacity * sizeof(T)) : nullptr;
                if (oldCount)
                {
                    if (newCount > 0)
                        Memory::MoveItems(newData, m_Data, newCount);
                    Memory::DestructItems(m_Data, oldCount);
                }
                if (m_Data)
                    RendererAllocation::Free(m_Data, m_Size);
                m_Data = newData;
                m_Size = capacity * sizeof(T);
            }

            FORCE_INLINE void Free()
            {
                if (m_Data)
                {
                    RendererAllocation::Free(m_Data, m_Size);
                    m_Data = nullptr;
                }
            }

            FORCE_INLINE void Swap(Data& other)
            {
                ::Swap(m_Data, other.m_Data);
                ::Swap(m_Size, other.m_Size);
            }
        };
    };

}
