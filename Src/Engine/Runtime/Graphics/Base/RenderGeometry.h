#pragma once

#include "VertexFactoryLayout.h"

namespace SE
{
	class GPUBuffer;

	/// <summary>
	/// RenderGeometry 中一个显式 Slot 的无所有权顶点缓冲引用。
	/// </summary>
	struct SE_API_RUNTIME RenderGeometryVertexBuffer
	{
		uint32 Slot = 0;
		GPUBuffer* Buffer = nullptr;
		uint32 Offset = 0;
	};

	/// <summary>
	/// 一次网格绘制所需的几何资源集合，不拥有 Layout 或 GPUBuffer 生命周期。
	/// </summary>
	struct SE_API_RUNTIME RenderGeometry
	{
		const VertexFactoryLayout* Layout = nullptr;
		List<RenderGeometryVertexBuffer> VertexBuffers;
		GPUBuffer* IndexBuffer = nullptr;
		uint32 IndexOffset = 0;
		PixelFormat IndexFormat = PixelFormat::Undefined;

		const RenderGeometryVertexBuffer* FindVertexBuffer(uint32 slot) const;
		bool Validate(bool indexed) const;
	};
}
