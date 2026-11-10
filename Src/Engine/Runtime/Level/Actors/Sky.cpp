
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
        RenderActor(params), _shader(nullptr), _psSky(nullptr), _psSkyBox(nullptr), RenderSkyType()
    {
        m_DrawNoCulling = 1;
        m_DrawCategory = SceneRendering::PreRender;
    }

    Sky::~Sky()
    {
        if (_shader)
            _shader->OnReloadingEvent.Unbind<Sky, &Sky::OnShaderReloading>(this);
        ReleasePrograms();
    }

    bool Sky::SetupProgram(const String& name, SLC2GPUPipelineState*& pipeline, ShaderProgramInstance*& instance)
    {
        if (pipeline && instance)
            return true;
        SLC2GPUShaderProgram* program = nullptr;
        if (!_shader || !_shader->SelectProgram(name, {}, program))
            return false;
        auto description             = SLC2GPUPipelineState::Description::Default;
        description.CullMode         = CullMode::Inverted;
        description.DepthWriteEnable = false;
        description.DepthClipEnable  = false;
        description.DepthFunc        = ComparisonFunc::LessEqual;
        pipeline                     = GPUDevice::instance->CreateSLC2PipelineState();
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
        DeleteObjectSafe(_psSky);
        DeleteObjectSafe(_psSkyBox);
        DeleteObjectSafe(m_SkyProgram);
        DeleteObjectSafe(m_SkyBoxProgram);
    }

    void Sky::RenderDraw(RenderContext& renderContext)
    {
        if (!HasContentLoaded())
            return;
        const bool ready = RenderSkyType == SkyType::SkyBox ?
                               SetupProgram(SE_TEXT("SkyBox"), _psSkyBox, m_SkyBoxProgram) :
                               SetupProgram(SE_TEXT("Sky"), _psSky, m_SkyProgram);
        if (ready)
            renderContext.list->Sky = this;
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

    bool Sky::IsDynamicSky() const
    {
        return !IsStatic();// || (SunLight && !SunLight->IsStatic());
    }

    ShaderProgramInstance* Sky::ApplySky(GPUContext* context, RenderContext& renderContext, const Matrix& world)
    {
        ShaderProgramInstance* instance = RenderSkyType == SkyType::SkyBox ? m_SkyBoxProgram : m_SkyProgram;
        SLC2GPUPipelineState*  pipeline = RenderSkyType == SkyType::SkyBox ? _psSkyBox : _psSky;
        if (!instance || !pipeline)
            return nullptr;
        Data   data = {};
        Matrix matrix;
        Matrix::Multiply(world, renderContext.view.frustum.GetMatrix(), matrix);
        Matrix::Transpose(matrix, data.WVP);
        GBufferPass::SetInputs(renderContext.view, data.GBuffer);
        data.ViewOffset = renderContext.view.Origin + GetPosition();
        if (!instance->SetConstantBuffer(SE_TEXT("data"), &data, sizeof(data)))
        {
            return nullptr;
        }

        if (RenderSkyType == SkyType::SkyBox)
        {
            GPUTexture* texture = CubeTexture ? CubeTexture->GetTexture() : nullptr;
            if (!texture || !instance->SetTexture(SE_TEXT("SkyBoxTex"), texture->ViewArray()))
            {
                return nullptr;
            }
        }
        context->SetSLC2State(pipeline);
        return instance;
    }

    void Sky::EndPlay()
    {
        ReleasePrograms();
        Actor::EndPlay();
    }

    void Sky::OnEnable()
    {
        _shader = AssetContent::LoadAsyncInternal<Shader>(SE_TEXT("Shaders/Sky"));
        if (!_shader)
            LOG_ERROR("Level", "Cannot load sky shader.");
        else
            _shader->OnReloadingEvent.Bind<Sky, &Sky::OnShaderReloading>(this);
        GetScene()->Rendering.AddRender(this, _sceneRenderingKey);
#if USE_EDITOR
        GetSceneRendering()->AddViewportIcon(this);
#endif
        Actor::OnEnable();
    }

    void Sky::OnDisable()
    {
        if (_shader)
            _shader->OnReloadingEvent.Unbind<Sky, &Sky::OnShaderReloading>(this);
        ReleasePrograms();
#if USE_EDITOR
        GetSceneRendering()->RemoveViewportIcon(this);
#endif
        GetScene()->Rendering.RemoveRender(this, _sceneRenderingKey);
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
