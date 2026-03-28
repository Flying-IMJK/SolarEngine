#pragma once

#include "Core/TYpes/Collections/ConcurrentQueue.h"
#include "Task.h"

namespace SE::Threading
{
	/// <summary>
	/// Lock-free implementation of thread-safe tasks queue.
	/// </summary>
	template<typename T = Task>
	class ConcurrentTaskQueue : public ConcurrentQueue<T*>
	{
	public:

		/// <summary>
		/// Adds item to the collection (thread-safe).
		/// </summary>
		/// <param name="item">Item to add.</param>
		inline void Add(T* item)
		{
			ConcurrentQueue<T*>::enqueue(item);
		}

		/// <summary>
		/// Cancels all the tasks from the queue and removes them.
		/// </summary>
		void CancelAll()
		{
			T* tasks[16];
			std::size_t count;
			while ((count = ConcurrentQueue<T*>::try_dequeue_bulk(tasks, ARRAY_SIZE(tasks))) != 0)
			{
				for (std::size_t i = 0; i != count; i++)
				{
					tasks[i]->Cancel();
				}
			}
		}
	};
}
