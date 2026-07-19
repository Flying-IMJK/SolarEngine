
#include "Sky.h"

#include "Runtime/Core/Serialization/Serialization.h"
#include "Runtime/Graphics/GPUContext.h"
#include "Runtime/Graphics/GPUDevice.h"
#include "Runtime/Graphics/Base/GPUPipelineState.h"
#include "Runtime/Graphics/Shaders/GPUShader.h"
#include "../Scene/Scene.h"
#include "Runtime/Render/RenderList.h"
#include "Runtime/Render/RenderPass/GBufferPass.h"

namespace SE
{
    GES_PACK_STRUCT(struct Data {
    Matrix WVP;
    Float3 ViewOffset;
    float Padding;
    GBufferData GBuffer;
    // AtmosphericFogData Fog;
    });

    Sky::Sky() : RenderActor()
        , _shader(nullptr)
        , _psSky(nullptr)
        , _psFog(nullptr)
        , _psSkyBox(nullptr)
        , RenderSkyType()
    {
        m_DrawNoCulling = 1;
        m_DrawCategory = SceneRendering::PreRender;
    }

    Sky::~Sky()
    {
        // SAFE_DELETE_GPU_RESOURCE(_psSky);
        // SAFE_DELETE_GPU_RESOURCE(_psFog);
    }

    /*void Sky::InitConfig(AtmosphericFogData& config) const
    {
        config.AtmosphericFogDensityScale = 1.0f;
        config.AtmosphericFogSunDiscScale = SunDiscScale;
        config.AtmosphericFogDistanceScale = 1;
        config.AtmosphericFogGroundOffset = 0;

        config.AtmosphericFogAltitudeScale = 1;
        config.AtmosphericFogStartDistance = 0;
        config.AtmosphericFogPower = 1;
        config.AtmosphericFogDistanceOffset = 0;

        config.AtmosphericFogSunPower = SunPower;
        config.AtmosphericFogDensityOffset = 0.0f;

        if (SunLight)
        {
            config.AtmosphericFogSunDirection = -SunLight->GetDirection();
            config.AtmosphericFogSunColor = SunLight->Color.ToFloat3() * SunLight->Color.A;
        }
        else
        {
            config.AtmosphericFogSunDirection = Float3::UnitY;
            config.AtmosphericFogSunColor = Float3::One;
        }
    }*/

    void Sky::RenderDraw(RenderContext& renderContext)
    {
        if (HasContentLoaded()/* && EnumHasAnyFlags(renderContext.view.flags, ViewFlags::Sky)*/)
        {
            const auto shader = _shader->GetShader();

            if (RenderSkyType == SkyType::SkyBox)
            {
                if (_psSkyBox == nullptr)
                {
                    _psSkyBox = GPUDevice::instance->CreatePipelineState();

                    GPUPipelineState::Description psDesc = GPUPipelineState::Description::Default;
                    psDesc.VS = shader->GetVS(SE_TEXT("VS"));
                    psDesc.PS = shader->GetPS(SE_TEXT("PS_SkyBox"));
                    psDesc.CullMode = CullMode::Inverted;
                    psDesc.DepthWriteEnable = false;
                    psDesc.DepthClipEnable = false;
                    psDesc.DepthFunc = ComparisonFunc::LessEqual;

                    if (!_psSkyBox->Init(psDesc))
                    {
                        LOG_WARNING("Render", "Cannot create graphics pipeline state object for '{0}'.", ToString());
                    }
                }
            }
            else if (RenderSkyType == SkyType::Physical)
            {
                // Ensure to have pipeline state cache created
                if (_psSky == nullptr || _psFog == nullptr)
                {
                    // Create pipeline states
                    if (_psSky == nullptr)
                    {
                        _psSky = GPUDevice::instance->CreatePipelineState();

                        GPUPipelineState::Description psDesc = GPUPipelineState::Description::Default;
                        psDesc.VS = shader->GetVS(SE_TEXT("VS"));
                        psDesc.PS = shader->GetPS(SE_TEXT("PS_Sky"));
                        psDesc.CullMode = CullMode::Inverted;
                        psDesc.DepthWriteEnable = false;
                        psDesc.DepthClipEnable = false;
                        psDesc.DepthFunc = ComparisonFunc::LessEqual;

                        if (!_psSky->Init(psDesc))
                        {
                            LOG_WARNING("Render", "Cannot create graphics pipeline state object for '{0}'.", ToString());
                        }
                    }
                    if (_psFog == nullptr)
                    {
                        _psFog = GPUDevice::instance->CreatePipelineState();

                        GPUPipelineState::Description psDesc = GPUPipelineState::Description::DefaultFullscreenTriangle;
                        psDesc.PS = shader->GetPS(SE_TEXT("PS_Fog"));
                        psDesc.DepthWriteEnable = false;
                        psDesc.DepthClipEnable = false;
                        psDesc.BlendMode = BlendingMode::Additive;

                        if (!_psFog->Init(psDesc))
                        {
                            LOG_WARNING("Render", "Cannot create graphics pipeline state object for '{0}'.", ToString());
                        }
                    }
                }
            }

            // Register for the sky and fog pass
            renderContext.list->Sky = this;
            //if(renderContext.View.Flags & ViewFlags::Fog) != 0)
            //renderContext.List->AtmosphericFog = this; // TODO: finish atmosphere fog
        }
    }

    void Sky::Serialize(SerializeContext& context)
    {
        // Base
        Actor::Serialize(context);

        SERIALIZE_GET_OTHER_OBJ(Sky, context.otherObj);

        // SERIALIZE_MEMBER(Sun, SunLight);
        SERIALIZE(SunDiscScale);
        SERIALIZE(SunPower);
    }

    void Sky::Deserialize(DeserializeContext& context)
    {
        // Base
        Actor::Deserialize(context);

        // DESERIALIZE_MEMBER(Sun, SunLight);
        DESERIALIZE(SunDiscScale);
        DESERIALIZE(SunPower);
    }

    bool Sky::HasContentLoaded() const
    {
        return _shader && _shader->IsLoaded() &&
            (RenderSkyType == SkyType::SkyBox && CubeTexture != nullptr && CubeTexture.Get()->IsLoaded());// && AtmospherePreCompute::GetCache(nullptr);
    }

    /*bool Sky::IntersectsItself(const Ray& ray, float& distance, Float3& normal)
    {
        return false;
    }*/

    /*void Sky::DrawFog(GPUContext* context, RenderContext& renderContext, GPUTextureView* output)
    {
        // Get precomputed cache and bind it to the pipeline
        AtmosphereCache cache;
        if (!AtmospherePreCompute::GetCache(&cache))
            return;
        context->BindSR(4, cache.Transmittance);
        context->BindSR(5, cache.Irradiance);
        context->BindSR(6, cache.Inscatter->ViewVolume());

        // Bind GBuffer inputs
        context->BindSR(0, renderContext.Buffers->GBuffer0);
        context->BindSR(1, renderContext.Buffers->GBuffer1);
        context->BindSR(2, renderContext.Buffers->GBuffer2);
        context->BindSR(3, renderContext.Buffers->DepthBuffer);

        // Setup constants data
        Data data;
        GBufferPass::SetInputs(renderContext.View, data.GBuffer);
        data.ViewOffset = renderContext.View.Origin + GetPosition();
        InitConfig(data.Fog);
        data.Fog.AtmosphericFogSunPower *= SunLight ? SunLight->Brightness : 1.0f;
        bool useSpecularLight = EnumHasAnyFlags(renderContext.View.Flags, ViewFlags::SpecularLight);
        if (!useSpecularLight)
        {
            data.Fog.AtmosphericFogSunDiscScale = 0;
        }

        // Bind pipeline
        auto cb = _shader->GetShader()->GetCB(0);
        context->UpdateCB(cb, &data);
        context->BindCB(0, cb);
        context->SetState(_psFog);
        context->SetRenderTarget(output);
        context->DrawFullscreenTriangle();
    }*/

    bool Sky::IsDynamicSky() const
    {
        return !IsStatic();// || (SunLight && !SunLight->IsStatic());
    }

    void Sky::ApplySky(GPUContext* context, RenderContext& renderContext, const Matrix& world)
    {
        if (RenderSkyType == SkyType::SkyBox)
        {
            // Setup constants data
            Matrix m;
            Data data;
            Matrix::Multiply(world, renderContext.view.frustum.GetMatrix(), m);
            Matrix::Transpose(m, data.WVP);
            GBufferPass::SetInputs(renderContext.view, data.GBuffer);
            data.ViewOffset = renderContext.view.Origin + GetPosition();

            // Bind pipeline
            auto cb = _shader->GetShader()->GetCB(0);
            context->UpdateCB(cb, &data);
            context->BindCB(0, cb);
            context->BindSR(5, CubeTexture.Get()->GetTexture());
            context->SetState(_psSkyBox);
        }
        else if (RenderSkyType == SkyType::Physical)
        {
            // Get precomputed cache and bind it to the pipeline
            // AtmosphereCache cache;
            // if (!AtmospherePreCompute::GetCache(&cache))
            //     return;
            /*context->BindSR(4, cache.Transmittance);
            context->BindSR(5, cache.Irradiance);
            context->BindSR(6, cache.Inscatter->ViewVolume());

            // Setup constants data
            Matrix m;
            Data data;
            Matrix::Multiply(world, renderContext.view.frustum.GetMatrix(), m);
            Matrix::Transpose(m, data.WVP);
            GBufferPass::SetInputs(renderContext.view, data.GBuffer);
            data.ViewOffset = renderContext.view.Origin + GetPosition();
            InitConfig(data.Fog);
            //data.Fog.AtmosphericFogSunPower *= SunLight ? SunLight->Brightness : 1.0f;
            bool useSpecularLight = EnumHasAnyFlags(renderContext.view.Flags, ViewFlags::SpecularLight);
            if (!useSpecularLight)
            {
                // Hide sun disc if specular light is disabled
                data.fog.AtmosphericFogSunDiscScale = 0;
            }

            // Bind pipeline
            auto cb = _shader->GetShader()->GetCB(0);
            context->UpdateCB(cb, &data);
            context->BindCB(0, cb);
            context->SetState(_psSky);*/
        }
    }

    void Sky::EndPlay()
    {
        // Cleanup
        DeleteObjectSafe(_psSky);
        DeleteObjectSafe(_psFog);
        DeleteObjectSafe(_psSkyBox);

        // Base
        Actor::EndPlay();
    }

    void Sky::OnEnable()
    {
        // Load shader
        _shader = AssetContent::LoadAsyncInternal<Shader>(SE_TEXT("Shaders/Sky"));
        if (_shader == nullptr)
        {
            LOG_FATAL("Level", "Cannot load sky shader.");
        }
        else
        {
#if COMPILE_WITH_DEV_ENV
            _shader.Get()->OnReloading.Bind<Sky, &Sky::OnShaderReloading>(this);
#endif
        }

        GetScene()->Rendering.AddRender(this, _sceneRenderingKey);
#if USE_EDITOR
        GetSceneRendering()->AddViewportIcon(this);
#endif

        // Base
        Actor::OnEnable();
    }

    void Sky::OnDisable()
    {
#if USE_EDITOR
        GetSceneRendering()->RemoveViewportIcon(this);
#endif
        GetScene()->Rendering.RemoveRender(this, _sceneRenderingKey);

        // Base
        Actor::OnDisable();
    }

    void Sky::OnTransformChanged()
    {
        // Base
        Actor::OnTransformChanged();

        m_Box = BoundingBox(m_Transform.Translation);
        m_Sphere = BoundingSphere(m_Transform.Translation, 0.0f);
    }
} // SE