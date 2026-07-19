
#include "LightPass.h"

#include "GBufferPass.h"
#include "ShadowPass.h"
#include "Runtime/Core/Profiler/Profiler.h"
#include "Runtime/Graphics/GPUContext.h"
#include "Runtime/Graphics/RenderTargetPool.h"
#include "Runtime/Graphics/Shaders/GPUConstantBuffer.h"
#include "Runtime/Graphics/Shaders/GPUShader.h"
#include "Runtime/Render/RenderBuffers.h"
#include "Runtime/Render/RenderList.h"
#include "Runtime/Resource/Assets/Materials/Shader.h"

namespace SE
{
	GES_PACK_STRUCT(struct PerLight{
		LightData Light;
		Matrix WVP;
	});

	GES_PACK_STRUCT(struct PerFrame{
		GBufferData GBuffer;
	});

	bool LightPass::Init()
	{
		m_Shader = AssetContent::LoadAsyncInternal<Shader>(SE_TEXT("Shaders/Lights"));
		if (m_Shader == nullptr)
		{
			return false;
		}

		m_Shader->WaitForLoaded();

		m_PsLightDir.CreatePipelineStates();
		m_PsLightPointNormal.CreatePipelineStates();
		m_PsLightPointInverted.CreatePipelineStates();

		m_SphereModel = AssetContent::LoadAsyncInternal<Model>(SE_TEXT("Assets/Models/Sphere"));
		if (m_SphereModel == nullptr)
		{
			return false;
		}

		m_SphereModel->WaitForLoaded();


		auto format = PixelFormat::R8G8_UNorm;
		if (!GPUDevice::instance->GetPixelFormatFeatures(format).Support.AllFlagsSet(FormatSupport::RenderTarget, FormatSupport::ShaderSample, FormatSupport::Texture2D))
		{
			format = PixelFormat::B8G8R8A8_UNorm;
		}
		m_ShadowMaskFormat = format;

		return true;
	}

	void LightPass::Dispose()
	{
		RendererPass<LightPass>::Dispose();

		// Cleanup
		m_PsLightDir.Delete();
	}

	bool LightPass::SetupResources()
	{
		if (!m_Shader.HasReference() || !m_Shader->IsLoaded() || !m_SphereModel->CanBeRendered())
		{
			return false;
		}

		auto shader = m_Shader->GetShader();

		// Validate shader constant buffers sizes
		if (shader->GetCB(0)->GetSize() != sizeof(PerLight))
		{
			REPORT_INVALID_SHADER_PASS_CB_SIZE(shader, 0, PerLight);
			return false;
		}
		if (shader->GetCB(1)->GetSize() != sizeof(PerFrame))
		{
			REPORT_INVALID_SHADER_PASS_CB_SIZE(shader, 1, PerFrame);
			return false;
		}

		// Create pipeline stages
		GPUPipelineState::Description psDesc;
		if (!m_PsLightDir.IsValid())
		{
			psDesc = GPUPipelineState::Description::DefaultFullscreenTriangle;
			psDesc.BlendMode = BlendingMode::Add;
			psDesc.BlendMode.RenderTargetWriteMask = BlendingMode::ColorWrite::RGB;
			if (!m_PsLightDir.Create(psDesc, shader, SE_TEXT("PS_Directional")))
			{
				return false;
			}
		}

		if (!m_PsLightPointNormal.IsValid() || !m_PsLightPointInverted.IsValid())
		{
			psDesc = GPUPipelineState::Description::DefaultNoDepth;
			psDesc.BlendMode = BlendingMode::Add;
			psDesc.BlendMode.RenderTargetWriteMask = BlendingMode::ColorWrite::RGB;
			psDesc.VS = shader->GetVS(SE_TEXT("VS_Model"));
			psDesc.CullMode = CullMode::Inverted;
			if (!m_PsLightPointInverted.Create(psDesc, shader, SE_TEXT("PS_Point")))
			{
				return false ;
			}
			psDesc.CullMode = CullMode::Normal;
			psDesc.DepthEnable = true;
			if (!m_PsLightPointNormal.Create(psDesc, shader, SE_TEXT("PS_Point")))
			{
				return false;
			}
		}

		return true;
	}

	void LightPass::RenderLight(RenderContextBatch& renderContextBatch, GPUTextureView* lightBuffer)
	{
		// Ensure to have valid data
		if (!CheckIfSkipPass())
		{
			// Resources are missing. Do not perform rendering.
			return;
		}

		// Cache data
		auto& renderContext = renderContextBatch.Contexts[0];
		auto context = renderContext.gpuContext;
		auto& view = renderContext.view;
		auto mainCache = renderContext.list;
		const auto lightShader = m_Shader->GetShader();
		const bool useShadows = ShadowsPass::Instance().IsReady();// && EnumHasAnyFlags(view.Flags, ViewFlags::Shadows);
		const bool disableSpecular = false; // (view.flags & ViewFlags::SpecularLight) == ViewFlags::None;


		PerLight perLight;
		PerFrame perFrame;

		// Bind output
		GPUTexture* depthBuffer = renderContext.buffers->DepthBuffer;
		const bool depthBufferReadOnly = depthBuffer->Flags().IsFlag(GPUTextureFlags::ReadOnlyDepthView);
		GPUTextureView* depthBufferRTV = depthBufferReadOnly ? depthBuffer->ViewReadOnlyDepth() : nullptr;
		GPUTextureView* depthBufferSRV = depthBufferReadOnly ? depthBuffer->ViewReadOnlyDepth() : depthBuffer->View();
		context->SetRenderTarget(lightBuffer, depthBufferRTV);


		// Set per frame data
		GBufferPass::SetInputs(renderContext.view, perFrame.GBuffer);
		auto cb0 = lightShader->GetCB(0);
		auto cb1 = lightShader->GetCB(1);
		context->UpdateCB(cb1, &perFrame);

		// Bind inputs
		context->BindSR(0, renderContext.buffers->GBuffer0);
		context->BindSR(1, renderContext.buffers->GBuffer1);
		context->BindSR(2, renderContext.buffers->GBuffer2);
		context->BindSR(3, depthBufferSRV);
		context->BindSR(4, renderContext.buffers->GBuffer3);

		const float sphereModelScale = 3.0f;

		// Fullscreen shadow mask buffer
		GPUTexture* shadowMask = nullptr;
#define GET_SHADOW_MASK()	\
if (!shadowMask)			\
{							\
auto rtDesc = GPUTextureDescription::New2D(renderContext.buffers->GetWidth(), renderContext.buffers->GetHeight(), m_ShadowMaskFormat); \
shadowMask = RenderTargetPool::Get(rtDesc);					\
RENDER_TARGET_POOL_SET_NAME(shadowMask, "ShadowMask");		\
}															\
auto shadowMaskView = shadowMask->View();


		// Render all point lights
		for (int32 lightIndex = 0; lightIndex < mainCache->PointLights.Count(); lightIndex++)
		{
			PROFILE_GPU_CPU_NAMED("Point Light");

			// Cache data
			auto& light = mainCache->PointLights[lightIndex];
			float lightRadius = light.Radius;
			Float3 lightPosition = light.Position;
			const bool renderShadow = useShadows && light.ShadowDataIndex != -1;
			bool useIES = light.IESTexture != nullptr;

			// Get distance from view center to light center less radius (check if view is inside a sphere)
			float viewToCenter = Float3::Distance(view.Position, lightPosition);
			float distance = viewToCenter - lightRadius * sphereModelScale;
			bool isViewInside = distance < 0;

			// Calculate world view projection matrix for the light sphere
			Matrix world, wvp, matrix;
			Matrix::Scaling(lightRadius * sphereModelScale, wvp);
			Matrix::Translation(lightPosition, matrix);
			Matrix::Multiply(wvp, matrix, world);
			Matrix::Multiply(world, view.ViewProjection(), wvp);

			// Check if render shadow
			if (renderShadow)
			{
				GET_SHADOW_MASK();
				ShadowsPass::Instance().RenderShadow(renderContextBatch, light, shadowMaskView);

				// Bind output
				context->SetRenderTarget(lightBuffer, depthBufferRTV);

				// Set shadow mask
				context->BindSR(5, shadowMaskView);
			}
			else
			{
				context->UnBindSR(5);
			}

			// Pack light properties buffer
			light.SetupLightData(&perLight.Light, renderShadow);
			Matrix::Transpose(wvp, perLight.WVP);
			if (useIES)
			{
				context->BindSR(6, light.IESTexture);
			}

			// Calculate lighting
			context->UpdateCB(cb0, &perLight);
			context->BindCB(0, cb0);
			context->BindCB(1, cb1);
			int32 permutationIndex = (disableSpecular ? 1 : 0) + (useIES ? 2 : 0);
			context->SetState((isViewInside ? m_PsLightPointInverted : m_PsLightPointNormal).Get(permutationIndex));
			m_SphereModel->Render(context);
		}

		context->UnBindCB(0);

		// Render all directional lights
		for (int32 lightIndex = 0; lightIndex < mainCache->DirectionalLights.Count(); lightIndex++)
		{
			PROFILE_GPU_CPU_NAMED("Directional Light");

			// Cache data
			auto& light = mainCache->DirectionalLights[lightIndex];
			const bool renderShadow = useShadows && light.ShadowDataIndex != -1;
			// Check if render shadow
			if (renderShadow)
			{
				GET_SHADOW_MASK();

				ShadowsPass::Instance().RenderShadow(renderContextBatch, light, lightIndex, shadowMaskView);

				// Bind output
				context->SetRenderTarget(lightBuffer, depthBufferRTV);

				// Set shadow mask
				context->BindSR(5, shadowMaskView);
			}
			else
			{
				context->UnBindSR(5);
			}

			// Pack light properties buffer
			light.SetupLightData(&perLight.Light, renderShadow);

			// Calculate lighting
			context->UpdateCB(cb0, &perLight);
			context->BindCB(0, cb0);
			context->BindCB(1, cb1);
			context->SetState(m_PsLightDir.Get(disableSpecular));
			context->DrawFullscreenTriangle();
		}

		context->UnBindCB(0);

		RenderTargetPool::Release(shadowMask);

		// Restore state
		context->ResetRenderTarget();
		context->ResetSR();
		context->ResetCB();
	}

	String LightPass::ToString() const
	{
		return SE_TEXT("LightPass");
	}
} // SE