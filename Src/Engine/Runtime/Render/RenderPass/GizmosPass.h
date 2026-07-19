#pragma once

#include "Runtime/Core/Math/NumericRange.h"
#include "Runtime/Graphics/Base/GPUPipelineState.h"
#include "Runtime/Render/RendererPass.h"
#include "Runtime/Resource/AssetRef.h"
#include "Runtime/Resource/Assets/Materials/Shader.h"

namespace SE
{
	class GPUConstantBuffer;
}

namespace SE
{
	class DynamicVertexBuffer;
	struct RenderContext;
	class GPUTexture;
	class GPUPipelineState;

	/*struct GizmosViewData
	{
		Matrix  viewMatrix;
		Matrix  projectionMatrix;
		Matrix  viewProjectionMatrix;
		Float4  viewport;
		Float3  cameraPosition;
	};

	class GizmosPass final : public RendererPass<GizmosPass>
	{
	public:
		String ToString() const override;
		bool Init() override;
		void Dispose() override;

		void Draw(RenderContext& renderContext, GPUTexture* input);

	private:
		bool SetupResources() override;
		bool InitPso(GPUPipelineState::Description desc, GPUShader* shader,
			GPUPipelineState* & linerPso, GPUPipelineState* & pointPso, GPUPipelineState* & textPso, GPUPipelineState* & primitivePso);

		void DrawPoints( RenderContext const& renderContext, List<Gizmos::PointCommand> const& commands );
		void DrawLines( RenderContext const& renderContext, List<Gizmos::LineCommand> const& commands );
		void DrawTriangles( RenderContext const& renderContext, List<Gizmos::TriangleCommand> const& commands );
		void DrawText( RenderContext const& renderContext, List<Gizmos::TextCommand> const& commands, IntRange cmdRange );

		AssetRef<Shader> m_Shader;
		GPUPipelineState* m_LinerDepthPSO;
		GPUPipelineState* m_LinerPSO;
		DynamicVertexBuffer* m_LineVertexBuffer;

		GPUPipelineState* m_PointDepthPSO;
		GPUPipelineState* m_PointPSO;
		DynamicVertexBuffer* m_PointVertexBuffer;

		GPUPipelineState* m_TextDepthPSO;
		GPUPipelineState* m_TextPSO;

		GPUPipelineState* m_PrimitiveDepthPSO;
		GPUPipelineState* m_PrimitivePSO;
		DynamicVertexBuffer* m_PrimitiveVertexBuffer;

		GPUConstantBuffer* m_ViewBuffer;

		Gizmos::FrameCommandBuffer m_DrawCommands;
		constexpr static uint32 const m_MaxPointsPerDrawCall = 100000;
		constexpr static uint32 const m_MaxLinesPerDrawCall = 100000;
		constexpr static uint32 const m_MaxTrianglesPerDrawCall = 100000;

	};*/

} // SE

