#pragma once

#include "Core/TypeSystem/IType.h"
#include "Core/Types/Delegate.h"

namespace SE::Threading
{
	enum class TaskState
	{
		Created,
		Failed,
		Canceled,
		Queued,
		Running,
		Finished
	};

	StringView GetTaskStateName(TaskState state);

	/// <summary>
	/// Represents an asynchronous operation.
	/// </summary>
	class SE_API_CORE Task
	{
		//
		// Tasks execution and states flow:
		//
		//  Task() [Created]
		//   \/
		// Start() [Queued]
		//   \/
		//  Run()  [Running]
		//    |
		//    ------------------------
		//    \/                     \/
		//  Finish() [Finished]  Fail/Cancel() [Failed/Canceled]
		//    \/                     \/
		//  child.Start()       child.Cancel()
		//    |                      \/
		//    -------------------------
		//    \/
		//   End()
		//
		NON_COPYABLE(Task)
	protected:
		/// <summary>
		/// The cancel flag used to indicate that there is request to cancel task operation.
		/// </summary>
		volatile int64 m_CancelFlag = 0;

		/// <summary>
		/// The current task state.
		/// </summary>
		volatile TaskState m_State = TaskState::Created;

		/// <summary>
		/// The task to start after finish.
		/// </summary>
		Task* m_ContinueWith = nullptr;

		Char* m_Name;

		void SetState(TaskState state)
		{
			Platform::AtomicStore((int64 volatile*)&m_State, (uint64)state);
		}

	public:
		Task() = default;
		virtual ~Task() = default;

		/// <summary>
		/// Gets the task state.
		/// </summary>
		TaskState GetState() const;

		/// <summary>
		/// Determines whether the specified object has reference to the given object.
		/// </summary>
		/// <param name="obj">The object.</param>
		/// <returns>True if the specified object has reference to the given object, otherwise false.</returns>
		virtual bool HasReference(void* obj) const
		{
			return false;
		}

		/// <summary>
		/// Gets the task to start after this one.
		/// </summary>
		Task* GetContinueWithTask() const;

		bool IsState(TaskState state) const;

		/// <summary>
		/// Checks if operation has been ended (via cancel, fail or finish).
		/// </summary>
		bool IsEnded() const;

		/// <summary>
		/// Returns true if task has been requested to cancel it's operation.
		/// </summary>
		bool IsCancelRequested();

	public:
		/// <summary>
		/// Starts this task execution (and will continue with all children).
		/// </summary>
		void Start();

		/// <summary>
		/// Cancels this task (and all child tasks).
		/// </summary>
		void Cancel();

		/// <summary>
		/// Waits the specified timeout for the task to be finished.
		/// </summary>
		/// <param name="timeout">The maximum amount of time to wait for the task to finish it's job. Timeout smaller/equal 0 will result in infinite waiting.</param>
		/// <returns>True if task failed or has been canceled or has timeout, otherwise false.</returns>
		bool Wait(const TimeSpan& timeout) const;

		/// <summary>
		/// Waits the specified timeout for the task to be finished.
		/// </summary>
		/// <param name="timeoutMilliseconds">The maximum amount of milliseconds to wait for the task to finish it's job. Timeout smaller/equal 0 will result in infinite waiting.</param>
		/// <returns>True if task failed or has been canceled or has timeout, otherwise false.</returns>
		bool Wait(double timeoutMilliseconds = -1) const;

		/// <summary>
		/// Waits for all the tasks from the list.
		/// </summary>
		/// <param name="tasks">The tasks list to wait for.</param>
		/// <param name="timeoutMilliseconds">The maximum amount of milliseconds to wait for the task to finish it's job. Timeout smaller/equal 0 will result in infinite waiting.</param>
		/// <returns>True if any task failed or has been canceled or has timeout, otherwise false.</returns>
		template<class T = Task, typename AllocationType = HeapAllocation>
		static bool WaitAll(List<T*, AllocationType>& tasks, double timeoutMilliseconds = -1)
		{
			for (int32 i = 0; i < tasks.Count(); i++)
			{
				if (!tasks[i]->Wait())
					return false;
			}
			return true;
		}

	public:
		/// <summary>
		/// Continues that task execution with a given task (will call Start on given task after finishing that one).
		/// </summary>
		/// <param name="task">The task to Start after current finish (will propagate OnCancel event if need to).</param>
		/// <returns>Enqueued task.</returns>
		Task* ContinueWith(Task* task);

		/// <summary>
		/// Continues that task execution with a given action (will spawn new async action).
		/// </summary>
		/// <param name="action">Action to run.</param>
		/// <param name="target">The action target object.</param>
		/// <returns>Enqueued task.</returns>
		Task* ContinueWith(const Action& action, void* target = nullptr);

		/// <summary>
		/// Continues that task execution with a given action (will spawn new async action).
		/// </summary>
		/// <param name="action">Action to run.</param>
		/// <param name="target">The action target object.</param>
		/// <returns>Enqueued task.</returns>
		Task* ContinueWith(const Function<void()>& action, void* target = nullptr);

		/// <summary>
		/// Continues that task execution with a given action (will spawn new async action).
		/// </summary>
		/// <param name="action">Action to run.</param>
		/// <param name="target">The action target object.</param>
		/// <returns>Enqueued task.</returns>
		Task* ContinueWith(const Function<bool()>& action, void* target = nullptr);

		virtual String ToString() const;

	public:
		/// <summary>
		/// Starts the new task.
		/// </summary>
		/// <param name="task">The task.</param>
		/// <returns>Task</returns>
		static Task* StartNew(Task* task);

		/// <summary>
		/// Starts the new task.
		/// </summary>
		/// <param name="action">The action.</param>
		/// <param name="target">The action target object.</param>
		/// <returns>Task</returns>
		static Task* StartNew(const Function<void()>& action, void* target = nullptr);

		/// <summary>
		/// Starts the new task.
		/// </summary>
		/// <param name="action">The action.</param>
		/// <param name="target">The action target object.</param>
		/// <returns>Task</returns>
		static Task* StartNew(Function<void()>::Signature action, void* target = nullptr);

		/// <summary>
		/// Starts the new task.
		/// </summary>
		/// <param name="callee">The callee object.</param>
		/// <returns>Task</returns>
		template<class T, void(T::* Method)()>
		static Task* StartNew(T* callee)
		{
			Function<void()> action;
			action.Bind<T, Method>(callee);
			return StartNew(action, dynamic_cast<void*>(callee));
		}

		/// <summary>
		/// Starts the new task.
		/// </summary>
		/// <param name="action">The action.</param>
		/// <param name="target">The action target object.</param>
		/// <returns>Task</returns>
		static Task* StartNew(const Function<bool()>& action, void* target = nullptr);

		/// <summary>
		/// Starts the new task.
		/// </summary>
		/// <param name="action">The action.</param>
		/// <param name="target">The action target object.</param>
		/// <returns>Task</returns>
		static Task* StartNew(Function<bool()>::Signature& action, void* target = nullptr);

		/// <summary>
		/// Starts the new task.
		/// </summary>
		/// <param name="callee">The callee object.</param>
		/// <returns>Task</returns>
		template<class T, bool(T::*Method)()>
		static Task* StartNew(T* callee)
		{
			Function<bool()> action;
			action.Bind<T, Method>(callee);
			return StartNew(action, dynamic_cast<void*>(callee));
		}

		/// <summary>
		/// Cancels all the tasks from the list and clears it.
		/// </summary>
		template<class T = Task, typename AllocationType = HeapAllocation>
		static void CancelAll(List<T*, AllocationType>& tasks)
		{
			for (int32 i = 0; i < tasks.Count(); i++)
				tasks[i]->Cancel();
			tasks.Clear();
		}

	protected:
		/// <summary>
		/// Executes this task. It should be called by the task consumer (thread pool or other executor of this task type). It calls run() and handles result).
		/// </summary>
		void Execute();

		/// <summary>
		/// Runs the task specified operations. It does not handle any task related logic, but only performs the actual job.
		/// </summary>
		/// <returns>The task execution result. Returns true if failed, otherwise false.</returns>
		virtual bool Run() = 0;

	protected:
		virtual void Enqueue() = 0;
		virtual void OnStart();
		virtual void OnFinish();
		virtual void OnFail();
		virtual void OnCancel();
		virtual void OnEnd();
	};

} // SE

