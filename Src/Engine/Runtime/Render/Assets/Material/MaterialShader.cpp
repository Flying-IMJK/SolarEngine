
#include "MaterialShader.h"

#include "DeferredMaterialShader.h"
#include "ForwardMaterialShader.h"
#include "GUIMaterialShader.h"
#include "IMaterial.h"

#include "Runtime/Core/Math/Matrix.h"

#include "Runtime/Utilities/Time.h"
#include "Runtime/Graphics/GPUContext.h"
#include "Runtime/Graphics/GPUDevice.h"
#include "Runtime/Graphics/Shaders/GPUConstantBuffer.h"
#include "Runtime/Graphics/Shaders/GPUShader.h"
#include "Runtime/Render/RenderContext.h"

namespace SE
{
    GES_PACK_STRUCT(struct MaterialShaderDataPerView {
        Matrix ViewMatrix;
        Matrix ViewProjectionMatrix;
        Matrix PrevViewProjectionMatrix;
        Matrix MainViewProjectionMatrix;
        Float4 MainScreenSize;
        Float3 ViewPos;
        float ViewFar;
        Float3 ViewDir;
        float TimeParam;
        Float4 ViewInfo;
        Float4 ScreenSize;
        Float4 TemporalAAJitter;
    });


    IMaterial::BindParameters::BindParameters(GPUContext* context, const RenderContext& renderContext)
        : gpuContext(context)
        , renderContext(renderContext)
        , firstDrawCall(nullptr)
        , drawCallsCount(0)
        , timeParam(Time::Render.UnscaledTime.GetTotalSeconds())
    {
    }

    IMaterial::BindParameters::BindParameters(GPUContext* context, const RenderContext& renderContext, const DrawCall& drawCall)
        : gpuContext(context)
        , renderContext(renderContext)
        , firstDrawCall(&drawCall)
        , drawCallsCount(1)
        , timeParam(Time::Render.UnscaledTime.GetTotalSeconds())
    {
    }

    IMaterial::BindParameters::BindParameters(GPUContext* context, const RenderContext& renderContext, const DrawCall* firstDrawCall, int32 drawCallsCount)
        : gpuContext(context)
        , renderContext(renderContext)
        , firstDrawCall(firstDrawCall)
        , drawCallsCount(drawCallsCount)
        , timeParam(Time::Render.UnscaledTime.GetTotalSeconds())
    {
    }

    GPUConstantBuffer* IMaterial::BindParameters::PerViewConstants = nullptr;

    void IMaterial::BindParameters::BindViewData() const
    {
        // Lazy-init
        if (!PerViewConstants)
        {
            PerViewConstants = GPUDevice::instance->CreateConstantBuffer(sizeof(MaterialShaderDataPerView), SE_TEXT("PerViewConstants"));
        }

        // Setup data
        MaterialShaderDataPerView cb;
        int aa1 = sizeof(MaterialShaderDataPerView);
        Matrix::Transpose(renderContext.view.frustum.GetMatrix(), cb.ViewProjectionMatrix);
        Matrix::Transpose(renderContext.view.View, cb.ViewMatrix);
        Matrix::Transpose(renderContext.view.PrevViewProjection, cb.PrevViewProjectionMatrix);
        Matrix::Transpose(renderContext.view.MainViewProjection, cb.MainViewProjectionMatrix);
        cb.MainScreenSize = renderContext.view.MainScreenSize;
        cb.ViewPos = renderContext.view.Position;
        cb.ViewFar = renderContext.view.Far;
        cb.ViewDir = renderContext.view.Direction;
        cb.TimeParam = timeParam;
        cb.ViewInfo = renderContext.view.ViewInfo;
        cb.ScreenSize = renderContext.view.ScreenSize;
        cb.TemporalAAJitter = renderContext.view.TemporalAAJitter;

        // Update constants
        gpuContext->UpdateCB(PerViewConstants, &cb);
        gpuContext->BindCB(1, PerViewConstants);
    }



    MaterialShader::PipelineStateCache::PipelineStateCache()
    {
        Platform::MemoryClear(PS, sizeof(PS));
    }

    GPUPipelineState* MaterialShader::PipelineStateCache::GetPS(CullMode mode, bool wireframe)
    {
        const int32 index = static_cast<int32>(mode) + (wireframe ? 3 : 0);
        auto ps = PS[index];
        if (!ps)
            PS[index] = ps = InitPS(mode, wireframe);
        return ps;
    }

    GPUPipelineState* MaterialShader::PipelineStateCache::InitPS(CullMode mode, bool wireframe)
    {
        Desc.CullMode = mode;
        Desc.Wireframe = wireframe;
        auto ps = GPUDevice::instance->CreatePipelineState();
        ps->Init(Desc);
        return ps;
    }

    void MaterialShader::PipelineStateCache::Release()
    {
        for (auto& resource : PS)
        {
            DeleteObjectSafe(resource);
        }
    }



    MaterialShader::MaterialShader(const StringView& name) :
        m_IsLoaded(false),
        m_Shader(nullptr),
        m_CB(nullptr)
    {
        ENGINE_ASSERT(GPUDevice::instance);
        m_Shader = GPUDevice::instance->CreateShader(name);
    }

    MaterialShader::~MaterialShader()
    {
        ENGINE_ASSERT(!m_IsLoaded && m_Shader);
        DeleteObjectSafe(m_Shader);
    }


    class DummyMaterial final : public MaterialShader
    {
    public:
        DummyMaterial()
            : MaterialShader(String::Empty)
        {
        }

    public:
        // [Material]
        void Bind(BindParameters& params) override
        {
        }

    protected:
        // [Material]
        bool OnLoad() override
        {
            return false;
        }
    };

    MaterialShader* MaterialShader::Create(const StringView& name, MemoryReadStream& shaderCacheStream, const MaterialInfo& info)
    {
        MaterialShader* material;
        switch (info.Domain)
        {
        case MaterialDomain::Surface:
            material = info.BlendMode == MaterialBlendMode::Opaque ? (MaterialShader*)New<DeferredMaterialShader>(name) : (MaterialShader*)New<ForwardMaterialShader>(name);
            break;
        /*case MaterialDomain::PostProcess:
            material = New<PostFxMaterialShader>(name);
            break;
        case MaterialDomain::Decal:
            material = New<DecalMaterialShader>(name);
            break;*/
        case MaterialDomain::GUI:
            material = New<GUIMaterialShader>(name);
            break;
        /*case MaterialDomain::Terrain:
            material = New<TerrainMaterialShader>(name);
            break;
        case MaterialDomain::Particle:
            material = New<ParticleMaterialShader>(name);
            break;
        case MaterialDomain::Deformable:
            material = New<DeformableMaterialShader>(name);
            break;
        case MaterialDomain::VolumeParticle:
            material = New<VolumeParticleMaterialShader>(name);
            break;*/
        default:
            LOG_ERROR("Render", "Unknown material type.");
            return nullptr;
        }
        if (!material->Load(shaderCacheStream, info))
        {
            Delete(material);
            return nullptr;
        }
        return material;
    }

    MaterialShader* MaterialShader::CreateDummy(MemoryReadStream& shaderCacheStream, const MaterialInfo& info)
    {
        MaterialShader* material = New<DummyMaterial>();
        if (material->Load(shaderCacheStream, info))
        {
            Delete(material);
            return nullptr;
        }
        return material;
    }

    GPUShader* MaterialShader::GetShader() const
    {
        return m_Shader;
    }

    const MaterialInfo& MaterialShader::GetInfo() const
    {
        return m_Info;
    }

    bool MaterialShader::IsReady() const
    {
        return m_IsLoaded;
    }

    bool MaterialShader::Load(MemoryReadStream& shaderCacheStream, const MaterialInfo& info)
    {
        ENGINE_ASSERT(!m_IsLoaded);

        // Cache material info
        m_Info = info;

        // Create shader
        if (!m_Shader->Create(shaderCacheStream))
        {
            LOG_WARNING("Render", "Cannot load shader.");
            return false;
        }

        // Init memory for a constant buffer
        m_CB = m_Shader->GetCB(0);
        if (m_CB)
        {
            int32 cbSize = m_CB->GetSize();
            if (cbSize == 0)
            {
                // Handle unused constant buffer (eg. postFx returning solid color)
                cbSize = 1024;
                m_CB = nullptr;
            }
            m_CBData.Resize(cbSize, false);
            Platform::MemoryClear(m_CBData.Get(), cbSize);
        }

        // Initialize the material based on type (create pipeline states and setup)
        if (!OnLoad())
        {
            return false;
        }

        m_IsLoaded = true;
        return true;
    }

    void MaterialShader::Unload()
    {
        m_IsLoaded = false;
        m_CB = nullptr;
        m_CBData.Resize(0, false);
        m_Shader->ReleaseGPU();
    }
} // SE