

#include "GizmosPass.h"

#include "Runtime/Core/Profiler/Profiler.h"
#include "Runtime/Graphics/DynamicBuffer.h"
#include "Runtime/Graphics/GPUContext.h"
#include "Runtime/Graphics/GPUDevice.h"
#include "Runtime/Graphics/Base/GPUPipelineState.h"
#include "Runtime/Graphics/Shaders/GPUShader.h"
#include "Runtime/Graphics/Textures/GPUTexture.h"
#include "Runtime/Render/RenderContext.h"

namespace SE
{



	/*String GizmosPass::ToString() const
	{
		return RendererPass<GizmosPass>::ToString();
	}

	bool GizmosPass::Init()
	{
		m_Shader = AssetContent::LoadAsyncInternal<Shader>(SE_TEXT("Shaders/Gizmos"));

		if (m_Shader == nullptr)
		{
			return false;
		}

		m_LinerPSO = GPUDevice::instance->CreatePipelineState();
		m_PointPSO = GPUDevice::instance->CreatePipelineState();
		m_PrimitivePSO = GPUDevice::instance->CreatePipelineState();
		m_TextPSO = GPUDevice::instance->CreatePipelineState();

		m_LinerDepthPSO = GPUDevice::instance->CreatePipelineState();
		m_PointDepthPSO = GPUDevice::instance->CreatePipelineState();
		m_TextDepthPSO = GPUDevice::instance->CreatePipelineState();
		m_PrimitiveDepthPSO = GPUDevice::instance->CreatePipelineState();

		m_ViewBuffer = GPUDevice::instance->CreateConstantBuffer(sizeof(GizmosViewData), SE_TEXT("GizmosViewData"));
		m_PointVertexBuffer = New<DynamicVertexBuffer>(m_MaxPointsPerDrawCall, sizeof(Gizmos::PointCommand), SE_TEXT("PointVertexBuffer"));
		m_LineVertexBuffer = New<DynamicVertexBuffer>(m_MaxLinesPerDrawCall, sizeof(Gizmos::LineCommand), SE_TEXT("LineVertexBuffer"));
		m_PrimitiveVertexBuffer = New<DynamicVertexBuffer>(m_MaxTrianglesPerDrawCall, sizeof(Gizmos::TriangleCommand), SE_TEXT("PrimitiveVertexBuffer"));

		return true;
	}

	void GizmosPass::Dispose()
	{

	}

	bool GizmosPass::SetupResources()
	{
		if (!m_Shader->IsLoaded() && !m_Shader->WaitForLoaded())
		{
			return false;
		}

		const auto shader = m_Shader->GetShader();
		GPUPipelineState::Description desc = GPUPipelineState::Description::Default;
		desc.BlendMode = BlendingMode::Additive;
		desc.CullMode = CullMode::TwoSided;

		if (!InitPso(desc, shader, m_LinerPSO, m_PointPSO, m_TextPSO, m_PrimitivePSO))
		{
			return false;
		}

		desc.DepthEnable = true;
		desc.DepthWriteEnable =  false;
		desc.DepthClipEnable =  false;
		if (!InitPso(desc, shader, m_LinerDepthPSO, m_PointDepthPSO, m_TextDepthPSO, m_PrimitiveDepthPSO))
		{
			return false;
		}

		return true;
	}

	bool GizmosPass::InitPso(GPUPipelineState::Description desc, GPUShader* shader,
		GPUPipelineState* & linerPso, GPUPipelineState* & pointPso, GPUPipelineState* & textPso, GPUPipelineState* & primitivePso)
	{
		// Linear
		if (!linerPso->IsValid())
		{
			desc.PrimitiveTopology = PrimitiveTopologyType::Line;

			desc.VS = shader->GetVS(SE_TEXT("Lines_VS"));
			desc.GS = shader->GetGS(SE_TEXT("Lines_GS"));
			desc.PS = shader->GetPS(SE_TEXT("Lines_PS"));
			if (!linerPso->Init(desc))
			{
				return false;
			}
		}

		// Point
		if (!pointPso->IsValid())
		{
			desc.VS = shader->GetVS(SE_TEXT("Point_VS"));
			desc.GS = shader->GetGS(SE_TEXT("Point_GS"));
			desc.PS = shader->GetPS(SE_TEXT("Point_PS"));
			desc.PrimitiveTopology = PrimitiveTopologyType::Point;

			if (!pointPso->Init(desc))
			{
				return false;
			}
		}

		// Primitive
		if (!primitivePso->IsValid())
		{
			desc.VS = shader->GetVS(SE_TEXT("Triangles_VS"));
			desc.PS = shader->GetPS(SE_TEXT("Triangles_PS"));
			desc.GS = nullptr;
			desc.PrimitiveTopology = PrimitiveTopologyType::Triangle;

			if (!primitivePso->Init(desc))
			{
				return false;
			}
		}

		if (!textPso->IsValid())
		{
			if (!textPso->Init(desc))
			{
				return false;
			}
		}

		return true;
	}

	void GizmosPass::DrawPoints(RenderContext const& renderContext, List<Gizmos::PointCommand> const& commands)
	{
		//-------------------------------------------------------------------------
		GPUContext* gpuContext = renderContext.gpuContext;

		m_PointVertexBuffer->Clear();
		int32 const numPoints = commands.Count();
		if ( numPoints > 0 )
		{
			// Fill the points vertex buffer
			uint32 const numDrawCalls = Math::Ceil((float) numPoints / m_MaxPointsPerDrawCall);
			for ( auto i = 0u; i < numDrawCalls; i++ )
			{
				uint32 const drawRangeStart = i * m_MaxPointsPerDrawCall;
				uint32 const drawRangeEnd = drawRangeStart + Math::Min(numPoints - i* m_MaxPointsPerDrawCall, m_MaxPointsPerDrawCall);
				uint32 const drawRangeLength = drawRangeEnd - drawRangeStart;
				uint32 const drawCommandMemorySize = drawRangeLength * sizeof(Gizmos::PointCommand);

				Gizmos::PointCommand* pData = m_PointVertexBuffer->WriteReserve<Gizmos::PointCommand>(drawRangeLength);
				Platform::MemoryCopy(pData, &commands[drawRangeStart], drawCommandMemorySize);

				m_PointVertexBuffer->Flush(gpuContext);

				gpuContext->BindVB(m_PointVertexBuffer->GetBuffer());
				gpuContext->Draw(0,drawRangeLength);
			}
		}
	}

	void GizmosPass::DrawLines(RenderContext const& renderContext, List<Gizmos::LineCommand> const& commands)
	{
		//-------------------------------------------------------------------------
		GPUContext* gpuContext = renderContext.gpuContext;

		m_LineVertexBuffer->Clear();
		int32 const numLines = commands.Count();
		if ( numLines > 0 )
		{
			// Fill the lines vertex buffer
			uint32 const numDrawCalls = Math::Ceil( (float) numLines / m_MaxLinesPerDrawCall);
			for ( auto i = 0u; i < numDrawCalls; i++ )
			{
				uint32 const drawRangeStart = i * m_MaxLinesPerDrawCall;
				uint32 const drawRangeEnd = drawRangeStart + Math::Min(numLines - i * m_MaxLinesPerDrawCall, m_MaxLinesPerDrawCall );
				uint32 const drawRangeLength = drawRangeEnd - drawRangeStart;
				uint32 const drawCommandMemorySize = drawRangeLength * sizeof(Gizmos::LineCommand);


				Gizmos::LineCommand* pData = m_LineVertexBuffer->WriteReserve<Gizmos::LineCommand>(drawRangeLength);
				Platform::MemoryCopy(pData, &commands[drawRangeStart], drawCommandMemorySize);

				m_LineVertexBuffer->Flush(gpuContext);

				gpuContext->BindVB(m_LineVertexBuffer->GetBuffer());
				gpuContext->Draw(0, drawRangeLength * 2);
			}
		}
	}

	void GizmosPass::DrawTriangles(RenderContext const& renderContext, List<Gizmos::TriangleCommand> const& commands)
	{
		//-------------------------------------------------------------------------
		GPUContext* gpuContext = renderContext.gpuContext;

		m_PrimitiveVertexBuffer->Clear();
		int32 const numTriangles = commands.Count();
		if ( numTriangles > 0 )
		{
			// Fill the lines vertex buffer
			uint32 const numDrawCalls = Math::Ceil( (float)numTriangles / m_MaxTrianglesPerDrawCall);
			for ( auto i = 0u; i < numDrawCalls; i++ )
			{
				uint32 const drawRangeStart = ( i * m_MaxTrianglesPerDrawCall );
				uint32 const drawRangeEnd = drawRangeStart + Math::Min( numTriangles - i * m_MaxTrianglesPerDrawCall, m_MaxTrianglesPerDrawCall);
				uint32 const drawRangeLength = ( drawRangeEnd - drawRangeStart );
				uint32 const drawCommandMemorySize = drawRangeLength * sizeof(Gizmos::TriangleCommand);

				Gizmos::TriangleCommand* pData = m_PrimitiveVertexBuffer->WriteReserve<Gizmos::TriangleCommand>(drawRangeLength);
				Platform::MemoryCopy(pData, &commands[drawRangeStart], drawCommandMemorySize);

				m_PrimitiveVertexBuffer->Flush(gpuContext);

				gpuContext->BindVB(m_PrimitiveVertexBuffer->GetBuffer());
				gpuContext->Draw(0, drawRangeLength * 3);
			}
		}
	}

	void GizmosPass::DrawText(RenderContext const& renderContext, List<Gizmos::TextCommand> const& commands, IntRange cmdRange)
	{
	}

	void GizmosPass::Draw(RenderContext& renderContext, GPUTexture* input)
	{
		if (!IsReady())
		{
			return;
		}

		PROFILE_GPU_CPU("Gizmos");

		GPUContext* gpuContext = renderContext.gpuContext;

		m_DrawCommands.Clear();
		GetFrameCommandBuffer(m_DrawCommands);

		GizmosViewData viewData;
		viewData.viewMatrix = renderContext.view.View;
		viewData.projectionMatrix = renderContext.view.Projection;
		viewData.viewProjectionMatrix = renderContext.view.MainViewProjection;
		viewData.viewport = renderContext.view.ScreenSize;
		viewData.cameraPosition = renderContext.view.WorldPosition;

		gpuContext->BindCB(0, m_ViewBuffer);
		gpuContext->SetRenderTarget(input->View());

		gpuContext->UpdateCB(m_ViewBuffer, &viewData);

		//-------------------------------------------------------------------------
		// Draw Points
		//-------------------------------------------------------------------------

		{
		    gpuContext->SetState(m_PointPSO);
		    DrawPoints( renderContext, m_DrawCommands.opaqueDepthOn.pointCommands );
		    DrawPoints( renderContext, m_DrawCommands.opaqueDepthOff.pointCommands );

		    //-------------------------------------------------------------------------

			gpuContext->SetState(m_PointDepthPSO);
		    DrawPoints( renderContext, m_DrawCommands.transparentDepthOn.pointCommands );
		    DrawPoints( renderContext, m_DrawCommands.transparentDepthOff.pointCommands );
		}

		//-------------------------------------------------------------------------
		// Draw Lines
		//-------------------------------------------------------------------------

		{
			gpuContext->SetState(m_LinerPSO);
		    DrawLines( renderContext, m_DrawCommands.opaqueDepthOn.lineCommands );
		    DrawLines( renderContext, m_DrawCommands.opaqueDepthOff.lineCommands );

		    //-------------------------------------------------------------------------
			gpuContext->SetState(m_LinerDepthPSO);
		    DrawLines( renderContext, m_DrawCommands.transparentDepthOn.lineCommands );
		    DrawLines( renderContext, m_DrawCommands.transparentDepthOff.lineCommands );
		}

		//-------------------------------------------------------------------------
		// Draw Primitives
		//-------------------------------------------------------------------------

		{
			gpuContext->SetState(m_PrimitivePSO);
		    DrawTriangles( renderContext, m_DrawCommands.opaqueDepthOn.triangleCommands );
		    DrawTriangles( renderContext, m_DrawCommands.opaqueDepthOff.triangleCommands );

		    //-------------------------------------------------------------------------
			gpuContext->SetState(m_PrimitiveDepthPSO);
		    DrawTriangles( renderContext, m_DrawCommands.transparentDepthOn.triangleCommands );
		    DrawTriangles( renderContext, m_DrawCommands.transparentDepthOff.triangleCommands );
		}

		//-------------------------------------------------------------------------
		// Draw text
		//-------------------------------------------------------------------------

		{
			/*gpuContext->SetState(m_PrimitivePSO);
		    auto textRenderfunc = [this] ( RenderContext const& renderContext, Viewport const& viewport, TVector<TextCommand> const& commands, IntRange cmdRange ) { DebugRenderer::DrawText( renderContext, viewport, commands, cmdRange ); };

		    renderContext.SetDepthTestMode( DepthTestMode::On );
		    DrawTextCommands( m_DrawCommands.opaqueDepthOn.m_textCommands, renderContext, viewport, textRenderfunc );
		    DrawTextCommands( m_DrawCommands.transparentDepthOn.m_textCommands, renderContext, viewport, textRenderfunc );

		    renderContext.SetDepthTestMode( DepthTestMode::Off );
		    DrawTextCommands( m_DrawCommands.opaqueDepthOff.m_textCommands, renderContext, viewport, textRenderfunc );
		    DrawTextCommands( m_DrawCommands.transparentDepthOff.m_textCommands, renderContext, viewport, textRenderfunc );#1#
		}
	}*/
} // SE
