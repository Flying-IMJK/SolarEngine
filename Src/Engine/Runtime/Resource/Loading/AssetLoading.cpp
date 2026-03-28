
#include "AssetLoading.h"
#include "AssetTask.h"
#include "Core/Logging/Logging.h"
#include "Core/Math/Math.h"
#include "Core/Types/Collections/List.h"
#include "Core/Platform/CPUInfo.h"
#include "Core/Platform/Thread.h"
#include "Core/Platform/ConditionVariable.h"
#include "Core/Systems.h"
#include "Core/Thread/Threading.h"
#if SE_EDITOR && PLATFORM_WINDOWS
#include "Core/Platform/Win32/IncludeWindowsHeaders.h"
#include <propidlbase.h>
#endif

#include "Runtime/Resource/AssetConfig.h"
#include "Core/Thread/ThreadLocal.h"
#include "Core/TypeSystem/Types.h"

namespace SE
{
	THREADLOCAL AssetLoadingThread* thisThread = nullptr;
	struct AssetLoadingSystemData
	{
		AssetLoadingThread* MainThread = nullptr;
		List<AssetLoadingThread*> Threads;
		Threading::ConcurrentTaskQueue<AssetTask> Tasks;
		ConditionVariable TasksSignal;
		CriticalSection TasksMutex;
	} * systemData;

	class AssetLoadingSystem final : public ISystem
	{
		ENGINE_SYSTEM(AssetLoadingSystem)
	public:
		AssetLoadingSystem() : ISystem(SE_TEXT("Asset Loading System"), -500)
		{
			systemData = New<AssetLoadingSystemData>();
		}

		~AssetLoadingSystem()
		{
			Delete<AssetLoadingSystemData>(systemData);
		}

	protected:
		bool OnInit() override
		{
			ENGINE_ASSERT(systemData->Threads.IsEmpty() && Threading::IsMainThread());

			// Calculate amount of loading threads to use
			const CPUInfo cpuInfo = Platform::GetCPUInfo();
			const int32 count = Math::Clamp(Math::CeilToInt((LOADING_THREAD_PER_LOGICAL_CORE * (float)cpuInfo.LogicalProcessorCount)), 1, 12);
			LOG_INFO("Resource", "Creating {0} content loading threads...", count);

			// Create loading threads
			systemData->MainThread = New<AssetLoadingThread>();
			thisThread = systemData->MainThread;
			systemData->Threads.EnsureCapacity(count);
			for (int32 i = 0; i < count; i++)
			{
				auto thread = New<AssetLoadingThread>();
				if (!thread->Start(String::Format(TEXT("Load Thread {0}"), i)))
				{
					LOG_FATAL("Resource", "Cannot spawn Asset thread {0}/{1}", i, count);
					Delete(thread);
					return false;
				}
				systemData->Threads.Add(thread);
			}

			return true;
		}

		void OnBeforeExit() override
		{
			// Signal threads to end work soon
			for (int32 i = 0; i < systemData->Threads.Count(); i++)
				systemData->Threads[i]->NotifyExit();

			systemData->TasksSignal.NotifyAll();
		}

		void OnDispose() override
		{
			// Exit all threads
			for (int32 i = 0; i < systemData->Threads.Count(); i++)
			{
				systemData->Threads[i]->NotifyExit();
			}
			systemData->TasksSignal.NotifyAll();
			for (int32 i = 0; i < systemData->Threads.Count(); i++)
			{
				systemData->Threads[i]->Join();
			}
			systemData->Threads.ClearDelete();
			Delete(systemData->MainThread);
			systemData->MainThread = nullptr;
			thisThread = nullptr;

			// Cancel all remaining tasks (no chance to execute them)
			systemData->Tasks.CancelAll();
		}
	};

	ENGINE_SYSTEM_REGISTER(AssetLoadingSystem);

	AssetLoadingThread* AssetLoading::GetCurrentLoadThread()
	{
		return thisThread;
	}

	int32 AssetLoading::GetTasksCount()
	{
		return systemData->Tasks.Count();
	}

	Threading::ConcurrentTaskQueue<AssetTask>& AssetLoading::GetAssetLoadTaskQueue()
	{
		return systemData->Tasks;
	}


	AssetLoadingThread::AssetLoadingThread()
		: m_ExitFlag(false)
		, m_Thread(nullptr)
		, m_TotalTasksDoneCount(0)
	{
	}

	AssetLoadingThread::~AssetLoadingThread()
	{
		// Check if has thread attached
		if (m_Thread != nullptr)
		{
			m_Thread->Kill(true);
			Delete(m_Thread);
		}
	}

	uint64 AssetLoadingThread::GetID() const
	{
		return m_Thread ? m_Thread->GetID() : 0;
	}

	void AssetLoadingThread::NotifyExit()
	{
		Platform::AtomicIncrement(&m_ExitFlag);
	}

	void AssetLoadingThread::Join()
	{
		auto thread = m_Thread;
		if (thread)
		{
			thread->Join();
		}
	}

	bool AssetLoadingThread::Start(const String& name)
	{
		ENGINE_ASSERT(m_Thread == nullptr && name.HasChars());

		// Create new thread
		auto thread = Thread::Create(this, name, ThreadPriority::Normal);
		if (thread == nullptr)
			return false;

		m_Thread = thread;

		return true;
	}

	String AssetLoadingThread::ToString() const
	{
		return String::Format(SE_TEXT("Loading Thread {0}"), GetID());
	}

	int32 AssetLoadingThread::Run()
	{
#if SE_EDITOR && PLATFORM_WINDOWS
		// Initialize COM
		// TODO: maybe add sth to Thread::Create to indicate that thread will use COM stuff
		const auto result = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
		if (FAILED(result))
		{
			LOG_ERROR("Resource", "Failed to init COM for WIC texture importing! Result: {0:x}", static_cast<uint32>(result));
			return -1;
		}

#endif

		AssetTask* task;
		thisThread = this;

		while (HasExitFlagClear())
		{
			if (systemData->Tasks.try_dequeue(task))
			{
				Execute(task);
			}
			else
			{
				systemData->TasksMutex.Lock();
				systemData->TasksSignal.Wait(systemData->TasksMutex);
				systemData->TasksMutex.Unlock();
			}
		}

		thisThread = nullptr;
		return 0;
	}

	void AssetLoadingThread::Exit()
	{
		// Send info
		ASSERT_LOW_LAYER(m_Thread);
		LOG_INFO("Resource", "Content thread '{0}' exited. Load calls: {1}", m_Thread->GetName(), m_TotalTasksDoneCount);
	}

	void AssetLoadingThread::Execute(AssetTask* task)
	{
		ENGINE_ASSERT(task);

		task->Execute();
		m_TotalTasksDoneCount++;
	}

	String AssetTask::ToString() const
	{
		return String::Format(SE_TEXT("Content Load Task ({0})"), Types::GetEnumString(GetState()));
	}

	void AssetTask::Enqueue()
	{
		systemData->Tasks.Add(this);
		systemData->TasksSignal.NotifyOne();
	}

	bool AssetTask::Run()
	{
		// Perform an operation
		const auto result = Process();

		// Process result
		const bool failed = result != Result::Ok;
		if (failed)
		{
			LOG_WARNING("Resource", "\'{0}\' failed with result: {1}", ToString(), Types::GetEnumString(result));
		}
		return failed;
	}

}
