#pragma once

#include "Memory.h"
#include "Runtime/Core/Math/Math.h"
#include "Runtime/Core/Logging/Logging.h"

namespace SE
{
    /// <summary>
    /// The memory allocation policy that uses inlined memory of the fixed size (no resize support, does not use heap allocations at all).
    /// </summary>
    template<int Capacity>
    class FixedAllocation
    {
    public:
        enum
        {
            HasSwap = false
        };

        template<typename T>
        class Data
        {
        private:
            byte m_data[Capacity * sizeof(T)];

        public:
            inline Data()
            {
				Memory::ConstructItems(m_data, Capacity * sizeof(T));
            }

            inline ~Data()
            {
            }

            inline T* Get()
            {
                return (T *) m_data;
            }

            inline const T* Get() const
            {
                return (T *) m_data;
            }

            inline int32 CalculateCapacityGrow(int32 capacity, int32 minCapacity) const
            {
                ENGINE_ASSERT(minCapacity <= Capacity);
                return Capacity;
            }

            inline void Allocate(int32 capacity)
            {
#if ENABLE_ASSERTION_LOW_LAYERS
                ENGINE_ASSERT(capacity <= Capacity);
#endif
            }

            inline void Relocate(int32 capacity, int32 oldCount, int32 newCount)
            {
#if ENABLE_ASSERTION_LOW_LAYERS
                ENGINE_ASSERT(capacity <= Capacity);
#endif
            }

            inline void Free()
            {
            }

            void Swap(Data &other)
            {
                // Not supported
            }
        };
    };

    /// <summary>
    /// The memory allocation policy that uses default heap allocator.
    /// </summary>
    class HeapAllocation
    {
    public:
        enum
        {
            HasSwap = true
        };

        template<typename T>
        class Data
        {
        private:
            T *m_Data = nullptr;

        public:
            inline Data()
            {
            }

            inline ~Data()
            {
				PlatformAllocator::Free(m_Data);
            }

            inline T* Get()
            {
                return m_Data;
            }

            inline const T* Get() const
            {
                return m_Data;
            }

            inline int32 CalculateCapacityGrow(int32 capacity, int32 minCapacity) const
            {
                if (capacity < minCapacity)
                {
                    capacity = minCapacity;
                }

                if (capacity < 8)
                {
                    capacity = 8;
                }
                else
                {
                    // Round up to the next power of 2 and multiply by 2 (http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2)
                    capacity--;
                    capacity |= capacity >> 1;
                    capacity |= capacity >> 2;
                    capacity |= capacity >> 4;
                    capacity |= capacity >> 8;
                    capacity |= capacity >> 16;
                    uint64 capacity64 = (uint64)(capacity + 1) * 2;
                    if (capacity64 > Max_int32)
                    {
                        capacity64 = Max_int32;
                    }

                    capacity = (int32)
                    capacity64;
                }
                return capacity;
            }

            inline void Allocate(int32 capacity)
            {
#if  ENABLE_ASSERTION_LOW_LAYERS
                ENGINE_ASSERT(!m_data);
#endif
				m_Data = (T *) PlatformAllocator::Allocate(capacity * sizeof(T));

#if !BUILD_RELEASE
                if (!m_Data)
                {
                    LOG_ERROR("Memory", "OUT_OF_MEMORY");
                }
#endif
				Memory::ConstructItems(m_Data, capacity);
            }

            inline void Relocate(int32 capacity, int32 oldCount, int32 newCount)
            {
                T* newData = capacity != 0 ? (T *) PlatformAllocator::Allocate(capacity * sizeof(T)) : nullptr;

#if !BUILD_RELEASE
                if (newData == nullptr && capacity != 0)
                {
                    LOG_ERROR("Memory", "OUT_OF_MEMORY");
                }
#endif
				if (oldCount)
				{
					if (newCount > 0)
					{
						Memory::MoveItems(newData, m_Data, newCount);
						Memory::ConstructItems(newData + newCount, capacity - newCount);
					}
					Memory::DestructItems(m_Data, oldCount);
				}
				else
				{
					Memory::ConstructItems(newData, capacity);
				}

                if (m_Data != nullptr)
                {
                    PlatformAllocator::Free(m_Data);
                }

				m_Data = newData;
            }

            inline void Free()
            {
				PlatformAllocator::Free(m_Data);
				m_Data = nullptr;
            }

            inline void Swap(Data &other)
            {
                ::Swap(m_Data, other.m_Data);
            }
        };
    };

    /// <summary>
    /// The memory allocation policy that uses inlined memory of the fixed size and supports using additional allocation to increase its capacity (eg. via heap allocation).
    /// </summary>
    template<int Capacity, typename OtherAllocator>
    class InlinedAllocation
    {
    public:
        enum
        {
            HasSwap = false
        };

        template<typename T>
        class Data
        {
        private:
            typedef typename OtherAllocator::template Data<T> OtherData;

            bool m_useOther = false;
            byte m_data[Capacity * sizeof(T)];
            OtherData m_other;

        public:
            inline Data()
            {
				Memory::ConstructItems(m_data, Capacity * sizeof(T));
            }

            inline ~Data()
            {
            }

            inline T* Get()
            {
                return m_useOther ? m_other.Get() : (T *) m_data;
            }

            inline const T* Get() const
            {
                return m_useOther ? m_other.Get() : (T *) m_data;
            }

            inline int32 CalculateCapacityGrow(int32 capacity, int32 minCapacity) const
            {
                return minCapacity <= Capacity ? Capacity : m_other.CalculateCapacityGrow(capacity, minCapacity);
            }

            inline void Allocate(int32 capacity)
            {
                if (capacity > Capacity)
                {
                    m_useOther = true;
                    m_other.Allocate(capacity);
                }
            }

            inline void Relocate(int32 capacity, int32 oldCount, int32 newCount)
            {
                // Check if the new allocation will fit into inlined storage
                if (capacity <= Capacity)
                {
                    if (m_useOther)
                    {
                        // Move the items from other allocation to the inlined storage
                        Memory::MoveItems((T *) m_data, m_other.Get(), newCount);

                        // Free the other allocation
                        Memory::DestructItems(m_other.Get(), oldCount);
                        m_other.Free();
                        m_useOther = false;
                    }
                }
                else
                {
                    if (m_useOther)
                    {
                        // Resize other allocation
                        m_other.Relocate(capacity, oldCount, newCount);
                    }
                    else
                    {
                        // Allocate other allocation
                        m_other.Allocate(capacity);
                        m_useOther = true;

                        // Move the items from the inlined storage to the other allocation
                        Memory::MoveItems(m_other.Get(), (T *) m_data, newCount);
                        Memory::DestructItems((T *) m_data, oldCount);
                    }
                }
            }

            inline void Free()
            {
                if (m_useOther)
                {
                    m_useOther = false;
                    m_other.Free();
                }
            }

            void Swap(Data &other)
            {
                // Not supported
            }
        };
    };

    typedef HeapAllocation DefaultAllocation;
}
