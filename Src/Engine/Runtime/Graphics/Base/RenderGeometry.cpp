#include "RenderGeometry.h"
#include "GPUBuffer.h"
#include "Runtime/Graphics/GlobalSettings_GPU.h"

namespace SE
{
	const RenderGeometryVertexBuffer* RenderGeometry::FindVertexBuffer(const uint32 slot) const
	{
		for (int32 index = 0; index < VertexBuffers.Count(); index++)
		{
			if (VertexBuffers[index].Slot == slot)
			{
				return &VertexBuffers[index];
			}
		}
		return nullptr;
	}

	bool RenderGeometry::Validate(const bool indexed) const
	{
		if (Layout == nullptr || !Layout->IsValid())
		{
            LOG_ERROR("Graphics", "RenderGeometry requires a valid VertexFactory layout.");
			return false;
		}
		if (VertexBuffers.Count() > GPU_MAX_VB_BINDED)
		{
            LOG_ERROR("Graphics", "RenderGeometry has too many vertex buffers. Maximum is {0}.", GPU_MAX_VB_BINDED);
			return false;
		}

		for (int32 bufferIndex = 0; bufferIndex < VertexBuffers.Count(); bufferIndex++)
		{
			const RenderGeometryVertexBuffer& vertexBuffer = VertexBuffers[bufferIndex];
			const VertexFactoryBufferBinding* binding = Layout->FindBinding(vertexBuffer.Slot);
			if (vertexBuffer.Slot >= GPU_MAX_VB_BINDED || binding == nullptr)
			{
                LOG_ERROR("Graphics", "RenderGeometry vertex buffer references unknown slot {0}.", vertexBuffer.Slot);
				return false;
			}
			if (vertexBuffer.Buffer == nullptr || !vertexBuffer.Buffer->IsAllocated())
			{
                LOG_ERROR("Graphics", "RenderGeometry vertex buffer slot {0} is null or unallocated.", vertexBuffer.Slot);
				return false;
			}
			if (!EnumHasAllFlags(vertexBuffer.Buffer->GetFlags(), GPUBufferFlags::VertexBuffer))
			{
                LOG_ERROR("Graphics", "RenderGeometry buffer at slot {0} is not a vertex buffer.", vertexBuffer.Slot);
				return false;
			}
			if (vertexBuffer.Buffer->GetStride() != 0 && vertexBuffer.Buffer->GetStride() != binding->Stride)
			{
                LOG_ERROR("Graphics", "RenderGeometry vertex buffer stride does not match slot {0} layout.", vertexBuffer.Slot);
				return false;
			}
			if (vertexBuffer.Offset > vertexBuffer.Buffer->GetSize())
			{
                LOG_ERROR("Graphics", "RenderGeometry vertex buffer offset exceeds slot {0} buffer size.", vertexBuffer.Slot);
				return false;
			}
			for (int32 previousIndex = 0; previousIndex < bufferIndex; previousIndex++)
			{
				if (VertexBuffers[previousIndex].Slot == vertexBuffer.Slot)
				{
                    LOG_ERROR("Graphics", "RenderGeometry contains duplicate vertex buffer slot {0}.", vertexBuffer.Slot);
					return false;
				}
			}
		}

		if (IndexBuffer == nullptr)
		{
			if (indexed)
			{
                LOG_ERROR("Graphics", "Indexed RenderGeometry requires an index buffer.");
				return false;
			}
			if (IndexOffset != 0 || IndexFormat != PixelFormat::Undefined)
			{
                LOG_ERROR("Graphics", "RenderGeometry without an index buffer must use zero offset and undefined format.");
				return false;
			}
			return true;
		}

		// 即使当前 Draw 非索引，只要携带了 IndexBuffer，它也必须形成自洽描述。
		if (!IndexBuffer->IsAllocated() || !EnumHasAllFlags(IndexBuffer->GetFlags(), GPUBufferFlags::IndexBuffer))
		{
            LOG_ERROR("Graphics", "RenderGeometry index buffer is unallocated or lacks the index-buffer flag.");
			return false;
		}
		if (IndexFormat != PixelFormat::R16_UInt && IndexFormat != PixelFormat::R32_UInt)
		{
            LOG_ERROR("Graphics", "RenderGeometry index format must be R16_UInt or R32_UInt.");
			return false;
		}
		if (IndexBuffer->GetFormat() != PixelFormat::Undefined && IndexBuffer->GetFormat() != IndexFormat)
		{
            LOG_ERROR("Graphics", "RenderGeometry index format does not match the index buffer format.");
			return false;
		}

		const uint32 indexSize = PixelFormatGetSizeInBytes(IndexFormat);
		if ((IndexOffset % indexSize) != 0 || IndexOffset > IndexBuffer->GetSize())
		{
            LOG_ERROR("Graphics", "RenderGeometry index offset is unaligned or exceeds the index buffer size.");
			return false;
		}
		return true;
	}
}
