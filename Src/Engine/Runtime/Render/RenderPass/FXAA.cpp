
#include "FXAA.h"

#include "Runtime/Core/Profiler/Profiler.h"
#include "Runtime/Graphics/GPUContext.h"
#include "Runtime/Graphics/Shaders/GPUConstantBuffer.h"

namespace SE
{
	GES_PACK_STRUCT(struct Data
		{
		Float4 ScreenSize;
		});

	String FXAA::ToString() const
	{
		return SE_TEXT("FXAA");
	}

	bool FXAA::Init()
	{
		_psFXAA.CreatePipelineStates();
		_shader = AssetContent::LoadAsyncInternal<Shader>(SE_TEXT("Shaders/FXAA"));
		if (_shader == nullptr)
		{
			return false;
		}
#if COMPILE_WITH_DEV_ENV
		_shader.Get()->OnReloading.Bind<FXAA, &FXAA::OnShaderReloading>(this);
#endif

		return true;
	}

	bool FXAA::SetupResources()
	{
		if (!_shader.HasReference())
		{
			return false;
		}

		_shader->WaitForLoaded();

		if (!_shader->IsLoaded())
		{
			return false;
		}

		const auto shader = _shader->GetShader();
		if (shader->GetCB(0)->GetSize() != sizeof(Data))
		{
			REPORT_INVALID_SHADER_PASS_CB_SIZE(shader, 0, Data);
			return false;
		}

		GPUPipelineState::Description psDesc;
		if (!_psFXAA.IsValid())
		{
			psDesc = GPUPipelineState::Description::DefaultFullscreenTriangle;
			if (!_psFXAA.Create(psDesc, shader, SE_TEXT("PS")))
			{
				return false;
			}
		}

		return true;
	}

	void FXAA::Dispose()
	{
		// Base
		RendererPass::Dispose();

		_psFXAA.Delete();
		_shader = nullptr;
	}

	void FXAA::Render(RenderContext& renderContext, GPUTexture* input, GPUTextureView* output)
	{
		auto context = renderContext.gpuContext;
		if (!CheckIfSkipPass())
		{
			// Resources are missing. Do not perform rendering, just copy input frame.
			context->SetRenderTarget(output);
			context->Draw(input);
			return;
		}
		PROFILE_GPU_CPU("Fast Approximate Antialiasing");

		// Bind input
		Data data;
		data.ScreenSize = renderContext.view.ScreenSize;
		const auto cb = _shader->GetShader()->GetCB(0);
		context->UpdateCB(cb, &data);
		context->BindCB(0, cb);
		context->BindSR(0, input);

		// Render
		context->SetRenderTarget(output);
		const auto qualityLevel = Math::Clamp(static_cast<int32>(1), 0, static_cast<int32>(Quality::MAX) - 1);
		context->SetState(_psFXAA.Get(qualityLevel));
		context->DrawFullscreenTriangle();
	}
} // SE