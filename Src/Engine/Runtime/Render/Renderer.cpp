
#include "Renderer.h"

#include "Utils/RendererAllocation.hpp"
#include "RenderBuffers.h"
#include "RenderContext.h"
#include "RendererPass.h"
#include "RenderList.h"
#include "RenderTask.h"
#include "Runtime/Graphics/GPUDevice.h"
#include "Runtime/Graphics/Viewport.h"
#include "Runtime/Graphics/GPUContext.h"
#include "RenderPass/ForwardPass.h"
#include "RenderPass/FXAA.h"
#include "RenderPass/GBufferPass.h"
#include "RenderPass/LightPass.h"
#include "Runtime/Graphics/RenderTargetPool.h"

#include "Runtime/Core/Systems.h"
#include "Runtime/Core/Profiler/ProfilerCPU.h"
#include "Runtime/Core/Profiler/ProfilerGPU.h"
#include "Runtime/Core/Thread/JobSystem.h"
#include "RenderPass/ColorGradingPass.h"
#include "RenderPass/GizmosPass.h"
#include "RenderPass/PostProcessingPass.h"
#include "RenderPass/ShadowPass.h"

namespace SE
{

	struct RendererData
	{
		List<RendererPassBase*> PassList = List<RendererPassBase*>(64);

	} *rendererData;


	class RendererSystem : public ISystem
	{
		ENGINE_SYSTEM(RendererSystem)
	public:
		RendererSystem() : ISystem(SE_TEXT("Renderer"), 20)
		{
		}

		bool OnInit() override;
		void OnDispose() override;
	};

	ENGINE_SYSTEM_REGISTER(RendererSystem)

	bool RendererSystem::OnInit()
	{
		GPUDevice* device = GPUDevice::instance;

		// Initialize
		if (!device->GetMainContext()->LoadDefaultResources())
		{
			LOG_FATAL("Graphic", "Cannot load default resources.");
			return false;
		}

		rendererData = New<RendererData>();

		rendererData->PassList.Add(&GBufferPass::Instance());
		rendererData->PassList.Add(&ShadowsPass::Instance());
		rendererData->PassList.Add(&LightPass::Instance());
		rendererData->PassList.Add(&ForwardPass::Instance());
		rendererData->PassList.Add(&ColorGradingPass::Instance());
		rendererData->PassList.Add(&PostProcessingPass::Instance());
		rendererData->PassList.Add(&FXAA::Instance());

		// Init child services
		for (int32 i = 0; i < rendererData->PassList.Count(); i++)
		{
			if (!rendererData->PassList[i]->Init())
			{
				// LOG_FATAL("Render", "Cannot init {0}. Please see a log file for more info.", rendererData->PassList[i]->ToString());
				return false;
			}
		}

		return ISystem::OnInit();
	}

	void RendererSystem::OnDispose()
	{
		Delete(rendererData);
		ISystem::OnDispose();
	}

	void RenderPath(SceneRenderTask* task, RenderContext& renderContext, RenderContextBatch& renderContextBatch);
	void RenderAntiAliasingPass(RenderContext& renderContext, GPUTexture* input, GPUTextureView* output, const Viewport& outputViewport);

	void Renderer::Render(SceneRenderTask* task)
	{
		// 清理上下文状态、设置Viewport
		GPUContext* context = GPUDevice::instance->GetMainContext();
		context->ClearState();
		context->FlushState();
		const Viewport viewport = task->GetViewport();
		context->SetViewportAndScissors(viewport);

		// Prepare render context
		RenderContext renderContext(task);
		renderContext.list = RenderList::GetFromPool();
		renderContext.gpuContext = context;
		RenderContextBatch renderContextBatch(task);
		renderContextBatch.Contexts.Add(renderContext);

		task->OnPreRender(renderContext);
		RenderPath(task, renderContext, renderContextBatch);
		task->OnPostRender(renderContext);


		// Copy back the view (modified during rendering with rendering state like TAA frame index and jitter)
		task->View = renderContext.view;

		// Cleanup
		for (const auto& e : renderContextBatch.Contexts)
		{
			RenderList::ReturnToPool(e.list);
		}
	}

	void RenderPath(SceneRenderTask* task, RenderContext& renderContext, RenderContextBatch& renderContextBatch)
	{
		auto context = renderContext.gpuContext;
		auto& view = renderContext.view;
		ENGINE_ASSERT(renderContext.buffers && renderContext.buffers->GetWidth() > 0);

		// Prepare
		view.Prepare(renderContext);
		renderContext.buffers->Prepare();
		ShadowsPass::Instance().Prepare();

		// Build batch of render contexts (main view and shadow projections)
		{
			PROFILE_CPU_NAMED("Collect Draw Calls");
			// view.Pass.SetFlags(DrawPass::GBuffer, DrawPass::Forward, DrawPass::Distortion);

			bool drawShadows = /*!isGBufferDebug && EnumHasAnyFlags(view.Flags, ViewFlags::Shadows) && */ShadowsPass::Instance().IsReady();
			if (drawShadows)
			{
				ShadowsPass::Instance().SetupShadows(renderContext, renderContextBatch);
			}


			// Dispatch drawing (via JobSystem - multiple job batches for every scene)
			Threading::JobSystem::SetJobStartingOnDispatch(false);
			task->OnCollectDrawCalls(renderContextBatch, SceneRendering::DrawCategory::SceneDraw);
			task->OnCollectDrawCalls(renderContextBatch, SceneRendering::DrawCategory::SceneDrawAsync);


			// Wait for async jobs to finish
			Threading::JobSystem::SetJobStartingOnDispatch(true);
			for (const uint64 label : renderContextBatch.WaitLabels)
			{
				Threading::JobSystem::Wait(label);
			}
			renderContextBatch.WaitLabels.Clear();

		}
		
		// Sort draw calls
		{
			PROFILE_CPU_NAMED("Sort Draw Calls");
			renderContext.list->SortDrawCalls(renderContext, false, DrawCallsListType::GBuffer);
			renderContext.list->SortDrawCalls(renderContext, false, DrawCallsListType::GBufferNoDecals);
			renderContext.list->SortDrawCalls(renderContext, true, DrawCallsListType::Forward);
			renderContext.list->SortDrawCalls(renderContext, false, DrawCallsListType::Distortion);
			/*if (setup.UseMotionVectors)
			{
				renderContext.list->SortDrawCalls(renderContext, false, DrawCallsListType::MotionVectors);
			}*/
			for (int32 i = 1; i < renderContextBatch.Contexts.Count(); i++)
			{
				auto& shadowContext = renderContextBatch.Contexts[i];
				shadowContext.list->SortDrawCalls(shadowContext, false, DrawCallsListType::Depth);
				shadowContext.list->SortDrawCalls(shadowContext, false, shadowContext.list->ShadowDepthDrawCallsList, renderContext.list->DrawCalls);
			}
		}

		// Get the light accumulation buffer
		auto outputFormat = renderContext.buffers->GetOutputFormat();
		GPUTextureBitFlags tempFlags = {GPUTextureFlags::ShaderResource , GPUTextureFlags::RenderTarget};
		if (GPUDevice::instance->GetGPULimits().HasCompute)
		{
			tempFlags.SetFlag(GPUTextureFlags::UnorderedAccess);
		}
		auto tempDesc = GPUTextureDescription::New2D(renderContext.buffers->GetWidth(), renderContext.buffers->GetHeight(), outputFormat, tempFlags);
		auto lightBuffer = RenderTargetPool::Get(tempDesc);
		RENDER_TARGET_POOL_SET_NAME(lightBuffer, "LightBuffer");

		// GBuffer
		GBufferPass::Instance().Execute(renderContext, lightBuffer);


		// Render lighting
		renderContextBatch.GetMainContext() = renderContext; // Sync render context in batch with the current value
		LightPass::Instance().RenderLight(renderContextBatch, *lightBuffer);


		// Run forward pass
		auto frameBuffer = RenderTargetPool::Get(tempDesc);
		RENDER_TARGET_POOL_SET_NAME(frameBuffer, "FrameBuffer");
		ForwardPass::Instance().Render(renderContext, lightBuffer, frameBuffer);

		// Cleanup
		context->ResetRenderTarget();
		context->ResetSR();
		context->FlushState();
		RenderTargetPool::Release(lightBuffer);


		const Viewport outputViewport = task->GetOutputViewport();
		GPUTextureView* outputView = task->GetOutputView();


		auto tempBuffer = RenderTargetPool::Get(tempDesc);
		RENDER_TARGET_POOL_SET_NAME(tempBuffer, "TempBuffer");
		// Color Grading LUT generation
		auto colorGradingLUT = ColorGradingPass::Instance().RenderLUT(renderContext);

		PostProcessingPass::Instance().Render(renderContext, frameBuffer, tempBuffer, colorGradingLUT);
		RenderTargetPool::Release(colorGradingLUT);
		Swap(frameBuffer, tempBuffer);

		// Cleanup
		context->ResetRenderTarget();
		context->ResetSR();
		context->FlushState();

		// Draw Gizmos
		if (view.DrawGizmos)
		{

		}

		// AA -> Back Buffer
		RenderAntiAliasingPass(renderContext, frameBuffer, outputView, outputViewport);


		RenderTargetPool::Release(tempBuffer);
		RenderTargetPool::Release(frameBuffer);
	}

	void RenderAntiAliasingPass(RenderContext& renderContext, GPUTexture* input, GPUTextureView* output, const Viewport& outputViewport)
	{
		auto context = renderContext.gpuContext;
		context->SetViewportAndScissors(outputViewport);

		FXAA::Instance().Render(renderContext, input, output);

		/*const auto aaMode = renderContext.list->Settings.AntiAliasing.Mode;
		if (aaMode == AntialiasingMode::FastApproximateAntialiasing)
		{
			FXAA::Instance().Render(renderContext, input, output);
		}
		else if (aaMode == AntialiasingMode::SubpixelMorphologicalAntialiasing)
		{
			SMAA::Instance()->Render(renderContext, input, output);
		}
		else
		{
			PROFILE_GPU("Copy frame");
			context->SetRenderTarget(output);
			context->Draw(input);
		}*/
	}

} // SE