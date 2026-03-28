#pragma once

#include "Base/GPUResource.h"
#include "Core/Profiler/ProfilerGPU.h"

namespace SE
{
	/// <summary>
	/// Represents a GPU query that measures execution time of GPU operations.
	/// The query will measure any GPU operations that take place between its Begin() and End() calls.
	/// </summary>
	class SE_API_RUNTIME GPUTimerQuery : public GPUResource, public IGPUTimerQueryProfile
	{
	public:
		/// <summary>
		/// Starts the counter.
		/// </summary>
		void Begin() override = 0;

		/// <summary>
		/// Stops the counter. Can be called more than once without failing.
		/// </summary>
		void End() override = 0;

		/// <summary>
		/// Determines whether this query has been completed and has valid result to gather.
		/// </summary>
		/// <returns><c>true</c> if this query has result; otherwise, <c>false</c>.</returns>
		bool HasResult() override = 0;

		/// <summary>
		/// Gets the query result time (in milliseconds) it took to execute GPU commands between Begin/End calls.
		/// </summary>
		/// <returns>The time in milliseconds.</returns>
		float GetResult() override = 0;

	public:
		// [GPUResource]
		String ToString() const override
		{
			return SE_TEXT("TimerQuery");
		}
		// [GPUResource]
		GPUResourceType GetResType() const override
		{
			return GPUResourceType::Query;
		}
	};
}
