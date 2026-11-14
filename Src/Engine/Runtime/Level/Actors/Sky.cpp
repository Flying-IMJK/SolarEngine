
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
        RenderActor(params), m_Shader(nullptr), m_PsoSky(nullptr), m_PsoSkyBox(nullptr), RenderSkyType()
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

    bool Sky::SetupProgram(const String& name, GPUPipelineState*& pipeline, ShaderProgramInstance*& instance)
    {
        if (pipeline && instance)
        {
            return true;
        }
        GPUShaderProgram* program = nullptr;
        if (!m_Shader || !m_Shader->SelectProgram(name, {}, program))
        {
            return false;
        }
        auto description             = GPUPipelineState::Description::Default;
        description.CullMode         = CullMode::Inverted;
        description.DepthWriteEnable = false;
        description.DepthClipEnable  = false;
        description.DepthFunc        = ComparisonFunc::LessEqual;
        pipeline                     = GPUDevice::instance->CreatePipelineState();
        if (!pipeline || !pipeline->Init(description, program))
        {
            DeleteObjectSafe(pipeline);
            return false;
        }
        instance = New<ShaderProgramInstance>();
        if (!instance->Initialize(program))
        {
            DeleteObjectSafe(instance);
            DeleteObjectSafe(pipeline);
            return false;
        }
        return true;
    }

    void Sky::ReleasePrograms()
    {
        DeleteObjectSafe(m_PsoSky);
        DeleteObjectSafe(m_PsoSkyBox);
        DeleteObjectSafe(m_SkyProgram);
        DeleteObjectSafe(m_SkyBoxProgram);
    }

    void Sky::RenderDraw(RenderContext& renderContext)
    {
        if (!HasContentLoaded())
        {
            return;
        }
        const bool ready = RenderSkyType == SkyType::SkyBox ?
                               SetupProgram(SE_TEXT("SkyBox"), m_PsoSkyBox, m_SkyBoxProgram) :
                               SetupProgram(SE_TEXT("Sky"), m_PsoSky, m_SkyProgram);
        if (ready)
        {
            renderContext.list->Sky = this;
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
        return m_Shader && m_Shader->IsLoaded() &&
            (RenderSkyType == SkyType::SkyBox && CubeTexture != nullptr && CubeTexture.Get()->IsLoaded());// && AtmospherePreCompute::GetCache(nullptr);
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
        ShaderProgramInstance* instance = nullptr;
        GPUPipelineState*  pipeline = nullptr;

        if (RenderSkyType == SkyType::SkyBox)
        {
            instance = m_SkyBoxProgram;
            pipeline = m_PsoSkyBox;
        }
        else
        {
            instance = m_SkyProgram;
            pipeline = m_PsoSky;
        }

        if (instance == nullptr || pipeline == nullptr)
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

        if (RenderSkyType == SkyType::SkyBox)
        {
            GPUTexture* texture = CubeTexture ? CubeTexture->GetTexture() : nullptr;
            if (!texture || !instance->SetTexture(SE_TEXT("SkyBoxTex"), texture->View()))
            {
                return nullptr;
            }
        }
        context->SetState(pipeline);
        return instance;
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
