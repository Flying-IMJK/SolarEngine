#include <Runtime/Render/RenderFeatures/PhysicalSky.h>

#include <Runtime/Core/Profiler/Profiler.h>
#include <Runtime/Graphics/Base/GPUPipelineState.h>
#include <Runtime/Graphics/GPUContext.h>
#include <Runtime/Graphics/GPUDevice.h>
#include <Runtime/Graphics/Textures/GPUSampler.h>
#include <Runtime/Graphics/Textures/GPUTexture.h>
#include <Runtime/Render/RenderBuffers.h>
#include <Runtime/Render/RenderContext.h>
#include <Runtime/Resource/AssetContent.h>

namespace SE
{
    PhysicalSky::~PhysicalSky()
    {
        if (m_Shader)
        {
            m_Shader->OnReloadingEvent.Unbind<PhysicalSky, &PhysicalSky::OnShaderReloading>(this);
        }
        OnShaderReloading(nullptr);
        DeleteObjectSafe(m_Sampler);
    }

    void PhysicalSky::OnShaderReloading(Asset* pAsset)
    {
        // Release objects that reference immutable programs before the asset replaces them.
        DeleteObjectSafe(m_Pipeline);
        DeleteObjectSafe(m_Program);
    }

    bool PhysicalSky::SetupResources()
    {
        auto device = GPUDevice::instance;
        if (!m_Shader)
        {
            m_Shader = AssetContent::LoadAsyncInternal<Shader>(SE_TEXT("Shaders/PhysicalSky"));
            if (!m_Shader)
            {
                LOG_ERROR("Render", "Cannot load PhysicalSky shader.");
                return false;
            }
            m_Shader->OnReloadingEvent.Bind<PhysicalSky, &PhysicalSky::OnShaderReloading>(this);
        }
        if (!m_Shader->IsLoaded())
        {
            return false;
        }
        if (!m_Sampler)
        {
            m_Sampler = device->CreateSampler();
            if (!m_Sampler || !m_Sampler->Init(GPUSamplerDescription::New(GPUSamplerFilter::Bilinear, GPUSamplerAddressMode::Clamp)))
            {
                DeleteObjectSafe(m_Sampler);
                return false;
            }
        }
        if (m_Pipeline && m_Program)
        {
            return true;
        }

        GPUShaderProgram* program = nullptr;
        if (!m_Shader->SelectProgram(SE_TEXT("PhysicalSky"), {}, program))
        {
            return false;
        }
        auto description = GPUPipelineState::Description::DefaultFullscreenTriangle;
        description.DepthEnable = true;
        description.DepthWriteEnable = false;
        description.DepthFunc = ComparisonFunc::Equal;
        m_Pipeline = device->CreatePipelineState();
        if (!m_Pipeline || !m_Pipeline->Init(description, program))
        {
            DeleteObjectSafe(m_Pipeline);
            return false;
        }
        m_Program = New<ShaderProgramInstance>();
        if (!m_Program->Initialize(program))
        {
            OnShaderReloading(nullptr);
            return false;
        }
        return true;
    }

    void PhysicalSky::Render(RenderContext& renderContext)
    {
        if (!SetupResources())
        {
            return;
        }

        auto buffers = renderContext.buffers;
        if (!buffers->SkyAtmosphereLutsSubmitted)
        {
            return;
        }
        GPUTexture* textures[] = {buffers->SkyAtmosphereTransmittanceLut, buffers->SkyAtmosphereMultiScatteringLut, buffers->SkyAtmosphereSkyViewLut};
        for (auto texture : textures)
        {
            if (!texture || !texture->IsAllocated())
            {
                return;
            }
        }

        PROFILE_GPU_CPU("PhysicalSky");
        struct SkyData
        {
            SkyAtmosphereParameters Atmosphere;
            SkyAtmosphereViewParameters View;
        } data = {buffers->SkyAtmosphereCachedParameters, buffers->SkyAtmosphereView};

        // Use the same view/atmosphere snapshot that produced this view's LUTs.
        if (!m_Program->SetConstantBuffer(SE_TEXT("data"), &data, sizeof(data)) ||
            !m_Program->SetTexture(SE_TEXT("TransmittanceLut"), textures[0]->View()) ||
            !m_Program->SetTexture(SE_TEXT("MultiScatteringLut"), textures[1]->View()) ||
            !m_Program->SetTexture(SE_TEXT("SkyViewLut"), textures[2]->View()) ||
            !m_Program->SetSampler(SE_TEXT("AtmosphereSampler"), m_Sampler))
        {
            return;
        }
        renderContext.gpuContext->SetState(m_Pipeline);
        renderContext.gpuContext->DrawFullscreenTriangle(m_Program);
    }
} // namespace SE
