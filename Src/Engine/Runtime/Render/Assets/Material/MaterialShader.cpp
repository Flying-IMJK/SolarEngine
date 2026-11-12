#include <Runtime/Render/Assets/Material/MaterialShader.h>

#include <Runtime/Core/Serialization/MemoryReadStream.h>
#include <Runtime/Graphics/GPUContext.h>
#include <Runtime/Graphics/GPUDevice.h>
#include <Runtime/Graphics/Shaders/GPUShader.h>
#include <Runtime/Graphics/Shaders/GPUShaderProgram.h>
#include <Runtime/Graphics/Shaders/ShaderProgramInstance.h>
#include <Runtime/Render/Assets/Material/DeferredMaterialShader.h>
#include <Runtime/Render/Assets/Material/ForwardMaterialShader.h>
#include <Runtime/Render/Assets/Material/GUIMaterialShader.h>
#include <Runtime/Render/Assets/Material/MaterialParams.h>
#include <Runtime/Render/RenderContext.h>
#include <Runtime/Render/RenderList.h>
#include <Runtime/Utilities/Time.h>

namespace SE
{
    namespace
    {
        class DummyMaterial final : public MaterialShader
        {
        public:
            DummyMaterial() : MaterialShader(String::Empty) {}
            void Bind(BindParameters&) override { m_SelectedProgram = nullptr; }
        };
    } // namespace

    IMaterial::BindParameters::BindParameters(GPUContext* pContext, const RenderContext& renderContext) :
        gpuContext(pContext), renderContext(renderContext), firstDrawCall(nullptr), drawCallsCount(0),
        timeParam(Time::Render.UnscaledTime.GetTotalSeconds())
    {}

    IMaterial::BindParameters::BindParameters(GPUContext*          pContext,
                                              const RenderContext& renderContext,
                                              const DrawCall&      drawCall) : BindParameters(pContext, renderContext)
    {
        firstDrawCall  = &drawCall;
        drawCallsCount = 1;
    }

    IMaterial::BindParameters::BindParameters(GPUContext*          pContext,
                                              const RenderContext& renderContext,
                                              const DrawCall*      pFirstDrawCall,
                                              int32 drawCallsCount) : BindParameters(pContext, renderContext)
    {
        firstDrawCall        = pFirstDrawCall;
        this->drawCallsCount = drawCallsCount;
    }

    MaterialShader::MaterialShader(const StringView& name)
    {
        if (GPUDevice::instance)
            m_Shader = GPUDevice::instance->CreateShader(name);
    }

    MaterialShader::~MaterialShader()
    {
        Unload();
        DeleteObjectSafe(m_Shader);
    }

    MaterialShader* MaterialShader::Create(const StringView& name, MemoryReadStream& shaderCacheStream, const MaterialInfo& info)
    {
        MaterialShader* material = nullptr;
        switch (info.Domain)
        {
            case MaterialDomain::Surface:
                material = info.BlendMode == MaterialBlendMode::Opaque ?
                               static_cast<MaterialShader*>(New<DeferredMaterialShader>(name)) :
                               static_cast<MaterialShader*>(New<ForwardMaterialShader>(name));
                break;
            case MaterialDomain::GUI:
                material = New<GUIMaterialShader>(name);
                break;
            default:
                LOG_ERROR("Render", "Unsupported ShaderArtifact material domain {0}.", static_cast<int32>(info.Domain));
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
        auto material        = New<DummyMaterial>();
        material->m_Info     = info;
        material->m_IsLoaded = true;
        return material;
    }

    bool MaterialShader::Load(MemoryReadStream& shaderCacheStream, const MaterialInfo& info)
    {
        m_Info = info;
        if (!m_Shader || !m_Shader->Load(shaderCacheStream.GetPositionHandle(),
                                         shaderCacheStream.GetLength() - shaderCacheStream.GetPosition()))
        {
            return false;
        }

        const String required = info.Domain == MaterialDomain::GUI          ? SE_TEXT("GUI") :
                                info.BlendMode == MaterialBlendMode::Opaque ? SE_TEXT("GBuffer") :
                                                                              SE_TEXT("Forward");
        if (!HasProgram(required) || (info.Domain == MaterialDomain::Surface &&
                                      (!HasProgram(required + SE_TEXT("Instanced")) || !HasProgram(SE_TEXT("Depth")) ||
                                       !HasProgram(SE_TEXT("DepthInstanced")))))
        {
            LOG_ERROR("Render", "ShaderArtifact material is missing required Programs.");
            return false;
        }
        m_IsLoaded = true;
        return true;
    }

    void MaterialShader::Unload()
    {
        m_IsLoaded        = false;
        m_SelectedProgram = nullptr;

        for (ProgramState& state : m_Programs)
        {
            DeleteObjectSafe(state.Instance);
            DeleteObjectSafe(state.Pipeline);
        }
        m_Programs.Clear();

        if (m_Shader)
        {
            m_Shader->ReleaseGPU();
        }
    }

    const MaterialInfo& MaterialShader::GetInfo() const { return m_Info; }
    GPUShader* MaterialShader::GetShader() const { return m_Shader; }
    bool MaterialShader::IsReady() const { return m_IsLoaded; }

    bool MaterialShader::HasProgram(const String& programName) const
    {
        return m_Shader && m_Shader->HasProgram(programName);
    }

    ShaderProgramInstance* MaterialShader::GetProgram() { return m_SelectedProgram; }

    GPUPipelineState::Description MaterialShader::GetSurfacePipelineDescription(const BindParameters& params) const
    {
        auto        description      = GPUPipelineState::Description::Default;
        const auto& view             = params.renderContext.view;
        description.DepthWriteEnable = EnumHasNoneFlags(m_Info.FeaturesFlags, MaterialFeatures::DisableDepthWrite);
        if (EnumHasAnyFlags(m_Info.FeaturesFlags, MaterialFeatures::DisableDepthTest))
        {
            description.DepthFunc   = ComparisonFunc::Always;
            description.DepthEnable = description.DepthWriteEnable;
        }
        if (view.Pass == DrawPass::Depth)
        {
            description.DepthClipEnable  = false;
            description.DepthWriteEnable = true;
            description.DepthEnable      = true;
            description.DepthFunc        = ComparisonFunc::Less;
        }
        else if (view.Pass == DrawPass::MotionVectors)
        {
            description.DepthWriteEnable = false;
            description.DepthEnable      = true;
            description.DepthFunc        = ComparisonFunc::LessEqual;
        }
        description.Wireframe =
            EnumHasAnyFlags(m_Info.FeaturesFlags, MaterialFeatures::Wireframe) || view.Mode == ViewMode::Wireframe;
        description.CullMode = view.Pass == DrawPass::Depth ? CullMode::TwoSided : m_Info.CullMode;
        if (description.CullMode != CullMode::TwoSided && params.firstDrawCall &&
            params.firstDrawCall->WorldDeterminantSign < 0)
            description.CullMode = description.CullMode == CullMode::Normal ? CullMode::Inverted : CullMode::Normal;
        return description;
    }

    bool MaterialShader::BindProgram(BindParameters&                          params,
                                     const String&                            programName,
                                     const GPUPipelineState::Description& description)
    {
        m_SelectedProgram = nullptr;
        if (!m_IsLoaded || !m_Shader)
        {
            return false;
        }

        ProgramState* selected = nullptr;
        for (ProgramState& state : m_Programs)
        {
            if (state.Name == programName && state.Instanced == params.UseInstancing)
            {
                selected = &state;
                break;
            }
        }

        if (!selected)
        {
            List<String> defines;
            if (params.UseInstancing)
            {
                defines.Add(SE_TEXT("USE_INSTANCING"));
            }

            ProgramState state;
            state.Name      = programName;
            state.Instanced = params.UseInstancing;
            if (!m_Shader->GetOrCreateProgram(params.UseInstancing ? programName + SE_TEXT("Instanced") : programName, defines, state.Program))
            {
                return false;
            }
            state.Instance = New<ShaderProgramInstance>();

            state.Pipeline = GPUDevice::instance->CreatePipelineState();

            if (!state.Pipeline->Init(description, state.Program))
            {
                DeleteObjectSafe(state.Instance);
                DeleteObjectSafe(state.Pipeline);
                return false;
            }

            m_Programs.Add(state);
            selected = &m_Programs.Last();
        }

        // Reset every draw, so a cleared override or missing resource never inherits a previous draw's value.
        if (!selected->Instance->Initialize(selected->Program))
        {
            return false;
        }

        MaterialParameter::BindMeta meta;
        meta.Program = selected->Instance;
        meta.Input   = params.Input;
        meta.Buffers = m_Info.Domain == MaterialDomain::GUI ? nullptr : params.renderContext.buffers;
        meta.CanSampleDepth = m_Info.BlendMode != MaterialBlendMode::Opaque && GPUDevice::instance->GetGPULimits().HasReadOnlyDepth;
        meta.CanSampleGBuffer = m_Info.BlendMode != MaterialBlendMode::Opaque;

        if (!MaterialParams::Bind(params.paramsLink, meta))
        {
            return false;
        }

        params.gpuContext->SetState(selected->Pipeline);
        m_SelectedProgram = selected->Instance;
        return true;
    }
} // namespace SE
