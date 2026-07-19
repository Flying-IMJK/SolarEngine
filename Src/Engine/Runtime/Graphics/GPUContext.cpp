
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
#include "Runtime/Resource/Assets/Materials/Shader.h"
#include "Runtime/Resource/Importers/AssetsImportingSystem.h"
#include "Shaders/GPUShader.h"

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

	struct GPUContext::PrivateData
	{
		AssetRef<Shader> QuadShader;
		GPUPipelineState* PS_CopyLinear = nullptr;
		GPUPipelineState* PS_Clear = nullptr;
		GPUBuffer* FullscreenTriangleVB = nullptr;
		AssetRef<Material> DefaultMaterial;
		// SoftAssetReference<Material> DefaultDeformableMaterial;
		AssetRef<Texture> DefaultNormalMap;
		AssetRef<Texture> DefaultWhiteTexture;
		AssetRef<Texture> DefaultBlackTexture;
	};


	GPUContext::GPUContext(GPUDevice* device) :
		m_Device(device),
		m_PrivateData(New<PrivateData>())
	{

	}

	bool GPUContext::LoadDefaultResources()
	{
		// Load internal rendering shader for GPU device low-level impl
		m_PrivateData->QuadShader = AssetContent::LoadAsyncInternal<Shader>(SE_TEXT("Shaders/Quad"));
		if (m_PrivateData->QuadShader == nullptr || !m_PrivateData->QuadShader->WaitForLoaded())
		{
			return false;
		}

		GPUShader* QuadShader = m_PrivateData->QuadShader->GetShader();
		GPUPipelineState::Description::DefaultFullscreenTriangle.VS = QuadShader->GetVS(SE_TEXT("VS"));
		m_PrivateData->PS_CopyLinear = m_Device->CreatePipelineState();
		GPUPipelineState::Description desc = GPUPipelineState::Description::DefaultFullscreenTriangle;
		desc.PS = QuadShader->GetPS(SE_TEXT("PS_CopyLinear"));
		if (!m_PrivateData->PS_CopyLinear->Init(desc))
		{
			return false;
		}
		m_PrivateData->PS_Clear = m_Device->CreatePipelineState();
		desc.PS = QuadShader->GetPS(SE_TEXT("PS_Clear"));
		if (!m_PrivateData->PS_Clear->Init(desc))
		{
			return false;
		}

		// Create fullscreen triangle vertex buffer
		{
		    // Create vertex buffer
		    // @formatter:off
		    static float vb[] =
		    {
		        //   XY            UV
		        -1.0f, -1.0f,  0.0f,  1.0f,
		        -1.0f,  3.0f,  0.0f, -1.0f,
		         3.0f, -1.0f,  2.0f,  1.0f,
		    };
		    // @formatter:on
		    m_PrivateData->FullscreenTriangleVB = m_Device->CreateBuffer(SE_TEXT("QuadVB"));
		    if (!m_PrivateData->FullscreenTriangleVB->Init(GPUBufferDescription::Vertex(sizeof(float) * 4, 3, vb)))
		    {
		    	return false;
		    }
		}

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

		return true;
	}

	MaterialBase* GPUContext::GetDefaultMaterial() const
	{
		return m_PrivateData->DefaultMaterial;
	}

	void GPUContext::FrameBegin()
	{
		_lastRenderTime = Platform::GetTimeSeconds();
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
		const Rectangle rect(viewport.location.x, viewport.location.y, viewport.width, viewport.height);
		SetScissor(rect);
	}

	void GPUContext::SetViewport(float width, float height)
	{
		const Viewport viewport(0.0f, 0.0f, width, height);
		SetViewport(viewport);
	}

	void GPUContext::DrawFullscreenTriangle()
	{
		auto vb = m_PrivateData->FullscreenTriangleVB;
		BindVB(ToSpan(&vb, 1));
		DrawInstanced(3, 1);
	}

	void GPUContext::Draw(GPUTexture* dst, GPUTexture* src)
	{
/*		ENGINE_ASSERT(dst && src);
		ResetRenderTarget();
		const float width = (float)dst->Width();
		const float height = (float)dst->Height();
		SetViewport(width, height);
		SetRenderTarget(dst->View());
		BindSR(0, src->View());
		SetState(_device->GetCopyLinearPS());
		DrawFullscreenTriangle();*/
	}

	void GPUContext::Draw(GPUTexture* rt)
	{
		ENGINE_ASSERT(rt);
		BindSR(0, rt);
		SetState(m_PrivateData->PS_CopyLinear);
		DrawFullscreenTriangle();
	}

	void GPUContext::Draw(GPUTextureView* rt)
	{
/*		ENGINE_ASSERT(rt);
		BindSR(0, rt);
		SetState(_device->GetCopyLinearPS());
		DrawFullscreenTriangle();*/
	}

	/*	void GPUContext::SetResourceState(GPUResource* resource, uint64 state, int32 subresource)
	{
	}

	void GPUContext::ForceRebindDescriptors()
	{
	}*/

} // SE