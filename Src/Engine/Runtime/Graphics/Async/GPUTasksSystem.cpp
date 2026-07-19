#include "GPUTasksSystem.h"

#include "GPUTask.h"
#include "GPUTasksExecutor.h"
#include "Runtime/Core/TypeSystem/Types.h"
#include "Runtime/Graphics/GPUDevice.h"

namespace SE
{
	GPUTask::GPUTask(Type type) : m_Type(type), m_SyncPoint(0), m_Context(nullptr)
	{
	}

	void GPUTask::Execute(GPUTasksContext* context)
	{
		ENGINE_ASSERT(IsState(Threading::TaskState::Queued) && m_Context == nullptr);
		SetState(Threading::TaskState::Running);

		// Perform an operation
		const auto result = run(context);

		// Process result
		if (IsCancelRequested())
		{
			SetState(Threading::TaskState::Canceled);
		}
		else if (result != Result::Ok)
		{
			LOG_WARNING("Graphic", "\'{0}\' failed with result: {1}", ToString(), Types::GetEnumString(result));
			OnFail();
		}
		else
		{
			// Save task completion point (for synchronization)
			m_SyncPoint = context->GetCurrentSyncPoint();
			m_Context = context;
		}
	}

	void GPUTask::Sync()
	{
		if (m_Context != nullptr)
		{
			ENGINE_ASSERT(IsSyncing());

			// Sync and finish
			m_Context = nullptr;
			OnSync();
			OnFinish();
		}
	}

	void GPUTask::CancelSync()
	{
		ENGINE_ASSERT(IsSyncing());

		// Rollback state and cancel
		m_Context = nullptr;
		SetState(Threading::TaskState::Queued);
		Cancel();
	}

	String GPUTask::ToString() const
	{
		return String::Format(SE_TEXT("GPU Async Task {0} ({1})"), Types::GetEnumString(GetType()), (int32)GetState());
	}

	void GPUTask::Enqueue()
	{
		GPUDevice::instance->GetTasksSystem()->_tasks.Add(this);
	}

	bool GPUTask::Run()
	{
		return true;
	}

	void GPUTask::OnCancel()
	{
		// Check if task is waiting for sync (very likely situation)
		if (IsSyncing())
		{
			// Task has been performed but is waiting for a CPU/GPU sync so we have to cancel that
			ENGINE_ASSERT(m_Context != nullptr);
			m_Context->OnCancelSync(this);
			m_Context = nullptr;
			SetState(Threading::TaskState::Canceled);
		}
		else
		{
			// Maybe we could also handle cancel event during running but not yet syncing
			ENGINE_ASSERT(!IsState(Threading::TaskState::Running));
		}

		// Base
		Task::OnCancel();
	}

	GPUTasksSystem::GPUTasksSystem()
	{
		_buffers[0].EnsureCapacity(64);
		_buffers[1].EnsureCapacity(64);
	}

	void GPUTasksSystem::SetExecutor(GPUTasksExecutor* value)
	{
		if (value != _executor && value)
		{
			if (_executor)
			{
				SAFE_DELETE(_executor);
			}

			_executor = value;
		}
	}

	void GPUTasksSystem::Dispose()
	{
		// Cancel all performed tasks (that are waiting for sync)
		SAFE_DELETE(_executor);

		// Cleanup
		Threading::Task::CancelAll(_buffers[0]);
		Threading::Task::CancelAll(_buffers[1]);
		_bufferIndex = 0;
		_tasks.CancelAll();
	}

	void GPUTasksSystem::FrameBegin()
	{
		_executor->FrameBegin();
	}

	void GPUTasksSystem::FrameEnd()
	{
		_executor->FrameEnd();
	}

	int32 GPUTasksSystem::RequestWork(GPUTask** buffer, int32 maxCount)
	{
		const auto b1Index = _bufferIndex;
		const auto b2Index = (_bufferIndex + 1) % 2;
		auto& b1 = _buffers[b1Index];
		auto& b2 = _buffers[b2Index];

		// Take maximum amount of tasks to the buffer at once
		ASSERT(b1.IsEmpty());
		const int32 takenTasksCount = (int32)_tasks.try_dequeue_bulk(b1.Get(), maxCount);
		b2.Add(b1.Get(), takenTasksCount);

		int32 count = 0;

		// Filter taken tasks to keep maxTotalComplexity limit
		b1.Clear();
		int32 i = 0;
		for (; i < b2.Count() && count < maxCount; i++)
		{
			auto task = b2[i];
			const auto state = task->GetState();
			switch (state)
			{
			case Threading::TaskState::Failed:
			case Threading::TaskState::Canceled:
			case Threading::TaskState::Finished:
				// Skip task
				break;
			case Threading::TaskState::Queued:
				// Run queued task
				buffer[count++] = task;
				break;
			case Threading::TaskState::Created:
			case Threading::TaskState::Running:
			default:
				// Keep task for the next RequestWork
				b1.Add(task);
				break;
			}
		}
		const int32 itemsLeft = b2.Count() - i;
		if (itemsLeft > 0)
			b1.Add(&b2[i], itemsLeft);
		b2.Clear();

		// Swap buffers
		_bufferIndex = b2Index;

		return count;
	}
}
