#pragma once

#include "Core/API.h"
#include "Core/Types/Variable.h"
#include "Core/Types/Collections/List.h"

namespace SE
{
    /// <summary>
    /// 任意连续内存的普遍表示。
    /// </summary>
    template<typename T>
    class Span
    {
    protected:
        T* m_data;
        int32 m_length;

    public:
        /// <summary>
        /// Initializes a new instance of the <see cref="Span"/> class.
        /// </summary>
        Span() : m_data(nullptr), m_length(0)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Span"/> class.
        /// </summary>
        /// <param name="data">The data pointer to link.</param>
        /// <param name="length">The data length.</param>
        Span(const T* data, int32 length)
                : m_data((T*)data)
                , m_length(length)
        {
        }

    public:
        /// <summary>
        /// Returns true if data is valid.
        /// </summary>
        inline bool IsValid() const
        {
            return m_data != nullptr;
        }

        /// <summary>
        /// Returns true if data is invalid.
        /// </summary>
        inline bool IsInvalid() const
        {
            return m_data == nullptr;
        }

        /// <summary>
        /// Gets length of the data.
        /// </summary>
        inline int32 Length() const
        {
            return m_length;
        }

        /// <summary>
        /// Gets the pointer to the data.
        /// </summary>
        inline T* Get()
        {
            return m_data;
        }

        /// <summary>
        /// Gets the pointer to the data.
        /// </summary>
        inline const T* Get() const
        {
            return m_data;
        }

        /// <summary>
        /// Gets the pointer to the data.
        /// </summary>
        template<typename U>
        inline U* Get() const
        {
            return (U*)m_data;
        }

    public:
        /// <summary>
        /// Gets or sets the element by index
        /// </summary>
        /// <param name="index">The index.</param>
        /// <returns>The item reference.</returns>
        inline T& operator[](int32 index)
        {
            ENGINE_ASSERT(index >= 0 && index < m_length);
            return m_data[index];
        }

        /// <summary>
        /// Gets or sets the element by index
        /// </summary>
        /// <param name="index">The index.</param>
        /// <returns>The item constant reference.</returns>
        inline const T& operator[](int32 index) const
        {
            ENGINE_ASSERT(index >= 0 && index < m_length);
            return m_data[index];
        }

	public:
		/// <summary>
		/// The collection iterator.
		/// </summary>
		struct Iterator
		{
		private:
			Span *m_array;
			int32 m_index;
		public:

			Iterator(Span *array, const int32 index)
				: m_array(array), m_index(index)
			{
			}

			Iterator(Span const *array, const int32 index)
				: m_array(const_cast<Span *>(array)), m_index(index)
			{
			}

			Iterator()
				: m_array(nullptr), m_index(-1)
			{
			}

			Iterator(const Iterator &i)
				: m_array(i.m_array), m_index(i.m_index)
			{
			}

			Iterator(Iterator &&i)
				: m_array(i.m_array), m_index(i.m_index)
			{
			}

		public:
			inline int32 GetIndex() const
			{
				return m_index;
			}

			inline bool IsEnd() const
			{
				return m_index == m_array->m_length;
			}

			inline bool IsNotEnd() const
			{
				return m_index != m_array->m_length;
			}

			inline T& operator*() const
			{
				return m_array->Get()[m_index];
			}

			inline T* operator->() const
			{
				return &m_array->Get()[m_index];
			}

			inline bool operator==(const Iterator &v) const
			{
				return m_array == v.m_array && m_index == v.m_index;
			}

			inline bool operator!=(const Iterator &v) const
			{
				return m_array != v.m_array || m_index != v.m_index;
			}

			Iterator &operator=(const Iterator &v)
			{
				m_array = v.m_array;
				m_index = v.m_index;
				return *this;
			}

			Iterator &operator++()
			{
				if (m_index != m_array->m_length)
				{
					m_index++;
				}
				return *this;
			}

			Iterator operator++(int)
			{
				Iterator temp = *this;
				if (m_index != m_array->m_length)
				{
					m_index++;
				}
				return temp;
			}

			Iterator &operator--()
			{
				if (m_index > 0)
				{
					m_index--;
				}
				return *this;
			}

			Iterator operator--(int)
			{
				Iterator temp = *this;
				if (m_index > 0)
				{
					m_index--;
				}
				return temp;
			}
		};


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
			return Iterator(this, m_length);
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
			return Iterator(this, m_length);
		}
    };

    template<typename T>
    inline Span<T> ToSpan(const T* ptr, int32 length)
    {
        return Span<T>(ptr, length);
    }

    template<typename T, typename U = T, typename AllocationType = HeapAllocation>
    inline Span<U> ToSpan(const List<T, AllocationType>& data)
    {
        return Span<U>((U*)data.Get(), data.Count());
    }

    template<typename T>
    inline bool SpanContains(const Span<T> span, const T& value)
    {
        for (int32 i = 0; i < span.Length(); i++)
        {
            if (span.Get()[i] == value)
                return true;
        }
        return false;
    }
}