
#include "GBufferPass.h"

#include "Runtime/Core/Profiler/Profiler.h"
#include "Runtime/Graphics/GPUContext.h"
#include "Runtime/Graphics/GPUDevice.h"
#include "Runtime/Graphics/Textures/GPUTexture.h"
#include "Runtime/Render/RenderBuffers.h"
#include "Runtime/Render/RenderList.h"

namespace SE
{
	void GBufferPass::SetInputs(const RenderView& view, GBufferData& gBuffer)
	{
		// GBuffer params:
		// ViewInfo             :  x-1/Projection[0,0]   y-1/Projection[1,1]   z-(Far / (Far - Near)   w-(-Far * Near) / (Far - Near) / Far)
		// ScreenSize           :  x-Width               y-Height              z-1 / Width             w-1 / Height
		// ViewPos,ViewFar      :  x,y,z - world space view position                                   w-Far
		// InvViewMatrix        :  inverse view matrix (4 rows by 4 columns)
		// InvProjectionMatrix  :  inverse projection matrix (4 rows by 4 columns)
		gBuffer.ViewInfo = view.ViewInfo;
		gBuffer.ScreenSize = view.ScreenSize;
		gBuffer.ViewPos = view.Position;
		gBuffer.ViewFar = view.Far;
		Matrix::Transpose(view.IV, gBuffer.InvViewMatrix);
		Matrix::Transpose(view.IP, gBuffer.InvProjectionMatrix);
	}

	void GBufferPass::Execute(RenderContext& renderContext, GPUTexture* lightBuffer)
	{
		PROFILE_GPU_CPU("GBuffer");

		auto context = renderContext.gpuContext;
		GPUTextureView* targetBuffers[5] =
		{
			lightBuffer->View(),
			renderContext.buffers->GBuffer0->View(),
			renderContext.buffers->GBuffer1->View(),
			renderContext.buffers->GBuffer2->View(),
			renderContext.buffers->GBuffer3->View(),
		};
		renderContext.view.Pass = DrawPass::GBuffer;

		// Clear GBuffer
		{
			PROFILE_GPU_CPU_NAMED("Clear");

			context->ClearDepth(*renderContext.buffers->DepthBuffer);
			context->Clear(lightBuffer->View(), Colors::Transparent);
			context->Clear(renderContext.buffers->GBuffer0->View(), Colors::Transparent);
			context->Clear(renderContext.buffers->GBuffer1->View(), Colors::Transparent);
			context->Clear(renderContext.buffers->GBuffer2->View(), Color(1, 0, 0, 0));
			context->Clear(renderContext.buffers->GBuffer3->View(), Colors::Transparent);
		}

		// Draw objects that can get decals
		context->SetRenderTarget(ToSpan(targetBuffers, ARRAY_SIZE(targetBuffers)), *renderContext.buffers->DepthBuffer);
		renderContext.list->ExecuteDrawCalls(renderContext, DrawCallsListType::GBuffer);

		// Draw decals
		// DrawDecals(renderContext, lightBuffer->View());

		// Draw objects that cannot get decals
		context->SetRenderTarget(ToSpan(targetBuffers, ARRAY_SIZE(targetBuffers)), *renderContext.buffers->DepthBuffer);
		renderContext.list->ExecuteDrawCalls(renderContext, DrawCallsListType::GBufferNoDecals);

		// Draw sky
		if (renderContext.list->Sky && m_SkyModel && m_SkyModel->CanBeRendered()/* && renderContext.view.Flags(ViewFlags::Sky)*/)
		{
			PROFILE_GPU_CPU_NAMED("Sky");
			context->SetRenderTarget(ToSpan(targetBuffers, ARRAY_SIZE(targetBuffers)), *renderContext.buffers->DepthBuffer);
			DrawSky(renderContext, context);
		}

		context->ResetRenderTarget();

	}

	void GBufferPass::DrawSky(RenderContext& renderContext, GPUContext* context)
	{
		// Cache data
		auto model = m_SkyModel.Get();
		auto box = model->GetBox();

		// Calculate sphere model transform to cover far plane
		Matrix m1, m2;
		Matrix::Scaling(renderContext.view.Far / ((float)box.GetSize().y * 0.5f) * 0.95f, m1); // Scale to fit whole view frustum
		Matrix::CreateWorld(renderContext.view.Position, Float3::Up, Float3::Backward, m2); // Rotate sphere model
		m1 *= m2;

		// Draw sky
		renderContext.list->Sky->ApplySky(context, renderContext, m1);
		model->Render(context);
	}

	String GBufferPass::ToString() const
	{
		return SE_TEXT("GBuffer");
	}

	bool GBufferPass::Init()
	{
		m_SkyModel = AssetContent::LoadAsyncInternal<Model>(SE_TEXT("Assets/Models/Sphere"));

		return true;
	}

	void GBufferPass::Dispose()
	{

	}

	bool GBufferPass::SetupResources()
	{
		m_SkyModel->WaitForLoaded();
		if (!m_SkyModel->IsLoaded())
		{
			return false;
		}

		return true;
	}
} // SE