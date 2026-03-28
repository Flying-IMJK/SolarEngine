#pragma once

#include "Core/Thread/Task.h"
#include "GPUTasksContext.h"

namespace SE
{
	class GPUResource;
	class GPUTasksContext;

	/// <summary>
	/// Describes GPU work object.
	/// </summary>
	class GPUTask : public Threading::Task
	{
		friend GPUTasksContext;

	public:
		/// <summary>
		/// Describes GPU work type
		/// </summary>
		enum class Type { Custom, CopyResource, UploadTexture, UploadBuffer};

		/// <summary>
		/// Describes GPU work result value
		/// </summary>
		enum class Result{ Ok, Failed, MissingResources, MissingData};

	public:
		/// <summary>
		/// Gets a task type.
		/// </summary>
		/// <returns>The type.</returns>
		FORCE_INLINE Type GetType() const
		{
			return m_Type;
		}

		/// <summary>
		/// Gets work finish synchronization point
		/// </summary>
		/// <returns>Finish task sync point</returns>
		FORCE_INLINE GPUSyncPoint GetSyncPoint() const
		{
			return m_SyncPoint;
		}

		/// <summary>
		/// Checks if operation is syncing
		/// </summary>
		/// <returns>True if operation is syncing, otherwise false</returns>
		FORCE_INLINE bool IsSyncing() const
		{
			return IsState(Threading::TaskState::Running) && m_SyncPoint != 0;
		}

		/// <summary>
		/// Executes this task.
		/// </summary>
		/// <param name="context">The context.</param>
		void Execute(GPUTasksContext* context);

		/// <summary>
		/// Action fired when asynchronous operation has been synchronized with a GPU
		/// </summary>
		void Sync();

		/// <summary>
		/// Cancels the task results synchronization.
		/// </summary>
		void CancelSync();

		// [Task]
		String ToString() const override;

	protected:
		/// <summary>
		/// Initializes a new instance of the <see cref="GPUTask"/> class.
		/// </summary>
		/// <param name="type">The type.</param>
		explicit GPUTask(Type type);

		virtual Result run(GPUTasksContext* context) = 0;

		virtual void OnSync() { }

		// [Task]
		void Enqueue() override;

		bool Run() override;

		void OnCancel() override;

	private:
		/// <summary>
		/// Task type
		/// </summary>
		Type m_Type;

		/// <summary>
		/// Synchronization point when async task has been done
		/// </summary>
		GPUSyncPoint m_SyncPoint;

		/// <summary>
		/// The context that performed this task, it's should synchronize it.
		/// </summary>
		GPUTasksContext* m_Context;

	};
}
