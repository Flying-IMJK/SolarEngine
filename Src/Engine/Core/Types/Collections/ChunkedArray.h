#pragma once

#include "List.h"
#include "Core/Math/Math.h"

namespace SE
{
    /**
     * 具有可变容量的动态数组，使用固定大小的内存块进行数据存储，而不是线性分配
     * @tparam T
     * @remarks Array with variable capacity that does not moves elements when it grows so you can add item and use pointer to it while still keep adding new items.
     */
    template<typename T, int32 ChunkSize>
    class ChunkedArray
    {
        friend ChunkedArray;

    private:
        // TODO: don't use Array but small struct and don't InlinedArray or Chunk* but Chunk (less dynamic allocations)
        typedef List<T> Chunk;

        int32 m_count = 0;
        List<Chunk*, InlinedAllocation<32>> m_chunks;

    public:
        ChunkedArray()
        {
        }

        ~ChunkedArray()
        {
            m_chunks.ClearDelete();
        }

    public:
        /// <summary>
        /// Gets the amount of the elements in the collection.
        /// </summary>
        inline int32 Count() const
        {
            return m_count;
        }

        /// <summary>
        /// Gets the amount of the elements that can be hold by collection without resizing.
        /// </summary>
        inline int32 Capacity() const
        {
            return m_chunks.Count() * ChunkSize;
        }

        /// <summary>
        /// Returns true if array isn't empty.
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

    public:
        // Gets element by index
        inline T& At(int32 index) const
        {
            ENGINE_ASSERT(index >= 0 && index < m_count);
            return m_chunks[index / ChunkSize]->At(index % ChunkSize);
        }

        // Gets/Sets element by index
        inline T& operator[](int32 index)
        {
            ENGINE_ASSERT(index >= 0 && index < m_count);
            return m_chunks[index / ChunkSize]->At(index % ChunkSize);
        }

        // Gets/Sets element by index
        inline const T& operator[](int32 index) const
        {
            ENGINE_ASSERT(index >= 0 && index < m_count);
            return m_chunks[index / ChunkSize]->At(index % ChunkSize);
        }

    public:
        /// <summary>
        /// Chunked array iterator.
        /// </summary>
        struct Iterator
        {
            friend ChunkedArray;
        private:
            ChunkedArray* m_collection;
            int32 m_chunkIndex;
            int32 m_index;

            Iterator(const ChunkedArray* collection, const int32 index)
                    : m_collection(const_cast<ChunkedArray*>(collection))
                    , m_chunkIndex(index / ChunkSize)
                    , m_index(index % ChunkSize)
            {
            }

        public:
            Iterator()
                    : m_collection(nullptr)
                    , m_chunkIndex(INVALID_INDEX)
                    , m_index(INVALID_INDEX)
            {
            }

            Iterator(const Iterator& i)
                    : m_collection(i.m_collection)
                    , m_chunkIndex(i.m_chunkIndex)
                    , m_index(i.m_index)
            {
            }

            Iterator(Iterator&& i)
                    : m_collection(i.m_collection)
                    , m_chunkIndex(i.m_chunkIndex)
                    , m_index(i.m_index)
            {
            }

        public:
            inline int32 Index() const
            {
                return m_chunkIndex * ChunkSize + m_index;
            }

            inline bool IsEnd() const
            {
                return (m_chunkIndex * ChunkSize + m_index) == m_collection->m_count;
            }

            inline bool IsNotEnd() const
            {
                return (m_chunkIndex * ChunkSize + m_index) != m_collection->m_count;
            }

            inline T& operator*() const
            {
                return m_collection->m_chunks[m_chunkIndex]->At(m_index);
            }

            inline T* operator->() const
            {
                return &m_collection->m_chunks[m_chunkIndex]->At(m_index);
            }

            inline bool operator==(const Iterator& v) const
            {
                return m_collection == v.m_collection && m_chunkIndex == v.m_chunkIndex && m_index == v.m_index;
            }

            inline bool operator!=(const Iterator& v) const
            {
                return m_collection != v.m_collection || m_chunkIndex != v.m_chunkIndex || m_index != v.m_index;
            }

            Iterator& operator=(const Iterator& v)
            {
                m_collection = v.m_collection;
                m_chunkIndex = v.m_chunkIndex;
                m_index = v.m_index;
                return *this;
            }

            Iterator& operator++()
            {
                // Check if it is not at end
                if ((m_chunkIndex * ChunkSize + m_index) != m_collection->m_count)
                {
                    // Move forward within chunk
                    m_index++;

                    if (m_index == ChunkSize && m_chunkIndex < m_collection->m_chunks.Count() - 1)
                    {
                        // Move to next chunk
                        m_chunkIndex++;
                        m_index = 0;
                    }
                }
                return *this;
            }

            Iterator operator++(int)
            {
                Iterator i = *this;
                ++i;
                return i;
            }

            Iterator& operator--()
            {
                // Check if it's not at beginning
                if (m_index != 0 || m_chunkIndex != 0)
                {
                    if (m_index == 0)
                    {
                        // Move to previous chunk
                        m_chunkIndex--;
                        m_index = ChunkSize - 1;
                    }
                    else
                    {
                        // Move backward within chunk
                        m_index--;
                    }
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
        /// Adds the specified item to the collection.
        /// </summary>
        /// <param name="item">The item.</param>
        /// <returns>The pointer to the allocated item in the storage.</returns>
        T* Add(const T& item)
        {
            // Find first chunk with some space
            Chunk* chunk = nullptr;
            for (int32 i = 0; i < m_chunks.Count(); i++)
            {
                if (m_chunks[i]->Count() < ChunkSize)
                {
                    chunk = m_chunks[i];
                    break;
                }
            }

            // Allocate chunk if missing
            if (chunk == nullptr)
            {
                chunk = New<Chunk>();
                chunk->SetCapacity(ChunkSize);
                m_chunks.Add(chunk);
            }

            // Add item
            m_count++;
            chunk->Add(item);
            return &chunk->At(chunk->Count() - 1);
        }

        /// <summary>
        /// Adds the one item to the collection and returns the reference to it.
        /// </summary>
        /// <returns>The reference to the added item.</returns>
        T& AddOne()
        {
            // Find first chunk with some space
            Chunk* chunk = nullptr;
            for (int32 i = 0; i < m_chunks.Count(); i++)
            {
                if (m_chunks[i]->Count() < ChunkSize)
                {
                    chunk = m_chunks[i];
                    break;
                }
            }

            // Allocate chunk if missing
            if (chunk == nullptr)
            {
                chunk = New<Chunk>();
                chunk->SetCapacity(ChunkSize);
                m_chunks.Add(chunk);
            }

            // Add item
            m_count++;
            return chunk->AddOne();
        }

        /// <summary>
        /// Removes the element at specified iterator position.
        /// </summary>
        /// <param name="i">The element iterator to remove.</param>
        void Remove(const Iterator& i)
        {
            if (IsEmpty())
                return;
            ENGINE_ASSERT(i.m_collection == this);
            ENGINE_ASSERT(i.m_chunkIndex < m_chunks.Count() && i.m_index < ChunkSize);
            ENGINE_ASSERT(i.Index() < Count());

            auto lastChunkIndex = (m_count - 1) / ChunkSize;
            auto lastIndex = (m_count - 1) % ChunkSize;
            auto& lastChunk = *m_chunks[lastChunkIndex];

            // Check if remove element from the last chunk
            if (i.m_chunkIndex == lastChunkIndex)
            {
                // Remove that item
                lastChunk.RemoveAt(i.m_index);
            }
            else
            {
                // Swap that item with the last item from the last chunk
                (*m_chunks[i.m_chunkIndex])[i.m_index] = lastChunk[lastIndex];
                lastChunk.RemoveLast();
            }

            m_count--;
        }

        /// <summary>
        /// Clears the collection but without changing its capacity.
        /// </summary>
        void Clear()
        {
            m_count = 0;
            for (int32 i = 0; i < m_chunks.Count(); i++)
            {
                m_chunks[i]->Clear();
            }
        }

        /// <summary>
        /// Clears the collection and releases the dynamic memory allocated within the container.
        /// </summary>
        void Release()
        {
            Clear();
            m_chunks.ClearDelete();
            m_chunks.Resize(0);
        }

        /// <summary>
        /// Ensures that collection has a given capacity. It does not preserve collection contents.
        /// </summary>
        /// <param name="minCapacity">The minimum required capacity.</param>
        void EnsureCapacity(int32 minCapacity)
        {
            int32 minChunks = minCapacity / ChunkSize;
            if (minCapacity % ChunkSize != 0)
                ++minChunks;
            while (m_chunks.Count() < minChunks)
            {
                auto chunk = New<Chunk>();
                chunk->SetCapacity(ChunkSize);
                m_chunks.Add(chunk);
            }
        }

        /// <summary>
        /// Resizes that collection to the specified new size. It may not preserve collection contents in case of shrinking.
        /// </summary>
        /// <param name="newSize">The new size.</param>
        void Resize(int32 newSize)
        {
            while (newSize < Count())
            {
                auto& chunk = m_chunks.Last();
                int32 itemsLeft = Count() - newSize;
                int32 itemsToRemove = Math::Min(chunk->Count(), itemsLeft);
                chunk->Resize(chunk->Count() - itemsToRemove);
                m_count -= itemsToRemove;
                if (chunk->Count() == 0)
                {
                    Delete(chunk);
                    m_chunks.RemoveLast();
                }
            }
            if (newSize > Count())
            {
                EnsureCapacity(newSize);

                // Add more items until reach the new size
                int32 chunkIndex = 0;
                int32 itemsReaming = newSize - Count();
                while (itemsReaming != 0)
                {
                    ENGINE_ASSERT(chunkIndex != m_chunks.Count());
                    auto& chunk = m_chunks[chunkIndex];

                    // Insert some items to this chunk if can
                    const int32 spaceLeft = chunk->Capacity() - chunk->Count();
                    int32 itemsToAdd = Math::Min(itemsReaming, spaceLeft);
                    chunk->Resize(chunk->Count() + itemsToAdd);
                    m_count += itemsToAdd;

                    // Update counter
                    itemsReaming = newSize - Count();

                    // Move to the next chunk to fill it
                    chunkIndex++;
                }
            }
            ENGINE_ASSERT(newSize == Count());
        }

        /// <summary>
        /// Searches for the specified object and returns the zero-based index of the first occurrence within the entire collection.
        /// </summary>
        /// <param name="item">The item to find.</param>
        /// <returns>The zero-based index of the first occurrence of itm within the entire collection, if found; otherwise, INVALID_INDEX.</returns>
        int32 Find(const T& item) const
        {
            int32 startIndex = 0;
            for (int32 chunkIndex = 0; chunkIndex < m_chunks.Count(); chunkIndex++)
            {
                const int32 index = m_chunks[chunkIndex]->Find(item);
                if (index != INVALID_INDEX)
                    return startIndex + index;
                startIndex += ChunkSize;
                if (startIndex > m_count)
                    break;
            }
            return INVALID_INDEX;
        }

    public:
        Iterator Begin() const
        {
            return Iterator(this, 0);
        }

        Iterator End() const
        {
            return Iterator(this, m_count);
        }

        Iterator IteratorAt(int32 index) const
        {
            return Iterator(this, index);
        }

        inline Iterator begin()
        {
            return Iterator(this, 0);
        }

        inline Iterator end()
        {
            return Iterator(this, m_count);
        }

        inline const Iterator begin() const
        {
            return Iterator(this, 0);
        }

        inline const Iterator end() const
        {
            return Iterator(this, m_count);
        }
    };
}
