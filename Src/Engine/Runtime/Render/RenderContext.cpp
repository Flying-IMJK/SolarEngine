
#include "RenderContext.h"

#include "RenderTask.h"

namespace SE
{
	RenderContext::RenderContext(SceneRenderTask* task) noexcept
	{
		buffers = task->Buffers;
		this->task = task;
		view = task->View;
	}

	RenderContextBatch::RenderContextBatch(SceneRenderTask* task)
	{
		buffers = task->Buffers;
		this->task = task;
	}

	RenderContextBatch::RenderContextBatch(const RenderContext& context)
	{
		buffers = context.buffers;
		task = context.task;
		Contexts.Add(context);
	}
}
