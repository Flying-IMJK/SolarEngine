#include <Runtime/Render/Assets/Material/MaterialShader.h>

#include <Runtime/Core/Math/BoundingVolumes.h>
#include <Runtime/Core/Math/CollisionsHelper.h>
#include <Runtime/Core/Serialization/MemoryReadStream.h>
#include <Runtime/Graphics/GPUContext.h>
#include <Runtime/Graphics/GPUDevice.h>
#include <Runtime/Graphics/Shaders/SLC2GPUShader.h>
#include <Runtime/Graphics/Shaders/SLC2GPUShaderProgram.h>
#include <Runtime/Graphics/Shaders/ShaderProgramInstance.h>
#include <Runtime/Render/2D/Render2D.h>
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
        // A field can disappear from a selected Program after optimization. A present field must match its ABI.
        template<typename T>
        bool WriteOptional(ShaderProgramInstance& instance, const String& path, const T& value)
        {
            ShaderVar variable;
            return !instance.FindVar(path, variable) ||
                   (variable.GetLocation().UniformByteSize == sizeof(T) && variable.SetUniform(&value, sizeof(T)));
        }

        bool WriteMatrix(ShaderProgramInstance& instance, const String& path, const Matrix& value)
        {
            Matrix transposed;
            Matrix::Transpose(value, transposed);
            return WriteOptional(instance, path, transposed);
        }

        bool WriteLight(ShaderProgramInstance& instance, const String& path, const LightData& data)
        {
            return WriteOptional(instance, path + SE_TEXT(".SpotAngles"), data.SpotAngles) &&
                   WriteOptional(instance, path + SE_TEXT(".SourceRadius"), data.SourceRadius) &&
                   WriteOptional(instance, path + SE_TEXT(".SourceLength"), data.SourceLength) &&
                   WriteOptional(instance, path + SE_TEXT(".Color"), data.Color) &&
                   WriteOptional(instance, path + SE_TEXT(".MinRoughness"), data.MinRoughness) &&
                   WriteOptional(instance, path + SE_TEXT(".Position"), data.Position) &&
                   WriteOptional(instance, path + SE_TEXT(".CastShadows"), data.CastShadows) &&
                   WriteOptional(instance, path + SE_TEXT(".Direction"), data.Direction) &&
                   WriteOptional(instance, path + SE_TEXT(".Radius"), data.Radius) &&
                   WriteOptional(instance, path + SE_TEXT(".FalloffExponent"), data.FalloffExponent) &&
                   WriteOptional(instance, path + SE_TEXT(".InverseSquared"), data.InverseSquared) &&
                   WriteOptional(instance, path + SE_TEXT(".RadiusInv"), data.RadiusInv);
        }

        class DummyMaterial final : public MaterialShader
        {
        public:
            DummyMaterial() : MaterialShader(String::Empty) {}
            void Bind(BindParameters& params) override { params.Program = nullptr; }
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
            m_Shader = GPUDevice::instance->CreateSLC2Shader(name);
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
                LOG_ERROR("Render", "Unsupported SLC2 material domain {0}.", static_cast<int32>(info.Domain));
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
            LOG_ERROR("Render", "SLC2 material is missing required Programs.");
            return false;
        }
        m_IsLoaded = true;
        return true;
    }

    void MaterialShader::Unload()
    {
        m_IsLoaded = false;

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
    SLC2GPUShader* MaterialShader::GetShader() const { return m_Shader; }
    bool MaterialShader::IsReady() const { return m_IsLoaded; }

    bool MaterialShader::HasProgram(const String& programName) const
    {
        return m_Shader && m_Shader->HasProgram(programName);
    }

    ShaderProgramInstance* MaterialShader::GetProgram()
    {
        return m_SelectedProgram ? m_SelectedProgram->Instance : nullptr;
    }

    SLC2GPUPipelineState::Description MaterialShader::GetSurfacePipelineDescription(const BindParameters& params) const
    {
        auto        description      = SLC2GPUPipelineState::Description::Default;
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
                                     const SLC2GPUPipelineState::Description& description)
    {
        params.Program = nullptr;
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

            state.Pipeline = GPUDevice::instance->CreateSLC2PipelineState();

            if (!state.Pipeline->Init(description, selected->Program))
            {
                DeleteObjectSafe(state.Pipeline);
                return false;
            }

            m_Programs.Add(state);
            selected = &m_Programs.Last();
        }

        // Reset every draw, so a cleared override or missing resource never inherits a previous draw's value.
        if (!selected->Instance->Initialize(selected->Program) || !BindRuntimeParameters(*selected->Instance, params))
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

        m_SelectedProgram = selected;

        params.gpuContext->SetSLC2State(selected->Pipeline);
        params.Program = selected->Instance;
        return true;
    }

    bool MaterialShader::BindRuntimeParameters(ShaderProgramInstance& instance, const BindParameters& params) const
    {
        if (m_Info.Domain == MaterialDomain::GUI)
        {
            if (!params.customData)
                return false;
            const auto&  data     = *static_cast<const Render2D::CustomData*>(params.customData);
            const auto&  viewport = Render2D::GetViewport();
            const Float4 screen(viewport.Width, viewport.Height, 1.0f / viewport.Width, 1.0f / viewport.Height);
            const Float4 size(data.ViewSize.X, data.ViewSize.Y, 1.0f / data.ViewSize.X, 1.0f / data.ViewSize.Y);
            return WriteMatrix(instance, SE_TEXT("Data.ViewProjectionMatrix"), data.ViewProjection) &&
                   WriteMatrix(instance, SE_TEXT("Data.WorldMatrix"), Matrix::Identity) &&
                   WriteMatrix(instance, SE_TEXT("Data.ViewMatrix"), Matrix::Identity) &&
                   WriteOptional(instance, SE_TEXT("Data.ViewDir"), Float3::Forward) &&
                   WriteOptional(instance, SE_TEXT("Data.TimeParam"), params.timeParam) &&
                   WriteOptional(instance, SE_TEXT("Data.ScreenSize"), screen) &&
                   WriteOptional(instance, SE_TEXT("Data.ViewSize"), size);
        }
        if (!params.firstDrawCall)
            return false;
        const auto& view = params.renderContext.view;
        const auto& draw = *params.firstDrawCall;
        if (!WriteMatrix(instance, SE_TEXT("ViewData.ViewMatrix"), view.View) ||
            !WriteMatrix(instance, SE_TEXT("ViewData.ViewProjectionMatrix"), view.frustum.GetMatrix()) ||
            !WriteMatrix(instance, SE_TEXT("ViewData.PrevViewProjectionMatrix"), view.PrevViewProjection) ||
            !WriteMatrix(instance, SE_TEXT("ViewData.MainViewProjectionMatrix"), view.MainViewProjection) ||
            !WriteOptional(instance, SE_TEXT("ViewData.MainScreenSize"), view.MainScreenSize) ||
            !WriteOptional(instance, SE_TEXT("ViewData.ViewPos"), view.Position) ||
            !WriteOptional(instance, SE_TEXT("ViewData.ViewFar"), view.Far) ||
            !WriteOptional(instance, SE_TEXT("ViewData.ViewDir"), view.Direction) ||
            !WriteOptional(instance, SE_TEXT("ViewData.TimeParam"), params.timeParam) ||
            !WriteOptional(instance, SE_TEXT("ViewData.ViewInfo"), view.ViewInfo) ||
            !WriteOptional(instance, SE_TEXT("ViewData.ScreenSize"), view.ScreenSize) ||
            !WriteOptional(instance, SE_TEXT("ViewData.TemporalAAJitter"), view.TemporalAAJitter) ||
            !WriteMatrix(instance, SE_TEXT("Data.WorldMatrix"), draw.World) ||
            !WriteMatrix(instance, SE_TEXT("Data.PrevWorldMatrix"), draw.Surface.PrevWorld) ||
            !WriteOptional(instance, SE_TEXT("Data.WorldDeterminantSign"), draw.WorldDeterminantSign) ||
            !WriteOptional(instance, SE_TEXT("Data.LODDitherFactor"), draw.Surface.LODDitherFactor) ||
            !WriteOptional(instance, SE_TEXT("Data.PerInstanceRandom"), draw.PerInstanceRandom) ||
            !WriteOptional(instance, SE_TEXT("Data.GeometrySize"), draw.Surface.GeometrySize))
        {
            return false;
        }

        if (m_Info.BlendMode != MaterialBlendMode::Opaque && params.renderContext.list)
        {
            const auto& list        = *params.renderContext.list;
            LightData   directional = {};
            if (list.DirectionalLights.HasItems())
                list.DirectionalLights.First().SetupLightData(&directional, false);
            if (!WriteLight(instance, SE_TEXT("Data.DirectionalLight"), directional))
                return false;
            uint32               count = 0;
            const BoundingSphere bounds(draw.ObjectPosition, draw.ObjectRadius);
            for (const auto& light : list.PointLights)
            {
                if (count == 4)
                    break;
                if (!CollisionsHelper::SphereIntersectsSphere(bounds, BoundingSphere(light.Position, light.Radius)))
                {
                    continue;
                }
                LightData data = {};
                light.SetupLightData(&data, false);
                if (!WriteLight(instance, String::Format(SE_TEXT("Data.LocalLights[{0}]"), count++), data))
                {
                    return false;
                }
            }
            for (const auto& light : list.SpotLights)
            {
                if (count == 4)
                {
                    break;
                }
                if (!CollisionsHelper::SphereIntersectsSphere(bounds, BoundingSphere(light.Position, light.Radius)))
                {
                    continue;
                }
                LightData data = {};
                light.SetupLightData(&data, false);
                if (!WriteLight(instance, String::Format(SE_TEXT("Data.LocalLights[{0}]"), count++), data))
                {
                    return false;
                }
            }
            return WriteOptional(instance, SE_TEXT("Data.LocalLightsCount"), count);
        }
        return true;
    }
} // namespace SE
