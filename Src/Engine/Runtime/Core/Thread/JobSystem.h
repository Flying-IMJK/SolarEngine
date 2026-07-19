#pragma once

#include "Runtime/API.h"
#include "Runtime/Core/Types/Delegate.h"
#include "Runtime/Core/Systems.h"

namespace SE::Threading
{
	typedef int64 JobHandle;

	class IJob
	{
		NON_COPYABLE(IJob)
	public:
		virtual void Run(int32 index) = 0;
		IJob() = default;
		virtual ~IJob() = default;
	};

	/// <summary>
	/// Lightweight multi-threaded jobs execution scheduler. Uses a pool of threads and supports work-stealing concept.
	/// </summary>
	class SE_API_RUNTIME JobSystem
	{
	public:
		/// <summary>
		/// Executes the job (utility to call dispatch and wait for the end).
		/// </summary>
		/// <param name="job">The job. Argument is an index of the job execution.</param>
		/// <param name="jobCount">The job executions count.</param>
		static void Execute(const Function<void(int32)> &job, int32 jobCount = 1);

		/// <summary>
		/// Dispatches the job for the execution.
		/// </summary>
		/// <param name="job">The job. Argument is an index of the job execution.</param>
		/// <param name="jobCount">The job executions count.</param>
		/// <returns>The label identifying this dispatch. Can be used to wait for the execution end.</returns>
		static JobHandle Dispatch(const Function<void(int32)> &job, int32 jobCount = 1);

		static JobHandle Dispatch(IJob* job, int32 jobCount = 1);

		/// <summary>
		/// 给定Job是否完成
		/// </summary>
		static bool IsCompleted(JobHandle handle);

		/// <summary>
		/// 等待所有已分派的Job完成。
		/// </summary>
		static void Wait();

		/// <summary>
		/// 等待所有 dispatch 的作业，直到给定Job完成
		/// </summary>
		/// <param name="label">The label.</param>
		static void Wait(JobHandle handle);

		/// <summary>
		/// 设置在调度时是否自动启动作业执行。如果禁用，作业将不会执行，直到重新启用。可用于优化应重叠的多个调度的执行。
		/// </summary>
		static void SetJobStartingOnDispatch(bool value);

		/// <summary>
		/// Gets the amount of job system threads.
		/// </summary>
		static int32 GetThreadsCount();
	};
}