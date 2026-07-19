#pragma once

//#include "Engine/Core/Object.h"
#include "Runtime/API.h"
#include "Runtime/Core/Types/Strings/String.h"
#include "Runtime/Core/Types/Delegate.h"
#include "Runtime/Core/Platform/Base/PlatformBase.h"

namespace SE
{
	namespace Threading
	{
		class IRunnable;
	}

	/// <summary>
	/// Base class for thread objects.
	/// </summary>
	/// <remarks>Ensure to call Kill or Join before deleting thread object.</remarks>
	class SE_API_RUNTIME ThreadBase
	{
		NON_COPYABLE(ThreadBase)

		/// <summary>
		/// The custom event called before thread execution just after startup. Can be used to setup per-thread state or data. Argument is: thread handle.
		/// </summary>
		static Delegate<Thread*> ThreadStarting;

		/// <summary>
		/// The custom event called after thread execution just before exit. Can be used to cleanup per-thread state or data. Arguments are: thread handle and exit code.
		/// </summary>
		static Delegate<Thread*, int32> ThreadExiting;

	protected:

		Threading::IRunnable* m_Runnable;
#if BUILD_DEBUG
		String _runnableName;
#endif
		ThreadPriority m_Priority;
		String m_Name;
		uint64 m_Id;
		bool m_IsRunning;
		bool m_CallAfterWork;

		ThreadBase(Threading::IRunnable* runnable, const String& name, ThreadPriority priority);

	public:

		/// <summary>
		/// Gets priority level of the thread.
		/// </summary>
		inline ThreadPriority GetPriority() const
		{
			return m_Priority;
		}

		/// <summary>
		/// Sets priority level of the thread
		/// </summary>
		/// <param name="priority">The thread priority to change to.</param>
		void SetPriority(ThreadPriority priority);

		/// <summary>
		/// Gets thread ID
		/// </summary>
		inline uint64 GetID() const
		{
			return m_Id;
		}

		/// <summary>
		/// Gets thread running state.
		/// </summary>
		inline bool IsRunning() const
		{
			return m_IsRunning;
		}

		/// <summary>
		/// Gets name of the thread.
		/// </summary>
		inline const String& GetName() const
		{
			return m_Name;
		}

	public:

		/// <summary>
		/// Aborts the thread execution by force.
		/// </summary>
		/// <param name="waitForJoin">True if should wait for thread end, otherwise false.</param>
		void Kill(bool waitForJoin = false);

		/// <summary>
		/// Stops the current thread execution and waits for the thread execution end.
		/// </summary>
		virtual void Join() = 0;

	protected:

		int32 Run();

		virtual void ClearHandleInternal() = 0;
		virtual void SetPriorityInternal(ThreadPriority priority) = 0;
		virtual void KillInternal(bool waitForJoin) = 0;

	public:

		// [Object]
		String ToString() const// override
		{
			return m_Name;
		}
	};

}