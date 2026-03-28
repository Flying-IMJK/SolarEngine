
#pragma once

#include "GPUTasksContext.h"

namespace SE
{
	class GPUTask;

	/// <summary>
	/// Describes object responsible for GPU jobs execution scheduling.
	/// </summary>
	class GPUTasksExecutor
	{
	protected:
		List<GPUTasksContext*> _contextList;

	public:
		/// <summary>
		/// Destructor
		/// </summary>
		virtual ~GPUTasksExecutor();

	public:
		/// <summary>
		/// Sync point event called on begin of the frame
		/// </summary>
		virtual void FrameBegin() = 0;

		/// <summary>
		/// Sync point event called on end of the frame
		/// </summary>
		virtual void FrameEnd() = 0;

	public:
		/// <summary>
		/// Gets the context list.
		/// </summary>
		FORCE_INLINE const List<GPUTasksContext*>* GetContextList() const
		{
			return &_contextList;
		}

	protected:
		GPUTasksContext* CreateContext();
	};


	/// <summary>
	/// Default implementation for GPU async job execution
	/// </summary>
	class DefaultGPUTasksExecutor : public GPUTasksExecutor
	{
	protected:
		GPUTasksContext* _context;

	public:
		/// <summary>
		/// Init
		/// </summary>
		DefaultGPUTasksExecutor();

	public:
		void FrameBegin() override;
		void FrameEnd() override;
	};
} // SE

