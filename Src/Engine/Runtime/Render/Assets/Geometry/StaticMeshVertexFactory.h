#pragma once

#include "Runtime/Core/Math/Half.h"
#include "Runtime/Graphics/Base/GPUBufferDescription.h"
#include "Runtime/Graphics/Base/RenderGeometry.h"

namespace SE
{
	/// <summary>
	/// Static Mesh 硬件实例化使用的单条 GPU 顶点流记录。
	/// 字段顺序属于 VertexFactory 物理布局契约，修改时必须同步布局断言。
	/// </summary>
	GES_PACK_STRUCT(struct SE_API_RUNTIME StaticMeshInstanceData
	{
		Float3 InstanceOrigin;
		float PerInstanceRandom;
		Float3 InstanceTransform1;
		float LODDitherFactor;
		Float3 InstanceTransform2;
		Float3 InstanceTransform3;
		Half4 InstanceLightmapArea;
	});

	/// <summary>
	/// Static Mesh 固定顶点流的逻辑分类。
	/// </summary>
	enum class StaticMeshVertexStream : byte
	{
		Position = 0,
		Attributes = 1,
		Color = 2,
		Instance = 3,
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
		static constexpr uint32 InstanceSlot = 3;

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

		/// <summary>
		/// 复制基础 Geometry 并在副本中附加可选实例流，不修改 Mesh 持有的基础 Geometry。
		/// </summary>
		static bool CreateRenderGeometryWithInstanceBuffer(
			const RenderGeometry& baseGeometry,
			GPUBuffer* instanceBuffer,
			uint32 instanceBufferOffset,
			RenderGeometry& output);

	private:
		StaticMeshVertexFactory() = delete;

		static VertexFactoryLayout CreateLayout();
		static uint32 GetSlot(StaticMeshVertexStream stream);
	};
}
