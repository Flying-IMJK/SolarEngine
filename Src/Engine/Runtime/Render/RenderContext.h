#pragma once

#include "RenderView.h"
#include "Core/Types/Collections/List.h"
#include "Utils/RendererAllocation.hpp"

namespace SE
{
	class GPUContext;
	class RenderBuffers;
	class RenderList;
	class SceneRenderTask;

	// <summary>
	/// The high-level renderer context. Used to collect the draw calls for the scene rendering. Can be used to perform a custom rendering.
	/// </summary>
	struct SE_API_RUNTIME RenderContext
	{
		GPUContext* gpuContext = nullptr;

		/// <summary>
		/// The render buffers.
		/// </summary>
		RenderBuffers* buffers = nullptr;

		/// <summary>
		/// The render list.
		/// </summary>
		RenderList* list = nullptr;

		/// <summary>
		/// The scene rendering task that is a source of renderable objects (optional).
		/// </summary>
		SceneRenderTask* task = nullptr;

		/// <summary>
		/// The proxy render view used to synchronize objects level of detail during rendering (eg. during shadow maps rendering passes). It's optional.
		/// </summary>
		RenderView* lodProxyView = nullptr;

		/// <summary>
		/// The render view.
		/// </summary>
		RenderView view;

		/// <summary>
		/// The GPU access locking critical section to protect data access when performing multi-threaded rendering.
		/// </summary>
		static CriticalSection GPULocker;

		RenderContext() = default;
		RenderContext(SceneRenderTask* task) noexcept;
	};

	/// <summary>
	/// The high-level renderer context batch that encapsulates multiple rendering requests within a single task (eg. optimize main view scene rendering and shadow projections at once).
	/// </summary>
	struct RenderContextBatch
	{
		/// <summary>
		/// The render buffers.
		/// </summary>
		RenderBuffers* buffers = nullptr;

		/// <summary>
		/// The scene rendering task that is a source of renderable objects (optional).
		/// </summary>
		SceneRenderTask* task = nullptr;

		/// <summary>
		/// The all render views collection for the current rendering (main view, shadow projections, etc.).
		/// </summary>
		List<RenderContext, RendererAllocation> Contexts;

		/// <summary>
		/// The Job System labels to wait on, after draw calls collecting.
		/// </summary>
		List<uint64, InlinedAllocation<8>> WaitLabels;

		/// <summary>
		/// Enables using async tasks via Job System when performing drawing.
		/// </summary>
		bool EnableAsync = true;

		RenderContextBatch() = default;
		RenderContextBatch(SceneRenderTask* task);
		RenderContextBatch(const RenderContext& context);

		FORCE_INLINE RenderContext& GetMainContext()
		{
			return Contexts.Get()[0];
		}

		FORCE_INLINE const RenderContext& GetMainContext() const
		{
			return Contexts.Get()[0];
		}
	};

}
