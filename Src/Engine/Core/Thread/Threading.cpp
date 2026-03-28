#include "Threading.h"
#include "Core/Types/Collections/Dictionary.h"
#include "Core/Profiler/ProfilerCPU.h"
#include "Core/Platform/Platform.h"
#include "Core/Platform/ConditionVariable.h"
#include "Core/Platform/CPUInfo.h"
#include "Core/Platform/Thread.h"
#include "IRunnable.h"
#include "ThreadPool.h"
#include "ConcurrentTaskQueue.h"
#include "JobSystem.h"


// Jobs storage perf info:
// (500 jobs, i7 9th gen)
// JOB_SYSTEM_USE_MUTEX=1, enqueue=130-280 cycles, dequeue=2-6 cycles
// JOB_SYSTEM_USE_MUTEX=0, enqueue=300-700 cycles, dequeue=10-16 cycles
// So using RingBuffer+Mutex+Signals is better than moodycamel::ConcurrentQueue
#define JOB_SYSTEM_ENABLED 1
#define JOB_SYSTEM_USE_MUTEX 1
#define JOB_SYSTEM_USE_STATS 0

#if JOB_SYSTEM_USE_STATS
#include "Core/Logging/Logging.h"
#endif
#if JOB_SYSTEM_USE_MUTEX
#include "Core/Types/Collections/RingBuffer.h"
#else
#include "ConcurrentQueue.h"
#endif

namespace SE::Threading
{
	static int64 mainThreadID = 0;

	struct ThreadPoolData
	{
		volatile int64 ExitFlag = 0;
		List<Thread*> Threads;
		ConcurrentTaskQueue<ThreadPoolTask> Jobs; // Hello Steve!
		ConditionVariable JobsSignal;
		CriticalSection JobsMutex;

		CriticalSection Locker;
		List<MainThreadTask*> Waiting;
		List<MainThreadTask*> Queue;
	};

	ThreadPoolData* threadData;
	
	bool InitJobSystem();
	void ShutdownJobSystem();
	//-------------------------------------------------------------------------

	bool Init()
	{
		threadData = New<ThreadPoolData>();
		
		// Spawn threads
		const int32 numThreads = Math::Clamp<int32>(Platform::GetCPUInfo().ProcessorCoreCount - 1, 2, PLATFORM_THREADS_LIMIT / 2);
		LOG_INFO("Threading","Spawning {0} Thread Pool workers", numThreads);
		for (int32 i = threadData->Threads.Count(); i < numThreads; i++)
		{
			// Create tread
			auto runnable = New<SimpleRunnable>(true);
			runnable->OnWork.Bind(ThreadPool::ThreadProc);
			auto thread = Thread::Create(runnable, String::Format(SE_TEXT("Thread Pool {0}"), i));
			if (thread == nullptr)
			{
				LOG_ERROR("Threading", "Failed to spawn {0} thread in the Thread Pool", i + 1);
				return false;
			}

			// Add to the list
			threadData->Threads.Add(thread);
		}

		return InitJobSystem();
	}

	void Shutdown()
	{
		ShutdownJobSystem();

		// Set exit flag and wake up threads
		Platform::AtomicStore(&threadData->ExitFlag, 1);
		threadData->JobsSignal.NotifyAll();

		// Wait some time
		Platform::Sleep(10);

		// Delete threads
		for (int32 i = 0; i < threadData->Threads.Count(); i++)
		{
			threadData->Threads[i]->Kill(true);
		}
		threadData->Threads.ClearDelete();
	}

	//-------------------------------------------------------------------------

	bool IsMainThread()
	{
		return mainThreadID == Platform::GetCurrentThreadID();
	}

	void SetMainThreadID(int64 threadID)
	{
		mainThreadID = threadID;
	}

	int64 GetMainThreadID()
	{
		return mainThreadID;
	}

	int64 GetCurrentThreadID()
	{
		return Platform::GetCurrentThreadID();
	}

	void Sleep(int64 milliseconds)
	{
		Platform::Sleep(milliseconds);
	}

	Thread* StartThread(const Function<int32()>& callback, const String& threadName, ThreadPriority priority)
	{
		auto runnable = New<SimpleRunnable>(true);
		runnable->OnWork = callback;
		return Thread::Create(runnable, threadName, priority);
	}

	//ThreadPool
	//-------------------------------------------------------------------------

	String ThreadPoolTask::ToString() const
	{
		return String::Format(SE_TEXT("Thread Pool Task ({0})"), (int32)GetState());
	}

	void ThreadPoolTask::Enqueue()
	{
		threadData->Jobs.Add(this);
		threadData->JobsSignal.NotifyOne();
	}

	int32 ThreadPool::ThreadProc()
	{
		ThreadPoolTask* task;

		// Work until end
		while (Platform::AtomicRead(&threadData->ExitFlag) == 0)
		{
			// Try to get a job
			if (threadData->Jobs.try_dequeue(task))
			{
				task->Execute();
			}
			else
			{
				threadData->JobsMutex.Lock();
				threadData->JobsSignal.Wait(threadData->JobsMutex);
				threadData->JobsMutex.Unlock();
			}
		}

		return 0;
	}

	//JobSystem
	//-------------------------------------------------------------------------
#if JOB_SYSTEM_ENABLED

	struct JobData
	{
		Function<void(int32)> JobFunc;
		int32 Index;
		int64 JobKey;
	};


	class JobSystemThread : public IRunnable
	{
	public:
		uint64 Index;

	public:
		// [IRunnable]
		String ToString() const override
		{
			return SE_TEXT("JobSystemThread");
		}

		int32 Run() override;

		void AfterWork(bool wasKilled) override
		{
			Delete(this);
		}
	};

	struct JobContext
	{
		volatile int64 JobsLeft;
	};

	struct JobSystemData
	{
		Thread* Threads[PLATFORM_THREADS_LIMIT / 2] = {};
		int32 ThreadsCount = 0;
		bool JobStartingOnDispatch = true;
		volatile int64 ExitFlag = 0;
		volatile int64 JobLabel = 0;
		Dictionary<int64, JobContext> JobContexts;
		ConditionVariable JobsSignal;
		CriticalSection JobsMutex;
		ConditionVariable WaitSignal;
		CriticalSection WaitMutex;
		CriticalSection JobsLocker;
#if JOB_SYSTEM_USE_MUTEX
		RingBuffer<JobData> Jobs;
#else
		ConcurrentQueue<JobData> Jobs;
#endif
#if JOB_SYSTEM_USE_STATS
		int64 DequeueCount = 0;
    int64 DequeueSum = 0;
#endif
	};

	JobSystemData* jobSystemData;

	int32 JobSystemThread::Run()
	{
		Platform::SetThreadAffinityMask(1ull << Index);

		JobData data;
		bool attachCSharpThread = true;
#if !JOB_SYSTEM_USE_MUTEX
		moodycamel::ConsumerToken consumerToken(Jobs);
#endif
		while (Platform::AtomicRead(&jobSystemData->ExitFlag) == 0)
		{
			// Try to get a job
#if JOB_SYSTEM_USE_STATS
			const auto start = Platform::GetTimeCycles();
#endif
#if JOB_SYSTEM_USE_MUTEX
			jobSystemData->JobsLocker.Lock();
			if (jobSystemData->Jobs.Count() != 0)
			{
				data = jobSystemData->Jobs.PeekFront();
				jobSystemData->Jobs.PopFront();
			}
			jobSystemData->JobsLocker.Unlock();
#else
			if (!jobSystemData->Jobs.try_dequeue(consumerToken, data))
            	data.Job.Unbind();
#endif
#if JOB_SYSTEM_USE_STATS
			Platform::InterlockedIncrement(&DequeueCount);
        	Platform::InterlockedAdd(&DequeueSum, Platform::GetTimeCycles() - start);
#endif

			if (data.JobFunc.IsBinded())
			{
#if USE_CSHARP
				// Ensure to have C# thread attached to this thead (late init due to MCore being initialized after Job System)
				if (attachCSharpThread)
				{
					MCore::Thread::Attach();
					attachCSharpThread = false;
				}
#endif
				// Run job
				data.JobFunc(data.Index);

				// Move forward with the job queue
				jobSystemData->JobsLocker.Lock();
				JobContext& context = jobSystemData->JobContexts.At(data.JobKey);
				if (Platform::AtomicDecrement(&context.JobsLeft) <= 0)
				{
					ASSERT_LOW_LAYER(context.JobsLeft <= 0);
					jobSystemData->JobContexts.Remove(data.JobKey);
				}
				jobSystemData->JobsLocker.Unlock();

				jobSystemData->WaitSignal.NotifyAll();

				data.JobFunc.Unbind();
			}
			else
			{
				// Wait for signal
				jobSystemData->JobsMutex.Lock();
				jobSystemData->JobsSignal.Wait(jobSystemData->JobsMutex);
				jobSystemData->JobsMutex.Unlock();
			}
		}
		return 0;
	}

#endif

	bool InitJobSystem()
	{
		jobSystemData = New<JobSystemData>();

#if JOB_SYSTEM_ENABLED
		jobSystemData->ThreadsCount = Math::Min<int32>(Platform::GetCPUInfo().LogicalProcessorCount, ARRAY_SIZE(jobSystemData->Threads));
		for (int32 i = 0; i < jobSystemData->ThreadsCount; i++)
		{
			auto runnable = New<JobSystemThread>();
			runnable->Index = (uint64)i;
			auto thread = Thread::Create(runnable, String::Format(SE_TEXT("Job System {0}"), i), ThreadPriority::AboveNormal);
			if (thread == nullptr)
				return false;
			jobSystemData->Threads[i] = thread;
		}
#else
		return true;
#endif
		return true;
	}

	void ShutdownJobSystem()
	{
#if JOB_SYSTEM_ENABLED
		Platform::AtomicStore(&jobSystemData->ExitFlag, 1);
		jobSystemData->JobsSignal.NotifyAll();

		Platform::Sleep(1);

		for (int32 i = 0; i < jobSystemData->ThreadsCount; i++)
		{
			if (jobSystemData->Threads[i])
			{
				jobSystemData->Threads[i]->Kill(true);
				Delete(jobSystemData->Threads[i]);
				jobSystemData->Threads[i] = nullptr;
			}
		}
#endif
	}

	void JobSystem::Execute(const Function<void(int32)> &job, int32 jobCount)
	{
#if JOB_SYSTEM_ENABLED
		// TODO: disable async if called on job thread? or maybe Wait should handle waiting in job thread to do the processing?
		if (jobCount > 1)
		{
			// Async
			const JobHandle jobWaitHandle = Dispatch(job, jobCount);
			Wait(jobWaitHandle);
		}
		else
#endif
		{
			// Sync
			for (int32 i = 0; i < jobCount; i++)
				job(i);
		}
	}

	JobHandle JobSystem::Dispatch(const Function<void(int32)> &job, int32 jobCount)
	{
		PROFILE_CPU();
		if (jobCount <= 0)
			return 0;
#if JOB_SYSTEM_ENABLED
#if JOB_SYSTEM_USE_STATS
		const auto start = Platform::GetTimeCycles();
#endif
		const auto label = Platform::AtomicAdd(&jobSystemData->JobLabel, (int64)jobCount) + jobCount;

		JobData data;
		data.JobFunc = job;
		data.JobKey = label;

		JobContext context;
		context.JobsLeft = jobCount;

#if JOB_SYSTEM_USE_MUTEX
		jobSystemData->JobsLocker.Lock();
		jobSystemData->JobContexts.Add(label, context);
		for (data.Index = 0; data.Index < jobCount; data.Index++)
			jobSystemData->Jobs.PushBack(data);
		jobSystemData->JobsLocker.Unlock();

		RingBuffer<JobData>* t = &jobSystemData->Jobs;
#else
		jobSystemData->JobsLocker.Lock();
		jobSystemData->JobContexts.Add(label, context);
		jobSystemData->JobsLocker.Unlock();
		for (data.Index = 0; data.Index < jobCount; data.Index++)
			jobSystemData->Jobs.enqueue(data);
#endif

#if JOB_SYSTEM_USE_STATS
		LOG(Info, "Job enqueue time: {0} cycles", (int64)(Platform::GetTimeCycles() - start));
#endif

		if (jobSystemData->JobStartingOnDispatch)
		{
			if (jobCount == 1)
				jobSystemData->JobsSignal.NotifyOne();
			else
				jobSystemData->JobsSignal.NotifyAll();
		}

		return label;
#else
		for (int32 i = 0; i < jobCount; i++)
			job(i);
		return 0;
#endif
	}

	JobHandle JobSystem::Dispatch(IJob* job, int32 jobCount)
	{
		PROFILE_CPU();
		if (jobCount <= 0)
			return 0;
#if JOB_SYSTEM_ENABLED
#if JOB_SYSTEM_USE_STATS
		const auto start = Platform::GetTimeCycles();
#endif
		const auto label = Platform::AtomicAdd(&jobSystemData->JobLabel, (int64)jobCount) + jobCount;

		JobData data;
		data.JobFunc = Function<void(int32)>([job](int index){
		  job->Run(index);
		});
		data.JobKey = label;

		JobContext context;
		context.JobsLeft = jobCount;

#if JOB_SYSTEM_USE_MUTEX
		jobSystemData->JobsLocker.Lock();
		jobSystemData->JobContexts.Add(label, context);
		for (data.Index = 0; data.Index < jobCount; data.Index++)
			jobSystemData->Jobs.PushBack(data);
		jobSystemData->JobsLocker.Unlock();
#else
		JobsLocker.Lock();
    JobContexts.Add(label, context);
    JobsLocker.Unlock();
    for (data.Index = 0; data.Index < jobCount; data.Index++)
        Jobs.enqueue(data);
#endif

#if JOB_SYSTEM_USE_STATS
		LOG_INFO("Thread", "Job enqueue time: {0} cycles", (int64)(Platform::GetTimeCycles() - start));
#endif

		if (jobSystemData->JobStartingOnDispatch)
		{
			if (jobCount == 1)
				jobSystemData->JobsSignal.NotifyOne();
			else
				jobSystemData->JobsSignal.NotifyAll();
		}

		return label;
#else
		for (int32 i = 0; i < jobCount; i++)
			job(i);
		return 0;
#endif
	}

	bool JobSystem::IsCompleted(JobHandle handle)
	{
		bool isCompleted = true;
#if JOB_SYSTEM_ENABLED
		PROFILE_CPU();

		jobSystemData->JobsLocker.Lock();
		const JobContext* context = jobSystemData->JobContexts.TryGet(handle);
		jobSystemData->JobsLocker.Unlock();

		isCompleted = context == nullptr;
#endif
		return isCompleted;
	}

	void JobSystem::Wait()
	{
#if JOB_SYSTEM_ENABLED
		jobSystemData->JobsLocker.Lock();
		int32 numJobs = jobSystemData->JobContexts.Count();
		jobSystemData->JobsLocker.Unlock();

		while (numJobs > 0)
		{
			jobSystemData->WaitMutex.Lock();
			jobSystemData->WaitSignal.Wait(jobSystemData->WaitMutex, 1);
			jobSystemData->WaitMutex.Unlock();

			jobSystemData->JobsLocker.Lock();
			numJobs = jobSystemData->JobContexts.Count();
			jobSystemData->JobsLocker.Unlock();
		}
#endif
	}

	void JobSystem::Wait(JobHandle handle)
	{
#if JOB_SYSTEM_ENABLED
		PROFILE_CPU();

		while (Platform::AtomicRead(&jobSystemData->ExitFlag) == 0)
		{
			jobSystemData->JobsLocker.Lock();
			const JobContext* context = jobSystemData->JobContexts.TryGet(handle);
			jobSystemData->JobsLocker.Unlock();

			// 如果 context 已执行，则跳过（最后一个Job会删除它）
			if (!context)
			{
				break;
			}

			// Wait on signal until input label is not yet done
			jobSystemData->WaitMutex.Lock();
			jobSystemData->WaitSignal.Wait(jobSystemData->WaitMutex, 1);
			jobSystemData->WaitMutex.Unlock();

			// Wake up any thread to prevent stalling in highly multi-threaded environment
			jobSystemData->JobsSignal.NotifyOne();
		}

#if JOB_SYSTEM_USE_STATS
		LOG_INFO("Thread", "Job average dequeue time: {0} cycles", DequeueSum / DequeueCount);
		DequeueSum = DequeueCount = 0;
#endif
#endif
	}

	void JobSystem::SetJobStartingOnDispatch(bool value)
	{
#if JOB_SYSTEM_ENABLED
		jobSystemData->JobStartingOnDispatch = value;

		if (value)
		{
#if JOB_SYSTEM_USE_MUTEX
			jobSystemData->JobsLocker.Lock();
			const int32 count = jobSystemData->Jobs.Count();
			jobSystemData->JobsLocker.Unlock();
#else
			const int32 count = Jobs.Count();
#endif
			if (count == 1)
				jobSystemData->JobsSignal.NotifyOne();
			else if (count != 0)
				jobSystemData->JobsSignal.NotifyAll();
		}
#endif
	}

	int32 JobSystem::GetThreadsCount()
	{
#if JOB_SYSTEM_ENABLED
		return jobSystemData->ThreadsCount;
#else
		return 0;
#endif
	}

	//MainThreadTask
	//-------------------------------------------------------------------------

	void MainThreadTask::RunAll(float dt)
	{
		PROFILE_CPU();
		threadData->Locker.Lock();
		for (int32 i = threadData->Waiting.Count() - 1; i >= 0; i--)
		{
			auto task = threadData->Waiting[i];
			task->InitialDelay -= dt;
			if (task->InitialDelay < Math::ZeroTolerance)
			{
				threadData->Waiting.RemoveAt(i);
				threadData->Queue.Add(task);
			}
		}
		for (int32 i = 0; i < threadData->Queue.Count(); i++)
		{
			threadData->Queue[i]->Execute();
		}
		threadData->Queue.Clear();
		threadData->Locker.Unlock();
	}

	String MainThreadTask::ToString() const
	{
		return String::Format(SE_TEXT("Main Thread Task ({0})"), (int32)GetState());
	}

	void MainThreadTask::Enqueue()
	{
		threadData->Locker.Lock();
		if (InitialDelay <= Math::ZeroTolerance)
			threadData->Queue.Add(this);
		else
			threadData->Waiting.Add(this);
		threadData->Locker.Unlock();
	}

	bool MainThreadActionTask::Run()
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
		return true;
	}

	bool MainThreadActionTask::HasReference(void* obj) const
	{
		return obj == _target;
	}
}

template<>
struct TIsPODType<SE::Threading::JobData>
{
	enum { Value = false };
};

template<>
struct TIsPODType<SE::Threading::JobContext>
{
	enum { Value = true };
};