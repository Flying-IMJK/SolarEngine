
#include "ShadowPass.h"

#include "GBufferPass.h"
#include "Core/Profiler/Profiler.h"
#include "Core/Profiler/ProfilerCPU.h"
#include "Runtime/Graphics/GPUContext.h"
#include "Runtime/Graphics/Shaders/GPUConstantBuffer.h"
#include "Runtime/Graphics/Textures/GPUTexture.h"
#include "Runtime/Graphics/Textures/GPUTextureDescription.h"
#include "Runtime/Render/RenderBuffers.h"
#include "Runtime/Render/RenderTask.h"

namespace SE
{
#define NormalOffsetScaleTweak 100.0f
#define SpotLight_NearPlane 10.0f
#define PointLight_NearPlane 0.1f

    // Maximum amount of directional light cascades (using CSM technique)
#define MAX_CSM_CASCADES 4

    GES_PACK_STRUCT(struct Data{
        GBufferData GBuffer;
        LightData Light;
        LightShadowData LightShadow;
        Matrix WVP;
        Matrix ViewProjectionMatrix;
        Float2 Dummy0;
        float ContactShadowsDistance;
        float ContactShadowsLength;
        });

    ShadowsPass::ShadowsPass()
        : _shader(nullptr)
        , _shadowMapsSizeCSM(0)
        , _shadowMapsSizeCube(0)
        , _shadowMapCSM(nullptr)
        , _shadowMapCube(nullptr)
        , _currentShadowMapsQuality((Quality)((int32)Quality::Ultra + 1))
        , _sphereModel(nullptr)
        , maxShadowsQuality(0)
    {
    }

    uint64 ShadowsPass::GetShadowMapsMemoryUsage() const
    {
        uint64 result = 0;

        if (_shadowMapCSM)
            result += _shadowMapCSM->GetMemoryUsage();
        if (_shadowMapCube)
            result += _shadowMapCube->GetMemoryUsage();

        return result;
    }

    String ShadowsPass::ToString() const
    {
        return SE_TEXT("ShadowsPass");
    }

    bool ShadowsPass::Init()
    {
        // Create pipeline states
        _psShadowDir.CreatePipelineStates();
        _psShadowPoint.CreatePipelineStates();
        _psShadowSpot.CreatePipelineStates();

        // Load assets
        _shader = AssetContent::LoadAsyncInternal<Shader>(SE_TEXT("Shaders/Shadows"));
        _sphereModel = AssetContent::LoadAsyncInternal<Model>(SE_TEXT("Assets/Models/Sphere"));
        if (_shader == nullptr || _sphereModel == nullptr)
        {
            return false;
        }

        // Create shadow maps
        _shadowMapCSM = GPUDevice::instance->CreateTexture(SE_TEXT("Shadow Map CSM"));
        _shadowMapCube = GPUDevice::instance->CreateTexture(SE_TEXT("Shadow Map Cube"));

#if COMPILE_WITH_DEV_ENV
        _shader.Get()->OnReloading.Bind<ShadowsPass, &ShadowsPass::OnShaderReloading>(this);
#endif

        // Select format for shadow maps
        _shadowMapFormat = PixelFormat::Undefined;
#if !PLATFORM_SWITCH // TODO: fix shadows performance issue on Switch
        for (const PixelFormat format : { PixelFormat::D16_UNorm, PixelFormat::D24_UNorm_S8_UInt, PixelFormat::D32_Float })
        {
            const auto formatFeaturesDepth = GPUDevice::instance->GetPixelFormatFeatures(format);
            const auto formatFeaturesTexture = GPUDevice::instance->GetPixelFormatFeatures(format);
            if (formatFeaturesDepth.Support.AllFlagsSet(FormatSupport::DepthStencil, FormatSupport::Texture2D, FormatSupport::TextureCube) &&
                formatFeaturesTexture.Support.AllFlagsSet(FormatSupport::ShaderSample, FormatSupport::ShaderSampleComparison))
            {
                _shadowMapFormat = format;
                break;
            }
        }
#endif
        if (_shadowMapFormat == PixelFormat::Undefined)
        {
            LOG_WARNING("Render", "GPU doesn't support shadows rendering");
        }

        return true;
    }

    bool ShadowsPass::SetupResources()
    {
        _sphereModel->WaitForLoaded();
        _shader->WaitForLoaded();

        // Wait for the assets
        if (!_sphereModel->CanBeRendered() || !_shader->IsLoaded())
        {
            return false;
        }

        auto shader = _shader->GetShader();

        // Validate shader constant buffers sizes
        if (shader->GetCB(0)->GetSize() != sizeof(Data))
        {
            REPORT_INVALID_SHADER_PASS_CB_SIZE(shader, 0, Data);
            return false;
        }

        // Create pipeline stages
        GPUPipelineState::Description psDesc;
        if (!_psShadowPoint.IsValid())
        {
            psDesc = GPUPipelineState::Description::DefaultNoDepth;
            psDesc.CullMode = CullMode::TwoSided;
            psDesc.VS = shader->GetVS(SE_TEXT("VS_Model"));
            if (!_psShadowPoint.Create(psDesc, shader, SE_TEXT("PS_PointLight")))
                return false;
        }
        if (!_psShadowDir.IsValid())
        {
            psDesc = GPUPipelineState::Description::DefaultFullscreenTriangle;
            if (!_psShadowDir.Create(psDesc, shader, SE_TEXT("PS_DirLight")))
                return false;
        }
        if (!_psShadowSpot.IsValid())
        {
            psDesc = GPUPipelineState::Description::DefaultNoDepth;
            psDesc.CullMode = CullMode::TwoSided;
            psDesc.VS = shader->GetVS(SE_TEXT("VS_Model"));
            if (!_psShadowSpot.Create(psDesc, shader, SE_TEXT("PS_SpotLight")))
                return false;
        }

        return true;
    }

    void ShadowsPass::UpdateShadowMapSize()
    {
        // Temporary data
        int32 newSizeCSM = 0;
        int32 newSizeCube = 0;

        // Select new size
        _currentShadowMapsQuality = Quality::Ultra;
        if (_shadowMapFormat != PixelFormat::Undefined)
        {
            switch (_currentShadowMapsQuality)
            {
            case Quality::Ultra:
                newSizeCSM = 2048;
                newSizeCube = 1024;
                break;
            case Quality::High:
                newSizeCSM = 1024;
                newSizeCube = 1024;
                break;
            case Quality::Medium:
                newSizeCSM = 1024;
                newSizeCube = 512;
                break;
            case Quality::Low:
                newSizeCSM = 512;
                newSizeCube = 256;
                break;
            }
        }

        // Check if size will change
        if (newSizeCSM > 0 && newSizeCSM != _shadowMapsSizeCSM)
        {
            if (!_shadowMapCSM->Init(GPUTextureDescription::New2D(newSizeCSM, newSizeCSM, _shadowMapFormat, {GPUTextureFlags::ShaderResource, GPUTextureFlags::DepthStencil}, 1, MAX_CSM_CASCADES)))
            {
                LOG_FATAL("Render", "Cannot setup shadow map '{0}' Size: {1}, format: {2}.", SE_TEXT("CSM"), newSizeCSM, Types::GetEnumString(_shadowMapFormat));
                return;
            }
            _shadowMapsSizeCSM = newSizeCSM;
        }
        if (newSizeCube > 0 && newSizeCube != _shadowMapsSizeCube)
        {
            if (!_shadowMapCube->Init(GPUTextureDescription::NewCube(newSizeCube, _shadowMapFormat, {GPUTextureFlags::ShaderResource, GPUTextureFlags::DepthStencil})))
            {
                LOG_FATAL("Render", "Cannot setup shadow map '{0}' Size: {1}, format: {2}.", SE_TEXT("Cube"), newSizeCube, Types::GetEnumString(_shadowMapFormat));
                return;
            }
            _shadowMapsSizeCube = newSizeCube;
        }
    }

    void ShadowsPass::SetupRenderContext(RenderContext& renderContext, RenderContext& shadowContext)
    {
        const auto& view = renderContext.view;

        // Use the current render view to sync model LODs with the shadow maps rendering stage
        shadowContext.lodProxyView = &renderContext.view;

        // Prepare properties
        auto& shadowView = shadowContext.view;
        // shadowView.Flags = view.flags;
        shadowView.StaticFlagsMask = view.StaticFlagsMask;
        shadowView.RenderLayersMask = view.RenderLayersMask;
        shadowView.IsOfflinePass = view.IsOfflinePass;
        shadowView.ModelLODBias = view.ModelLODBias;
        shadowView.ModelLODDistanceFactor = view.ModelLODDistanceFactor;
        shadowView.Pass = DrawPass::Depth;
        shadowView.Origin = view.Origin;
        shadowContext.list = RenderList::GetFromPool();
        shadowContext.buffers = renderContext.buffers;
        shadowContext.task = renderContext.task;
        shadowContext.gpuContext = GPUDevice::instance->GetMainContext();
    }

    void ShadowsPass::SetupLight(RenderContext& renderContext, RenderContextBatch& renderContextBatch, RendererDirectionalLightData& light)
    {
        const RenderView& view = renderContext.view;
        auto mainCache = renderContext.list;
        Float3 lightDirection = light.Direction;
        float shadowsDistance = Math::Min(view.Far, light.ShadowsDistance);
        int32 csmCount = Math::Clamp(light.CascadeCount, 0, MAX_CSM_CASCADES);
        bool blendCSM = true;//Graphics::AllowCSMBlending;
        const auto shadowMapsSizeCSM = (float)_shadowMapsSizeCSM;
#if SE_EDITOR
        /*if (IsRunningRadiancePass)
            blendCSM = false;*/
#elif PLATFORM_SWITCH || PLATFORM_IOS || PLATFORM_ANDROID
        // Disable cascades blending on low-end platforms
        blendCSM = false;
#endif

        // Views with orthographic cameras cannot use cascades, we force it to 1 shadow map here
        if (view.Projection.M44 == 1.0f)
            csmCount = 1;

        // Calculate cascade splits
        auto cameraNear = view.Near;
        auto cameraFar = view.Far;
        auto cameraRange = cameraFar - cameraNear;
        float minDistance;
        float maxDistance;
        float cascadeSplits[MAX_CSM_CASCADES];
        {
            // TODO: use HiZ and get view min/max range to fit cascades better
            minDistance = cameraNear;
            maxDistance = cameraNear + shadowsDistance;

            PartitionMode partitionMode = light.PartitionMode;
            float pssmFactor = 0.5f;
            float splitDistance0 = light.Cascade1Spacing;
            float splitDistance1 = Math::Max(splitDistance0, light.Cascade2Spacing);
            float splitDistance2 = Math::Max(splitDistance1, light.Cascade3Spacing);
            float splitDistance3 = Math::Max(splitDistance2, light.Cascade4Spacing);

            // Compute the split distances based on the partitioning mode
            if (partitionMode == PartitionMode::Manual)
            {
                if (csmCount == 1)
                {
                    cascadeSplits[0] = minDistance + splitDistance3 * maxDistance;
                }
                else if (csmCount == 2)
                {
                    cascadeSplits[0] = minDistance + splitDistance1 * maxDistance;
                    cascadeSplits[1] = minDistance + splitDistance3 * maxDistance;
                }
                else if (csmCount == 3)
                {
                    cascadeSplits[0] = minDistance + splitDistance1 * maxDistance;
                    cascadeSplits[1] = minDistance + splitDistance2 * maxDistance;
                    cascadeSplits[2] = minDistance + splitDistance3 * maxDistance;
                }
                else if (csmCount == 4)
                {
                    cascadeSplits[0] = minDistance + splitDistance0 * maxDistance;
                    cascadeSplits[1] = minDistance + splitDistance1 * maxDistance;
                    cascadeSplits[2] = minDistance + splitDistance2 * maxDistance;
                    cascadeSplits[3] = minDistance + splitDistance3 * maxDistance;
                }
            }
            else if (partitionMode == PartitionMode::Logarithmic || partitionMode == PartitionMode::PSSM)
            {
                const float lambda = partitionMode == PartitionMode::PSSM ? pssmFactor : 1.0f;

                const auto range = maxDistance - minDistance;
                const auto ratio = maxDistance / minDistance;
                const auto logRatio = Math::Clamp(1.0f - lambda, 0.0f, 1.0f);
                for (int32 cascadeLevel = 0; cascadeLevel < csmCount; cascadeLevel++)
                {
                    // Compute cascade split (between znear and zfar)
                    const float distribute = static_cast<float>(cascadeLevel + 1) / csmCount;
                    float logZ = static_cast<float>(minDistance * powf(ratio, distribute));
                    float uniformZ = minDistance + range * distribute;
                    cascadeSplits[cascadeLevel] = Math::Lerp(uniformZ, logZ, logRatio);
                }
            }

            // Convert distance splits to ratios cascade in the range [0, 1]
            for (int32 i = 0; i < MAX_CSM_CASCADES; i++)
            {
                cascadeSplits[i] = (cascadeSplits[i] - cameraNear) / cameraRange;
            }
        }

        // Select best Up vector
        Float3 side = Float3::UnitX;
        Float3 upDirection = Float3::UnitX;
        Float3 vectorUps[] = { Float3::UnitY, Float3::UnitX, Float3::UnitZ };
        for (int32 i = 0; i < ARRAY_SIZE(vectorUps); i++)
        {
            const Float3 vectorUp = vectorUps[i];
            if (Math::Abs(Float3::Dot(lightDirection, vectorUp)) < (1.0f - 0.0001f))
            {
                side = Float3::Normalize(Float3::Cross(vectorUp, lightDirection));
                upDirection = Float3::Normalize(Float3::Cross(lightDirection, side));
                break;
            }
        }

        // Temporary data
        Float3 frustumCorners[8];
        Matrix shadowView, shadowProjection, shadowVP;

        // Init shadow data
        light.ShadowDataIndex = _shadowData.Count();
        auto& shadowData = _shadowData.AddOne();
        shadowData.ContextIndex = renderContextBatch.Contexts.Count();
        shadowData.ContextCount = csmCount;
        shadowData.BlendCSM = blendCSM;
        renderContextBatch.Contexts.AddDefault(shadowData.ContextCount);

        // Create the different view and projection matrices for each split
        float splitMinRatio = 0;
        float splitMaxRatio = (minDistance - cameraNear) / cameraRange;
        for (int32 cascadeIndex = 0; cascadeIndex < csmCount; cascadeIndex++)
        {
            // Cascade splits
            const auto oldSplitMinRatio = splitMinRatio;
            splitMinRatio = splitMaxRatio;
            splitMaxRatio = cascadeSplits[cascadeIndex];

            // Calculate cascade split frustum corners in view space
            for (int32 j = 0; j < 4; j++)
            {
                float overlap = 0;
                if (blendCSM)
                {
                    overlap = 0.2f * (splitMinRatio - oldSplitMinRatio);
                }
                const auto frustumRangeVS = mainCache->FrustumCornersVs[j + 4] - mainCache->FrustumCornersVs[j];
                frustumCorners[j] = mainCache->FrustumCornersVs[j] + frustumRangeVS * (splitMinRatio - overlap);
                frustumCorners[j + 4] = mainCache->FrustumCornersVs[j] + frustumRangeVS * splitMaxRatio;
            }

            // Perform stabilization
            enum StabilizationMode
            {
                None,
                ProjectionSnapping,
                ViewSnapping,
            };
            const StabilizationMode stabilization = ViewSnapping; // TODO: expose to graphics settings maybe
            Float3 cascadeMinBoundLS;
            Float3 cascadeMaxBoundLS;
            Float3 target;
            {
                // Make sure we are using the same direction when stabilizing
                BoundingSphere boundingVS;
                BoundingSphere::FromPoints(frustumCorners, ARRAY_SIZE(frustumCorners), boundingVS);

                // Compute bounding box center
                Float3::TransformCoordinate(boundingVS.Center, view.IV, target);
                float boundingVSRadius = (float)boundingVS.Radius;
                cascadeMaxBoundLS = Float3(boundingVSRadius);
                cascadeMinBoundLS = -cascadeMaxBoundLS;

                if (stabilization == ViewSnapping)
                {
                    // Snap the target to the texel units (reference: ShaderX7 - Practical Cascaded Shadows Maps)
                    float shadowMapHalfSize = shadowMapsSizeCSM * 0.5f;
                    float x = Math::Ceil(Float3::Dot(target, upDirection) * shadowMapHalfSize / boundingVSRadius) * boundingVSRadius / shadowMapHalfSize;
                    float y = Math::Ceil(Float3::Dot(target, side) * shadowMapHalfSize / boundingVSRadius) * boundingVSRadius / shadowMapHalfSize;
                    float z = Float3::Dot(target, lightDirection);
                    target = upDirection * x + side * y + lightDirection * z;
                }
            }

            const auto nearClip = 0.0f;
            const auto farClip = cascadeMaxBoundLS.z - cascadeMinBoundLS.z;

            // Create shadow view matrix
            Matrix::LookAt(target - lightDirection * cascadeMaxBoundLS.z, target, upDirection, shadowView);

            // Create viewport for culling with extended near/far planes due to culling issues
            Matrix cullingVP;
            {
                const float cullRangeExtent = 100000.0f;
                Matrix::OrthoOffCenter(cascadeMinBoundLS.x, cascadeMaxBoundLS.x, cascadeMinBoundLS.y, cascadeMaxBoundLS.y, -cullRangeExtent, farClip + cullRangeExtent, shadowProjection);
                Matrix::Multiply(shadowView, shadowProjection, cullingVP);
            }

            // Create shadow projection matrix
            Matrix::OrthoOffCenter(cascadeMinBoundLS.x, cascadeMaxBoundLS.x, cascadeMinBoundLS.y, cascadeMaxBoundLS.y, nearClip, farClip, shadowProjection);

            // Construct shadow matrix (View * Projection)
            Matrix::Multiply(shadowView, shadowProjection, shadowVP);

            // Stabilize the shadow matrix on the projection
            if (stabilization == ProjectionSnapping)
            {
                Float3 shadowPixelPosition = shadowVP.GetTranslation() * (shadowMapsSizeCSM * 0.5f);
                shadowPixelPosition.z = 0;
                const Float3 shadowPixelPositionRounded(Math::Round(shadowPixelPosition.x), Math::Round(shadowPixelPosition.y), 0.0f);
                const Float4 shadowPixelOffset((shadowPixelPositionRounded - shadowPixelPosition) * (2.0f / shadowMapsSizeCSM), 0.0f);
                shadowProjection.SetRow(4, shadowProjection.GetRow(4) + shadowPixelOffset);
                Matrix::Multiply(shadowView, shadowProjection, shadowVP);
            }

            // Transform NDC space [-1,+1]^2 to texture space [0,1]^2
            {
                const Matrix T(
                    0.5f, 0.0f, 0.0f, 0.0f,
                    0.0f, -0.5f, 0.0f, 0.0f,
                    0.0f, 0.0f, 1.0f, 0.0f,
                    0.5f, 0.5f, 0.0f, 1.0f);
                Matrix m;
                Matrix::Multiply(shadowVP, T, m);
                Matrix::Transpose(m, shadowData.Constants.ShadowVP[cascadeIndex]);
            }

            // Setup context for cascade
            auto& shadowContext = renderContextBatch.Contexts[shadowData.ContextIndex + cascadeIndex];
            SetupRenderContext(renderContext, shadowContext);
            shadowContext.list->Clear();
            shadowContext.view.Position = -lightDirection * shadowsDistance + view.Position;
            shadowContext.view.Direction = lightDirection;
            shadowContext.view.SetUp(shadowView, shadowProjection);
            shadowContext.view.cullingFrustum.SetMatrix(cullingVP);
            shadowContext.view.PrepareCache(shadowContext, shadowMapsSizeCSM, shadowMapsSizeCSM, Float2::Zero, &view);
        }

        // Setup constant buffer data
        shadowData.Constants.ShadowMapSize = shadowMapsSizeCSM;
        shadowData.Constants.Sharpness = light.ShadowsSharpness;
        shadowData.Constants.Fade = Math::Saturate(light.ShadowsStrength);
        shadowData.Constants.NormalOffsetScale = light.ShadowsNormalOffsetScale * NormalOffsetScaleTweak * (1.0f / shadowMapsSizeCSM);
        shadowData.Constants.Bias = light.ShadowsDepthBias;
        shadowData.Constants.FadeDistance = Math::Max(light.ShadowsFadeDistance, 0.1f);
        shadowData.Constants.NumCascades = csmCount;
        shadowData.Constants.CascadeSplits = view.Near + Float4(cascadeSplits) * cameraRange;
    }

    void ShadowsPass::SetupLight(RenderContext& renderContext, RenderContextBatch& renderContextBatch, RendererPointLightData& light)
    {
        // Init shadow data
        light.ShadowDataIndex = _shadowData.Count();
        auto& shadowData = _shadowData.AddOne();
        shadowData.ContextIndex = renderContextBatch.Contexts.Count();
        shadowData.ContextCount = 6;
        renderContextBatch.Contexts.AddDefault(shadowData.ContextCount);

        const auto& view = renderContext.view;
        const auto shadowMapsSizeCube = (float)_shadowMapsSizeCube;

        // Fade shadow on distance
        const float fadeDistance = Math::Max(light.ShadowsFadeDistance, 0.1f);
        const float dstLightToView = Float3::Distance(light.Position, view.Position);
        const float fade = 1 - Math::Saturate((dstLightToView - light.Radius - light.ShadowsDistance + fadeDistance) / fadeDistance);

        // Render depth to all 6 faces of the cube map
        for (int32 faceIndex = 0; faceIndex < 6; faceIndex++)
        {
            auto& shadowContext = renderContextBatch.Contexts[shadowData.ContextIndex + faceIndex];
            SetupRenderContext(renderContext, shadowContext);
            shadowContext.list->Clear();
            shadowContext.view.SetUpCube(PointLight_NearPlane, light.Radius, light.Position);
            shadowContext.view.SetFace(faceIndex);
            shadowContext.view.PrepareCache(shadowContext, shadowMapsSizeCube, shadowMapsSizeCube, Float2::Zero, &view);
            Matrix::Transpose(shadowContext.view.ViewProjection(), shadowData.Constants.ShadowVP[faceIndex]);
        }

        // Setup constant buffer data
        shadowData.Constants.ShadowMapSize = shadowMapsSizeCube;
        shadowData.Constants.Sharpness = light.ShadowsSharpness;
        shadowData.Constants.Fade = Math::Saturate(light.ShadowsStrength * fade);
        shadowData.Constants.NormalOffsetScale = light.ShadowsNormalOffsetScale * NormalOffsetScaleTweak * (1.0f / shadowMapsSizeCube);
        shadowData.Constants.Bias = light.ShadowsDepthBias;
        shadowData.Constants.FadeDistance = Math::Max(light.ShadowsFadeDistance, 0.1f);
        shadowData.Constants.NumCascades = 1;
        shadowData.Constants.CascadeSplits = Float4::Zero;
    }

    void ShadowsPass::SetupLight(RenderContext& renderContext, RenderContextBatch& renderContextBatch, RendererSpotLightData& light)
    {
        // Init shadow data
        light.ShadowDataIndex = _shadowData.Count();
        auto& shadowData = _shadowData.AddOne();
        shadowData.ContextIndex = renderContextBatch.Contexts.Count();
        shadowData.ContextCount = 1;
        renderContextBatch.Contexts.AddDefault(shadowData.ContextCount);

        const auto& view = renderContext.view;
        const auto shadowMapsSizeCube = (float)_shadowMapsSizeCube;

        // Fade shadow on distance
        const float fadeDistance = Math::Max(light.ShadowsFadeDistance, 0.1f);
        const float dstLightToView = Float3::Distance(light.Position, view.Position);
        const float fade = 1 - Math::Saturate((dstLightToView - light.Radius - light.ShadowsDistance + fadeDistance) / fadeDistance);

        // Render depth to all 1 face of the cube map
        constexpr int32 faceIndex = 0;
        {
            auto& shadowContext = renderContextBatch.Contexts[shadowData.ContextIndex + faceIndex];
            SetupRenderContext(renderContext, shadowContext);
            shadowContext.list->Clear();
            shadowContext.view.SetProjector(SpotLight_NearPlane, light.Radius, light.Position, light.Direction, light.UpVector, light.OuterConeAngle * 2.0f);
            shadowContext.view.PrepareCache(shadowContext, shadowMapsSizeCube, shadowMapsSizeCube, Float2::Zero, &view);
            Matrix::Transpose(shadowContext.view.ViewProjection(), shadowData.Constants.ShadowVP[faceIndex]);
        }

        // Setup constant buffer data
        shadowData.Constants.ShadowMapSize = shadowMapsSizeCube;
        shadowData.Constants.Sharpness = light.ShadowsSharpness;
        shadowData.Constants.Fade = Math::Saturate(light.ShadowsStrength * fade);
        shadowData.Constants.NormalOffsetScale = light.ShadowsNormalOffsetScale * NormalOffsetScaleTweak * (1.0f / shadowMapsSizeCube);
        shadowData.Constants.Bias = light.ShadowsDepthBias;
        shadowData.Constants.FadeDistance = Math::Max(light.ShadowsFadeDistance, 0.1f);
        shadowData.Constants.NumCascades = 1;
        shadowData.Constants.CascadeSplits = Float4::Zero;
    }

    void ShadowsPass::Dispose()
    {
        // Base
        RendererPass::Dispose();

        // Cleanup
        _psShadowDir.Delete();
        _psShadowPoint.Delete();
        _psShadowSpot.Delete();
        _shader = nullptr;
        _sphereModel = nullptr;
        DeleteObjectSafe(_shadowMapCSM);
        DeleteObjectSafe(_shadowMapCube);
    }

    void ShadowsPass::Prepare()
    {
        // Clear cached data
        _shadowData.Clear();
        LastDirLightIndex = -1;
        LastDirLightShadowMap = nullptr;
    }

    void ShadowsPass::SetupShadows(RenderContext& renderContext, RenderContextBatch& renderContextBatch)
    {
        PROFILE_CPU();
        auto& view = renderContext.view;

        // Update shadow map
        const auto shadowMapsQuality = Quality::Ultra;
        if (shadowMapsQuality != _currentShadowMapsQuality)
            UpdateShadowMapSize();

        auto shadowsQuality = Quality::Ultra;
        maxShadowsQuality = Math::Clamp(Math::Min<int32>(static_cast<int32>(shadowsQuality), static_cast<int32>(view.MaxShadowsQuality)), 0, static_cast<int32>(Quality::MAX) - 1);

        // Create shadow projections for lights
        for (auto& light : renderContext.list->DirectionalLights)
        {
            if (::SE::CanRenderShadow(view, light) && CanRenderShadow(renderContext, light))
            {
                SetupLight(renderContext, renderContextBatch, light);
            }
        }
        for (auto& light : renderContext.list->PointLights)
        {
            if (::SE::CanRenderShadow(view, light) && CanRenderShadow(renderContext, light))
            {
                SetupLight(renderContext, renderContextBatch, light);
            }
        }
        for (auto& light : renderContext.list->SpotLights)
        {
            if (::SE::CanRenderShadow(view, light) && CanRenderShadow(renderContext, light))
            {
                SetupLight(renderContext, renderContextBatch, light);
            }
        }
    }

    bool ShadowsPass::CanRenderShadow(const RenderContext& renderContext, const RendererPointLightData& light)
    {
        const Float3 lightPosition = light.Position;
        const float dstLightToView = Float3::Distance(lightPosition, renderContext.view.Position);

        // Fade shadow on distance
        const float fadeDistance = Math::Max(light.ShadowsFadeDistance, 0.1f);
        const float fade = 1 - Math::Saturate((dstLightToView - light.Radius - light.ShadowsDistance + fadeDistance) / fadeDistance);

        return fade > Math::ZeroTolerance && _shadowMapFormat != PixelFormat::Undefined;
    }

    bool ShadowsPass::CanRenderShadow(const RenderContext& renderContext, const RendererSpotLightData& light)
    {
        const Float3 lightPosition = light.Position;
        const float dstLightToView = Float3::Distance(lightPosition, renderContext.view.Position);

        // Fade shadow on distance
        const float fadeDistance = Math::Max(light.ShadowsFadeDistance, 0.1f);
        const float fade = 1 - Math::Saturate((dstLightToView - light.Radius - light.ShadowsDistance + fadeDistance) / fadeDistance);

        return fade > Math::ZeroTolerance && _shadowMapFormat != PixelFormat::Undefined;
    }

    bool ShadowsPass::CanRenderShadow(const RenderContext& renderContext, const RendererDirectionalLightData& light)
    {
        return _shadowMapFormat != PixelFormat::Undefined;
    }

    void ShadowsPass::RenderShadow(RenderContextBatch& renderContextBatch, RendererPointLightData& light, GPUTextureView* shadowMask)
    {
        if (light.ShadowDataIndex == -1)
        {
            return;
        }

        PROFILE_GPU_CPU("Shadow");
        GPUContext* context = GPUDevice::instance->GetMainContext();
        RenderContext& renderContext = renderContextBatch.GetMainContext();
        ShadowData& shadowData = _shadowData[light.ShadowDataIndex];
        const float sphereModelScale = 3.0f;
        auto& view = renderContext.view;
        auto shader = _shader->GetShader();

        // TODO: here we can use lower shadows quality based on light distance to view (LOD switching) and per light setting for max quality
        int32 shadowQuality = maxShadowsQuality;

        // Set up GPU context and render view
        const auto shadowMapsSizeCube = (float)_shadowMapsSizeCube;
        context->SetViewportAndScissors(shadowMapsSizeCube, shadowMapsSizeCube);

        {
            PROFILE_GPU("Face");
            // Render depth to all 6 faces of the cube map
            for (int32 faceIndex = 0; faceIndex < 6; faceIndex++)
            {
                auto rt = _shadowMapCube->View(faceIndex);
                context->ResetSR();
                context->SetRenderTarget(nullptr, rt);
                context->ClearDepth(rt);
                auto& shadowContext = renderContextBatch.Contexts[shadowData.ContextIndex + faceIndex];
                shadowContext.list->ExecuteDrawCalls(shadowContext, DrawCallsListType::Depth);
                shadowContext.list->ExecuteDrawCalls(shadowContext, shadowContext.list->ShadowDepthDrawCallsList, renderContext.list->DrawCalls, nullptr);
            }
        }

        // Restore GPU context
        context->ResetSR();
        context->ResetRenderTarget(); 
        const Viewport viewport = renderContext.task->GetViewport();
        GPUTexture* depthBuffer = renderContext.buffers->DepthBuffer;
        GPUTextureView* depthBufferSRV = depthBuffer->Flags().IsFlag(GPUTextureFlags::ReadOnlyDepthView) ? depthBuffer->ViewReadOnlyDepth() : depthBuffer->View();
        context->SetViewportAndScissors(viewport);
        context->BindSR(0, renderContext.buffers->GBuffer0);
        context->BindSR(1, renderContext.buffers->GBuffer1);
        context->BindSR(2, renderContext.buffers->GBuffer2);
        context->BindSR(3, depthBufferSRV);
        context->BindSR(4, renderContext.buffers->GBuffer3);

        // Setup shader data
        Data sperLight;
        GBufferPass::SetInputs(view, sperLight.GBuffer);
        light.SetupLightData(&sperLight.Light, true);
        sperLight.LightShadow = shadowData.Constants;
        Matrix::Transpose(view.ViewProjection(), sperLight.ViewProjectionMatrix);
        sperLight.ContactShadowsDistance = light.ShadowsDistance;
        sperLight.ContactShadowsLength = 0;//EnumHasAnyFlags(view.Flags, ViewFlags::ContactShadows) ? light.ContactShadowsLength : 0.0f;

        // Calculate world view projection matrix for the light sphere
        Matrix world, wvp, matrix;
        Matrix::Scaling(light.Radius * sphereModelScale, wvp);
        Matrix::Translation(light.Position, matrix);
        Matrix::Multiply(wvp, matrix, world);
        Matrix::Multiply(world, view.ViewProjection(), wvp);
        Matrix::Transpose(wvp, sperLight.WVP);

        // Render shadow in screen space
        context->UpdateCB(shader->GetCB(0), &sperLight);
        context->BindCB(0, shader->GetCB(0));
        context->BindCB(1, shader->GetCB(1));
        context->BindSR(5, _shadowMapCube->ViewArray());
        context->SetRenderTarget(shadowMask);
        context->SetState(_psShadowPoint.Get(shadowQuality + (sperLight.ContactShadowsLength > Math::ZeroTolerance ? 4 : 0)));
        _sphereModel->Render(context);

        // Cleanup
        context->ResetRenderTarget();
        context->UnBindSR(5);
    }

    void ShadowsPass::RenderShadow(RenderContextBatch& renderContextBatch, RendererSpotLightData& light, GPUTextureView* shadowMask)
    {
        if (light.ShadowDataIndex == -1)
            return;
        PROFILE_GPU_CPU("Shadow");
        GPUContext* context = GPUDevice::instance->GetMainContext();
        RenderContext& renderContext = renderContextBatch.GetMainContext();
        ShadowData& shadowData = _shadowData[light.ShadowDataIndex];
        const float sphereModelScale = 3.0f;
        auto& view = renderContext.view;
        auto shader = _shader->GetShader();

        // TODO: here we can use lower shadows quality based on light distance to view (LOD switching) and per light setting for max quality
        int32 shadowQuality = maxShadowsQuality;

        // Set up GPU context and render view
        const auto shadowMapsSizeCube = (float)_shadowMapsSizeCube;
        context->SetViewportAndScissors(shadowMapsSizeCube, shadowMapsSizeCube);

        // Render depth to all 1 face of the cube map
        constexpr int32 faceIndex = 0;
        {
            auto rt = _shadowMapCube->View(faceIndex);
            context->ResetSR();
            context->SetRenderTarget(nullptr, rt);
            context->ClearDepth(rt);
            auto& shadowContext = renderContextBatch.Contexts[shadowData.ContextIndex + faceIndex];
            shadowContext.list->ExecuteDrawCalls(shadowContext, DrawCallsListType::Depth);
            shadowContext.list->ExecuteDrawCalls(shadowContext, shadowContext.list->ShadowDepthDrawCallsList, renderContext.list->DrawCalls, nullptr);
        }

        // Restore GPU context
        context->ResetSR();
        context->ResetRenderTarget();
        const Viewport viewport = renderContext.task->GetViewport();
        GPUTexture* depthBuffer = renderContext.buffers->DepthBuffer;
        GPUTextureView* depthBufferSRV =depthBuffer->Flags().IsFlag(GPUTextureFlags::ReadOnlyDepthView) ? depthBuffer->ViewReadOnlyDepth() : depthBuffer->View();
        context->SetViewportAndScissors(viewport);
        context->BindSR(0, renderContext.buffers->GBuffer0);
        context->BindSR(1, renderContext.buffers->GBuffer1);
        context->BindSR(2, renderContext.buffers->GBuffer2);
        context->BindSR(3, depthBufferSRV);
        context->BindSR(4, renderContext.buffers->GBuffer3);

        // Setup shader data
        Data sperLight;
        GBufferPass::SetInputs(view, sperLight.GBuffer);
        light.SetupLightData(&sperLight.Light, true);
        sperLight.LightShadow = shadowData.Constants;
        Matrix::Transpose(view.ViewProjection(), sperLight.ViewProjectionMatrix);
        sperLight.ContactShadowsDistance = light.ShadowsDistance;
        sperLight.ContactShadowsLength = 0;//EnumHasAnyFlags(view.Flags, ViewFlags::ContactShadows) ? light.ContactShadowsLength : 0.0f;

        // Calculate world view projection matrix for the light sphere
        Matrix world, wvp, matrix;
        Matrix::Scaling(light.Radius * sphereModelScale, wvp);
        Matrix::Translation(light.Position, matrix);
        Matrix::Multiply(wvp, matrix, world);
        Matrix::Multiply(world, view.ViewProjection(), wvp);
        Matrix::Transpose(wvp, sperLight.WVP);

        // Render shadow in screen space
        context->UpdateCB(shader->GetCB(0), &sperLight);
        context->BindCB(0, shader->GetCB(0));
        context->BindCB(1, shader->GetCB(1));
        context->BindSR(5, _shadowMapCube->View(faceIndex));
        context->SetRenderTarget(shadowMask);
        context->SetState(_psShadowSpot.Get(shadowQuality + (sperLight.ContactShadowsLength > Math::ZeroTolerance ? 4 : 0)));
        _sphereModel->Render(context);

        // Cleanup
        context->ResetRenderTarget();
        context->UnBindSR(5);
    }

    void ShadowsPass::RenderShadow(RenderContextBatch& renderContextBatch, RendererDirectionalLightData& light, int32 index, GPUTextureView* shadowMask)
    {
        if (light.ShadowDataIndex == -1)
        {
            return;
        }

        PROFILE_GPU_CPU("Shadow");
        GPUContext* context = GPUDevice::instance->GetMainContext();
        RenderContext& renderContext = renderContextBatch.GetMainContext();
        ShadowData& shadowData = _shadowData[light.ShadowDataIndex];
        const float shadowMapsSizeCSM = (float)_shadowMapsSizeCSM;
        context->SetViewportAndScissors(shadowMapsSizeCSM, shadowMapsSizeCSM);

        // Render shadow map for each projection
        for (int32 cascadeIndex = 0; cascadeIndex < shadowData.ContextCount; cascadeIndex++)
        {
            const auto rt = _shadowMapCSM->View(cascadeIndex);
            context->ResetSR();
            context->SetRenderTarget(nullptr, rt);
            context->ClearDepth(rt);
            auto& shadowContext = renderContextBatch.Contexts[shadowData.ContextIndex + cascadeIndex];
            shadowContext.list->ExecuteDrawCalls(shadowContext, DrawCallsListType::Depth);
            shadowContext.list->ExecuteDrawCalls(shadowContext, shadowContext.list->ShadowDepthDrawCallsList, renderContext.list->DrawCalls, nullptr);
        }

        // Restore GPU context
        context->ResetSR();
        context->ResetRenderTarget();
        GPUTexture* depthBuffer = renderContext.buffers->DepthBuffer;
        GPUTextureView* depthBufferSRV = depthBuffer->Flags().IsFlag(GPUTextureFlags::ReadOnlyDepthView) ? depthBuffer->ViewReadOnlyDepth() : depthBuffer->View();
        context->SetViewportAndScissors(renderContext.task->GetViewport());
        context->BindSR(0, renderContext.buffers->GBuffer0);
        context->BindSR(1, renderContext.buffers->GBuffer1);
        context->BindSR(2, renderContext.buffers->GBuffer2);
        context->BindSR(3, depthBufferSRV);
        context->BindSR(4, renderContext.buffers->GBuffer3);

        // Setup shader data
        Data sperLight;
        auto& view = renderContext.view;
        GBufferPass::SetInputs(view, sperLight.GBuffer);
        light.SetupLightData(&sperLight.Light, true);
        sperLight.LightShadow = shadowData.Constants;
        Matrix::Transpose(view.ViewProjection(), sperLight.ViewProjectionMatrix);
        sperLight.ContactShadowsDistance = light.ShadowsDistance;
        sperLight.ContactShadowsLength = 0;//EnumHasAnyFlags(view.Flags, ViewFlags::ContactShadows) ? light.ContactShadowsLength : 0.0f;

        // Render shadow in screen space
        auto shader = _shader->GetShader();
        context->UpdateCB(shader->GetCB(0), &sperLight);
        context->BindCB(0, shader->GetCB(0));
        context->BindCB(1, shader->GetCB(1));
        context->BindSR(5, _shadowMapCSM->ViewArray());
        context->SetRenderTarget(shadowMask);
        context->SetState(_psShadowDir.Get(maxShadowsQuality + static_cast<int32>(Quality::MAX) * shadowData.BlendCSM + (sperLight.ContactShadowsLength > Math::ZeroTolerance ? 8 : 0)));
        context->DrawFullscreenTriangle();

        // Cleanup
        context->ResetRenderTarget();
        context->UnBindSR(5);

        // Cache params for the volumetric fog or other effects that use dir light shadow sampling
        LastDirLightIndex = index;
        LastDirLightShadowMap = _shadowMapCSM->ViewArray();
        LastDirLight = sperLight.LightShadow;
    }
} // SE