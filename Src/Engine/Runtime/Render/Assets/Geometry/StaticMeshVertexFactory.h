#pragma once

#include "Runtime/Graphics/Base/GPUBufferDescription.h"
#include "Runtime/Graphics/Base/RenderGeometry.h"

namespace SE
{
	/// <summary>
	/// Static Mesh 固定顶点流的逻辑分类。
	/// </summary>
	enum class StaticMeshVertexStream : byte
	{
		Position = 0,
		Attributes = 1,
		Color = 2,
	};

	/// <summary>
	/// 集中描述 Static Mesh 的物理顶点布局，并把 Mesh Buffer 映射为 RenderGeometry。
	/// </summary>
	class SE_API_RUNTIME StaticMeshVertexFactory final
	{
	public:
		static constexpr uint32 PositionSlot = 0;
		static constexpr uint32 AttributesSlot = 1;
		static constexpr uint32 ColorSlot = 2;

		/// <summary>
		/// 获取所有 Static Mesh 实例共享的不可变物理布局。
		/// </summary>
		static const VertexFactoryLayout& GetLayout();

		/// <summary>
		/// 根据共享布局创建指定顶点流的 GPU Buffer 描述。
		/// </summary>
		static GPUBufferDescription CreateVertexBufferDescription(
			StaticMeshVertexStream stream,
			uint32 vertexCount,
			const void* data);

		/// <summary>
		/// 将实际存在的 Static Mesh 顶点 Buffer 组织为显式 Slot 的 RenderGeometry。
		/// </summary>
		static RenderGeometry CreateRenderGeometry(
			GPUBuffer* positionBuffer,
			GPUBuffer* attributesBuffer,
			GPUBuffer* colorBuffer);

	private:
		StaticMeshVertexFactory() = delete;

		static VertexFactoryLayout CreateLayout();
		static uint32 GetSlot(StaticMeshVertexStream stream);
	};
}
