
#include "ColorGradingPass.h"

#include "Core/Profiler/Profiler.h"
#include "Runtime/Graphics/GPUContext.h"
#include "Runtime/Graphics/RenderTargetPool.h"
#include "Runtime/Graphics/Shaders/GPUConstantBuffer.h"
#include "Runtime/Render/RenderList.h"

namespace SE
{
	GES_PACK_STRUCT(struct Data {
	Float4 ColorSaturationShadows;
	Float4 ColorContrastShadows;
	Float4 ColorGammaShadows;
	Float4 ColorGainShadows;
	Float4 ColorOffsetShadows;

	Float4 ColorSaturationMidtones;
	Float4 ColorContrastMidtones;
	Float4 ColorGammaMidtones;
	Float4 ColorGainMidtones;
	Float4 ColorOffsetMidtones;

	Float4 ColorSaturationHighlights;
	Float4 ColorContrastHighlights;
	Float4 ColorGammaHighlights;
	Float4 ColorGainHighlights;
	Float4 ColorOffsetHighlights;

	float ColorCorrectionShadowsMax;
	float ColorCorrectionHighlightsMin;
	float WhiteTemp;
	float WhiteTint;

	Float3 Dummy;
	float LutWeight;
	});


	ColorGradingPass::ColorGradingPass()
		: m_UseVolumeTexture(false)
		, m_LutFormat()
		, m_Shader(nullptr)
	{
	}


	String ColorGradingPass::ToString() const
	{
		return SE_TEXT("ColorGradingPass");
	}

	bool ColorGradingPass::Init()
	{
		// Detect if can use volume texture (3d) for a LUT (faster, requires geometry shader)
		const auto device = GPUDevice::instance;
#if GPU_ALLOW_GEOMETRY_SHADERS
		_useVolumeTexture = device->Limits.HasGeometryShaders && device->Limits.HasVolumeTextureRendering;
#endif

		// Pick a proper LUT pixels format
		m_LutFormat = PixelFormat::R10G10B10A2_UNorm;
		const auto formatSupport = device->GetPixelFormatFeatures(m_LutFormat).Support;
		EnumFlags<FormatSupport> formatSupportFlags = {FormatSupport::ShaderSample, FormatSupport::RenderTarget};
		if (m_UseVolumeTexture)
		{
			formatSupportFlags.SetFlag(FormatSupport::Texture3D);
		}
		else
		{
			formatSupportFlags.SetFlag(FormatSupport::Texture2D);
		}

		if (formatSupport.IsNotFlag(formatSupportFlags))
		{
			// Fallback to format that is supported on every washing machine
			m_LutFormat = PixelFormat::R8G8B8A8_UNorm;
		}

		// Create pipeline state
		m_PsLut.CreatePipelineStates();

		// Load shader
		m_Shader = AssetContent::LoadAsyncInternal<Shader>(SE_TEXT("Shaders/ColorGrading"));
		if (m_Shader == nullptr)
		{
			return false;
		}
#if COMPILE_WITH_DEV_ENV
		_shader.Get()->OnReloading.Bind<ColorGradingPass, &ColorGradingPass::OnShaderReloading>(this);
#endif

		return true;
	}

	void ColorGradingPass::Dispose()
	{
		// Base
		RendererPass::Dispose();

		// Cleanup
		m_PsLut.Delete();
		m_Shader = nullptr;
	}

	bool ColorGradingPass::SetupResources()
	{
		if (m_Shader == nullptr)
		{
			return false;
		}

		m_Shader->WaitForLoaded();

		// Wait for shader
		if (!m_Shader->IsLoaded())
		{
			return false;
		}

		const auto shader = m_Shader->GetShader();

		// Validate shader constant buffer size
		if (shader->GetCB(0)->GetSize() != sizeof(Data))
		{
			REPORT_INVALID_SHADER_PASS_CB_SIZE(shader, 0, Data);
			return false;
		}

		// Create pipeline stages
		GPUPipelineState::Description psDesc = GPUPipelineState::Description::DefaultFullscreenTriangle;
		if (!m_PsLut.IsValid())
		{
			StringView psName;
#if GPU_ALLOW_GEOMETRY_SHADERS
			if (_useVolumeTexture)
			{
				psDesc.VS = shader->GetVS("VS_WriteToSlice");
				psDesc.GS = shader->GetGS("GS_WriteToSlice");
				psName = "PS_Lut3D";
			}
			else
#endif
			{
				psName = SE_TEXT("PS_Lut2D");
			}
			if (!m_PsLut.Create(psDesc, shader, psName))
			{
				return false;
			}
		}

		return true;
	}

	GPUTexture* ColorGradingPass::RenderLUT(RenderContext& renderContext)
	{
		// Ensure to have valid data
		if (!CheckIfSkipPass())
			return nullptr;

		PROFILE_GPU_CPU("Color Grading LUT");

		// For a 3D texture, the viewport is 16x16 (per slice), for a 2D texture, it's unwrapped to 256x16
		const int32 LutSize = 32; // this must match value in shader (see ColorGrading.shader and PostProcessing.shader)
		GPUTextureDescription lutDesc;
#if GPU_ALLOW_GEOMETRY_SHADERS
		if (_useVolumeTexture)
		{
			lutDesc = GPUTextureDescription::New3D(LutSize, LutSize, LutSize, 1, _lutFormat);
		}
		else
#endif
		{
			lutDesc = GPUTextureDescription::New2D(LutSize * LutSize, LutSize, 1, m_LutFormat);
		}
		const auto lut = RenderTargetPool::Get(lutDesc);
		RENDER_TARGET_POOL_SET_NAME(lut, "ColorGrading.LUT");

		// Prepare the parameters
		Data data;
		auto& toneMapping = renderContext.list->PostSettings.ToneMapping;
		auto& colorGrading = renderContext.list->PostSettings.ColorGrading;
		// White Balance
		data.WhiteTemp = toneMapping.WhiteTemperature;
		data.WhiteTint = toneMapping.WhiteTint;
		// Shadows
		data.ColorSaturationShadows = colorGrading.ColorSaturationShadows * colorGrading.ColorSaturation;
		data.ColorContrastShadows = colorGrading.ColorContrastShadows * colorGrading.ColorContrast;
		data.ColorGammaShadows = colorGrading.ColorGammaShadows * colorGrading.ColorGamma;
		data.ColorGainShadows = colorGrading.ColorGainShadows * colorGrading.ColorGain;
		data.ColorOffsetShadows = colorGrading.ColorOffsetShadows + colorGrading.ColorOffset;
		data.ColorCorrectionShadowsMax = colorGrading.ShadowsMax;
		// Midtones
		data.ColorSaturationMidtones = colorGrading.ColorSaturationMidtones * colorGrading.ColorSaturation;
		data.ColorContrastMidtones = colorGrading.ColorContrastMidtones * colorGrading.ColorContrast;
		data.ColorGammaMidtones = colorGrading.ColorGammaMidtones * colorGrading.ColorGamma;
		data.ColorGainMidtones = colorGrading.ColorGainMidtones * colorGrading.ColorGain;
		data.ColorOffsetMidtones = colorGrading.ColorOffsetMidtones + colorGrading.ColorOffset;
		// Highlights
		data.ColorSaturationHighlights = colorGrading.ColorSaturationHighlights * colorGrading.ColorSaturation;
		data.ColorContrastHighlights = colorGrading.ColorContrastHighlights * colorGrading.ColorContrast;
		data.ColorGammaHighlights = colorGrading.ColorGammaHighlights * colorGrading.ColorGamma;
		data.ColorGainHighlights = colorGrading.ColorGainHighlights * colorGrading.ColorGain;
		data.ColorOffsetHighlights = colorGrading.ColorOffsetHighlights + colorGrading.ColorOffset;
		data.ColorCorrectionHighlightsMin = colorGrading.HighlightsMin;
		//
		const bool useLut = colorGrading.LutTexture && colorGrading.LutTexture->IsLoaded() &&
			colorGrading.LutTexture->GetResidentMipLevels() > 0 &&
				colorGrading.LutWeight > Math::ZeroTolerance;

		data.LutWeight = useLut ? colorGrading.LutWeight : 0.0f;

		// Prepare
		auto device = GPUDevice::instance;
		auto context = device->GetMainContext();
		const auto cb = m_Shader->GetShader()->GetCB(0);
		context->UpdateCB(cb, &data);
		context->BindCB(0, cb);
		context->SetViewportAndScissors((float)lutDesc.Width, (float)lutDesc.Height);
		context->SetState(m_PsLut.Get((int32)toneMapping.Mode));
		context->BindSR(0, useLut ? colorGrading.LutTexture->GetTexture() : nullptr);

		// Draw
#if GPU_ALLOW_GEOMETRY_SHADERS
		if (_useVolumeTexture)
		{
			context->SetRenderTarget(lut->ViewVolume());

			// Render one fullscreen-triangle per slice intersecting the bounds
			const int32 numInstances = lutDesc.Depth;
			context->DrawFullscreenTriangle(numInstances);
		}
		else
#endif
		{
			context->SetRenderTarget(lut->View());
			context->DrawFullscreenTriangle();
		}
		context->UnBindSR(0);

		return lut;
	}
}
