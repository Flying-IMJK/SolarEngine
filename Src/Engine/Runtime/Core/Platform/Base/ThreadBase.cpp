
#include "Runtime/Core/Platform/Thread.h"
#include "Runtime/Core/Thread/IRunnable.h"
#include "Runtime/Core/Thread/ThreadRegistry.h"
#include "Runtime/Core/Logging/Logging.h"
#include "Runtime/Core/TypeSystem/CoreTypeConversions.h"
#if SE_PROFILER
#include "Runtime/Core/Math/Math.h"
#include "Runtime/Core/Profiler/Profiler.h"
#endif

namespace SE
{
	Delegate<Thread*> ThreadBase::ThreadStarting;
	Delegate<Thread*, int32> ThreadBase::ThreadExiting;

	ThreadBase::ThreadBase(Threading::IRunnable* runnable, const String& name, ThreadPriority priority)
		: m_Runnable(runnable)
		, m_Priority(priority)
		, m_Name(name)
		, m_Id(0)
		, m_IsRunning(false)
		, m_CallAfterWork(true)
	{
		ENGINE_ASSERT(m_Runnable);

#if BUILD_DEBUG
		// Cache name (in case if object gets deleted somewhere)
		_runnableName = _runnable->ToString();
#endif
	}

	void ThreadBase::SetPriority(ThreadPriority priority)
	{
		// Check if value won't change
		if (m_Priority != priority)
		{
			// Set new value
			m_Priority = priority;
			SetPriorityInternal(priority);
		}
	}

	void ThreadBase::Kill(bool waitForJoin)
	{
		if (!m_IsRunning)
		{
			ClearHandleInternal();
			return;
		}
		ENGINE_ASSERT(GetID());
		const auto thread = static_cast<Thread*>(this);

		// Stop runnable object
		if (m_CallAfterWork && m_Runnable)
		{
			m_Runnable->Stop();
		}

		LOG_INFO("Platform", "Thread Killing thread \'{0}\' ID=0x{1:x}", m_Name, m_Id);

		// Kill platform thread
		KillInternal(waitForJoin);
		ClearHandleInternal();

		// End
		if (m_CallAfterWork)
		{
			m_CallAfterWork = false;
			m_Runnable->AfterWork(true);
		}
		m_IsRunning = false;
		Threading::ThreadRegistry::Remove(thread);
	}

	int32 ThreadBase::Run()
	{
		// Setup
		ENGINE_ASSERT(m_Runnable);
		const auto thread = static_cast<Thread*>(this);
		m_Id = Platform::GetCurrentThreadID();
#if SE_PROFILER
		char threadName[100];
        const int32 threadNameLength = Math::Min<int32>(ARRAY_SIZE(threadName) - 1, m_Name.Length());
        StringUtils::ConvertUTF162ANSI(*m_Name, threadName, threadNameLength);
        threadName[threadNameLength] = 0;
        tracy::SetThreadName(threadName);
#endif
		Threading::ThreadRegistry::Add(thread);
		ThreadStarting(thread);
		int32 exitCode = 1;
		m_IsRunning = true;

		LOG_INFO("Platform", "Thread \'{0}\' ID=0x{1:x} started with priority {2}", m_Name, m_Id, (int32)GetPriority());

		if (m_Runnable->Init())
		{
			exitCode = m_Runnable->Run();

			if (m_CallAfterWork) // Prevent from calling this after calling AfterWork since object may be deleted
				m_Runnable->Exit();
		}

		LOG_INFO("Platform", "Thread \'{0}\' ID=0x{1:x} exits with code {2}", m_Name, m_Id, exitCode);

		// End
		if (m_CallAfterWork)
		{
			m_CallAfterWork = false;
			m_Runnable->AfterWork(false);
		}
		m_IsRunning = false;
		ThreadExiting(thread, exitCode);
		Threading::ThreadRegistry::Remove(thread);
//		MCore::Thread::Exit(); // TODO: use mono_thread_detach instead of ext and unlink mono runtime from thread in ThreadExiting delegate
		// mono terminates the native thread..

		return exitCode;
	}

}