
#include "GPUTasksExecutor.h"
#include "Runtime/Graphics/GPUDevice.h"
#include "GPUTasksSystem.h"

namespace SE
{
	GPUTasksExecutor::~GPUTasksExecutor()
	{
		// Stats
		int32 totalJobsDone = 0;
		for (int32 i = 0; i < _contextList.Count(); i++)
			totalJobsDone += _contextList[i]->GetTotalTasksDoneCount();
		LOG_INFO("Graphic", "Total GPU tasks done: {0}", totalJobsDone);

		_contextList.ClearDelete();
	}

	GPUTasksContext* GPUTasksExecutor::CreateContext()
	{
		auto context = GPUDevice::instance->CreateTasksContext();
		if (context == nullptr)
		{
			LOG_ERROR("Graphic", "Cannot create new GPU Tasks Context");
		}
		_contextList.Add(context);
		return context;
	}


	DefaultGPUTasksExecutor::DefaultGPUTasksExecutor()
		: _context(nullptr)
	{
	}

	void DefaultGPUTasksExecutor::FrameBegin()
	{
		// Ensure to have valid async context
		if (_context == nullptr)
			_context = CreateContext();

		_context->OnFrameBegin();
	}

	void DefaultGPUTasksExecutor::FrameEnd()
	{
		ENGINE_ASSERT(_context != nullptr);

		// Default implementation performs async operations on end of the frame which is synchronized with a rendering thread
		GPUTask* buffer[32];
		const int32 count = GPUDevice::instance->GetTasksSystem()->RequestWork(buffer, 32);
		for (int32 i = 0; i < count; i++)
		{
			_context->Run(buffer[i]);
		}

		_context->OnFrameEnd();
	}
} // SE