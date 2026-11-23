
#include <Runtime/Render/RenderPass/ColorGradingPass.h>

#include <Runtime/Core/Profiler/Profiler.h>
#include <Runtime/Graphics/Base/GPULimits.h>
#include <Runtime/Graphics/GPUContext.h>
#include <Runtime/Graphics/GPUDevice.h>
#include <Runtime/Graphics/RenderTargetPool.h>
#include <Runtime/Graphics/Shaders/GPUConstantBuffer.h>
#include <Runtime/Graphics/Shaders/ShaderProgramInstance.h>
#include <Runtime/Render/RenderList.h>
#include <Runtime/Resource/Assets/Materials/Shader.h>

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
    Color  ScreenFadeColor;
	});

    ColorGradingPass::ColorGradingPass() : m_UseVolumeTexture(false), m_LutFormat(), m_Shader(nullptr) {}

    String ColorGradingPass::ToString() const { return SE_TEXT("ColorGradingPass"); }

    bool ColorGradingPass::Init()
	{
		// Detect if can use volume texture (3d) for a LUT (faster, requires geometry shader)
		const auto device = GPUDevice::instance;
#if GPU_ALLOW_GEOMETRY_SHADERS
        m_UseVolumeTexture = device->GetGPULimits().HasGeometryShaders && device->GetGPULimits().HasVolumeTextureRendering;
#endif

		// Pick a proper LUT pixels format
		m_LutFormat = PixelFormat::R10G10B10A2_UNorm;
		const auto formatSupport = device->GetPixelFormatFeatures(m_LutFormat).Support;
		FormatSupport formatSupportFlags = FormatSupport::ShaderSample | FormatSupport::RenderTarget;
		if (m_UseVolumeTexture)
		{
			formatSupportFlags = EnumAddFlags(formatSupportFlags, FormatSupport::Texture3D);
		}
		else
		{
			formatSupportFlags = EnumAddFlags(formatSupportFlags, FormatSupport::Texture2D);
		}

        if (!EnumHasAllFlags(formatSupport, formatSupportFlags))
        {
            // Preserve the existing lower-precision format choice when 10-bit LUTs are unavailable.
            m_LutFormat = PixelFormat::R8G8B8A8_UNorm;
            if (!EnumHasAllFlags(device->GetPixelFormatFeatures(m_LutFormat).Support, formatSupportFlags))
            {
                LOG_ERROR("Render", "Color grading requires a sampleable render-target LUT format.");
                return false;
            }
        }

		// Load shader
        m_Shader = AssetContent::LoadAsyncInternal<Shader>(SE_TEXT("Shaders/PostProcess/ColorGrading"));
        if (m_Shader == nullptr)
        {
            return false;
        }
        m_Shader->OnReloadingEvent.Bind<ColorGradingPass, &ColorGradingPass::OnShaderReloading>(this);

        m_ProgramInstance = New<ShaderProgramInstance>();

        m_GPUConstantBuffer = device->CreateConstantBuffer(sizeof(Data), SE_TEXT("ColorGradingCB"));

        m_PipelineState = device->CreatePipelineState();

        return m_ProgramInstance && m_GPUConstantBuffer && m_PipelineState;
    }

	void ColorGradingPass::Dispose()
	{
        if (m_Shader)
            m_Shader->OnReloadingEvent.Unbind<ColorGradingPass, &ColorGradingPass::OnShaderReloading>(this);

        // Base
        RendererPass::Dispose();

        // Cleanup
        Delete(m_GPUConstantBuffer);
        Delete(m_PipelineState);
        Delete(m_ProgramInstance);
		m_Shader = nullptr;
	}

    void ColorGradingPass::OnShaderReloading(Asset* pAsset)
    {
        DeleteObjectSafe(m_PipelineState);
        DeleteObjectSafe(m_ProgramInstance);
        m_PipelineState   = GPUDevice::instance->CreatePipelineState();
        m_ProgramInstance = New<ShaderProgramInstance>();
        InvalidateResources();
    }

    bool ColorGradingPass::SetupResources()
    {
        if (m_Shader == nullptr)
		{
			return false;
		}

		// Wait for shader
        if (!m_Shader->WaitForLoaded())
		{
			return false;
		}

        return m_ProgramInstance && m_GPUConstantBuffer && m_PipelineState;
    }

    GPUTexture* ColorGradingPass::RenderLUT(RenderContext& renderContext, const Color& screenFadeColor)
    {
		// Ensure to have valid data
		if (CheckIfSkipPass())
		{
            return nullptr;
		}

		PROFILE_GPU_CPU("Color Grading LUT");

        auto&       toneMapping  = renderContext.list->PostSettings.ToneMapping;
        auto&       colorGrading = renderContext.list->PostSettings.ColorGrading;
        const bool  useLut       = colorGrading.LutTexture && colorGrading.LutTexture->IsLoaded() && colorGrading.LutTexture->GetResidentMipLevels() > 0 && colorGrading.LutWeight > Math::ZeroTolerance;
        GPUTexture* ldrLut       = useLut ? colorGrading.LutTexture->GetTexture() : nullptr;
        // A 16^3 unwrapped LDR table stores encoded values as data; an sRGB view would decode while filtering.
        if (useLut && (!ldrLut || !ldrLut->IsAllocated() || ldrLut->IsSRGB() || ldrLut->IsVolume() || ldrLut->IsCubeMap() || ldrLut->IsArray() || ldrLut->IsMultiSample() || ldrLut->Width() != 256 || ldrLut->Height() != 16 ||
                       colorGrading.LutTexture->GetResidentMipLevels() != ldrLut->MipLevels()))
        {
            LOG_ERROR("Render", "LDR color grading requires a fully resident 256x16 LUT with sRGB texture decoding disabled.");
            return nullptr;
        }
        if (!UpdateProgram(toneMapping.Mode, useLut))
        {
            LOG_ERROR("Render", "Cannot prepare the color grading LUT program.");
            return nullptr;
        }

        // A 32^3 LUT uses 32x32 slices or a 1024x32 unwrapped texture.
        const int32           LutSize = 32;
        GPUTextureDescription lutDesc;
#if GPU_ALLOW_GEOMETRY_SHADERS
        if (m_UseVolumeTexture)
        {
            lutDesc = GPUTextureDescription::New3D(LutSize, LutSize, LutSize, 1, m_LutFormat);
        }
		else
#endif
		{
			lutDesc = GPUTextureDescription::New2D(LutSize * LutSize, LutSize, 1, m_LutFormat);
		}
		const auto lut = RenderTargetPool::Get(lutDesc);
        if (!lut)
        {
            return nullptr;
        }
        RENDER_TARGET_POOL_SET_NAME(lut, "ColorGrading.LUT");

		// Prepare the parameters
        Data data            = {};
        data.ScreenFadeColor = screenFadeColor;

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
        data.LutWeight = useLut ? Math::Saturate(colorGrading.LutWeight) : 0.0f;

        // Prepare
		auto device = GPUDevice::instance;
		auto context = device->GetMainContext();

        context->UpdateCB(m_GPUConstantBuffer, &data);

        if (!m_ProgramInstance->SetConstantBuffer(SE_TEXT("data"), m_GPUConstantBuffer) || (useLut && !m_ProgramInstance->SetTexture(SE_TEXT("LutTexture"), ldrLut->View())))
        {
            LOG_ERROR("Render", "Cannot bind the color grading LUT inputs.");
            RenderTargetPool::Release(lut);
            return nullptr;
        }
        context->SetViewportAndScissors((float)lutDesc.Width, (float)lutDesc.Height);
        context->SetState(m_PipelineState);

        // Draw
#if GPU_ALLOW_GEOMETRY_SHADERS
        if (m_UseVolumeTexture)
        {
			context->SetRenderTarget(lut->ViewVolume());

			// Render one fullscreen-triangle per slice intersecting the bounds
			const int32 numInstances = lutDesc.Depth;
            context->DrawFullscreenTriangle(m_ProgramInstance);
            context->DrawInstanced(m_ProgramInstance, 3, numInstances - 1, 1);
        }
		else
#endif
		{
			context->SetRenderTarget(lut->View());
            context->DrawFullscreenTriangle(m_ProgramInstance);
		}

        context->ResetRenderTarget();
        return lut;
	}

    bool ColorGradingPass::UpdateProgram(ToneMappingMode mappingMode, bool useLdrLut)
    {
        String       programName;
        List<String> macros;
#if GPU_ALLOW_GEOMETRY_SHADERS
        if (m_UseVolumeTexture)
        {
            programName = SE_TEXT("Lut3D");
            macros.Add(SE_TEXT("USE_VOLUME_LUT"));
        }
        else
#endif
        {
            programName = SE_TEXT("Lut2D");
        }

        String toneMappingMacro;
        switch (mappingMode)
        {

            case ToneMappingMode::None:
                toneMappingMacro = SE_TEXT("TONE_MAPPING_MODE_NONE");
                break;
            case ToneMappingMode::Neutral:
                toneMappingMacro = SE_TEXT("TONE_MAPPING_MODE_NEUTRAL");
                break;
            case ToneMappingMode::ACES:
                toneMappingMacro = SE_TEXT("TONE_MAPPING_MODE_ACES");
                break;
            default:
                return false;
        }

        macros.Add(toneMappingMacro);
        if (useLdrLut)
        {
            macros.Add(SE_TEXT("USE_LDR_LUT"));
        }
        GPUShaderProgram* program = nullptr;
        if (!m_Shader->SelectProgram(programName, macros, program))
        {
            return false;
        }
        if (m_ProgramInstance->GetProgram() != program || !m_PipelineState->IsValid())
        {
            m_PipelineState->ReleaseGPU();
            if (!m_ProgramInstance->Initialize(program) || !m_PipelineState->Init(GPUPipelineState::Description::DefaultFullscreenTriangle, program))
            {
                return false;
            }
        }
        return true;
    }
} // namespace SE
