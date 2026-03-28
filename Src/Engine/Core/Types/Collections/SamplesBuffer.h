#pragma once


#include "Core/Types/Variable.h"

namespace SE
{
    /**
     * 用于计算最小/最大/平均值的小缓冲区。
     * @tparam T
     * @tparam Size 缓冲区大小
     */
    template<typename T, int32 Size>
    class SamplesBuffer
    {
    protected:
        int32 m_count = 0;
        T m_data[Size];

    public:
        /// <summary>
        /// Gets amount of elements in the collection.
        /// </summary>
        inline int32 Count() const
        {
            return m_count;
        }

        /// <summary>
        /// Gets amount of elements that can be added to the collection.
        /// </summary>
        inline int32 Capacity() const
        {
            return Size;
        }

        /// <summary>
        /// Returns true if collection has any elements added.
        /// </summary>
        inline bool HasItems() const
        {
            return m_count > 0;
        }

        /// <summary>
        /// Returns true if collection is empty.
        /// </summary>
        inline bool IsEmpty() const
        {
            return m_count < 1;
        }

        /// <summary>
        /// Gets pointer to the first element in the collection.
        /// </summary>
        inline T* Get() const
        {
            return m_data;
        }

        /// <summary>
        /// Gets or sets element at the specified index.
        /// </summary>
        /// <param name="index">The index.</param>
        /// <returns>The element.</returns>
        inline T& At(int32 index) const
        {
            ENGINE_ASSERT(index >= 0 && index < m_count);
            return m_data[index];
        }

        /// <summary>
        /// Gets or sets element at the specified index.
        /// </summary>
        /// <param name="index">The index.</param>
        /// <returns>The element.</returns>
        inline T& operator[](int32 index)
        {
            ENGINE_ASSERT(index >= 0 && index < m_count);
            return m_data[index];
        }

        /// <summary>
        /// Gets or sets element at the specified index.
        /// </summary>
        /// <param name="index">The index.</param>
        /// <returns>The element.</returns>
        inline const T& operator[](int32 index) const
        {
            ENGINE_ASSERT(index >= 0 && index < m_count);
            return m_data[index];
        }

        /// <summary>
        /// Gets the first element value.
        /// </summary>
        inline T First() const
        {
            ENGINE_ASSERT(HasItems());
            return m_data[0];
        }

        /// <summary>
        /// Gets last element value.
        /// </summary>
        inline T Last() const
        {
            ENGINE_ASSERT(HasItems());
            return m_data[m_count - 1];
        }

    public:
        /// <summary>
        /// Adds the specified value to the buffer.
        /// </summary>
        /// <param name="value">The value to add.</param>
        void Add(const T& value)
        {
            if (m_count != Size)
                m_count++;
            if (m_count > 1)
            {
                for (int32 i = m_count - 1; i > 0; i--)
                    m_data[i] = m_data[i - 1];
            }
            m_data[0] = value;
        }

        /// <summary>
        /// Sets all elements to the given value.
        /// </summary>
        /// <param name="value">The value.</param>
        void SetAll(const T value)
        {
            for (int32 i = 0; i < m_count; i++)
                m_data[i] = value;
        }

        /// <summary>
        /// Clears this collection.
        /// </summary>
        inline void Clear()
        {
            m_count = 0;
        }

    public:
        /// <summary>
        /// Gets the minimum value in the buffer.
        /// </summary>
        T Minimum() const
        {
            ENGINE_ASSERT(HasItems());
            T result = m_data[0];
            for (int32 i = 1; i < m_count; i++)
            {
                if (m_data[i] < result)
                    result = m_data[i];
            }
            return result;
        }

        /// <summary>
        /// Gets the maximum value in the buffer.
        /// </summary>
        T Maximum() const
        {
            ENGINE_ASSERT(HasItems());
            T result = m_data[0];
            for (int32 i = 1; i < m_count; i++)
            {
                if (m_data[i] > result)
                    result = m_data[i];
            }
            return result;
        }

        /// <summary>
        /// Gets the average value in the buffer.
        /// </summary>
        T Average() const
        {
            ENGINE_ASSERT(HasItems());
            T result = m_data[0];
            for (int32 i = 1; i < m_count; i++)
            {
                result += m_data[i];
            }
            return result / m_count;
        }
    };
}
