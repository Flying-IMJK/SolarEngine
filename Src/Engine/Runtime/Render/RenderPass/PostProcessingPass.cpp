
#include "PostProcessingPass.h"

#include "Runtime/Core/Profiler/Profiler.h"
#include "Runtime/Graphics/GPUContext.h"
#include "Runtime/Graphics/RenderTargetPool.h"
#include "Runtime/Graphics/Shaders/GPUConstantBuffer.h"
#include "Runtime/Graphics/Textures/GPUTexture.h"
#include "Runtime/Render/PostProcessSettings.h"
#include "Runtime/Render/RenderList.h"
#include "Runtime/Resource/Assets/SLC2Materials/GraphicShader.h"
#include "Runtime/Utilities/Time.h"

namespace SE
{
    PostProcessingPass::PostProcessingPass() :
        m_Shader(nullptr), m_PSOThreshold(nullptr), m_PSOScale(nullptr), m_PSOBlurH(nullptr), m_PSOBlurV(nullptr),
        m_PSOGenGhosts(nullptr), m_PSOComposite(nullptr), m_SPThreshold(nullptr), m_SPScale(nullptr),
        m_SPBlurH(nullptr), m_SPBlurV(nullptr), m_SPGenGhosts(nullptr), m_SPComposite(nullptr),
        _defaultLensColor(nullptr), _defaultLensStar(nullptr), _defaultLensDirt(nullptr)
    {}

    String PostProcessingPass::ToString() const { return SE_TEXT("PostProcessingPass"); }

    bool PostProcessingPass::Init()
    {

        // Load shader
        m_Shader = AssetContent::LoadAsyncInternal<GraphicShader>(SE_TEXT("Shaders/PostProcess/PostProcessing"));
        if (m_Shader == nullptr)
        {
            return false;
        }
#if COMPILE_WITH_DEV_ENV
        m_Shader.Get()->OnReloading.Bind<PostProcessingPass, &PostProcessingPass::OnShaderReloading>(this);
#endif

        GPUDevice* device = GPUDevice::instance;

        // Create pipeline states
        m_PSOThreshold = device->CreateSLC2PipelineState();
        m_PSOScale     = device->CreateSLC2PipelineState();
        m_PSOBlurH     = device->CreateSLC2PipelineState();
        m_PSOBlurV     = device->CreateSLC2PipelineState();
        m_PSOGenGhosts = device->CreateSLC2PipelineState();
        m_PSOComposite = device->CreateSLC2PipelineState();

        m_SPThreshold = New<ShaderProgramInstance>();
        m_SPScale     = New<ShaderProgramInstance>();
        m_SPBlurH     = New<ShaderProgramInstance>();
        m_SPBlurV     = New<ShaderProgramInstance>();
        m_SPGenGhosts = New<ShaderProgramInstance>();
        m_SPComposite = New<ShaderProgramInstance>();

        m_DataBuffer         = device->CreateConstantBuffer(sizeof(Data));
        m_GaussianBlurBuffer = device->CreateConstantBuffer(sizeof(GaussianBlurData));

        return true;
    }

    bool PostProcessingPass::SetupResources()
    {
        // Wait for shader
        if (!m_Shader->WaitForLoaded())
        {
            return false;
        }

        // Create pipeline stages
        SLC2GPUPipelineState::Description psDesc = SLC2GPUPipelineState::Description::DefaultFullscreenTriangle;
        SLC2GPUShaderProgram*             program;

        if (!m_Shader->SelectProgram(SE_TEXT("Threshold"), {}, program) || !m_PSOThreshold->Init(psDesc, program))
        {
            return false;
        }

        if (!m_Shader->SelectProgram(SE_TEXT("Scale"), {}, program) || !m_PSOScale->Init(psDesc, program))
        {
            return false;
        }

        if (!m_Shader->SelectProgram(SE_TEXT("GaussainBlurH"), {}, program) || !m_PSOBlurH->Init(psDesc, program))
        {
            return false;
        }

        if (!m_Shader->SelectProgram(SE_TEXT("GaussainBlurV"), {}, program) || !m_PSOBlurV->Init(psDesc, program))
        {
            return false;
        }

        if (!m_Shader->SelectProgram(SE_TEXT("Ghosts"), {}, program) || !m_PSOGenGhosts->Init(psDesc, program))
        {
            return false;
        }

        return true;
    }

    GPUTexture* PostProcessingPass::GetCustomOrDefault(Texture*           customTexture,
                                                       AssetRef<Texture>& defaultTexture,
                                                       const Char*        defaultName)
    {
        // Check if use custom texture
        if (customTexture)
            return customTexture->GetTexture();

        // Check if need to load default texture
        if (defaultTexture == nullptr)
        {
            // Load default
            defaultTexture = AssetContent::LoadAsyncInternal<Texture>(defaultName);
        }

        // Use default texture or nothing
        return defaultTexture ? defaultTexture->GetTexture() : nullptr;
    }

    void PostProcessingPass::GB_ComputeKernel(float sigma, float width, float height)
    {
        float total          = 0.0f;
        float twoSigmaSquare = 2.0f * sigma * sigma;
        float sigmaRoot      = Math::Sqrt(twoSigmaSquare * Math::PI);
        float xOffset        = 1.0f / width;
        float yOffset        = 1.0f / height;

        // Calculate weights and offsets
        for (int32 i = -GB_RADIUS; i <= GB_RADIUS; i++)
        {
            // Calculate pixel distance and index
            const float distance = static_cast<float>(i * i);
            const int32 index    = i + GB_RADIUS;

            // Calculate pixel weight
            const float weight = Math::Exp(-distance / twoSigmaSquare) / sigmaRoot;

            // Calculate total weights sum
            total += weight;

            GaussianBlurCacheH[index] = Float4(weight, i * xOffset, 0, 0);
            GaussianBlurCacheV[index] = Float4(weight, i * yOffset, 0, 0);
        }

        // Normalize weights
        for (int32 i = 0; i < GB_KERNEL_SIZE; i++)
        {
            GaussianBlurCacheH[i].X /= total;
            GaussianBlurCacheV[i].X /= total;
        }

        // Assign size
        _gbData.Size = Float2(width, height);
    }

    void PostProcessingPass::Dispose()
    {
        // Base
        RendererPass::Dispose();

        // Cleanup
        DeleteObjectSafe(m_PSOThreshold);
        DeleteObjectSafe(m_PSOScale);
        DeleteObjectSafe(m_PSOBlurH);
        DeleteObjectSafe(m_PSOBlurV);
        DeleteObjectSafe(m_PSOGenGhosts);
        DeleteObjectSafe(m_PSOComposite);

        DeleteObjectSafe(m_SPThreshold);
        DeleteObjectSafe(m_SPScale);
        DeleteObjectSafe(m_SPBlurH);
        DeleteObjectSafe(m_SPBlurV);
        DeleteObjectSafe(m_SPGenGhosts);
        DeleteObjectSafe(m_SPComposite);
        m_Shader           = nullptr;
        _defaultLensColor = nullptr;
        _defaultLensDirt  = nullptr;
        _defaultLensStar  = nullptr;
    }

    void PostProcessingPass::Render(RenderContext& renderContext,
                                    GPUTexture*    input,
                                    GPUTexture*    output,
                                    GPUTexture*    colorGradingLUT)
    {
        PROFILE_GPU_CPU("Post Processing");
        auto  device  = GPUDevice::instance;
        auto  context = device->GetMainContext();
        auto& view    = renderContext.view;

        context->ResetRenderTarget();

        PostProcessSettings& settings = renderContext.list->PostSettings;
        bool useBloom = false; // EnumHasAnyFlags(view.Flags, ViewFlags::Bloom) && settings.Bloom.Enabled &&
                               // settings.Bloom.Intensity > 0.0f;
        bool useToneMapping = /*EnumHasAnyFlags(view.Flags, ViewFlags::ToneMapping) && */ settings.ToneMapping.Mode !=
                              ToneMappingMode::None;
        bool useCameraArtifacts = false;
            // EnumHasAnyFlags(view.Flags, ViewFlags::CameraArtifacts) &&
            // (settings.CameraArtifacts.VignetteIntensity > 0.0f || settings.CameraArtifacts.GrainAmount > 0.0f
            // || settings.CameraArtifacts.ChromaticDistortion > 0.0f ||
            // settings.CameraArtifacts.ScreenFadeColor.A > 0.0f);
        bool useLensFlares = false; // EnumHasAnyFlags(view.Flags, ViewFlags::LensFlares) &&
                                    // settings.LensFlares.Intensity > 0.0f && useBloom;

        // Cache viewport sizes
        int32 w1 = input->Width();
        int32 w2 = w1 >> 1;
        int32 w4 = w2 >> 1;
        int32 w8 = w4 >> 1;
        int32 h1 = input->Height();
        int32 h2 = h1 >> 1;
        int32 h4 = h2 >> 1;
        int32 h8 = h4 >> 1;

        // Ensure to have valid data and if at least one effect should be applied
        if (!(useBloom || useToneMapping || useCameraArtifacts) || CheckIfSkipPass() || w8 == 0 || h8 == 0)
        {
            // Resources are missing. Do not perform rendering. Just copy raw frame
            context->SetViewportAndScissors((float)output->Width(), (float)output->Height());
            context->SetRenderTarget(*output);
            context->Draw(input);
            return;
        }


        ////////////////////////////////////////////////////////////////////////////////////
        // Setup shader

        Data  data;
        float time = Time::Render.UnscaledTime.GetTotalSeconds();
        data.Time  = Math::Fractional(time);
        if (useCameraArtifacts)
        {
            /*data.VignetteColor = settings.CameraArtifacts.VignetteColor;
            data.VignetteIntensity = settings.CameraArtifacts.VignetteIntensity;
            data.VignetteShapeFactor = settings.CameraArtifacts.VignetteShapeFactor;
            data.GrainAmount = settings.CameraArtifacts.GrainAmount;
            data.GrainParticleSize = Math::Max(0.0001f, settings.CameraArtifacts.GrainParticleSize);
            data.GrainTime = time * 0.5f * settings.CameraArtifacts.GrainSpeed;
            data.ChromaticDistortion = Math::Saturate(settings.CameraArtifacts.ChromaticDistortion);
            data.ScreenFadeColor = settings.CameraArtifacts.ScreenFadeColor;*/
        }
        else
        {
            data.VignetteIntensity   = 0;
            data.GrainAmount         = 0;
            data.ChromaticDistortion = 0;
            data.ScreenFadeColor     = Colors::Transparent;
        }

        if (useBloom)
        {
            /*data.BloomMagnitude = settings.Bloom.Intensity;
            data.BloomThreshold = settings.Bloom.Threshold;
            data.BloomBlurSigma = Math::Max(settings.Bloom.BlurSigma, 0.0001f);
            data.BloomLimit = settings.Bloom.Limit;*/
        }
        else
        {
            data.BloomMagnitude = 0;
        }

        if (useLensFlares)
        {
            /*data.LensFlareIntensity = settings.LensFlares.Intensity;
            data.LensDirtIntensity = settings.LensFlares.LensDirtIntensity;
            data.Ghosts = settings.LensFlares.Ghosts;

            data.HaloWidth = settings.LensFlares.HaloWidth;
            data.HaloIntensity = settings.LensFlares.HaloIntensity;
            data.Distortion = settings.LensFlares.Distortion;
            data.GhostDispersal = settings.LensFlares.GhostDispersal;

            data.LensBias = settings.LensFlares.ThresholdBias;
            data.LensScale = settings.LensFlares.ThresholdScale;
            data.LensInputDistortion = Float2(-(1.0f / w4) * settings.LensFlares.Distortion, (1.0f / w4) *
            settings.LensFlares.Distortion);

            // Calculate star texture rotation matrix
            Float3 camX = renderContext.View.View.GetRight();
            Float3 camZ = renderContext.View.View.GetForward();
            float camRot = Float3::Dot(camX, Float3::Forward) + Float3::Dot(camZ, Float3::Up);
            float camRotCos = Math::Cos(camRot) * 0.8f;
            float camRotSin = Math::Sin(camRot) * 0.8f;
            Matrix rotation(
                camRotCos, -camRotSin, 0.0f, 0.0f,
                camRotSin, camRotCos, 0.0f, 0.0f,
                0.0f, 0.0f, 1.0f, 0.0f,
                0.0f, 0.0f, 0.01f, 1.0f
            );
            data.LensFlareStarMat = rotation;*/
        }
        else
        {
            data.LensFlareIntensity = 0;
            data.LensDirtIntensity  = 0;
        }
        data.PostExposure = Math::Exp2(8 /*settings.EyeAdaptation.PostExposure*/);
        data.InputSize    = Float2(static_cast<float>(w1), static_cast<float>(h1));
        data.InvInputSize = Float2(1.0f / static_cast<float>(w1), 1.0f / static_cast<float>(h1));
        data.InputAspect  = static_cast<float>(w1) / h1;
        context->UpdateCB(m_DataBuffer, &data);

        ////////////////////////////////////////////////////////////////////////////////////
        // Bloom

        /*auto tempDesc = GPUTextureDescription::New2D(w2, h2, 0, output->Format(), GPUTextureFlags::ShaderResource |
        GPUTextureFlags::RenderTarget | GPUTextureFlags::PerMipViews); auto bloomTmp1 = RenderTargetPool::Get(tempDesc);
        RENDER_TARGET_POOL_SET_NAME(bloomTmp1, "PostProcessing.Bloom");
        // TODO: bloomTmp2 could be quarter res because we don't use it's first mip
        auto bloomTmp2 = RenderTargetPool::Get(tempDesc);
        RENDER_TARGET_POOL_SET_NAME(bloomTmp2, "PostProcessing.Bloom");

        // Check if use bloom
        if (useBloom)
        {
            // Bloom Threshold and downscale to 1/2
            context->SetRenderTarget(bloomTmp1->View(0, 0));
            context->SetViewportAndScissors((float)w2, (float)h2);
            context->BindSR(0, input->View());
            context->SetState(_psThreshold);
            context->DrawFullscreenTriangle();
            context->ResetRenderTarget();

            // Downscale to 1/4
            context->SetRenderTarget(bloomTmp1->View(0, 1));
            context->SetViewportAndScissors((float)w4, (float)h4);
            context->BindSR(0, bloomTmp1->View(0, 0));
            context->SetState(_psScale);
            context->DrawFullscreenTriangle();
            context->ResetRenderTarget();

            // Downscale to 1/8
            context->SetRenderTarget(bloomTmp1->View(0, 2));
            context->SetViewportAndScissors((float)w8, (float)h8);
            context->BindSR(0, bloomTmp1->View(0, 1));
            context->SetState(_psScale);
            context->DrawFullscreenTriangle();
            context->ResetRenderTarget();

            // TODO: perform blur when downscaling (13 tap) and when upscaling? (9 tap)

            // Gaussian Blur
            GB_ComputeKernel(data.BloomBlurSigma, static_cast<float>(w8), static_cast<float>(h8));
            //int32 blurStages = (int)Rendering.Quality + 1;
            int32 blurStages = 2;
            for (int32 i = 0; i < blurStages; i++)
            {
                // Horizontal Bloom Blur
                Platform::MemoryCopy(_gbData.GaussianBlurCache, GaussianBlurCacheH, sizeof(GaussianBlurCacheH));
                context->UpdateCB(cb1, &_gbData);
                context->BindCB(1, cb1);
                //
                context->SetRenderTarget(bloomTmp2->View(0, 2));
                context->BindSR(0, bloomTmp1->View(0, 2));
                context->SetState(_psBlurH);
                context->DrawFullscreenTriangle();
                context->ResetRenderTarget();

                // Vertical Bloom Blur
                Platform::MemoryCopy(_gbData.GaussianBlurCache, GaussianBlurCacheV, sizeof(GaussianBlurCacheV));
                context->UpdateCB(cb1, &_gbData);
                context->BindCB(1, cb1);
                //
                context->SetRenderTarget(bloomTmp1->View(0, 2));
                context->BindSR(0, bloomTmp2->View(0, 2));
                context->SetState(_psBlurV);
                context->DrawFullscreenTriangle();
                context->ResetRenderTarget();
            }

            // Upscale to 1/4 (use second tmp target to cache that downscale thress data for lens flares)
            context->SetRenderTarget(bloomTmp2->View(0, 1));
            context->SetViewportAndScissors((float)w4, (float)h4);
            context->BindSR(0, bloomTmp1->View(0, 2));
            context->SetState(_psScale);
            context->DrawFullscreenTriangle();
            context->ResetRenderTarget();

            // Upscale to 1/2
            context->SetRenderTarget(bloomTmp1->View(0, 0));
            context->SetViewportAndScissors((float)w2, (float)h2);
            context->BindSR(0, bloomTmp2->View(0, 1));
            context->SetState(_psScale);
            context->DrawFullscreenTriangle();
            context->ResetRenderTarget();

            // Set bloom
            context->UnBindSR(0);
            context->BindSR(2, bloomTmp1->View(0, 0));
        }
        else
        {
            // No bloom texture
            context->UnBindSR(2);
        }*/

        ////////////////////////////////////////////////////////////////////////////////////
        // Lens Flares

        /*
        // Check if use lens flares
        if (useLensFlares)
        {
            // Prepare lens flares helper textures
            context->BindSR(5, getCustomOrDefault(settings.LensFlares.LensStar, _defaultLensStar,
        TEXT("Engine/Textures/DefaultLensStarburst"))); context->BindSR(6,
        getCustomOrDefault(settings.LensFlares.LensColor, _defaultLensColor, TEXT("Engine/Textures/DefaultLensColor")));

            // Render lens flares
            context->SetRenderTarget(bloomTmp2->View(0, 1));
            context->SetViewportAndScissors((float)w4, (float)h4);
            context->BindSR(3, bloomTmp1->View(0, 1));
            context->SetState(_psGenGhosts);
            context->DrawFullscreenTriangle();
            context->ResetRenderTarget();
            context->UnBindSR(3);

            // Gaussian blur kernel
            GB_ComputeKernel(2.0f, static_cast<float>(w4), static_cast<float>(h4));

            // Gaussian blur H
            Platform::MemoryCopy(_gbData.GaussianBlurCache, GaussianBlurCacheH, sizeof(GaussianBlurCacheH));
            context->UpdateCB(cb1, &_gbData);
            context->BindCB(1, cb1);
            context->SetRenderTarget(bloomTmp1->View(0, 1));
            context->BindSR(0, bloomTmp2->View(0, 1));
            context->SetState(_psBlurH);
            context->DrawFullscreenTriangle();
            context->ResetRenderTarget();

            // Gaussian blur V
            Platform::MemoryCopy(_gbData.GaussianBlurCache, GaussianBlurCacheV, sizeof(GaussianBlurCacheV));
            context->UpdateCB(cb1, &_gbData);
            context->BindCB(1, cb1);
            context->SetRenderTarget(bloomTmp2->View(0, 1));
            context->BindSR(0, bloomTmp1->View(0, 1));
            context->SetState(_psBlurV);
            context->DrawFullscreenTriangle();
            context->ResetRenderTarget();

            // Set lens flares output
            context->BindSR(3, bloomTmp2->View(0, 1));
        }
        else
        {
            context->BindSR(3, (GPUResourceView*)nullptr);
        }*/

        ////////////////////////////////////////////////////////////////////////////////////
        // Final composite

        // TODO: consider to use more compute shader for post processing

        // TODO: maybe don't use this rt swap and start using GetTempRt to make this design easier

        // Check if use Tone Mapping + Color Grading LUT
        List<String>    macro;
        int32           compositePermutationIndex = 0;
        GPUTextureView* colorGradingLutView       = nullptr;
        if (colorGradingLUT)
        {
            macro.Add(SE_TEXT("USE_GRADING_LUT"));

            if (colorGradingLUT->IsVolume())
            {
                macro.Add(SE_TEXT("USE_3D_LUT"));
                colorGradingLutView = colorGradingLUT->ViewVolume();
            }
            else
            {
                colorGradingLutView = colorGradingLUT->View();
            }
        }


        bool validComposite = false;
        if (m_Shader->SelectProgram(SE_TEXT("Composite"), macro, m_SPComposite) &&
            m_PSOComposite->Init(SLC2GPUPipelineState::Description::DefaultFullscreenTriangle, m_SPComposite->GetProgram()))
        {
            validComposite = true;
        }

        if (validComposite)
        {
            m_SPComposite->SetConstantBuffer(SE_TEXT("data"), m_DataBuffer);

            // Composite pass inputs mapping:
            // - 0 - Input0   - scene color
            // - 1 - Input1   - <unused>
            // - 2 - Input2   - bloom
            // - 3 - Input3   - lens flare color
            // - 4 - LensDirt - lens dirt texture
            // - 5 - LensStar - lens star texture
            // - 7 - ColorGradingLUT
            m_SPComposite->SetTexture(SE_TEXT("Input0"), input->View());
            // context->BindSR(4, GetCustomOrDefault(settings.LensFlares.LensDirt, _defaultLensDirt,
            // SE_TEXT("Engine/Textures/DefaultLensDirt")));
            if (colorGradingLUT)
            {
                m_SPComposite->SetTexture(SE_TEXT("ColorGradingLUT"), colorGradingLutView);
            }

            // Composite final frame during single pass (done in full resolution)
            context->SetViewportAndScissors((float)output->Width(), (float)output->Height());
            context->SetRenderTarget(*output);
            context->SetSLC2State(m_PSOComposite);
            context->DrawFullscreenTriangle(m_SPComposite);
        }


        // Cleanup
        /*RenderTargetPool::Release(bloomTmp1);
        RenderTargetPool::Release(bloomTmp2);*/
    }

} // namespace SE
