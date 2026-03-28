#pragma once

#include "Core/Templates.h"
#include "Core/Platform/Platform.h"
#include "Core/Logging/Logging.h"
#include "memory"

namespace SE
{
    namespace Memory
    {
		inline uint64 CalculatePaddingForAlignment(uintptr_t addressOffset, uint64 requiredAlignment)
		{
			return (requiredAlignment - (addressOffset % requiredAlignment)) % requiredAlignment;
		}

		inline uint64 CalculatePaddingForAlignment(void *address, uint64 requiredAlignment)
		{
			return CalculatePaddingForAlignment(reinterpret_cast<uintptr>(address), requiredAlignment);
		}

        /// <summary>
        /// Constructs the item in the memory.
        /// </summary>
        /// <remarks>The optimized version is noop.</remarks>
        /// <param name="dst">The address of the memory location to construct.</param>
        template<typename T>
        inline typename TEnableIf<!TIsTriviallyConstructible<T>::Value>::Type ConstructItem(T *dst)
        {
            new(dst) T();
        }

        /// <summary>
        /// Constructs the item in the memory.
        /// </summary>
        /// <remarks>The optimized version is noop.</remarks>
        /// <param name="dst">The address of the memory location to construct.</param>
        template<typename T>
        inline typename TEnableIf<TIsTriviallyConstructible<T>::Value>::Type ConstructItem(T *dst)
        {
            // More undefined behavior! No more clean memory! More performance! Yay!
            Platform::MemoryClear(dst, sizeof(T));
        }

        /// <summary>
        /// Constructs the range of items in the memory.
        /// </summary>
        /// <remarks>The optimized version is noop.</remarks>
        /// <param name="dst">The address of the first memory location to construct.</param>
        /// <param name="count">The number of element to construct. Can be equal 0.</param>
        template<typename T>
        inline typename TEnableIf<!TIsTriviallyConstructible<T>::Value>::Type ConstructItems(T *dst, int32 count)
        {
            while (count--)
            {
                new(dst) T();
                ++(T * &)dst;
            }
        }

		/// <summary>
		/// Constructs the range of items in the memory.
		/// </summary>
		/// <remarks>The optimized version is noop.</remarks>
		/// <param name="dst">The address of the first memory location to construct.</param>
		/// <param name="count">The number of element to construct. Can be equal 0.</param>
		template<typename T>
		inline typename TEnableIf<TIsTriviallyConstructible<T>::Value>::Type ConstructItems(T *dst, int32 count)
		{
			// More undefined behavior! No more clean memory! More performance! Yay!
			Platform::MemoryClear(dst, count * sizeof(T));
		}

        /// <summary>
        /// Constructs the range of items in the memory from the set of arguments.
        /// </summary>
        /// <remarks>The optimized version uses low-level memory copy.</remarks>
        /// <param name="dst">The address of the first memory location to construct.</param>
        /// <param name="value">The address of the first memory location to pass to the constructor.</param>
        /// <param name="count">The number of element to construct. Can be equal 0.</param>
        template<typename T, typename U>
        inline typename TEnableIf<!TIsBitwiseConstructible<T, U>::Value>::Type ConstructItems(T *dst, const U *value, int32 count)
        {
            while (count--)
            {
                new(dst) T(*value);
                ++(T * &)(dst);
                ++value;
            }
        }

        /// <summary>
        /// Constructs the range of items in the memory from the set of arguments.
        /// </summary>
        /// <remarks>The optimized version uses low-level memory copy.</remarks>
        /// <param name="dst">The address of the first memory location to construct.</param>
        /// <param name="src">The address of the first memory location to pass to the constructor.</param>
        /// <param name="count">The number of element to construct. Can be equal 0.</param>
        template<typename T, typename U>
        inline typename TEnableIf<TIsBitwiseConstructible<T, U>::Value>::Type ConstructItems(T *dst, const U *src, int32 count)
        {
			Platform::MemoryCopy(dst, src, sizeof(U) * count);
        }

        /// <summary>
        /// Destructs the item in the memory.
        /// </summary>
        /// <remarks>The optimized version is noop.</remarks>
        /// <param name="dst">The address of the memory location to destruct.</param>
        template<typename T>
        inline typename TEnableIf<!TIsTriviallyDestructible<T>::Value>::Type DestructItem(T *dst)
        {
            dst->~T();
        }

        /// <summary>
        /// Destructs the item in the memory.
        /// </summary>
        /// <remarks>The optimized version is noop.</remarks>
        /// <param name="dst">The address of the memory location to destruct.</param>
        template<typename T>
        inline typename TEnableIf<TIsTriviallyDestructible<T>::Value>::Type DestructItem(T *dst)
        {
        }

        /// <summary>
        /// Destructs the range of items in the memory.
        /// </summary>
        /// <remarks>The optimized version is noop.</remarks>
        /// <param name="dst">The address of the first memory location to destruct.</param>
        /// <param name="count">The number of element to destruct. Can be equal 0.</param>
        template<typename T>
        inline typename TEnableIf<!TIsTriviallyDestructible<T>::Value>::Type DestructItems(T *dst, int32 count)
        {
            while (count--)
            {
                dst->~T();
                ++dst;
            }
        }

        /// <summary>
        /// Destructs the range of items in the memory.
        /// </summary>
        /// <remarks>The optimized version is noop.</remarks>
        /// <param name="dst">The address of the first memory location to destruct.</param>
        /// <param name="count">The number of element to destruct. Can be equal 0.</param>
        template<typename T>
        inline typename TEnableIf<TIsTriviallyDestructible<T>::Value>::Type DestructItems(T *dst, int32 count)
        {
        }

        /// <summary>
        /// Copies the range of items using the assignment operator.
        /// </summary>
        /// <remarks>The optimized version is low-level memory copy.</remarks>
        /// <param name="dst">The address of the first memory location to start assigning to.</param>
        /// <param name="src">The address of the first memory location to assign from.</param>
        /// <param name="count">The number of element to assign. Can be equal 0.</param>
        template<typename T>
        inline typename TEnableIf<!TIsTriviallyCopyAssignable<T>::Value>::Type CopyItems(T *dst, const T *src, int32 count)
        {
            while (count--)
            {
                *dst = *src;
                ++dst;
                ++src;
            }
        }

        /// <summary>
        /// Copies the range of items using the assignment operator.
        /// </summary>
        /// <remarks>The optimized version is low-level memory copy.</remarks>
        /// <param name="dst">The address of the first memory location to start assigning to.</param>
        /// <param name="src">The address of the first memory location to assign from.</param>
        /// <param name="count">The number of element to assign. Can be equal 0.</param>
        template<typename T>
        inline typename TEnableIf<TIsTriviallyCopyAssignable<T>::Value>::Type CopyItems(T *dst, const T *src, int32 count)
        {
			Platform::MemoryCopy(dst, src, count * sizeof(T));
        }

        /// <summary>
        /// Moves the range of items in the memory from the set of arguments.
        /// </summary>
        /// <remarks>The optimized version uses low-level memory copy.</remarks>
        /// <param name="dst">The address of the first memory location to move.</param>
        /// <param name="src">The address of the first memory location to pass to the move constructor.</param>
        /// <param name="count">The number of element to move. Can be equal 0.</param>
        template<typename T, typename U>
        inline typename TEnableIf<!TIsBitwiseConstructible<T, U>::Value>::Type MoveItems(T *dst, const U *src, int32 count)
        {
            while (count--)
            {
                new(dst) T((T &&) *src);
                ++(T * &)dst;
                ++src;
            }
        }

        /// <summary>
        /// Moves the range of items in the memory from the set of arguments.
        /// </summary>
        /// <remarks>The optimized version uses low-level memory copy.</remarks>
        /// <param name="dst">The address of the first memory location to move.</param>
        /// <param name="src">The address of the first memory location to pass to the move constructor.</param>
        /// <param name="count">The number of element to move. Can be equal 0.</param>
        template<typename T, typename U>
        inline typename TEnableIf<TIsBitwiseConstructible<T, U>::Value>::Type MoveItems(T *dst, const U *src, int32 count)
        {
			Platform::MemoryCopy(dst, src, count * sizeof(U));
        }
    }

	/// <summary>
	/// The default implementation of the memory allocator using traditional ANSI allocation API under the hood (malloc/free).
	/// </summary>
	class SE_API_CORE PlatformAllocator
	{
	public:

		/// <summary>
		/// Allocates memory on a specified alignment boundary.
		/// </summary>
		/// <param name="size">The size of the allocation (in bytes).</param>
		/// <param name="alignment">The memory alignment (in bytes). Must be an integer power of 2.</param>
		/// <returns>The pointer to the allocated chunk of the memory. The pointer is a multiple of alignment.</returns>
    	static void* Allocate(uint64 size, uint64 alignment = 16);

		/// <summary>
		/// Reallocates block of the memory.
		/// </summary>
		/// </summary>
		/// <param name="ptr">A pointer to the memory block to reallocate.</param>
		/// <param name="newSize">The size of the new allocation (in bytes).</param>
		/// <returns>The pointer to the allocated chunk of the memory. The pointer is a multiple of alignment.</returns>
    	static void* Realloc(void* ptr, uint64 newSize);

		/// <summary>
		/// Reallocates block of the memory.
		/// </summary>
		/// </summary>
		/// <param name="ptr">A pointer to the memory block to reallocate.</param>
		/// <param name="newSize">The size of the new allocation (in bytes).</param>
		/// <param name="alignment">The memory alignment (in bytes). Must be an integer power of 2.</param>
		/// <returns>The pointer to the allocated chunk of the memory. The pointer is a multiple of alignment.</returns>
    	static void* ReallocAligned(void* ptr, uint64 newSize, uint64 alignment);

		/// <summary>
		/// Reallocates block of the memory.
		/// </summary>
		/// </summary>
		/// <param name="ptr">A pointer to the memory block to reallocate.</param>
		/// <param name="oldSize">The size of the old allocation (in bytes).</param>
		/// <param name="newSize">The size of the new allocation (in bytes).</param>
		/// <returns>The pointer to the allocated chunk of the memory. The pointer is a multiple of alignment.</returns>
		static void* Realloc(void* ptr, uint64 oldSize, uint64 newSize);

		/// <summary>
		/// Frees a block of allocated memory.
		/// </summary>
		/// <param name="ptr">A pointer to the memory block to deallocate.</param>
		static void Free(void* ptr);

		/// <summary>
		/// Gets the name of the allocator.
		/// </summary>
		/// <returns>The name.</returns>
		static const Char* Name();
	};

    //-------------------------------------------------------------------------
    template <class T, class MemoryAllocator = PlatformAllocator, class... Args>
    inline T *New(Args&&... params)
    {
		T *pMemory = static_cast<T*>(MemoryAllocator::Allocate(sizeof(T), alignof(T)));
		if (pMemory == nullptr)
		{
			throw std::bad_alloc();
		}

		new(pMemory) T(Forward<Args>(params)...);
        return pMemory;
    }

	template <class T, class MemoryAllocator = PlatformAllocator>
	inline T *NewArray(uint64 const numElements)
	{
		T* pMemory = static_cast<T*>(MemoryAllocator::Allocate(sizeof(T) * numElements, alignof(T)));

		if (pMemory == nullptr)
		{
			throw std::bad_alloc();
		}

		Memory::ConstructItems(pMemory, numElements);

		return pMemory;
	}

	template <class T, class MemoryAllocator = PlatformAllocator, class... Args>
	inline T *NewArray(uint64 const numElements, Args &&...params)
	{
		T* pMemory = static_cast<T*>(MemoryAllocator::Allocate(sizeof(T) * numElements, alignof(T)));

		if (pMemory == nullptr)
		{
			throw std::bad_alloc();
		}

		for (int i = 0; i < numElements; ++i)
		{
			new(pMemory) T(Forward<Args>(params)...);
			++pMemory;
		}

		return pMemory;
	}

	template <class T, class MemoryAllocator = PlatformAllocator>
	inline T *NewArray(uint64 const numElements, T const &value)
	{
		T* pMemory = static_cast<T*>(MemoryAllocator::Allocate(sizeof(T) * numElements, alignof(T)));

		if (pMemory == nullptr)
		{
			throw std::bad_alloc();
		}

		for (int i = 0; i < numElements; ++i)
		{
			new(pMemory) T(value);
			++pMemory;
		}

		return pMemory;
	}

    template <class T>
    inline void Delete(T *ptr)
    {
        if (ptr != nullptr)
        {
			Memory::DestructItem(ptr);
			Platform::Free(ptr);
        }
    }

    template <typename T>
    inline void DeleteArray(T *&pArray, int32 count)
    {
		Memory::DestructItems(pArray, count);
		Platform::Free(pArray);
        pArray = nullptr;
    }

	//-------------------------------------------------------------------------
	#define SAFE_DELETE(x) if(x) { Delete(x); (x) = nullptr; }
	#define OUT_OF_MEMORY
	//-------------------------------------------------------------------------

	template <typename T>
	struct DefaultDeleter
	{
		constexpr DefaultDeleter() noexcept = default;

		void operator()( T* p ) const noexcept
		{
			Delete( p );
		}
	};

	template <typename T>
	struct DefaultDeleter<T[]> // Specialization for arrays.
	{
		constexpr DefaultDeleter() noexcept = default;

		void operator()( T* p ) const noexcept
		{
			DeleteArray(p);
		}
	};

	//-------------------------------------------------------------------------
	template <typename T>
	using Scope = std::unique_ptr<T, DefaultDeleter<T>>;

	template <typename T, typename... Args>
	constexpr Scope<T> CreateScope(Args &&...args)
	{
		return Scope<T>(New<T>(std::forward<Args>(args)...));
	}

	template <typename T>
	using Ref = std::shared_ptr<T>;

	template <typename T, typename... Args>
	constexpr Ref<T> MakeRef(Args &&...args)
	{
		return std::make_shared<T>(std::forward<Args>(args)...);
	}

	template <typename T>
	constexpr Ref<T> CreateRef(T* target)
	{
		return Ref<T>(target);
	}

	template <typename T>
	using WeakRef = std::weak_ptr<T>;
}