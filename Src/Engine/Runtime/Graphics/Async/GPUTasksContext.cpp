#include "GPUTasksContext.h"

#include "GPUTask.h"
#include "Runtime/Engine.h"
#include "Runtime/Graphics/GPUDevice.h"

namespace SE
{
#define GPU_TASKS_USE_DEDICATED_CONTEXT 0

	GPUTasksContext::GPUTasksContext(GPUDevice* device)
		: _tasksDone(64), _totalTasksDoneCount(0)
	{
		_currentSyncPoint = 0;

#if GPU_TASKS_USE_DEDICATED_CONTEXT
		// Create GPU context
		GPU = device->CreateContext(true);
#else
		// Reuse Graphics Device context
		GPU = device->GetMainContext();
#endif
	}

	GPUTasksContext::~GPUTasksContext()
	{
		ENGINE_ASSERT(Threading::IsMainThread());

		// Cancel jobs to sync
		auto tasks = _tasksDone;
		_tasksDone.Clear();
		for (int32 i = 0; i < tasks.Count(); i++)
		{
			auto task = tasks[i];
			LOG_WARNING("Graphic", "{0} has been canceled before a sync", task->ToString());
			tasks[i]->CancelSync();
		}

#if GPU_TASKS_USE_DEDICATED_CONTEXT
		SAFE_DELETE(GPU);
#endif
	}

	void GPUTasksContext::Run(GPUTask* task)
	{
		ENGINE_ASSERT(task != nullptr);

		_tasksDone.Add(task);
		task->Execute(this);
	}

	void GPUTasksContext::OnCancelSync(GPUTask* task)
	{
		ENGINE_ASSERT(task != nullptr);

		_tasksDone.Remove(task);

		LOG_WARNING("Graphic", "{0} has been canceled before a sync", task->ToString());
	}

	void GPUTasksContext::OnFrameBegin()
	{
#if GPU_TASKS_USE_DEDICATED_CONTEXT
		GPU->FrameBegin();
#endif

		// Move forward one frame
		++_currentSyncPoint;

		// Try to flush done jobs
		for (int32 i = _tasksDone.Count() - 1; i >= 0; i--)
		{
			GPUSyncPoint syncPoint = _tasksDone[i]->GetSyncPoint();
			if (_currentSyncPoint - syncPoint >= GPU_ASYNC_LATENCY)
			{
				// TODO: add stats counter and count performed jobs, print to log on exit.

				auto job = _tasksDone[i];
				job->Sync();

				_tasksDone.RemoveAt(i);
				_totalTasksDoneCount++;
			}
		}
	}

	void GPUTasksContext::OnFrameEnd()
	{
#if GPU_TASKS_USE_DEDICATED_CONTEXT
		GPU->FrameEnd();
#endif
	}
}