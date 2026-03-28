#pragma once

#include "Core/Platform/CriticalSection.h"
#include "Core/Types/Collections/List.h"
#include "Runtime/Graphics/GPUContext.h"

namespace SE
{
	/// <summary>
	/// Helper type used to synchronize CPU and GPU work
	/// </summary>
	typedef uint64 GPUSyncPoint;

	/// <summary>
	/// Default latency (in frames) between CPU and GPU used for async gpu jobs
	/// </summary>
	#define GPU_ASYNC_LATENCY 2

	class GPUTask;
	class GPUContext;

	/// <summary>
	/// GPU tasks context
	/// </summary>
	class GPUTasksContext
	{
	protected:
		CriticalSection _locker;
		GPUSyncPoint _currentSyncPoint;
		List<GPUTask*> _tasksDone;
		int32 _totalTasksDoneCount;

	public:
		/// <summary>
		/// Initializes a new instance of the <see cref="GPUTasksContext"/> class.
		/// </summary>
		/// <param name="device">The graphics device.</param>
		GPUTasksContext(GPUDevice* device);

		/// <summary>
		/// Finalizes an instance of the <see cref="GPUTasksContext"/> class.
		/// </summary>
		~GPUTasksContext();

	public:
		/// <summary>
		/// The GPU commands context used for tasks execution (can be only copy/upload without graphics capabilities on some platforms).
		/// </summary>
		GPUContext* GPU;

	public:
		/// <summary>
		/// Gets graphics device handle
		/// </summary>
		/// <returns>Graphics device</returns>
		FORCE_INLINE GPUDevice* GetDevice() const
		{
			return GPU->GetDevice();
		}

		/// <summary>
		/// Gets current synchronization point of that context (CPU position, GPU has some latency)
		/// </summary>
		/// <returns>Context sync point</returns>
		FORCE_INLINE GPUSyncPoint GetCurrentSyncPoint() const
		{
			return _currentSyncPoint;
		}

		/// <summary>
		/// Gets total amount of tasks done by this context
		/// </summary>
		/// <returns>Done tasks count</returns>
		FORCE_INLINE int32 GetTotalTasksDoneCount() const
		{
			return _totalTasksDoneCount;
		}

		/// <summary>
		/// Perform given task
		/// </summary>
		/// <param name="task">Task to do</param>
		void Run(GPUTask* task);

		void OnCancelSync(GPUTask* task);

	public:
		void OnFrameBegin();

		void OnFrameEnd();
	};
}
