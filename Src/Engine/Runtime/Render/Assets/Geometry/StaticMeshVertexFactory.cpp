#include "StaticMeshVertexFactory.h"

#include "Runtime/Core/Math/Vector4.h"
#include "MeshDataLayout.h"
#include "Runtime/Core/Platform/Compiler.h"

namespace SE
{
	static_assert(sizeof(VB0ElementType) == sizeof(Float3));
	static_assert(OFFSET_OF(VB0ElementType, Position) == 0);
	static_assert(sizeof(Float3) == PixelFormatGetSizeInBytes(PixelFormat::R32G32B32_Float));

	static_assert(OFFSET_OF(VB1ElementType, TexCoord) == 0);
	static_assert(OFFSET_OF(VB1ElementType, Normal) == sizeof(Half2));
	static_assert(OFFSET_OF(VB1ElementType, Tangent) == sizeof(Half2) + sizeof(Float1010102));
	static_assert(OFFSET_OF(VB1ElementType, LightmapUVs) == sizeof(Half2) + sizeof(Float1010102) * 2);
	static_assert(sizeof(VB1ElementType) == sizeof(Half2) * 2 + sizeof(Float1010102) * 2);
	static_assert(sizeof(Half2) == PixelFormatGetSizeInBytes(PixelFormat::R16G16_Float));
	static_assert(sizeof(Float1010102) == PixelFormatGetSizeInBytes(PixelFormat::R10G10B10A2_UNorm));

	static_assert(sizeof(VB2ElementType) == sizeof(Color32));
	static_assert(OFFSET_OF(VB2ElementType, Color) == 0);
	static_assert(sizeof(Color32) == PixelFormatGetSizeInBytes(PixelFormat::R8G8B8A8_UNorm));

	static_assert(OFFSET_OF(StaticMeshInstanceData, InstanceOrigin) == 0);
	static_assert(OFFSET_OF(StaticMeshInstanceData, PerInstanceRandom) == sizeof(Float3));
	static_assert(OFFSET_OF(StaticMeshInstanceData, InstanceTransform1) == sizeof(Float4));
	static_assert(OFFSET_OF(StaticMeshInstanceData, LODDitherFactor) == sizeof(Float4) + sizeof(Float3));
	static_assert(OFFSET_OF(StaticMeshInstanceData, InstanceTransform2) == sizeof(Float4) * 2);
	static_assert(OFFSET_OF(StaticMeshInstanceData, InstanceTransform3) == sizeof(Float4) * 2 + sizeof(Float3));
	static_assert(OFFSET_OF(StaticMeshInstanceData, InstanceLightmapArea) == sizeof(Float4) * 2 + sizeof(Float3) * 2);
	static_assert(sizeof(StaticMeshInstanceData) == sizeof(Float4) * 2 + sizeof(Float3) * 2 + sizeof(Half4));

	const VertexFactoryLayout& StaticMeshVertexFactory::GetLayout()
	{
		// 函数局部静态对象保证所有 Static Mesh 共享同一份布局事实。
		static const VertexFactoryLayout Layout = CreateLayout();
		return Layout;
	}

	GPUBufferDescription StaticMeshVertexFactory::CreateVertexBufferDescription(
		const StaticMeshVertexStream stream,
		const uint32 vertexCount,
		const void* data)
	{
		const VertexFactoryBufferBinding* binding = GetLayout().FindBinding(GetSlot(stream));
		ENGINE_ASSERT(binding != nullptr);
		return GPUBufferDescription::Vertex(binding->Stride, vertexCount, data);
	}

	RenderGeometry StaticMeshVertexFactory::CreateRenderGeometry(
		GPUBuffer* positionBuffer,
		GPUBuffer* attributesBuffer,
		GPUBuffer* colorBuffer)
	{
		RenderGeometry geometry;
		geometry.Layout = &GetLayout();

		// 可选流不创建占位 Buffer，RenderGeometry 仅记录实际可绑定资源。
		if (positionBuffer != nullptr)
		{
			geometry.VertexBuffers.Add({PositionSlot, positionBuffer, 0});
		}
		if (attributesBuffer != nullptr)
		{
			geometry.VertexBuffers.Add({AttributesSlot, attributesBuffer, 0});
		}
		if (colorBuffer != nullptr)
		{
			geometry.VertexBuffers.Add({ColorSlot, colorBuffer, 0});
		}

		return geometry;
	}

	bool StaticMeshVertexFactory::CreateRenderGeometryWithInstanceBuffer(
		const RenderGeometry& baseGeometry,
		GPUBuffer* instanceBuffer,
		const uint32 instanceBufferOffset,
		RenderGeometry& output)
	{
		if (baseGeometry.Layout != &GetLayout())
		{
			LOG_ERROR("Graphics", "StaticMesh instance geometry requires the shared StaticMesh VertexFactory layout.");
			return false;
		}
		if (instanceBuffer == nullptr)
		{
			LOG_ERROR("Graphics", "StaticMesh instance geometry requires an instance buffer at slot {0}.", InstanceSlot);
			return false;
		}
		if ((instanceBufferOffset % sizeof(StaticMeshInstanceData)) != 0)
		{
			LOG_ERROR("Graphics", "StaticMesh instance buffer offset {0} is not aligned to instance stride {1}.",
				instanceBufferOffset, sizeof(StaticMeshInstanceData));
			return false;
		}
		if (baseGeometry.FindVertexBuffer(InstanceSlot) != nullptr)
		{
			LOG_ERROR("Graphics", "StaticMesh base geometry already contains instance slot {0}.", InstanceSlot);
			return false;
		}

		// RenderGeometry 是通用值类型；实例批次只复制并附加一个显式 Slot。
		RenderGeometry candidate = baseGeometry;
		candidate.VertexBuffers.Add({InstanceSlot, instanceBuffer, instanceBufferOffset});
		output = MoveTemp(candidate);
		return true;
	}

	VertexFactoryLayout StaticMeshVertexFactory::CreateLayout()
	{
		const List<VertexFactoryBufferBinding> bindings = {
			{PositionSlot, sizeof(VB0ElementType), VertexFactoryInputRate::PerVertex, 0},
			{AttributesSlot, sizeof(VB1ElementType), VertexFactoryInputRate::PerVertex, 0},
			{ColorSlot, sizeof(VB2ElementType), VertexFactoryInputRate::PerVertex, 0},
			{InstanceSlot, sizeof(StaticMeshInstanceData), VertexFactoryInputRate::PerInstance, 1},
		};
		const List<VertexFactoryInputElement> elements = {
			{SE_TEXT("POSITION"), 0, PixelFormat::R32G32B32_Float, PositionSlot, OFFSET_OF(VB0ElementType, Position)},
			{SE_TEXT("TEXCOORD"), 0, PixelFormat::R16G16_Float, AttributesSlot, OFFSET_OF(VB1ElementType, TexCoord)},
			{SE_TEXT("NORMAL"), 0, PixelFormat::R10G10B10A2_UNorm, AttributesSlot, OFFSET_OF(VB1ElementType, Normal)},
			{SE_TEXT("TANGENT"), 0, PixelFormat::R10G10B10A2_UNorm, AttributesSlot, OFFSET_OF(VB1ElementType, Tangent)},
			{SE_TEXT("TEXCOORD"), 1, PixelFormat::R16G16_Float, AttributesSlot, OFFSET_OF(VB1ElementType, LightmapUVs)},
			{SE_TEXT("COLOR"), 0, PixelFormat::R8G8B8A8_UNorm, ColorSlot, OFFSET_OF(VB2ElementType, Color)},
			{SE_TEXT("ATTRIBUTE"), 0, PixelFormat::R32G32B32A32_Float, InstanceSlot, OFFSET_OF(StaticMeshInstanceData, InstanceOrigin)},
			{SE_TEXT("ATTRIBUTE"), 1, PixelFormat::R32G32B32A32_Float, InstanceSlot, OFFSET_OF(StaticMeshInstanceData, InstanceTransform1)},
			{SE_TEXT("ATTRIBUTE"), 2, PixelFormat::R32G32B32_Float, InstanceSlot, OFFSET_OF(StaticMeshInstanceData, InstanceTransform2)},
			{SE_TEXT("ATTRIBUTE"), 3, PixelFormat::R32G32B32_Float, InstanceSlot, OFFSET_OF(StaticMeshInstanceData, InstanceTransform3)},
			{SE_TEXT("ATTRIBUTE"), 4, PixelFormat::R16G16B16A16_Float, InstanceSlot, OFFSET_OF(StaticMeshInstanceData, InstanceLightmapArea)},
		};

		VertexFactoryLayout layout;
        if (!VertexFactoryLayout::Create(bindings, elements, layout))
		{
			// 固定描述若无法构造，说明 MeshDataLayout 与 VertexFactory 契约已经失配。
			ENGINE_ASSERT(false);
		}
		return layout;
	}

	uint32 StaticMeshVertexFactory::GetSlot(const StaticMeshVertexStream stream)
	{
		switch (stream)
		{
		case StaticMeshVertexStream::Position:
			return PositionSlot;
		case StaticMeshVertexStream::Attributes:
			return AttributesSlot;
		case StaticMeshVertexStream::Color:
			return ColorSlot;
		case StaticMeshVertexStream::Instance:
			return InstanceSlot;
		default:
			ENGINE_ASSERT(false);
			return PositionSlot;
		}
	}
}
