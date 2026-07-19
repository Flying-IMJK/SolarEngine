
#pragma once

#include "Runtime/Core/Types/Delegate.h"
#include "Runtime/Core/Types/Collections/SamplesBuffer.h"

#include "Runtime/API.h"

namespace SE
{
	namespace Threading
	{
		class Task;
	}

	class StreamingGroup;

	/// <summary>
	/// Base class for all resource types that can be dynamically streamed.
	/// </summary>
	class SE_API_RUNTIME StreamableResource
	{
	protected:

		StreamingGroup* m_Group;
		bool m_IsDynamic, m_IsStreaming;
		float m_StreamingQuality;

		StreamableResource();
		StreamableResource(StreamingGroup* group);
		~StreamableResource();

	public:

		/// <summary>
		/// Gets resource group.
		/// </summary>
		FORCE_INLINE StreamingGroup* GetGroup() const
		{
			return m_Group;
		}

		/// <summary>
		/// Gets value indicating whenever resource can be used in dynamic streaming (otherwise use always the best quality).
		/// </summary>
		FORCE_INLINE bool IsDynamic() const
		{
			return m_IsDynamic;
		}

		/// <summary>
		/// Gets resource streaming quality level
		/// </summary>
		FORCE_INLINE float GetStreamingQuality() const
		{
			return m_StreamingQuality;
		}

		/// <summary>
		/// Gets resource target residency level.
		/// </summary>
		FORCE_INLINE int32 GetTargetResidency() const
		{
			return Streaming.TargetResidency;
		}

		/// <summary>
		/// Gets a value indicating whether this resource has been allocated.
		/// </summary>
		FORCE_INLINE bool IsAllocated() const
		{
			return GetAllocatedResidency() != 0;
		}

		/// <summary>
		/// 获取资源最大驻留级别。
		/// </summary>
		virtual int32 GetMaxResidency() const = 0;

		/// <summary>
		/// 获取资源当前驻留级别。
		/// </summary>
		virtual int32 GetCurrentResidency() const = 0;

		/// <summary>
		/// 获取资源分配的驻留级别。
		/// </summary>
		virtual int32 GetAllocatedResidency() const = 0;

	public:

		/// <summary>
		/// Determines whether this instance can be updated. Which means: no async streaming, no pending action in background.
		/// </summary>
		/// <returns><c>true</c> if this instance can be updated; otherwise, <c>false</c>.</returns>
		virtual bool CanBeUpdated() const = 0;

		/// <summary>
		/// Updates the resource allocation to the given residency level. May not be updated now but in an async operation.
		/// </summary>
		/// <param name="residency">The target allocation residency.</param>
		/// <returns>Async task that updates resource allocation or null if already done it. Warning: need to call task start to perform allocation.</returns>
		virtual Threading::Task* UpdateAllocation(int32 residency) = 0;

		/// <summary>
		/// Creates streaming task (or tasks sequence) to perform resource streaming for the desire residency level.
		/// </summary>
		/// <param name="residency">The target residency.</param>
		/// <returns>Async task or tasks that update resource residency level. Must be preceded with UpdateAllocation call.</returns>
		virtual Threading::Task* CreateStreamingTask(int32 residency) = 0;

		/// <summary>
		/// Cancels any streaming task (or tasks sequence) started for this resource.
		/// </summary>
		virtual void CancelStreamingTasks() = 0;

	public:

		struct StreamingCache
		{
			double LastUpdateTime = 0.0;
			double TargetResidencyChangeTime = 0;
			int32 TargetResidency = 0;
			bool Error = false;
			SamplesBuffer<float, 5> QualitySamples;
		};

		StreamingCache Streaming;

		/// <summary>
		/// Event called when current resource residency gets changed (eg. model LOD or texture MIP gets loaded). Usually called from async thread.
		/// </summary>
		Action ResidencyChanged;

		/// <summary>
		/// Requests the streaming update for this resource during next streaming manager update.
		/// </summary>
		void RequestStreamingUpdate();

		/// <summary>
		/// Stops the streaming (eg. on streaming fail).
		/// </summary>
		/// <param name="error">True if streaming failed.</param>
		void ResetStreaming(bool error = true);

	protected:

		void StartStreaming(bool isDynamic);
		void StopStreaming();
	};
}