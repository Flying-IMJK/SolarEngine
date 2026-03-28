
#pragma once

#include "Core/Memory/Memory.h"

namespace SE
{
    /// <summary>
    /// Implementation of STL memory allocator that uses Flax default Allocator.
    /// <summary>
    template<class T>
    class SE_API_CORE STLAllocator
    {
    public:

#if PLATFORM_64BITS
        typedef unsigned long long size_type;
    typedef long long difference_type;
#else
        typedef unsigned int size_type;
        typedef int difference_type;
#endif
        typedef T *pointer;
        typedef const T *const_pointer;
        typedef T &reference;
        typedef const T &const_reference;
        typedef T value_type;

        STLAllocator()
        {
        }

        STLAllocator(const STLAllocator &)
        {
        }

        pointer allocate(size_type n, const void * = 0)
        {
            return (pointer) PlatformAllocator::Allocate(n * sizeof(T));
        }

        void deallocate(void *p, size_type)
        {
			PlatformAllocator::Free(p);
        }

        pointer address(reference x) const
        {
            return &x;
        }

        const_pointer address(const_reference x) const
        {
            return &x;
        }

        STLAllocator<T> &operator=(const STLAllocator &)
        {
            return *this;
        }

        void construct(pointer p, const T &val)
        {
            new((T *) p) T(val);
        }

        void destroy(pointer p)
        {
            p->~T();
        }

        size_type max_size() const
        {
            return size_type(-1);
        }

        template<class U>
        struct rebind
        {
            typedef STLAllocator<U> other;
        };

        template<class U>
        STLAllocator(const STLAllocator<U> &)
        {
        }

        template<class U>
        STLAllocator &operator=(const STLAllocator<U> &)
        {
            return *this;
        }
    };

}