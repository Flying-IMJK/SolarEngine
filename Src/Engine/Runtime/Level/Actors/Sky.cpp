
#include <Runtime/Level/Actors/Sky.h>

#include <Runtime/Core/Serialization/Serialization.h>
#include <Runtime/Graphics/Base/GPUPipelineState.h>
#include <Runtime/Graphics/GPUContext.h>
#include <Runtime/Graphics/GPUDevice.h>
#include <Runtime/Graphics/Shaders/ShaderProgramInstance.h>
#include <Runtime/Level/Scene/Scene.h>
#include <Runtime/Render/RenderList.h>
#include <Runtime/Render/RenderPass/GBufferPass.h>
#include <Runtime/Graphics/Textures/GPUTexture.h>

namespace SE
{
    GES_PACK_STRUCT(struct Data {
    Matrix WVP;
    Float3 ViewOffset;
    float Padding;
    GBufferData GBuffer;
    // AtmosphericFogData Fog;
    });

    Sky::Sky(const SpawnParams& params) :
        RenderActor(params), m_Shader(nullptr), m_PsoSkyBox(nullptr), RenderSkyType()
    {
        m_DrawNoCulling = 1;
        m_DrawCategory = SceneRendering::PreRender;
    }

    Sky::~Sky()
    {
        if (m_Shader)
        {
            m_Shader->OnReloadingEvent.Unbind<Sky, &Sky::OnShaderReloading>(this);
        }
        ReleasePrograms();
    }

    bool Sky::SetupProgram()
    {
        if (m_PsoSkyBox && m_SkyBoxProgram)
        {
            return true;
        }
        GPUShaderProgram* program = nullptr;
        if (!m_Shader || !m_Shader->SelectProgram(SE_TEXT("SkyBox"), {}, program))
        {
            return false;
        }
        auto description             = GPUPipelineState::Description::Default;
        description.CullMode         = CullMode::Inverted;
        description.DepthWriteEnable = false;
        description.DepthClipEnable  = false;
        description.DepthFunc        = ComparisonFunc::LessEqual;
        m_PsoSkyBox                     = GPUDevice::instance->CreatePipelineState();
        if (!m_PsoSkyBox || !m_PsoSkyBox->Init(description, program))
        {
            DeleteObjectSafe(m_PsoSkyBox);
            return false;
        }
        m_SkyBoxProgram = New<ShaderProgramInstance>();
        if (!m_SkyBoxProgram->Initialize(program))
        {
            DeleteObjectSafe(m_SkyBoxProgram);
            DeleteObjectSafe(m_PsoSkyBox);
            return false;
        }
        return true;
    }

    void Sky::ReleasePrograms()
    {
        DeleteObjectSafe(m_PsoSkyBox);
        DeleteObjectSafe(m_SkyBoxProgram);
    }

    void Sky::RenderDraw(RenderContext& renderContext)
    {
        // A Sky actor selects exactly one rendering path for this view.
        renderContext.list->Sky = nullptr;
        renderContext.list->SkyAtmosphere = nullptr;
        if (RenderSkyType == SkyType::Physical)
        {
            renderContext.list->SkyAtmosphere = this;
            return;
        }
        if (HasContentLoaded() && SetupProgram())
        {
            renderContext.list->Sky = this;
        }
    }

    void Sky::Serialize(SerializeContext& context)
    {
        // Base
        Actor::Serialize(context);

        SERIALIZE_GET_OTHER_OBJ(Sky, context.otherObj);

        SERIALIZE(RenderSkyType);
        SERIALIZE_MEMBER(Sun, SunLight);
        SERIALIZE(SunDiscScale);
        SERIALIZE(SunPower);
        context.stream.JKEY("AtmosphereData");
        context.stream.StartObject();
        SERIALIZE_MEMBER(GroundRadius, AtmosphereData.GroundRadius);
        SERIALIZE_MEMBER(AtmosphereHeight, AtmosphereData.AtmosphereHeight);
        SERIALIZE_MEMBER(RayleighScaleHeight, AtmosphereData.RayleighScaleHeight);
        SERIALIZE_MEMBER(RayleighScatteringScale, AtmosphereData.RayleighScatteringScale);
        SERIALIZE_MEMBER(RayleighScattering, AtmosphereData.RayleighScattering);
        SERIALIZE_MEMBER(MieScaleHeight, AtmosphereData.MieScaleHeight);
        SERIALIZE_MEMBER(MieScatteringScale, AtmosphereData.MieScatteringScale);
        SERIALIZE_MEMBER(MieScattering, AtmosphereData.MieScattering);
        SERIALIZE_MEMBER(MieAbsorptionScale, AtmosphereData.MieAbsorptionScale);
        SERIALIZE_MEMBER(MieAbsorption, AtmosphereData.MieAbsorption);
        SERIALIZE_MEMBER(MieAnisotropy, AtmosphereData.MieAnisotropy);
        SERIALIZE_MEMBER(AbsorptionExtinctionScale, AtmosphereData.AbsorptionExtinctionScale);
        SERIALIZE_MEMBER(AbsorptionExtinction, AtmosphereData.AbsorptionExtinction);
        SERIALIZE_MEMBER(AbsorptionTipAltitude, AtmosphereData.AbsorptionTipAltitude);
        SERIALIZE_MEMBER(AbsorptionWidth, AtmosphereData.AbsorptionWidth);
        SERIALIZE_MEMBER(GroundAlbedo, AtmosphereData.GroundAlbedo);
        SERIALIZE_MEMBER(MultiScatteringFactor, AtmosphereData.MultiScatteringFactor);
        SERIALIZE_MEMBER(HeightFogContribution, AtmosphereData.HeightFogContribution);
        SERIALIZE_MEMBER(AerialPerspectiveStartDepth, AtmosphereData.AerialPerspectiveStartDepth);
        SERIALIZE_MEMBER(AerialPerspectiveDepth, AtmosphereData.AerialPerspectiveDepth);
        context.stream.EndObject();
    }

    void Sky::Deserialize(DeserializeContext& context)
    {
        // Base
        Actor::Deserialize(context);
        
        DESERIALIZE(RenderSkyType);
        DESERIALIZE_MEMBER(Sun, SunLight);
        DESERIALIZE(SunDiscScale);
        DESERIALIZE(SunPower);
        const auto atmosphere = SERIALIZE_FIND_MEMBER(context.stream, "AtmosphereData");
        if (atmosphere != context.stream->MemberEnd() && atmosphere->value.IsObject())
        {
            DeserializeContext::StreamScope scope(context, atmosphere->value);
            DESERIALIZE_MEMBER(GroundRadius, AtmosphereData.GroundRadius);
            DESERIALIZE_MEMBER(AtmosphereHeight, AtmosphereData.AtmosphereHeight);
            DESERIALIZE_MEMBER(RayleighScaleHeight, AtmosphereData.RayleighScaleHeight);
            DESERIALIZE_MEMBER(RayleighScatteringScale, AtmosphereData.RayleighScatteringScale);
            DESERIALIZE_MEMBER(RayleighScattering, AtmosphereData.RayleighScattering);
            DESERIALIZE_MEMBER(MieScaleHeight, AtmosphereData.MieScaleHeight);
            DESERIALIZE_MEMBER(MieScatteringScale, AtmosphereData.MieScatteringScale);
            DESERIALIZE_MEMBER(MieScattering, AtmosphereData.MieScattering);
            DESERIALIZE_MEMBER(MieAbsorptionScale, AtmosphereData.MieAbsorptionScale);
            DESERIALIZE_MEMBER(MieAbsorption, AtmosphereData.MieAbsorption);
            DESERIALIZE_MEMBER(MieAnisotropy, AtmosphereData.MieAnisotropy);
            DESERIALIZE_MEMBER(AbsorptionExtinctionScale, AtmosphereData.AbsorptionExtinctionScale);
            DESERIALIZE_MEMBER(AbsorptionExtinction, AtmosphereData.AbsorptionExtinction);
            DESERIALIZE_MEMBER(AbsorptionTipAltitude, AtmosphereData.AbsorptionTipAltitude);
            DESERIALIZE_MEMBER(AbsorptionWidth, AtmosphereData.AbsorptionWidth);
            DESERIALIZE_MEMBER(GroundAlbedo, AtmosphereData.GroundAlbedo);
            DESERIALIZE_MEMBER(MultiScatteringFactor, AtmosphereData.MultiScatteringFactor);
            DESERIALIZE_MEMBER(HeightFogContribution, AtmosphereData.HeightFogContribution);
            DESERIALIZE_MEMBER(AerialPerspectiveStartDepth, AtmosphereData.AerialPerspectiveStartDepth);
            DESERIALIZE_MEMBER(AerialPerspectiveDepth, AtmosphereData.AerialPerspectiveDepth);
        }
    }

    bool Sky::HasContentLoaded() const
    {
        if (RenderSkyType == SkyType::Physical)
        {
            return true;
        }
        if (!m_Shader || !m_Shader->IsLoaded())
        {
            return false;
        }

        if ((RenderSkyType == SkyType::SkyBox && (CubeTexture == nullptr || !CubeTexture.Get()->IsLoaded())))
        {
            return false;
        }

        return true;
    }

    /*bool Sky::IntersectsItself(const Ray& ray, float& distance, Float3& normal)
    {
        return false;
    }*/

    bool Sky::IsDynamicSky() const
    {
        return !IsStatic();// || (SunLight && !SunLight->IsStatic());
    }

    ShaderProgramInstance* Sky::ApplySky(GPUContext* context, RenderContext& renderContext, const Matrix& world)
    {
        ShaderProgramInstance* instance = m_SkyBoxProgram;
        GPUPipelineState* pipeline = m_PsoSkyBox;
        if (RenderSkyType != SkyType::SkyBox || instance == nullptr || pipeline == nullptr)
        {
            return nullptr;
        }

        Data   data = {};
        Matrix::Multiply(world, renderContext.view.frustum.GetMatrix(), data.WVP);
        GBufferPass::SetInputs(renderContext.view, data.GBuffer);
        data.ViewOffset = renderContext.view.Origin + GetPosition();

        if (!instance->SetConstantBuffer(SE_TEXT("data"), &data, sizeof(data)))
        {
            return nullptr;
        }

        GPUTexture* texture = CubeTexture ? CubeTexture->GetTexture() : nullptr;
        if (!texture || !instance->SetTexture(SE_TEXT("SkyBoxTex"), texture->View()))
        {
            return nullptr;
        }
        context->SetState(pipeline);
        return instance;
    }

    void Sky::ApplySkyAtmosphere(SkyAtmosphereData& data)
    {
        data = AtmosphereData;
        data.SunDiscScale = SunDiscScale;

        data.GroundPosition = GetPosition();
        data.SunDirection = Float3::Up;
        data.SunIlluminance = Float3::Zero;
        if (SunLight && SunLight->IsActiveInHierarchy())
        {
            data.SunDirection = -SunLight->GetForward();
            data.SunIlluminance = SunLight->Color.ToFloat3() * (SunLight->Color.A * SunLight->Brightness);
        }
    }

    void Sky::EndPlay()
    {
        ReleasePrograms();
        Actor::EndPlay();
    }

    void Sky::OnEnable()
    {
        m_Shader = AssetContent::LoadAsyncInternal<Shader>(SE_TEXT("Shaders/Sky"));
        if (!m_Shader)
            LOG_ERROR("Level", "Cannot load sky shader.");
        else
            m_Shader->OnReloadingEvent.Bind<Sky, &Sky::OnShaderReloading>(this);
        GetScene()->Rendering.AddRender(this, m_SceneRenderingKey);
#if USE_EDITOR
        GetSceneRendering()->AddViewportIcon(this);
#endif
        Actor::OnEnable();
    }

    void Sky::OnDisable()
    {
        if (m_Shader)
            m_Shader->OnReloadingEvent.Unbind<Sky, &Sky::OnShaderReloading>(this);
        ReleasePrograms();
#if USE_EDITOR
        GetSceneRendering()->RemoveViewportIcon(this);
#endif
        GetScene()->Rendering.RemoveRender(this, m_SceneRenderingKey);
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
