
#include "GPUContext.h"

#include "GPUDevice.h"
#include "Base/GPUBuffer.h"
#include "Base/GPUBufferDescription.h"
#include "Runtime/Core/Math/Rectangle.h"
#include "Runtime/EngineContext.h"
#include "Runtime/Graphics//Textures/GPUTexture.h"
#include "Runtime/Graphics/Base/GPUPipelineState.h"
#include "Runtime/Render/Assets/Texture/Texture.h"
#include "Runtime/Resource/AssetRef.h"
#include "Runtime/Resource/Assets/Materials/Material.h"
#include "Runtime/Resource/Assets/SLC2Materials/GraphicShader.h"
#include "Runtime/Resource/Importers/AssetsImportingSystem.h"

#include "Base/RenderGeometry.h"
#include "Shaders/GPUShader.h"
#include "Textures/GPUSampler.h"

namespace SE
{
	GPUPipelineState::Description GPUPipelineState::Description::Default =
		{
			true, // DepthEnable
			true, // DepthWriteEnable
			true, // DepthClipEnable
			ComparisonFunc::Less, // DepthFunc
			false, // StencilEnable
			0xff, // StencilReadMask
			0xff, // StencilWriteMask
			ComparisonFunc::Always, // StencilFunc
			StencilOperation::Keep, // StencilFailOp
			StencilOperation::Keep, // StencilDepthFailOp
			StencilOperation::Keep, // StencilPassOp
			nullptr, // VS
			nullptr, // HS
			nullptr, // DS
			nullptr, // GS
			nullptr, // PS
			PrimitiveTopologyType::Triangle, // PrimitiveTopology
			false, // Wireframe
			CullMode::Normal, // CullMode
			BlendingMode::Opaque, // BlendMode
		};

	GPUPipelineState::Description GPUPipelineState::Description::DefaultNoDepth =
		{
			false, // DepthEnable
			false, // DepthWriteEnable
			false, // DepthClipEnable
			ComparisonFunc::Less, // DepthFunc
			false, // StencilEnable
			0xff, // StencilReadMask
			0xff, // StencilWriteMask
			ComparisonFunc::Always, // StencilFunc
			StencilOperation::Keep, // StencilFailOp
			StencilOperation::Keep, // StencilDepthFailOp
			StencilOperation::Keep, // StencilPassOp
			nullptr, // VS
			nullptr, // HS
			nullptr, // DS
			nullptr, // GS
			nullptr, // PS
			PrimitiveTopologyType::Triangle, // PrimitiveTopology
			false, // Wireframe
			CullMode::Normal, // CullMode
			BlendingMode::Opaque, // BlendMode
		};

	GPUPipelineState::Description GPUPipelineState::Description::DefaultFullscreenTriangle =
		{
			false, // DepthEnable
			false, // DepthWriteEnable
			false, // DepthClipEnable
			ComparisonFunc::Less, // DepthFunc
			false, // StencilEnable
			0xff, // StencilReadMask
			0xff, // StencilWriteMask
			ComparisonFunc::Always, // StencilFunc
			StencilOperation::Keep, // StencilFailOp
			StencilOperation::Keep, // StencilDepthFailOp
			StencilOperation::Keep, // StencilPassOp
			nullptr, // VS (Set to default quad VS via GPUDevice)
			nullptr, // HS
			nullptr, // DS
			nullptr, // GS
			nullptr, // PS
			PrimitiveTopologyType::Triangle, // PrimitiveTopology
			false, // Wireframe
			CullMode::TwoSided, // CullMode
			BlendingMode::Opaque, // BlendMode
		};

	SLC2GPUPipelineState::Description SLC2GPUPipelineState::Description::Default = {
        true,                            // DepthEnable
        true,                            // DepthWriteEnable
        true,                            // DepthClipEnable
        ComparisonFunc::Less,            // DepthFunc
        false,                           // StencilEnable
        0xff,                            // StencilReadMask
        0xff,                            // StencilWriteMask
        ComparisonFunc::Always,          // StencilFunc
        StencilOperation::Keep,          // StencilFailOp
        StencilOperation::Keep,          // StencilDepthFailOp
        StencilOperation::Keep,          // StencilPassOp
        PrimitiveTopologyType::Triangle, // PrimitiveTopology
        false,                           // Wireframe
        CullMode::Normal,                // CullMode
        BlendingMode::Opaque,            // BlendMode
    };

    SLC2GPUPipelineState::Description SLC2GPUPipelineState::Description::DefaultNoDepth = {
        false,                           // DepthEnable
        false,                           // DepthWriteEnable
        false,                           // DepthClipEnable
        ComparisonFunc::Less,            // DepthFunc
        false,                           // StencilEnable
        0xff,                            // StencilReadMask
        0xff,                            // StencilWriteMask
        ComparisonFunc::Always,          // StencilFunc
        StencilOperation::Keep,          // StencilFailOp
        StencilOperation::Keep,          // StencilDepthFailOp
        StencilOperation::Keep,          // StencilPassOp
        PrimitiveTopologyType::Triangle, // PrimitiveTopology
        false,                           // Wireframe
        CullMode::Normal,                // CullMode
        BlendingMode::Opaque,            // BlendMode
    };

    SLC2GPUPipelineState::Description SLC2GPUPipelineState::Description::DefaultFullscreenTriangle = {
        false,                           // DepthEnable
        false,                           // DepthWriteEnable
        false,                           // DepthClipEnable
        ComparisonFunc::Less,            // DepthFunc
        false,                           // StencilEnable
        0xff,                            // StencilReadMask
        0xff,                            // StencilWriteMask
        ComparisonFunc::Always,          // StencilFunc
        StencilOperation::Keep,          // StencilFailOp
        StencilOperation::Keep,          // StencilDepthFailOp
        StencilOperation::Keep,          // StencilPassOp
        PrimitiveTopologyType::Triangle, // PrimitiveTopology
        false,                           // Wireframe
        CullMode::TwoSided,              // CullMode
        BlendingMode::Opaque,            // BlendMode
    };

	struct GPUContext::PrivateData
	{
        RenderGeometry FullscreenTriangleGeometry;

		AssetRef<GraphicShader> QuadShader;
		SLC2GPUPipelineState* PS_CopyLinear = nullptr;
        SLC2GPUPipelineState* PS_Clear      = nullptr;
        ShaderProgramInstance* SP_CopyLinear = nullptr;
        ShaderProgramInstance* SP_Clear      = nullptr;

		GPUSampler* SamplerLinearClamp;
        GPUSampler* SamplerPointClamp;
        GPUSampler* SamplerLinearWrap;
        GPUSampler* SamplerPointWrap;

		AssetRef<Material> DefaultMaterial;
		// SoftAssetReference<Material> DefaultDeformableMaterial;
		AssetRef<Texture> DefaultNormalMap;
		AssetRef<Texture> DefaultWhiteTexture;
		AssetRef<Texture> DefaultBlackTexture;
	};


	GPUContext::GPUContext(GPUDevice* device) : ScriptingObject(ScriptingObjectSpawnParams(UID::New(), TypeInitializer)),
	                                            m_Device(device),
	                                            m_PrivateData(New<PrivateData>())
	{
	}

	bool GPUContext::LoadDefaultResources()
	{
		// Load internal rendering shader for GPU device low-level impl
		m_PrivateData->QuadShader = AssetContent::LoadAsyncInternal<GraphicShader>(SE_TEXT("Shaders/Quad"));
		if (m_PrivateData->QuadShader == nullptr || !m_PrivateData->QuadShader->WaitForLoaded())
		{
			return false;
		}

        SLC2GPUShaderProgram* progarm;
        if (!m_PrivateData->QuadShader->SelectProgram(SE_TEXT("CopyLinear"), {}, progarm))
        {
            return false;
        }

		m_PrivateData->PS_CopyLinear = m_Device->CreateSLC2PipelineState();
        if (!m_PrivateData->PS_CopyLinear->Init(SLC2GPUPipelineState::Description::DefaultFullscreenTriangle, progarm))
        {
            return false;
        }
        m_PrivateData->SP_CopyLinear = New<ShaderProgramInstance>();

		m_PrivateData->SP_CopyLinear->Initialize(progarm);

		if (!m_PrivateData->QuadShader->SelectProgram(SE_TEXT("Clear"), {}, progarm))
        {
            return false;
        }

		m_PrivateData->PS_Clear      = m_Device->CreateSLC2PipelineState();
        if (!m_PrivateData->PS_Clear->Init(SLC2GPUPipelineState::Description::DefaultFullscreenTriangle, progarm))
        {
            return false;
        }
        m_PrivateData->SP_Clear      = New<ShaderProgramInstance>();
        m_PrivateData->SP_Clear->Initialize(progarm);


		// Load default material
		m_PrivateData->DefaultMaterial = AssetContent::LoadAsyncInternal<Material>(SE_TEXT("Shaders/Materials/DefaultMaterial"));

		if (m_PrivateData->DefaultMaterial == nullptr)
		{
			String outPath = EngineContext::ProjectCacheFolder / "Shaders/Materials/DefaultMaterial" + ASSET_FILES_EXTENSION_WITH_DOT;
			AssetsImporting::Create(AssetsImporting::CreateMaterialTag, outPath);

			m_PrivateData->DefaultMaterial = AssetContent::LoadAsyncInternal<Material>(SE_TEXT("Shaders/Materials/DefaultMaterial"));
			if (m_PrivateData->DefaultMaterial == nullptr)
			{
				return false;
			}
		}

		m_PrivateData->DefaultMaterial->WaitForLoaded();

		// m_PrivateData->DefaultDeformableMaterial = UID(0x639e12c0, 0x42d34bae, 0x89dd8b81, 0x7e1efc2d);

		// Load default normal map
		/*m_PrivateData->DefaultNormalMap = AssetContent::LoadAsyncInternal<Texture>(SE_TEXT("Engine/Textures/NormalTexture"));
		if (m_PrivateData->DefaultNormalMap == nullptr)
		{
			return false;
		}

		// Load default solid white
		m_PrivateData->DefaultWhiteTexture = AssetContent::LoadAsyncInternal<Texture>(SE_TEXT("Engine/Textures/WhiteTexture"));
		if (m_PrivateData->DefaultWhiteTexture == nullptr)
		{
			return false;
		}

		// Load default solid black
		m_PrivateData->DefaultBlackTexture = AssetContent::LoadAsyncInternal<Texture>(SE_TEXT("Engine/Textures/BlackTexture"));
		if (m_PrivateData->DefaultBlackTexture == nullptr)
		{
			return false;
		}*/


		// Create fullscreen triangle vertex buffer
        {
            // Create vertex buffer
            // @formatter:off
            static float vb[] = {
                //   XY            UV
                -1.0f,
                -1.0f,
                0.0f,
                1.0f,
                -1.0f,
                3.0f,
                0.0f,
                -1.0f,
                3.0f,
                -1.0f,
                2.0f,
                1.0f,
            };
            // @formatter:on

		    RenderGeometryVertexBuffer vertexBuffer;
            vertexBuffer.Slot   = 0;
            vertexBuffer.Offset = 0;
            vertexBuffer.Buffer = m_Device->CreateBuffer(SE_TEXT("QuadVB"));
            if (!vertexBuffer.Buffer->Init(GPUBufferDescription::Vertex(sizeof(float) * 4, 3, vb)))
            {
                return false;
            }

            m_PrivateData->FullscreenTriangleGeometry.VertexBuffers.Add(vertexBuffer);

			VertexFactoryLayout* vertexLayout = New<VertexFactoryLayout>();

			List<VertexFactoryBufferBinding> bindings = {
                {0, sizeof(float) * 4, VertexFactoryInputRate::PerVertex, 0}
            };
            List<VertexFactoryInputElement> elements = {
                {SE_TEXT("POSITION"), 0, PixelFormat::R32G32_Float, 0, 0},
                {SE_TEXT("TEXCOORD"), 0, PixelFormat::R32G32_Float, 0, sizeof(float) * 2},
            };

            if (!VertexFactoryLayout::Create(bindings, elements, *vertexLayout))
            {
                return false;
            }

			m_PrivateData->FullscreenTriangleGeometry.Layout = vertexLayout;

        }

		// Create Default Sampler
        {
            m_PrivateData->SamplerLinearClamp = m_Device->CreateSampler();
            m_PrivateData->SamplerPointClamp  = m_Device->CreateSampler();
            m_PrivateData->SamplerLinearWrap  = m_Device->CreateSampler();
            m_PrivateData->SamplerPointWrap   = m_Device->CreateSampler();

            GPUSamplerDescription linearClampDesc =
                GPUSamplerDescription::New(GPUSamplerFilter::Bilinear, GPUSamplerAddressMode::Clamp);
            GPUSamplerDescription pointClampDesc =
                GPUSamplerDescription::New(GPUSamplerFilter::Point, GPUSamplerAddressMode::Clamp);
            GPUSamplerDescription linearWrapDesc =
                GPUSamplerDescription::New(GPUSamplerFilter::Bilinear, GPUSamplerAddressMode::Wrap);
            GPUSamplerDescription pointWrapDesc =
                GPUSamplerDescription::New(GPUSamplerFilter::Point, GPUSamplerAddressMode::Wrap);

            if (!m_PrivateData->SamplerLinearClamp->Init(linearClampDesc))
            {
                return false;
            }
            if (!m_PrivateData->SamplerPointClamp->Init(pointClampDesc))
            {
                return false;
            }
            if (!m_PrivateData->SamplerLinearWrap->Init(linearWrapDesc))
            {
                return false;
            }
            if (!m_PrivateData->SamplerPointWrap->Init(pointWrapDesc))
            {
                return false;
            }
        }

		return true;
	}

	MaterialBase* GPUContext::GetDefaultMaterial() const
	{
		return m_PrivateData->DefaultMaterial;
	}

	void GPUContext::FrameBegin()
	{
		m_LastRenderTime = Platform::GetTimeSeconds();
	}

	void GPUContext::FrameEnd()
	{
		ClearState();
		FlushState();
	}

	void GPUContext::BindSR(int32 slot, GPUTexture* texture)
	{
		ENGINE_ASSERT(texture == nullptr || texture->ResidentMipLevels() == 0 || texture->IsShaderResource());
		BindSR(slot, GET_TEXTURE_VIEW_SAFE(texture));
	}

    void GPUContext::SetViewportAndScissors(float width, float height)
	{
		const Viewport viewport(0.0f, 0.0f, width, height);
		SetViewport(viewport);
		const Rectangle rect(0.0f, 0.0f, width, height);
		SetScissor(rect);
	}

	void GPUContext::SetViewportAndScissors(const Viewport& viewport)
	{
		SetViewport(viewport);
		const Rectangle rect(viewport.Location.X, viewport.Location.Y, viewport.Width, viewport.Height);
		SetScissor(rect);
	}

	void GPUContext::SetViewport(float width, float height)
	{
		const Viewport viewport(0.0f, 0.0f, width, height);
		SetViewport(viewport);
	}

	void GPUContext::DrawFullscreenTriangle(ShaderProgramInstance* instance)
	{
        BindRenderGeometry(m_PrivateData->FullscreenTriangleGeometry);
        DrawInstanced(instance, 3, 1);
	}

	void GPUContext::Draw(GPUTexture* dst, GPUTexture* src)
	{
		ENGINE_ASSERT(dst && src);
		ResetRenderTarget();
		const float width = (float)dst->Width();
		const float height = (float)dst->Height();
		SetViewport(width, height);
		SetRenderTarget(dst->View());
        m_PrivateData->SP_CopyLinear->SetTexture(SE_TEXT("Source"), src->View());
        SetSLC2State(m_PrivateData->PS_CopyLinear);
        DrawFullscreenTriangle(m_PrivateData->SP_CopyLinear);
	}

	void GPUContext::Draw(GPUTexture* rt)
	{
		ENGINE_ASSERT(rt);
        m_PrivateData->SP_CopyLinear->SetTexture(SE_TEXT("Source"), rt->View());
		SetSLC2State(m_PrivateData->PS_CopyLinear);
        DrawFullscreenTriangle(m_PrivateData->SP_CopyLinear);
	}

	void GPUContext::Draw(GPUTextureView* rt)
	{
		ENGINE_ASSERT(rt);
        m_PrivateData->SP_CopyLinear->SetTexture(SE_TEXT("Source"), rt);
        SetSLC2State(m_PrivateData->PS_CopyLinear);
        DrawFullscreenTriangle(m_PrivateData->SP_CopyLinear);
	}

	
    void GPUContext::GlobalBinding(ShaderProgramInstance* program)
	{
	    // Default

		ShaderVar var;
        if (program->FindVar(SE_TEXT("SamplerLinearClamp"), var))
        {
            var.SetSampler(m_PrivateData->SamplerLinearClamp);
        }

		if (program->FindVar(SE_TEXT("SamplerPointClamp"), var))
        {
            var.SetSampler(m_PrivateData->SamplerPointClamp);
        }

		if (program->FindVar(SE_TEXT("SamplerLinearWrap"), var))
        {
            var.SetSampler(m_PrivateData->SamplerLinearWrap);
        }

		if (program->FindVar(SE_TEXT("SamplerPointWrap"), var))
        {
            var.SetSampler(m_PrivateData->SamplerPointWrap);
        }

	}


	/*	void GPUContext::SetResourceState(GPUResource* resource, uint64 state, int32 subresource)
	{
	}

	void GPUContext::ForceRebindDescriptors()
	{
	}*/

} // SE
