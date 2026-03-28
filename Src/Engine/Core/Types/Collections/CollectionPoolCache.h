#pragma once
#include "Core/Types/Collections/List.h"
#include "Core/Platform/CriticalSection.h"

namespace SE
{
    namespace CollectionPoolCacheUtils
    {
        /// <summary>
        /// Clear callback used to initialize the given collection container type (clear array, etc.). Called when pool item is being reused or initialized.
        /// </summary>
        template<typename T>
        using ClearCallback = void(*)(T*);

        /// <summary>
        /// Create callback spawns a new entry of the pooled collection
        /// </summary>
        template<typename T>
        using CreateCallback = T * (*)();

        template<typename T>
        inline void DefaultClearCallback(T* obj)
        {
            obj->Clear();
        }

        template<typename T>
        inline T* DefaultCreateCallback()
        {
            return New<T>();
        }
    }

    /// <summary>
    /// Cache container that holds a list of cached collections to allow reuse and reduced memory allocation amount. Helps with sharing data across code and usages. It's thread-safe.
    /// </summary>
    template<typename T, CollectionPoolCacheUtils::ClearCallback<T> ClearCallback = CollectionPoolCacheUtils::DefaultClearCallback<T>, CollectionPoolCacheUtils::CreateCallback<T> CreateCallback = CollectionPoolCacheUtils::DefaultCreateCallback<T>>
    class CollectionPoolCache
    {
    public:
        /// <summary>
        /// Helper object used to access the pooled collection and return it to the pool after usage (on code scope execution end).
        /// </summary>
        struct ScopeCache
        {
            friend CollectionPoolCache;

        private:
            CollectionPoolCache* m_Pool;

            FORCE_INLINE ScopeCache(CollectionPoolCache* pool, T* value)
            {
                m_Pool = pool;
                Value = value;
            }

        public:
            T* Value;

            ScopeCache() = delete;
            ScopeCache(const ScopeCache& other) = delete;
            ScopeCache& operator=(const ScopeCache& other) = delete;
            ScopeCache& operator=(ScopeCache&& other) noexcept = delete;

            ScopeCache(ScopeCache&& other) noexcept
            {
                Value = other.Value;
                other.Value = nullptr;
            }

            ~ScopeCache()
            {
                m_Pool->Put(Value);
            }

            T* operator->()
            {
                return Value;
            }

            const T* operator->() const
            {
                return Value;
            }

            T& operator*()
            {
                return *Value;
            }

            const T& operator*() const
            {
                return *Value;
            }
        };

    private:
        CriticalSection m_Locker;
        List<T*, InlinedAllocation<64>> m_Pool;

    public:
        /// <summary>
        /// Finalizes an instance of the <see cref="CollectionPoolCache"/> class.
        /// </summary>
        ~CollectionPoolCache()
        {
            m_Pool.ClearDelete();
        }

    public:
        /// <summary>
        /// Gets the collection instance from the pool. Can reuse the object from the pool or create a new one. Returns collection is always cleared and ready to use.
        /// </summary>
        /// <returns>The collection (cleared).</returns>
        FORCE_INLINE ScopeCache Get()
        {
            return ScopeCache(this, GetUnscoped());
        }

        /// <summary>
        /// Gets the collection instance from the pool. Can reuse the object from the pool or create a new one. Returns collection is always cleared and ready to use.
        /// </summary>
        /// <returns>The collection (cleared).</returns>
        T* GetUnscoped()
        {
            T* result;
            m_Locker.Lock();
            if (m_Pool.HasItems())
            {
                result = m_Pool.Pop();
            }
            else
            {
                result = CreateCallback();
            }
            m_Locker.Unlock();
            ClearCallback(result);
            return result;
        }

        /// <summary>
        /// Puts the collection value back to the pool.
        /// </summary>
        void Put(T* value)
        {
            m_Locker.Lock();
            m_Pool.Add(value);
            m_Locker.Unlock();
        }

        /// <summary>
        /// Releases all the allocated resources (existing in the pool that are not during usage).
        /// </summary>
        void Release()
        {
            m_Locker.Lock();
            m_Pool.ClearDelete();
            m_Pool.Resize(0);
            m_Locker.Unlock();
        }
    };
} // SE
