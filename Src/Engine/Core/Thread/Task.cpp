#include "Task.h"

#include "ThreadPool.h"
#include "Core/Logging/Logging.h"
#include "Core/Platform/Platform.h"
#include "Core/Types/DateTime.h"
#include "Core/Types/Collections/List.h"
#include "Core/Math/Math.h"
#include "Core/Profiler/ProfilerCPU.h"

namespace SE::Threading
{
	StringView GetTaskStateName(TaskState state)
	{
		switch (state)
		{
		case TaskState::Created:
			return SE_TEXT("Create");
			break;
		case TaskState::Failed:
			return SE_TEXT("Failed");
			break;
		case TaskState::Canceled:
			return SE_TEXT("Canceled");
			break;
		case TaskState::Queued:
			return SE_TEXT("Queued");
			break;
		case TaskState::Running:
			return SE_TEXT("Running");
			break;
		case TaskState::Finished:
			return SE_TEXT("Finished");
			break;
		}

		return String::Empty;
	}

	TaskState Task::GetState() const
	{
		return (TaskState)Platform::AtomicRead((int64 const volatile*)&m_State);
	}

	Task* Task::GetContinueWithTask() const
	{
		return m_ContinueWith;
	}

	bool Task::IsState(TaskState state) const
	{
		return GetState() == state;
	}

	bool Task::IsEnded() const
	{
		auto state = GetState();
		return state == TaskState::Failed || state == TaskState::Canceled || state == TaskState::Finished;
	}

	bool Task::IsCancelRequested()
	{
		return Platform::AtomicRead(&m_CancelFlag) != 0;
	}

	void Task::Start()
	{
		if (GetState() != TaskState::Created)
			return;

		OnStart();

		// Change state
		SetState(TaskState::Queued);

		// Add task to the execution queue
		Enqueue();
	}

	void Task::Cancel()
	{
		if (Platform::AtomicRead(&m_CancelFlag) == 0)
		{
			// Send event
			OnCancel();

			// Cancel child
			if (m_ContinueWith)
				m_ContinueWith->Cancel();
		}
	}

	bool Task::Wait(const TimeSpan& timeout) const
	{
		return Wait(timeout.GetTotalMilliseconds());
	}

	bool Task::Wait(double timeoutMilliseconds) const
	{
		PROFILE_CPU();
		double startTime = Platform::GetTimeSeconds() * 0.001;

		// TODO: no active waiting! use a semaphore!

		do
		{
			auto state = GetState();

			// Finished
			if (state == TaskState::Finished)
			{
				// Wait for child if has
				if (m_ContinueWith)
				{
					auto spendTime = Platform::GetTimeSeconds() * 0.001 - startTime;
					return m_ContinueWith->Wait(timeoutMilliseconds - spendTime);
				}

				return true;
			}

			// Failed or canceled
			if (state == TaskState::Failed || state == TaskState::Canceled)
				return false;

			Platform::Sleep(1);
		} while (timeoutMilliseconds <= 0.0 || Platform::GetTimeSeconds() * 0.001 - startTime < timeoutMilliseconds);

		// Timeout reached!
		LOG_WARNING("Threading", "\'{0}\' has timed out. Wait time: {1} ms", m_Name, timeoutMilliseconds);
		return false;
	}

	Task* Task::ContinueWith(Task* task)
	{
		ENGINE_ASSERT(task != nullptr && task != this);
		if (m_ContinueWith)
			return m_ContinueWith->ContinueWith(task);
		m_ContinueWith = task;
		return task;
	}

	Task* Task::ContinueWith(const Action& action, void* target)
	{
		// Get binded functions
		List <Action::FunctionType> bindings;
		bindings.Resize(action.Count());
		action.GetBindings(bindings.Get(), bindings.Count());

		// Continue with every action
		Task* result = this;
		for (int32 i = 0; i < bindings.Count(); i++)
			result = result->ContinueWith(bindings[i], target);
		return result;
	}

	Task* Task::ContinueWith(const Function<void()>& action, void* target)
	{
		ENGINE_ASSERT(action.IsBinded());
		return ContinueWith(New<ThreadPoolActionTask>(action, target));
	}

	Task* Task::ContinueWith(const Function<bool()>& action, void* target)
	{
		ENGINE_ASSERT(action.IsBinded());
		return ContinueWith(New<ThreadPoolActionTask>(action, target));
	}

	Task* Task::StartNew(Task* task)
	{
		ENGINE_ASSERT(task);
		task->Start();
		return task;
	}

	Task* Task::StartNew(const Function<void()>& action, void* target)
	{
		return StartNew(New<ThreadPoolActionTask>(action, target));
	}

	Task* Task::StartNew(const Function<void()>::Signature action, void* target)
	{
		return StartNew(New<ThreadPoolActionTask>(action, target));
	}

	Task* Task::StartNew(const Function<bool()>& action, void* target)
	{
		return StartNew(New<ThreadPoolActionTask>(action, target));
	}

	Task* Task::StartNew(Function<bool()>::Signature& action, void* target)
	{
		return StartNew(New<ThreadPoolActionTask>(action, target));
	}

	void Task::Execute()
	{
		if (IsState(TaskState::Canceled))
			return;
		ENGINE_ASSERT(IsState(TaskState::Queued));
		SetState(TaskState::Running);

		// Perform an operation
		bool failed = Run();

		// Process result
		if (IsCancelRequested())
		{
			SetState(TaskState::Canceled);
		}
		else if (failed)
		{
			OnFail();
		}
		else
		{
			OnFinish();
		}
	}

	void Task::OnStart()
	{
	}

	void Task::OnFinish()
	{
		ENGINE_ASSERT(IsState(TaskState::Running) && !IsCancelRequested());
		SetState(TaskState::Finished);

		// Send event further
		if (m_ContinueWith)
			m_ContinueWith->Start();

		OnEnd();
	}

	void Task::OnFail()
	{
		SetState(TaskState::Failed);

		// Send event further
		if (m_ContinueWith)
			m_ContinueWith->OnFail();

		OnEnd();
	}

	void Task::OnCancel()
	{
		// Set flag
		Platform::AtomicIncrement(&m_CancelFlag);
		Platform::MemoryBarrier();

		// If task is active try to wait for a while
		if (IsState(TaskState::Running))
		{
			// Wait for it a little bit
			const double timeout = 2000.0;
			LOG_WARNING("Threading", "Cannot cancel \'{0}\' because it's still running, waiting for end with timeout: {1} ms", m_Name, timeout);
			Wait(timeout);
		}

		// Don't call OnEnd twice
		const auto state = GetState();
		if (state != TaskState::Finished && state != TaskState::Failed)
		{
			SetState(TaskState::Canceled);
			OnEnd();
		}
	}

	void Task::OnEnd()
	{
		ENGINE_ASSERT(!IsState(TaskState::Running));

		// Add to delete
		// DeleteObject(30.0f, false);
	}

	String Task::ToString() const
	{
		static String taskName{ SE_TEXT("" });
		return taskName;
	}
}