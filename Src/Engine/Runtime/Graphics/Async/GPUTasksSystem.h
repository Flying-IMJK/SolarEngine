#pragma once

#include "Runtime/Core/Thread/ConcurrentTaskQueue.h"
#include "Runtime/Core/Types/Collections/List.h"

namespace SE
{
	class GPUTask;
	class GPUDevice;
	class GPUTasksExecutor;
	class GPUTasksContext;

	/// <summary>
	/// Graphics Device work manager.
	/// </summary>
	class GPUTasksSystem
	{
		friend GPUDevice;
		friend GPUTask;
		NON_COPYABLE(GPUTasksSystem)
	private:
		GPUTasksExecutor* _executor = nullptr;
		Threading::ConcurrentTaskQueue<GPUTask> _tasks;
		List<GPUTask*> _buffers[2];
		int32 _bufferIndex = 0;

	public:
		GPUTasksSystem();

		/// <summary>
		/// Gets the GPU tasks executor.
		/// </summary>
		FORCE_INLINE GPUTasksExecutor* GetExecutor() const
		{
			return _executor;
		}

		/// <summary>
		/// Sets the GPU tasks executor.
		/// </summary>
		/// <param name="value">The tasks executor.</param>
		void SetExecutor(GPUTasksExecutor* value);

		/// <summary>
		/// Gets the amount of enqueued tasks to perform.
		/// </summary>
		FORCE_INLINE int32 GetTaskCount() const
		{
			return _tasks.Count();
		}

	public:
		/// <summary>
		/// Clears asynchronous resources loading queue and cancels all tasks.
		/// </summary>
		void Dispose();

	public:
		/// <summary>
		/// On begin rendering frame.
		/// </summary>
		void FrameBegin();

		/// <summary>
		/// On end rendering frame.
		/// </summary>
		void FrameEnd();

	public:
		/// <summary>
		/// Requests work to do. Should be used only by GPUTasksExecutor.
		/// </summary>
		/// <param name="buffer">The output buffer.</param>
		/// <param name="maxCount">The maximum allowed amount of tasks to get.</param>
		/// <returns>The amount of tasks added to the buffer.</returns>
		int32 RequestWork(GPUTask** buffer, int32 maxCount);
	};
}
