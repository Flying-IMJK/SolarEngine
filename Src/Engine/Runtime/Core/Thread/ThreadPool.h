#pragma once

#include "Runtime/Core/Types/Variable.h"
#include "Runtime/Core/Systems.h"
#include "Task.h"

namespace SE::Threading
{
	/// <summary>
	/// Main engine thread pool for threaded tasks system.
	/// </summary>
	class SE_API_RUNTIME ThreadPool
	{
		friend class ThreadPoolTask;
		friend SE_API_RUNTIME bool Init();
	private:

		static int32 ThreadProc();
	};

	/// <summary>
	/// 使用 Thread Pool 执行的通用Task
	/// </summary>
	class SE_API_RUNTIME ThreadPoolTask : public Task
	{
		friend ThreadPool;

	protected:

		/// <summary>
		/// Initializes a new instance of the <see cref="ThreadPoolTask"/> class.
		/// </summary>
		ThreadPoolTask() : Task()
		{
		}

	public:

		// [Task]
		String ToString() const override;

	protected:

		// [Task]
		void Enqueue() override;
	};

	/// <summary>
	/// 使用 Thread Pool 执行自定义操作的通用Task。
	/// </summary>
	class SE_API_RUNTIME ThreadPoolActionTask : public ThreadPoolTask
	{
	protected:

		Function<void()> _action1;
		Function<bool()> _action2;
		void* _target;

	public:

		/// <summary>
		/// Initializes a new instance of the <see cref="ThreadPoolActionTask"/> class.
		/// </summary>
		/// <param name="action">The action.</param>
		/// <param name="target">The target object.</param>
		ThreadPoolActionTask(const Function<void()>& action, void* target = nullptr)
			: ThreadPoolTask(), _action1(action), _target(target)
		{
		}

		/// <summary>
		/// Initializes a new instance of the <see cref="ThreadPoolActionTask"/> class.
		/// </summary>
		/// <param name="action">The action.</param>
		/// <param name="target">The target object.</param>
		ThreadPoolActionTask(Function<void()>::Signature action, void* target = nullptr)
			: ThreadPoolTask(), _action1(action), _target(target)
		{
		}

		/// <summary>
		/// Initializes a new instance of the <see cref="ThreadPoolActionTask"/> class.
		/// </summary>
		/// <param name="action">The action.</param>
		/// <param name="target">The target object.</param>
		ThreadPoolActionTask(const Function<bool()>& action, void* target = nullptr)
			: ThreadPoolTask(), _action2(action), _target(target)
		{
		}

		/// <summary>
		/// Initializes a new instance of the <see cref="ThreadPoolActionTask"/> class.
		/// </summary>
		/// <param name="action">The action.</param>
		/// <param name="target">The target object.</param>
		ThreadPoolActionTask(Function<bool()>::Signature action, void* target = nullptr)
			: ThreadPoolTask(), _action2(action), _target(target)
		{
		}

	public:

		// [ThreadPoolTask]
		bool HasReference(void* obj) const override
		{
			return obj == _target;
		}

	protected:

		// [ThreadPoolTask]
		bool Run() override
		{
			if (_action1.IsBinded())
			{
				_action1();
				return false;
			}

			if (_action2.IsBinded())
			{
				return _action2();
			}

			return false;
		}
	};
}
