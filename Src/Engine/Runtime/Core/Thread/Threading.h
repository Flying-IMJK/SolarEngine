#pragma once
#include "Task.h"
#include "Runtime/Core/Types/Variable.h"
#include "Runtime/Core/Platform/CriticalSection.h"
#include "Runtime/Core/Platform/Thread.h"
//-------------------------------------------------------------------------

namespace SE
{
	// Invokes a target method on a main thread (using task or directly if already on main thread)
	// Example: INVOKE_ON_MAIN_THREAD(Collector, Collector::SyncData, this);
#define INVOKE_ON_MAIN_THREAD(targetType, targetMethod, targetObject) 		\
		if (Threading::IsMainThread()) 										\
		{ 																	\
			targetObject->targetMethod(); 									\
		} else                                                              \
		{ 																	\
			Function<void()> action; 										\
			action.Bind<targetType, &targetMethod>(targetObject); 			\
			Threading::Task::StartNew(New<Threading::MainThreadActionTask>(action))->Wait(); \
		}
}

namespace SE::Threading
{
	/// <summary>
	/// Scope locker for critical section.
	/// </summary>
	class ScopeLock
	{
	private:

		const CriticalSection* m_Section;

		ScopeLock() = default;
		ScopeLock(const ScopeLock&) = delete;
		ScopeLock& operator=(const ScopeLock&) = delete;

	public:

		/// <summary>
		/// Init, enters critical section.
		/// </summary>
		/// <param name="section">The synchronization object to lock.</param>
		ScopeLock(const CriticalSection& section)
			: m_Section(&section)
		{
			m_Section->Lock();
		}

		/// <summary>
		/// Init, enters critical section.
		/// </summary>
		/// <param name="section">The synchronization object to lock.</param>
		ScopeLock(const CriticalSection* section)
			: m_Section(section)
		{
			m_Section->Lock();
		}

		/// <summary>
		/// Destructor, releases critical section.
		/// </summary>
		~ScopeLock()
		{
			m_Section->Unlock();
		}
	};

	SE_API_RUNTIME bool Init();
	SE_API_RUNTIME void Shutdown();

	SE_API_RUNTIME bool IsMainThread();

	SE_API_RUNTIME void SetMainThreadID(int64 threadID);

	SE_API_RUNTIME int64 GetMainThreadID();

	SE_API_RUNTIME int64 GetCurrentThreadID();

	SE_API_RUNTIME void Sleep(int64 milliseconds);

	SE_API_RUNTIME Thread* StartThread(const Function<int32()>& callback, const String& threadName, ThreadPriority priority = ThreadPriority::Normal);

	/// <summary>
	/// General purpose task executed on Main Thread in the beginning of the next frame.
	/// </summary>
	/// <seealso cref="Task" />
	class SE_API_RUNTIME MainThreadTask : public Task
	{
		friend class Engine;
	private:
		static void RunAll(float dt);

	public:
		// [Task]
		String ToString() const override;

		/// <summary>
		/// The initial time delay (in seconds) before task execution. Use 0 to skip this feature.
		/// </summary>
		float InitialDelay = 0.0f;

	protected:

		// [Task]
		void Enqueue() override;
	};

	/// <summary>
	/// General purpose task executing custom action using Main Thread in the beginning of the next frame.
	/// </summary>
	/// <seealso cref="MainThreadTask" />
	/// <seealso cref="Task" />
	class SE_API_RUNTIME MainThreadActionTask : public MainThreadTask
	{
	protected:
		Function<void()> _action1;
		Function<bool()> _action2;
		void* _target;

	public:

		/// <summary>
		/// Initializes a new instance of the <see cref="MainThreadActionTask"/> class.
		/// </summary>
		/// <param name="action">The action.</param>
		/// <param name="target">The target object.</param>
		MainThreadActionTask(const Function<void()>& action, void* target = nullptr)
			: MainThreadTask(), _action1(action), _target(target)
		{
		}

		/// <summary>
		/// Initializes a new instance of the <see cref="MainThreadActionTask"/> class.
		/// </summary>
		/// <param name="action">The action.</param>
		/// <param name="target">The target object.</param>
		MainThreadActionTask(Function<void()>::Signature action, void* target = nullptr)
			: MainThreadTask(), _action1(action), _target(target)
		{
		}

		/// <summary>
		/// Initializes a new instance of the <see cref="MainThreadActionTask"/> class.
		/// </summary>
		/// <param name="action">The action.</param>
		/// <param name="target">The target object.</param>
		MainThreadActionTask(const Function<bool()>& action, void* target = nullptr)
			: MainThreadTask(), _action2(action), _target(target)
		{
		}

		/// <summary>
		/// Initializes a new instance of the <see cref="MainThreadActionTask"/> class.
		/// </summary>
		/// <param name="action">The action.</param>
		/// <param name="target">The target object.</param>
		MainThreadActionTask(Function<bool()>::Signature action, void* target = nullptr)
			: MainThreadTask(), _action2(action), _target(target)
		{
		}

	protected:

		// [MainThreadTask]
		bool Run() override;

	public:

		// [MainThreadTask]
		bool HasReference(void* obj) const override;
	};
}