#pragma once

#include "Runtime/Core/Memory/Memory.h"
#define MOODYCAMEL_EXCEPTIONS_ENABLED 0
#include "Runtime/ThirdParty/concurrentqueue/concurrentqueue.h"

namespace SE
{
	/// <summary>
	/// The default engine configuration for concurrentqueue.
	/// </summary>
	struct ConcurrentQueueSettings : public moodycamel::ConcurrentQueueDefaultTraits
	{
		// Use bigger blocks
		static const size_t BLOCK_SIZE = 256;

		// Use default engine memory allocator
		static inline void* malloc(size_t size)
		{
			return PlatformAllocator::Allocate((uint64)size);
		}

		// Use default engine memory allocator
		static inline void free(void* ptr)
		{
			return PlatformAllocator::Free(ptr);
		}
	};

	/// <summary>
	/// 线程安全队列的无锁实现。
	/// Based on: https://github.com/cameron314/concurrentqueue
	/// </summary>
	template<typename T>
	class ConcurrentQueue : public moodycamel::ConcurrentQueue<T, ConcurrentQueueSettings>
	{
	public:
		typedef moodycamel::ConcurrentQueue<T, ConcurrentQueueSettings> Base;

	public:
		/// <summary>
		/// 获取队列中当前元素总数的估计值。
		/// </summary>
		inline int32 Count() const
		{
			return static_cast<int32>(Base::size_approx());
		}

		/// <summary>
		/// Adds item to the collection.
		/// </summary>
		/// <param name="item">The item to add.</param>
		inline void Add(const T& item)
		{
			enqueue(item);
		}

		/// <summary>
		/// Adds item to the collection.
		/// </summary>
		/// <param name="item">The item to add.</param>
		inline void Add(T&& item)
		{
			enqueue(item);
		}
	};

}