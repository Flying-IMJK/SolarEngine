
#pragma once

#include "Core/Thread/IRunnable.h"
#include "Core/Systems.h"
#include "Core/Thread/ConcurrentTaskQueue.h"

namespace SE
{
	class Asset;
	class AssetLoadingThread;
	class AssetTask;

	/// <summary>
	/// 资源加载线程
	/// </summary>
	class AssetLoadingThread : public Threading::IRunnable
	{
	protected:
		volatile int64 m_ExitFlag;
		Thread* m_Thread;
		int32 m_TotalTasksDoneCount;

	public:
		/// <summary>
		/// Init
		/// </summary>
		AssetLoadingThread();

		/// <summary>
		/// Destructor
		/// </summary>
		~AssetLoadingThread();

	public:
		/// <summary>
		/// Gets the thread identifier.
		/// </summary>
		/// <returns>Thread ID</returns>
		uint64 GetID() const;

	public:
		/// <summary>
		/// Returns true if thread has empty exit flag, so it can continue it's work
		/// </summary>
		/// <returns>True if exit flag is empty, otherwise false</returns>
		FORCE_INLINE bool HasExitFlagClear()
		{
			return Platform::AtomicRead(&m_ExitFlag) == 0;
		}

		/// <summary>
		/// Set exit flag to true so thread must exit
		/// </summary>
		void NotifyExit();

		/// <summary>
		/// Stops the calling thread execution until the loading thread ends its execution.
		/// </summary>
		void Join();

	public:
		/// <summary>
		/// 启动线程
		/// </summary>
		/// <param name="name">线程名称</param>
		/// <returns>false if cannot start, otherwise True</returns>
		bool Start(const String& name);

		/// <summary>
		/// 运行指定的任务
		/// </summary>
		/// <param name="task">The task.</param>
		void Execute(AssetTask* task);

	public:
		// [IRunnable]
		String ToString() const override;
		int32 Run() override;
		void Exit() override;
	};

	/// <summary>
	/// 资产加载系统
	/// </summary>
	class AssetLoading
	{
		friend class AssetTask;
		friend class AssetLoadingThread;
		friend class Asset;

	public:
		/// <summary>
		/// Checks if current execution context is thread used to load assets.
		/// </summary>
		/// <returns>True if execution is in Load Thread, otherwise false.</returns>
		FORCE_INLINE static bool IsInLoadThread()
		{
			return GetCurrentLoadThread() != nullptr;
		}

		/// <summary>
		/// Gets content loading thread handle if current thread is one of them.
		/// </summary>
		/// <returns>Current load thread or null if current thread is different.</returns>
		static AssetLoadingThread* GetCurrentLoadThread();

	public:
		/// <summary>
		/// Gets amount of enqueued tasks to perform.
		/// </summary>
		/// <returns>The tasks count.</returns>
		static int32 GetTasksCount();

	private:
		static Threading::ConcurrentTaskQueue<AssetTask>& GetAssetLoadTaskQueue();
	};
}
